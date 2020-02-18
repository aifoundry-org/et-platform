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

add_library(bootrom_helpers STATIC
  src/boot_config.c
  src/constant_memory_compare.c
  src/pcie.c
  src/pcie_int.c
  src/ringbuffer.c
  src/backtrace.c
  )

target_include_directories(bootrom_helpers PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/esperanto-fw/bootrom_helpers>)

target_include_directories(bootrom_helpers PRIVATE
                                ${SHARED_INC_DIRS})
install(
  TARGETS bootrom_helpers
  EXPORT esperanto-fw-targets
  ARCHIVE
    DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw
    COMPONENT boorom_helpers
  PUBLIC_HEADER
    DESTINATION ${INCLUDE_INSTALL_DIR}/esperanto-fw/bootrom_helpers
    COMPONENT bootrom_helpers
)

install(
  FILES
    include/boot_config.h
    include/constant_memory_compare.h
    include/pcie.h
    include/pcie_int.h
    include/mailbox_common.h
    include/ringbuffer_common.h
    include/ringbuffer.h
    include/mailbox_id.h
  DESTINATION
    ${INCLUDE_INSTALL_DIR}/esperanto-fw/bootrom_helpers
  COMPONENT bootrom_helpers
  )


add_library(fw_helpers STATIC
  src/log.c
  src/message.c
  src/printf.c
  )
target_include_directories(fw_helpers PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/esperanto-fw/fw-helpers>"
  )

install(
  TARGETS fw_helpers
  EXPORT esperanto-fw-targets
  ARCHIVE
    DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw
    COMPONENT fw_helpers
  PUBLIC_HEADER
    DESTINATION ${INCLUDE_INSTALL_DIR}/esperanto-fw/fw-helpers
    COMPONENT fw-helpers
  )
 install(
   FILES
    include/atomic.h
    include/cacheops.h
    include/common.h
    include/ec_domain_parameters.h
    include/esr_defines.h
    include/fcc.h
    include/flb.h
    include/hal_device.h
    include/hart.h
    include/key_derivation_data.h
    include/kernel_params.h
    include/layout.h
    include/log.h
    include/log_levels.h
    include/macros.h
    include/minion_esr_defines.h
    include/movellus_hpdpll_modes_config.h
    include/printf.h
    include/serial.h
    include/service_processor_ROM_data.h
    include/service_processor_spi_flash.h
    include/sp_otp_data_layout.h
    include/spio_DW_apb_ssi_config.h
    include/syscall.h
    include/tensor.h
    include/trap_handler.h
    include/vpu.h
  DESTINATION
    ${INCLUDE_INSTALL_DIR}/esperanto-fw/fw-helpers
  COMPONENT fw-types
  )

install(
  DIRECTORY
    include/etsoc_hal
  DESTINATION
    ${INCLUDE_INSTALL_DIR}/esperanto-fw/fw-helpers
  COMPONENT fw-types
  )
