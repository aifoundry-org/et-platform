/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#ifndef __CACHEOPS_H
#define __CACHEOPS_H

#if defined(__cplusplus) && (__cplusplus >= 201103L)
#include <cinttypes>
#else
#include <inttypes.h>
#include <stdbool.h>
#endif

#include "macros.h" // FENCE

inline void __attribute__((always_inline)) lock_sw(uint64_t use_tmask,uint64_t way, uint64_t paddr, __attribute__((unused)) uint64_t num_lines) {
   uint64_t csr_enc = ((use_tmask & 1) << 63) |
                      ((way & 0x3) << 55)     |
                      ((paddr & 0x0FFFFFFFFFFULL));

   __asm__ __volatile__ ( "csrw 0x7fd, %[csr_enc]\n" : : [csr_enc] "r" (csr_enc));
}

inline void __attribute__((always_inline)) unlock_sw(uint64_t use_tmask,uint64_t way, uint64_t paddr, __attribute__((unused)) uint64_t num_lines) {
   uint64_t csr_enc = ((use_tmask & 1) << 63)         |
                      ((way & 0x3) << 55)             |
                      ((paddr & 0x0FFFFFFFFFFULL));

   __asm__ __volatile__ ( "csrw 0x7ff, %[csr_enc]\n" : : [csr_enc] "r" (csr_enc));
}

inline void __attribute__((always_inline)) evict_sw(uint64_t use_tmask, uint64_t dst, uint64_t way, uint64_t set, uint64_t num_lines) {
   uint64_t csr_enc = ((use_tmask & 1) << 63) |
                      ((dst & 0x3) << 58)     |
                      ((set & 0xF) << 14)     |
                      ((way & 0x3) << 6)      |
                      ((num_lines & 0xF));

   __asm__ __volatile__ ( "csrw 0x7f9, %[csr_enc]\n" : : [csr_enc] "r" (csr_enc));

}

inline void __attribute__((always_inline)) flush_sw(uint64_t use_tmask, uint64_t dst, uint64_t way, uint64_t set, uint64_t num_lines) {
   uint64_t csr_enc = ((use_tmask & 1) << 63) |
                      ((dst & 0x3) << 58)     |
                      ((set & 0xF) << 14)     |
                      ((way & 0x3) << 6)      |
                      ((num_lines & 0xF));

   __asm__ __volatile__ ( "csrw 0x7fb, %[csr_enc]\n" : : [csr_enc] "r" (csr_enc));
}


inline void __attribute__((always_inline)) cop_va(uint64_t opcode, uint64_t use_tmask, uint64_t dst, uint64_t start, uint64_t way, uint64_t addr, uint64_t num_lines, uint64_t stride, uint64_t id) {
   uint64_t csr_enc = ((use_tmask & 1) << 63)      |
                      ((opcode & 0x7) << 60)       |
                      ((dst & 0x3) << 58)          |
                      ((start & 0x3) << 56)        |
                      ((way & 0xFF) << 48)         |
                      ((addr & 0xFFFFFFFFFFC0ULL)) |
                      ((num_lines & 0xF));

   uint64_t x31_enc = (stride & 0xFFFFFFFFFFC0ULL) | (id & 0x1);

   __asm__ __volatile__ (
      "add x31, zero, %[x31_enc]\n"
      "csrw 0x81f, %[csr_enc]\n"
      :
      : [x31_enc] "r" (x31_enc), [csr_enc] "r" (csr_enc)
      : "x31"
   );
}

inline void __attribute__((always_inline)) cop_va_usermode(uint64_t opcode, uint64_t use_tmask, uint64_t dst, uint64_t start, uint64_t way, uint64_t addr, uint64_t num_lines, uint64_t stride, uint64_t id) {
   uint64_t csr_enc = ((use_tmask & 1) << 63)      |
                      ((opcode & 0x7) << 60)       |
                      ((dst & 0x3) << 58)          |
                      ((start & 0x3) << 56)        |
                      ((way & 0xFF) << 48)         |
                      ((addr & 0xFFFFFFFFFFC0ULL)) |
                      ((num_lines & 0xF));

   uint64_t x31_enc = (stride & 0xFFFFFFFFFFC0ULL) | (id & 0x1);

   __asm__ __volatile__ (
      "add x31, zero, %[x31_enc]\n"
      "csrw 0x81f, %[csr_enc]\n"
      :
      : [x31_enc] "r" (x31_enc), [csr_enc] "r" (csr_enc)
      : "x31"
   );
}

inline void __attribute__((always_inline)) evict_va(uint64_t use_tmask, uint64_t dst, __attribute__((unused)) uint64_t start, uint64_t vaddr, uint64_t num_lines, uint64_t stride, uint64_t id) {

   uint64_t csr_enc = ((use_tmask & 1) << 63)      |
                      ((dst & 0x3) << 58)          |
                      ((vaddr & 0xFFFFFFFFFFC0ULL))|
                      ((num_lines & 0xF));

   uint64_t x31_enc = (stride & 0xFFFFFFFFFFC0ULL) | (id & 0x1);

   __asm__ __volatile__ (
      "add x31, zero, %[x31_enc]\n"
      "csrw 0x89f, %[csr_enc]\n"
      :
      : [x31_enc] "r" (x31_enc), [csr_enc] "r" (csr_enc)
      : "x31"
   );

}

inline void __attribute__((always_inline)) flush_va(uint64_t use_tmask, uint64_t dst, __attribute__((unused)) uint64_t start, uint64_t addr, uint64_t num_lines, uint64_t stride, uint64_t id) {

   uint64_t csr_enc = ((use_tmask & 1) << 63)      |
                      ((dst & 0x3) << 58)          |
                      ((addr & 0xFFFFFFFFFFC0ULL)) |
                      ((num_lines & 0xF));

   uint64_t x31_enc = (stride & 0xFFFFFFFFFFC0ULL) | (id & 0x1);

   __asm__ __volatile__ (
      "add x31, zero, %[x31_enc]\n"
      "csrw 0x8bf, %[csr_enc]\n"
      :
      : [x31_enc] "r" (x31_enc), [csr_enc] "r" (csr_enc)
      : "x31"
   );


}

inline void __attribute__((always_inline)) prefetch_va(bool use_tmask, uint64_t dst, __attribute__((unused)) uint64_t start, uint64_t addr, uint64_t num_lines, uint64_t stride, uint64_t id) {
   cop_va(0x4, use_tmask, dst, 0, 0, addr, num_lines, stride, id);
}

inline void __attribute__((always_inline)) lock_va(uint64_t use_tmask, __attribute__((unused)) uint64_t way, uint64_t addr, uint64_t num_lines, uint64_t stride, uint64_t id) {
   uint64_t csr_enc = ((use_tmask & 1) << 63)      |

                      ((addr & 0xFFFFFFFFFFC0ULL)) |
                      ((num_lines & 0xF));

   uint64_t x31_enc = (stride & 0xFFFFFFFFFFC0ULL) | (id & 0x1);

   __asm__ __volatile__ (
      "add x31, zero, %[x31_enc]\n"
      "csrw 0x8df, %[csr_enc]\n"
      :
      : [x31_enc] "r" (x31_enc), [csr_enc] "r" (csr_enc)
      : "x31"
   );

}

inline void __attribute__((always_inline)) unlock_va(uint64_t use_tmask, __attribute__((unused)) bool keep_valid, uint64_t addr, uint64_t num_lines, uint64_t stride, uint64_t id) {
   uint64_t csr_enc = ((use_tmask & 1) << 63)      |
                      ((addr & 0xFFFFFFFFFFC0ULL)) |
                      ((num_lines & 0xF));

   uint64_t x31_enc = (stride & 0xFFFFFFFFFFC0ULL) | (id & 0x1);

   __asm__ __volatile__ (
      "add x31, zero, %[x31_enc]\n"
      "csrw 0x8ff, %[csr_enc]\n"
      :
      : [x31_enc] "r" (x31_enc), [csr_enc] "r" (csr_enc)
      : "x31"
   );
}


inline void __attribute__((always_inline)) new_evict_va(uint64_t use_tmask, uint64_t dst, uint64_t addr, uint64_t num_lines, uint64_t WaRl) {
   uint64_t csr_enc = ((use_tmask & 1) << 63) 		|
		      ((WaRl & 0x7) << 60)    		|		
                      ((dst & 0x3) << 58)     		| //00=L1, 01=L2, 10=L3, 11=MEM
		      ((WaRl & 0x3ff) << 48)  	  	|		
                      ((addr & 0x3FFFFFFFFFF) << 6)	|
		      ((WaRl & 0x3) << 4)  	  	|		
                      ((num_lines & 0xF));

   __asm__ __volatile__ ( "csrw 0x89f, %[csr_enc]\n" : : [csr_enc] "r" (csr_enc));
}

inline void __attribute__((always_inline)) mcache_control(uint64_t WaRl, uint64_t d1_split, uint64_t scp_en) {
   uint64_t csr_enc = ((WaRl & 0x7FFFFFFFFFFFFFFF) << 62)      |
                      ((scp_en& 0x1) << 1)                          |
                      ((d1_split& 0x1) << 0);

   __asm__ __volatile__ (
      "csrw 0x7e0, %[csr_enc]\n"
      :
      : [csr_enc] "r" (csr_enc)
      : "x31"
   );
}
inline void __attribute__((always_inline)) excl_mode(uint64_t val) {
   __asm__ __volatile__ (
      "csrw 0x7d3, %[csr_enc]\n"
      :
      : [csr_enc] "r" (val)
      : "x31"
   );
}

inline void __attribute__((always_inline)) scp(uint64_t WaRl, uint64_t DEscratchpad) {

    // Hard partition L1 Data cache between the harts  
    //mcache_control(0,0,0);
    FENCE;
    WAIT_CACHEOPS;

    // Enable scratchpad
    uint64_t csr_enc = ((WaRl & 0x7FFFFFFFFFFFFFFF) << 1)      |
                      ((DEscratchpad & 0x1));

   __asm__ __volatile__ (
      "csrw 0x810, %[csr_enc]\n"
      :
      : [csr_enc] "r" (csr_enc)
      : "x31"
   );
}

#endif // ! __CACHEOPS_H
