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
project(deviceManagementApplication VERSION 1.17.0 LANGUAGES CXX C)

include(ExternalProject)
include(GNUInstallDirs)

find_package(deviceLayer REQUIRED)
find_package(fmt REQUIRED)
find_package(hostUtils REQUIRED)
find_package(deviceManagement REQUIRED)
find_package(glog REQUIRED)
find_package(Threads REQUIRED)
find_package(esperantoTrace REQUIRED)

option(ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

if (NOT DEFINED ftxui_PROVIDER)
  message(STATUS "ftxui_PROVIDER not defined. Setting default to 'sources'.")
  set(ftxui_PROVIDER "sources" CACHE STRING "Find ftxui dependency using 'find_package' or forcing a build it from 'sources'")
endif()
if ("${ftxui_PROVIDER}" STREQUAL "find_package")
    find_package(ftxui REQUIRED)
else()
  include(FetchContent)
  
  FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
    GIT_TAG v5.0.0
  )
  
  FetchContent_GetProperties(ftxui)
  if(NOT ftxui_POPULATED)
    FetchContent_Populate(ftxui)
    add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
endif()

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
        fmt::fmt
        glog::glog
        ${CMAKE_THREAD_LIBS_INIT}
        ${CMAKE_DL_LIBS}
        deviceManagement::DM
        esperantoTrace::et_trace
        hostUtils::logging
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0.0>>:stdc++fs>
        deviceLayer::deviceLayer
)
target_compile_options(dev_mngt_service
    PRIVATE
        -Wall
        $<$<BOOL:${ENABLE_WARNINGS_AS_ERRORS}>:-Werror>
)
install(TARGETS dev_mngt_service
    EXPORT deviceManagementApplicationTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_executable(et-powertop
    src/et-top.cc
    src/render.cpp
)
target_compile_features(et-powertop PUBLIC cxx_std_17)
target_include_directories(et-powertop
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
	    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)
target_compile_definitions(et-powertop
    PUBLIC
        -DET_TOP_VERSION="${CMAKE_PROJECT_VERSION}"
)
target_link_libraries(et-powertop
    PRIVATE
        glog::glog
        ${CMAKE_THREAD_LIBS_INIT}
        ${CMAKE_DL_LIBS}
        deviceManagement::DM
        esperantoTrace::et_trace
        hostUtils::logging
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0.0>>:stdc++fs>
        deviceLayer::deviceLayer
        ftxui::screen
        ftxui::dom
        ftxui::component

)
target_compile_options(et-powertop
    PRIVATE
        -Wall
        $<$<BOOL:${ENABLE_WARNINGS_AS_ERRORS}>:-Werror>
)
install(TARGETS et-powertop
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
