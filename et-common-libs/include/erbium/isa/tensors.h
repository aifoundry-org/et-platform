/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/
/***********************************************************************/
/*! \file tensors.h
    \brief Tensor instruction wrappers for erbium.

*/
/***********************************************************************/

#ifndef _ERBIUM_ISA_TENSORS_H_
#define _ERBIUM_ISA_TENSORS_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__cplusplus) && (__cplusplus >= 201103L)
#include <cinttypes>
#if (__cplusplus < 202002L)
#include <cstdbool>
#endif
#else
#include <inttypes.h>
#include <stdbool.h>
#endif

/* Range-check macro. Default to the standard assert(); callers who
 * can't link libc or want zero cost can override with a no-op:
 *     #define ERBIUM_TENSOR_ASSERT(cond) ((void)(cond))
 *     #include <erbium/isa/tensors.h>
 * or compile with -DNDEBUG. */
#ifndef ERBIUM_TENSOR_ASSERT
#include <assert.h>
#define ERBIUM_TENSOR_ASSERT(cond) assert(cond)
#endif

/*! \def QUANT_LAST_TRANS
    \brief Tensor Quant: do not perform any more transformations. */
#define QUANT_LAST_TRANS    0
#define QUANT_INT32_TO_FP32 1
#define QUANT_FP32_TO_INT32 2
#define QUANT_RELU          3
#define QUANT_INT32_ADD_ROW 4
#define QUANT_INT32_ADD_COL 5
#define QUANT_FP32_MUL_ROW  6
#define QUANT_FP32_MUL_COL  7
#define QUANT_SATINT8       8
#define QUANT_SATUINT8      9
#define QUANT_PACK_128B     10

/*! \def TENSOR_REDUCE_OP_*
    \brief Reduction operations encoded in the reduce CSR. */
#define TENSOR_REDUCE_OP_FADD 0
/* #define TENSOR_REDUCE_OP_FSUB 1 — not supported */
#define TENSOR_REDUCE_OP_FMAX 2
#define TENSOR_REDUCE_OP_FMIN 3
#define TENSOR_REDUCE_OP_IADD 4
/* #define TENSOR_REDUCE_OP_ISUB 5 — not supported */
#define TENSOR_REDUCE_OP_IMAX 6
#define TENSOR_REDUCE_OP_IMIN 7
#define TENSOR_REDUCE_OP_FGET 8

/*! \def TENSOR_*_WAIT
    \brief Argument values for tensor_wait() — pick which class of
    prior tensor instructions the wait should drain. */
#define TENSOR_LOAD_WAIT_0 0
#define TENSOR_LOAD_WAIT_1 1
#define TENSOR_FMA_WAIT    7
#define TENSOR_STORE_WAIT  8
#define TENSOR_REDUCE_WAIT 9
#define TENSOR_QUANT_WAIT  10

/*! \def TENSOR_ERROR_*
    \brief Error bits reported via the tensor_error CSR (0x808). */
#define TENSOR_ERROR_LOAD_TRANSFORM 1
#define TENSOR_ERROR_FCC_OVERFLOW   3
#define TENSOR_ERROR_SCP_DISABLED   4
#define TENSOR_ERROR_LOCKSW         5
#define TENSOR_ERROR_TL1_FMA        6
#define TENSOR_ERROR_MEM_FAULT      7
#define TENSOR_ERROR_STORE_COOP     8
#define TENSOR_ERROR_REDUCE         9

/*! \enum reduce_transform_t */
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

/*! \struct et_tensor_load_conf */
typedef struct et_tensor_load_conf {
    bool use_tmask;
    bool use_coop;
    bool use_tenb;
    uint64_t dst_start;
    uint64_t transformation;
    uint64_t rd_l2scp;
    uint64_t addr;
    uint64_t offset;
    uint64_t num_lines;
    uint64_t stride;
    uint64_t id;
} et_tensor_load_conf_t;

/*! \fn tensor_wait(long id) */
static inline __attribute__((always_inline))
void tensor_wait(long id)
{
    __asm__ __volatile__(" csrw 0x830, %[id]\n" : : [id] "r"(id) : "memory");
}

/*! \fn tensor_load(...) */
static inline __attribute__((always_inline))
void tensor_load(bool use_tmask, bool use_coop, uint64_t dst_start, uint64_t transformation,
                 uint64_t use_tenb, uint64_t addr, uint64_t offset, uint64_t num_lines,
                 uint64_t stride, uint64_t id)
{
    uint64_t csr_enc = (((uint64_t)use_tmask & 1) << 63) | (((uint64_t)use_coop & 1) << 62) |
                       ((transformation & 0x7) << 59) | ((dst_start & 0x3F) << 53) |
                       ((use_tenb & 0x1) << 52) | ((addr & 0xFFFFFFFFFFC0ULL)) |
                       ((offset & 0x3) << 4) | ((num_lines & 0xF));
    register uint64_t x31_enc asm("x31") = (stride & 0xFFFFFFFFFFC0ULL) | (id & 0x1);

    __asm__ __volatile__("csrw 0x83f, %[csr_enc]\n"
                         :
                         : [x31_enc] "r"(x31_enc), [csr_enc] "r"(csr_enc));
}

/*! \fn et_tensor_load(conf) */
static inline __attribute__((always_inline))
void et_tensor_load(et_tensor_load_conf_t *conf)
{
    tensor_load(conf->use_tmask, conf->use_coop, conf->dst_start, conf->transformation,
                (uint64_t)conf->use_tenb, conf->addr, conf->offset, conf->num_lines, conf->stride,
                conf->id);
}

/*! \fn tensor_load_setup_b(...) */
static inline __attribute__((always_inline))
void tensor_load_setup_b(bool use_coop, uint64_t addr, uint64_t num_lines, uint64_t stride,
                         uint64_t id)
{
    uint64_t csr_enc = (((uint64_t)use_coop & 1) << 62) | (0x1ULL << 52) |
                       ((addr & 0xFFFFFFFFFFC0ULL)) | ((num_lines & 0xF));
    register uint64_t x31_enc asm("x31") = (stride & 0xFFFFFFFFFFC0ULL) | (id & 0x1);

    __asm__ __volatile__("csrw 0x83f, %[csr_enc]\n"
                         :
                         : [x31_enc] "r"(x31_enc), [csr_enc] "r"(csr_enc));
}

/* NOTE: et_tensor_load_l2scp() and et_tensor_load_l2scp_conf_t are
 * intentionally absent — erbium has no load-to-L2SCP path. */

/*! \fn tensor_store_scp(entry_stride, start_scp_entry, Arows, addr, stride) */
static inline __attribute__((always_inline))
void tensor_store_scp(uint64_t entry_stride, uint64_t start_scp_entry, uint64_t Arows,
                      uint64_t addr, uint64_t stride)
{
    uint64_t csr_enc = ((entry_stride & 0x3) << 62) | ((start_scp_entry & 0x3F) << 56) |
                       ((addr & 0xFFFFFFFFFFC0ULL)) | ((Arows & 0xF) << 51) |
                       (((uint64_t)1) << 48);
    register uint64_t x31_enc asm("x31") = (stride & 0xFFFFFFFFFFC0UL);

    __asm__ __volatile__("csrw 0x87f, %[csr_enc]\n"
                         :
                         : [x31_enc] "r"(x31_enc), [csr_enc] "r"(csr_enc));
}

/*! \fn tensor_store(...) */
static inline __attribute__((always_inline))
void tensor_store(uint64_t reg_stride, uint64_t start_reg, uint64_t cols, uint64_t Arows,
                  uint64_t addr, uint64_t coop_store, uint64_t stride)
{
    uint64_t warl = 0;
    uint64_t csr_enc = ((reg_stride & 0x3) << 62) | ((start_reg & 0x1F) << 57) |
                       ((cols & 0x3) << 55) | ((addr & 0xFFFFFFFFFFF0)) | ((Arows & 0xF) << 51) |
                       ((coop_store & 0x3) << 49) | ((warl & 0xF));

    register uint64_t x31_enc asm("x31") = (stride & 0xFFFFFFFFFF0UL);

    __asm__ __volatile__("csrw 0x87f, %[csr_enc]\n"
                         :
                         : [x31_enc] "r"(x31_enc), [csr_enc] "r"(csr_enc));
}

/*! \fn tensor_fma(...) */
static inline __attribute__((always_inline))
void tensor_fma(bool use_tmask, uint64_t b_num_col, uint64_t a_num_rows, uint64_t a_num_cols,
                uint64_t offset, bool tenc_loc, bool tenb_unsigned, bool tena_unsigned,
                bool tenb_loc, uint64_t scp_loc_b, uint64_t scp_loc_a, uint64_t opcode,
                bool first_pass)
{
    uint64_t csr_enc =
        (((uint64_t)use_tmask & 1) << 63) | ((b_num_col & 0x3) << 55) | ((a_num_rows & 0xF) << 51) |
        ((a_num_cols & 0xF) << 47) | ((offset & 0xF) << 43) | (((uint64_t)tenc_loc & 1) << 23) |
        (((uint64_t)tena_unsigned & 1) << 22) | (((uint64_t)tenb_unsigned & 1) << 21) |
        (((uint64_t)tenb_loc & 1) << 20) | ((scp_loc_b & 0xFF) << 12) | ((scp_loc_a & 0xFF) << 4) |
        ((opcode & 0x7) << 1) | ((uint64_t)first_pass & 1);

    __asm__ __volatile__("csrw 0x801, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc) :);
}

/*! \fn tensor_reduce_uint32(value, operation, partnerID, action)
    \brief Raw reduce with 32-bit value. partnerID interpretation
    depends on action — no assert here. */
static inline __attribute__((always_inline))
uint32_t tensor_reduce_uint32(uint32_t value, uint64_t operation, uint64_t partnerID,
                              uint64_t action)
{
    uint64_t warl = 0;
    uint32_t out;
    uint64_t csr_enc = ((warl & 0x2) << 62) | ((0ULL & 0x1F) << 57) | ((warl & 0x1FFFFFFF) << 28) |
                       ((operation & 0xF) << 24) | ((1ULL & 0xFF) << 16) |
                       ((partnerID & 0x1FFF) << 3) | ((warl & 0x1) << 2) | ((action & 0x3));

    __asm__ __volatile__("fmv.s.x     f0, %[value]\n"
                         "csrw 0x800, %[csr_enc]\n"
                         "fmv.x.s     %[out], f0\n"
                         : [out] "=r"(out)
                         : [csr_enc] "r"(csr_enc), [value] "r"(value)
                         : "f0");

    return out;
}

/*! \fn tensor_reduce_float(freg, operation, num_reg, partnerID, action) */
static inline __attribute__((always_inline))
float tensor_reduce_float(float freg, uint64_t operation, uint64_t num_reg, uint64_t partnerID,
                          uint64_t action)
{
    uint64_t warl = 0;
    float out;
    uint64_t csr_enc = ((warl & 0x2) << 62) | ((0ULL & 0x1F) << 57) | ((warl & 0x1FFFFFFF) << 28) |
                       ((operation & 0xF) << 24) | ((num_reg & 0xFF) << 16) |
                       ((partnerID & 0x1FFF) << 3) | ((warl & 0x1) << 2) | ((action & 0x3));

    __asm__ __volatile__("fmv.s   f0, %[freg]\n"
                         "csrw 0x800, %[csr_enc]\n"
                         "fmv.s   %[out], f0\n"
                         : [out] "=f"(out)
                         : [csr_enc] "r"(csr_enc), [freg] "f"(freg)
                         : "f0");

    return out;
}

/*! \fn tensor_reduce(start_reg, operation, num_reg, partnerID, action)
    \brief Raw reduce. No assert — action dictates whether partnerID
    is a minion ID, a tree-depth blob, or an autopair level pair. */
static inline __attribute__((always_inline))
void tensor_reduce(uint64_t start_reg, uint64_t operation, uint64_t num_reg, uint64_t partnerID,
                   uint64_t action)
{
    uint64_t warl = 0;

    uint64_t csr_enc = ((warl & 0x2) << 62) | ((start_reg & 0x1F) << 57) |
                       ((warl & 0x1FFFFFFF) << 28) | ((operation & 0xF) << 24) |
                       ((num_reg & 0xFF) << 16) | ((partnerID & 0x1FFF) << 3) |
                       ((warl & 0x1) << 2) | ((action & 0x3));

    __asm__ __volatile__("csrw 0x800, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc) :);
}

/*! \fn tensor_reduce_send(start_reg, num_reg, partnerID)
    \brief Send to target minion. partnerID must be a valid erbium
    minion index. */
static inline __attribute__((always_inline))
void tensor_reduce_send(uint64_t start_reg, uint64_t num_reg, uint64_t partnerID)
{
    ERBIUM_TENSOR_ASSERT(partnerID < 8);  /* erbium has 8 minions */
    uint64_t warl = 0;
    tensor_reduce(start_reg, warl, num_reg, partnerID, 0);
}

/*! \fn tensor_reduce_recv(start_reg, operation, num_reg, partnerID)
    \brief Receive from target minion. */
static inline __attribute__((always_inline))
void tensor_reduce_recv(uint64_t start_reg, uint64_t operation, uint64_t num_reg,
                        uint64_t partnerID)
{
    ERBIUM_TENSOR_ASSERT(partnerID < 8);  /* erbium has 8 minions */
    tensor_reduce(start_reg, operation, num_reg, partnerID, 1);
}

/*! \fn tensor_reduce_auto(start_reg, operation, num_reg, tree_depth)
    \brief Auto-tree reduce across all participating harts. With 8
    minions the tree is at most 3 levels deep. */
static inline __attribute__((always_inline))
void tensor_reduce_auto(uint64_t start_reg, uint64_t operation, uint64_t num_reg,
                        uint64_t tree_depth)
{
    ERBIUM_TENSOR_ASSERT(tree_depth < 4);  /* log2(8 minions) = 3 */
    tensor_reduce(start_reg, operation, num_reg, (0ULL << 4) | (tree_depth & 0xF), 3);
}

/*! \fn tensor_broadcast(start_reg, operation, num_reg, tree_depth) */
static inline __attribute__((always_inline))
void tensor_broadcast(uint64_t start_reg, uint64_t operation, uint64_t num_reg,
                      uint64_t tree_depth)
{
    ERBIUM_TENSOR_ASSERT(tree_depth < 4);  /* log2(8 minions) = 3 */
    tensor_reduce(start_reg, operation, num_reg, (0ULL << 4) | (tree_depth & 0xF), 2);
}

/*! \fn tensor_reduce_autopair(start_reg, operation, num_reg, start_lvl, end_lvl, action)
    \brief Auto-pair variant. Level-range encoding of partnerID —
    no assert here since the start/end tree levels have their own
    semantics and the autopair behavior is user-controlled. */
static inline __attribute__((always_inline))
void tensor_reduce_autopair(uint64_t start_reg, uint64_t operation, uint64_t num_reg,
                            uint64_t start_lvl, uint64_t end_lvl, uint64_t action)
{
    uint64_t partnerID;
    uint64_t warl = 0;
    partnerID = ((warl & 0xF) << 11) | ((end_lvl & 0xF) << 7) | ((start_lvl & 0xF) << 3);
    tensor_reduce(start_reg, operation, num_reg, (partnerID >> 3), action);
}

/*! \fn tensor_quant(...) */
static inline __attribute__((always_inline))
void tensor_quant(uint64_t start_reg, uint64_t col, uint64_t row, uint64_t scp_loc,
                  uint64_t transf9, uint64_t transf8, uint64_t transf7, uint64_t transf6,
                  uint64_t transf5, uint64_t transf4, uint64_t transf3, uint64_t transf2,
                  uint64_t transf1, uint64_t transf0)
{
    uint64_t csr_enc = ((start_reg & 0x1F) << 57) | ((col & 0x3) << 55) | ((row & 0xF) << 51) |
                       ((scp_loc & 0x3F) << 45) | ((transf9 & 0xF) << 36) |
                       ((transf8 & 0xF) << 32) | ((transf7 & 0xF) << 28) |
                       ((transf6 & 0xF) << 24) | ((transf5 & 0xF) << 20) |
                       ((transf4 & 0xF) << 16) | ((transf3 & 0xF) << 12) |
                       ((transf2 & 0xF) << 8)  | ((transf1 & 0xF) << 4)  |
                       ((transf0 & 0xF) << 0);

    __asm__ __volatile__("csrw 0x806, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc) :);
}

/*! \fn tensor_mask(zeros, mask_bits) */
static inline __attribute__((always_inline))
void tensor_mask(uint64_t zeros, uint64_t mask_bits)
{
    uint64_t csr_enc = ((zeros & 0x000000000000) << 16) | (mask_bits & 0xFFFF);

    __asm__ __volatile__("csrw 0x805, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc) :);
}

/*! \fn tensor_coop(val)
    \brief Raw write to the tensor_coop CSR. The encoding has a
    coop id, a minion mask and a neighborhood mask. On erbium the
    minion-mask field is 8 bits (was 32 on etsoc) and the
    neighborhood-mask collapses to a single bit (one neighborhood).
    Callers are responsible for composing a valid bit pattern;
    no runtime assert here. */
static inline __attribute__((always_inline))
void tensor_coop(uint64_t val)
{
    __asm__ __volatile__("csrw 0x804, %[val]\n" : : [val] "r"(val) :);
}

/*! \fn convolution_ctrl(row_start, col_start) */
static inline __attribute__((always_inline))
void convolution_ctrl(uint64_t row_start, uint64_t col_start)
{
    uint64_t csr_enc = ((row_start & 0xFFFF) << 32) | (col_start & 0xFFFF);

    __asm__ __volatile__("csrw 0x803, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc) :);
}

/*! \fn convolution_size(srow, nrow, scol, ncol) */
static inline __attribute__((always_inline))
void convolution_size(uint64_t srow, uint64_t nrow, uint64_t scol, uint64_t ncol)
{
    uint64_t csr_enc = ((srow & 0xFF) << 56) | ((nrow & 0xFFFF) << 32) | ((scol & 0xFF) << 24) |
                       ((ncol & 0xFFFF));

    __asm__ __volatile__("csrw 0x802, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc) :);
}

/*! \fn get_tensor_error() */
static inline __attribute__((always_inline))
unsigned long get_tensor_error(void)
{
    unsigned long error;
    __asm__ __volatile__("csrr %0, 0x808" : "=r"(error));
    return error;
}

/*! \fn get_tensor_mask() */
static inline __attribute__((always_inline))
uint64_t get_tensor_mask(void)
{
    uint64_t val;
    __asm__ __volatile__("csrr %0, 0x805" : "=r"(val));
    return val;
}

#define mask_set(msk, val)                                          \
    do                                                              \
    {                                                               \
        __asm__ volatile("mov.m.x m" #msk ", zero, %0" ::"n"(val)); \
    } while (0)

#define flw_ps(fd, ptr)                                       \
    do                                                        \
    {                                                         \
        __asm__ volatile("flw.ps f" #fd ", (%0)" ::"r"(ptr)); \
    } while (0)

#define fsw_ps(fd, ptr)                                                  \
    do                                                                   \
    {                                                                    \
        __asm__ volatile("fsw.ps f" #fd ", (%0)" ::"r"(ptr) : "memory"); \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* _ERBIUM_ISA_TENSORS_H_ */
