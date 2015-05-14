cmake_minimum_required(VERSION 2.8)
project(deepflite)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

find_package(Boost COMPONENTS thread program_options date_time REQUIRED)
find_package(Eigen REQUIRED)
find_package(OpenCV REQUIRED)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(AprilTags_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/ext/apriltags)

add_subdirectory(ext)
add_subdirectory(src)