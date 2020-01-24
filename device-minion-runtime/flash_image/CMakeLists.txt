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
  configure_file(gen_opt.sh.in gen_opt.sh)
  configure_file(flash_16Mbit_template.json.in flash_16Mbit_template.json)

  set(OTP_ARTIFACTS
    ${CMAKE_CURRENT_BINARY_DIR}/zebu_sp_otp_3.hex
    ${CMAKE_CURRENT_BINARY_DIR}/zebu_sp_otp_4.hex
    ${CMAKE_CURRENT_BINARY_DIR}/zebu_sp_otp_5.hex)

  # Gen the OTP file
  add_custom_target(gen-otp
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/gen_opt.sh
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/gen_opt.sh
            ${PROJECT_SOURCE_DIR}/src/ServiceProcessorROM/scripts/sp_otp_editor.py
            ${PROJECT_SOURCE_DIR}/src/ServiceProcessorROM/scripts/sp_otp_template.json
            ${PROJECT_SOURCE_DIR}/src/sp_otp_convert.py
    BYPRODUCTS ${OTP_ARTIFACTS}
    )

  install(FILES ${OTP_ARTIFACTS}
          DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/otp
          COMPONENT zebu)

  # Given that all depednencies for generating the flash image cannot be checked out
  # in all configurations of device-fw, do not add the following target to the ALL
  # list.
  add_custom_target(gen-flash-image
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/sign_flash_image.sh
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/sign_flash_image.sh
            ${CMAKE_CURRENT_BINARY_DIR}/flash_16Mbit_template.json
            ${EXTERNAL_PROJECT_DEPENDENCIES}
            ${FLASH_IMAGE_DEPS}
            gen-otp
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/flash_16Mbit.hex

    )

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/flash_16Mbit.hex
          DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/flash
          COMPONENT zebu)

endif(BUILD_FLASH_IMAGE)
