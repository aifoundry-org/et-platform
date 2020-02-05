#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

randomized_test(
  NAME tl_tfma_tstore
  NUMBER_OF_TESTS 9
  GEN_COMMAND python3 ${SCRIPTS_KERNEL_DIR}/gen_random_params.py --shires 3 --scenario tload_tfma_tstore --target_dir include --output_type array --repeat 10
  OUTPUT_GEN_FILES random_seed
                   include/tstore_configs.h
                   include/tfma_configs.h
                   include/tl0_configs.h
                   include/tl1_configs.h
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tl_tfma_tstore.c
  )
