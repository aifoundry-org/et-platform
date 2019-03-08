#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8)

message(STATUS "CMAKE_CURRENT_SOURCE_DIR=" ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "CMAKE_CURRENT_BINARY_DIR=" ${CMAKE_CURRENT_BINARY_DIR})

include(ExternalProject)

# FIXME Remove external dependency to the SDK
ExternalProject_Add(sdk
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/sdk
  INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/sdk-install
  CMAKE_ARGS all
  INSTALL_COMMAND ""
  )
LINK_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/sdk)

option(BUILD_SHARED_LIBS "Build shared libs." ON)
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type selected, default to Debug")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type." FORCE)
endif()

set(kernels_elf_file "${CMAKE_CURRENT_BINARY_DIR}/net_test")
set(kernels_offsets_hdr "${CMAKE_CURRENT_BINARY_DIR}/kernels_offsets.h")
set(kernels_cdescr_file "${CMAKE_CURRENT_BINARY_DIR}/kernels_cdescr.inc")
set(kernels_symbol_name "gEtKernelsElf")

set(bootrom_file "${CMAKE_CURRENT_BINARY_DIR}/bootrom")
set(bootrom_cdescr_file "${CMAKE_CURRENT_BINARY_DIR}/bootrom_cdescr.inc")
set(bootrom_symbol_name "gEtBootrom")

set(GEN_KERNELS_OFFSETS_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_entry_points.sh")
set(GEN_KERNELS_CDESCR_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_cdescr.sh")

#message(STATUS "GEN_KERNELS_OFFSETS_SCRIPT=${GEN_KERNELS_OFFSETS_SCRIPT}")
#message(STATUS "GEN_KERNELS_CDESCR_SCRIPT=${GEN_KERNELS_CDESCR_SCRIPT}")

add_custom_command(OUTPUT ${kernels_offsets_hdr}
        COMMAND ${GEN_KERNELS_OFFSETS_SCRIPT} ${kernels_elf_file} ${kernels_offsets_hdr}
        DEPENDS ${GEN_KERNELS_OFFSETS_SCRIPT} ${kernels_elf_file}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating ${kernels_offsets_hdr}")
add_custom_command(OUTPUT ${kernels_cdescr_file}
        COMMAND ${GEN_KERNELS_CDESCR_SCRIPT} ${kernels_elf_file} ${kernels_cdescr_file} ${kernels_symbol_name}
        DEPENDS ${GEN_KERNELS_CDESCR_SCRIPT} ${kernels_elf_file}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating ${kernels_cdescr_file}")
add_custom_command(OUTPUT ${bootrom_cdescr_file}
        COMMAND ${GEN_KERNELS_CDESCR_SCRIPT} ${bootrom_file} ${bootrom_cdescr_file} ${bootrom_symbol_name}
        DEPENDS ${GEN_KERNELS_CDESCR_SCRIPT} ${bootrom_file}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating ${bootrom_cdescr_file}")
add_custom_target(kernels-offsets ALL
        DEPENDS ${kernels_offsets_hdr})
add_custom_target(kernels-cdescr ALL
        DEPENDS ${kernels_cdescr_file})
add_custom_target(bootrom-cdescr ALL
        DEPENDS ${bootrom_cdescr_file})

if(NOT RV_TOOLCHAIN)
    set(RV_TOOLCHAIN "$ENV{RISCV}" CACHE FILEPATH "RISCV toolchain location")
endif()
if(NOT RV_TOOLCHAIN)
    message(FATAL_ERROR "RISCV toolchain location not set\n"
                        "Use RV_TOOLCHAIN parameter (cmake <path> -DRV_TOOLCHAIN=<path>)\n"
                        "Or use RISCV environment variable (RISCV=<path> cmake <path>)")
endif()
message(STATUS "RISCV toolchain location: ${RV_TOOLCHAIN}")

set(ALLOW_DUPLICATE_CUSTOM_TARGETS TRUE)
if(ETSOC)
    #Neuralizer-produced ELF file copied already to the build dir
    add_custom_target(kernels-elf
        #Empty custom target
    )
else()
    add_custom_target(kernels-elf ALL make TOOLCHAIN=${RV_TOOLCHAIN} OUTDIR=${CMAKE_CURRENT_BINARY_DIR}
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/kernels"
            COMMENT "Building kernels ELF file: ${kernels_elf_file}")
endif()

add_dependencies(kernels-offsets kernels-elf)
add_dependencies(kernels-cdescr kernels-elf)
add_dependencies(bootrom-cdescr kernels-elf)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden -fPIC -std=c++11 -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -fPIC -Wall")

set(ETRT_SOURCES
    src/etrt.cc
    src/etblas.cc
    src/etdnn.cc
    src/et_device.cc
    src/et_stream.cc
    src/et_event.cc
    src/cpu_algo.cc
    src/cpu_algo_launch.cc
    src/registry.h
    src/registry.cc
    src/eti.h
    src/eti.cc
    src/et_bootrom.cc
)
add_library(etrt SHARED ${ETRT_SOURCES})
target_include_directories(etrt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(etrt PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
## FIXME remove external SDK dependencies
target_include_directories(etrt PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../sdk")
target_link_libraries(etrt pthread)
target_link_libraries(etrt etrpc)
add_dependencies(etrt kernels-offsets)
add_dependencies(etrt kernels-cdescr)
add_dependencies(etrt bootrom-cdescr)
add_dependencies(etrt sdk)

set(ETCUDA_SOURCES
    src/interceptors.cc
)

add_library(etcuda SHARED ${ETCUDA_SOURCES})
target_include_directories(etcuda PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(etcuda etrt)
target_link_libraries(etcuda dl)


install(TARGETS etrt
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
install(DIRECTORY include DESTINATION ".")
