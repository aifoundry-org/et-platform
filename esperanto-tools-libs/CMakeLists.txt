#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.10.3)
project(runtime VERSION 0.18.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)


list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external/sanitizer-cmake/cmake)

option(ENABLE_WARNINGS_AS_ERRORS "Wether runtime targets should treat warnings as errors" OFF)
include(CompilerWarnings)

include(GNUInstallDirs)
include(CompilerSanitizers)
include(CoverageSupport)

option(BUILD_TESTS "Enable/disable building tests" ON)
option(BUILD_TOOLS "Enable/disable building tools" ON)
option(BUILD_DOCS "Enable/disable building documentation" ON)
option(DISABLE_EASY_PROFILER "Enable/disable easy profiler" ON)
option(ENABLE_CCACHE "Enable ccahe in the builds" OFF)
option(ENABLE_SANITIZER_ADDRESS "" OFF)
option(ENABLE_SANITIZER_LEAK "" OFF)
option(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR "" OFF)
option(ENABLE_SANITIZER_THREAD "" OFF)
option(ENABLE_SANITIZER_MEMORY "" OFF)
option(DISABLE_SANITY_CHECKS "Disable completely sanity checks" OFF)
option(SYNCHRONOUS_MODE "Runs the runtime in synchronous mode, no need to do a waitForEvent/waitForStream" OFF)

# If not empty install any runtime python packages
set(DOCUMENTATION_INSTALL_DIR "${CMAKE_INSTALL_DIR}/doc" CACHE PATH "Documentation installation path")

if(ENABLE_CCACHE)
  find_program(CCACHE_PROGRAM ccache)
  if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CCACHE_PROGRAM}")
    set(EXTERNAL_PROJECT_CCACHE -DCMAKE_CXX_COMPILER_LAUNCHER:STRING=${CCACHE_PROGRAM} -DCMAKE_C_COMPILER_LAUNCHER:STRING=${CCACHE_PROGRAM})
    set(EXTERNAL_PROJECT_ENABLE_CCACHE -DENABLE_CCACHE:BOOL=ON)
  endif(CCACHE_PROGRAM)
endif(ENABLE_CCACHE)

set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include/ CACHE STRING "Include install dir")
set(LIB_INSTALL_DIR  ${CMAKE_INSTALL_PREFIX}/lib/  CACHE STRING "Library install dir")

# System dependencies
find_package(cereal REQUIRED)
find_package(elfio REQUIRED)
find_package(gflags REQUIRED)
find_package(libcap REQUIRED)
find_package(easy_profiler REQUIRED)  


if (NOT TARGET easy_profiler)
  add_library(easy_profiler ALIAS easy_profiler::easy_profiler)
endif()


# Esperanto dependencies
find_package(deviceApi REQUIRED)
find_package(deviceLayer REQUIRED)
find_package(hostUtils REQUIRED)


function(etrt_add_library)
    set(options)
    set(oneValueArgs NAME TYPE)
    set(multiValueArgs)
    cmake_parse_arguments(etrt_add_library "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Runtime base object library
    add_library(${etrt_add_library_NAME} ${etrt_add_library_TYPE}
            src/Runtime.cpp
            src/RuntimeImp.cpp
            src/MemoryManager.cpp
            src/EventManager.cpp
            src/CommandSender.cpp
            src/CoreDumper.cpp
            src/ExecutionContextCache.cpp
            src/ResponseReceiver.cpp
            src/KernelLaunch.cpp
            src/MemcpyOps.cpp
            src/dma/CmaManager.cpp
            src/dma/MemcpyContext.h
            src/dma/MemcpyD2HAction.h
            src/dma/MemcpyH2DAction.cpp
            src/dma/MemcpyH2DAction.h
            src/dma/MemcpyListD2HAction.cpp
            src/dma/MemcpyListD2HAction.h
            src/dma/MemcpyListH2DAction.cpp
            src/dma/MemcpyListH2DAction.h
            src/dma/MemcpyD2HAction.cpp
            src/ProfileEvent.cpp
            src/ProfilerImp.cpp
            src/RemoteProfiler.cpp
            src/StreamManager.cpp
            src/Types.cpp
            src/Utils.cpp
            src/server/Server.cpp
            src/server/Worker.cpp
            src/server/Client.cpp
            src/KernelLaunchOptions.cpp
    )
    add_library(runtime::${etrt_add_library_NAME} ALIAS ${etrt_add_library_NAME})

    set(${etrt_add_library_NAME}_PUBLIC_HEADERS
            include/runtime/IProfiler.h
            include/runtime/IRuntime.h
            include/runtime/IProfileEvent.h
            include/runtime/Types.h
            include/runtime/DeviceLayerFake.h
            ${CMAKE_CURRENT_BINARY_DIR}/include/runtime/IRuntimeExport.h
    )
    set_target_properties(${etrt_add_library_NAME} PROPERTIES
        PUBLIC_HEADER "${${etrt_add_library_NAME}_PUBLIC_HEADERS}"
        POSITION_INDEPENDENT_CODE ON
    )
    target_compile_definitions(${etrt_add_library_NAME}
        PRIVATE
            $<$<BOOL:${SYNCHRONOUS_MODE}>: RUNTIME_SYNCHRONOUS_MODE>
            $<$<BOOL:${DISABLE_SANITY_CHECKS}>: DISABLE_SANITY_CHECKS>
            $<$<BOOL:${DISABLE_EASY_PROFILER}>: DISABLE_EASY_PROFILER>
    )
    target_include_directories(${etrt_add_library_NAME}
        PUBLIC
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_set_project_warnings(${etrt_add_library_NAME})
    target_compile_options(${etrt_add_library_NAME}
        PRIVATE
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Werror=attributes>
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Werror=unused-function>
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Werror=missing-declarations>
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Werror=cast-align>
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Werror=write-strings>
    )
    target_add_sanitizers(${etrt_add_library_NAME})
    target_link_libraries(${etrt_add_library_NAME}
        PUBLIC
            hostUtils::debug
            deviceApi::deviceApi
        PRIVATE
            easy_profiler
            libcap::libcap
            cereal::cereal
            deviceLayer::deviceLayer
            hostUtils::logging
            hostUtils::threadPool
            hostUtils::actionList
            elfio::elfio
    )
endfunction()


## Build shared library
etrt_add_library(NAME etrt TYPE SHARED)
include(GenerateExportHeader)
generate_export_header(etrt
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/runtime/IRuntimeExport.h
    EXPORT_MACRO_NAME     ETRT_API
    NO_EXPORT_MACRO_NAME  ETRT_PRIVATE
    DEPRECATED_MACRO_NAME ETRT_DEPRECATED
    BASE_NAME             ETRT
)

## Build static library
etrt_add_library(NAME etrt_static TYPE STATIC)
# STATIC target needs special preprocessor define to prevent symbol import/export keywords to be added
target_compile_definitions(etrt_static PUBLIC ETRT_STATIC_DEFINE)

## Add server daemon
add_executable(server src/server/main.cpp)
target_include_directories(server PRIVATE src)
target_link_libraries(server 
    PRIVATE 
        runtime::etrt
        deviceLayer::deviceLayer
        gflags::gflags
        cereal::cereal)
target_compile_definitions(server 
    PRIVATE
        $<$<BOOL:${DISABLE_EASY_PROFILER}>: DISABLE_EASY_PROFILER>
)
    
## Install library targets (new)
install(TARGETS etrt etrt_static server
    EXPORT runtimeTargets
    RUNTIME       DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/runtime
)

# Install the export set for use with the install-tree
install(EXPORT runtimeTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/runtime
    NAMESPACE runtime::
)

## project documentation
if (BUILD_DOCS)
    add_subdirectory(docs)
endif()


## Tools
if (BUILD_TOOLS)
    add_subdirectory(tools)
endif()

## tests
if (BUILD_TESTS)
    find_package(GTest CONFIG REQUIRED)

    enable_testing()
    add_subdirectory(tests)
endif()




##### Package

### Add package information
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/runtimeConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/runtimeConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/runtime
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/runtimeConfigVersion.cmake
    VERSION ${VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the runtimeConfig.cmake and runtimeConfigVersion.cmake
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/runtimeConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/runtimeConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/runtime
)

# export targets from the build tree (this allows consuming generated runtimeTargets.cmake without having to install first - not relocatable)
export(TARGETS etrt etrt_static
    FILE ${CMAKE_CURRENT_BINARY_DIR}/runtimeTargets.cmake
    NAMESPACE runtime::
)

