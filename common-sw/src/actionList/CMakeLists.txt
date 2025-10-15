# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0


cmake_minimum_required(VERSION 3.21)
project(actionList VERSION 0.1.0)

include(GNUInstallDirs)
option(BUILD_TESTS "Build ActionList tests" ON)

add_library(actionList 
    include/hostUtils/actionList/ActionList.h
    include/hostUtils/actionList/Runner.h
    src/ActionList.cpp
    src/Runner.cpp
)

add_library(hostUtils::actionList ALIAS actionList)
target_compile_features(actionList INTERFACE cxx_std_17)
target_set_project_warnings(actionList)
target_compile_options(actionList
    PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Werror=attributes>
)
target_include_directories(actionList
    PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)
target_link_libraries(actionList PUBLIC logging)

set_target_properties(actionList PROPERTIES
    PUBLIC_HEADER "include/hostUtils/actionList/ActionList.h;include/hostUtils/actionList/Runner.h;${CMAKE_CURRENT_BINARY_DIR}/include/hostUtils/actionList/ActionListExport.h"
    POSITION_INDEPENDENT_CODE ON
)

include(GenerateExportHeader)
generate_export_header(actionList
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/hostUtils/actionList/ActionListExport.h
    EXPORT_MACRO_NAME ACTION_LIST_API
)

# Add package information
include(CMakePackageConfigHelpers)

install(TARGETS actionList
    EXPORT hostUtilsTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hostUtils/actionList
)

# Install the export set for use with the install-tree
install(EXPORT hostUtilsTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hostUtils
    NAMESPACE hostUtils::
)

if (BUILD_TESTS)
    add_subdirectory(tests)
endif(BUILD_TESTS)

