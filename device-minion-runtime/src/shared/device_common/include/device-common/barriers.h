/*-------------------------------------------------------------------------
* Copyright (C) 2019, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#ifndef _BARRIERS_H_
#define _BARRIERS_H_

#include <inttypes.h>

#include "device-common/utils.h"
#include "device-common/esr_defines.h"

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

// Shire-only barrier using FLBs and FCCs
inline uint64_t __attribute__((always_inline)) shire_barrier(uint64_t flb, uint64_t fcc, uint64_t thread_count, uint64_t minion_mask_t0, uint64_t minion_mask_t1)
{
  uint64_t last = flbarrier(flb, thread_count - 1);

  if (last) {
    uint64_t minion_id, thread_id;
    minion_id = get_minion_id() & (SOC_MINIONS_PER_SHIRE - 1);
    thread_id = get_thread_id();
    fcc_send(SHIRE_OWN, THREAD_0, fcc, minion_mask_t0);
    fcc_send(SHIRE_OWN, THREAD_1, fcc, minion_mask_t1);
  }
  fcc_consume(fcc);

  return last;
}

#endif

