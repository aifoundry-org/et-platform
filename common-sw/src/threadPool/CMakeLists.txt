#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------


cmake_minimum_required(VERSION 3.21)
project(threadPool VERSION 0.1.0)

include(GNUInstallDirs)
option(BUILD_TESTS "Build ThreadPool tests" ON)

add_library(threadPool 
    include/hostUtils/threadPool/ThreadPool.h
    src/ThreadPool.cpp
)
add_library(hostUtils::threadPool ALIAS threadPool)
target_compile_features(threadPool INTERFACE cxx_std_17)
target_set_project_warnings(threadPool)
target_include_directories(threadPool
    PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)
target_link_libraries(threadPool PUBLIC logging)

set_target_properties(threadPool PROPERTIES
    PUBLIC_HEADER "include/hostUtils/threadPool/ThreadPool.h;include/hostUtils/threadPool/function2.hpp;${CMAKE_CURRENT_BINARY_DIR}/include/hostUtils/threadPool/ThreadPoolExport.h"
    POSITION_INDEPENDENT_CODE ON
)
include(GenerateExportHeader)
generate_export_header(threadPool
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/hostUtils/threadPool/ThreadPoolExport.h
    EXPORT_MACRO_NAME THREAD_POOL_API
)

# Add package information
include(CMakePackageConfigHelpers)

install(TARGETS threadPool
    EXPORT hostUtilsTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hostUtils/threadPool
)

# Install the export set for use with the install-tree
install(EXPORT hostUtilsTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hostUtils
    NAMESPACE hostUtils::
)

if (BUILD_TESTS)
    add_subdirectory(tests)
endif(BUILD_TESTS)

