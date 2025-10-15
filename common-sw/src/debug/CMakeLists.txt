# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0


cmake_minimum_required(VERSION 3.21)
project(debug VERSION 0.1.0)

include(GNUInstallDirs)

find_package(g3log REQUIRED)

# WARNING: 'debug' is a very unfortunate name 
# because it's reserved as a special keword in CMake target_link_libraries
# INFO: I will introduce a new target 'debugging' with a name to avoid collisions

# old target
add_library(debug
    include/hostUtils/debug/StackException.h
    src/StackException.cpp
)
add_library(hostUtils::debug ALIAS debug)
target_compile_features(debug INTERFACE cxx_std_17)
target_set_project_warnings(debug)
target_include_directories(debug
    PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)
target_link_libraries(debug PUBLIC g3log)

set_target_properties(debug PROPERTIES
    PUBLIC_HEADER "include/hostUtils/debug/StackException.h;include/hostUtils/debug/Check.h;${CMAKE_CURRENT_BINARY_DIR}/include/hostUtils/debug/DebugExport.h"
    POSITION_INDEPENDENT_CODE ON
)
include(GenerateExportHeader)
generate_export_header(debug
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/hostUtils/debug/DebugExport.h
    EXPORT_MACRO_NAME DEBUG_API
)

###########
# new target 
add_library(debugging $<TARGET_OBJECTS:debug>)
target_compile_features(debugging INTERFACE cxx_std_17)
target_set_project_warnings(debugging)
target_include_directories(debugging
    PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)
target_link_libraries(debugging PUBLIC g3log)

###########

# Add package information
include(CMakePackageConfigHelpers)

install(TARGETS debug debugging
    EXPORT hostUtilsTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hostUtils/debug
)

# Install the export set for use with the install-tree
install(EXPORT hostUtilsTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hostUtils
    NAMESPACE hostUtils::
)

