#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

get_filename_component(SHARED_SRC_DIR "shared/src" ABSOLUTE)
get_filename_component(SHARED_INC_DIR "shared/include" ABSOLUTE)
get_filename_component(SHARED_HAL_DIR "shared/include/etsoc_hal" ABSOLUTE)
set(SHARED_INC_DIRS ${SHARED_INC_DIR} ${SHARED_HAL_DIR})

#FreeRTOS
set(FREERTOS_DIR "shared/FreeRTOS")

#FreeRTOS source
file(GLOB FREERTOS_SRC_FILES "${FREERTOS_DIR}/Source/*.c"
                             "${FREERTOS_DIR}/Source/portable/GCC/RISC-V/*.c"
                             "${FREERTOS_DIR}/Source/portable/GCC/RISC-V/*.S")
                             #"${FREERTOS_DIR}/Source/mem_mang/heap_3.c" not using FreeRTOS heap. Heap is only for libc.

#FreeRTOS includes
get_filename_component(FREERTOS_INC_DIR "${FREERTOS_DIR}/Source/include/" ABSOLUTE)
get_filename_component(FREERTOS_PORT_INC_DIR "${FREERTOS_DIR}/Source/portable/GCC/RISC-V/" ABSOLUTE)
get_filename_component(FREERTOS_CHIP_SPECIFIC_INC_DIR "${FREERTOS_DIR}/Source/portable/GCC/RISC-V/chip_specific_extensions/etsoc_RV64IMF" ABSOLUTE)
set(FREERTOS_INC_DIRS ${FREERTOS_INC_DIR} ${FREERTOS_PORT_INC_DIR} ${FREERTOS_CHIP_SPECIFIC_INC_DIR})

add_subdirectory("Tracing")
add_subdirectory("shared/")
add_subdirectory("MachineMinion")
add_subdirectory("MasterMinion")
add_subdirectory("WorkerMinion")
