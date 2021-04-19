#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

add_subdirectory(include)
add_subdirectory(device_trace)

set(FIRMWARE_HELPERS_SOURCES
  src/pcie_int.c
  src/etsoc_memory.c
  src/circbuff.c
  src/vq.c
  )

# create a static library with MasterMinion compile time definition
add_library(mm_firmware_helpers STATIC ${FIRMWARE_HELPERS_SOURCES})
target_compile_definitions(mm_firmware_helpers PUBLIC -DMASTER_MINION=1)

target_include_directories(mm_firmware_helpers PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/esperanto-fw/firmware_helpers>)

target_include_directories(mm_firmware_helpers PRIVATE
  ${SHARED_INC_DIRS})

install(
  TARGETS mm_firmware_helpers
  EXPORT EsperantoDeviceMinionRuntimeTargets
  ARCHIVE
    DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw
    COMPONENT firmware_helpers
  PUBLIC_HEADER
    DESTINATION ${INCLUDE_INSTALL_DIR}/esperanto-fw/firmware_helpers
    COMPONENT firmware_helpers
)

# create a static library with ServiceProcessorBL2 compile time definition
add_library(sp_firmware_helpers STATIC ${FIRMWARE_HELPERS_SOURCES})
target_compile_definitions(sp_firmware_helpers PUBLIC -DSERVICE_PROCESSOR_BL2=1)

target_include_directories(sp_firmware_helpers PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/esperanto-fw/firmware_helpers>)

target_include_directories(sp_firmware_helpers PRIVATE
  ${SHARED_INC_DIRS})

install(
  TARGETS sp_firmware_helpers
  EXPORT EsperantoDeviceMinionRuntimeTargets
  ARCHIVE
    DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw
    COMPONENT firmware_helpers
  PUBLIC_HEADER
    DESTINATION ${INCLUDE_INSTALL_DIR}/esperanto-fw/firmware_helpers
    COMPONENT firmware_helpers
)

install(
  FILES
    include/device-common/atomic.h
    include/device-common/atomic-impl.h
    include/device-common/esr_defines.h
    include/layout.h
    include/minion_esr_defines.h
    include/minion_fw_boot_config.h
    include/mm_sp_cmd_spec.h
    include/pcie_device.h
    include/pcie_int.h
    include/common_defs.h
    include/circbuff.h
    include/etsoc_memory.h
    include/vq.h
  DESTINATION
    ${INCLUDE_INSTALL_DIR}/esperanto-fw/firmware_helpers
  COMPONENT firmware_helpers
  )
