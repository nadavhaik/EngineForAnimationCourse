get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

message(STATUS "Creating target: ${PROJECT_NAME}")

project(${PROJECT_NAME})
include(FetchContent)
set(BUILD_SHARED_LIBS OFF)
FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.5.1
)
FetchContent_MakeAvailable(SFML)
find_package(OpenAL REQUIRED)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}) # set the binary directory to build folder
file(GLOB ${PROJECT_NAME}_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h*)
add_executable(${PROJECT_NAME}_bin ${${PROJECT_NAME}_SOURCES} PeriodicExecutor.cpp PeriodicExecutor.h MovingObject.cpp MovingObject.h types_macros.h BoundableModel.cpp BoundableModel.h functionals.h SoundManager.h SoundHandler.h)
target_link_libraries(${PROJECT_NAME}_bin engine igl::core igl::opengl igl::opengl_glfw igl::opengl_glfw_imgui igl::png sfml-audio)
set_target_properties(${PROJECT_NAME}_bin PROPERTIES CXX_STANDARD 20)
set_property(TARGET ${PROJECT_NAME}_bin PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$(SolutionDir)")
