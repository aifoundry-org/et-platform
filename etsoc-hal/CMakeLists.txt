#------------------------------------------------------------------------------
# Copyright (C) 2021, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.15)
project(etsoc_hal VERSION 1.8.0 LANGUAGES C)

set(TARGET_NAME etsoc_hal)
set(TARGET_NAMESPACE etsoc_hal)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)

### Compile library

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-zero-initialized-in-bss -ffunction-sections -fdata-sections ")
if (NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 11)
endif()

add_library(${TARGET_NAME} STATIC
    #src/dmac_api.c        REQUIRES header (cpu.h) not present in the project
    #src/DW_apb_gpio.c     FAILS due -Werror
    #src/DW_apb_i2c.c      FAILS due -Werror
    #src/DW_apb_uart.c     FAILS due -Werror
    #src/et_cru_api.c      REQUIRES header (cpu.h) not present in the project
    # src/et_i2c.c         NO DEPENDENCY on it
    #src/memshire_ddr_init.c REQUIRES header (cpu.h) not present in the project
    #src/memshire_pll_init.c REQUIRES header (cpu.h) not present in the project
    #src/memshire_utils.c    REQUIRES header (cpu.h) not present in the project
    #src/MX25_CMD.c          REQUIRES header (cpu.h) not present in the project
    #src/pllMovellus_api.c   REQUIRES header (cpu.h) not present in the project
    #src/pvtcMoortec_api.c   REQUIRES header (cpu.h) not present in the project
    #src/sp_minion_cold_reset_sequence.c REQUIRES header (esr.h) not present in the project
    #src/spi_api.c           REQUIRES header (cpu.h) not present in the project
    #src/usb_phy.c           REQUIRES header (cpu.h) not present in the project
    src/slam_engine.c
    wrapper/ddr_init.c
    wrapper/minion_pll.c
    wrapper/minion_pll_program.c
    wrapper/noc_reconfig.c
)
add_library(${TARGET_NAMESPACE}::${TARGET_NAME} ALIAS ${TARGET_NAME})
target_include_directories(${TARGET_NAME} SYSTEM BEFORE
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include_sys
)
target_include_directories(${TARGET_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>                      # for intenral usage (#include "hwinc/foo.h" is used)
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/${TARGET_NAME}>               # for users that do #include <etsoc_hal/inc/foo.h>
)
target_compile_options(${TARGET_NAME} PRIVATE -Wall)

### Install project

include(CMakePackageConfigHelpers)

install(DIRECTORY include/etsoc_hal include/hwinc
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/${TARGET_NAME}
)
install(FILES
    include/hal_ddr_init.h
    include/hal_minion_pll.h
    include/hal_noc_reconfig.h
    include/esr.h
    include/hal_vaultip_hw.h
    include/hal_vaultip_sw.h
    include/hal_vaultip_sw_asset.h
    include/hal_vaultip_static_assets.h
    include/slam_engine.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/${TARGET_NAME})

install(TARGETS ${TARGET_NAME}
    EXPORT ${TARGET_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto-fw
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto-fw
)

install(EXPORT ${TARGET_NAME}Targets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${TARGET_NAME}
    NAMESPACE ${TARGET_NAMESPACE}::
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}Config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${TARGET_NAME}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}ConfigVersion.cmake
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}ConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${TARGET_NAME}
)
