cmake_minimum_required(VERSION 3.8)
project(test_package)

find_package(hostUtils REQUIRED)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(${PROJECT_NAME}_debug test_package_debug.cpp)
target_compile_features(${PROJECT_NAME}_debug PRIVATE cxx_std_17)
target_link_libraries(${PROJECT_NAME}_debug PRIVATE hostUtils::debug)

add_executable(${PROJECT_NAME}_logging test_package_logging.cpp)
target_compile_features(${PROJECT_NAME}_logging PRIVATE cxx_std_17)
target_link_libraries(${PROJECT_NAME}_logging PRIVATE hostUtils::logging)

add_executable(${PROJECT_NAME}_threadpool test_package_threadpool.cpp)
target_compile_features(${PROJECT_NAME}_threadpool PRIVATE cxx_std_17)
target_link_libraries(${PROJECT_NAME}_threadpool PRIVATE hostUtils::threadPool)

add_executable(${PROJECT_NAME}_all test_package_all.cpp)
target_compile_features(${PROJECT_NAME}_all PRIVATE cxx_std_17)
target_link_libraries(${PROJECT_NAME}_all PRIVATE hostUtils::hostUtils)
