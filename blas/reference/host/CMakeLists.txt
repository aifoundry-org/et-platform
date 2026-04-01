cmake_minimum_required(VERSION 3.20)

project(blas-reference-host LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ET_PLATFORM_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.." CACHE PATH "Path to the et-platform source tree")
set(BUILD_TESTS OFF CACHE BOOL "Build GP-SDK example launchers" FORCE)

list(APPEND CMAKE_MODULE_PATH /opt/et/lib/cmake /opt/et/lib/cmake/cmake-modules)
list(APPEND CMAKE_PREFIX_PATH /opt/et/lib/cmake /opt/et/lib/cmake/cmake-modules)

add_subdirectory(${ET_PLATFORM_SOURCE_DIR}/gp-sdk/host gp-sdk-host)

add_executable(blas_reference_level1_verifier level1_verifier.cpp)
target_link_libraries(blas_reference_level1_verifier PRIVATE etsoc_gpsdk)

add_executable(blas_reference_level2_verifier level2_verifier.cpp)
target_link_libraries(blas_reference_level2_verifier PRIVATE etsoc_gpsdk)

add_executable(blas_reference_level3_verifier level3_verifier.cpp)
target_link_libraries(blas_reference_level3_verifier PRIVATE etsoc_gpsdk)

add_executable(blas_reference_full_verifier full_verifier.cpp)

install(TARGETS
  blas_reference_level1_verifier
  blas_reference_level2_verifier
  blas_reference_level3_verifier
  blas_reference_full_verifier
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)
