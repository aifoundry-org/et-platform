#include "etsoc/isa/hart.h"
#include "etsoc/isa/cacheops.h"
#include "etsoc/common/utils.h"
#include "common.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BASE_ADDR_FOR_THIS_TEST  0x8200000000ULL

// tensor_a is destination of the cacheop
// tensor_b is stride value
// tensor_c is used to choose b/w flush and evict
// Each shire accesses the memshire that is closest to it

typedef struct {
  uint64_t dst;
  uint64_t stride;
  uint64_t op;
} Parameters;
int64_t entry_point(const Parameters* const kernel_params_ptr) {
  if ((kernel_params_ptr == NULL)) {
    // Bad arguments
    et_printf( "Programming returing due to error\n");
    return -1;
  }

  uint64_t hart_id = get_hart_id();
  uint64_t shire_id = ((hart_id >> 6) & 0x3F);
  uint64_t minion_id = ((hart_id >> 1) & 0x1F);
  int64_t N_TIMES = 1;
  uint64_t dst = kernel_params_ptr->dst;        // 1 or 2 or 3
  uint64_t stride = kernel_params_ptr->stride;  // 1024, 512, 256, 128
  uint64_t op = kernel_params_ptr->op;          // 1 is flush, 2 is evict

  // if ((hart_id & 1) == 1) //Only Thread1 (or prefetch threads to do prefetch)
  if (hart_id == 1)  // Only Thread1 (or prefetch threads to do prefetch)
  {
    for (int i = 0; i < N_TIMES;
         i++)  // N_TIMES = # of times the thread1 performs the prefetch CSR
    {
      volatile uint64_t VA;
      // VA Address to prefetch from
      //                                |39-32= 0x80|    |31-28= 0xa|   |27-23 |
      //                                |22-18 |            |17-13 = i factor|
      //                                |9-6= MC/MS/L2Bank|

      /****** Prefetches from these shires should get data always form mem-sire
       * 0*******/
      if ((shire_id == 0) || (shire_id == 1) || (shire_id == 16) ||
          (shire_id == 8) || (shire_id == 12) || (shire_id == 24)) {
        VA = (uint64_t)(((0x82ULL) << 32) + ((0xaU) << 28U) + (shire_id << 23) +
                        (minion_id << 18) + ((0x01 * (uint64_t)i) << 13) +
                        (0x0 << 6));
      }
      /****** Prefetches from these shires should get data always form mem-sire
         1*******/
      else if ((shire_id == 9) || (shire_id == 17) || (shire_id == 25)) {
        VA = (uint64_t)(((0x82ULL) << 32) + ((0xaU) << 28U) + (shire_id << 23) +
                        (minion_id << 18) + ((0x01 * (uint64_t)i) << 13) +
                        (0x1 << 6));
      }
      /****** Prefetches from these shires should get data always form mem-sire
         2*******/
      else if ((shire_id == 2) || (shire_id == 18) || (shire_id == 26)) {
        VA = (uint64_t)(((0x82ULL) << 32) + ((0xaU) << 28U) + (shire_id << 23) +
                        (minion_id << 18) + ((0x01 * (uint64_t)i) << 13) +
                        (0x2 << 6));
      }
      /****** Prefetches from these shires should get data always form mem-sire
         3*******/
      else if ((shire_id == 10) || (shire_id == 3) || (shire_id == 11) ||
               (shire_id == 19) || (shire_id == 27) || (shire_id == 32)) {
        VA = (uint64_t)(((0x82ULL) << 32) + ((0xaU) << 28U) + (shire_id << 23) +
                        (minion_id << 18) + ((0x01 * (uint64_t)i) << 13) +
                        (0x3 << 6));
      }
      /****** Prefetches from these shires should get data always form mem-sire
         4*******/
      else if ((shire_id == 5) || (shire_id == 20) || (shire_id == 28) ||
               (shire_id == 4)) {
        VA = (uint64_t)(((0x82ULL) << 32) + ((0xaU) << 28U) + (shire_id << 23) +
                        (minion_id << 18) + ((0x01 * (uint64_t)i) << 13) +
                        (0x4 << 6));
      }
      /****** Prefetches from these shires should get data always form mem-sire
         5*******/
      else if ((shire_id == 13) || (shire_id == 21) || (shire_id == 29)) {
        VA = (uint64_t)(((0x82ULL) << 32) + ((0xaU) << 28U) + (shire_id << 23) +
                        (minion_id << 18) + ((0x01 * (uint64_t)i) << 13) +
                        (0x5 << 6));
      }
      /****** Prefetches from these shires should get data always form mem-sire
         6*******/
      else if ((shire_id == 6) || (shire_id == 22) || (shire_id == 30)) {
        VA = (uint64_t)(((0x82ULL) << 32) + ((0xaU) << 28U) + (shire_id << 23) +
                        (minion_id << 18) + ((0x01 * (uint64_t)i) << 13) +
                        (0x6 << 6));
      }
      /****** Prefetches from these shires should get data always form mem-sire
         7*******/
      else if ((shire_id == 14) || (shire_id == 15) || (shire_id == 23) ||
               (shire_id == 7) || (shire_id == 31)) {
        VA = (uint64_t)(((0x82ULL) << 32) + ((0xaU) << 28U) + (shire_id << 23) +
                        (minion_id << 18) + ((0x01 * (uint64_t)i) << 13) +
                        (0x7 << 6));
      } else  // defualt just use mem-shire 0
      {
        VA = (uint64_t)(((0x82ULL) << 32) + ((0xaU) << 28U) + (shire_id << 23) +
                        (minion_id << 18) + ((0x01 * (uint64_t)i) << 13) +
                        (0x0 << 6));
      }

      // Prefetch performed by one minion from each shire based on VA formed
      prefetch_va(false, 0, (uint64_t)(VA), 15, 1024, 0);
      // WAIT_PREFETCH_0;
    }

    for (int i = 0; i < N_TIMES;
         i++)  // N_TIMES = # of times the thread1 performs the prefetch CSR
    {
      volatile uint64_t VA;
      // VA Address to prefetch from
      //                                |39-32= 0x80|    |31-28= 0xa|   |27-23 |
      //                                |22-18 |            |17-13 = i factor|
      //                                |9-6= MC/MS/L2Bank|

      /****** Prefetches from these shires should get data always form mem-sire
       * 0*******/
      if ((shire_id == 0) || (shire_id == 1) || (shire_id == 16) ||
          (shire_id == 8) || (shire_id == 12) || (shire_id == 24)) {
        VA = (uint64_t)(((0x82ULL) << 32) + ((0xaU) << 28U) + (shire_id << 23) +
                        (minion_id << 18) + ((0x01 * (uint64_t)i) << 13) +
                        (0x0 << 6));
      }
      /****** Prefetches from these shires should get data always form mem-sire
         1*******/
      else if ((shire_id == 9) || (shire_id == 17) || (shire_id == 25)) {
        VA = (uint64_t)(((0x82ULL) << 32) + ((0xaU) << 28U) + (shire_id << 23) +
                        (minion_id << 18) + ((0x01 * (uint64_t)i) << 13) +
                        (0x1 << 6));
      }
      /****** Prefetches from these shires should get data always form mem-sire
         2*******/
      else if ((shire_id == 2) || (shire_id == 18) || (shire_id == 26)) {
        VA = (uint64_t)(((0x82ULL) << 32) + ((0xaU) << 28U) + (shire_id << 23) +
                        (minion_id << 18) + ((0x01 * (uint64_t)i) << 13) +
                        (0x2 << 6));
      }
      /****** Prefetches from these shires should get data always form mem-sire
         3*******/
      else if ((shire_id == 10) || (shire_id == 3) || (shire_id == 11) ||
               (shire_id == 19) || (shire_id == 27) || (shire_id == 32)) {
        VA = (uint64_t)(((0x82ULL) << 32) + ((0xaU) << 28U) + (shire_id << 23) +
                        (minion_id << 18) + ((0x01 * (uint64_t)i) << 13) +
                        (0x3 << 6));
      }
      /****** Prefetches from these shires should get data always form mem-sire
         4*******/
      else if ((shire_id == 5) || (shire_id == 20) || (shire_id == 28) ||
               (shire_id == 4)) {
        VA = (uint64_t)(((0x82ULL) << 32) + ((0xaU) << 28U) + (shire_id << 23) +
                        (minion_id << 18) + ((0x01 * (uint64_t)i) << 13) +
                        (0x4 << 6));
      }
      /****** Prefetches from these shires should get data always form mem-sire
         5*******/
      else if ((shire_id == 13) || (shire_id == 21) || (shire_id == 29)) {
        VA = (uint64_t)(((0x82ULL) << 32) + ((0xaU) << 28U) + (shire_id << 23) +
                        (minion_id << 18) + ((0x01 * (uint64_t)i) << 13) +
                        (0x5 << 6));
      }
      /****** Prefetches from these shires should get data always form mem-sire
         6*******/
      else if ((shire_id == 6) || (shire_id == 22) || (shire_id == 30)) {
        VA = (uint64_t)(((0x82ULL) << 32) + ((0xaU) << 28U) + (shire_id << 23) +
                        (minion_id << 18) + ((0x01 * (uint64_t)i) << 13) +
                        (0x6 << 6));
      }
      /****** Prefetches from these shires should get data always form mem-sire
         7*******/
      else if ((shire_id == 14) || (shire_id == 15) || (shire_id == 23) ||
               (shire_id == 7) || (shire_id == 31)) {
        VA = (uint64_t)(((0x82ULL) << 32) + ((0xaU) << 28U) + (shire_id << 23) +
                        (minion_id << 18) + ((0x01 * (uint64_t)i) << 13) +
                        (0x7 << 6));
      } else  // defualt just use mem-shire 0
      {
        VA = (uint64_t)(((0x82ULL) << 32) + ((0xaU) << 28U) + (shire_id << 23) +
                        (minion_id << 18) + ((0x01 * (uint64_t)i) << 13) +
                        (0x0 << 6));
      }

      if (op == 1) flush_va(false, dst, (uint64_t)(VA), 15, stride, 0);
      if (op == 2) evict_va(false, dst, (uint64_t)(VA), 15, stride, 0);
      // WAIT_CACHEOPS;
    }
    return 0;
  } else {
    return 0;
  }
}
