/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------
 */

#include <stdint.h>
#include <stddef.h>
#include "etsoc/isa/hart.h"
#include "etsoc/common/utils.h"

typedef struct {
  uint64_t base_addr;
  uint64_t num_minions;
  uint64_t num_cache_lines;
} Parameters;

int64_t main(const Parameters *const kernel_params_ptr)
{
  /* TODO: Define the body of the kernel */
  et_printf("Hart[%d]:Kernel Param:base_addr:%ld\r\n", get_hart_id(), kernel_params_ptr->base_addr);
  et_printf("Hart[%d]:Kernel Param:num_minions:%ld\r\n", get_hart_id(), kernel_params_ptr->num_minions);
  et_printf("Hart[%d]:Kernel Param:num_cache_lines:%ld\r\n", get_hart_id(), kernel_params_ptr->num_cache_lines);

  return 0;
}
