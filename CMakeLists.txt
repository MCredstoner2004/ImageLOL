cmake_minimum_required(VERSION 3.10)

project(ImageLOL VERSION 0.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory("External/stb_image-cmake")

add_executable(ImageLOL ImageLOL.cpp)
target_link_libraries(ImageLOL PRIVATE stb_image)
