project( stb_image )

add_library(${PROJECT_NAME} STATIC "stb_image.cpp")
set_target_properties( ${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON )
target_include_directories( ${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../stb" )
target_link_libraries( ${PROJECT_NAME} )

