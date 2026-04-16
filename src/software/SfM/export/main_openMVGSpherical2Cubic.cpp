// This file is part of OpenMVG, an Open Multiple View Geometry C++ library.

// Copyright (c) 2019 Pierre MOULON.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "openMVG/geometry/pose3.hpp"
#include "openMVG/image/image_io.hpp"
#include "openMVG/numeric/eigen_alias_definition.hpp"
#include "openMVG/sfm/sfm_data.hpp"
#include "openMVG/sfm/sfm_data_io.hpp"
#include "openMVG/sfm/sfm_landmark.hpp"
#include "openMVG/sfm/sfm_view.hpp"
#include "openMVG/spherical/cubic_image_sampler.hpp"
#include "openMVG/system/loggerprogress.hpp"
#include "openMVG/types.hpp"

#include "third_party/cmdLine/cmdLine.h"
#include "third_party/stlplus3/filesystemSimplified/file_system.hpp"

#include <array>
#include <cstdlib>
#include <cmath>
#include <iomanip>
using namespace openMVG;
using namespace openMVG::cameras;
using namespace openMVG::geometry;
using namespace openMVG::image;
using namespace openMVG::sfm;

int main(int argc, char *argv[]) {

  CmdLine cmd;
  std::string s_sfm_data_filename;
  std::string s_out_dir = "";
  std::string s_mode = "landscape"; // [新增] 模式，默认为 landscape
  int force_recompute_images = 1;
  int size_cubic_images_width = -1;  // [修改] 初始设为-1，便于后续动态分配
  int size_cubic_images_height = -1; // [修改] 初始设为-1，便于后续动态分配

  cmd.add( make_option('i', s_sfm_data_filename, "sfmdata") );
  cmd.add( make_option('o', s_out_dir, "outdir") );
  cmd.add( make_option('m', s_mode, "mode") ); // [新增]
  cmd.add( make_option('f', force_recompute_images, "force_compute_cubic_images") );
  cmd.add( make_option('w', size_cubic_images_width, "size-cubic-images-width") );
  cmd.add( make_option('h', size_cubic_images_height, "size-cubic-images-height") );

  try {
      if (argc == 1) throw std::string("Invalid command line parameter.");
      cmd.process(argc, argv);
  } catch (const std::string& s) {
      std::cerr << "Usage: " << argv[0] << '\n'
      << "[-i|--sfmdata] filename, the SfM_Data file to convert\n"
      << "[-o|--outdir path]\n"
      << "[-m|--mode] 'landscape' (12 views, default) or 'portrait' (8 views)\n"
      << "[-f|--force_recompute_images] (default 1)\n"
      << "[-w|--size-cubic-images] pixel size of the resulting cubic images width\n"
      << "[-h|--size-cubic-images] pixel size of the resulting cubic images height\n"
      << "non-positive values will automatically scale the output based on the mode"
      << std::endl;

      std::cerr << s << std::endl;
      return EXIT_FAILURE;
  }

  // [新增] 如果未通过命令行指定宽和高，则根据 mode 自动设定最优分辨率
  if (size_cubic_images_width <= 0 || size_cubic_images_height <= 0) {
      if (s_mode == "portrait") {
          size_cubic_images_width = 480;
          size_cubic_images_height = 640;
      } else {
          size_cubic_images_width = 640;
          size_cubic_images_height = 480;
      }
  }

  OPENMVG_LOG_INFO << "force_recompute_images = " << force_recompute_images;
  OPENMVG_LOG_INFO << "mode = " << s_mode;

  std::cout << "size_cubic_images = " << size_cubic_images_width << " " << size_cubic_images_height << std::endl;

  // Create output dir
  if (!stlplus::folder_exists(s_out_dir))
      stlplus::folder_create(s_out_dir);

  SfM_Data sfm_data;
  if (!Load(sfm_data, s_sfm_data_filename, ESfM_Data(ALL))) {
      OPENMVG_LOG_ERROR << "The input SfM_Data file \""
        << s_sfm_data_filename << "\" cannot be read.";
      return EXIT_FAILURE;
  }

  SfM_Data sfm_data_out; // the sfm_data that stores the cubical image list
  sfm_data_out.s_root_path = s_out_dir;

  // Convert every spherical view to cubic views
  {
    system::LoggerProgress my_progress_bar(
      sfm_data.GetViews().size(),
      "Generating perspective views:");
    const Views & views = sfm_data.GetViews();
    const Poses & poses = sfm_data.GetPoses();
    const Landmarks & structure = sfm_data.GetLandmarks();

    openMVG::cameras::Pinhole_Intrinsic pinhole_camera = 
        spherical::ComputeCubicCameraIntrinsics(size_cubic_images_width, size_cubic_images_height);

    std::cout << "pinhole camera focalxy: "<< pinhole_camera.focal().x() <<" "<<pinhole_camera.focal().y()<<"\n";

    int error_status = 0;
    #pragma omp parallel for shared(error_status)
    for (int i = 0; i < static_cast<int>(views.size()); ++i) {
        // #pragma omp atomic
        ++my_progress_bar;
        
        auto view_it = views.begin();
        std::advance(view_it, i);
        const View * view = view_it->second.get();
        if (!sfm_data.IsPoseAndIntrinsicDefined(view))
            continue;

        Intrinsics::const_iterator iter_intrinsic = sfm_data.GetIntrinsics().find(view->id_intrinsic);
        const IntrinsicBase * cam = iter_intrinsic->second.get();
        if (cam && cam->getType() == CAMERA_SPHERICAL) {
            
            const std::string view_path = stlplus::create_filespec(sfm_data.s_root_path, view->s_Img_path);
            image::Image<image::RGBColor> spherical_image;
            if (!ReadImage(view_path.c_str(), &spherical_image)) {
                std::cerr << "Cannot read the input panoramic image: " << view_path << std::endl;
                #pragma omp atomic
                ++error_status;
                continue;
            }

            Pinhole_Intrinsic local_pinhole_camera = pinhole_camera;

            // [修改] 获取动态的旋转矩阵和切分数量
            const std::vector<Mat3> rot_matrix = spherical::GetPerspectiveRotations(s_mode);
            const int num_splits = rot_matrix.size(); // 12 or 8

            // when cubical image computation is needed
            std::vector<image::Image<image::RGBColor>> cube_images;
            if (force_recompute_images) {
                std::vector<Mat3> rot_matrix_transposed;
                rot_matrix_transposed.reserve(num_splits);
                for (const auto& mat : rot_matrix) {
                    rot_matrix_transposed.push_back(mat.transpose());
                }

                spherical::SphericalToPinholes(
                    spherical_image,
                    local_pinhole_camera,
                    cube_images,
                    rot_matrix_transposed,
                    image::Sampler2d<image::SamplerCubic>());
            }

            for ( int cubic_image_id = 0; cubic_image_id < num_splits; cubic_image_id ++ ) {
                std::ostringstream os;
                os << std::setw(8) << std::setfill('0') << cubic_image_id;
                const std::string dst_cube_image = stlplus::create_filespec(
                    stlplus::folder_append_separator(s_out_dir),
                    stlplus::basename_part(view_path)
                    + "_perspective_"
                    + os.str(),
                    "jpg");
                    
                if (force_recompute_images) {
                    if (!WriteImage(dst_cube_image.c_str(), cube_images[cubic_image_id])) {
                        OPENMVG_LOG_ERROR << "Cannot export cubic images to: " << dst_cube_image;
                        #pragma omp atomic
                        ++error_status;
                        continue;
                    }
                }

                const View v(
                    stlplus::filename_part(dst_cube_image),
                    view->id_view * num_splits + cubic_image_id, // [修改] 动态 Id view
                    0,  // Id intrinsic
                    view->id_pose * num_splits + cubic_image_id,  // [修改] 动态 Id pose
                    local_pinhole_camera.w(),
                    local_pinhole_camera.h());
                    
                #pragma omp critical
                {
                    sfm_data_out.views[v.id_view] = std::make_shared<View>(v);
                }

                Mat3 tmp_rotation = poses.at(view->id_pose).rotation();
                if (tmp_rotation.determinant() < 0) {
                    OPENMVG_LOG_INFO << "Negative determinant";
                    tmp_rotation = tmp_rotation * (-1.0f);
                }

                #pragma omp critical
                {
                    sfm_data_out.poses[v.id_pose] =
                        Pose3(rot_matrix[cubic_image_id] * tmp_rotation,
                              poses.at(view->id_pose).center());

                    if (sfm_data_out.GetIntrinsics().count(v.id_intrinsic) == 0)
                        sfm_data_out.intrinsics[v.id_intrinsic] =
                            std::make_shared<Pinhole_Intrinsic>(local_pinhole_camera);
                }
            }
        } else {
            OPENMVG_LOG_INFO << "Loaded scene does not have spherical camera";
            #pragma omp atomic
            ++error_status;
            continue;
        }
    }

  if (error_status > 0) // early exit
    return EXIT_FAILURE;

  // generate structure and associate it with new camera views
  {
    // [新增] 获取切分数量，供下方的 View ID 计算使用
    int num_splits = (s_mode == "portrait") ? 8 : 12;

    system::LoggerProgress my_progress_bar(structure.size(), "Creating cubic sfm_data structure:");
    for (const auto & it_structure : structure)
    {
      ++my_progress_bar;

      const Observations & obs = it_structure.second.obs;

      Landmark out_landmark;
      out_landmark.X = it_structure.second.X;
      bool found_any_reprojection = false;

      // iterate across 360 views that can see the point
      for(const auto & it_obs : obs)
      {
        const IndexT pano_view_key = it_obs.first;
        const IndexT feature_key   = it_obs.second.id_feat;

        bool is_reprojection_found = false;
        // [修改] 遍历该全景图切分出来的所有子图
        for (IndexT local_view_index = pano_view_key * num_splits; local_view_index < pano_view_key * num_splits + num_splits; ++local_view_index)
        {
          const IndexT intrinsic_id = sfm_data_out.views[local_view_index]->id_intrinsic;
          const IndexT extrinsic_id = sfm_data_out.views[local_view_index]->id_pose;
          const Pose3 pose = sfm_data_out.poses[extrinsic_id];
          const auto cam = sfm_data_out.intrinsics[intrinsic_id];

          const int image_height = cam->h();
          const int image_width  = cam->w();

          const Vec2 projection = cam->project(pose(it_structure.second.X));

          if (projection.x() < 0 || projection.x() >= image_width ||
              projection.y() < 0 || projection.y() >= image_height)
              continue;

          const Vec3 point_to_cam_dir = (it_structure.second.X - pose.center()).normalized();
          const Vec3 cam_looking_dir = (pose.rotation().transpose() * Vec3(0, 0, 1)).normalized();

          const double angle = R2D(acos(point_to_cam_dir.dot(cam_looking_dir)));

          if (angle < 0 || angle > 90)
            continue;

          out_landmark.obs[local_view_index] = Observation(projection, feature_key);
          is_reprojection_found = true;
          found_any_reprojection = true;
        }

        if (!is_reprojection_found) {
            std::cerr << "Warning: Could not find reprojection for feature " << feature_key
                      << " in pano view " << pano_view_key << " "<<sfm_data.views[pano_view_key]->s_Img_path<<std::endl;
        }
      }
      if (found_any_reprojection){
          sfm_data_out.structure.insert({it_structure.first, out_landmark});
      }
    }
  }

  } // end of converting spherical view to cubical

  if (!Save(sfm_data_out,
            stlplus::create_filespec(stlplus::folder_append_separator(s_out_dir),
                                     "sfm_data_perspective.bin"),
            ESfM_Data(ALL))) {
    OPENMVG_LOG_ERROR << std::endl
    << "Cannot save the output sfm_data file";
    return EXIT_FAILURE;
  }

  OPENMVG_LOG_INFO
    << " #views: " << sfm_data_out.views.size() << "\n"
    << " #poses: " << sfm_data_out.poses.size() << "\n"
    << " #intrinsics: " << sfm_data_out.intrinsics.at(0)->getParams().at(0)<<" "<<sfm_data_out.intrinsics.at(0)->getParams().at(1)
    <<" "<<sfm_data_out.intrinsics.at(0)->getParams().at(2)<<" "<<sfm_data_out.intrinsics.at(0)->getParams().at(3)<<"\n"
    << " #tracks: " << sfm_data_out.structure.size();

  // Exit program
  return EXIT_SUCCESS;
}