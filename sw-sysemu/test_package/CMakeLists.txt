cmake_minimum_required(VERSION 3.8)
project(test_package CXX)

find_package(sw-sysemu REQUIRED)

add_executable(${PROJECT_NAME} test_package.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_link_libraries(${PROJECT_NAME} PRIVATE sw-sysemu::sw-sysemu)
