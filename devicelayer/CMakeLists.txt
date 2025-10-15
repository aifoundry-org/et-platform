# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)
project(deviceLayer VERSION 4.1.0 LANGUAGES CXX)

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
option(BUILD_DOCS "Enable/disable building documentation" OFF)

# Hide things by default
set(VISIBILITY_PRESET_VALUES default protected hidden)
set(CMAKE_ASM_VISIBILITY_PRESET default CACHE STRING "Value for ASM symbol visibility compile flags. If ON, adds -fvisibility")
set(CMAKE_C_VISIBILITY_PRESET   default CACHE STRING "Value for C   symbol visibility compile flags. If ON, adds -fvisibility")
set(CMAKE_CXX_VISIBILITY_PRESET default CACHE STRING "Value for C++ symbol visibility compile flags. If ON, adds -fvisibility")
set(CMAKE_VISIBILITY_INLINES_HIDDEN OFF CACHE BOOL "Whether to add a compile flag to hide symbols of inline functions. If ON, adds -fvisibility-inlines-hidden")
set_property(CACHE CMAKE_ASM_VISIBILITY_PRESET PROPERTY STRINGS ${VISIBILITY_PRESET_VALUES})
set_property(CACHE CMAKE_C_VISIBILITY_PRESET PROPERTY STRINGS ${VISIBILITY_PRESET_VALUES})
set_property(CACHE CMAKE_CXX_VISIBILITY_PRESET PROPERTY STRINGS ${VISIBILITY_PRESET_VALUES})
if (NOT CMAKE_ASM_VISIBILITY_PRESET IN_LIST VISIBILITY_PRESET_VALUES)
    message(FATAL_ERROR "CMAKE_ASM_VISIBILITY_PRESET must be one of ${VISIBILITY_PRESET_VALUES}")
endif()
if (NOT CMAKE_C_VISIBILITY_PRESET IN_LIST VISIBILITY_PRESET_VALUES)
    message(FATAL_ERROR "CMAKE_C_VISIBILITY_PRESET must be one of ${VISIBILITY_PRESET_VALUES}")
endif()
if (NOT CMAKE_CXX_VISIBILITY_PRESET IN_LIST VISIBILITY_PRESET_VALUES)
    message(FATAL_ERROR "CMAKE_CXX_VISIBILITY_PRESET must be one of ${VISIBILITY_PRESET_VALUES}")
endif()

find_package(linuxDriver REQUIRED)
find_package(sw-sysemu REQUIRED)
find_package(hostUtils REQUIRED)
find_package(Boost REQUIRED)

add_library(deviceLayer
    src/DeviceLayer.cpp
    src/DeviceSysEmu.cpp
    src/DeviceSysEmuMulti.cpp
    src/DevicePcie.cpp
    src/SysEmuHostListener.cpp
)

add_library(deviceLayer::deviceLayer ALIAS deviceLayer)
target_compile_features(deviceLayer PUBLIC cxx_std_17)
target_set_project_warnings(deviceLayer)
target_compile_options(deviceLayer
    PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Werror=all>
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Werror=attributes>
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Werror=unused-function>
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Werror=missing-declarations>
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Werror=cast-align>
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Werror=write-strings>
)
target_add_sanitizers(deviceLayer)
target_include_directories(deviceLayer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(deviceLayer PROPERTIES
    PUBLIC_HEADER "include/device-layer/IDeviceLayer.h;include/device-layer/DeviceLayerMock.h;${CMAKE_CURRENT_BINARY_DIR}/include/device-layer/IDeviceLayerExport.h"
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

include(GenerateExportHeader)
generate_export_header(deviceLayer
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/device-layer/IDeviceLayerExport.h
    EXPORT_MACRO_NAME DEVICE_LAYER_EXPORT
    NO_EXPORT_MACRO_NAME  DEVICE_LAYER_PRIVATE
    DEPRECATED_MACRO_NAME DEVICE_LAYER_DEPRECATED
    BASE_NAME             DEVICE_LAYER
)
target_sources(deviceLayer PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include/device-layer/IDeviceLayerExport.h)

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
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/device-layer
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install the export set for use with the install-tree
install(EXPORT DeviceLayerTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceLayer
    NAMESPACE deviceLayer::
)

## project documentation
if (BUILD_DOCS)
    add_subdirectory(docs)
endif()

if (ENABLE_TESTS)
    enable_testing()
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
