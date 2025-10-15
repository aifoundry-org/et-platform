# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0


cmake_minimum_required(VERSION 3.21)
project(logging VERSION 0.1.0)

include(GNUInstallDirs)

find_package(g3log REQUIRED)

add_library(logging 
    include/hostUtils/logging/Logging.h
    include/hostUtils/logging/Logger.h
    include/hostUtils/logging/DefaultSinks.h
    include/hostUtils/logging/Instance.h
    src/Instance.cpp
)
add_library(hostUtils::logging ALIAS logging)
target_compile_features(logging INTERFACE cxx_std_17)
target_set_project_warnings(logging)
target_include_directories(logging
    PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)
target_compile_features(logging PRIVATE cxx_std_17)
target_link_libraries(logging PUBLIC g3log)

set_target_properties(logging PROPERTIES
    PUBLIC_HEADER "include/hostUtils/logging/Logging.h;include/hostUtils/logging/Logger.h;include/hostUtils/logging/DefaultSinks.h;include/hostUtils/logging/Instance.h;${CMAKE_CURRENT_BINARY_DIR}/include/hostUtils/logging/LoggingExport.h"
    POSITION_INDEPENDENT_CODE ON
)
include(GenerateExportHeader)
generate_export_header(logging
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/hostUtils/logging/LoggingExport.h
    EXPORT_MACRO_NAME LOGGING_API
)

# Add package information
include(CMakePackageConfigHelpers)

install(TARGETS logging
    EXPORT hostUtilsTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hostUtils/logging
)

# Install the export set for use with the install-tree
install(EXPORT hostUtilsTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hostUtils
    NAMESPACE hostUtils::
)

