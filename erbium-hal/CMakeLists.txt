# Copyright (c) 2026 AIFoundry
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.15)
project(erbium_hal VERSION 0.1.0 LANGUAGES C)

set(TARGET_NAME erbium_hal)
set(TARGET_NAMESPACE erbium_hal)

include(GNUInstallDirs)

### Header-only library

# hwinc/*.h are generated register headers — no .c files to compile, so
# erbium_hal is an INTERFACE library that just exposes the include tree.
# If later additions introduce compiled sources (e.g. SP-boot wrappers
# analogous to etsoc_hal), switch STATIC and add sources here.
add_library(${TARGET_NAME} INTERFACE)
add_library(${TARGET_NAMESPACE}::${TARGET_NAME} ALIAS ${TARGET_NAME})

target_include_directories(${TARGET_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/${TARGET_NAME}>
)

### Install project

include(CMakePackageConfigHelpers)

# hwinc/*.h — generated register headers + top.h
install(DIRECTORY include/hwinc
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/${TARGET_NAME}
)

install(TARGETS ${TARGET_NAME}
    EXPORT ${TARGET_NAME}Targets
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
