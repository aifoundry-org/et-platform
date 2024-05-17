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
project(deviceManagement VERSION 0.20.0 LANGUAGES C CXX)

include(GNUInstallDirs)

find_package(deviceApi REQUIRED)
find_package(deviceLayer REQUIRED)
find_package(hostUtils REQUIRED)

option(BUILD_TESTS "Build tests" ON)
option(BUILD_DOC "Build documentation" ON)

add_library(DM_obj OBJECT
    src/DeviceManagement.cc
    src/utils.h
)
target_compile_features(DM_obj PUBLIC cxx_std_17)
target_include_directories(DM_obj
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(DM_obj
        PUBLIC
            -DMINION_DEBUG_INTERFACE
            -UNDEBUG
    )
else()
    target_compile_options(DM_obj
        PUBLIC
            -UMINION_DEBUG_INTERFACE
            -DNDEBUG
    )
endif()

set_target_properties(DM_obj PROPERTIES
    PUBLIC_HEADER "include/deviceManagement/DeviceManagement.h;include/deviceManagement/dm.h"
    POSITION_INDEPENDENT_CODE TRUE
)
#TODO: target_link_directories should be forbidden
target_link_directories(DM_obj PUBLIC ${CMAKE_INSTALL_PREFIX}/lib)
target_link_libraries(DM_obj
    PUBLIC
        deviceApi::deviceApi
        deviceLayer::deviceLayer
    PRIVATE
        hostUtils::logging
)

# Generate shared library
add_library(DM SHARED)
add_library(deviceManagement::DM ALIAS DM)
target_link_libraries(DM PUBLIC DM_obj)

# Generate static library
add_library(DM_static STATIC)
add_library(deviceManagement::DM_static ALIAS DM_static)
target_link_libraries(DM_static PUBLIC DM_obj)

install(TARGETS DM_obj DM DM_static
    EXPORT deviceManagementTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/deviceManagement
)

# Install the export set for use with the install-tree
install(EXPORT deviceManagementTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceManagement
    NAMESPACE deviceManagement::
)

if (BUILD_TESTS)
    add_subdirectory(tests)
endif()

if (BUILD_DOC)
    set(DOCUMENTATION_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}" CACHE STRING "Documentation installation path")

    # check if Doxygen is installed
    find_package(Doxygen)
    if (DOXYGEN_FOUND)
        add_subdirectory(doc)
    else (DOXYGEN_FOUND)
        message("Doxygen need to be installed to generate the doxygen documentation")
    endif (DOXYGEN_FOUND)
endif()

##### Package

# Add package information
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/deviceManagementConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/deviceManagementConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceManagement
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/deviceManagementConfigVersion.cmake
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/deviceManagementConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/deviceManagementConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceManagement
)

# export targets from the build tree (this allows consuming generated deviceManagementTargets.cmake without having to install first - not relocatable)
export(TARGETS DM_obj DM DM_static
    FILE "${PROJECT_BINARY_DIR}/deviceManagementTargets.cmake"
    NAMESPACE deviceManagement::
)
