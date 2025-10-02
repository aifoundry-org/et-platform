/* --------------------------------------------------------------------
** 
** Synopsys DesignWare DW_apb_i2c Software Driver Kit and
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

#ifndef DW_I2C_PRIVATE_H
#define DW_I2C_PRIVATE_H

#ifdef __cplusplus
extern "C" {    // allow C++ to use these headers
#endif

/****id* i2c.macros/I2C_COMMON_REQUIREMENTS
 * NAME
 *  Common API Requirements
 * DESCRIPTION
 *  These are the common preconditions which must be met for all driver
 *  functions.  Primarily, they check that a function has been passed
 *  a legitimate dw_device structure.
 * SOURCE
 */
#define I2C_COMMON_REQUIREMENTS(dev)            \
do {                                            \
    DW_REQUIRE(dev != NULL);                    \
    DW_REQUIRE(dev->instance != NULL);          \
    DW_REQUIRE(dev->comp_param != NULL);        \
    DW_REQUIRE(dev->base_address != NULL);      \
    DW_REQUIRE(dev->comp_type == Dw_apb_i2c);   \
    DW_REQUIRE((dev->data_width == 32)          \
            || (dev->data_width == 16)          \
            || (dev->data_width == 8));         \
} while(0)
/*****/

/****id* i2c.macros/bit_definitions
 * NAME
 *  bitfield width/shift definitions
 * DESCRIPTION
 *  Used in conjunction with bitops.h to access register bitfields.
 *  They are defined as bit offset/mask pairs for each i2c register
 *  bitfield.
 * EXAMPLE
 *  targetAddress = BIT_GET(in32(I2C_TAR), I2C_TAR_ADDR);
 * NOTES
 *  bfo is the offset of the bitfield with respect to LSB;
 *  bfw is the width of the bitfield
 * SEE ALSO
 *  dw_common_bitops.h
 ***/
// control register
#define bfoI2C_CON_MASTER_MODE          ((uint32_t) 0)
#define bfwI2C_CON_MASTER_MODE          ((uint32_t) 1)
#define bfoI2C_CON_SPEED                ((uint32_t) 1)
#define bfwI2C_CON_SPEED                ((uint32_t) 2)
#define bfoI2C_CON_10BITADDR_SLAVE      ((uint32_t) 3)
#define bfwI2C_CON_10BITADDR_SLAVE      ((uint32_t) 1)
#define bfoI2C_CON_10BITADDR_MASTER     ((uint32_t) 4)
#define bfwI2C_CON_10BITADDR_MASTER     ((uint32_t) 1)
#define bfoI2C_CON_RESTART_EN           ((uint32_t) 5)
#define bfwI2C_CON_RESTART_EN           ((uint32_t) 1)
#define bfoI2C_CON_SLAVE_DISABLE        ((uint32_t) 6)
#define bfwI2C_CON_SLAVE_DISABLE        ((uint32_t) 1)
// target address register
#define bfoI2C_TAR_ADDR                 ((uint32_t) 0)
#define bfwI2C_TAR_ADDR                 ((uint32_t) 10)
#define bfoI2C_TAR_GC_OR_START          ((uint32_t) 10)
#define bfwI2C_TAR_GC_OR_START          ((uint32_t) 1)
#define bfoI2C_TAR_SPECIAL              ((uint32_t) 11)
#define bfwI2C_TAR_SPECIAL              ((uint32_t) 1)
// slave address register
#define bfoI2C_SAR_ADDR                 ((uint32_t) 0)
#define bfwI2C_SAR_ADDR                 ((uint32_t) 10)
// high speed master mode code address register
#define bfoI2C_HS_MADDR_HS_MAR          ((uint32_t) 0)
#define bfwI2C_HS_MADDR_HS_MAR          ((uint32_t) 3)
// tx/rx data buffer and command register
#define bfoI2C_DATA_CMD_DAT             ((uint32_t) 0)
#define bfwI2C_DATA_CMD_DAT             ((uint32_t) 8)
#define bfoI2C_DATA_CMD_CMD             ((uint32_t) 8)
#define bfwI2C_DATA_CMD_CMD             ((uint32_t) 1)
// standard speed scl clock high count register
#define bfoI2C_SS_SCL_HCNT_COUNT        ((uint32_t) 0)
#define bfwI2C_SS_SCL_HCNT_COUNT        ((uint32_t) 16)
// standard speed scl clock low count register
#define bfoI2C_SS_SCL_LCNT_COUNT        ((uint32_t) 0)
#define bfwI2C_SS_SCL_LCNT_COUNT        ((uint32_t) 16)
// fast speed scl clock high count register
#define bfoI2C_FS_SCL_HCNT_COUNT        ((uint32_t) 0)
#define bfwI2C_FS_SCL_HCNT_COUNT        ((uint32_t) 16)
// fast speed scl clock low count register
#define bfoI2C_FS_SCL_LCNT_COUNT        ((uint32_t) 0)
#define bfwI2C_FS_SCL_LCNT_COUNT        ((uint32_t) 16)
// high speed scl clock high count register
#define bfoI2C_HS_SCL_HCNT_COUNT        ((uint32_t) 0)
#define bfwI2C_HS_SCL_HCNT_COUNT        ((uint32_t) 16)
// high speed scl clock low count register
#define bfoI2C_HS_SCL_LCNT_COUNT        ((uint32_t) 0)
#define bfwI2C_HS_SCL_LCNT_COUNT        ((uint32_t) 16)
// [raw] interrupt status & mask registers
#define bfoI2C_INTR_RX_UNDER            ((uint32_t) 0)
#define bfwI2C_INTR_RX_UNDER            ((uint32_t) 1)
#define bfoI2C_INTR_RX_OVER             ((uint32_t) 1)
#define bfwI2C_INTR_RX_OVER             ((uint32_t) 1)
#define bfoI2C_INTR_RX_FULL             ((uint32_t) 2)
#define bfwI2C_INTR_RX_FULL             ((uint32_t) 1)
#define bfoI2C_INTR_TX_OVER             ((uint32_t) 3)
#define bfwI2C_INTR_TX_OVER             ((uint32_t) 1)
#define bfoI2C_INTR_TX_EMPTY            ((uint32_t) 4)
#define bfwI2C_INTR_TX_EMPTY            ((uint32_t) 1)
#define bfoI2C_INTR_RD_REQ              ((uint32_t) 5)
#define bfwI2C_INTR_RD_REQ              ((uint32_t) 1)
#define bfoI2C_INTR_TX_ABRT             ((uint32_t) 6)
#define bfwI2C_INTR_TX_ABRT             ((uint32_t) 1)
#define bfoI2C_INTR_RX_DONE             ((uint32_t) 7)
#define bfwI2C_INTR_RX_DONE             ((uint32_t) 1)
#define bfoI2C_INTR_ACTIVITY            ((uint32_t) 8)
#define bfwI2C_INTR_ACTIVITY            ((uint32_t) 1)
#define bfoI2C_INTR_STOP_DET            ((uint32_t) 9)
#define bfwI2C_INTR_STOP_DET            ((uint32_t) 1)
#define bfoI2C_INTR_START_DET           ((uint32_t) 10)
#define bfwI2C_INTR_START_DET           ((uint32_t) 1)
#define bfoI2C_INTR_GEN_CALL            ((uint32_t) 11)
#define bfwI2C_INTR_GEN_CALL            ((uint32_t) 1)
// receive fifo threshold register
#define bfoI2C_RX_TL_RX_TL              ((uint32_t) 0)
#define bfwI2C_RX_TL_RX_TL              ((uint32_t) 8)
// transmit fifo threshold register
#define bfoI2C_TX_TL_TX_TL              ((uint32_t) 0)
#define bfwI2C_TX_TL_TX_TL              ((uint32_t) 8)
// clear combind and individual interrupts register
#define bfoI2C_CLR_INTR_CLR_INTR        ((uint32_t) 0)
#define bfwI2C_CLR_INTR_CLR_INTR        ((uint32_t) 1)
// clear rx_under interrupt register
#define bfoI2C_CLR_RX_UNDER_CLEAR       ((uint32_t) 0)
#define bfwI2C_CLR_RX_UNDER_CLEAR       ((uint32_t) 1)
// clear rx_over interrupt register
#define bfoI2C_CLR_RX_OVER_CLEAR        ((uint32_t) 0)
#define bfwI2C_CLR_RX_OVER_CLEAR        ((uint32_t) 1)
// clear tx_over interrupt register
#define bfoI2C_CLR_TX_OVER_CLEAR        ((uint32_t) 0)
#define bfwI2C_CLR_TX_OVER_CLEAR        ((uint32_t) 1)
// clear rd_req interrupt register
#define bfoI2C_CLR_RD_REQ_CLEAR         ((uint32_t) 0)
#define bfwI2C_CLR_RD_REQ_CLEAR         ((uint32_t) 1)
// clear rx_abrt interrupt register
#define bfoI2C_CLR_TX_ABRT_CLEAR        ((uint32_t) 0)
#define bfwI2C_CLR_TX_ABRT_CLEAR        ((uint32_t) 1)
// clear rx_done interrupt register
#define bfoI2C_CLR_RX_DONE_CLEAR        ((uint32_t) 0)
#define bfwI2C_CLR_RX_DONE_CLEAR        ((uint32_t) 1)
// clear activity interrupt register
#define bfoI2C_CLR_ACTIVITY_CLEAR       ((uint32_t) 0)
#define bfwI2C_CLR_ACTIVITY_CLEAR       ((uint32_t) 1)
// clear stop detection interrupt register
#define bfoI2C_CLR_STOP_DET_CLEAR       ((uint32_t) 0)
#define bfwI2C_CLR_STOP_DET_CLEAR       ((uint32_t) 1)
// clear start detection interrupt register
#define bfoI2C_CLR_START_DET_CLEAR      ((uint32_t) 0)
#define bfwI2C_CLR_START_DET_CLEAR      ((uint32_t) 1)
// clear general call interrupt register
#define bfoI2C_CLR_GEN_CALL_CLEAR       ((uint32_t) 0)
#define bfwI2C_CLR_GEN_CALL_CLEAR       ((uint32_t) 1)
// enable register
#define bfoI2C_ENABLE_ENABLE            ((uint32_t) 0)
#define bfwI2C_ENABLE_ENABLE            ((uint32_t) 1)
// status register
#define bfoI2C_STATUS_ACTIVITY          ((uint32_t) 0)
#define bfwI2C_STATUS_ACTIVITY          ((uint32_t) 1)
#define bfoI2C_STATUS_TFNF              ((uint32_t) 1)
#define bfwI2C_STATUS_TFNF              ((uint32_t) 1)
#define bfoI2C_STATUS_TFE               ((uint32_t) 2)
#define bfwI2C_STATUS_TFE               ((uint32_t) 1)
#define bfoI2C_STATUS_RFNE              ((uint32_t) 3)
#define bfwI2C_STATUS_RFNE              ((uint32_t) 1)
#define bfoI2C_STATUS_RFF               ((uint32_t) 4)
#define bfwI2C_STATUS_RFF               ((uint32_t) 1)
// transmit fifo level register
#define bfoI2C_TXFLR_TXFL               ((uint32_t) 0)
#define bfwI2C_TXFLR_TXFL               ((uint32_t) 9)
// receive fifo level register
#define bfoI2C_RXFLR_RXFL               ((uint32_t) 0)
#define bfwI2C_RXFLR_RXFL               ((uint32_t) 9)
// soft reset register
#define bfoI2C_SRESET_SRST              ((uint32_t) 0)
#define bfwI2C_SRESET_SRST              ((uint32_t) 1)
#define bfoI2C_SRESET_MASTER_SRST       ((uint32_t) 1)
#define bfwI2C_SRESET_MASTER_SRST       ((uint32_t) 1)
#define bfoI2C_SRESET_SLAVE_SRST        ((uint32_t) 2)
#define bfwI2C_SRESET_SLAVE_SRST        ((uint32_t) 1)
// transmit abort status register
#define bfoI2C_TX_ABRT_SRC_7B_ADDR_NOACK    ((uint32_t) 0)
#define bfwI2C_TX_ABRT_SRC_7B_ADDR_NOACK    ((uint32_t) 1)
#define bfoI2C_TX_ABRT_SRC_10ADDR1_NOACK    ((uint32_t) 1)
#define bfwI2C_TX_ABRT_SRC_10ADDR1_NOACK    ((uint32_t) 1)
#define bfoI2C_TX_ABRT_SRC_10ADDR2_NOACK    ((uint32_t) 2)
#define bfwI2C_TX_ABRT_SRC_10ADDR2_NOACK    ((uint32_t) 1)
#define bfoI2C_TX_ABRT_SRC_TXDATA_NOACK     ((uint32_t) 3)
#define bfwI2C_TX_ABRT_SRC_TXDATA_NOACK     ((uint32_t) 1)
#define bfoI2C_TX_ABRT_SRC_GCALL_NOACK      ((uint32_t) 4)
#define bfwI2C_TX_ABRT_SRC_GCALL_NOACK      ((uint32_t) 1)
#define bfoI2C_TX_ABRT_SRC_GCALL_READ       ((uint32_t) 5)
#define bfwI2C_TX_ABRT_SRC_GCALL_READ       ((uint32_t) 1)
#define bfoI2C_TX_ABRT_SRC_HS_ACKDET        ((uint32_t) 6)
#define bfwI2C_TX_ABRT_SRC_HS_ACKDET        ((uint32_t) 1)
#define bfoI2C_TX_ABRT_SRC_SBYTE_ACKDET     ((uint32_t) 7)
#define bfwI2C_TX_ABRT_SRC_SBYTE_ACKDET     ((uint32_t) 1)
#define bfoI2C_TX_ABRT_SRC_HS_NORSTRT       ((uint32_t) 8)
#define bfwI2C_TX_ABRT_SRC_HS_NORSTRT       ((uint32_t) 1)
#define bfoI2C_TX_ABRT_SRC_SBYTE_NORSTRT    ((uint32_t) 9)
#define bfwI2C_TX_ABRT_SRC_SBYTE_NORSTRT    ((uint32_t) 1)
#define bfoI2C_TX_ABRT_SRC_10B_RD_NORSTRT   ((uint32_t) 10)
#define bfwI2C_TX_ABRT_SRC_10B_RD_NORSTRT   ((uint32_t) 1)
#define bfoI2C_TX_ABRT_SRC_ARB_MASTER_DIS   ((uint32_t) 11)
#define bfwI2C_TX_ABRT_SRC_ARB_MASTER_DIS   ((uint32_t) 1)
#define bfoI2C_TX_ABRT_SRC_ARB_LOST         ((uint32_t) 12)
#define bfwI2C_TX_ABRT_SRC_ARB_LOST         ((uint32_t) 1)
#define bfoI2C_TX_ABRT_SRC_SLVFLUSH_TXFIFO  ((uint32_t) 13)
#define bfwI2C_TX_ABRT_SRC_SLVFLUSH_TXFIFO  ((uint32_t) 1)
#define bfoI2C_TX_ABRT_SRC_SLV_ARBLOST      ((uint32_t) 14)
#define bfwI2C_TX_ABRT_SRC_SLV_ARBLOST      ((uint32_t) 1)
#define bfoI2C_TX_ABRT_SRC_SLVRD_INTX       ((uint32_t) 15)
#define bfwI2C_TX_ABRT_SRC_SLVRD_INTX       ((uint32_t) 1)
// dma control register
#define bfoI2C_DMA_CR_RDMAE             ((uint32_t) 0)
#define bfwI2C_DMA_CR_RDMAE             ((uint32_t) 1)
#define bfoI2C_DMA_CR_TDMAE             ((uint32_t) 1)
#define bfwI2C_DMA_CR_TDMAE             ((uint32_t) 1)
// dma transmit data level register
#define bfoI2C_DMA_TDLR_DMATDL          ((uint32_t) 0)
#define bfwI2C_DMA_TDLR_DMATDL          ((uint32_t) 8)
// dma receive data level register
#define bfoI2C_DMA_RDLR_DMARDL          ((uint32_t) 0)
#define bfwI2C_DMA_RDLR_DMARDL          ((uint32_t) 8)
// i2c component parameters
#define bfoI2C_PARAM_DATA_WIDTH         ((uint32_t) 0)
#define bfwI2C_PARAM_DATA_WIDTH         ((uint32_t) 2)
#define bfoI2C_PARAM_MAX_SPEED_MODE     ((uint32_t) 2)
#define bfwI2C_PARAM_MAX_SPEED_MODE     ((uint32_t) 2)
#define bfoI2C_PARAM_HC_COUNT_VALUES    ((uint32_t) 4)
#define bfwI2C_PARAM_HC_COUNT_VALUES    ((uint32_t) 1)
#define bfoI2C_PARAM_INTR_IO            ((uint32_t) 5)
#define bfwI2C_PARAM_INTR_IO            ((uint32_t) 1)
#define bfoI2C_PARAM_HAS_DMA            ((uint32_t) 6)
#define bfwI2C_PARAM_HAS_DMA            ((uint32_t) 1)
#define bfoI2C_PARAM_ADD_ENCODED_PARAMS ((uint32_t) 7)
#define bfwI2C_PARAM_ADD_ENCODED_PARAMS ((uint32_t) 1)
#define bfoI2C_PARAM_RX_BUFFER_DEPTH    ((uint32_t) 8)
#define bfwI2C_PARAM_RX_BUFFER_DEPTH    ((uint32_t) 8)
#define bfoI2C_PARAM_TX_BUFFER_DEPTH    ((uint32_t) 16)
#define bfwI2C_PARAM_TX_BUFFER_DEPTH    ((uint32_t) 8)
////
// derived bitfield definitions
////
// the following bitfield is a concatenation of SPECIAL and GC_OR_START
#define bfoI2C_TAR_TX_MODE              ((uint32_t) 10)
#define bfwI2C_TAR_TX_MODE              ((uint32_t) 2)
// generic definition used for all scl clock count reads/modifications
#define bfoI2C_SCL_COUNT                ((uint32_t) 0)
#define bfwI2C_SCL_COUNT                ((uint32_t) 16)
// group bitfield of everything in TX_ABRT_SOURCE
#define bfoI2C_TX_ABRT_SRC_ALL          ((uint32_t) 0)
#define bfwI2C_TX_ABRT_SRC_ALL          ((uint32_t) 16)

/****id* i2c.macros/DW_CC_DEFINE_I2C_PARAMS
 * USAGE
 *  DW_CC_DEFINE_I2C_PARAMS(prefix);
 * ARGUMENTS
 *  prefix      prefix of peripheral (can be blank/empty)
 * DESCRIPTION
 *  This macro is intended for use in initializing values for the
 *  dw_i2c_param structure (upon which it is dependent).  These
 *  values are obtained from DW_apb_i2c_defs.h (upon which this
 *  macro is also dependent).
 * NOTES
 *  The relevant DW_apb_i2c coreKit C header must be included before
 *  this macro can be used.
 * EXAMPLE
 *  const struct dw_i2c_param i2c = DW_CC_DEFINE_I2C_PARAMS();
 *  const struct dw_i2c_param i2c_m = DW_CC_DEFINE_I2C_PARAMS(master_);
 * SEE ALSO
 *  struct dw_i2c_param
 * SOURCE
 */
#define DW_CC_DEFINE_I2C_PARAMS(x) DW_CC_DEFINE_I2C_PARAMS_1_03(x)

#define DW_CC_DEFINE_I2C_PARAMS_1_03(prefix) {              \
    prefix ## CC_IC_HC_COUNT_VALUES,                        \
    prefix ## CC_IC_HAS_DMA,                                \
    prefix ## CC_IC_RX_BUFFER_DEPTH,                        \
    prefix ## CC_IC_TX_BUFFER_DEPTH,                        \
    (enum dw_i2c_speed_mode) prefix ## CC_IC_MAX_SPEED_MODE \
}

#define DW_CC_DEFINE_I2C_PARAMS_1_01(prefix) {              \
    prefix ## CC_IC_HC_COUNT_VALUES,                        \
    false,                                                  \
    prefix ## CC_IC_RX_BUFFER_DEPTH,                        \
    prefix ## CC_IC_TX_BUFFER_DEPTH,                        \
    (enum dw_i2c_speed_mode) prefix ## CC_IC_MAX_SPEED_MODE \
}
/*****/

/****id* i2c.macros/APB_ACCESS
 * DESCRIPTION
 *  This macro is used to hardcode the APB data accesses, if the APB
 *  data width is the same for an entire system.  Simply defining
 *  APB_DATA_WIDTH at compile time will force all DW_apb_i2c memory map
 *  I/O accesses to be performed with the specifed data width.  By
 *  default, no I/O access is performed until the APB data width of a
 *  device is checked in the dw_device data structure.
 * SOURCE
 */
//#ifdef APB_DATA_WIDTH
//#if (APB_DATA_WIDTH == 32)
//#define I2C_INP     DW_IN32_32P
//#define I2C_OUTP    DW_OUT32_32P
//#elif (APB_DATA_WIDTH == 16)
//#define I2C_INP     DW_IN32_16P
//#define I2C_OUTP    DW_OUT32_16P
//#else
//#define I2C_INP     DW_IN32_8P
//#define I2C_OUTP    DW_OUT32_8P
//#endif      // (APB_DATA_WIDTH == 32)
//#else
#define I2C_INP     DW_INP
#define I2C_OUTP    DW_OUTP
//#endif      // APB_DATA_WIDTH
/*****/

/****id* i2c.macros/I2C_FIFO_READ
 * DESCRIPTION
 *  This macro reads up to MAX bytes from the I2C Rx FIFO.  However, to
 *  improve AMBA bus efficieny, it only writes to the user-specified
 *  buffer in memory once for every four bytes received.  It
 *  accomplishes this using an intermediate holding variable
 *  instance->rxHold.
 * SEE ALSO
 *  I2C_FIFO_WRITE, I2C_FIFO_WRITE16
 * SOURCE
 */
#define I2C_FIFO_READ(MAX)                                          \
do {                                                                \
    int k;                                                          \
    uint32_t *ptr;                                                  \
    ptr = (uint32_t *) ((uint64_t)instance->rxBuffer);                          \
    for(k = 0; k < (MAX); k++) {                                    \
        instance->rxHold >>= 8;                                     \
        instance->rxHold |= (unsigned)(DW_IN16P(portmap->data_cmd) << 24);    \
        if(--instance->rxIdx == 0) {                                \
            *(ptr++) = instance->rxHold;                            \
            instance->rxIdx = 4;                                    \
        }                                                           \
        if(--instance->rxRemain == 0)                               \
            break;                                                  \
    }                                                               \
    instance->rxBuffer = (uint8_t *) ptr;                           \
} while(0)
/*****/

/****id* i2c.macros/I2C_FIFO_WRITE
 * DESCRIPTION
 *  This macro writes up to MAX bytes to the I2C Tx FIFO.  However, to
 *  improve AMBA bus efficieny, it only reads from the user-specified
 *  buffer in memory once for every four bytes transmitted.  It
 *  accomplishes this using an intermediate holding variable
 *  instance->txHold.
 * SEE ALSO
 *  I2C_FIFO_READ, I2C_FIFO_WRITE16
 * SOURCE
 */
#define I2C_FIFO_WRITE(MAX)                                         \
do {                                                                \
    int k;                                                          \
    uint32_t *ptr;                                                  \
    ptr = (uint32_t *) ((uint64_t)instance->txBuffer);                          \
    for(k = 0; k < (MAX); k++) {                                    \
        if(instance->txIdx == 0) {                                  \
            instance->txHold = *(ptr++);                            \
            instance->txIdx = 4;                                    \
        }                                                           \
        DW_OUT32P((instance->txHold & 0xff), portmap->data_cmd);    \
        instance->txHold >>= 8;                                     \
        instance->txIdx--;                                          \
        if(--instance->txRemain == 0)                               \
            break;                                                  \
    }                                                               \
    instance->txBuffer = (uint8_t *) ptr;                           \
} while(0)
/*****/

/****id* i2c.macros/I2C_FIFO_WRITE16
 * DESCRIPTION
 *  This macro writes up to MAX words to the I2C Tx FIFO.  However, to
 *  improve AMBA bus efficieny, it only reads from the user-specified
 *  buffer in memory once for every two words transmitted.  It
 *  accomplishes this using an intermediate holding variable
 *  instance->txHold.
 * SEE ALSO
 *  I2C_FIFO_READ, I2C_FIFO_WRITE
 * SOURCE
 */
#define I2C_FIFO_WRITE16(MAX)                                          \
do {                                                                   \
    int i;                                                             \
    uint16_t val;                                                      \
    uint32_t *ptr;                                                     \
    ptr = (uint32_t *) instance->b2bBuffer;                            \
    for(i = 0; i < (MAX); i++) {                                       \
        switch(instance->txIdx) {                                      \
            case 0:                                                    \
                instance->txHold = *(ptr++);                           \
                instance->txIdx = 4;                                   \
            case 2:                                                    \
            case 4:                                                    \
                DW_OUT16P((instance->txHold & 0xffff),                 \
                        portmap->data_cmd);                            \
                instance->txHold >>= 16;                               \
                instance->txIdx -= 2;                                  \
                break;                                                 \
            case 1:                                                    \
                val = instance->txHold & 0xff;                         \
                instance->txHold = *(ptr++);                           \
                val |= ((instance->txHold & 0xff) << 8);               \
                DW_OUT16P(val, portmap->data_cmd);                     \
                instance->txHold >>= 8;                                \
                instance->txIdx = 3;                                   \
                break;                                                 \
            case 3:                                                    \
                DW_OUT16P((instance->txHold & 0xffff),                 \
                        portmap->data_cmd);                            \
                instance->txHold >>= 16;                               \
                instance->txIdx -= 2;                                  \
                break;                                                 \
        }                                                              \
    }                                                                  \
    instance->b2bBuffer = (uint16_t *) ptr;                            \
} while(0)
/*****/

/****id* i2c.macros/I2C_ENTER_CRITICAL_SECTION
 * DESCRIPTION
 *  This macro disables DW_apb_i2c interrupts, to avoid shared data
 *  issues when entering a critical section of the driver code.  A copy
 *  of the current intr_mask value is kept in the instance structure so
 *  that the interrupts can be later restored.
 * SEE ALSO
 *  I2C_EXIT_CRITICAL_SECTION, dw_i2c_instance
 * SOURCE
 */
#define I2C_ENTER_CRITICAL_SECTION()                \
do {                                                \
    /* Disable I2C interrupts */                    \
    I2C_OUTP(0x0, portmap->intr_mask);              \
} while (0)
/*****/

/****id* i2c.macros/I2C_EXIT_CRITICAL_SECTION
 * DESCRIPTION
 *  This macro restores DW_apb_i2x interrupts, after a critical code
 *  section.  It uses the saved intr_mask value in the instance
 *  structure to accomplish this.
 * SEE ALSO
 *  I2C_ENTER_CRITICAL_SECTION, dw_i2c_instance
 * SOURCE
 */
#define I2C_EXIT_CRITICAL_SECTION()                         \
do {                                                        \
    /* Restore I2C interrupts */                            \
    I2C_OUTP(instance->intr_mask_save, portmap->intr_mask); \
} while (0)
/*****/

/****id* i2c.data/dw_i2c_state
 * NAME
 *  dw_i2c_state
 * DESCRIPTION
 *  This is the data type used for managing the i2c driver state.
 * SOURCE
 */
enum dw_i2c_state {
    // driver is idle
    I2c_state_idle = 0,
   // driver in back2back transfer mode
    I2c_state_back2back,
    // driver in master-transmitter mode
    I2c_state_master_tx,
    // driver in master-receiver mode
    I2c_state_master_rx,
    // driver in slave-transmitter mode
    I2c_state_slave_tx,
    // driver in slave-transmitter bulk transfer mode
    I2c_state_slave_bulk_tx,
    // driver in slave-receiver mode
    I2c_state_slave_rx,
    // waiting for user to set up a slave-tx transfer (read request)
    I2c_state_rd_req,
    // waiting for user to set up slave-rx transfer (data already in rx
    // fifo or general call received)
    I2c_state_rx_req,
    // master Tx mode is general call and slave on same device is
    // responding to the general call.  Waiting for user to set up a
    // slave-rx transfer.
    I2c_state_master_tx_gen_call,
    // master is transmitting a general call which the slave is
    // receiving
    I2c_state_master_tx_slave_rx,
    // slave-tx transfer is in progress and the Rx full interrupt is
    // triggered.  Waiting for user to set up a slave-rx transfer.
    I2c_state_slave_tx_rx_req,
    // slave-tx bulk transfer is in progress and the Rx full interrupt
    // is triggered.  Waiting for user to set up a slave-rx transfer.
    I2c_state_slave_bulk_tx_rx_req,
    // slave-rx transfer is in progress and the read request interrupt
    // is triggered.  Waiting for user to set up a slave-tx transfer.
    I2c_state_slave_rx_rd_req,
    // slave is both servicing read requests and is also receiving data
    // (e.g. when it is the target of a back-to-back transfer)
    I2c_state_slave_tx_rx,
    // slave is both servicing read requests (bulk transfer mode) and is
    // also receiving data (e.g. when it is the target of a back-to-back
    // transfer)
    I2c_state_slave_bulk_tx_rx,
    // A Tx abort or fifo over/underflow error has occurred.  Waiting
    // for user to call dw_i2c_terminate().
    I2c_state_error
};
/*****/

/****is* i2c.api/dw_i2c_param
 * NAME
 *  dw_i2c_param -- i2c hardware configuration parameters
 * DESCRIPTION
 *  This structure comprises the i2c hardware parameters that affect
 *  the software driver.  This structure needs to be initialized with
 *  the correct values and be pointed to by the (struct
 *  dw_device).comp_param member of the relevant i2c device structure.
 * SOURCE
 */
struct dw_i2c_param {
    bool hc_count_values;       // hardcoded scl count values?
    bool has_dma;               // i2c has a dma interface?
    uint16_t rx_buffer_depth;   // Rx fifo depth
    uint16_t tx_buffer_depth;   // Tx fifo depth
    enum dw_i2c_speed_mode max_speed_mode;  // standard, fast or high
};
/*****/

/****is* i2c.api/dw_i2c_portmap
 * NAME
 *  dw_i2c_portmap
 * DESCRIPTION
 *  This is the structure used for accessing the i2c register
 *  portmap.
 * EXAMPLE
 *  struct dw_i2c_portmap *portmap;
 *  portmap = (struct dw_i2c_portmap *) DW_APB_I2C_BASE;
 *  foo = in32p(portmap->TX_ABRT_SOURCE);
 * SOURCE
 */
struct dw_i2c_portmap {
    volatile uint32_t con;           // control register          (0x00)
    volatile uint32_t tar;           // target address            (0x04)
    volatile uint32_t sar;           // slave address             (0x08)
    volatile uint32_t hs_maddr;      // high speed master code    (0x0c)
    volatile uint32_t data_cmd;      // tx/rx data/command buffer (0x10)
    volatile uint32_t ss_scl_hcnt;   // standard SCL high count   (0x14)
    volatile uint32_t ss_scl_lcnt;   // standard SCL low count    (0x18)
    volatile uint32_t fs_scl_hcnt;   // full speed SCL high count (0x1c)
    volatile uint32_t fs_scl_lcnt;   // full speed SCL low count  (0x20)
    volatile uint32_t hs_scl_hcnt;   // high speed SCL high count (0x24)
    volatile uint32_t hs_scl_lcnt;   // high speed SCL low count  (0x28)
    volatile uint32_t intr_stat;     // irq status                (0x2c)
    volatile uint32_t intr_mask;     // irq mask                  (0x30)
    volatile uint32_t raw_intr_stat; // raw irq status            (0x34)
    volatile uint32_t rx_tl;         // Rx fifo threshold         (0x38)
    volatile uint32_t tx_tl;         // Tx fifo threshold         (0x3c)
    volatile uint32_t clr_intr;      // clear all interrupts      (0x40)
    volatile uint32_t clr_rx_under;  // clear RX_UNDER irq        (0x44)
    volatile uint32_t clr_rx_over;   // clear RX_OVER irq         (0x48)
    volatile uint32_t clr_tx_over;   // clear TX_OVER irq         (0x4c)
    volatile uint32_t clr_rd_req;    // clear RD_REQ irq          (0x50)
    volatile uint32_t clr_tx_abrt;   // clear TX_ABRT irq         (0x54)
    volatile uint32_t clr_rx_done;   // clear RX_DONE irq         (0x58)
    volatile uint32_t clr_activity;  // clear ACTIVITY irq        (0x5c)
    volatile uint32_t clr_stop_det;  // clear STOP_DET irq        (0x60)
    volatile uint32_t clr_start_det; // clear START_DET irq       (0x64)
    volatile uint32_t clr_gen_call;  // clear GEN_CALL irq        (0x68)
    volatile uint32_t enable;        // i2c enable register       (0x6c)
    volatile uint32_t status;        // i2c status register       (0x70)
    volatile uint32_t txflr;         // Tx fifo level register    (0x74)
    volatile uint32_t rxflr;         // Rx fifo level regsiter    (0x78)
    volatile uint32_t reserved1;     // reserved                  (0x7c)
    volatile uint32_t tx_abrt_source;// Tx abort status register  (0x80)
    volatile uint32_t reserved2;     // reserved                  (0x84)
    volatile uint32_t dma_cr;        // dma control register      (0x88)
    volatile uint32_t dma_tdlr;      // dma transmit data level   (0x8c)
    volatile uint32_t dma_rdlr;      // dma receive data level    (0x90)
    volatile uint32_t reserved3[24]; // reserved             (0x94-0xf0)
    volatile uint32_t comp_param_1;  // component parameters 1    (0xf4)
    volatile uint32_t comp_version;  // component version         (0xf8)
    volatile uint32_t comp_type;     // component type            (0xfc)
};
/*****/

/****is* i2c.api/dw_i2c_instance
 * DESCRIPTION
 *  This structure contains variables which relate to each individual
 *  i2c instance.  Cumulatively, they can be thought of as the "state
 *  variables" for each i2c instance (or as the global variables per
 *  i2c driver instance).
 * SOURCE
 */
struct dw_i2c_instance {
    enum dw_i2c_state state;    // i2c driver state
    uint32_t intr_mask_save;    // saved value of interrupt mask
    dw_callback listener;       // user event listener
    uint16_t *b2bBuffer;        // pointer to user Tx buffer
    uint8_t *txBuffer;          // pointer to user Tx buffer
    uint32_t txHold;            // Tx holding register
    unsigned txIdx;             // Tx holding register index
    unsigned txLength;          // length of user Tx buffer
    unsigned txRemain;          // space left in user Tx buffer
    dw_callback txCallback;     // user Tx callback function
    uint8_t *rxBuffer;          // pointer to user Rx buffer
    uint32_t rxHold;            // Rx holding register
    unsigned rxIdx;             // Rx holding register index
    unsigned rxLength;          // length of user Rx buffer
    unsigned rxRemain;          // space left in user Rx buffer
    dw_callback rxCallback;     // user Rx callback function
    uint8_t rxThreshold;        // default Rx fifo threshold level
    uint8_t txThreshold;        // default Tx fifo threshold level
    bool rxAlign;               // is Rx buffer 32-bit word-aligned?
    struct dw_dma_config dmaTx; // DMA Tx configuration
    struct dw_dma_config dmaRx; // DMA Rx configuration
};
/*****/

/****if* i2c.api/dw_i2c_resetInstance
 * NAME
 *  dw_i2c_resetInstance
 * USAGE
 *  dw_i2c_resetInstance(i2c);
 * DESCRIPTION
 *  This function resets/zeros all variables found in the
 *  dw_i2c_instance structure, except for dw_i2c_statistics.
 * ARGUMENTS
 *  i2c         i2c device handle
 * RETURN VALUE
 *  none
 * SEE ALSO
 *  dw_i2c_init(), dw_i2c_resetStatistics()
 * SOURCE
 */
void dw_i2c_resetInstance(struct dw_device *);
/*****/

/****if* i2c.api/dw_i2c_autoCompParams
 * DESCRIPTION
 *  This function attempts to automatically discover the hardware
 *  component parameters, if this supported by the i2c in question.
 *  This is usually controlled by the ADD_ENCODED_PARAMS coreConsultant
 *  parameter.
 * ARGUMENTS
 *  i2c         i2c device handle
 * RETURN VALUE
 *  0           if successful
 *  -ENOSYS     function not supported
 * USES
 *  Accesses the following DW_apb_i2c register/bitfield(s):
 *   - ic_comp_type
 *   - ic_comp_version
 *   - ic_comp_param_1/all bits
 * NOTES
 *  This function does not allocate any memory.  An instance of
 *  dw_i2c_param must already be allocated and properly referenced from
 *  the relevant compParam dw_device structure member.
 * SEE ALSO
 *  dw_i2c_init()
 * SOURCE
 */
int dw_i2c_autoCompParams(struct dw_device *);
/*****/

/****if* i2c.api/dw_i2c_flushRxHold
 * NAME
 *  dw_i2c_flushRxHold
 * USAGE
 *  ecode = dw_i2c_flushRxHold(i2c);
 * DESCRIPTION
 *  This functions virtually flushes any data in the hold variable to
 *  the buffer (both in the dw_i2c_instance structure).  The 'hold'
 *  variable normally stores up to four data bytes before they are
 *  written to memory (i.e. the user buffer) to optimize bus performace.
 *  Flushing the
 *  (instance->) hold variable only makes sense when the i2c is in
 *  either master-receiver or slave-receiver mode.
 * ARGUMENTS
 *  i2c         i2c device handle
 * RETURN VALUE
 *  0           if successful
 *  -EPERM      if the i2c is not in a receive mode (master-rx/slave-rx)
 * NOTES
 *  This function comprises part of the interrupt-driven interface and
 *  normally should never need to be called directly.  The
 *  dw_i2c_terminate function always calls dw_i2c_flushRxHold before
 *  terminating a transfer.
 * SEE ALSO
 *  dw_i2c_terminate()
 * SOURCE
 */
int dw_i2c_flushRxHold(struct dw_device *);
/*****/

#ifdef __cplusplus
}
#endif

#endif  // DW_I2C_PRIVATE_H

