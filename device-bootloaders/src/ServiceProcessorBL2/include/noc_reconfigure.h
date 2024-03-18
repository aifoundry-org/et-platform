/***********************************************************************
*
* Copyright (C) 2023 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file noc_reconfigure.h
    \brief A C header that defines the prerequisites needed for NOC reconfiguration
    public interfaces.
*/
/***********************************************************************/
#ifndef __NOC_RECONFIGURE_H__
#define __NOC_RECONFIGURE_H__

#include <stdint.h>
#include "hwinc/noc_esr.h"

// Define macros
#define HIGHEST_SET_BIT_POSITION(shire_mask) (uint8_t)(64 - __builtin_clzl(shire_mask) - 1)
#define DISPLACE_SHIRE(shire_mask)           (__builtin_ctzl(~shire_mask))
#define NUM_ENABLED_SHIRES(shire_mask)       (__builtin_popcountll(shire_mask))
#define NUM_SHIRES                           34
#define SPARE_SHIRE_BIT_POSITION             (NUM_SHIRES - 1)
#define MAX_SHIRE_BIT_POSITION               (SPARE_SHIRE_BIT_POSITION - 1)
#define SHIRE_MASK_DEFAULT                   ((uint64_t)((1ull << NUM_SHIRES) - 1))
#define MAGIC_SHIRE_MEM_OFFSET               0x20
#define NUM_NOC_ESR_BRIDGE                   6
#define NUM_NOC_ESR_BRIDGE_SCP               3
#define NUM_NOC_ESR_SIB_TOL3                 2
#define NUM_NOC_ESR_SIB_TOL3_SCP             1
#define NUM_NOC_ESR_SIB_TOSYS                1
#define NUM_NOC_ESR_BRIDGE_IOS               7
#define NUM_NOC_ESR_BRIDGE_IOS_DRAM          3
#define NUM_NOC_ESR_BRIDGE_IOS_SCP           2
#define NUM_NOC_ESR_BRIDGE_PS                4
#define NUM_NOC_ESR_BRIDGE_PS_SCP            1
#define NUM_NOC_ESR_BRIDGE_PS_CSR            1
#define NUM_NOC_ESR_BRIDGE_MEM               1

#define VIRTUAL_ID_MASK_CSR  0x3FC00000ul //(0xFFull << 22)
#define VIRTUAL_ID_MASK_DRAM 0x7C0ul
#define VIRTUAL_ID_MASK_SCP  0x7F800000ul

#define BRIDGE_RANGE_SCP_ADBASE 0x0000000080000000ul
#define BRIDGE_RANGE_SCP_ADMASK 0x000000ffff800000ul

#define BRIDGE_RANGE_CSR_ADBASE 0x0000000100000000ul
#define BRIDGE_RANGE_CSR_ADMASK 0x000000ff3fc00000ul

#define BRIDGE_RANGE_DRAM_ADBASE 0x0000008000000000ul
#define BRIDGE_RANGE_DRAM_ADMASK 0x000000f8000007c0ul

#define BRIDGE_RANGE_DRAM_ADBASE_VID_32 0x000000ffffffff90ul
#define BRIDGE_RANGE_DRAM_ADMASK_VID_32 0x000000ffffffffc0ul

#define BRIDGE_RANGE_DRAM_ADBASE_VID_33 0x000000fffffffe90ul
#define BRIDGE_RANGE_DRAM_ADMASK_VID_33 0x000000ffffffffc0ul

typedef enum
{
    BRIDGE_RANGE_SCP,
    BRIDGE_RANGE_CSR,
    BRIDGE_RANGE_DRAM,
} bridge_range_t;

typedef enum
{
    BRIDGE_NOC_ESR,
    BRIDGE_NOC_ESR_SIB_TOL3,
    BRIDGE_NOC_ESR_SIB_TOSYS,
    BRIDGE_NOC_ESR_BRIDGE_IOS,
    BRIDGE_NOC_ESR_BRIDGE_PS,
    BRIDGE_NOC_ESR_BRIDGE_MEM,
} bridge_t;

// This is the default Shire Virtual ID Map, based on the NOC spec:
//  	0	16	12	4	253 254
// mc0	1	8	24	28	20	5	mc4
// mc1	9	17	25	29	21	13	mc5
// mc2	2	18	26	30	22	6	mc6
// mc3	10	3	27	31	7	14	mc7
//  	11	19	32	33	23	15

// This is the default Shire Bridge ID Map, based on the NOC spec:
//      0	1	2	3	pcie0	io0
// mc0	4	5	6	7	8	9	mc4
// mc1	10	11	12	13	14	15	mc5
// mc2	16	17	18	19	20	21	mc6
// mc3	22	23	24	25	26	27	mc7
// 	28	29	30	31	32	33

// virtual shire ID -> offset multiplier for calculating bridge addresses:
int virt_to_mem_offset[NUM_SHIRES] = {
    0, 28, 8,  16, 23, 33, 14, 19, 29, 2,  15, 21, 12, 7,  20, 27, 1,
    3, 9,  22, 32, 6,  13, 26, 30, 4,  10, 17, 31, 5,  11, 18, 24, 25,
};

// clang-format off
// virtual -> new virtual shire IDs mapping when a shire is being displaced
static uint8_t new_shire_virtual_id[NUM_SHIRES][NUM_SHIRES] = {
    // Virtual Shire 0 in default map is being displaced by the spare shire
    {
        33, 16, 17, 10,
        32, 28, 30,  7, 24, 1,
         2,  3,  8, 29, 15, 31,
         0,  9, 25, 11, 20, 21,
        22, 23,  4,  5,  6, 18,
        12, 13, 14, 26, 19, 27,
    },
    // Virtual Shire 1 in the default map is being displaced by the spare shire
    {
         0, 33, 17, 10,
        32, 28, 30,  7, 24, 1,
         2,  3, 16, 29, 15, 31,
         8,  9, 25, 11, 20, 21,
        22, 23,  4,  5,  6, 18,
        12, 13, 14, 26, 19, 27,
    },
    // Virtual Shire 2 in the default map is being displaced by the spare shire
    {
         0, 24, 33, 10,
        32, 28, 30,  7, 16,  9,
         2,  3, 17,  5, 15, 31, 
         8,  1, 25, 11, 20, 29,
        22, 23,  4, 13,  6, 18,
        12, 21, 14, 26, 19, 27,
    },
    // Virtual Shire 3 in the default map is being displaced by the spare shire
    {
         0, 24, 25, 33,
        32, 28,  6,  7, 16,  9,
        10,  3,  2,  5, 15, 31,
         8,  1, 17, 11, 20, 29,
        30, 23,  4, 13, 14, 18,
        12, 21, 22, 26, 19, 27,
    },
    // Virtual Shire 4 in the default map is being displaced by the spare shire
    {
         0, 16, 17, 10,
        33, 28, 30,  7, 24,  1,
         2,  3, 32, 29, 15, 31,
         8,  9, 25, 11, 20, 21,
        22, 23,  4,  5,  6, 18,
        12, 13, 14, 26, 19, 27,
    },
    // Virtual Shire 5 in the default map is being displaced by the spare shire
    {
         0, 24, 17, 10,
        32, 33, 30,  7, 16,  1,
         2,  3,  4, 29, 15, 31,
         8,  9, 25, 11, 28, 21,
        22, 23, 12,  5,  6, 18,
        20, 13, 14, 26, 19, 27,
    },
    // Virtual Shire 6 in the default map is being displaced by the spare shire
    {
         0, 24, 25, 10,
        32, 28, 33,  7, 16,  9,
         2,  3,  6,  5, 15, 31,
         8,  1, 17, 11, 20, 29,
        30, 23,  4, 13, 14, 18,
        12, 21, 22, 26, 19, 27,
    },
    // Virtual Shire 7 in the default map is being displaced by the spare shire
    {
         0, 24, 25, 10,
        32, 28,  6, 33, 16,  9,
         2,  3,  7,  5, 15, 31,
         8,  1, 17, 11, 20, 29,
        30, 23, 4,  13, 14, 18,
        12, 21, 22, 26, 19, 27,
    },
    // Virtual Shire 8 in the default map is being displaced by the spare shire
    {
         0, 24, 17, 10,
        32, 28, 30,  7, 33,  1,
         2,  3, 16, 29, 15, 31,
         8,  9, 25, 11, 20, 21,
        22, 23,  4,  5,  6, 18,
        12, 13, 14, 26, 19, 27,
    },
    // Virtual Shire 9 in the default map is being displaced by the spare shire
    {
         0, 24, 17, 10,
        32, 28, 30,  7, 16, 33,
         2,  3,  1, 29, 15, 31,
         8,  9, 25, 11, 20, 21,
        22, 23,  4,  5,  6, 18,
        12, 13, 14, 26, 19, 27,
    },
    // Virtual Shire 10 in the default map is being displaced by the spare shire
    {
         0, 24, 25, 10,
        32, 28,  6,  7, 16,  9,
        33,  3,  2,  5, 15, 31,
         8,  1, 17, 11, 20, 29,
        30, 23,  4, 13, 14, 18,
        12, 21, 22, 26, 19, 27,
    },
    // Virtual Shire 11 in the default map is being displaced by the spare shire
    {
         0, 24, 25, 10,
        32, 28,  6, 15, 16,  9,
         2, 33,  3,  5,  7, 31, 
         8,  1, 17, 11, 20, 29,
        30, 23,  4, 13, 14, 18,
        12, 21, 22, 26, 19, 27,
    },
    // Virtual Shire 12 in the default map is being displaced by the spare shire
    {
         0, 16, 17, 10,
        32, 28, 30,  7, 24,  1,
         2,  3, 33, 29, 15, 31,
         8,  9, 25, 11, 20, 21,
        22, 23,  4,  5,  6, 18,
        12, 13, 14, 26, 19, 27,
    },
    // Virtual Shire 13 in the default map is being displaced by the spare shire
    {
         0, 24, 17, 10,
        32, 28, 30,  7, 16,  9,
         2,  3,  5, 33, 15, 31,
         8,  1, 25, 11, 20, 29,
        22, 23,  4, 13,  6, 18,
        12, 21, 14, 26, 19, 27,
    },
    // Virtual Shire 14 in the default map is being displaced by the spare shire
    {
        0, 24, 25, 10,
       32, 28,  6, 15, 16,  9,
        2,  3,  7,  5, 33, 31, 
        8,  1, 17, 11, 20, 29,
       30, 23,  4, 13, 14, 18,
       12, 21, 22, 26, 19, 27,
    },
    // Virtual Shire 15 in the default map is being displaced by the spare shire
    {
         0, 24, 25, 10,
        32, 28,  6, 15, 16,  9,
         2,  3, 23,  5,  7, 33,
         8,  1, 17, 11, 20, 29,
        30, 31, 4,  13, 14, 18,
        12, 21, 22, 26, 19, 27,
    },
    // Virtual Shire 16 in the default map is being displaced by the spare shire
    {
         0, 16, 17, 10,
        32, 28, 30,  7, 24,  1,
         2,  3,  8, 29, 15, 31,
        33,  9, 25, 11, 20, 21,
        22, 23,  4,  5,  6, 18,
        12, 13, 14, 26, 19, 27,
    },
    // Virtual Shire 17 in the default map is being displaced by the spare shire
    {
         0, 24, 17, 10,
        32, 28, 30,  7, 16,  9,
         2,  3,  1, 29, 15, 31,
         8, 33, 25, 11, 20, 21,
        22, 23,  4,  5,  6, 18,
        12, 13, 14, 26, 19, 27,
    },
    // Virtual Shire 18 in the default map is being displaced by the spare shire
    {
         0, 24, 25, 10,
        32, 28, 30,  7, 16, 9,
         2,  3, 17,  5, 15, 31,
         8,  1, 33, 11, 20, 29,
        22, 23,  4, 13,  6, 18,
        12, 21, 14, 26, 19, 27,
    },
    // Virtual Shire 19 in the default map is being displaced by the spare shire
    {
         0, 24, 25, 10,
        32, 28,  6, 15, 16,  9,
         2, 11,  3,  5,  7, 31,
         8,  1, 17, 33, 20, 29,
        30, 23,  4, 13, 14, 18,
        12, 21, 22, 26, 19, 27,
    },
    // Virtual Shire 20 in the default map is being displaced by the spare shire
    {
         0, 24, 17, 10,
        32, 28, 30,  7, 16,  1,
         2,  3,  4, 29, 15, 31,
         8,  9, 25, 11, 33, 21,
        22, 23, 12,  5,  6, 18,
        20, 13, 14, 26, 19, 27,
    },
    // Virtual Shire 21 in the default map is being displaced by the spare shire
    {
         0, 24, 17, 10,
        32, 28, 30,  7, 16,  9,
         2,  3,  5, 29, 15, 31,
         8,  1, 25, 11, 20, 33,
        22, 23,  4, 13,  6, 18,
        12, 21, 14, 26, 19, 27,
    },
    // Virtual Shire 22 in the default map is being displaced by the spare shire
    {
         0, 24, 25, 10,
        32, 28, 30,  7, 16,  9,
         2,  3,  6,  5, 15, 31,
         8,  1, 17, 11, 20, 29,
        33, 23,  4, 13, 14, 18,
        12, 21, 22, 26, 19, 27,
    },
    // Virtual Shire 23 in the default map is being displaced by the spare shire
    {
         0, 24, 25, 10,
        32, 28,  6, 15, 16,  9,
         2,  3, 23,  5,  7, 31,
         8,  1, 17, 11, 20, 29,
        30, 33,  4, 13, 14, 18,
        12, 21, 22, 26, 19, 27,
    },
    // Virtual Shire 24 in the default map is being displaced by the spare shire
    {
         0, 24, 17, 10,
        32, 28, 30,  7, 16,  1,
         2,  3,  4, 29, 15, 31,
         8,  9, 25, 11, 20, 21,
        22, 23, 33,  5,  6, 18,
        12, 13, 14, 26, 19, 27,
    },
    // Virtual Shire 25 in the default map is being displaced by the spare shire
    {
         0, 24, 17, 10,
        32, 28, 30,  7, 16,  9,
         2,  3,  5, 29, 15, 31,
         8,  1, 25, 11, 20, 21,
        22, 23,  4, 33,  6, 18,
        12, 13, 14, 26, 19, 27,
    },
    // Virtual Shire 26 in the default map is being displaced by the spare shire
    {
         0, 24, 25, 10,
        32, 28, 30,  7, 16, 9,
         2,  3,  6,  5, 15, 31,
         8,  1, 17, 11, 20, 29,
        22, 23,  4, 13, 33, 18,
        12, 21, 14, 26, 19, 27,
    },
    // Virtual Shire 27 in the default map is being displaced by the spare shire
    {
         0, 24, 25, 18,
        32, 28,  6,  7, 16,  9,
        10,  3,  2,  5, 15, 31,
         8,  1, 17, 11, 20, 29,
        30, 23,  4, 13, 14, 33,
        12, 21, 22, 26, 19, 27,
    },
    // Virtual Shire 28 in the default map is being displaced by the spare shire
    {
         0, 24, 17, 10,
        32, 28, 30,  7, 16,  1,
         2,  3,  4, 29, 15, 31,
         8,  9, 25, 11, 20, 21,
        22, 23, 12,  5,  6, 18,
        33, 13, 14, 26, 19, 27,
    },
    // Virtual Shire 29 in the default map is being displaced by the spare shire
    {
         0, 24, 17, 10,
        32, 28, 30,  7, 16,  9,
         2,  3,  5, 29, 15, 31, 
         8,  1, 25, 11, 20, 21,
        22, 23,  4, 13,  6, 18,
        12, 33, 14, 26, 19, 27,
    },
    // Virtual Shire 30 in the default map is being displaced by the spare shire
    {
         0, 24, 25, 10,
        32, 28, 30,  7, 16, 9,
         2,  3,  6,  5, 15, 31,
         8,  1, 17, 11, 20, 29,
        22, 23,  4, 13, 14, 18,
        12, 21, 33, 26, 19, 27,
    },
    // Virtual Shire 31 in the default map is being displaced by the spare shire
    {
         0, 24, 25, 18,
        32, 28,  6,  7, 16, 9,
        10,  3,  2,  5, 15, 31,
         8,  1, 17, 11, 20, 29,
        30, 23,  4, 13, 14, 26,
        12, 21, 22, 33, 19, 27,
    },
    // Virtual Shire 32 in the default map is being displaced by the spare shire
    {
         0, 24, 25, 10,
        32, 28,  6, 15, 16,  9,
         2, 11,  3,  5,  7, 31,
         8,  1, 17, 19, 20, 29,
        30, 23,  4, 13, 14, 18,
        12, 21, 22, 26, 33, 27,
    },
    // No displacement
    {
         0, 24, 25, 10,
        32, 28,  6, 15, 16,  9,
         2, 11,  3,  5,  7, 31,
         8,  1, 17, 19, 20, 29,
        30, 23,  4, 13, 14, 18,
        12, 21, 22, 26, 27, 33,
    },
};
// clang-format on

//Minion
static uint32_t adbase_noc_esr_bridge[NUM_NOC_ESR_BRIDGE] = {
    // mS, l2tol3b, L2 scratchpad
    NOC_ESR_BRIDGE_BRIDGE_L2TOL3B_M_AM_ADBASE_MEM_SH0_L3B_S_SR_SH0_L3B_S_MAP_SCP_SHIRE_CACHE_SCP_0_0_ADDRESS,
    // mS, l2tol3c, L2 scratchpad
    NOC_ESR_BRIDGE_BRIDGE_L2TOL3C_M_AM_ADBASE_MEM_SH0_L3C_S_SR_SH0_L3C_S_MAP_SCP_SHIRE_CACHE_SCP_0_0_ADDRESS,
    // mS, l2tol3d, L2 scratchpad
    NOC_ESR_BRIDGE_BRIDGE_L2TOL3D_M_AM_ADBASE_MEM_SH0_L3D_S_SR_SH0_L3D_S_MAP_SCP_SHIRE_CACHE_SCP_0_0_ADDRESS,
    // mS, l2tol3b, dram
    NOC_ESR_BRIDGE_BRIDGE_L2TOL3B_M_AM_ADBASE_MEM_SH0_L3B_S_SR_SH0_L3B_S_MAP_MEM_DRAM_M_1_0_ADDRESS,
    // mS, l2tol3c, dram
    NOC_ESR_BRIDGE_BRIDGE_L2TOL3C_M_AM_ADBASE_MEM_SH0_L3C_S_SR_SH0_L3C_S_MAP_MEM_DRAM_M_1_0_ADDRESS,
    // mS, l2tol3d, dram
    NOC_ESR_BRIDGE_BRIDGE_L2TOL3D_M_AM_ADBASE_MEM_SH0_L3D_S_SR_SH0_L3D_S_MAP_MEM_DRAM_M_1_0_ADDRESS,
};

static uint32_t adbase_noc_esr_sib_tol3[NUM_NOC_ESR_SIB_TOL3] = {
    // mS, tol3, L2 scratchpad
    NOC_ESR_SIB_TOL3_BRIDGE_SIB_TOL3_M_AM_ADBASE_MEM_SH0_L3_S_SR_SH0_L3_S_MAP_SCP_SHIRE_CACHE_SCP_0_0_ADDRESS,
    // mS, sibtol3, dram
    NOC_ESR_SIB_TOL3_BRIDGE_SIB_TOL3_M_AM_ADBASE_MEM_SH0_L3_S_SR_SH0_L3_S_MAP_MEM_DRAM_M_1_0_ADDRESS,
};

static uint32_t adbase_noc_esr_sib_tosys[NUM_NOC_ESR_SIB_TOSYS] = {
    // mS, sibtosys, esr
    NOC_ESR_SIB_TOSYS_BRIDGE_SIB_TOSYS_M_AM_ADBASE_MEM_SH0_SB_S_SR_SH0_SB_S_MAP_GLOBAL_SHIRE_CSR_0_ADDRESS,
};

//ioshire
static uint32_t adbase_noc_esr_bridge_ios[NUM_NOC_ESR_BRIDGE_IOS] = {
    // ioS, tol3, dram
    NOC_ESR_BRIDGE_IOS_BRIDGE_IO0_TOL3_M_1_14_AM_ADBASE_MEM_SH0_L3_S_SR_SH0_L3_S_MAP_MEM_DRAM_M_1_0_ADDRESS,
    // ioS, tol3b, dram
    NOC_ESR_BRIDGE_IOS_BRIDGE_IO0_TOL3B_M_2_14_AM_ADBASE_MEM_SH0_L3B_S_SR_SH0_L3B_S_MAP_MEM_DRAM_M_1_0_ADDRESS,
    // ioS, pm, dram
    NOC_ESR_BRIDGE_IOS_BRIDGE_IO0_PM_0_14_AM_ADBASE_MEM_SH0_L3D_S_SR_SH0_L3D_S_MAP_MEM_DRAM_M_1_0_ADDRESS,
    // ioS, tol3, L2 scratchpad
    NOC_ESR_BRIDGE_IOS_BRIDGE_IO0_TOL3_M_1_14_AM_ADBASE_MEM_SH0_L3_S_SR_SH0_L3_S_MAP_SCP_SHIRE_CACHE_SCP_0_0_ADDRESS,
    // ioS, tol3b, L2 scratchpad
    NOC_ESR_BRIDGE_IOS_BRIDGE_IO0_TOL3B_M_2_14_AM_ADBASE_MEM_SH0_L3B_S_SR_SH0_L3B_S_MAP_SCP_SHIRE_CACHE_SCP_0_0_ADDRESS,
    // ioS, pm, esr
    NOC_ESR_BRIDGE_IOS_BRIDGE_IO0_PM_0_14_AM_ADBASE_MEM_SH0_SB_S_SR_SH0_SB_S_MAP_GLOBAL_SHIRE_CSR_0_ADDRESS,
    // ioS, tosys, esr
    NOC_ESR_BRIDGE_IOS_BRIDGE_IO0_TOSYS_M_3_14_AM_ADBASE_MEM_SH0_SB_S_SR_SH0_SB_S_MAP_GLOBAL_SHIRE_CSR_0_ADDRESS,
};

//pshire
static uint32_t adbase_noc_esr_bridge_ps[NUM_NOC_ESR_BRIDGE_PS] = {
    // pS, tol3, L2 scratchpad
    NOC_ESR_BRIDGE_PS_BRIDGE_PS0_TOL3_M_12_13_AM_ADBASE_MEM_SH0_L3_S_SR_SH0_L3_S_MAP_SCP_SHIRE_CACHE_SCP_0_0_ADDRESS,
    // pS, tosys, esr
    NOC_ESR_BRIDGE_PS_BRIDGE_PS0_TOSYS_M_13_13_AM_ADBASE_MEM_SH0_SB_S_SR_SH0_SB_S_MAP_GLOBAL_SHIRE_CSR_0_ADDRESS,
    // pS, tosys, dram
    NOC_ESR_BRIDGE_PS_BRIDGE_PS0_TOSYS_M_13_13_AM_ADBASE_MEM_SH0_L3D_S_SR_SH0_L3D_S_MAP_MEM_DRAM_M_1_0_ADDRESS,
    // pS, tol3, dram
    NOC_ESR_BRIDGE_PS_BRIDGE_PS0_TOL3_M_12_13_AM_ADBASE_MEM_SH0_L3_S_SR_SH0_L3_S_MAP_MEM_DRAM_M_1_0_ADDRESS,
};

//mc
static uint32_t adbase_noc_esr_bridge_mem[NUM_NOC_ESR_BRIDGE_MEM] = {
    //memS, atomic_resp, esr
    NOC_ESR_BRIDGE_MEM_BRIDGE_MC_ATOMIC_RESP_M_AM_ADBASE_MEM_SH0_SB_S_SR_SH0_SB_S_MAP_GLOBAL_SHIRE_CSR_0_ADDRESS,
};

#endif
