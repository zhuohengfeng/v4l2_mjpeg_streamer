cmake_minimum_required(VERSION 3.10)

project(v4l2_mjpeg_stream)

set(CMAKE_CXX_STANDARD 11)

include_directories(${CMAKE_SOURCE_DIR}/external)

add_executable(v4l2_mjpeg_stream main.cpp)
target_link_libraries(v4l2_mjpeg_stream v4l2 pthread)

