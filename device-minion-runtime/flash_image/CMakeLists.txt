#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

if(BUILD_FLASH_IMAGE)
  set(SIGN_TOOLS_DIR ${DEVFW_EXTERNAL_INSTALL_PREFIX}/bin)
  set(FLASH_TOOLS_DIR ${DEVFW_EXTERNAL_INSTALL_PREFIX}/bin)

  configure_file(sign_flash_image.sh.in sign_flash_image.sh)
  configure_file(flash_16Mbit_template.json.in flash_16Mbit_template.json)

  message("FLASH IMAGE DEPS ${FLASH_IMAGE_DEPS}")

  # Given that all depednencies for generating the flash image cannot be checked out
  # in all configurations of device-fw, do not add the following target to the ALL
  # list.
  add_custom_target(gen-flash-image
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/sign_flash_image.sh
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/sign_flash_image.sh
    ${CMAKE_CURRENT_BINARY_DIR}/flash_16Mbit_template.json
    ${EXTERNAL_PROJECT_DEPENDENCIES}
    ${FLASH_IMAGE_DEPS}
    )

endif(BUILD_FLASH_IMAGE)
