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

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#include "etsoc/isa/utils.h"
#include "etsoc/isa/esr_defines.h"

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
    fcc_send(SHIRE_OWN, THREAD_0, fcc, minion_mask_t0);
    fcc_send(SHIRE_OWN, THREAD_1, fcc, minion_mask_t1);
  }
  fcc_consume(fcc);

  return last;
}

#ifdef __cplusplus
}
#endif

#endif

