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
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -fPIC -Wall")

set(ETRT_SOURCES
  # FIXME the following should be deprecated once we move to the simulator API
    etrpc/et_socket_addr.cc
    etrpc/backend.cc
    etrpc/backend-sysemu.cc
    etrpc/et-card-proxy.cc
    etrpc/card-emu.cc
    etrpc/et-rpc.cc
    # --------
    C-API/etrt.cc
    Core/Commands.cc
    Core/CodeModule.cc
    Core/DeviceManager.cc
    Core/Error.cc
    Core/MemoryManager.cc
    Core/ModuleManager.cc
    Core/Device.cc
    Core/ELFSupport.cc
    Core/et_event.cc
    Core/registry.cc
    Core/Stream.cc
    Device/DeviceTarget.cc
    Device/PCIeDevice.cc
    Device/SysEmuLauncher.cpp
    Device/TargetCardProxy.cc
    Device/TargetRPC.cc
    Device/TargetSysEmu.cc
    DeviceFW/FWManager.cc
    DeviceFW/FakeFW.cc
    DeviceFW/DeviceFW.cc
    registry.h
    Support/DeviceGuard.cc
)

add_library(etrt_obj OBJECT ${ETRT_SOURCES})

# Include path dependencies
target_include_directories(etrt_obj PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(etrt_obj PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_include_directories(etrt_obj PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Device")
target_include_directories(etrt_obj PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/etrpc")
target_include_directories(etrt_obj PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/")
target_include_directories(etrt_obj PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/..")
target_include_directories(etrt_obj PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/../include")
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
  ${GLOG_LIBRARIES}
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
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  COMPONENT et-runtime
  )

add_library(etrt_static STATIC $<TARGET_OBJECTS:etrt_obj>)
target_link_libraries(etrt_static ${GLOG_LIBRARIES})
target_link_libraries(etrt_static ${FMTLIB_LIBRARIES})

## Build static library
install(TARGETS etrt_static
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  COMPONENT et-runtime
  )
