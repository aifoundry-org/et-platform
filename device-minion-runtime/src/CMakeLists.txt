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
set(SHARED_INC_DIRS ${SHARED_INC_DIR} ${SHARED_INC_DIR}/device-common ${SHARED_HAL_DIR})

add_subdirectory(shared)
add_subdirectory(MachineMinion)
add_subdirectory(MasterMinion)
add_subdirectory(WorkerMinion)
