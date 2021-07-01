#------------------------------------------------------------------------------
# Copyright (C) 2021, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------


cmake_minimum_required(VERSION 3.17)
project(debug VERSION 0.1.0)

include(GNUInstallDirs)

find_package(g3log REQUIRED)

add_library(debug 
    include/debug/StackException.h
    src/StackException.cpp
)
add_library(hostUtils::debug ALIAS debug)
target_compile_features(debug INTERFACE cxx_std_17)
target_set_project_warnings(debug)
target_include_directories(debug
    PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>     
)
target_link_libraries(debug PUBLIC g3log::g3log)

set_target_properties(debug PROPERTIES
  PUBLIC_HEADER "include/debug/StackException.h;include/debug/Check.h"
)
set_target_properties(debug PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Add package information
include(CMakePackageConfigHelpers)

install(TARGETS debug
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

