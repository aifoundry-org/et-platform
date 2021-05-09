#------------------------------------------------------------------------------
# Copyright (C) 2021, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

try_compile(CHECK_RESULT ${CMAKE_BINARY_DIR}
    SOURCES ${PROJECT_SOURCE_DIR}/cmake/checks/mm_config_values.c
    CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${PROJECT_SOURCE_DIR}/src/MasterMinion/include;${PROJECT_SOURCE_DIR}/src/shared/firmware_helpers/include"
    OUTPUT_VARIABLE MM_CONFIG_VALUES_CHECK_OUTPUT
)

string(REGEX MATCH "DISPATCHER_BASE_HART_ID=[0-9]+" DISPATCHER_BASE_HART_ID ${MM_CONFIG_VALUES_CHECK_OUTPUT})
string(REGEX MATCH "[0-9]+" DISPATCHER_BASE_HART_ID ${DISPATCHER_BASE_HART_ID})

string(REGEX MATCH "SQW_BASE_HART_ID=[0-9]+" SQW_BASE_HART_ID ${MM_CONFIG_VALUES_CHECK_OUTPUT})
string(REGEX MATCH "[0-9]+" SQW_BASE_HART_ID ${SQW_BASE_HART_ID})

string(REGEX MATCH "KW_BASE_HART_ID=[0-9]+" KW_BASE_HART_ID ${MM_CONFIG_VALUES_CHECK_OUTPUT})
string(REGEX MATCH "[0-9]+" KW_BASE_HART_ID ${KW_BASE_HART_ID})

string(REGEX MATCH "DMAW_BASE_HART_ID=[0-9]+" DMAW_BASE_HART_ID ${MM_CONFIG_VALUES_CHECK_OUTPUT})
string(REGEX MATCH "[0-9]+" DMAW_BASE_HART_ID ${DMAW_BASE_HART_ID})

string(REGEX MATCH "DMAW_NUM=[0-9]+" DMAW_NUM ${MM_CONFIG_VALUES_CHECK_OUTPUT})
string(REGEX MATCH "[0-9]+" DMAW_NUM ${DMAW_NUM})

set(DMAW_RX_HART_ID ${DMAW_BASE_HART_ID})
math(EXPR DMAW_TX_HART_ID "${DMAW_BASE_HART_ID} + ${DMAW_NUM}")

set(SQW_0_HART_ID ${SQW_BASE_HART_ID})
math(EXPR SQW_1_HART_ID "${SQW_BASE_HART_ID} + ${DMAW_NUM}")

message(DEBUG "DISPATCHER_BASE_HART_ID ${DISPATCHER_BASE_HART_ID}")
message(DEBUG "SQW_BASE_HART_ID        ${SQW_BASE_HART_ID}")
message(DEBUG "KW_BASE_HART_ID         ${KW_BASE_HART_ID}")
message(DEBUG "DMAW_RX_HART_ID         ${DMAW_RX_HART_ID}")
message(DEBUG "DMAW_TX_HART_ID         ${DMAW_TX_HART_ID}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/threads.json.in ${CMAKE_CURRENT_BINARY_DIR}/threads.json)
