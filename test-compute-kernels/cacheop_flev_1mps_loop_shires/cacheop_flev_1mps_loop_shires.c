#include "kernel_params.h"
#include "hart.h"
#include "cacheops.h"
#include "common.h"
#include "log.h"
#include "fcc.h"
#include "macros.h"

#include <stdint.h>
#include <stddef.h>

#define BASE_ADDR_FOR_THIS_TEST  0x8200000000ULL

#define POLYNOMIAL_BIT 0x000008012ULL 


#define LFSR_SHIFTS_PER_READ 5

static inline uint64_t generate_random_value(uint64_t lfsr) __attribute((always_inline));

       
int64_t main(const kernel_params_t* const kernel_params_ptr)
{

    if ((kernel_params_ptr == NULL))
    {
        // Bad arguments
        log_write(LOG_LEVEL_CRITICAL, "Programming returing due to error\n");
        return -1;
    }

    uint64_t lfsr_init = kernel_params_ptr->tensor_a & 0xFFFF;
    const uint64_t hart_id = get_hart_id();
    uint64_t lfsr = (((hart_id << 24) | (hart_id << 12) | hart_id) & 0x3FFFFFFFF) ^ lfsr_init;
    uint64_t lfsr_use;
    uint64_t dst = kernel_params_ptr->tensor_b; // 1 or 2 or 3 
    uint64_t op = kernel_params_ptr->tensor_c; // 1 is flush, 2 is evict 

    if (hart_id == 0 || hart_id == 64 || hart_id == 128 || hart_id == 192 || hart_id == 256 || hart_id == 320 || hart_id == 384 || hart_id == 448 || hart_id == 512 || hart_id == 576 || hart_id == 640 || hart_id == 704 || hart_id == 768 || hart_id == 832 || hart_id == 896 || hart_id == 960 || hart_id == 1024 || hart_id == 1088 || hart_id == 1152 || hart_id == 1216 || hart_id == 1280 || hart_id == 1344 || hart_id == 1408 || hart_id == 1472 || hart_id == 1536 || hart_id == 1600 || hart_id == 1664 || hart_id == 1728 || hart_id == 1792 || hart_id == 1856 || hart_id == 1920 || hart_id == 1984) {
       
       lfsr = generate_random_value(lfsr);
       lfsr_use = lfsr & 0x1F; 
       for(int i=0;i<31;i++) {
          //WAIT_FCC(0);
          long unsigned int shire_addr = BASE_ADDR_FOR_THIS_TEST | (lfsr_use << 6);
          prefetch_va(false,     0,   shire_addr,  0,         1024,      0, 0 );
          //WAIT_PREFETCH_0;
          if(lfsr_use == 31) { lfsr_use = 0; } else { lfsr_use++; }
       }
       for(int i=0;i<31;i++) {
          //WAIT_FCC(1);
          long unsigned int shire_addr = BASE_ADDR_FOR_THIS_TEST | (lfsr_use << 6);
          if(op == 1) flush_va(false,     dst,   shire_addr,  0,         1024,      0, 0 );
          if(op == 2) evict_va(false,     dst,   shire_addr,  0,         1024,      0, 0 );
          //WAIT_CACHEOPS;
          if(lfsr_use == 31) { lfsr_use = 0; } else { lfsr_use++; }
       }
       return 0; 
    }
    else if(hart_id == 1 || hart_id == 65 || hart_id == 129 || hart_id == 193 || hart_id == 257 || hart_id == 321 || hart_id == 385 || hart_id == 449 || hart_id == 513 || hart_id == 577 || hart_id == 641 || hart_id == 705 || hart_id == 769 || hart_id == 833 || hart_id == 897 || hart_id == 961 || hart_id == 1025 || hart_id == 1089 || hart_id == 1153 || hart_id == 1217 || hart_id == 1281 || hart_id == 1345 || hart_id == 1409 || hart_id == 1473 || hart_id == 1537 || hart_id == 1601 || hart_id == 1665 || hart_id == 1729 || hart_id == 1793 || hart_id == 1857 || hart_id == 1921 || hart_id == 1985) {
       lfsr = generate_random_value(lfsr);
       lfsr_use = lfsr & 0x1F; 
       for(int i=0;i<31;i++) {
          //SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, hart_id); 
          long unsigned int shire_addr = BASE_ADDR_FOR_THIS_TEST | (lfsr_use << 6);
          prefetch_va(false,     0,   shire_addr,  0,         1024,      0, 0 );
          //WAIT_PREFETCH_0;
          if(lfsr_use == 31) { lfsr_use = 0; } else { lfsr_use++; }
       }
       for(int i=0;i<31;i++) {
          //SEND_FCC(THIS_SHIRE, THREAD_0, FCC_1, hart_id); 
          long unsigned int shire_addr = BASE_ADDR_FOR_THIS_TEST | (lfsr_use << 6);
          if(op == 1) flush_va(false,     dst,   shire_addr,  0,         1024,      0, 0 );
          if(op == 2) evict_va(false,     dst,   shire_addr,  0,         1024,      0, 0 );
          //WAIT_CACHEOPS;
          if(lfsr_use == 31) { lfsr_use = 0; } else { lfsr_use++; }
       }
       return 0; 
    }
    else {return 0;}
}

// The following function is flicked from random_read
uint64_t generate_random_value(uint64_t lfsr)
{
    register const uint64_t polynomial = POLYNOMIAL_BIT;

// Minion are slow to branch so unroll this loop
#pragma GCC unroll 35
    for (int i = 0; i < LFSR_SHIFTS_PER_READ; i++)
    {
#ifdef ASM_LFSR
        // Not measurably faster
        uint64_t lsb, polyAndMask;

        asm volatile (
            "andi %1, %0, 1  \n" // lsb = lfsr & 1
            "srli %0, %0, 1  \n" // lfsr >>= 1
            "neg  %1, %1     \n" // convert lsb to mask: 0->0, 1->0xFFFFFFFFFFFFFFFF
            "and  %2, %3, %1 \n" // polyAndMask = polynomial & mask
            "xor  %0, %0, %2 \n" // lfsr ^= (polynomial & mask), noop if mask is 0.
            : "+r" (lfsr), "=&r" (lsb), "=&r" (polyAndMask)
            : "r" (polynomial)
        );
#else
        uint64_t lsb = lfsr & 1U;

        lfsr >>= 1U;

        // Minion are slow to branch so replace if (lsb) branch with algebra so
        // there's no branch but the XOR is a noop if lsb == 0 (X ^ 0 = X)

        // if (lsb)
        // {
        //     lfsr ^= polynomial;
        // }

        // mask = 0 if lsb = 0, 0xFFFFFFFFFFFFFFFF if lsb = 1
        int64_t mask = -(int64_t)lsb;

        // noop if mask is 0
        lfsr ^= (polynomial & (uint64_t)mask);
#endif
    }
    return lfsr;  
}
