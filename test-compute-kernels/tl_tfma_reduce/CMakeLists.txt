#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

set(SOURCES
    ${SHARED_KERNEL_SRC_DIR}/crt.S
    ${SHARED_SRC_DIR}/log.c
    ${SHARED_SRC_DIR}/printf.c # log needs printf.c to build string
    ${SHARED_SRC_DIR}/message.c # log needs message.c to send log message to master minion
    tl_tfma_reduce.c
    )

set(TARGET_NAME tl_tfma_reduce)
set(LINKER_SCRIPT ${SHARED_KERNEL_SRC_DIR}/sections.ld)
set(ZEBU_TARGET DDR_NEW)
set(ZEBU_FILENAME memImage)

include_directories(include)
include_directories(${SHARED_INC_DIRS})
include_directories(${SHARED_KERNEL_INC_DIR})
include_directories(${ESPERANTO_DEVICE_API_INCLUDE_DIR})

add_riscv_executable(${TARGET_NAME} ${SOURCES})

install(TARGETS ${TARGET_NAME}.elf
  EXPORT esperanto-fw-targets
  RUNTIME DESTINATION ${BIN_INSTALL_DIR}/esperanto-fw/kernels
  COMPONENT kernels)

message("Tl_tfma_reduce kernel hex to install ${TARGET_NAME_OUTPUT}")

string(REPLACE " " ";" OUTPUT_LIST ${${TARGET_NAME}_OUTPUT})
foreach(FILE ${OUTPUT_LIST})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${FILE}
    DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/kernels/tl_tfma_reduce
    COMPONENT zebu)
endforeach(FILE)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mem_desc.txt ${CMAKE_CURRENT_BINARY_DIR}/mem_desc.txt)
execute_process(COMMAND python3 gen_random_params.py --shires 3 --scenario tload_tfma_reduce --target_dir include --output_type array --repeat 10
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )             
