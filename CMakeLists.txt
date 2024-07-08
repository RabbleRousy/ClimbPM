cmake_minimum_required(VERSION 3.28)
project(ClimbPM)

set(CMAKE_CXX_STANDARD 17)

# OpenCV_DIR has to be set as system environment variable.
message(STATUS "OpenCV_DIR is " $ENV{OpenCV_DIR})
set(OpenCV_DIR $ENV{OpenCV_DIR})
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui structured_light)

include_directories(${OpenCV_INCLUDE_DIRS}) # not needed for opencv>=4.0
add_executable(ClimbPM main.cpp
        ProjectorConfig.cpp
        ProjectorConfig.h)
target_link_libraries(ClimbPM ${OpenCV_LIBS})