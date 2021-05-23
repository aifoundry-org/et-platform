#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------


cmake_minimum_required(VERSION 3.17)
project(logging VERSION 0.1.0)

include(GNUInstallDirs)

find_package(g3log REQUIRED)

add_library(logging 
    include/logging/Logging.h 
    include/logging/Logger.h 
    include/logging/DefaultSinks.h 
    include/logging/Instance.h
    src/Instance.cpp
)
add_library(hostUtils::logging ALIAS logging)
target_compile_features(logging INTERFACE cxx_std_17)
target_include_directories(logging
    PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>     
)
target_link_libraries(logging PUBLIC g3log::g3log)

set_target_properties(logging PROPERTIES
  PUBLIC_HEADER "include/logging/Logging.h;include/logging/Logger.h;include/logging/DefaultSinks.h;include/logging/Instance.h"
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

