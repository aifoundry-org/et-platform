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

project(deviceApi VERSION 0.1.0  DESCRIPTION "Esperanto DeviceAPI Project" LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

option(DEVICEAPI_BUILD_DOC "Build documentation" OFF)
option(DEVICEAPI_DEPRECATED "Enable deprecated install targets" ON)

if (DEVICEAPI_DEPRECATED)
    set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include/ CACHE STRING "Include install dir")
    set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib/ CACHE STRING "Library install dir")
    set(SYSCONFIG_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/etc/et-runtime/ CACHE STRING "Config install dir")
endif()

include(GNUInstallDirs)

add_subdirectory(src)

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

if (DEVICEAPI_DEPRECATED)
    export(PACKAGE esperanto-device-api)

    configure_package_config_file(esperanto-device-api-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/esperanto-device-api-config.cmake
        INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}
        INSTALL_DESTINATION ${LIB_INSTALL_DIR}/esperanto-device-api/cmake
        PATH_VARS INCLUDE_INSTALL_DIR LIB_INSTALL_DIR)
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/esperanto-device-api-config-version.cmake
        VERSION ${VERSION}
        COMPATIBILITY SameMajorVersion)

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/esperanto-device-api-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/esperanto-device-api-config-version.cmake"
        DESTINATION ${LIB_INSTALL_DIR}/cmake/esperanto-device-api
        COMPONENT device-api)
endif()