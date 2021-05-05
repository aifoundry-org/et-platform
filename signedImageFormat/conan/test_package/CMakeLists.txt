cmake_minimum_required(VERSION 3.1)
project(test_package LANGUAGES C CXX)

include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
conan_basic_setup(TARGETS)

find_package(signedImageFormat REQUIRED)

# signedImageFormat must be capable of be used in C/C++ projects .. so test both

add_executable(${PROJECT_NAME}-c test_package.c)
set_property(TARGET ${PROJECT_NAME}-c PROPERTY C_STANDARD 11)
target_link_libraries(${PROJECT_NAME}-c signedImageFormat::signedImageFormat)

add_executable(${PROJECT_NAME}-cpp test_package.cpp)
set_property(TARGET ${PROJECT_NAME}-cpp PROPERTY CXX_STANDARD 11)
target_link_libraries(${PROJECT_NAME}-cpp signedImageFormat::signedImageFormat)