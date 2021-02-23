#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

#TODO this should be improved, make use of modern cmake and not hardcode compiler flags. THis is a mess, the compiler gets flags repeated with no control.
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined" )
# FIXME : disable for now hidden visibility by default: -fvisibility=hidden -fvisibility-inlines-hidden
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++17 -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g3 -O0 -Werror")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -fPIC -Wall")

set(SUBFOLDERS
    NewCore
)

foreach(FOLDER ${SUBFOLDERS})
    add_subdirectory(${FOLDER})
endforeach()

# Construct list of object files from the object libraries to be linked
# together in the runtime static and dynamic libraries
set(LIB_OBJS "")
foreach(FOLDER ${SUBFOLDERS})
    list(APPEND LIB_OBJS $<TARGET_OBJECTS:etrt_${FOLDER}>)
endforeach()

# List of common library dependencies
set(COMMON_LIBRARY_DEPENDENCIES
    glog
    gflags
    stdc++fs
    fmt::fmt
)

if(SANITIZE_ADDRESS)
    list(APPEND COMMON_LIBRARY_DEPENDENCIES asan)
elseif(SANITIZE_THREAD)
    list(APPEND COMMON_LIBRARY_DEPENDENCIES tsan)
elseif(SANITIZE_UNDEFINED)
    list(APPEND COMMON_LIBRARY_DEPENDENCIES ubsan)
endif()

# Generate both static and shared libraries
add_library(etrt SHARED ${LIB_OBJS})

target_include_directories(etrt
 PUBLIC
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
   $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(etrt
    ${COMMON_LIBRARY_DEPENDENCIES}
    -Wl,--whole-archive
    deviceLayer::deviceLayer
    sw-sysemu::sw-sysemu
    -Wl,--no-whole-archive
)

target_link_directories(etrt PUBLIC
    ${EXTERNAL_LINK_DIRECTORIES}
)

## Build shared library
install(TARGETS etrt
  EXPORT EsperantoRuntimeTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  COMPONENT et-runtime
  )

add_library(etrt_static STATIC ${LIB_OBJS})

target_include_directories(etrt_static
 PUBLIC
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
   $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(etrt_static
    ${COMMON_LIBRARY_DEPENDENCIES}
    deviceLayer::deviceLayer
    sw-sysemu::sw-sysemu
)

target_link_directories(etrt_static PUBLIC
    ${EXTERNAL_LINK_DIRECTORIES}
)

## Build static library
install(TARGETS etrt_static
  EXPORT EsperantoRuntimeTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  COMPONENT et-runtime
  )
