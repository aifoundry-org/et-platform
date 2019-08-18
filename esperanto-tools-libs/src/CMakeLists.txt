#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

# Add the device-api as an external project that we build from source
include(ExternalProject)

# FIXME device-api disabled in esperanto-soc because of cloning permision
# issues
# ExternalProject_Add(device-api
#  PREFIX device-api
#  GIT_REPOSITORY git@gitlab:software/device-api.git
#  GIT_TAG 29f910a5744dfdfe1f0f5e408382117b5cb6f44d
#  )

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/Device/TargetDeviceInfo.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Device/TargetDeviceInfo.h"
  )

option(BUILD_SHARED_LIBS "Build shared libs." ON)
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type selected, default to Debug")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type." FORCE)
endif()


set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined" )

# FIXME : disable for now hidden visibility by default: -fvisibility=hidden -fvisibility-inlines-hidden
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++17 -Wall -Werror")
## FIXME  should do a proper Debug release for now force the flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -O0")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -fPIC -Wall")

set(ETRT_SOURCES
  ${PROJECT_BINARY_DIR}/include/Common/ProjectAutogen.h
  ${PROJECT_SOURCE_DIR}/include/Support/Logging.h
  ${PROJECT_SOURCE_DIR}/include/Support/ErrorOr.h
  ${PROJECT_SOURCE_DIR}/include/Support/STLHelpers.h
  ${PROJECT_SOURCE_DIR}/include/Support/RuntimeErrors.h
  ${PROJECT_SOURCE_DIR}/include/Support/HelperMacros.h
  ${PROJECT_SOURCE_DIR}/include/Support/DeviceGuard.h
  ${PROJECT_SOURCE_DIR}/include/Support/MemoryRange.h
  ${PROJECT_SOURCE_DIR}/include/etrt-bin.h
  ${PROJECT_SOURCE_DIR}/include/Common/ProjectAutogen.h.in
  ${PROJECT_SOURCE_DIR}/include/Common/ErrorTypes.h
  ${PROJECT_SOURCE_DIR}/include/Common/et-misc.h
  ${PROJECT_SOURCE_DIR}/include/Common/CommonTypes.h
  ${PROJECT_SOURCE_DIR}/include/C-API/etrt.h
  ${PROJECT_SOURCE_DIR}/include/Core/DeviceTarget.h
  ${PROJECT_SOURCE_DIR}/include/Core/DeviceManager.h
  ${PROJECT_SOURCE_DIR}/include/Core/DeviceInformation.h
  ${PROJECT_SOURCE_DIR}/include/Core/Memory.h
  ${PROJECT_SOURCE_DIR}/include/Core/Kernel.h
  ${PROJECT_SOURCE_DIR}/include/Core/Event.h
  ${PROJECT_SOURCE_DIR}/include/Core/MemoryManager.h
  ${PROJECT_SOURCE_DIR}/include/Core/Error.h
  ${PROJECT_SOURCE_DIR}/include/Core/Device.h
  ${PROJECT_SOURCE_DIR}/include/Core/Stream.h
  ${PROJECT_SOURCE_DIR}/include/Core/CodeModule.h
  ${PROJECT_SOURCE_DIR}/include/Core/CommandLineOptions.h
  ${PROJECT_SOURCE_DIR}/include/Core/CommandLineOptionsDef.in
  ${PROJECT_SOURCE_DIR}/include/DeviceAPI
  ${PROJECT_SOURCE_DIR}/include/DeviceAPI/Response.h
  ${PROJECT_SOURCE_DIR}/include/DeviceAPI/Command.h
  ${PROJECT_SOURCE_DIR}/include/EsperantoRuntime.h
  # FIXME the following should be deprecated once we move to the simulator API
  C-API/etrt.cc
  Core/CodeModule.cc
  Core/CommandQueue.h
  Core/Device.cc
  Core/DeviceManager.cc
  Core/ELFSupport.cc
  Core/ELFSupport.h
  Core/Error.cc
  Core/Event.cc
  Core/MemoryManager.cc
  Core/MemoryAllocator.cc
  Core/MemoryAllocator.h
  Core/ModuleManager.cc
  Core/ModuleManager.h
  Core/registry.cc
  Core/Stream.cc
  demangle.h
  DeviceAPI/Commands.cc
  DeviceAPI/Commands.h
  Device/DeviceTarget.cc
  Device/EmuMailBoxDev.cc
  Device/EmuMailBoxDev.h
  DeviceFW/DeviceFW.cc
  DeviceFW/DeviceFW.h
  DeviceFW/FakeFW.cc
  DeviceFW/FakeFW.h
  DeviceFW/FWManager.cc
  DeviceFW/FWManager.h
  Device/CharDevice.cc
  Device/CharDevice.h
  Device/MailBoxDev.cc
  Device/MailBoxDev.h
  Device/PCIeDevice.cc
  Device/PCIeDevice.h
  Device/SysEmuLauncher.cpp
  Device/SysEmuLauncher.h
  Device/TargetDeviceInfo.h.in
  Device/TargetRPC.cc
  Device/TargetRPC.h
  Device/TargetSysEmu.cc
  Device/TargetSysEmu.h
  etlibdevice.h
  registry.h
  Support/CommandLineOptions.cc
  Support/DeviceGuard.cc
  Support/MemoryRange.cc
  )

add_library(etrt_obj OBJECT ${ETRT_SOURCES})

# Include path dependencies
if(esperanto-fw_FOUND)
  target_include_directories(etrt_obj PRIVATE "${ESPERANTO_FW_INCLUDE_DIR}/esperanto-fw")
endif(esperanto-fw_FOUND)
target_include_directories(etrt_obj PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(etrt_obj PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_include_directories(etrt_obj PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Device")
target_include_directories(etrt_obj PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/etrpc")
target_include_directories(etrt_obj PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/")
target_include_directories(etrt_obj PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/..")
target_include_directories(etrt_obj PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/../include")
include_directories(${PROJECT_SOURCE_DIR}/external/abseil)
target_include_directories(etrt_obj PUBLIC ${EXTERNAL_INCLUDE_DIRECTORIES})
target_include_directories(etrt_obj PUBLIC ${GLOG_INCLUDE_DIRS})
target_include_directories(etrt_obj PUBLIC ${FMTLIB_INCLUDE_DIRS})

# Library dependencies
target_link_libraries(etrt_obj pthread)
add_dependencies(etrt_obj ${external_project_dependencies})
link_directories(${EXTERNAL_LINK_DIRECTORIES})

set_property(TARGET etrt_obj PROPERTY POSITION_INDEPENDENT_CODE 1)

# Generate both static and shared libraries
add_library(etrt SHARED $<TARGET_OBJECTS:etrt_obj>)
target_link_libraries(etrt
  absl::flags
  absl::flags_internal
  absl::flags_parse
  ${GLOG_LIBRARIES}
  ${GFLAGS_LIBRAIES}
  ${FMTLIB_LIBRARIES}
  etsim_api
  protobuf
  grpc++
  grpc
  gpr
  z
  cares
  address_sorting
  pthread
  stdc++fs
  )

## Build shared library
install(TARGETS etrt
  EXPORT EsperantoRuntimeTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  COMPONENT et-runtime
  )

add_library(etrt_static STATIC $<TARGET_OBJECTS:etrt_obj>)

# FIXME The static library as part of its targets should provide a list
# of all the library dependencies it has. To be done correctly this requires
# us to invoke find_package on all of the runtime's dependencies. Currently
# this is not possible because the dependencies have not been built and installed
# prior to configuring the cmake, and this results in calls to find_package to fail.
target_link_libraries(etrt_static
  absl::flags
  absl::flags_internal
  absl::flags_parse
  ${GLOG_LIBRARIES}
  ${GFLAGS_LIBRAIES}
  ${FMTLIB_LIBRARIES}
  # libetsim_api_static.a
  # libprotobuf.a
  # libgrpc++.a
  # libgrpc.a
  # libgpr.a
  # libz.a
  # libcares.a
  # libaddress_sorting.a
  pthread
  stdc++fs
  )

## Build static library
install(TARGETS etrt_static
  EXPORT EsperantoRuntimeTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  COMPONENT et-runtime
  )
