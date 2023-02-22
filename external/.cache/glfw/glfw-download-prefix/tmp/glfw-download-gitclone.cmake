# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

if(EXISTS "D:/3D animation/project/new/project-main/external/.cache/glfw/glfw-download-prefix/src/glfw-download-stamp/glfw-download-gitclone-lastrun.txt" AND EXISTS "D:/3D animation/project/new/project-main/external/.cache/glfw/glfw-download-prefix/src/glfw-download-stamp/glfw-download-gitinfo.txt" AND
  "D:/3D animation/project/new/project-main/external/.cache/glfw/glfw-download-prefix/src/glfw-download-stamp/glfw-download-gitclone-lastrun.txt" IS_NEWER_THAN "D:/3D animation/project/new/project-main/external/.cache/glfw/glfw-download-prefix/src/glfw-download-stamp/glfw-download-gitinfo.txt")
  message(STATUS
    "Avoiding repeated git clone, stamp file is up to date: "
    "'D:/3D animation/project/new/project-main/external/.cache/glfw/glfw-download-prefix/src/glfw-download-stamp/glfw-download-gitclone-lastrun.txt'"
  )
  return()
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E rm -rf "D:/3D animation/project/new/project-main/external/glfw"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to remove directory: 'D:/3D animation/project/new/project-main/external/glfw'")
endif()

# try the clone 3 times in case there is an odd git clone issue
set(error_code 1)
set(number_of_tries 0)
while(error_code AND number_of_tries LESS 3)
  execute_process(
    COMMAND "D:/Git/cmd/git.exe" -c http.sslVerify=false
            clone --no-checkout --config "advice.detachedHead=false" --config "advice.detachedHead=false" "https://github.com/glfw/glfw.git" "glfw"
    WORKING_DIRECTORY "D:/3D animation/project/new/project-main/external"
    RESULT_VARIABLE error_code
  )
  math(EXPR number_of_tries "${number_of_tries} + 1")
endwhile()
if(number_of_tries GREATER 1)
  message(STATUS "Had to git clone more than once: ${number_of_tries} times.")
endif()
if(error_code)
  message(FATAL_ERROR "Failed to clone repository: 'https://github.com/glfw/glfw.git'")
endif()

execute_process(
  COMMAND "D:/Git/cmd/git.exe" -c http.sslVerify=false
          checkout "3327050ca66ad34426a82c217c2d60ced61526b7" --
  WORKING_DIRECTORY "D:/3D animation/project/new/project-main/external/glfw"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to checkout tag: '3327050ca66ad34426a82c217c2d60ced61526b7'")
endif()

set(init_submodules TRUE)
if(init_submodules)
  execute_process(
    COMMAND "D:/Git/cmd/git.exe" -c http.sslVerify=false
            submodule update --recursive --init 
    WORKING_DIRECTORY "D:/3D animation/project/new/project-main/external/glfw"
    RESULT_VARIABLE error_code
  )
endif()
if(error_code)
  message(FATAL_ERROR "Failed to update submodules in: 'D:/3D animation/project/new/project-main/external/glfw'")
endif()

# Complete success, update the script-last-run stamp file:
#
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy "D:/3D animation/project/new/project-main/external/.cache/glfw/glfw-download-prefix/src/glfw-download-stamp/glfw-download-gitinfo.txt" "D:/3D animation/project/new/project-main/external/.cache/glfw/glfw-download-prefix/src/glfw-download-stamp/glfw-download-gitclone-lastrun.txt"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to copy script-last-run stamp file: 'D:/3D animation/project/new/project-main/external/.cache/glfw/glfw-download-prefix/src/glfw-download-stamp/glfw-download-gitclone-lastrun.txt'")
endif()
