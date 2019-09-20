#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

get_filename_component(SHARED_KERNEL_SRC_DIR "shared" ABSOLUTE)
get_filename_component(SHARED_KERNEL_INC_DIR "shared/include" ABSOLUTE)

add_subdirectory("beef")
add_subdirectory("crc32")
add_subdirectory("empty")
add_subdirectory("hang")
add_subdirectory("log")
add_subdirectory("power")
add_subdirectory("random_read")
