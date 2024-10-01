#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------


cmake_minimum_required(VERSION 3.15)
project(threadPool VERSION 0.1.0)

include(GNUInstallDirs)
option(BUILD_TESTS "Build ThreadPool tests" ON)

add_library(threadPool 
    include/threadPool/ThreadPool.h 
    src/ThreadPool.cpp
)
add_library(et-host-utils::threadPool ALIAS threadPool)
target_compile_features(threadPool INTERFACE cxx_std_14)
target_set_project_warnings(threadPool)
target_include_directories(threadPool
    PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>     
)
target_link_libraries(threadPool PUBLIC logging)

set_target_properties(threadPool PROPERTIES
    PUBLIC_HEADER "include/threadPool/ThreadPool.h;include/threadPool/function2.hpp"
    POSITION_INDEPENDENT_CODE ON
)

# Add package information
include(CMakePackageConfigHelpers)

install(TARGETS threadPool
    EXPORT et-host-utils
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/et-host-utils/threadPool
)

# Install the export set for use with the install-tree
install(EXPORT et-host-utils
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/et-host-utils
    NAMESPACE et-host-utils::
)

if (BUILD_TESTS)
    add_subdirectory(tests)
endif(BUILD_TESTS)

