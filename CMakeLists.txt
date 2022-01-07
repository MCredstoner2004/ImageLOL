cmake_minimum_required(VERSION 3.10)

project(ImageLOL VERSION 0.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_subdirectory("External/stb_image-cmake")

set(SKIP_INSTALL_ALL ON CACHE BOOL "")
add_subdirectory("External/zlib-no-shared-no-examples")


target_include_directories(zlibstatic PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/External/zlib")
get_target_property(ZLIB_INCLUDE_DIRECTORIES zlibstatic INCLUDE_DIRECTORIES)
set(PNG_BUILD_ZLIB ON CACHE INTERNAL "")
set(ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIRECTORIES} CACHE INTERNAL "")
set(ZLIB_INCLUDE_DIR ${ZLIB_INCLUDE_DIRECTORIES} CACHE INTERNAL "")
set(ZLIB_LIBRARIES zlibstatic CACHE INTERNAL "")
set(PNG_SHARED OFF CACHE INTERNAL "")
set(PNG_EXECUTABLES OFF CACHE INTERNAL "")
add_subdirectory("External/libpng")
target_include_directories(png_static PRIVATE ${ZLIB_INCLUDE_DIRS})
add_dependencies(genfiles zlibstatic)

add_subdirectory("External/argh")
unset(SKIP_INSTALL_ALL CACHE)
get_target_property(LIBPNG_INCLUDE_DIRECTORIES png_static INCLUDE_DIRECTORIES)

add_subdirectory("lib")
add_executable(imagelol main.cpp)
target_include_directories(imagelol PRIVATE ${LIBPNG_INCLUDE_DIRECTORIES})
target_link_libraries(imagelol PRIVATE stb_image png_static zlibstatic libimagelol argh)
