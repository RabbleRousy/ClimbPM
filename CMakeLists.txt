cmake_minimum_required(VERSION 3.28)
project(ClimbPM)

set(CMAKE_CXX_STANDARD 17)

message(STATUS "Include path is: $ENV{OPENCV_INCLUDE_PATHS}")
include_directories($ENV{OPENCV_INCLUDE_PATHS})
message(STATUS "Link path is: $ENV{OPENCV_LINK_PATHS}")
link_directories($ENV{OPENCV_LINK_PATHS})

add_executable(ClimbPM main.cpp)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Linking debug libraries: $ENV{OPENCV_LINK_LIBS_D}")
    target_link_libraries(ClimbPM $ENV{OPENCV_LINK_LIBS_D})
else ()
    message(STATUS "Linking release libraries: $ENV{OPENCV_LINK_LIBS}")
    target_link_libraries(ClimbPM $ENV{OPENCV_LINK_LIBS})
endif()