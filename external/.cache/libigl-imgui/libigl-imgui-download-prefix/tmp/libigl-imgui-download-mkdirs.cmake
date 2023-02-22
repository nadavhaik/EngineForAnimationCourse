# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/3D animation/project/new/project-main/external/libigl-imgui"
  "D:/3D animation/project/new/project-main/cmake-build-debug/libigl-imgui-build"
  "D:/3D animation/project/new/project-main/external/.cache/libigl-imgui/libigl-imgui-download-prefix"
  "D:/3D animation/project/new/project-main/external/.cache/libigl-imgui/libigl-imgui-download-prefix/tmp"
  "D:/3D animation/project/new/project-main/external/.cache/libigl-imgui/libigl-imgui-download-prefix/src/libigl-imgui-download-stamp"
  "D:/3D animation/project/new/project-main/external/.cache/libigl-imgui/libigl-imgui-download-prefix/src"
  "D:/3D animation/project/new/project-main/external/.cache/libigl-imgui/libigl-imgui-download-prefix/src/libigl-imgui-download-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/3D animation/project/new/project-main/external/.cache/libigl-imgui/libigl-imgui-download-prefix/src/libigl-imgui-download-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/3D animation/project/new/project-main/external/.cache/libigl-imgui/libigl-imgui-download-prefix/src/libigl-imgui-download-stamp${cfgdir}") # cfgdir has leading slash
endif()
