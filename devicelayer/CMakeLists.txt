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
project(deviceLayer VERSION 0.1.0)

option(ENABLE_CONAN "Enable using conan to inject dependencies" OFF)
if (ENABLE_CONAN)
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
                    "${CMAKE_BINARY_DIR}/conan.cmake"
                    EXPECTED_HASH SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
                    TLS_VERIFY ON)
    endif()
    include(${CMAKE_BINARY_DIR}/conan.cmake)

    conan_cmake_configure(REQUIRES sw-sysemu/1.0
                          GENERATORS cmake_find_package
                          OPTIONS gflags:namespace="google")

    conan_cmake_autodetect(conan_settings)

    conan_cmake_install(PATH_OR_REFERENCE .
                    BUILD missing
                    SETTINGS ${conan_settings})
endif()
                   
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR} ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR} ${CMAKE_PREFIX_PATH} ${CMAKE_CURRENT_SOURCE_DIR})

message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

set(ET_DEFAULT_BUILD_TYPE Release)

option(ENABLE_STRICT_BUILD_TYPES "Whether project configuration should fail if build_type is not one of the allowed values: Debug Release RelWithDebInfo MinSizeRel" ON)
include(CommonProjectSettings)
include(GNUInstallDirs)

option(ENABLE_WARNINGS_AS_ERRORS "Wether deviceLayer targets should treat warnings as errors" OFF)
include(CompilerWarnings)

option(ENABLE_SANITIZER_ADDRESS "" OFF)
option(ENABLE_SANITIZER_LEAK "" OFF)
option(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR "" OFF)
option(ENABLE_SANITIZER_THREAD "" OFF)
option(ENABLE_SANITIZER_MEMORY "" OFF)
include(CompilerSanitizers)

option(ENABLE_TESTS "Enable compiling tests" ON)

option(ENABLE_DEPRECATED "Generate Deprecated config file" ON)

find_package(linuxDriver REQUIRED)
find_package(sw-sysemu REQUIRED)
find_package(hostUtils REQUIRED)
find_package(Boost REQUIRED)

add_library(deviceLayer
    src/DeviceLayer.cpp
    src/DeviceSysEmu.cpp
    src/DevicePcie.cpp
    src/SysEmuHostListener.cpp
)

add_library(deviceLayer::deviceLayer ALIAS deviceLayer)
target_compile_features(deviceLayer PUBLIC cxx_std_17)
target_set_project_warnings(deviceLayer)
target_add_sanitizers(deviceLayer)
target_include_directories(deviceLayer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(deviceLayer PROPERTIES
    PUBLIC_HEADER "include/deviceLayer/IDeviceLayer.h;include/deviceLayer/IDeviceLayerMock.h;include/deviceLayer/IDeviceLayerFake.h"
    POSITION_INDEPENDENT_CODE TRUE
)
target_link_libraries(deviceLayer
    PUBLIC
        sw-sysemu::sw-sysemu
        hostUtils::debug
    PRIVATE
        hostUtils::logging
        linuxDriver::linuxDriver
        Boost::boost
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9")
    target_link_libraries(deviceLayer PUBLIC stdc++fs)
endif ()

# Add package information
include(CMakePackageConfigHelpers)

# Add all targets to the build-tree export set
export(TARGETS deviceLayer
    FILE "${PROJECT_BINARY_DIR}/DeviceLayer.cmake"
)

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE deviceLayer)

install(TARGETS deviceLayer
    EXPORT DeviceLayerTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/device-layer
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/deviceLayer
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install the export set for use with the install-tree
install(EXPORT DeviceLayerTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceLayer
    NAMESPACE deviceLayer::
)

if (ENABLE_TESTS)
    get_target_property(PCIE_DRIVER_INC_DIR linuxDriver::linuxDriver INTERFACE_INCLUDE_DIRECTORIES)
    add_subdirectory(tests)
endif()

##### Package

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/deviceLayerConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/deviceLayerConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceLayer
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/deviceLayerConfigVersion.cmake
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/deviceLayerConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/deviceLayerConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceLayer
)

if (ENABLE_DEPRECATED)
    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/DeviceLayerConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/DeviceLayerConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceLayer
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/DeviceLayerConfigVersion.cmake
        VERSION ${CMAKE_PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/DeviceLayerConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/DeviceLayerConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceLayer
    )
endif()