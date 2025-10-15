# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.5)
project(tf-protocol VERSION 1.5.0 DESCRIPTION "Esperanto Test Framework Project" LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(GNUInstallDirs)

### Build
add_subdirectory(src)

### Installation
include(CMakePackageConfigHelpers)

install(TARGETS tf-protocol
    EXPORT tf-protocolTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto/tf-protocol
)
install(EXPORT tf-protocolTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tf-protocol
    NAMESPACE tf-protocol::
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/tf-protocolConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/tf-protocolConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tf-protocol
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/tf-protocolConfigVersion.cmake
    VERSION ${VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/tf-protocolConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/tf-protocolConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tf-protocol
)
