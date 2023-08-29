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

#ifndef _UTILS_H_
#define _UTILS_H_


/* Kernel status codes */
#define SUCCESS 0
#define ERROR_SELF_CHECK_MASK_MISMATCH -1

typedef struct et_tensor_load_conf
{
   bool     use_tmask;
   bool     use_coop;
   bool     use_tenb;
   uint64_t dst_start;
   uint64_t transformation;
   uint64_t rd_l2scp;
   uint64_t addr;
   uint64_t offset;
   uint64_t num_lines;
   uint64_t stride;
   uint64_t id;
} et_tensor_load_conf_t;

inline void __attribute__((always_inline)) et_tensor_load (et_tensor_load_conf_t *conf);
inline void __attribute__((always_inline)) excl_mode(uint64_t val);
inline void __attribute__((always_inline)) evict_sw(uint64_t use_tmask, uint64_t dst, uint64_t way, uint64_t set, uint64_t num_lines, uint64_t warl);
inline void __attribute__((always_inline)) mcache_control(uint64_t d1_split, uint64_t scp_en, uint64_t warl);
inline void __attribute__((always_inline)) clear_l1d(void);
void setup_cache_scp(void);
void setup_cache_shared(void);


#define WAIT_TENSOR_LOAD_0     __asm__ __volatile__ ( "csrwi tensor_wait, 0\n" : : );
#define WAIT_TENSOR_LOAD_1     __asm__ __volatile__ ( "csrwi tensor_wait, 1\n" : : );
#define WAIT_TENSOR_LOAD_L2_0  __asm__ __volatile__ ( "csrwi tensor_wait, 2\n" : : );
#define WAIT_TENSOR_LOAD_L2_1  __asm__ __volatile__ ( "csrwi tensor_wait, 3\n" : : );
#define WAIT_PREFETCH_0        __asm__ __volatile__ ( "csrwi tensor_wait, 4\n" : : );
#define WAIT_PREFETCH_1        __asm__ __volatile__ ( "csrwi tensor_wait, 5\n" : : );
#define WAIT_CACHEOPS          __asm__ __volatile__ ( "csrwi tensor_wait, 6\n" : : );
#define WAIT_TENSOR_FMA        __asm__ __volatile__ ( "csrwi tensor_wait, 7\n" : : );
#define WAIT_TENSOR_STORE      __asm__ __volatile__ ( "csrwi tensor_wait, 8\n" : : );
#define WAIT_TENSOR_REDUCE     __asm__ __volatile__ ( "csrwi tensor_wait, 9\n" : : );
#define WAIT_TENSOR_QUANT      __asm__ __volatile__ ( "csrwi tensor_wait, 10\n" : : );

inline void __attribute__((always_inline)) tensor_load (bool     use_tmask,
                                                        bool     use_coop,
                                                        uint64_t dst_start,
                                                        uint64_t transformation,
                                                        bool     use_tenb,
                                                        uint64_t addr,
                                                        uint64_t offset,
                                                        uint64_t num_lines,
                                                        uint64_t stride,
                                                        uint64_t id)
{
   uint64_t csr_enc = (((uint64_t)use_tmask & 1) << 63) |
                      (((uint64_t)use_coop & 1)  << 62) |
                      ((transformation & 0x7) << 59)    |
                      ((dst_start & 0x3F) << 53)        |
                      (((uint64_t)use_tenb & 0x1) << 52) |
                      ((addr & 0xFFFFFFFFFFC0ULL))      |
                      ((offset & 0x3) << 4)             |
                      ((num_lines & 0xF));
   uint64_t x31_enc = (stride & 0xFFFFFFFFFFC0ULL) | (id & 0x1);

   __asm__ __volatile__ (
         "add x31, zero, %[x31_enc]\n"
         "csrw 0x83f, %[csr_enc]\n"
         :
         : [x31_enc] "r" (x31_enc), [csr_enc] "r" (csr_enc)
         : "x31"
   );
}

inline void __attribute__((always_inline)) et_tensor_load (et_tensor_load_conf_t *conf)
{

   tensor_load (conf->use_tmask, conf->use_coop, conf->dst_start, conf->transformation, (uint64_t) conf->use_tenb, conf->addr, \
                conf->offset, conf->num_lines, conf->stride, conf->id);

}

inline void __attribute__((always_inline)) excl_mode(uint64_t val)
{
   __asm__ __volatile__ (
      "csrw 0x7d3, %[csr_enc]\n"
      :
      : [csr_enc] "r" (val)
      : "x31"
   );
}

inline void __attribute__((always_inline)) evict_sw(uint64_t use_tmask, uint64_t dst, uint64_t way, uint64_t set, uint64_t num_lines, uint64_t warl)
{
   uint64_t csr_enc = ((warl      & 0x73FFFFFFFFFC3F30ULL )       ) |
                      ((use_tmask & 1                     ) << 63 ) |
                      ((dst       & 0x3                   ) << 58 ) |
                      ((set       & 0xF                   ) << 14 ) |
                      ((way       & 0x3                   ) << 6  ) |
                      ((num_lines & 0xF                   )       ) ;

   __asm__ __volatile__ ( "csrw 0x7f9, %[csr_enc]\n" : : [csr_enc] "r" (csr_enc));

}

inline void __attribute__((always_inline)) mcache_control(uint64_t d1_split, uint64_t scp_en, uint64_t warl)
{
   uint64_t csr_enc = ((warl     & 0xFFFFFFFFFFFFFFFCULL )      ) |
                      ((scp_en   & 0x1                   ) << 1 ) |
                      ((d1_split & 0x1                   ) << 0 ) ;

   __asm__ __volatile__ (
      "csrw 0x7e0, %[csr_enc]\n"
      :
      : [csr_enc] "r" (csr_enc)
      : "x31"
   );
}

#define FENCE __asm__ __volatile__ ("fence\n");

#define L1_WAYS     4
#define L1_SETS     16
#define L1_CL_SIZE  64

enum cop_dest {
   to_L1  = 0x0ULL,
   to_L2  = 0x1ULL,
   to_L3  = 0x2ULL,
   to_Mem = 0x3ULL
};

inline void __attribute__((always_inline)) clear_l1d() {

   for (uint64_t w = 0; w < L1_WAYS; ++w) {
      evict_sw(false, to_L2, w, 0, 15, 0);
   }
   WAIT_CACHEOPS;
}

void setup_cache_scp(){
      // PRM-8: Cache Control Extension
      excl_mode(1);
      // Evict the whole L1$
      //       use_tmask, dst, way, set, num_lines, warl
      evict_sw(        0,   1,   0,   0,       0xf,    0);
      evict_sw(        0,   1,   1,   0,       0xf,    0);
      evict_sw(        0,   1,   2,   0,       0xf,    0);
      evict_sw(        0,   1,   3,   0,       0xf,    0);

      // Shared Mode
      mcache_control(0,0,0);
      FENCE;
      WAIT_CACHEOPS;

      // Clear the L1 to avoid following locks to fail
      clear_l1d();
      WAIT_CACHEOPS;

      // D1Split Mode
      mcache_control(1,0,0);
      WAIT_CACHEOPS;
      //NOP;   // VERIF-3295: Xavier suggested an extra NOP before FENCE; (here the above "WAIT_CACHEOPS" takes at least 1 cycle)
      FENCE;   // PRM-8; VERIF-3295

      // Scratchpad Mode
      mcache_control(1,1,0);
      WAIT_CACHEOPS;

      excl_mode(0);
}

void setup_cache_shared(){
      // PRM-8: Cache Control Extension
      excl_mode(1);
      // Evict the whole L1$
      //       use_tmask, dst, way, set, num_lines, warl
      evict_sw(        0,   1,   0,   0,       0xf,    0);
      evict_sw(        0,   1,   1,   0,       0xf,    0);
      evict_sw(        0,   1,   2,   0,       0xf,    0);
      evict_sw(        0,   1,   3,   0,       0xf,    0);

      // Shared Mode
      mcache_control(0,0,0);
      FENCE;
      WAIT_CACHEOPS;

      // Clear the L1 to avoid following locks to fail
      clear_l1d();
      WAIT_CACHEOPS;

      excl_mode(0);
}

inline void __attribute__((always_inline)) tensor_fma(bool use_tmask, uint64_t b_num_col, uint64_t a_num_rows, uint64_t a_num_cols, uint64_t offset, bool tenc_loc, bool tenb_unsigned, bool tena_unsigned, bool tenb_loc, uint64_t scp_loc_b, uint64_t scp_loc_a, uint64_t opcode, bool first_pass) {
   uint64_t csr_enc = (((uint64_t)use_tmask & 1) << 63)       |
                      ((b_num_col & 0x3) << 55)               |
                      ((a_num_rows & 0xF) << 51)              |
                      ((a_num_cols & 0xF) << 47)              |
                      ((offset & 0xF) << 43)                  |
                      (((uint64_t) tenc_loc & 1) << 23)       |
                      (((uint64_t) tena_unsigned & 1) << 22)  |
                      (((uint64_t) tenb_unsigned & 1) << 21)  |
                      (((uint64_t) tenb_loc & 1) << 20)       |
                      ((scp_loc_b & 0xFF) << 12)              |
                      ((scp_loc_a & 0xFF) << 4)               |
                      ((opcode & 0x7) << 1)                   |
                      ((uint64_t)first_pass & 1);

   __asm__ __volatile__ (
         "csrw 0x801, %[csr_enc]\n"
         :
         : [csr_enc] "r" (csr_enc)
         :
   );
}

#endif
