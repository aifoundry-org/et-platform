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

# List of subdirectories with our sources
set(SUBFOLDERS
  CodeManagement
  Core
  DeviceAPI
  DeviceFW
  PCIEDevice
  RPCDevice
  Support
  Tracing
  )


foreach(FOLDER ${SUBFOLDERS})
  add_subdirectory(${FOLDER})
endforeach()

## Add top-level target to capture all generated code
add_custom_target(etrt-gen-code DEPENDS
  etrt-tracing-gen-code
  etrt-device-api-gen-code
  )

## Add a dependency to all libraries on the generated code
## as they could be consuming it
foreach(FOLDER ${SUBFOLDERS})
  add_dependencies(etrt_${FOLDER}
    etrt-gen-code
    )
endforeach()

# FIXME device-api disabled in esperanto-soc because of cloning permision
# issues
# ExternalProject_Add(device-api
#  PREFIX device-api
#  GIT_REPOSITORY git@gitlab:software/device-api.git
#  GIT_TAG 29f910a5744dfdfe1f0f5e408382117b5cb6f44d
#  )

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/RPCDevice/TargetDeviceInfo.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/RPCDevice/TargetDeviceInfo.h"
  )

set(ETRT_SOURCES
  ${PROJECT_BINARY_DIR}/include/esperanto/runtime/Common/ProjectAutogen.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Support/Logging.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Support/ErrorOr.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Support/STLHelpers.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Support/RuntimeErrors.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Support/HelperMacros.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Support/DeviceGuard.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Support/MemoryRange.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/etrt-bin.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Common/ProjectAutogen.h.in
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Common/ErrorTypes.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Common/CommonTypes.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/C-API/etrt.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/CommandLineOptions.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/CommandLineOptionsDef.in
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/Device.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/DeviceTarget.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/DeviceManager.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/DeviceInformation.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/Event.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/Error.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/Kernel.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/Memory.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/MemoryManager.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/Stream.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/DeviceAPI
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/DeviceAPI/Response.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/DeviceAPI/Event.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/DeviceAPI/Command.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/EsperantoRuntime.h
  # FIXME the following should be deprecated once we move to the simulator API
  C-API/etrt.cc
  demangle.h
  etlibdevice.h
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
add_dependencies(etrt_obj
  ${external_project_dependencies}
  etrt-gen-code
  )
link_directories(${EXTERNAL_LINK_DIRECTORIES}
                 ${SIMULATOR_API_LIB_DIR})


set_property(TARGET etrt_obj PROPERTY POSITION_INDEPENDENT_CODE 1)

# Construct list of object files from the object libraries to be linked
# together in the runtime static and dynamic libraries
set(LIB_OBJS "")
foreach(FOLDER ${SUBFOLDERS})
  list(APPEND LIB_OBJS $<TARGET_OBJECTS:etrt_${FOLDER}>)
endforeach()

# Generate both static and shared libraries
add_library(etrt SHARED $<TARGET_OBJECTS:etrt_obj> ${LIB_OBJS})
target_link_libraries(etrt
  absl::flags
  absl::flags_internal
  absl::flags_parse
  ${GLOG_LIBRARIES}
  ${GFLAGS_LIBRARIES}
  ${FMTLIB_LIBRARIES}
  etsim_api
  stdc++fs
  )
target_link_directories(etrt PUBLIC
                               ${EXTERNAL_LINK_DIRECTORIES}
                               ${SIMULATOR_API_LIB_DIR})

## Build shared library
install(TARGETS etrt
  EXPORT EsperantoRuntimeTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  COMPONENT et-runtime
  )

add_library(etrt_static STATIC $<TARGET_OBJECTS:etrt_obj> ${LIB_OBJS})

message("GLOG ${GLOG_LIBRARIES}")

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
  ${GFLAGS_LIBRARIES}
  ${FMTLIB_LIBRARIES}
  etsim_api_static
  stdc++fs
  )

target_link_directories(etrt_static PUBLIC
                               ${EXTERNAL_LINK_DIRECTORIES}
                               ${SIMULATOR_API_LIB_DIR})


## Build static library
install(TARGETS etrt_static
  EXPORT EsperantoRuntimeTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  COMPONENT et-runtime
  )
