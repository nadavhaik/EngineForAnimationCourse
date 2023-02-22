# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/3D animation/project/new/project-main/external/imguizmo"
  "D:/3D animation/project/new/project-main/cmake-build-debug/imguizmo-build"
  "D:/3D animation/project/new/project-main/external/.cache/imguizmo/imguizmo-download-prefix"
  "D:/3D animation/project/new/project-main/external/.cache/imguizmo/imguizmo-download-prefix/tmp"
  "D:/3D animation/project/new/project-main/external/.cache/imguizmo/imguizmo-download-prefix/src/imguizmo-download-stamp"
  "D:/3D animation/project/new/project-main/external/.cache/imguizmo/imguizmo-download-prefix/src"
  "D:/3D animation/project/new/project-main/external/.cache/imguizmo/imguizmo-download-prefix/src/imguizmo-download-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/3D animation/project/new/project-main/external/.cache/imguizmo/imguizmo-download-prefix/src/imguizmo-download-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/3D animation/project/new/project-main/external/.cache/imguizmo/imguizmo-download-prefix/src/imguizmo-download-stamp${cfgdir}") # cfgdir has leading slash
endif()
