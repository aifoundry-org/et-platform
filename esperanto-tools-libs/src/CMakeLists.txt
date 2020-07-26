#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined" )
# FIXME : disable for now hidden visibility by default: -fvisibility=hidden -fvisibility-inlines-hidden
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++17 -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g3 -O0 -Werror")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -fPIC -Wall")

# List of subdirectories with our sources
set(SUBFOLDERS
  CodeManagement
  Core
  DeviceAPI
  DeviceFW
  MemoryManagement
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
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/CodeManagement/Kernel.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/CodeManagement/Library.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/CodeManagement/CodeRegistry.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/CodeManagement/UberKernel.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/CommandLineOptions.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/CommandLineOptionsDef.in
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/Device.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/DeviceTarget.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/DeviceManager.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/DeviceInformation.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/Event.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/Error.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/Memory.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/MemoryManager.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/Core/Stream.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/DeviceAPI
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/DeviceAPI/Response.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/DeviceAPI/Event.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/DeviceAPI/Command.h
  ${PROJECT_SOURCE_DIR}/include/esperanto/runtime/EsperantoRuntime.h
  demangle.h
  etlibdevice.h
  )

add_library(etrt_obj OBJECT ${ETRT_SOURCES})

# Include path dependencies
# target_include_directories(etrt_obj PRIVATE "${ESPERANTO_FW_INCLUDE_DIR}/esperanto-fw")

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

# List of common library dependencies
set(COMMON_LIBRARY_DEPENDENCIES
  absl::flags
  absl::flags_internal
  absl::flags_parse
  glog
  gflags
  etsim_api
  stdc++fs
  fmt::fmt
 )

if(SANITIZE_ADDRESS)
  list(APPEND COMMON_LIBRARY_DEPENDENCIES asan)
elseif(SANITIZE_THREAD)
  list(APPEND COMMON_LIBRARY_DEPENDENCIES  tsan)
elseif(SANITIZE_UNDEFINED)
  list(APPEND COMMON_LIBRARY_DEPENDENCIES ubsan)
endif()

# Generate both static and shared libraries
add_library(etrt SHARED $<TARGET_OBJECTS:etrt_obj> ${LIB_OBJS})
target_link_libraries(etrt ${COMMON_LIBRARY_DEPENDENCIES})
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
target_link_libraries(etrt_static ${COMMON_LIBRARY_DEPENDENCIES}

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
