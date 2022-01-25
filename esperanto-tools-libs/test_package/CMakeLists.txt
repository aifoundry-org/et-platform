cmake_minimum_required(VERSION 3.8)
project(test_package LANGUAGES CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(runtime REQUIRED)

add_executable(${PROJECT_NAME}_static test_package.cpp)
target_compile_features(${PROJECT_NAME}_static PRIVATE cxx_std_17)
target_link_libraries(${PROJECT_NAME}_static PRIVATE runtime::etrt_static)

add_executable(${PROJECT_NAME}_shared test_package.cpp)
target_compile_features(${PROJECT_NAME}_shared PRIVATE cxx_std_17)
target_link_libraries(${PROJECT_NAME}_shared PRIVATE runtime::etrt)