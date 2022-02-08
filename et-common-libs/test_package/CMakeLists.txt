cmake_minimum_required(VERSION 3.8)
project(test_package C CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

find_package(et-common-libs REQUIRED COMPONENTS cm-umode)

add_executable(${PROJECT_NAME}_c test_package.c)
target_link_libraries(${PROJECT_NAME}_c PRIVATE et-common-libs::cm-umode)

# Enable once this error is fixed:
# include/etsoc/common/log_common.h:14:6: error: using typedef-name 'log_interface_t' after 'enum'
#
# add_executable(${PROJECT_NAME}_cpp test_package.cpp)
# target_link_libraries(${PROJECT_NAME}_cpp PRIVATE et-common-libs::cm-umode)
