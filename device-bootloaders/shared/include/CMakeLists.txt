#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

# Download the etsoc_hard headers from artifactory

set(ETSOC_HAL ${CMAKE_CURRENT_BINARY_DIR}/etsoc_hal.tgz)
set(ETSOC_HAL_COMMIT 1466661a9a55042cda18df7417b47f8a1157e3f8)
set(ETSOC_HAL_MD5 8699c869debd21411c33a81f6402a93a)
set(ETSOC_HAL_URL http://sc-artifactory1.esperanto.ai/artifactory/sw-ci-inputs/HAL/${ETSOC_HAL_COMMIT}/etsoc_hal.tgz)

# Download and extract the etsoc_hal files in the current folder
execute_process(
  COMMAND  wget -c --retry-connrefused --tries=400 --timeout=15 -nv ${ETSOC_HAL_URL} -O ${ETSOC_HAL}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  RESULT_VARIABLE ret
  )

if(NOT ret EQUAL "0")
  message(FATAL_ERROR "Failed to download the etsoc_hal headers: ${ret}")
endif()

execute_process(
  COMMAND bash "-c" "md5sum ${ETSOC_HAL} | awk '{print $1}'"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  RESULT_VARIABLE ret
  OUTPUT_VARIABLE output
  )

if(NOT ret EQUAL "0")
  message(FATAL_ERROR "failed md5sum: ${ret} ${ETSOC_HAL}")
endif()

message("output : ${output}")

if(NOT output EQUAL ${ETSOC_HAL_MD5})
  message(FATAL_ERROR "md5sum does not match ${output} ${ETSOC_HAL_MD5}")
endif()

execute_process(
  COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/etsoc_hal
  COMMAND tar -zxf ${ETSOC_HAL} -C ${CMAKE_CURRENT_BINARY_DIR}/etsoc_hal
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  RESULT_VARIABLE ret
  )

if(NOT ret EQUAL "0")
  message(FATAL_ERROR "Failed to extract the etsoc_hal headers: ${ret}")
endif()
