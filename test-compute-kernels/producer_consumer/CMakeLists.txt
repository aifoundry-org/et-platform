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
  NAME producer_consumer
  NUMBER_OF_TESTS 9
  GEN_COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/gen_random_params.py --shires 2 --scenario producer_consumer --target_dir include --output_type array --repeat 4
  OUTPUT_GEN_FILES random_seed
                   include/tstore_configs.h
                   include/tfma_configs.h
                   include/tl0_configs.h
                   include/tl1_configs.h
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/producer_consumer.c
  )
