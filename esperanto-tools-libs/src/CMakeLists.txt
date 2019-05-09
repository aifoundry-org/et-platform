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


#set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined" )

# FIXME : disable for now hidden visibility by default: -fvisibility=hidden -fvisibility-inlines-hidden
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++17 -Wall -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -fPIC -Wall")

set(ETRT_SOURCES
  # FIXME the following should be deprecated once we move to the simulator API
    et_socket_addr.cc
    etrpc/backend.cc
    etrpc/backend-sysemu.cc
    etrpc/et-card-proxy.cc
    etrpc/card-emu.cc
    etrpc/et-rpc.cc
    # --------
    C-API/etrt.cc
    BLAS/etblas.cc
    DNN/etdnn.cc
    Core/Commands.cc
    Core/DeviceManager.cc
    Core/Error.cc
    Core/MemoryManager.cc
    Core/Device.cc
    Core/et_stream.cc
    Core/et_event.cc
    Core/registry.cc
    Device/DeviceTarget.cc
    Device/PCIeDevice.cc
    Device/TargetCardProxy.cc
    Device/TargetRPC.cc
    Device/TargetSysEmu.cc
    cpu_algo.h
    registry.h
    eti.h
    ComputeCommon/eti.cc
    Support/DeviceGuard.cc
    Target/et_bootrom.cc
#     et_socket_addr.cc
)
if(BUILD_KERNELS)
  list(APPEND ETRT_SOURCES
    Cpu/cpu_algo.cc
    Cpu/cpu_algo_launch.cc
    )
endif(BUILD_KERNELS)

add_library(etrt SHARED ${ETRT_SOURCES})
target_include_directories(etrt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(etrt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_include_directories(etrt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Device")
target_include_directories(etrt PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/")
target_include_directories(etrt PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/..")
target_include_directories(etrt PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/../include")

MESSAGE("GLOG: " ${GLOG_INCLUDE_DIRS})

target_include_directories(etrt PUBLIC ${GLOG_INCLUDE_DIRS})
target_include_directories(etrt PUBLIC ${FMTLIB_INCLUDE_DIRS})
target_link_libraries(etrt pthread)
add_dependencies(etrt ${external_project_dependencies})
target_link_libraries(etrt ${GLOG_LIBRARIES})
target_link_libraries(etrt ${FMTLIB_LIBRARIES})
target_link_libraries(etrt ${GFLAGS_LIBRARIES})

if(BUILD_KERNELS)
  add_dependencies(etrt kernels-elf-file)
  add_dependencies(etrt bootrom-file)
  add_dependencies(etrt kernels-offsets)
  add_dependencies(etrt kernels-cdescr)
  add_dependencies(etrt bootrom-cdescr)
endif(BUILD_KERNELS)


set(ETCUDA_SOURCES
    ETCuda/interceptors.cc
)

add_library(etcuda SHARED ${ETCUDA_SOURCES})
target_include_directories(etcuda PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_include_directories(etcuda PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/")
target_link_libraries(etcuda etrt)
target_link_libraries(etcuda dl)


install(TARGETS etrt
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  COMPONENT et-runtime
  )
