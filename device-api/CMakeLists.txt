# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.15)
project(deviceApi VERSION 2.6.0 DESCRIPTION "Esperanto DeviceAPI Project" LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

option(DEVICEAPI_BUILD_DOC "Build documentation" OFF)

include(GNUInstallDirs)

add_subdirectory(include)

if (DEVICEAPI_BUILD_DOC)
    add_subdirectory(doc)
endif ()

### Add package information
include(CMakePackageConfigHelpers)

install(TARGETS deviceApi
    EXPORT deviceApiTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto/device-apis
)
install(EXPORT deviceApiTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceApi
    NAMESPACE deviceApi::
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/deviceApiConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/deviceApiConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceApi
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/deviceApiConfigVersion.cmake
    VERSION ${VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/deviceApiConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/deviceApiConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceApi
)
