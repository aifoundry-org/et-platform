cmake_minimum_required(VERSION 3.20)

project(blas-optimized-host LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ET_PLATFORM_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.." CACHE PATH "Path to the et-platform source tree")
set(BUILD_TESTS OFF CACHE BOOL "Build GP-SDK example launchers" FORCE)

list(APPEND CMAKE_MODULE_PATH /opt/et/lib/cmake /opt/et/lib/cmake/cmake-modules)
list(APPEND CMAKE_PREFIX_PATH /opt/et/lib/cmake /opt/et/lib/cmake/cmake-modules)

add_subdirectory(${ET_PLATFORM_SOURCE_DIR}/gp-sdk/host gp-sdk-host)

add_executable(blas_optimized_sdot_verifier sdot_verifier.cpp)
target_link_libraries(blas_optimized_sdot_verifier PRIVATE etsoc_gpsdk)

add_executable(blas_optimized_proof_of_life_verifier proof_of_life_verifier.cpp)
target_link_libraries(blas_optimized_proof_of_life_verifier PRIVATE etsoc_gpsdk)

install(TARGETS
  blas_optimized_proof_of_life_verifier
  blas_optimized_sdot_verifier
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)
