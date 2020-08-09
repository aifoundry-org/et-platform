#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

add_library(etrt_DeviceManagement OBJECT
  DeviceManagement.cc
  )

target_include_directories(etrt_DeviceManagement PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_BINARY_DIR}/src
)

link_directories(${EXTERNAL_LINK_DIRECTORIES})

# Generate shared library
set(LIB_OBJS "")

# List of common library dependencies
set(COMMON_LIBRARY_DEPENDENCIES "etrt")
add_library(DM SHARED $<TARGET_OBJECTS:etrt_DeviceManagement> ${LIB_OBJS})
target_link_libraries(DM PUBLIC ${COMMON_LIBRARY_DEPENDENCIES})

target_link_directories(DM PUBLIC ${EXTERNAL_LINK_DIRECTORIES})

## Build shared library
install(TARGETS DM
        EXPORT EsperantoRuntimeTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        COMPONENT et-runtime
)

target_include_directories(etrt_DeviceManagement PRIVATE "${ESPERANTO_DEVICE_MINION_RUNTIME_INCLUDE_DIR}/")

add_sanitizers(etrt_DeviceManagement)
