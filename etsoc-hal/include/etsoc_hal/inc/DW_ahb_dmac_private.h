/* --------------------------------------------------------------------
** 
** Synopsys DesignWare DW_ahb_dmac Software Driver Kit and
** documentation (hereinafter, "Software") is an Unsupported
** proprietary work of Synopsys, Inc. unless otherwise expressly
** agreed to in writing between Synopsys and you.
** 
** The Software IS NOT an item of Licensed Software or Licensed
** Product under any End User Software License Agreement or Agreement
** for Licensed Product with Synopsys or any supplement thereto. You
** are permitted to use and redistribute this Software in source and
** binary forms, with or without modification, provided that
** redistributions of source code must retain this notice. You may not
** view, use, disclose, copy or distribute this file or any information
** contained herein except pursuant to this license grant from Synopsys.
** If you do not agree with this notice, including the disclaimer
** below, then you are not authorized to use the Software.
** 
** THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS"
** BASIS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
** FOR A PARTICULAR PURPOSE ARE HEREBY DISCLAIMED. IN NO EVENT SHALL
** SYNOPSYS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
** OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
** USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
** DAMAGE.
** 
** --------------------------------------------------------------------
*/

#ifndef DW_AHB_DMAC_PUBLIC_H
#error "The DMA public header file must be included before the private header file"
#endif

#ifndef DW_AHB_DMAC_PRIVATE_H
#define DW_AHB_DMAC_PRIVATE_H

// Common requirements (preconditions) for all dmac driver functions
#define DMAC_COMMON_REQUIREMENTS(p)             \
do {                                            \
    DW_REQUIRE(p != NULL);                      \
    DW_REQUIRE(p->instance != NULL);            \
    DW_REQUIRE(p->comp_param != NULL);          \
    DW_REQUIRE(p->base_address != NULL);        \
    DW_REQUIRE(p->comp_type == Dw_ahb_dmac);    \
} while(0)

// Macro definitions for DMA controller limits
#define DMAC_MAX_CHANNELS    8
#define DMAC_MAX_INTERRUPTS  5

// Macro definitions for the flow control mode of the DMA Controller
#define DMAC_DMA_FC_ONLY     0x0
#define DMAC_SRC_FC_ONLY     0x1
#define DMAC_DST_FC_ONLY     0x2
#define DMAC_ANY_FC          0x3

// Macro definitions for channel masks

// i.e if DMAC_MAX_CHANNELS = 8 : = 0xff 
#define DMAC_MAX_CH_MASK  \
     (~(~0x0u << DMAC_MAX_CHANNELS))

// i.e. if NUM_CHANNELS = 3 : = 0xf8 
#define DMAC_CH_MASK \
     (DMAC_MAX_CH_MASK & (DMAC_MAX_CH_MASK << param->num_channels))

// i.e. if NUM_CHANNELS = 3 : = 0x7 
#define DMAC_CH_EN_MASK \
     (DMAC_MAX_CH_MASK & ~(DMAC_CH_MASK))

// i.e. if NUM_CHANNELS = 3 : = 0x707 
#define DMAC_CH_ALL_MASK \
     ((DMAC_CH_EN_MASK) + ((DMAC_CH_EN_MASK) << DMAC_MAX_CHANNELS))

// get a dw_dmac_channel_number from a channel index
#define DMAC_CH_NUM(ch_idx)     (0x101 << (ch_idx))

// power of 2 macro
#define POW2(pow) \
     (1 << (pow))

// Is A greater than or equal to B macro ?
#define A_MAXEQ_B(a,b) \
  ((a) >= (b))


/* allow C++ to use this header */
#ifdef __cplusplus
extern "C" {
#endif

/****id* dmac.api/dmac.bfws
 * DESCRIPTION
 *  Used in conjunction with bitops.h to access register bitfields.
 *  They are defined as bit offset/mask pairs for each DMA register
 *  bitfield.
 * EXAMPLE
 *  int_status = BIT_GET(INP(portmap->status_int_l),
 *               DMAC_STATUSINT_L_DSTTRAN);
 * NOTES
 *  bfo is the offset of the bitfield with respect to LSB;
 *  bfw is the width of the bitfield
 * SEE ALSO
 *  dw_common_bitops.h
 * SOURCE
 */
#define bfoDMAC_DMACFGREG_L_DMA_EN     ((uint32_t)    0)
#define bfwDMAC_DMACFGREG_L_DMA_EN     ((uint32_t)    1)

#define bfoDMAC_CHENREG_L_CH_EN(ch)    ((uint32_t)   (ch))
#define bfwDMAC_CHENREG_L_CH_EN(ch)    ((uint32_t)    1)
#define bfoDMAC_CHENREG_L_CH_EN_ALL    ((uint32_t)    0)
#define bfwDMAC_CHENREG_L_CH_EN_ALL    ((uint32_t)    8)

#define bfoDMAC_CHENREG_L_CH_EN_WE(ch) ((uint32_t) (ch)+8)
#define bfwDMAC_CHENREG_L_CH_EN_WE(ch) ((uint32_t)    1)
#define bfoDMAC_CHENREG_L_CH_EN_WE_ALL ((uint32_t)    8)
#define bfwDMAC_CHENREG_L_CH_EN_WE_ALL ((uint32_t)    8)

#define bfoDMAC_SAR_L_SAR              ((uint32_t)    0)
#define bfwDMAC_SAR_L_SAR              ((uint32_t)   32)
#define bfoDMAC_DAR_L_DAR              ((uint32_t)    0)
#define bfwDMAC_DAR_L_DAR              ((uint32_t)   32)

#define bfoDMAC_LLP_L_LMS              ((uint32_t)    0)
#define bfwDMAC_LLP_L_LMS              ((uint32_t)    2)
#define bfoDMAC_LLP_L_LOC              ((uint32_t)    2)
#define bfwDMAC_LLP_L_LOC              ((uint32_t)   30)

#define bfoDMAC_CTL_L_INT_EN           ((uint32_t)    0)
#define bfwDMAC_CTL_L_INT_EN           ((uint32_t)    1)
#define bfoDMAC_CTL_L_DST_TR_WIDTH     ((uint32_t)    1)
#define bfwDMAC_CTL_L_DST_TR_WIDTH     ((uint32_t)    3)
#define bfoDMAC_CTL_L_SRC_TR_WIDTH     ((uint32_t)    4)
#define bfwDMAC_CTL_L_SRC_TR_WIDTH     ((uint32_t)    3)
#define bfoDMAC_CTL_L_DINC             ((uint32_t)    7)
#define bfwDMAC_CTL_L_DINC             ((uint32_t)    2)
#define bfoDMAC_CTL_L_SINC             ((uint32_t)    9)
#define bfwDMAC_CTL_L_SINC             ((uint32_t)    2)
#define bfoDMAC_CTL_L_DEST_MSIZE       ((uint32_t)   11)
#define bfwDMAC_CTL_L_DEST_MSIZE       ((uint32_t)    3)
#define bfoDMAC_CTL_L_SRC_MSIZE        ((uint32_t)   14)
#define bfwDMAC_CTL_L_SRC_MSIZE        ((uint32_t)    3)
#define bfoDMAC_CTL_L_SRC_GATHER_EN    ((uint32_t)   17)
#define bfwDMAC_CTL_L_SRC_GATHER_EN    ((uint32_t)    1)
#define bfoDMAC_CTL_L_DST_SCATTER_EN   ((uint32_t)   18)
#define bfwDMAC_CTL_L_DST_SCATTER_EN   ((uint32_t)    1)
#define bfoDMAC_CTL_L_TT_FC            ((uint32_t)   20)
#define bfwDMAC_CTL_L_TT_FC            ((uint32_t)    3)
#define bfoDMAC_CTL_L_DMS              ((uint32_t)   23)
#define bfwDMAC_CTL_L_DMS              ((uint32_t)    2)
#define bfoDMAC_CTL_L_SMS              ((uint32_t)   25)
#define bfwDMAC_CTL_L_SMS              ((uint32_t)    2)
#define bfoDMAC_CTL_L_LLP_DST_EN       ((uint32_t)   27)
#define bfwDMAC_CTL_L_LLP_DST_EN       ((uint32_t)    1)
#define bfoDMAC_CTL_L_LLP_SRC_EN       ((uint32_t)   28)
#define bfwDMAC_CTL_L_LLP_SRC_EN       ((uint32_t)    1)

#define bfoDMAC_CTL_H_BLOCK_TS         ((uint32_t)    0)
#define bfwDMAC_CTL_H_BLOCK_TS         ((uint32_t)   12)
#define bfoDMAC_CTL_H_DONE             ((uint32_t)   12)
#define bfwDMAC_CTL_H_DONE             ((uint32_t)    1)

#define bfoDMAC_SSTAT_L_SSTAT          ((uint32_t)    0)
#define bfwDMAC_SSTAT_L_SSTAT          ((uint32_t)   32)

#define bfoDMAC_DSTAT_L_DSTAT          ((uint32_t)    0)
#define bfwDMAC_DSTAT_L_DSTAT          ((uint32_t)   32)

#define bfoDMAC_SSTATAR_L_SSTATAR      ((uint32_t)    0)
#define bfwDMAC_SSTATAR_L_SSTATAR      ((uint32_t)   32)

#define bfoDMAC_DSTATAR_L_DSTATAR      ((uint32_t)    0)
#define bfwDMAC_DSTATAR_L_DSTATAR      ((uint32_t)   32)

#define bfoDMAC_CFG_L_CH_PRIOR         ((uint32_t)    5)
#define bfwDMAC_CFG_L_CH_PRIOR         ((uint32_t)    3)
#define bfoDMAC_CFG_L_CH_SUSP          ((uint32_t)    8)
#define bfwDMAC_CFG_L_CH_SUSP          ((uint32_t)    1)
#define bfoDMAC_CFG_L_FIFO_EMPTY       ((uint32_t)    9)
#define bfwDMAC_CFG_L_FIFO_EMPTY       ((uint32_t)    1)
#define bfoDMAC_CFG_L_HS_SEL_DST       ((uint32_t)   10)
#define bfwDMAC_CFG_L_HS_SEL_DST       ((uint32_t)    1)
#define bfoDMAC_CFG_L_HS_SEL_SRC       ((uint32_t)   11)
#define bfwDMAC_CFG_L_HS_SEL_SRC       ((uint32_t)    1)
#define bfoDMAC_CFG_L_LOCK_CH_L        ((uint32_t)   12)
#define bfwDMAC_CFG_L_LOCK_CH_L        ((uint32_t)    2)
#define bfoDMAC_CFG_L_LOCK_B_L         ((uint32_t)   14)
#define bfwDMAC_CFG_L_LOCK_B_L         ((uint32_t)    2)
#define bfoDMAC_CFG_L_LOCK_CH          ((uint32_t)   16)
#define bfwDMAC_CFG_L_LOCK_CH          ((uint32_t)    1)
#define bfoDMAC_CFG_L_LOCK_B           ((uint32_t)   17)
#define bfwDMAC_CFG_L_LOCK_B           ((uint32_t)    1)
#define bfoDMAC_CFG_L_DST_HS_POL       ((uint32_t)   18)
#define bfwDMAC_CFG_L_DST_HS_POL       ((uint32_t)    1)
#define bfoDMAC_CFG_L_SRC_HS_POL       ((uint32_t)   19)
#define bfwDMAC_CFG_L_SRC_HS_POL       ((uint32_t)    1)
#define bfoDMAC_CFG_L_MAX_ABRST        ((uint32_t)   20)
#define bfwDMAC_CFG_L_MAX_ABRST        ((uint32_t)   10)
#define bfoDMAC_CFG_L_RELOAD_SRC       ((uint32_t)   30)
#define bfwDMAC_CFG_L_RELOAD_SRC       ((uint32_t)    1)
#define bfoDMAC_CFG_L_RELOAD_DST       ((uint32_t)   31)
#define bfwDMAC_CFG_L_RELOAD_DST       ((uint32_t)    1)

#define bfoDMAC_CFG_H_FCMODE           ((uint32_t)    0)
#define bfwDMAC_CFG_H_FCMODE           ((uint32_t)    1)
#define bfoDMAC_CFG_H_FIFO_MODE        ((uint32_t)    1)
#define bfwDMAC_CFG_H_FIFO_MODE        ((uint32_t)    1)
#define bfoDMAC_CFG_H_PROTCTL          ((uint32_t)    2)
#define bfwDMAC_CFG_H_PROTCTL          ((uint32_t)    3)
#define bfoDMAC_CFG_H_DS_UPD_EN        ((uint32_t)    5)
#define bfwDMAC_CFG_H_DS_UPD_EN        ((uint32_t)    1)
#define bfoDMAC_CFG_H_SS_UPD_EN        ((uint32_t)    6)
#define bfwDMAC_CFG_H_SS_UPD_EN        ((uint32_t)    1)
#define bfoDMAC_CFG_H_SRC_PER          ((uint32_t)    7)
#define bfwDMAC_CFG_H_SRC_PER          ((uint32_t)    4)
#define bfoDMAC_CFG_H_DEST_PER         ((uint32_t)   11)
#define bfwDMAC_CFG_H_DEST_PER         ((uint32_t)    4)

#define bfoDMAC_SGR_L_SGI              ((uint32_t)    0)
#define bfwDMAC_SGR_L_SGI              ((uint32_t)   20)
#define bfoDMAC_SGR_L_SGC              ((uint32_t)   20)
#define bfwDMAC_SGR_L_SGC              ((uint32_t)   12)

#define bfoDMAC_DSR_L_DSI              ((uint32_t)    0)
#define bfwDMAC_DSR_L_DSI              ((uint32_t)   20)
#define bfoDMAC_DSR_L_DSC              ((uint32_t)   20)
#define bfwDMAC_DSR_L_DSC              ((uint32_t)   12)

#define bfoDMAC_INT_RAW_STAT_CLR(ch)   ((uint32_t) (ch))
#define bfwDMAC_INT_RAW_STAT_CLR(ch)   ((uint32_t)    1)
#define bfoDMAC_INT_RAW_STAT_CLR_ALL   ((uint32_t)    0)
#define bfwDMAC_INT_RAW_STAT_CLR_ALL   ((uint32_t)    8)

#define bfoDMAC_INT_MASK_L(ch)         ((uint32_t) (ch))
#define bfwDMAC_INT_MASK_L(ch)         ((uint32_t)    1)
#define bfoDMAC_INT_MASK_L_ALL         ((uint32_t)    0)
#define bfwDMAC_INT_MASK_L_ALL         ((uint32_t)    8)
#define bfoDMAC_INT_MASK_L_WE(ch)      ((uint32_t) (ch)+8)
#define bfwDMAC_INT_MASK_L_WE(ch)      ((uint32_t)    1)
#define bfoDMAC_INT_MASK_L_WE_ALL      ((uint32_t)    8)
#define bfwDMAC_INT_MASK_L_WE_ALL      ((uint32_t)    8)

#define bfoDMAC_STATUSINT_L_TFR        ((uint32_t)    0)
#define bfwDMAC_STATUSINT_L_TFR        ((uint32_t)    1)
#define bfoDMAC_STATUSINT_L_BLOCK      ((uint32_t)    1)
#define bfwDMAC_STATUSINT_L_BLOCK      ((uint32_t)    1)
#define bfoDMAC_STATUSINT_L_SRCTRAN    ((uint32_t)    2)
#define bfwDMAC_STATUSINT_L_SRCTRAN    ((uint32_t)    1)
#define bfoDMAC_STATUSINT_L_DSTTRAN    ((uint32_t)    3)
#define bfwDMAC_STATUSINT_L_DSTTRAN    ((uint32_t)    1)
#define bfoDMAC_STATUSINT_L_ERR        ((uint32_t)    4)
#define bfwDMAC_STATUSINT_L_ERR        ((uint32_t)    1)

#define bfoDMAC_SW_HANDSHAKE_L(ch)     ((uint32_t)   (ch))
#define bfwDMAC_SW_HANDSHAKE_L(ch)     ((uint32_t)    1)
#define bfoDMAC_SW_HANDSHAKE_L_WE(ch)  ((uint32_t) (ch)+8)
#define bfwDMAC_SW_HANDSHAKE_L_WE(ch)  ((uint32_t)    1)

#define bfoDMAC_DMALD_L_DMA_ID         ((uint32_t)    0)
#define bfwDMAC_DMALD_L_DMA_ID         ((uint32_t)   32)

#define bfoDMAC_DMATESTREG_L_TEST_SLV_IF ((uint32_t)  0)
#define bfwDMAC_DMATESTREG_L_TEST_SLV_IF ((uint32_t)  1)

#define bfoDMAC_DMACOMPVER_L_DMACOMPVER  ((uint32_t)  0)
#define bfwDMAC_DMACOMPVER_L_DMACOMPVER  ((uint32_t) 32)

#define bfoDMAC_PARAM_CHX_DTW           ((uint32_t)    0)
#define bfwDMAC_PARAM_CHX_DTW           ((uint32_t)    3)
#define bfoDMAC_PARAM_CHX_STW           ((uint32_t)    3)
#define bfwDMAC_PARAM_CHX_STW           ((uint32_t)    3)
#define bfoDMAC_PARAM_CHX_STAT_DST      ((uint32_t)    6)
#define bfwDMAC_PARAM_CHX_STAT_DST      ((uint32_t)    1)
#define bfoDMAC_PARAM_CHX_STAT_SRC      ((uint32_t)    7)
#define bfwDMAC_PARAM_CHX_STAT_SRC      ((uint32_t)    1)
#define bfoDMAC_PARAM_CHX_DST_SCA_EN    ((uint32_t)    8)
#define bfwDMAC_PARAM_CHX_DST_SCA_EN    ((uint32_t)    1)
#define bfoDMAC_PARAM_CHX_SRC_GAT_EN    ((uint32_t)    9)
#define bfwDMAC_PARAM_CHX_SRC_GAT_EN    ((uint32_t)    1)
#define bfoDMAC_PARAM_CHX_LOCK_EN       ((uint32_t)   10)
#define bfwDMAC_PARAM_CHX_LOCK_EN       ((uint32_t)    1)
#define bfoDMAC_PARAM_CHX_MULTI_BLK_EN  ((uint32_t)   11)
#define bfwDMAC_PARAM_CHX_MULTI_BLK_EN  ((uint32_t)    1)
#define bfoDMAC_PARAM_CHX_CTL_WB_EN     ((uint32_t)   12)
#define bfwDMAC_PARAM_CHX_CTL_WB_EN     ((uint32_t)    1)
#define bfoDMAC_PARAM_CHX_HC_LLP        ((uint32_t)   13)
#define bfwDMAC_PARAM_CHX_HC_LLP        ((uint32_t)    1)
#define bfoDMAC_PARAM_CHX_FC            ((uint32_t)   14)
#define bfwDMAC_PARAM_CHX_FC            ((uint32_t)    2)
#define bfoDMAC_PARAM_CHX_MAX_MULT_SIZE ((uint32_t)   16)
#define bfwDMAC_PARAM_CHX_MAX_MULT_SIZE ((uint32_t)    3)
#define bfoDMAC_PARAM_CHX_DMS           ((uint32_t)   19)
#define bfwDMAC_PARAM_CHX_DMS           ((uint32_t)    3)
#define bfoDMAC_PARAM_CHX_LMS           ((uint32_t)   22)
#define bfwDMAC_PARAM_CHX_LMS           ((uint32_t)    3)
#define bfoDMAC_PARAM_CHX_SMS           ((uint32_t)   25)
#define bfwDMAC_PARAM_CHX_SMS           ((uint32_t)    3)
#define bfoDMAC_PARAM_CHX_FIFO_DEPTH    ((uint32_t)   28)
#define bfwDMAC_PARAM_CHX_FIFO_DEPTH    ((uint32_t)    3)

#define bfoDMAC_PARAM_BIG_ENDIAN        ((uint32_t)    0)
#define bfwDMAC_PARAM_BIG_ENDIAN        ((uint32_t)    1)
#define bfoDMAC_PARAM_INTR_IO           ((uint32_t)    1)
#define bfwDMAC_PARAM_INTR_IO           ((uint32_t)    2)
#define bfoDMAC_PARAM_MABRST            ((uint32_t)    3)
#define bfwDMAC_PARAM_MABRST            ((uint32_t)    1)
#define bfoDMAC_PARAM_NUM_CHANNELS      ((uint32_t)    8)
#define bfwDMAC_PARAM_NUM_CHANNELS      ((uint32_t)    3)
#define bfoDMAC_PARAM_NUM_MASTER_INT    ((uint32_t)   11)
#define bfwDMAC_PARAM_NUM_MASTER_INT    ((uint32_t)    2)
#define bfoDMAC_PARAM_S_HDATA_WIDTH     ((uint32_t)   13)
#define bfwDMAC_PARAM_S_HDATA_WIDTH     ((uint32_t)    2)
#define bfoDMAC_PARAM_M1_HDATA_WIDTH    ((uint32_t)   15)
#define bfwDMAC_PARAM_M1_HDATA_WIDTH    ((uint32_t)    2)
#define bfoDMAC_PARAM_M2_HDATA_WIDTH    ((uint32_t)   17)
#define bfwDMAC_PARAM_M2_HDATA_WIDTH    ((uint32_t)    2)
#define bfoDMAC_PARAM_M3_HDATA_WIDTH    ((uint32_t)   19)
#define bfwDMAC_PARAM_M3_HDATA_WIDTH    ((uint32_t)    2)
#define bfoDMAC_PARAM_M4_HDATA_WIDTH    ((uint32_t)   21)
#define bfwDMAC_PARAM_M4_HDATA_WIDTH    ((uint32_t)    2)
#define bfoDMAC_PARAM_NUM_HS_INT        ((uint32_t)   23)
#define bfwDMAC_PARAM_NUM_HS_INT        ((uint32_t)    5)
#define bfoDMAC_PARAM_ADD_ENCODED_PARAMS ((uint32_t)  28)
#define bfwDMAC_PARAM_ADD_ENCODED_PARAMS ((uint32_t)   1)

#define bfoDMAC_PARAM_CH0_MAX_BLK_SIZE  ((uint32_t)    0)
#define bfwDMAC_PARAM_CH0_MAX_BLK_SIZE  ((uint32_t)    4)
#define bfoDMAC_PARAM_CH1_MAX_BLK_SIZE  ((uint32_t)    4)
#define bfwDMAC_PARAM_CH1_MAX_BLK_SIZE  ((uint32_t)    4)
#define bfoDMAC_PARAM_CH2_MAX_BLK_SIZE  ((uint32_t)    8)
#define bfwDMAC_PARAM_CH2_MAX_BLK_SIZE  ((uint32_t)    4)
#define bfoDMAC_PARAM_CH3_MAX_BLK_SIZE  ((uint32_t)   12)
#define bfwDMAC_PARAM_CH3_MAX_BLK_SIZE  ((uint32_t)    4)
#define bfoDMAC_PARAM_CH4_MAX_BLK_SIZE  ((uint32_t)   16)
#define bfwDMAC_PARAM_CH4_MAX_BLK_SIZE  ((uint32_t)    4)
#define bfoDMAC_PARAM_CH5_MAX_BLK_SIZE  ((uint32_t)   20)
#define bfwDMAC_PARAM_CH5_MAX_BLK_SIZE  ((uint32_t)    4)
#define bfoDMAC_PARAM_CH6_MAX_BLK_SIZE  ((uint32_t)   24)
#define bfwDMAC_PARAM_CH6_MAX_BLK_SIZE  ((uint32_t)    4)
#define bfoDMAC_PARAM_CH7_MAX_BLK_SIZE  ((uint32_t)   28)
#define bfwDMAC_PARAM_CH7_MAX_BLK_SIZE  ((uint32_t)    4)
/*****/

/****id* dmac.api/DW_CC_DEFINE_DMAC_PARAMS
 * ARGUMENTS
 *  prefix    -    prefix of peripheral (can be blaank/empty)
 * DESCRIPTION
 *  This macro is intended for use in initializing values for the
 *  dw_dmac_param structure (upon which it is dependent).  These
 *  values are obtained from DW_ahb_dmac_defs.h (upon which this
 *  macro is also dependent).
 * NOTES
 *  The relevant dmac coreConsultant C header must be included before
 *  this macro can be used.
 * SEE ALSO
 *  struct dw_dmac_param
 ***/
#define DW_CC_DEFINE_DMAC_PARAMS(prefix) {  \
    prefix ## CC_DMAH_ADD_ENCODED_PARAMS,   \
    prefix ## CC_DMAH_NUM_MASTER_INT,       \
    prefix ## CC_DMAH_NUM_CHANNELS,         \
    prefix ## CC_DMAH_NUM_HS_INT,           \
    prefix ## CC_DMAH_INTR_IO,              \
    prefix ## CC_DMAH_MABRST,               \
    prefix ## CC_DMAH_BIG_ENDIAN,           \
    prefix ## CC_DMAH_S_HDATA_WIDTH,        \
    prefix ## CC_DMAH_M1_HDATA_WIDTH,       \
    prefix ## CC_DMAH_M2_HDATA_WIDTH,       \
    prefix ## CC_DMAH_M3_HDATA_WIDTH,       \
    prefix ## CC_DMAH_M4_HDATA_WIDTH,       \
    prefix ## CC_DMAH_CH0_FIFO_DEPTH,       \
    prefix ## CC_DMAH_CH1_FIFO_DEPTH,       \
    prefix ## CC_DMAH_CH2_FIFO_DEPTH,       \
    prefix ## CC_DMAH_CH3_FIFO_DEPTH,       \
    prefix ## CC_DMAH_CH4_FIFO_DEPTH,       \
    prefix ## CC_DMAH_CH5_FIFO_DEPTH,       \
    prefix ## CC_DMAH_CH6_FIFO_DEPTH,       \
    prefix ## CC_DMAH_CH7_FIFO_DEPTH,       \
    prefix ## CC_DMAH_CH0_MAX_MULT_SIZE,    \
    prefix ## CC_DMAH_CH1_MAX_MULT_SIZE,    \
    prefix ## CC_DMAH_CH2_MAX_MULT_SIZE,    \
    prefix ## CC_DMAH_CH3_MAX_MULT_SIZE,    \
    prefix ## CC_DMAH_CH4_MAX_MULT_SIZE,    \
    prefix ## CC_DMAH_CH5_MAX_MULT_SIZE,    \
    prefix ## CC_DMAH_CH6_MAX_MULT_SIZE,    \
    prefix ## CC_DMAH_CH7_MAX_MULT_SIZE,    \
    prefix ## CC_DMAH_CH0_MAX_BLK_SIZE,     \
    prefix ## CC_DMAH_CH1_MAX_BLK_SIZE,     \
    prefix ## CC_DMAH_CH2_MAX_BLK_SIZE,     \
    prefix ## CC_DMAH_CH3_MAX_BLK_SIZE,     \
    prefix ## CC_DMAH_CH4_MAX_BLK_SIZE,     \
    prefix ## CC_DMAH_CH5_MAX_BLK_SIZE,     \
    prefix ## CC_DMAH_CH6_MAX_BLK_SIZE,     \
    prefix ## CC_DMAH_CH7_MAX_BLK_SIZE,     \
    prefix ## CC_DMAH_CH0_FC,               \
    prefix ## CC_DMAH_CH1_FC,               \
    prefix ## CC_DMAH_CH2_FC,               \
    prefix ## CC_DMAH_CH3_FC,               \
    prefix ## CC_DMAH_CH4_FC,               \
    prefix ## CC_DMAH_CH5_FC,               \
    prefix ## CC_DMAH_CH6_FC,               \
    prefix ## CC_DMAH_CH7_FC,               \
    prefix ## CC_DMAH_CH0_LOCK_EN,          \
    prefix ## CC_DMAH_CH1_LOCK_EN,          \
    prefix ## CC_DMAH_CH2_LOCK_EN,          \
    prefix ## CC_DMAH_CH3_LOCK_EN,          \
    prefix ## CC_DMAH_CH4_LOCK_EN,          \
    prefix ## CC_DMAH_CH5_LOCK_EN,          \
    prefix ## CC_DMAH_CH6_LOCK_EN,          \
    prefix ## CC_DMAH_CH7_LOCK_EN,          \
    prefix ## CC_DMAH_CH0_STW,              \
    prefix ## CC_DMAH_CH1_STW,              \
    prefix ## CC_DMAH_CH2_STW,              \
    prefix ## CC_DMAH_CH3_STW,              \
    prefix ## CC_DMAH_CH4_STW,              \
    prefix ## CC_DMAH_CH5_STW,              \
    prefix ## CC_DMAH_CH6_STW,              \
    prefix ## CC_DMAH_CH7_STW,              \
    prefix ## CC_DMAH_CH0_DTW,              \
    prefix ## CC_DMAH_CH1_DTW,              \
    prefix ## CC_DMAH_CH2_DTW,              \
    prefix ## CC_DMAH_CH3_DTW,              \
    prefix ## CC_DMAH_CH4_DTW,              \
    prefix ## CC_DMAH_CH5_DTW,              \
    prefix ## CC_DMAH_CH6_DTW,              \
    prefix ## CC_DMAH_CH7_DTW,              \
    prefix ## CC_DMAH_CH0_MULTI_BLK_EN,     \
    prefix ## CC_DMAH_CH1_MULTI_BLK_EN,     \
    prefix ## CC_DMAH_CH2_MULTI_BLK_EN,     \
    prefix ## CC_DMAH_CH3_MULTI_BLK_EN,     \
    prefix ## CC_DMAH_CH4_MULTI_BLK_EN,     \
    prefix ## CC_DMAH_CH5_MULTI_BLK_EN,     \
    prefix ## CC_DMAH_CH6_MULTI_BLK_EN,     \
    prefix ## CC_DMAH_CH7_MULTI_BLK_EN,     \
    (enum dw_dmac_transfer_type) prefix ## CC_DMAH_CH0_MULTI_BLK_TYPE, \
    (enum dw_dmac_transfer_type) prefix ## CC_DMAH_CH1_MULTI_BLK_TYPE, \
    (enum dw_dmac_transfer_type) prefix ## CC_DMAH_CH2_MULTI_BLK_TYPE, \
    (enum dw_dmac_transfer_type) prefix ## CC_DMAH_CH3_MULTI_BLK_TYPE, \
    (enum dw_dmac_transfer_type) prefix ## CC_DMAH_CH4_MULTI_BLK_TYPE, \
    (enum dw_dmac_transfer_type) prefix ## CC_DMAH_CH5_MULTI_BLK_TYPE, \
    (enum dw_dmac_transfer_type) prefix ## CC_DMAH_CH6_MULTI_BLK_TYPE, \
    (enum dw_dmac_transfer_type) prefix ## CC_DMAH_CH7_MULTI_BLK_TYPE, \
    prefix ## CC_DMAH_CH0_CTL_WB_EN,        \
    prefix ## CC_DMAH_CH1_CTL_WB_EN,        \
    prefix ## CC_DMAH_CH2_CTL_WB_EN,        \
    prefix ## CC_DMAH_CH3_CTL_WB_EN,        \
    prefix ## CC_DMAH_CH4_CTL_WB_EN,        \
    prefix ## CC_DMAH_CH5_CTL_WB_EN,        \
    prefix ## CC_DMAH_CH6_CTL_WB_EN,        \
    prefix ## CC_DMAH_CH7_CTL_WB_EN,        \
    prefix ## CC_DMAH_CH0_HC_LLP,           \
    prefix ## CC_DMAH_CH1_HC_LLP,           \
    prefix ## CC_DMAH_CH2_HC_LLP,           \
    prefix ## CC_DMAH_CH3_HC_LLP,           \
    prefix ## CC_DMAH_CH4_HC_LLP,           \
    prefix ## CC_DMAH_CH5_HC_LLP,           \
    prefix ## CC_DMAH_CH6_HC_LLP,           \
    prefix ## CC_DMAH_CH7_HC_LLP,           \
    prefix ## CC_DMAH_CH0_DST_SCA_EN,       \
    prefix ## CC_DMAH_CH1_DST_SCA_EN,       \
    prefix ## CC_DMAH_CH2_DST_SCA_EN,       \
    prefix ## CC_DMAH_CH3_DST_SCA_EN,       \
    prefix ## CC_DMAH_CH4_DST_SCA_EN,       \
    prefix ## CC_DMAH_CH5_DST_SCA_EN,       \
    prefix ## CC_DMAH_CH6_DST_SCA_EN,       \
    prefix ## CC_DMAH_CH7_DST_SCA_EN,       \
    prefix ## CC_DMAH_CH0_SRC_GAT_EN,       \
    prefix ## CC_DMAH_CH1_SRC_GAT_EN,       \
    prefix ## CC_DMAH_CH2_SRC_GAT_EN,       \
    prefix ## CC_DMAH_CH3_SRC_GAT_EN,       \
    prefix ## CC_DMAH_CH4_SRC_GAT_EN,       \
    prefix ## CC_DMAH_CH5_SRC_GAT_EN,       \
    prefix ## CC_DMAH_CH6_SRC_GAT_EN,       \
    prefix ## CC_DMAH_CH7_SRC_GAT_EN,       \
    prefix ## CC_DMAH_CH0_SMS,              \
    prefix ## CC_DMAH_CH1_SMS,              \
    prefix ## CC_DMAH_CH2_SMS,              \
    prefix ## CC_DMAH_CH3_SMS,              \
    prefix ## CC_DMAH_CH4_SMS,              \
    prefix ## CC_DMAH_CH5_SMS,              \
    prefix ## CC_DMAH_CH6_SMS,              \
    prefix ## CC_DMAH_CH7_SMS,              \
    prefix ## CC_DMAH_CH0_DMS,              \
    prefix ## CC_DMAH_CH1_DMS,              \
    prefix ## CC_DMAH_CH2_DMS,              \
    prefix ## CC_DMAH_CH3_DMS,              \
    prefix ## CC_DMAH_CH4_DMS,              \
    prefix ## CC_DMAH_CH5_DMS,              \
    prefix ## CC_DMAH_CH6_DMS,              \
    prefix ## CC_DMAH_CH7_DMS,              \
    prefix ## CC_DMAH_CH0_LMS,              \
    prefix ## CC_DMAH_CH1_LMS,              \
    prefix ## CC_DMAH_CH2_LMS,              \
    prefix ## CC_DMAH_CH3_LMS,              \
    prefix ## CC_DMAH_CH4_LMS,              \
    prefix ## CC_DMAH_CH5_LMS,              \
    prefix ## CC_DMAH_CH6_LMS,              \
    prefix ## CC_DMAH_CH7_LMS,              \
    prefix ## CC_DMAH_CH0_STAT_DST,         \
    prefix ## CC_DMAH_CH1_STAT_DST,         \
    prefix ## CC_DMAH_CH2_STAT_DST,         \
    prefix ## CC_DMAH_CH3_STAT_DST,         \
    prefix ## CC_DMAH_CH4_STAT_DST,         \
    prefix ## CC_DMAH_CH5_STAT_DST,         \
    prefix ## CC_DMAH_CH6_STAT_DST,         \
    prefix ## CC_DMAH_CH7_STAT_DST,         \
    prefix ## CC_DMAH_CH0_STAT_SRC,         \
    prefix ## CC_DMAH_CH1_STAT_SRC,         \
    prefix ## CC_DMAH_CH2_STAT_SRC,         \
    prefix ## CC_DMAH_CH3_STAT_SRC,         \
    prefix ## CC_DMAH_CH4_STAT_SRC,         \
    prefix ## CC_DMAH_CH5_STAT_SRC,         \
    prefix ## CC_DMAH_CH6_STAT_SRC,         \
    prefix ## CC_DMAH_CH7_STAT_SRC          \
}
/***/

/****id dmac.api/io_access
 * DESCRIPTION
 *  These are the macros used for accessing the DW_ahb_dmac memory map.
 *  All read and write DMA memory map accesses are 32-bits wide
 * SOURCE
 */
#define DMAC_INP    DW_IN32_32P
#define DMAC_OUTP   DW_OUT32_32P
/*****/

/****is* dmac.api/dw_dmac_param
 * DESCRIPTION
 *  This structure comprises the dmac hardware parameters that affect
 *  the software driver.  This structure needs to be initialized with
 *  the correct values and be pointed to by the (struct dw_device).cc
 *  member of the relevant dmac device structure.
 * SOURCE
 */
struct dw_dmac_param {
  bool      encoded_params;   /* include encoded hardware parameters */
  uint8_t   num_master_int;   /* number of AHB master interfaces     */
  uint8_t   num_channels;     /* number of DMA channels              */
  uint8_t   num_hs_int;       /* number of handshaking interfaces    */
  uint8_t   intr_io;          /* individual or combined interrupts   */
  bool      mabrst;           /* Max AMBA burst length               */
  bool      big_endian;       /* big or little endian 1=big          */
  uint16_t  s_hdata_width;    /* AHB slave data bus width            */
  uint16_t  m_hdata_width[4]; /* AHB master data bus widths          */
                              /*  can be - 32,64,128,256             */
  uint8_t   ch_fifo_depth[8]; /* channels FIFO depths in bytes       */
                              /*  can be - 8,16,32,64,128            */
  uint16_t  ch_max_mult_size[8]; /* channel max burst transaction    */
                                 /*  sizes can be - 4,8,16,32,64,    */
                                 /*  128,256                         */
  uint16_t  ch_max_blk_size[8];  /* channel max block sizes          */
                                 /*  can be - 3,7,15,31,63,127,255,  */
                                 /*  511, 1023,2047,4095             */
  uint8_t   ch_fc[8];        /* channels flow control hard-codded?   */
                             /*  0=DMA_only, 1=SRC_only, 2=DST_only  */
                             /*  3=ANY                               */
  bool      ch_lock_en[8];   /* bus locking for each channel         */
  uint16_t  ch_stw[8];       /* hard-code src channel transfer width */
                             /*  0=NO-HARD_CODE, 8=BYTE, 16=HALFWORD */
                             /*  32=WORD, 64=TWO_WORD, 128=FOUR_WORD */ 
                             /*  256=EIGHT_WORD                      */
  uint16_t  ch_dtw[8];       /* hard-code dst channel transfer width */
                             /*  uses same encoding as the src above */
  bool      ch_multi_blk_en[8];  /* Enable multi block transfers     */
  uint16_t  ch_multi_blk_type[8];  /* Multi block transfers types    */
  bool      ch_ctl_wb_en[8]; /* Transfer write back enable           */
  bool      ch_hc_llp[8];    /* hard-code LLP register               */
                             /*  1=hard-coded, 0=not hard-coded      */
  bool      ch_dst_sca_en[8];/* enable scatter feature on dst        */
  bool      ch_src_gat_en[8];/* enable gather feature on src         */
  uint8_t   ch_sms[8];       /* hard-code master interface on src    */
                             /*  0=master_1, 1=master_2, 2=master_3, */  
                             /*  3=master_4, 4=no_hard-code          */
  uint8_t   ch_dms[8];       /* hard-code master interface on dst    */
                             /*  same encoding as for src above      */
  uint8_t   ch_lms[8];       /* hard-code master interface on LLP    */
                             /*  same encoding as for src above      */
  bool      ch_stat_dst[8];  /* fetch status from dst peripheral     */
                             /*  1=enable feature, 0=disable feature */
  bool      ch_stat_src[8];  /* fetch status from src peripheral     */
                             /*  1=enable feature, 0=disable feature */
};
/*****/

/****is* dmac.api/dw_dmac_portmap
 * DESCRIPTION
 *  This is the structure used for accessing the dmac register
 *  portmap.
 * EXAMPLE
 *  struct dw_dmac_portmap *portmap;
 *  portmap = (struct dw_dmac_portmap *) DW_AHB_DMAC_BASE;
 *  foo = INP(portmap->sar_l);
 * SOURCE
 */
struct dw_dmac_portmap {

  /* Channel registers                                    */
  /* The offset address for each of the channel registers */
  /*  is shown for channel 0. For other channel numbers   */
  /*  use the following equation.                         */
  /*                                                      */
  /*    offset = (channel_num * 0x058) + channel_0 offset */
  /*                                                      */
  struct {
      volatile uint32_t sar_l;     /* Source Address Reg      (0x000) */
      volatile uint32_t sar_h;
      volatile uint32_t dar_l;     /* Destination Address Reg (0x008) */
      volatile uint32_t dar_h;
      volatile uint32_t llp_l;     /* Linked List Pointer Reg (0x010) */
      volatile uint32_t llp_h;  
      volatile uint32_t ctl_l;     /* Control Reg             (0x018) */
      volatile uint32_t ctl_h;
      volatile uint32_t sstat_l;   /* Source Status Reg       (0x020) */
      volatile uint32_t sstat_h;
      volatile uint32_t dstat_l;   /* Destination Status Reg  (0x028) */
      volatile uint32_t dstat_h;   
      volatile uint32_t sstatar_l; /* Source Status Addr Reg  (0x030) */
      volatile uint32_t sstatar_h; 
      volatile uint32_t dstatar_l; /* Dest Status Addr Reg    (0x038) */
      volatile uint32_t dstatar_h; 
      volatile uint32_t cfg_l;     /* Configuration Reg       (0x040) */
      volatile uint32_t cfg_h;
      volatile uint32_t sgr_l;     /* Source Gather Reg       (0x048) */
      volatile uint32_t sgr_h; 
      volatile uint32_t dsr_l;     /* Destination Scatter Reg (0x050) */
      volatile uint32_t dsr_h;
  } ch[8];

  /* Interrupt Raw Status Registers */
  volatile uint32_t raw_tfr_l;     /* Raw Status for IntTfr   (0x2c0) */
  volatile uint32_t raw_tfr_h;   
  volatile uint32_t raw_block_l;   /* Raw Status for IntBlock (0x2c8) */
  volatile uint32_t raw_block_h;
  volatile uint32_t raw_srctran_l; /* Raw Status IntSrcTran   (0x2d0) */
  volatile uint32_t raw_srctran_h; 
  volatile uint32_t raw_dsttran_l; /* Raw Status IntDstTran   (0x2d8) */
  volatile uint32_t raw_dsttran_h;
  volatile uint32_t raw_err_l;     /* Raw Status for IntErr   (0x2e0) */
  volatile uint32_t raw_err_h;

  /* Interrupt Status Registers */
  volatile uint32_t status_tfr_l;    /* Status for IntTfr     (0x2e8) */
  volatile uint32_t status_tfr_h;
  volatile uint32_t status_block_l;  /* Status for IntBlock   (0x2f0) */
  volatile uint32_t status_block_h;
  volatile uint32_t status_srctran_l;/* Status for IntSrcTran (0x2f8) */
  volatile uint32_t status_srctran_h;
  volatile uint32_t status_dsttran_l;/* Status for IntDstTran (0x300) */
  volatile uint32_t status_dsttran_h;
  volatile uint32_t status_err_l;    /* Status for IntErr     (0x308) */
  volatile uint32_t status_err_h;

  /* Interrupt Mask Registers */
  volatile uint32_t mask_tfr_l;      /* Mask for IntTfr       (0x310) */
  volatile uint32_t mask_tfr_h;
  volatile uint32_t mask_block_l;    /* Mask for IntBlock     (0x318) */
  volatile uint32_t mask_block_h;
  volatile uint32_t mask_srctran_l;  /* Mask for IntSrcTran   (0x320) */
  volatile uint32_t mask_srctran_h;
  volatile uint32_t mask_dsttran_l;  /* Mask for IntDstTran   (0x328) */
  volatile uint32_t mask_dsttran_h;
  volatile uint32_t mask_err_l;      /* Mask for IntErr       (0x330) */
  volatile uint32_t mask_err_h;

  /* Interrupt Clear Registers */
  volatile uint32_t clear_tfr_l;     /* Clear for IntTfr      (0x338) */
  volatile uint32_t clear_tfr_h;
  volatile uint32_t clear_block_l;   /* Clear for IntBlock    (0x340) */
  volatile uint32_t clear_block_h;
  volatile uint32_t clear_srctran_l; /* Clear for IntSrcTran  (0x348) */
  volatile uint32_t clear_srctran_h;
  volatile uint32_t clear_dsttran_l; /* Clear for IntDstTran  (0x350) */
  volatile uint32_t clear_dsttran_h;
  volatile uint32_t clear_err_l;     /* Clear for IntErr      (0x358) */
  volatile uint32_t clear_err_h;
  volatile uint32_t status_int_l;    /* Combined Intr Status  (0x360) */
  volatile uint32_t status_int_h;

  /* Software Handshaking Registers */
  volatile uint32_t req_src_reg_l; /* Src Sw Transaction Req  (0x368) */
  volatile uint32_t req_src_reg_h;   
  volatile uint32_t req_dst_reg_l; /* Dest Sw Transaction Req (0x370) */
  volatile uint32_t req_dst_reg_h;
  volatile uint32_t sgl_rq_src_reg_l; /* Sgl Src Transac Req  (0x378) */
  volatile uint32_t sgl_rq_src_reg_h;
  volatile uint32_t sgl_rq_dst_reg_l; /* Sgl Dest Transac Req (0x380) */
  volatile uint32_t sgl_rq_dst_reg_h;
  volatile uint32_t lst_src_reg_l;   /* Last Src Transac Req  (0x388) */
  volatile uint32_t lst_src_reg_h;
  volatile uint32_t lst_dst_reg_l;   /* Last Dest Transac Req (0x390) */
  volatile uint32_t lst_dst_reg_h;

  /* Misc Registers */
  volatile uint32_t dma_cfg_reg_l; /* Configuration Register  (0x398) */
  volatile uint32_t dma_cfg_reg_h;
  volatile uint32_t ch_en_reg_l;   /* Channel Enable Register (0x3a0) */
  volatile uint32_t ch_en_reg_h;
  volatile uint32_t dma_id_reg_l;    /* ID Register           (0x3a8) */
  volatile uint32_t dma_id_reg_h;
  volatile uint32_t dma_test_reg_l;  /* Test Register         (0x3b0) */
  volatile uint32_t dma_test_reg_h;
  volatile uint32_t old_version_id_l;/* legacy support        (0x3b8) */
  volatile uint32_t old_version_id_h;
  volatile uint32_t reserved_low;    /* reserved              (0x3c0) */
  volatile uint32_t reserved_high;
  volatile uint32_t dma_comp_params_6_l;/* hardware params    (0x3c8) */
  volatile uint32_t dma_comp_params_6_h;
  volatile uint32_t dma_comp_params_5_l;/* hardware params    (0x3d0) */
  volatile uint32_t dma_comp_params_5_h;
  volatile uint32_t dma_comp_params_4_l;/* hardware params    (0x3d8) */
  volatile uint32_t dma_comp_params_4_h;
  volatile uint32_t dma_comp_params_3_l;/* hardware params    (0x3e0) */
  volatile uint32_t dma_comp_params_3_h;
  volatile uint32_t dma_comp_params_2_l;/* hardware params    (0x3e8) */
  volatile uint32_t dma_comp_params_2_h;
  volatile uint32_t dma_comp_params_1_l;/* hardware params    (0x3f0) */
  volatile uint32_t dma_comp_params_1_h;
  volatile uint32_t dma_version_id_l;/* Version ID Register   (0x3f8) */
  volatile uint32_t dma_version_id_h;
};
/*****/

/****id* dmac.api/dw_dmac_ch_state
 * DESCRIPTION
 *  This data type is used to record the state of the DMA channels
 *  source and destination in releation to the transfer.
 *  Idle indicates that no software handshaking transfer is under
 *  way. single_region indicates that the src/dst is in single
 *  transaction region, and will fetch/deliever only one amba
 *  word. burst_region indicates indicates that the src/dst is not
 *  in single transaction region and perform a burst transfer.
 * SEE ALSO
 *  dw_dmac_resetInstance(), dw_dmac_irqHandler(),
 *  dw_dmac_sourceReady(), dw_dmac_destinationReady(),
 *  dw_dmac_setSingleRegion(), dw_dmac_startTransfer()
 * SOURCE
 */
enum dw_dmac_ch_state {
    Dmac_idle          = 0x0,
    Dmac_single_region = 0x1,
    Dmac_burst_region  = 0x2
};
/*****/

/****is* dmac.api/dw_dmac_instance
 * DESCRIPTION
 *  This structure is used to pass transfer information into
 *  the driver, so that it can be accessed and manipulated by
 *  the dw_dmac_irqHandler function.
 * SOURCE
 */
struct dw_dmac_instance {
    struct {
        enum dw_dmac_channel_number ch_num; // channel number
        enum dw_dmac_ch_state src_state;    // dmac source state
        enum dw_dmac_ch_state dst_state;    // dmac destination state
        int block_cnt;      // count of completed blocks
        int total_blocks;   // total number blocks in the transfer
        int src_byte_cnt;   // count of source bytes completed
        int dst_byte_cnt;   // count of destination bytes completed
        int src_single_inc; // src byte increment in single region
        int src_burst_inc;  // src byte increment in burst region
        int dst_single_inc; // dst byte increment in single region
        int dst_burst_inc;  // dst byte increment in burst region
        enum dw_dmac_transfer_type trans_type;  // transfer type (row)
        enum dw_dmac_transfer_flow tt_fc;   // transfer device type flow
                                            // control
        dw_callback userCallback; // callback functon for IRQ handler
        dw_callback userListener; // listener functon for IRQ handler
    } ch[8];
    int ch_order[DMAC_MAX_CHANNELS]; // channel order based on priority
};
/*****/

/****if* dmac.api/dw_dmac_resetInstance
 * DESCRIPTION
 *  This functions resets the dw_dmac_instance structure
 * ARGUMENTS
 *  dw_device     - dmac device handle
 * SEE ALSO
 *  dw_dmac_instance
 * SOURCE
 */
void dw_dmac_resetInstance(struct dw_device *dev);
/*****/

/****if* dmac.api/dw_dmac_autoCompParams
 * DESCRIPTION
 *  This function attempts to automatically discover the hardware
 *  component parameters, if this supported by the i2c in question.
 *  This is usually controlled by the ADD_ENCODED_PARAMS coreConsultant
 *  parameter.
 * ARGUMENTS
 *  dw_device     - dmac device handle
 * RETURN VALUE
 *  0           if successful
 *  -ENOSYS     function not supported
 * USES
 *  Accesses the following DW_apb_i2c register/bitfield(s):
 *   - 
 * NOTES
 *  This function does not allocate any memory.  An instance of
 *  dw_i2c_param must already be allocated and properly referenced from
 *  the relevant comp_param dw_device structure member.
 * SEE ALSO
 *  dw_dmac_init()
 * SOURCE
 */
int dw_dmac_autoCompParams(struct dw_device *dev);
/*****/

/****if* dmac.api/dw_dmac_checkChannelBusy
 * DESCRIPTION
 *  This function checks if the specified DMA channel is Busy (enabled)
 *  or not. Also checks if the specified channel is in range.
 * ARGUMENTS
 *  dw_device     - dmac device handle
 *  dw_dmac_channel_number - enumerated channel number
 * RETURN VALUE
 *  0             - if channel is in range and not busy
 *  -EBUSY        - if channel is in range but busy
 *  -ECHRNG       - if channel is out of range
 * NOTES
 *  This is a private function used by the device driver. This function
 *  should not be called by the user.
 * SOURCE
 */
int dw_dmac_checkChannelBusy(struct dw_device *dev,
        enum   dw_dmac_channel_number ch_num);
/*****/

/****if* dmac.api/dw_dmac_checkChannelRange
 * DESCRIPTION
 *  This function checks if the specified DMA channel is in range.
 * ARGUMENTS
 *  dw_device              - dmac device handle
 *  dw_dmac_channel_number - enumerated channel number
 * RETURN VALUE
 *  0             - if channel is in range
 *  -ECHRNG       - if channel is out of range
 * NOTES
 *  This is a private function used by the device driver. This function
 *  should not be called by the user.
 * SOURCE
 */
int dw_dmac_checkChannelRange(struct dw_device *dev,
        enum   dw_dmac_channel_number ch_num);
/*****/

/****if* dmac.api/dw_dmac_setChannelPriorityOrder
 * DESCRIPTION
 *  This function places each channel number into an ordered array
 *  based on the priority level setting for each channel.
 * ARGUMENTS
 *  dw_device              - dmac device handle
 * NOTES
 *  This is a private function used by the device driver. This function
 *  should not be called by the user.
 *  The the dw_dmac_irqHandler() API function makes use of the ordered
 *  array created by this function to correctly prioritise incoming
 *  interrupts from the DMA controller
 * SOURCE
 */
void dw_dmac_setChannelPriorityOrder(struct dw_device *dev);
/*****/

void dw_dmac_open(struct dw_device *dev);
et_status_t dw_dmac_close(struct dw_device *dev);
et_status_t dw_dmac_configure(struct dw_device *dev);
void dw_dmac_waitForLastBlock(struct dw_device *dev, uint32_t block_num, uint32_t ch_num, uint8_t clr_src);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* DMAC_PRIVATE_H */


