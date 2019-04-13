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


if (HOST_SW)
  # HACK the following should be removed in the future and the runtime should have
  # no dependency on the binary artifacts.
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/artifacts.done
      ${CMAKE_CURRENT_BINARY_DIR}/esperanto-soc-artifacts/artifacts/sdk-build/build/libetrpc.a
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../../tools/artifacts/manifest.json
       ${CMAKE_CURRENT_SOURCE_DIR}/../../tools/artifacts/getArtifacts.py
    COMMAND rm -rf esperanto-soc-artifacts && mkdir -p esperanto-soc-artifacts
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../../tools/artifacts/getArtifacts.py ${CMAKE_CURRENT_BINARY_DIR}/esperanto-soc-artifacts
    COMMAND date > ${CMAKE_CURRENT_BINARY_DIR}/artifacts.done
    COMMENT "Download the build artifacts"
    VERBATIM)
  add_custom_target(download_esperanto_soc_artifacts
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/artifacts.done)

else(HOST_SW)

    # FIXME Remove external dependency to the SDK
    ExternalProject_Add(sdk
      SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk
      BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/sdk
      INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/sdk-install
      CMAKE_ARGS all
      INSTALL_COMMAND ""
      )
    LINK_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/sdk)

endif()

option(BUILD_SHARED_LIBS "Build shared libs." ON)
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type selected, default to Debug")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type." FORCE)
endif()

if (HOST_SW)
  add_library(etrpc STATIC IMPORTED)
  set_property(TARGET etrpc
    PROPERTY IMPORTED_LOCATION
    ${CMAKE_CURRENT_BINARY_DIR}/esperanto-soc-artifacts/artifacts/sdk-build/build/libetrpc.a)
  add_dependencies(etrpc download_esperanto_soc_artifacts)
endif(HOST_SW)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden -fPIC -std=c++11 -Wall -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -fPIC -Wall")

set(ETRT_SOURCES
    C-API/etrt.cc
    BLAS/etblas.cc
    DNN/etdnn.cc
    Core/Commands.cc
    Core/Error.cc
    Core/MemoryManager.cc
    Core/et_device.cc
    Core/et_stream.cc
    Core/et_event.cc
    Core/registry.cc
    Device/DeviceTarget.cc
    Device/PCIeDevice.cc
    Device/TargetCardProxy.cc
    Device/TargetRPC.cc
    Device/TargetSysEmu.cc
    et_device.h
    et_stream.h
    et_event.h
    Cpu/cpu_algo.cc
    Cpu/cpu_algo_launch.cc
    cpu_algo.h
    registry.h
    eti.h
    ComputeCommon/eti.cc
    Target/et_bootrom.cc
    et_socket_addr.cc
)
add_library(etrt SHARED ${ETRT_SOURCES})
target_include_directories(etrt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(etrt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_include_directories(etrt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Device")
target_include_directories(etrt PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/..")
target_include_directories(etrt PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/../include")

MESSAGE("GLOG: " ${GLOG_INCLUDE_DIRS})

target_include_directories(etrt PUBLIC ${GLOG_INCLUDE_DIRS})
if (HOST_SW)
  target_include_directories(etrt PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/esperanto-soc-artifacts/artifacts/sdk-build/build")
else()
  target_include_directories(etrt PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../sdk")
  add_dependencies(etrt sdk)
endif(HOST_SW)
target_link_libraries(etrt pthread)
target_link_libraries(etrt etrpc)
add_dependencies(etrt kernels-elf-file)
add_dependencies(etrt bootrom-file)
add_dependencies(etrt glog)
target_link_libraries(etrt ${GLOG_LIBRARIES})
add_dependencies(etrt kernels-offsets)
add_dependencies(etrt kernels-cdescr)
add_dependencies(etrt bootrom-cdescr)

set(ETCUDA_SOURCES
    ETCuda/interceptors.cc
)

add_library(etcuda SHARED ${ETCUDA_SOURCES})
target_include_directories(etcuda PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_include_directories(etcuda PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/")
target_link_libraries(etcuda etrt)
target_link_libraries(etcuda dl)


install(TARGETS etrt
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
install(DIRECTORY include DESTINATION ".")
