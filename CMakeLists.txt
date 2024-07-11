cmake_minimum_required(VERSION 3.28)
set(PROJECT_NAME ClimbPM)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)

# OpenCV_DIR has to be set as system environment variable.
message(STATUS "OpenCV_DIR is " $ENV{OpenCV_DIR})
set(OpenCV_DIR $ENV{OpenCV_DIR})
# Find OpenCV
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui structured_light)
# Find OpenGL
find_package(OpenGL REQUIRED)

# Include directory for GLFW & GLAD
message(STATUS "Include_DIR is " $ENV{Include_DIR})
set(GLOBAL_INCLUDE_DIR $ENV{Include_DIR})
# Including GLFW
add_subdirectory(${GLOBAL_INCLUDE_DIR}/glfw ${GLOBAL_INCLUDE_DIR}/glfw/bin)
include_directories(glfw/include)
# Include OpenCV
include_directories(${OpenCV_INCLUDE_DIRS}) # not needed for opencv>=4.0
# Including GLAD
include_directories(${GLOBAL_INCLUDE_DIR}/glad/include)

# Glad source files
set(GLAD_SOURCE ${GLOBAL_INCLUDE_DIR}/glad/src/glad.c)

add_executable(${PROJECT_NAME} main.cpp
        ProjectorConfig.cpp
        ProjectorConfig.h
        ${GLAD_SOURCE})

# Link OpenCV
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})
# Link GLFW and OPENGL
target_link_libraries(${PROJECT_NAME} glfw ${GLFW_LIBRARIES} ${OPENGL_LIBRARY})
# Public include directory
target_include_directories(${PROJECT_NAME} PUBLIC ${GLOBAL_INCLUDE_DIR})
