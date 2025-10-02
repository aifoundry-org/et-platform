#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------


set(CFLAGS -march=rv64gc -mabi=lp64 -O2 -std=gnu11 -Wall -I. -nostartfiles -fno-common -g)
set(LFLAGS -static -nostdlib -Ttext=10000) 

set(OBJCOPY_CMD        ${CMAKE_OBJCOPY} -O binary -j .text bootrom.elf gs_maxion_bootrom.bin)
set(CONVERT_TO_HEX_CMD xxd -i gs_maxion_bootrom.bin > maxion_bootrom.h)
set(SED_ARG1           s/unsigned/static )
set(SED_ARG2           unsigned/g )
##+ custom command 
##+ consume the artifact in another bootloader build


message("HERE: ${CMAKE_C_COMPILER} ${CFLAGS} ${LFLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/bootrom.S -o bootrom.elf")

add_custom_target(maxion_boot_rom ALL
                 DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bootrom.S
                 COMMAND ${CMAKE_C_COMPILER} ${CFLAGS} ${LFLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/bootrom.S -o bootrom.elf
                 COMMAND ${OBJCOPY_CMD}
                 COMMAND ${CONVERT_TO_HEX_CMD}
                 COMMAND sed -i "${SED_ARG1} ${SED_ARG2}" maxion_bootrom.h
                 VERBATIM
                 BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/maxion_bootrom.h
                 )


