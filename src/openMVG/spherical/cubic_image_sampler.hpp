// This file is part of OpenMVG, an Open Multiple View Geometry C++ library.

// Copyright (c) 2020 Pierre MOULON.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "openMVG/spherical/image_resampling.hpp"

#include <array>
#include <vector>
#include <string>

namespace openMVG
{
namespace spherical
{

/// Compute a rectilinear camera focal from a given angular desired FoV
double FocalFromPinholeHeight
(
  int h,
  double fov_radian = openMVG::D2R(45) // Camera FoV
)
{
  return h / (2 * tan(fov_radian));
}

// [修改] 将原来的 GetCubicRotations 替换为动态获取旋转矩阵的 GetPerspectiveRotations
const static std::vector<openMVG::Mat3> GetPerspectiveRotations(const std::string& mode)
{
  using namespace openMVG;
  std::vector<openMVG::Mat3> rotations;

  if (mode == "portrait") {
    // 【竖图模式】：水平每隔 45 度切一张，共 8 张，无需 Pitch (俯仰)
    std::vector<double> yaws = {0, -45, -90, -135, -180, -225, -270, -315};
    for (double y : yaws) {
        rotations.push_back(RotationAroundY(D2R(y)));
    }
  } else {
    // 【横图模式 (默认)】：4 个方向 * 3 个 Pitch 角，共 12 张
    std::vector<double> yaws = {0, -90, -180, -270};
    std::vector<double> pitches = {0, -30, 30};
    for (double y : yaws) {
        for (double p : pitches) {
            // 先俯仰(Pitch/X轴)，再偏航(Yaw/Y轴)
            rotations.push_back(RotationAroundX(D2R(p)) * RotationAroundY(D2R(y)));
        }
    }
  }
  return rotations;
}

openMVG::cameras::Pinhole_Intrinsic ComputeCubicCameraIntrinsics
(
  const int cubic_image_size_width,
  const int cubic_image_size_height,
  const double fov = D2R(45)
)
{
  // 只计算一次基准焦距，保证 fx == fy
  // 为了保证视野不被过度裁剪，我们用宽和高里面较大的那个值来推算焦距
  int max_side = std::max(cubic_image_size_width, cubic_image_size_height);
  const double focal = spherical::FocalFromPinholeHeight(max_side, fov);
  
  // 强行让 x 和 y 的焦距相等（正方形像素，消除拉伸畸变）
  const double focal_x = focal; 
  const double focal_y = focal; 
  
  const double principal_point_x = cubic_image_size_width / 2.0;
  const double principal_point_y = cubic_image_size_height / 2.0;
  
  return cameras::Pinhole_Intrinsic(cubic_image_size_width,
                                     cubic_image_size_height,
                                     focal_x,
                                     focal_y,
                                     principal_point_x,
                                     principal_point_y);
}

template <typename ImageT, typename SamplerT>
void SphericalToCubic
(
  const ImageT & equirectangular_image,
  const openMVG::cameras::Pinhole_Intrinsic & pinhole_camera,
  std::vector<ImageT> & cube_images,
  const std::string& mode = "landscape",
  const SamplerT sampler = image::Sampler2d<image::SamplerLinear>()
)
{
  // 适配新的动态函数
  const std::vector<Mat3> rot_matrix_vec = GetPerspectiveRotations(mode);
  SphericalToPinholes(
      equirectangular_image,
      pinhole_camera,
      cube_images,
      rot_matrix_vec,
      sampler);
}

} // namespace spherical
} // namespace openMVG