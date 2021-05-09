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

#ifndef __TENSORS_H
#define __TENSORS_H

#define QUANT_LAST_TRANS 0
#define QUANT_INT32_TO_FP32 1
#define QUANT_FP32_TO_INT32 2
#define QUANT_RELU 3
#define QUANT_INT32_ADD_ROW 4
#define QUANT_INT32_ADD_COL 5
#define QUANT_FP32_MUL_ROW 6
#define QUANT_FP32_MUL_COL 7
#define QUANT_SATINT8 8
#define QUANT_SATUINT8 9
#define QUANT_PACK_128B 10

#define TENSOR_REDUCE_OP_FADD 0
// #define TENSOR_REDUCE_OP_FSUB 1 -- Not supported
#define TENSOR_REDUCE_OP_FMAX 2
#define TENSOR_REDUCE_OP_FMIN 3
#define TENSOR_REDUCE_OP_IADD 4
// #define TENSOR_REDUCE_OP_ISUB 5 -- Not supported
#define TENSOR_REDUCE_OP_IMAX 6
#define TENSOR_REDUCE_OP_IMIN 7
#define TENSOR_REDUCE_OP_FGET 8

#define TENSOR_LOAD_WAIT_0 0
#define TENSOR_FMA_WAIT 7
#define TENSOR_STORE_WAIT 8
#define TENSOR_REDUCE_WAIT 9
#define TENSOR_QUANT_WAIT 10

#define TENSOR_ERROR_LOAD_TRANSFORM 1
#define TENSOR_ERROR_FCC_OVERFLOW 3
#define TENSOR_ERROR_SCP_DISABLED 4
#define TENSOR_ERROR_LOCKSW 5
#define TENSOR_ERROR_TL1_FMA 6
#define TENSOR_ERROR_MEM_FAULT 7
#define TENSOR_ERROR_STORE_COOP 8
#define TENSOR_ERROR_REDUCE 9

#if defined(__cplusplus) && (__cplusplus >= 201103L)
#include <cinttypes>
#else
#include <inttypes.h>
#endif
#include "device-common/cacheops.h"

typedef enum {
   FADD = 0x0ULL,
   FSUB = 0x1ULL,
   FMAX = 0x2ULL,
   FMIN = 0x3ULL,
   IADD = 0x4ULL,
   ISUB = 0x5ULL,
   IMAX = 0x6ULL,
   IMIN = 0x7ULL,
   FGET = 0x8ULL
} reduce_transform_t;

inline void __attribute__((always_inline)) tensor_load (bool     use_tmask,
                                                        bool     use_coop,
                                                        uint64_t dst_start,
                                                        uint64_t transformation,
                                                        uint64_t use_tenb,
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
                      ((use_tenb & 0x1) << 52)          |
                      ((addr & 0xFFFFFFFFFFC0ULL))      |
                      ((offset & 0x3) << 4)             |
                      ((num_lines & 0xF));
   register uint64_t x31_enc asm("x31") = (stride & 0xFFFFFFFFFFC0ULL) | (id & 0x1);

   __asm__ __volatile__ (
                         "csrw 0x83f, %[csr_enc]\n"
         :
         : [x31_enc] "r" (x31_enc), [csr_enc] "r" (csr_enc)

   );
}

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

inline void __attribute__((always_inline)) et_tensor_load (et_tensor_load_conf_t *conf)
{

   tensor_load (conf->use_tmask, conf->use_coop, conf->dst_start, conf->transformation, (uint64_t) conf->use_tenb, conf->addr, \
                conf->offset, conf->num_lines, conf->stride, conf->id);

}

inline void __attribute__((always_inline)) tensor_load_setup_b (bool     use_coop,
                                                                uint64_t addr,
                                                                uint64_t num_lines,
                                                                uint64_t stride,
                                                                uint64_t id)
{
   uint64_t csr_enc = (((uint64_t)use_coop & 1)  << 62) |
                      (0x1ULL << 52)                    |
                      ((addr & 0xFFFFFFFFFFC0ULL))      |
                      ((num_lines & 0xF));
   register uint64_t x31_enc asm("x31") = (stride & 0xFFFFFFFFFFC0ULL) | (id & 0x1);

   __asm__ __volatile__ (
         "csrw 0x83f, %[csr_enc]\n"
         :
         : [x31_enc] "r" (x31_enc), [csr_enc] "r" (csr_enc)
   );
}

typedef struct et_tensor_load_l2scp_conf
{
   bool     use_tmask;
   uint64_t dst_start;
   uint64_t addr;
   uint64_t num_lines;
   uint64_t stride;
   uint64_t id;
} et_tensor_load_l2scp_conf_t;

inline void __attribute__((always_inline)) et_tensor_load_l2scp (et_tensor_load_l2scp_conf_t *conf)
{

   uint64_t csr_enc = (((((uint64_t) conf->use_tmask) & 1               ) <<     63) |
                       ((            conf->dst_start  & 0x1FFFCUL       ) << (48-2)) |
                       ((            conf->dst_start  & 0x3UL           ) <<      4) |
                       ((            conf->addr       & 0xFFFFFFFFFFC0UL)          ) |
                       ((            conf->num_lines  & 0x0FUL          )          ) );
   register uint64_t x31_enc asm("x31") = (conf->stride & 0xFFFFFFFFFFC0ULL) | (conf->id & 0x1);

   __asm__ __volatile__ (
         "csrw 0x85f, %[csr_enc]\n"
         :
         : [x31_enc] "r" (x31_enc), [csr_enc] "r" (csr_enc)
   );
}

inline void __attribute__((always_inline)) tensor_store_scp(uint64_t entry_stride,
                                                            uint64_t start_scp_entry,
                                                            uint64_t Arows,
                                                            uint64_t addr,
                                                            uint64_t stride)
{
   uint64_t csr_enc = ((entry_stride & 0x3) << 62)      |
                      ((start_scp_entry & 0x3F) << 56)  |
                      ((addr & 0xFFFFFFFFFFC0ULL))      |
                      ((Arows & 0xF) << 51)             |
                      (((uint64_t)1) << 48);
   register uint64_t x31_enc asm("x31") = (stride & 0xFFFFFFFFFFC0UL);

   __asm__ __volatile__ (
         "csrw 0x87f, %[csr_enc]\n"
         :
         : [x31_enc] "r" (x31_enc), [csr_enc] "r" (csr_enc)
   );
}

inline void __attribute__((always_inline)) tensor_store(uint64_t reg_stride,
                                                         uint64_t start_reg,
                                                         uint64_t cols,
                                                         uint64_t Arows,
                                                         uint64_t addr,
                                                         uint64_t coop_store,
                                                         uint64_t stride)
{
   uint64_t warl = 0;
   uint64_t csr_enc = ((reg_stride     & 0x3 ) << 62) |
                      ((start_reg      & 0x1F) << 57) |
                      ((cols           & 0x3 ) << 55) |
                      ((addr & 0xFFFFFFFFFFF0)      ) |
                      ((Arows          & 0xF ) << 51) |
                      ((coop_store     & 0x3 ) << 49) |
                      ((warl           & 0xF )      );

   register uint64_t x31_enc asm("x31") = (stride & 0xFFFFFFFFFF0UL);

   __asm__ __volatile__ (
         "csrw 0x87f, %[csr_enc]\n"
         :
         : [x31_enc] "r" (x31_enc), [csr_enc] "r" (csr_enc)
   );
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

inline uint32_t __attribute__((always_inline)) tensor_reduce_uint32(uint32_t value, uint64_t operation, uint64_t partnerID, uint64_t action) {
   uint64_t warl = 0;
   uint32_t out;
   uint64_t csr_enc = ((warl      & 0x2        ) << 62) |
                      ((0ULL      & 0x1F       ) << 57) |
                      ((warl      & 0x1FFFFFFF ) << 28) |
                      ((operation & 0xF        ) << 24) |
                      ((1ULL      & 0xFF       ) << 16) |
                      ((partnerID & 0x1FFF     ) << 3 ) |
                      ((warl      & 0x1        ) << 2 ) |
                      ((action    & 0x3        )      );

   __asm__ __volatile__ (
         "fmv.s.x     f0, %[value]\n"
         "csrw 0x800, %[csr_enc]\n"
         "fmv.x.s     %[out], f0\n"
         : [out] "=r" (out)
         : [csr_enc] "r" (csr_enc),
           [value] "r" (value)
         : "f0"
   );

   return out;
}

inline float __attribute__((always_inline)) tensor_reduce_float(float freg, uint64_t operation, uint64_t num_reg, uint64_t partnerID, uint64_t action) {
   uint64_t warl = 0;
   float out;
   uint64_t csr_enc = ((warl      & 0x2        ) << 62) |
                      ((0ULL      & 0x1F       ) << 57) |
                      ((warl      & 0x1FFFFFFF ) << 28) |
                      ((operation & 0xF        ) << 24) |
                      ((num_reg   & 0xFF       ) << 16) |
                      ((partnerID & 0x1FFF     ) << 3 ) |
                      ((warl      & 0x1        ) << 2 ) |
                      ((action    & 0x3        )      );

   __asm__ __volatile__ (
         "fmv.s   f0, %[freg]\n"
         "csrw 0x800, %[csr_enc]\n"
         "fmv.s   %[out], f0\n"
         : [out] "=f" (out)
         : [csr_enc] "r" (csr_enc),
           [freg] "f" (freg)
         : "f0"
   );

   return out;
}


//#define tensor_reduce_float1(fval, operation, partnerID, action) do {
//   uint64_t warl = 0;
//   float out;
//   uint64_t csr_enc = ((warl      & 0x2        ) << 62) |
//                      ((0         & 0x1F       ) << 57) |
//                      ((warl      & 0x1FFFFFFF ) << 28) |
//                      ((operation & 0xF        ) << 24) |
//                      ((1         & 0xFF       ) << 16) |
//                      ((partnerID & 0x1FFF     ) << 3 ) |
//                      ((warl      & 0x1        ) << 2 ) |
//                      ((action    & 0x3        )      );
//
//   register float asm("f0") fval;
//   __asm__ volatile (
//         "csrw 0x800, %[csr_enc]"
//         : "+r" (ftmp)
//         : [csr_enc] "r" (csr_enc)
//   );
//} while (0)
//
//
//inline float __attribute__((always_inline)) tensor_reduce_float(uint64_t fstart, uint64_t operation, uint64_t num_reg, uint64_t partnerID, uint64_t action) {
//   uint64_t warl = 0;
//   float out;
//   uint64_t csr_enc = ((warl      & 0x2        ) << 62) |
//                      ((fstart    & 0x1F       ) << 57) |
//                      ((warl      & 0x1FFFFFFF ) << 28) |
//                      ((operation & 0xF        ) << 24) |
//                      ((num_reg   & 0xFF       ) << 16) |
//                      ((partnerID & 0x1FFF     ) << 3 ) |
//                      ((warl      & 0x1        ) << 2 ) |
//                      ((action    & 0x3        )      );
//
//   __asm__ volatile (
//         "csrw 0x800, %[csr_enc]\n"
//         : /*empty*/
//	 : [csr_enc] "r" (csr_enc),
//         : /*"f0", "f1", "f2", "f3", "f4",
//           "f5", "f6", "f7", "f8", "f9",
//           "f10", "f11", "f12", "f13", "f14",
//           "f15", "f16", "f17", "f18", "f19",
//           "f20", "f21", "f22", "f23", "f24",
//           "f25", "f26", "f27", "f28", "f29",
//           "f30", "f31"*/
//   );
//
//   return out;
//}



inline void __attribute__((always_inline)) tensor_reduce(uint64_t start_reg, uint64_t operation, uint64_t num_reg, uint64_t partnerID, uint64_t action) {
   uint64_t warl = 0;

   uint64_t csr_enc = ((warl      & 0x2        ) << 62) |
                      ((start_reg & 0x1F       ) << 57) |
                      ((warl      & 0x1FFFFFFF ) << 28) |
                      ((operation & 0xF        ) << 24) |
                      ((num_reg   & 0xFF       ) << 16) |
                      ((partnerID & 0x1FFF     ) << 3 ) |
                      ((warl      & 0x1        ) << 2 ) |
                      ((action    & 0x3        )      );

   __asm__ __volatile__ (
         "csrw 0x800, %[csr_enc]\n"
         :
         : [csr_enc] "r" (csr_enc)
         :
   );
}

inline void __attribute__((always_inline)) tensor_reduce_send(uint64_t start_reg, uint64_t num_reg, uint64_t partnerID) {
   uint64_t warl = 0;
   tensor_reduce(start_reg, warl, num_reg, partnerID, 0);
}

inline void __attribute__((always_inline)) tensor_reduce_recv(uint64_t start_reg, uint64_t operation, uint64_t num_reg, uint64_t partnerID) {
   tensor_reduce(start_reg, operation, num_reg, partnerID, 1);
}

inline void __attribute__((always_inline)) tensor_reduce_auto(uint64_t start_reg, uint64_t operation, uint64_t num_reg, uint64_t tree_depth) {
   tensor_reduce(start_reg, operation, num_reg, (0ULL << 4) | (tree_depth & 0xF), 3);
}

inline void __attribute__((always_inline)) tensor_broadcast(uint64_t start_reg, uint64_t operation, uint64_t num_reg, uint64_t tree_depth) {
   tensor_reduce(start_reg, operation, num_reg, (0ULL << 4) | (tree_depth & 0xF), 2);
}

inline void __attribute__((always_inline)) tensor_reduce_autopair(uint64_t start_reg, uint64_t operation, uint64_t num_reg, uint64_t start_lvl, uint64_t end_lvl, uint64_t action) {
   uint64_t partnerID;
   // PRM-10 defines the partnerID field for Tensor Reduce (auto-pair variant) as following:
   // [15:11] WARL(0)
   // [10: 7] End level for autopair
   // [ 6: 3] Start level for autopair
   uint64_t warl = 0;
   partnerID = ((warl      & 0xF) << 11) |
               ((end_lvl   & 0xF) << 7 ) |
               ((start_lvl & 0xF) << 3 );
   // Operations encoding:
   // 0000=fadd, 0001=fsub, 0010=fmax, 0011=fmin, 0100=iadd, 0101=isub, 0110=imax, 0111=imin, 1000=fget
   //
   // Action encoding:
   // 00=send, 01=receive, 10=auto-pair broadcast derive from hartid,11=auto-pair reduce derive from hartid
   tensor_reduce(start_reg, operation, num_reg, (partnerID >> 3), action);
}

inline void __attribute__((always_inline)) tensor_quant(uint64_t start_reg, uint64_t col, uint64_t row, uint64_t scp_loc, uint64_t transf9, uint64_t transf8, uint64_t transf7, uint64_t transf6, uint64_t transf5, uint64_t transf4, uint64_t transf3, uint64_t transf2, uint64_t transf1, uint64_t transf0 ) {
   uint64_t csr_enc = ((start_reg  & 0x1F) << 57)       |
                      ((col        & 0x3)  << 55)       |
                      ((row        & 0xF)  << 51)       |
                      ((scp_loc    & 0x3F) << 45)       |
                      ((transf9    & 0xF)  << 36)       |
                      ((transf8    & 0xF)  << 32)       |
                      ((transf7    & 0xF)  << 28)       |
                      ((transf6    & 0xF)  << 24)       |
                      ((transf5    & 0xF)  << 20)       |
                      ((transf4    & 0xF)  << 16)       |
                      ((transf3    & 0xF)  << 12)       |
                      ((transf2    & 0xF)  << 8)        |
                      ((transf1    & 0xF)  << 4)        |
                      ((transf0    & 0xF)  << 0);

   __asm__ __volatile__ (
         "csrw 0x806, %[csr_enc]\n"
         :
         : [csr_enc] "r" (csr_enc)
         :
   );
}

inline void __attribute__((always_inline)) tensor_mask(uint64_t zeros, uint64_t mask_bits)
{
    uint64_t csr_enc = ((zeros & 0x000000000000) << 16) |
                        (mask_bits & 0xFFFF);

   __asm__ __volatile__ (
         "csrw 0x805, %[csr_enc]\n"
         :
         : [csr_enc] "r" (csr_enc)
         :
         );
}

inline void __attribute__((always_inline)) tensor_coop(uint64_t neigh_mask, uint64_t minion_mask, uint64_t coop_id)
{
   uint64_t warl = 0;
   uint64_t csr_enc = ((warl        & 0xFFFFFFFFFFF) << 20) |
                      ((neigh_mask  & 0xFF         ) << 16) |
                      ((minion_mask & 0xFF         ) <<  8) |
                      ((warl        & 0x7          ) <<  6) |
                      ((coop_id     & 0x1F         ) <<  0);

   __asm__ __volatile__ (
         "csrw 0x804, %[csr_enc]\n"
         :
         : [csr_enc] "r" (csr_enc)
         :
         );
}


inline void __attribute__((always_inline)) convolution_ctrl(uint64_t row_start, uint64_t col_start)
{
    uint64_t csr_enc = ((row_start & 0xFFFF) << 32) |
                        (col_start & 0xFFFF);

   __asm__ __volatile__ (
         "csrw 0x803, %[csr_enc]\n"
         :
         : [csr_enc] "r" (csr_enc)
         :
         );
}

inline void __attribute__((always_inline)) convolution_size(uint64_t srow, uint64_t nrow, uint64_t scol, uint64_t ncol)
{
    uint64_t csr_enc = ((srow & 0xFF) << 56) |
        ((nrow & 0xFFFF) << 32 ) |
        ((scol & 0xFF) << 24) |
        ((ncol & 0xFFFF));

    __asm__ __volatile__ (
         "csrw 0x802, %[csr_enc]\n"
         :
         : [csr_enc] "r" (csr_enc)
         :
         );
}

inline unsigned long __attribute__((always_inline)) get_tensor_error()
{
   unsigned long error;

   __asm__ __volatile__ (
         "csrr %0, 0x808"
         : "=r" (error)
         );

   return error;
}

inline uint64_t __attribute__((always_inline)) get_tensor_mask()
{
   uint64_t val;

   __asm__ __volatile__ (
         "csrr %0, 0x805"
         : "=r" (val)
         );

   return val;
}

#define mask_set(msk, val) do { \
    __asm__ volatile("mov.m.x m" #msk ", zero, %0" :: "n"(val)); \
} while (0)

#define flw_ps(fd, ptr) do { \
    __asm__ volatile("flw.ps f" #fd ", (%0)" :: "r"(ptr)); \
} while (0)

#define fsw_ps(fd, ptr) do { \
    __asm__ volatile("fsw.ps f" #fd ", (%0)" :: "r"(ptr) : "memory"); \
} while (0)

#endif // ! __TENSORS_H
