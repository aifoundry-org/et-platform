#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.5)
project(deviceManagementApplication VERSION 1.3.0 LANGUAGES CXX C)

include(ExternalProject)
include(GNUInstallDirs)

find_package(deviceLayer REQUIRED)
find_package(hostUtils REQUIRED)
find_package(deviceManagement REQUIRED)
find_package(glog REQUIRED)
find_package(Threads REQUIRED)
find_package(esperantoTrace REQUIRED)

add_executable(dev_mngt_service
    src/dev_mngt_service.cc
)
target_compile_features(dev_mngt_service PUBLIC cxx_std_17)
target_include_directories(dev_mngt_service
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
)
target_compile_definitions(dev_mngt_service
    PUBLIC
        -DDM_APP_VERSION="${CMAKE_PROJECT_VERSION}"
)
target_link_libraries(dev_mngt_service
    PRIVATE
        glog::glog
        ${CMAKE_THREAD_LIBS_INIT}
        ${CMAKE_DL_LIBS}
        deviceManagement::DM
        esperantoTrace::et_trace
        hostUtils::logging
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0.0>>:stdc++fs>
        deviceLayer::deviceLayer
)

install(TARGETS dev_mngt_service
    EXPORT deviceManagementApplicationTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_executable(et-top
    src/et-top.cc
)
target_compile_features(et-top PUBLIC cxx_std_17)
target_include_directories(et-top
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
)

target_link_libraries(et-top
    PRIVATE
        glog::glog
        ${CMAKE_THREAD_LIBS_INIT}
        ${CMAKE_DL_LIBS}
        deviceManagement::DM
        esperantoTrace::et_trace
        hostUtils::logging
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0.0>>:stdc++fs>
        deviceLayer::deviceLayer
)

install(TARGETS et-top
    EXPORT deviceManagementApplicationTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Add package information
include(CMakePackageConfigHelpers)

##### Package

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/deviceManagementApplicationConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/deviceManagementApplicationConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceManagementApplication
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/deviceManagementApplicationConfigVersion.cmake
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/deviceManagementApplicationConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/deviceManagementApplicationConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceManagementApplication
)
