cmake_minimum_required(VERSION 3.8)
project(test_package C CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(esperantoTrace REQUIRED)

add_executable(${PROJECT_NAME}_c test_package.c)
target_link_libraries(${PROJECT_NAME}_c PRIVATE esperantoTrace::et_trace)

add_executable(${PROJECT_NAME}_cpp test_package.cpp)
target_link_libraries(${PROJECT_NAME}_cpp PRIVATE esperantoTrace::et_trace)
