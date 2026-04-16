# Install script for directory: /home/yuancaimaiyi/文档/openMVG/src/openMVG

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xheadersx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/openMVG" TYPE DIRECTORY FILES "/home/yuancaimaiyi/文档/openMVG/src/openMVG/." FILES_MATCHING REGEX "/[^/]*\\.hpp$" REGEX "/[^/]*\\.h$")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/cameras/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/clustering/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/exif/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/features/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/graph/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/graphics/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/image/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/linearProgramming/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/geodesy/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/geometry/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/matching/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/matching_image_collection/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/multiview/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/numeric/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/robust_estimation/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/tracks/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/color_harmonization/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/spherical/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/system/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/sfm/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/stl/cmake_install.cmake")
  include("/home/yuancaimaiyi/文档/openMVG/src/build/openMVG/vector_graphics/cmake_install.cmake")

endif()

