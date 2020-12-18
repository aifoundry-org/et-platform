#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

add_subdirectory("include")

add_library(firmware_helpers STATIC
  src/pcie_int.c
  src/ringbuffer.c
  src/backtrace.c
  src/circbuffer.c
  src/circbuff.c
  src/vq.c
  )

target_include_directories(firmware_helpers PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/esperanto-fw/firmware_helpers>)

target_include_directories(firmware_helpers PRIVATE
  ${SHARED_INC_DIRS})

install(
  TARGETS firmware_helpers
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
    include/mailbox_common.h
    include/mailbox_id.h
    include/minion_esr_defines.h
    include/minion_fw_boot_config.h
    include/pcie_device.h
    include/pcie_int.h
    include/ringbuffer_common.h
    include/ringbuffer.h
    include/trap_handler.h
    include/circbuff.h
    include/circbuffer_common.h
    include/circbuffer.h
    include/vqueue_common.h
    include/vq.h
  DESTINATION
    ${INCLUDE_INSTALL_DIR}/esperanto-fw/firmware_helpers
  COMPONENT firmware_helpers
  )
