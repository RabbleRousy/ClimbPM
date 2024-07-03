cmake_minimum_required(VERSION 2.8.12)
project(ClimbPM)

# OpenCV_DIR has to be set as system environment variable.
message(STATUS "OpenCV_DIR is " $ENV{OpenCV_DIR})
set(OpenCV_DIR $ENV{OpenCV_DIR})
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui structured_light)

include_directories(${OpenCV_INCLUDE_DIRS}) # not needed for opencv>=4.0
add_executable(ClimbPM main.cpp)
target_link_libraries(ClimbPM ${OpenCV_LIBS})