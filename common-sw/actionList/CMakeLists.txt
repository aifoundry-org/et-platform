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
project(actionList VERSION 0.1.0)

include(GNUInstallDirs)
option(BUILD_TESTS "Build ActionList tests" ON)

add_library(actionList 
    include/actionList/ActionList.h 
    include/actionList/Runner.h 
    src/ActionList.cpp
    src/Runner.cpp
)

add_library(hostUtils::actionList ALIAS actionList)
target_compile_features(actionList INTERFACE cxx_std_14)
target_set_project_warnings(actionList)
target_include_directories(actionList
    PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>     
)
target_link_libraries(actionList PUBLIC logging)

set_target_properties(actionList PROPERTIES
    PUBLIC_HEADER "include/actionList/ActionList.h;include/actionList/Runner.h"
    POSITION_INDEPENDENT_CODE ON
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

