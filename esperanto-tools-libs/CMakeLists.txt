#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.5)

project(EsperantoRuntime VERSION 0.1.0 LANGUAGES CXX C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(HOST_SW "Enable building for the host-sw repo")

option(BUILD_SHARED_LIBS "Build shared libs." ON)
option(ETSOC "Build for the ETSOC")
option(ENABLE_TESTS "Enable tests" ON)
# To be overwritten in cases where we want to also "release" the external tools
# used by the runtime
set(RT_EXTERNAL_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external-install"
  CACHE PATH "Instalation path of external dependencies")

message(STATUS "CMAKE_CURRENT_SOURCE_DIR=" ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "CMAKE_CURRENT_BINARY_DIR=" ${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "HOST_SW=" ${HOST_SW})
message(STATUS "ETSOC=" ${ETSOC})
message(STATUS "ENABLE_TESTS=" ${ENABLE_TESTS})

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type selected, default to Debug")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type." FORCE)
endif()

set(external_project_dependencies "" CACHE STRING "External project list")

# Add rules for auto-generated headers.
add_subdirectory(include)

################################################################################
#                           Kernel build rules
################################################################################

if (BUILD_KERNELS)
  set(kernels_elf_file "${CMAKE_CURRENT_BINARY_DIR}/net_test")
  set(kernels_offsets_hdr "${CMAKE_CURRENT_BINARY_DIR}/kernels_offsets.h")
  set(kernels_cdescr_file "${CMAKE_CURRENT_BINARY_DIR}/kernels_cdescr.inc")
  set(kernels_symbol_name "gEtKernelsElf")

  set(bootrom_file "${CMAKE_CURRENT_BINARY_DIR}/bootrom")
  set(bootrom_cdescr_file "${CMAKE_CURRENT_BINARY_DIR}/bootrom_cdescr.inc")
  set(bootrom_symbol_name "gEtBootrom")

  set(GEN_KERNELS_OFFSETS_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_entry_points.sh")
  set(GEN_KERNELS_CDESCR_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_cdescr.sh")


  if(NOT RV_TOOLCHAIN)
    set(RV_TOOLCHAIN "$ENV{RISCV}" CACHE FILEPATH "RISCV toolchain location")
  endif()
  if(NOT RV_TOOLCHAIN)
    message(FATAL_ERROR "RISCV toolchain location not set\n"
      "Use RV_TOOLCHAIN parameter (cmake <path> -DRV_TOOLCHAIN=<path>)\n"
      "Or use RISCV environment variable (RISCV=<path> cmake <path>)")
  endif()
  message(STATUS "RISCV toolchain location: ${RV_TOOLCHAIN}")

  add_custom_command(OUTPUT ${kernels_elf_file} ${bootrom_file}
    COMMAND MAKEFLAGS=  make TOOLCHAIN=${RV_TOOLCHAIN} OUTDIR=${CMAKE_CURRENT_BINARY_DIR} INC_DIR=${CMAKE_CURRENT_BINARY_DIR}/include
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/kernels"
    COMMENT "Building kernels ELF file: ${kernels_elf_file}")

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

  add_custom_target(kernels-elf-file ALL
    DEPENDS ${kernels_elf_file})

  add_custom_target(bootrom-file ALL
    DEPENDS ${bootrom_file})

  add_custom_target(kernels-offsets ALL
    DEPENDS ${kernels_offsets_hdr})

  add_custom_target(kernels-cdescr ALL
    DEPENDS ${kernels_cdescr_file})

  add_custom_target(bootrom-cdescr ALL
    DEPENDS ${bootrom_cdescr_file})

  set(ALLOW_DUPLICATE_CUSTOM_TARGETS TRUE)

endif(BUILD_KERNELS)

################################################################################
#                           Add Other subdirectories
################################################################################

add_subdirectory(docs)
add_subdirectory(external)
add_subdirectory(src)
if (ENABLE_TESTS)
  add_subdirectory(tests)
  add_subdirectory(tools)
endif(ENABLE_TESTS)
