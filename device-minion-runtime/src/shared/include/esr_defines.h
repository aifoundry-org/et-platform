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

#ifndef _ESR_DEFINES_H_
#define _ESR_DEFINES_H_


// ESR region 'base address' field in bits [39:32]
#define ESR_REGION             0x0100000000ULL  // ESR Region bit [32] == 1
#define ESR_REGION_MASK        0xFF00000000ULL  // Mask to determine if address is in the ESR region (bits [39:32])
#define ESR_REGION_MASK_SHIFT  32

// ESR region 'pp' field in bits [31:30]
#define ESR_REGION_PROT_MASK   0x00C0000000ULL  // ESR Region Protection is defined in bits [31:30]
#define ESR_REGION_PROT_SHIFT  30               // Bits to shift to get the ESR Region Protection defined in bits [31:30]

// ESR region 'shireid' field in bits [29:22]
#define ESR_REGION_SHIRE_MASK  0x003FC00000ULL  // On the ESR Region the Shire is defined by bits [29:22]
#define ESR_REGION_LOCAL_SHIRE 0x003FC00000ULL  // On the ESR Region the Local Shire is defined by bits [29:22] == 8'hff
#define ESR_REGION_SHIRE_SHIFT 22               // Bits to shift to get Shire, Shire is defined by bits [19:22]
#define ESR_REGION_OFFSET      0x0000400000ULL

// ESR region 'subregion' field in bits [21:20]
#define ESR_SREGION_MASK       0x0000300000ULL  // The ESR Region is defined by bits [21:20] and further limited by bits [19:12] depending on the region
#define ESR_SREGION_SHIFT      20               // Bits to shift to get Region, Region is defined in bits [21:20]

// ESR region 'extregion' field in bits [21:17] (when 'subregion' is 2'b11)
#define ESR_SREGION_EXT_MASK   0x00003E0000ULL  // The ESR Extended Region is defined by bits [21:17]
#define ESR_SREGION_EXT_SHIFT  17               // Bits to shift to get Extended Region, Extended Region is defined in bits [21:17]

// ESR region 'hart' field in bits [19:12] (when 'subregion' is 2'b00)
#define ESR_HART_MASK          0x00000FF000ULL  // On HART ESR Region the HART is defined in bits [19:12]
#define ESR_HART_SHIFT         12               // On HART ESR Region bits to shift to get the HART defined in bits [19:12]

// ESR region 'neighborhood' field in bits [19:16] (when 'subregion' is 2'b01)
#define ESR_NEIGH_MASK         0x00000F0000ULL  // On Neighborhood ESR Region Neighborhood is defined when bits [19:16]
#define ESR_NEIGH_SHIFT        16               // On Neighborhood ESR Region bits to shift to get the Neighborhood defined in bits [19:16]
#define ESR_NEIGH_OFFSET       0x0000010000ULL  // On Neighborhood ESR Region the Neighborhood is defined by bits [19:16]
#define ESR_NEIGH_BROADCAST    15               // On Neighborhood ESR Region Neighborhood Broadcast is defined when bits [19:16] == 4'b1111

// ESR region 'bank' field in bits [16:13] (when 'subregion' is 2'b11)
#define ESR_BANK_MASK          0x000001E000ULL  // On Shire Cache ESR Region Bank is defined in bits [16:13]
#define ESR_BANK_SHIFT         13               // On Shire Cache ESR Region bits to shift to get Bank defined in bits [16:13]

// ESR region 'ESR' field in bits [xx:3] (depends on 'subregion' and 'extregion' fields)
#define ESR_HART_ESR_MASK      0x0000000FF8ULL  // On HART ESR Region the ESR is defined by bits [11:3]
#define ESR_NEIGH_ESR_MASK     0x000000FFF8ULL  // On Neighborhood ESR Region the ESR is defined in bits [15:3]
#define ESR_SC_ESR_MASK        0x0000001FF8ULL  // On Shire Cache ESR Region the ESR is defined in bits [12:3]
#define ESR_SHIRE_ESR_MASK     0x000001FFF8ULL  // On Shire ESR Region the ESR is defined in bits [16:3]
#define ESR_RBOX_ESR_MASK      0x000001FFF8ULL  // On RBOX ESR Region the ESR is defined in bits [16:3]
#define ESR_ESR_ID_SHIFT       3

// Base addresses for the various ESR subregions
#define ESR_HART_REGION        0x0100000000ULL  // Neighborhood ESR Region is at region [21:20] == 2'b00 and [11] == 1'b0
#define ESR_NEIGH_REGION       0x0100100000ULL  // Neighborhood ESR Region is at region [21:20] == 2'b01
#define ESR_CACHE_REGION       0x0100300000ULL  // Shire Cache ESR Region is at region [21:20] == 2'b11 and [19:17] == 2'b000
#define ESR_RBOX_REGION        0x0100320000ULL  // RBOX ESR Region is at region [21:20] == 2'b11 and [19:17] == 2'b001
#define ESR_SHIRE_REGION       0x0100340000ULL  // Shire ESR Region is at region [21:20] == 2'b11 and [19:17] == 2'b010

// Helper macros to construct ESR addresses in the various subregions

#define ESR_HART(prot, shire, hart, name) \
    ((uint64_t)(ESR_HART_REGION) + \
     ((uint64_t)(prot) << ESR_REGION_PROT_SHIFT) + \
     ((uint64_t)(shire) << ESR_REGION_SHIRE_SHIFT) + \
     ((uint64_t)(hart) << ESR_HART_SHIFT) + \
      (uint64_t)(ESR_HART_ ## name))

#define ESR_NEIGH(prot, shire, neigh, name) \
    ((uint64_t)(ESR_NEIGH_REGION) + \
     ((uint64_t)(prot) << ESR_REGION_PROT_SHIFT) + \
     ((uint64_t)(shire) << ESR_REGION_SHIRE_SHIFT) + \
     ((uint64_t)(neigh) << ESR_NEIGH_SHIFT) + \
      (uint64_t)(ESR_NEIGH_ ## name))

#define ESR_CACHE(prot, shire, bank, name) \
    ((uint64_t)(ESR_CACHE_REGION) + \
     ((uint64_t)(prot) << ESR_REGION_PROT_SHIFT) + \
     ((uint64_t)(shire) << ESR_REGION_SHIRE_SHIFT) + \
     ((uint64_t)(bank) << ESR_BANK_SHIFT) + \
      (uint64_t)(ESR_CACHE_ ## name))

#define ESR_RBOX(prot, shire, name) \
    ((uint64_t)(ESR_RBOX_REGION) + \
     ((uint64_t)(prot) << ESR_REGION_PROT_SHIFT) + \
     ((uint64_t)(shire) << ESR_REGION_SHIRE_SHIFT) + \
      (uint64_t)(ESR_RBOX_ ## name))

#define ESR_SHIRE(prot, shire, name) \
    ((uint64_t)(ESR_SHIRE_REGION) + \
     ((uint64_t)(prot) << ESR_REGION_PROT_SHIFT) + \
     ((uint64_t)(shire) << ESR_REGION_SHIRE_SHIFT) + \
      (uint64_t)(ESR_SHIRE_ ## name))

// Hart ESRs
#define ESR_HART_0                      0x000   /* PP = 0b00 */
#define ESR_HART_PORT0                  0x800   /* PP = 0b00 */
#define ESR_HART_PORT1                  0x840   /* PP = 0b00 */
#define ESR_HART_PORT2                  0x880   /* PP = 0b00 */
#define ESR_HART_PORT3                  0x8C0   /* PP = 0b00 */

// Neighborhood ESRs
#define ESR_NEIGH_0                     0x0000  /* PP = 0b00 */
#define ESR_NEIGH_MPROT                 0x0020  /* PP = 0b03 */
#define ESR_NEIGH_IPI_REDIRECT_PC       0x0040  /* PP = 0b00 */
#define ESR_NEIGH_TBOX_CONTROL          0x8000  /* PP = 0b00 */
#define ESR_NEIGH_TBOX_STATUS           0x8008  /* PP = 0b00 */
#define ESR_NEIGH_TBOX_IMGT_PTR         0x8010  /* PP = 0b00 */

// ShireCache ESRs
#define ESR_CACHE_0                     0x0000  /* PP = 0b00 */
#define ESR_CACHE_IDX_COP_SM_CTL        0x0030  /* PP = 0b00 */
#define ESR_CACHE_UNKNOWN               0x01D8  /* PP = 0b00 */

// RBOX ESRs
#define ESR_RBOX_0                      0x00000 /* PP = 0b00 */
#define ESR_RBOX_CONFIG                 0x00000 /* PP = 0b00 */
#define ESR_RBOX_IN_BUF_PG              0x00001 /* PP = 0b00 */
#define ESR_RBOX_IN_BUF_CFG             0x00002 /* PP = 0b00 */
#define ESR_RBOX_OUT_BUF_PG             0x00003 /* PP = 0b00 */
#define ESR_RBOX_OUT_BUF_CFG            0x00004 /* PP = 0b00 */
#define ESR_RBOX_STATUS                 0x00005 /* PP = 0b00 */
#define ESR_RBOX_START                  0x00006 /* PP = 0b00 */
#define ESR_RBOX_CONSUME                0x00007 /* PP = 0b00 */

// Shire ESRs
#define ESR_SHIRE_0                         0x00000 /* PP = 0b00 */
#define ESR_SHIRE_MINION_FEATURE            0x00000
#define ESR_SHIRE_IPI_REDIRECT_TRIGGER      0x00080
#define ESR_SHIRE_IPI_REDIRECT_FILTER       0x00088
#define ESR_SHIRE_IPI_TRIGGER               0x00090
#define ESR_SHIRE_IPI_TRIGGER_CLEAR         0x00098
#define ESR_SHIRE_FCC0                      0x000C0
#define ESR_SHIRE_FCC1                      0x000C8
#define ESR_SHIRE_FCC2                      0x000D0
#define ESR_SHIRE_FCC3                      0x000D8
#define ESR_SHIRE_FLB0                      0x00100
#define ESR_SHIRE_FLB1                      0x00108
#define ESR_SHIRE_FLB2                      0x00110
#define ESR_SHIRE_FLB3                      0x00118
#define ESR_SHIRE_FLB4                      0x00120
#define ESR_SHIRE_FLB5                      0x00128
#define ESR_SHIRE_FLB6                      0x00130
#define ESR_SHIRE_FLB7                      0x00138
#define ESR_SHIRE_FLB8                      0x00140
#define ESR_SHIRE_FLB9                      0x00148
#define ESR_SHIRE_FLB10                     0x00150
#define ESR_SHIRE_FLB11                     0x00158
#define ESR_SHIRE_FLB12                     0x00160
#define ESR_SHIRE_FLB13                     0x00168
#define ESR_SHIRE_FLB14                     0x00170
#define ESR_SHIRE_FLB15                     0x00178
#define ESR_SHIRE_FLB16                     0x00180
#define ESR_SHIRE_FLB17                     0x00188
#define ESR_SHIRE_FLB18                     0x00190
#define ESR_SHIRE_FLB19                     0x00198
#define ESR_SHIRE_FLB20                     0x001A0
#define ESR_SHIRE_FLB21                     0x001A8
#define ESR_SHIRE_FLB22                     0x001B0
#define ESR_SHIRE_FLB23                     0x001B8
#define ESR_SHIRE_FLB24                     0x001C0
#define ESR_SHIRE_FLB25                     0x001C8
#define ESR_SHIRE_FLB26                     0x001D0
#define ESR_SHIRE_FLB27                     0x001D8
#define ESR_SHIRE_FLB28                     0x001E0
#define ESR_SHIRE_FLB29                     0x001E8
#define ESR_SHIRE_FLB30                     0x001F0
#define ESR_SHIRE_FLB31                     0x001F8
#define ESR_SHIRE_COOP_MODE                 0x00290
#define ESR_SHIRE_ICACHE_UPREFETCH          0x002F8
#define ESR_SHIRE_ICACHE_SPREFETCH          0x00300
#define ESR_SHIRE_ICACHE_MPREFETCH          0x00308
#define ESR_SHIRE_BROADCAST0                0x1FFF0
#define ESR_SHIRE_BROADCAST1                0x1FFF8

// Broadcast ESR fields
#define ESR_BROADCAST_PROT_MASK              0x1800000000000000ULL // Region protection is defined in bits [60:59] in esr broadcast data write.
#define ESR_BROADCAST_PROT_SHIFT             59
#define ESR_BROADCAST_ESR_SREGION_MASK       0x07C0000000000000ULL // bits [21:17] in Memory Shire Esr Map. Esr region.
#define ESR_BROADCAST_ESR_SREGION_MASK_SHIFT 54
#define ESR_BROADCAST_ESR_ADDR_MASK          0x003FFF0000000000ULL // bits[17:3] in Memory Shire Esr Map. Esr address
#define ESR_BROADCAST_ESR_ADDR_SHIFT         40
#define ESR_BROADCAST_ESR_SHIRE_MASK         0x000000FFFFFFFFFFULL // bit mask Shire to spread the broadcast bits
#define ESR_BROADCAST_ESR_MAX_SHIRES         ESR_BROADCAST_ESR_ADDR_SHIFT

// Atomic and IPI defines
#define ATOMIC_REGION ESR_SHIRE(PRV_U, SHIRE_OWN, FLB0)
#define IPI_THREAD0   ESR_SHIRE(PRV_U, SHIRE_OWN, FCC0)
#define IPI_THREAD1   ESR_SHIRE(PRV_U, SHIRE_OWN, FCC2)
#define IPI_NET       ESR_HART (PRV_U, SHIRE_MASTER, 0, PORT0)
#define FCC0_MASTER   ESR_SHIRE(PRV_U, SHIRE_MASTER, FCC0)
#define FCC1_MASTER   ESR_SHIRE(PRV_U, SHIRE_MASTER, FCC1)

// Privilege defines
#define PRV_U 0
#define PRV_S 1
#define PRV_H 2
#define PRV_M 3

// Shire defines
#define SHIRE_MASTER 32
#define SHIRE_OWN    0xFF

// Thread defines
#define THREAD_0 0
#define THREAD_1 1

#endif
