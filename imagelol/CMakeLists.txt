project(libimagelol)

add_library(${PROJECT_NAME} STATIC "ImageLOL.cpp")
set_target_properties( ${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON )
target_include_directories( ${PROJECT_NAME} PUBLIC "./" )
target_link_libraries( ${PROJECT_NAME} )
