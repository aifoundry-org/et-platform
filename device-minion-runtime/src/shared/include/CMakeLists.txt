#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

install(FILES host_message.h
              kernel_info.h
              kernel_params.h
              layout.h
              log_levels.h
              mailbox_id.h
              mailbox_common.h
              ringbuffer_common.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/esperanto-fw
  COMPONENT master-minion)


add_library(vaultip INTERFACE)
target_include_directories(vaultip INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/vaultip>"
  )

install(
  TARGETS vaultip
  EXPORT esperanto-fw-targets
  LIBRARY
    DESTINATION lib
    COMPONENT vaultip
  PUBLIC_HEADER
    DESTINATION ${CMAKE_INSTALL_INCLUDE_DIR}/esperanto-fw/vaultip
    COMPONENT vaultip
  )
install(
  FILES
    vaultip_hw.h
    vaultip_static_assets.h
    vaultip_sw.h
    vaultip_sw_asset.h
  DESTINATION
    ${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/vaultip
  COMPONENT vaultip
  )
