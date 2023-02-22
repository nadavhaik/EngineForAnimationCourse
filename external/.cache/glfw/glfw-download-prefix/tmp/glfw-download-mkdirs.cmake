# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/3D animation/project/new/project-main/external/glfw"
  "D:/3D animation/project/new/project-main/cmake-build-debug/glfw-build"
  "D:/3D animation/project/new/project-main/external/.cache/glfw/glfw-download-prefix"
  "D:/3D animation/project/new/project-main/external/.cache/glfw/glfw-download-prefix/tmp"
  "D:/3D animation/project/new/project-main/external/.cache/glfw/glfw-download-prefix/src/glfw-download-stamp"
  "D:/3D animation/project/new/project-main/external/.cache/glfw/glfw-download-prefix/src"
  "D:/3D animation/project/new/project-main/external/.cache/glfw/glfw-download-prefix/src/glfw-download-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/3D animation/project/new/project-main/external/.cache/glfw/glfw-download-prefix/src/glfw-download-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/3D animation/project/new/project-main/external/.cache/glfw/glfw-download-prefix/src/glfw-download-stamp${cfgdir}") # cfgdir has leading slash
endif()
