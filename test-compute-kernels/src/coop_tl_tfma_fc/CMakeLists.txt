# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

randomized_test(
  NAME coop_tl_tfma_fc
  NUMBER_OF_TESTS 4
  GEN_COMMAND python3 ${PROJECT_SOURCE_DIR}/scripts/gen_random_params.py --shires 32 --scenario coop_tload_tfma --target_dir include --output_type array --repeat 10
  OUTPUT_GEN_FILES random_seed
                   include/tfma_configs.h
                   include/tl0_configs.h
                   include/tl1_configs.h
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/coop_tl_tfma_fc.c
  )
