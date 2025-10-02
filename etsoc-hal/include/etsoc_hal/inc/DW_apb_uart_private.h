/* --------------------------------------------------------------------
** 
** Synopsys DesignWare DW_apb_uart Software Driver Kit and
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

#ifndef DW_UART_PRIVATE_H
#define DW_UART_PRIVATE_H

#ifdef __cplusplus
extern "C" {    // allow C++ to use these headers
#endif

/****id* uart.macros/requirements
 * NAME
 *  Common API Requirements
 * DESCRIPTION
 *  These are the common preconditions which must be met for all driver
 *  functions.  Primarily, they check that a function has been passed
 *  a legitimate dw_device structure.
 * SOURCE
 */
#define UART_COMMON_REQUIREMENTS(p)             \
do {                                            \
    DW_REQUIRE(p != NULL);                      \
    DW_REQUIRE(p->instance != NULL);            \
    DW_REQUIRE(p->comp_param != NULL);          \
    DW_REQUIRE(p->base_address != NULL);        \
    DW_REQUIRE(p->comp_type == Dw_apb_uart);    \
    DW_REQUIRE((p->data_width == 32)            \
            || (p->data_width == 16)            \
            || (p->data_width == 8));           \
} while(0)
/*****/

/****id* uart.macros/bfws
 * NAME
 *  Bit-field width/shift definitions
 * DESCRIPTION
 *  Used in conjunction with bitops.h to access register bitfields.
 *  They are defined as bit offset/mask tuples for each uart register
 *  bitfield.
 * EXAMPLE
 *  uint32_t irqID = DW_BIT_GET(in32(UART_IIR), UART_IIR_INTERRUPT_ID);
 * NOTES
 *  bfo is the offset of the bitfield with respect to LSB;
 *  bfw is the width of the bitfield
 * SEE ALSO
 *  bitops.h
 ***/
// interrupt enable register
#define bfoUART_IER_ERBFI               0   // received data available
#define bfwUART_IER_ERBFI               1
#define bfoUART_IER_ETBEI               1   // transmitter holding
#define bfwUART_IER_ETBEI               1   //  register
#define bfoUART_IER_ELSI                2   // receiver line status
#define bfwUART_IER_ELSI                1
#define bfoUART_IER_EDSSI               3   // modem status
#define bfwUART_IER_EDSSI               1
#define bfoUART_IER_PTIME               7   // programmable THRE
#define bfwUART_IER_PTIME               1   //  interrupt mode
// interrupt identity register
#define bfoUART_IIR_INTERRUPT_ID        0   // interrupt identity
#define bfwUART_IIR_INTERRUPT_ID        4
#define bfoUART_IIR_FIFO_STATUS         6   // FIFO status
#define bfwUART_IIR_FIFO_STATUS         2
// FIFO control register
#define bfoUART_FCR_FIFO_ENABLE         0   // FIFO enable
#define bfwUART_FCR_FIFO_ENABLE         1
#define bfoUART_FCR_RCVR_FIFO_RESET     1   // receiver FIFO reset
#define bfwUART_FCR_RCVR_FIFO_RESET     1
#define bfoUART_FCR_XMIT_FIFO_RESET     2   // transmitter FIFO reset
#define bfwUART_FCR_XMIT_FIFO_RESET     1
#define bfoUART_FCR_DMA_MODE            3   // dma mode
#define bfwUART_FCR_DMA_MODE            1
#define bfoUART_FCR_TX_EMPTY_TRIGGER    4   // transmitter empty trigger
#define bfwUART_FCR_TX_EMPTY_TRIGGER    2
#define bfoUART_FCR_RCVR_TRIGGER        6   // receiver trigger
#define bfwUART_FCR_RCVR_TRIGGER        2
// line control register
#define bfoUART_LCR_CLS                 0   // number of bits per
#define bfwUART_LCR_CLS                 2   //  character
#define bfoUART_LCR_STOP                2   // stop bits
#define bfwUART_LCR_STOP                1
#define bfoUART_LCR_PEN                 3   // parity enable
#define bfwUART_LCR_PEN                 1
#define bfoUART_LCR_EPS                 4   // parity select
#define bfwUART_LCR_EPS                 1
#define bfoUART_LCR_PARITY              3   // composite enable and
#define bfwUART_LCR_PARITY              2   //  select field
#define bfoUART_LCR_LINE                0   // composite line
#define bfwUART_LCR_LINE                5   //   control field
#define bfoUART_LCR_STICK_PARITY        5   // unsupported
#define bfwUART_LCR_STICK_PARITY        1   // unsupported
#define bfoUART_LCR_BREAK               6   // line break
#define bfwUART_LCR_BREAK               1
#define bfoUART_LCR_DLAB                7   // divisor latch address bit
#define bfwUART_LCR_DLAB                1
// modem control register
#define bfoUART_MCR_DTR                 0   // dtr
#define bfwUART_MCR_DTR                 1
#define bfoUART_MCR_RTS                 1   // rts
#define bfwUART_MCR_RTS                 1
#define bfoUART_MCR_OUT1                2   // out1
#define bfwUART_MCR_OUT1                1
#define bfoUART_MCR_OUT2                3   // out2
#define bfwUART_MCR_OUT2                1
#define bfoUART_MCR_LOOPBACK            4   // loopback
#define bfwUART_MCR_LOOPBACK            1
#define bfoUART_MCR_AFCE                5   // automatic flow control
#define bfwUART_MCR_AFCE                1
#define bfoUART_MCR_SIRE                6   // serial infra-red
#define bfwUART_MCR_SIRE                1
// line status register
#define bfoUART_LSR_DR                  0   // data ready
#define bfwUART_LSR_DR                  1
#define bfoUART_LSR_OE                  1   // overrun error
#define bfwUART_LSR_OE                  1
#define bfoUART_LSR_PE                  2   // parity error
#define bfwUART_LSR_PE                  1
#define bfoUART_LSR_FE                  3   // framing error
#define bfwUART_LSR_FE                  1
#define bfoUART_LSR_BI                  4   // break interrupt
#define bfwUART_LSR_BI                  1
#define bfoUART_LSR_THRE                5   // transmitter holding
#define bfwUART_LSR_THRE                1   //  register empty
#define bfoUART_LSR_TEMT                6   // transmitter empty
#define bfwUART_LSR_TEMT                1
#define bfoUART_LSR_RX_FIFO_ERROR       7   // receiver FIFO error
#define bfwUART_LSR_RX_FIFO_ERROR       1
// modem status register
#define bfoUART_MSR_DCTS                0   // dcts
#define bfwUART_MSR_DCTS                1
#define bfoUART_MSR_DDSR                1   // ddsr
#define bfwUART_MSR_DDSR                1
#define bfoUART_MSR_TERI                2   // teri
#define bfwUART_MSR_TERI                1
#define bfoUART_MSR_DDCD                3   // ddcd
#define bfwUART_MSR_DDCD                1
#define bfoUART_MSR_CTS                 4   // cts
#define bfwUART_MSR_CTS                 1
#define bfoUART_MSR_DSR                 5   // dsr
#define bfwUART_MSR_DSR                 1
#define bfoUART_MSR_RI                  6   // ri
#define bfwUART_MSR_RI                  1
#define bfoUART_MSR_DCD                 7   // dcd
#define bfwUART_MSR_DCD                 1
// FIFO access register
#define bfoUART_FAR_FIFO_ACCESS         0   // FIFO access test mode
#define bfwUART_FAR_FIFO_ACCESS         1
// transmit FIFO read register
#define bfoUART_TFR_TFRD                0   // Tx FIFO read data
#define bfwUART_TFR_TFRD                8
// receive FIFO write register
#define bfoUART_RFW_RFWD                0   // Rx FIFO write data
#define bfwUART_RFW_RFWD                8
#define bfoUART_RFW_RFPE                8   // Rx FIFO parity data
#define bfwUART_RFW_RFPE                1
#define bfoUART_RFW_RFFE                9   // Rx FIFO framing error
#define bfwUART_RFW_RFFE                1
// uart status register
#define bfoUART_USR_BUSY                0   // serial transfer is in
#define bfwUART_USR_BUSY                1   //  progress
#define bfoUART_USR_TFNF                1   // Tx FIFO not full
#define bfwUART_USR_TFNF                1
#define bfoUART_USR_TFE                 2   // Tx FIFO empty
#define bfwUART_USR_TFE                 1
#define bfoUART_USR_RFNE                3   // Rx FIFO not empty
#define bfwUART_USR_RFNE                1
#define bfoUART_USR_RFF                 4   // Rx FIFO full
#define bfwUART_USR_RFF                 1
// transmit FIFO level register
#define bfoUART_TFL_LEVEL               0   // Tx FIFO data level
#define bfwUART_TFL_LEVEL               8
// receive FIFO level register
#define bfoUART_RFL_LEVEL               0   // Rx FIFO data level
#define bfwUART_RFL_LEVEL               8
// software reset register
#define bfoUART_SRR_UR                  0   // UART reset
#define bfwUART_SRR_UR                  1
#define bfoUART_SRR_RFR                 1   // Rx FIFO reset
#define bfwUART_SRR_RFR                 1
#define bfoUART_SRR_XFR                 2   // Tx FIFO reset
#define bfwUART_SRR_XFR                 1
// shadow request to send register
#define bfoUART_SRTS_RTS                0   // shadow request to send
#define bfwUART_SRTS_RTS                1
// shadow break control register
#define bfoUART_SBCR_BCR                0   // shadow break control
#define bfwUART_SBCR_BCR                1
// shadow DMA mode register
#define bfoUART_SDMAM_MODE              0   // shadow DMA mode
#define bfwUART_SDMAM_MODE              1
// shadow FIFO enable register
#define bfoUART_SFE_FE                  0   // shadow FIFO enable
#define bfwUART_SFE_FE                  1
// shadow receiver trigger register
#define bfoUART_SRT_TRIGGER             0   // shadow Rx FIFO full
#define bfwUART_SRT_TRIGGER             2   //  trigger
// shadow transmitter empty trigger register
#define bfoUART_STET_TRIGGER            0   // shadow Tx FIFO empty
#define bfwUART_STET_TRIGGER            2   //  trigger
// halt transmitter register
#define bfoUART_HTX_HALT                0   // halt transmitter
#define bfwUART_HTX_HALT                1
// DMA software acknowledge register
#define bfoUART_DMASA_ACK               0   // DMA software acknowledge
#define bfwUART_DMASA_ACK               1
// uart component parameters
#define bfoUART_PARAM_DATA_WIDTH        0
#define bfwUART_PARAM_DATA_WIDTH        2
#define bfoUART_PARAM_AFCE_MODE         4
#define bfwUART_PARAM_AFCE_MODE         1
#define bfoUART_PARAM_THRE_MODE         5
#define bfwUART_PARAM_THRE_MODE         1
#define bfoUART_PARAM_SIR_MODE          6
#define bfwUART_PARAM_SIR_MODE          1
#define bfoUART_PARAM_SIR_LP_MODE       7
#define bfwUART_PARAM_SIR_LP_MODE       1
#define bfoUART_PARAM_NEW_FEAT          8
#define bfwUART_PARAM_NEW_FEAT          1
#define bfoUART_PARAM_FIFO_ACCESS       9
#define bfwUART_PARAM_FIFO_ACCESS       1
#define bfoUART_PARAM_FIFO_STAT         10
#define bfwUART_PARAM_FIFO_STAT         1
#define bfoUART_PARAM_SHADOW            11
#define bfwUART_PARAM_SHADOW            1
#define bfoUART_PARAM_ADD_ENCODED_PARAMS    12
#define bfwUART_PARAM_ADD_ENCODED_PARAMS    1
#define bfoUART_PARAM_DMA_EXTRA         13
#define bfwUART_PARAM_DMA_EXTRA         1
#define bfoUART_PARAM_FIFO_MODE         16
#define bfwUART_PARAM_FIFO_MODE         8

/****id* uart.macros/DW_CC_DEFINE_UART_PARAMS
 * DESCRIPTION
 *  This macro is used to initialize a dw_uart_param structure.  To use
 *  this macro, the relevant C header file must also be included.  This
 *  is generated when a DesignWare device is synthesized.
 * NOTES
 *  The relevant uart coreConsultant C header must be included before
 *  this macro can be used.
 * SOURCE
 */
#define DW_CC_DEFINE_UART_PARAMS_3_01(prefix) { \
    prefix ## CC_UART_AFCE_MODE,                \
    prefix ## CC_UART_DMA_EXTRA,                \
    prefix ## CC_UART_FIFO_ACCESS,              \
    prefix ## CC_UART_FIFO_STAT,                \
    prefix ## CC_UART_ADDITIONAL_FEATURES,      \
    prefix ## CC_UART_SHADOW,                   \
    prefix ## CC_UART_SIR_LP_MODE,              \
    prefix ## CC_UART_SIR_MODE,                 \
    prefix ## CC_UART_THRE_MODE,                \
    prefix ## CC_UART_FIFO_MODE                 \
}

#define DW_CC_DEFINE_UART_PARAMS_2_00(prefix) { \
    prefix ## CC_UART_AFCE_MODE_USER,           \
    false,                                      \
    false,                                      \
    false,                                      \
    false,                                      \
    false,                                      \
    false,                                      \
    prefix ## CC_UART_SIR_MODE,                 \
    prefix ## CC_UART_THRE_MODE_USER,           \
    prefix ## CC_UART_FIFO_MODE                 \
}
/*****/

/****id* uart.macros/APB_ACCESS
 * DESCRIPTION
 *  This macro is used to hardcode the APB data accesses, if the APB
 *  data width is the same for an entire system.  Simply defining
 *  APB_DATA_WIDTH at compile time will force all DW_apb_uart memory map
 *  I/O accesses to be performed with the specifed data width.  By
 *  default, no I/O access is performed until the APB data width of a
 *  device is checked in the dw_device data structure.
 * SOURCE
 */
// Byte accesses remain unchanged regardless of APB data width
#define UART_IN8P   DW_IN32_32P   //FUTURE: Nikola Stanojev changed this by fixing the driver source code
#define UART_OUT8P  DW_OUT32_32P  //FUTURE: Nikola Stanojev changed this by fixing the driver source code

// Change 32-bit data accesses depending on APB data width
#ifdef APB_DATA_WIDTH

#if (APB_DATA_WIDTH == 32)

#define UART_INP    DW_IN32_32P
#define UART_OUTP   DW_OUT32_32P

#elif (APB_DATA_WIDTH == 16)

#define UART_INP    DW_IN32_16P
#define UART_OUTP   DW_OUT32_16P

#else       // (APB_DATA_WIDTH == 32)

#define UART_INP    DW_IN32_8P
#define UART_OUTP   DW_OUT32_8P

#endif      // (APB_DATA_WIDTH == 32)

#else       // APB_DATA_WIDTH

#define UART_INP    DW_INP
#define UART_OUTP   DW_OUTP

#endif      // APB_DATA_WIDTH
/*****/

/****id* uart.macros/UART_FIFO_READ
 * DESCRIPTION
 *  This macro reads as much data from the Rx FIFO as possible until its
 *  or the destination buffer is full.  Writes to the destination buffer
 *  are in word-size memory accesses for speed and bus efficieny.
 * SEE ALSO
 *  UART_X_FIFO_READ, UART_FIFO_WRITE
 * SOURCE
 */
#define UART_FIFO_READ()                                            \
do {                                                                \
    volatile uint32_t regg;                                          \
    if(instance->rxRemain == 0)                                     \
        break;                                                      \
    regg = UART_INP(portmap->lsr);                                   \
    instance->lsr_save = (regg & (Uart_line_oe | Uart_line_pe |      \
                Uart_line_fe | Uart_line_bi));                      \
    /* While the Rx FIFO is not empty. */                           \
    while(DW_BIT_GET(regg, UART_LSR_DR) == Dw_set) {                 \
        instance->rxHold >>= 8;                                     \
        instance->rxHold |= (uint32_t) (DW_IN8P(portmap->rbr_thr_dll) << 24);  \
        if(--instance->rxIdx == 0) {                                \
            *(instance->rxBuffer++) = instance->rxHold;             \
            instance->rxIdx = 4;                                    \
        }                                                           \
        if(--instance->rxRemain == 0)                               \
            break;                                                  \
        regg = UART_INP(portmap->lsr);                               \
        /* Save any line status errors that may have occurred */    \
        instance->lsr_save = (regg & (Uart_line_oe | Uart_line_pe |  \
                    Uart_line_fe | Uart_line_bi));                  \
    }                                                               \
} while(0)
/*****/

/****id* uart.macros/UART_X_FIFO_READ
 * DESCRIPTION
 *  This macro is similar to UART_FIFO_READ, except it read 'MAX'
 *  characters from the Rx FIFO, as long as rxRemain > 0.  This
 *  implementation is much more efficient because the line status
 *  register is not polled after each FIFO read in order to determine if
 *  there is still valid data available.
 * SEE ALSO
 *  UART_FIFO_READ, UART_X_FIFO_WRITE
 * SOURCE
 */
#define UART_X_FIFO_READ(MAX)                                       \
do {                                                                \
    int i;                                                          \
    if(instance->rxRemain == 0)                                     \
        break;                                                      \
    for(i = 0; i < (MAX); i++) {                                    \
        instance->rxHold >>= 8;                                     \
        instance->rxHold |= (unsigned) (DW_IN8P(portmap->rbr_thr_dll) << 24);  \
        if(--instance->rxIdx == 0) {                                \
            *(instance->rxBuffer++) = instance->rxHold;             \
            instance->rxIdx = 4;                                    \
        }                                                           \
        if(--instance->rxRemain == 0)                               \
            break;                                                  \
    }                                                               \
} while(0)
/*****/

/****id* uart.macros/UART_FIFO_WRITE
 * DESCRIPTION
 *  This macro writes as much of the current user Tx buffer data to the
 *  Tx FIFO until it fills or there is no more data to send.  Reads
 *  words at a time to improve speed and bus efficiency.
 * SEE ALSO
 *  UART_X_FIFO_WRITE, UART_FIFO_READ
 * SOURCE
 */
#define UART_FIFO_WRITE()                                           \
do {                                                                \
    volatile uint32_t regg;                                          \
    if(instance->txRemain == 0)                                     \
        break;                                                      \
    regg = UART_INP(portmap->lsr);                                   \
    /* Save any line status errors that may have occurred */        \
    instance->lsr_save = (regg & (Uart_line_oe | Uart_line_pe |      \
                Uart_line_fe | Uart_line_bi));                      \
    /* Fill up the FIFO until the THRE bit is Set (Tx FIFO full) */ \
    if((dw_uart_isPtimeEnabled(dev) == true) &&                     \
            (instance->fifos_enabled == true)) {                    \
        /* While the Tx FIFO is not full (PTIME enabled). */        \
        while(DW_BIT_GET(regg, UART_LSR_THRE) == Dw_clear) {         \
            if(instance->txIdx == 0) {                              \
                instance->txHold = *(instance->txBuffer++);         \
                instance->txIdx = 4;                                \
            }                                                       \
            DW_OUT8P((instance->txHold & 0xff),                     \
                    portmap->rbr_thr_dll);                          \
            instance->txHold >>= 8;                                 \
            instance->txIdx--;                                      \
            if(--instance->txRemain == 0)                           \
                break;                                              \
            regg = UART_INP(portmap->lsr);                           \
            /* Save any line status errors that may have occurred */\
            instance->lsr_save = (regg & (Uart_line_oe | Uart_line_pe\
                        | Uart_line_fe | Uart_line_bi));            \
        }                                                           \
    }                                                               \
    /* Fill up the FIFO if the THRE bit is Set (Tx FIFO empty) */   \
    else if(instance->fifos_enabled == true) {                      \
        if(DW_BIT_GET(regg, UART_LSR_THRE) == Dw_set)                \
            UART_X_FIFO_WRITE(param->fifo_mode);                    \
    }                                                               \
    /* Can only write one character if FIFOs are disabled */        \
    else if(DW_BIT_GET(regg, UART_LSR_THRE) == Dw_set) {             \
        if(instance->txIdx == 0) {                                  \
            instance->txHold = *(instance->txBuffer++);             \
            instance->txIdx = 4;                                    \
        }                                                           \
        DW_OUT8P((instance->txHold & 0xff), portmap->rbr_thr_dll);  \
        instance->txHold >>= 8;                                     \
        instance->txIdx--;                                          \
        instance->txRemain--;                                       \
    }                                                               \
    /* This clause should never be reached */                       \
    else {                                                          \
        DW_ASSERT(false);                                           \
    }                                                               \
} while(0)
/*****/

/****id* uart.macros/UART_X_FIFO_WRITE
 * DESCRIPTION
 *  This macro is similar to UART_FIFO_WRITE except it writes 'MAX'
 *  characters to the Tx FIFO, as long as txRemain > 0.  This
 *  implementation is much more efficient as it does not check the line
 *  status register after every write to determine if there is space
 *  available in the Tx FIFO or not.
 * SEE ALSO
 *  UART_FIFO_WRITE, UART_X_FIFO_READ
 * SOURCE
 */
#define UART_X_FIFO_WRITE(MAX)                                      \
do {                                                                \
    int i;                                                          \
    if(instance->txRemain == 0)                                     \
        break;                                                      \
    for(i = 0; i < (MAX); i++) {                                    \
        if(instance->txIdx == 0) {                                  \
            instance->txHold = *(instance->txBuffer++);             \
            instance->txIdx = 4;                                    \
        }                                                           \
        DW_OUT8P((instance->txHold & 0xff), portmap->rbr_thr_dll);  \
        instance->txHold >>= 8;                                     \
        instance->txIdx--;                                          \
        if(--instance->txRemain == 0)                               \
            break;                                                  \
    }                                                               \
} while(0)
/*****/

/****id* uart.macros/UART_ENTER_CRITICAL_SECTION
 * DESCRIPTION
 *  This macro disables DW_apb_uart interrupts, to avoid shared data
 *  issues when entering a critical section of the driver.  A copy of
 *  the current IER value is kept in the instance structure so that the
 *  interrupts can be later restored.
 * SEE ALSO
 *  UART_EXIT_CRITICAL_SECTION, dw_uart_instance
 * SOURCE
 */
#define UART_ENTER_CRITICAL_SECTION()           \
do {                                            \
    uint32_t reg;                               \
    /* Don't change the value of PTIME */       \
    reg = (unsigned)instance->ier_save & ~((unsigned)Uart_irq_all);   \
    /* Disable UART interrupts */               \
    UART_OUT8P(reg, portmap->ier_dlh);          \
} while (0)
/*****/

/****id* uart.macros/UART_EXIT_CRITICAL_SECTION
 * DESCRIPTION
 *  This macro restores DW_apb_uart interrupts, after a critical code
 *  section.  It uses the saved IER value in the instance structure to
 *  accomplish this.
 * SEE ALSO
 *  UART_ENTER_CRITICAL_SECTION, dw_uart_instance
 * SOURCE
 */
#define UART_EXIT_CRITICAL_SECTION()                    \
do {                                                    \
    /* Restore UART interrupts */                       \
    UART_OUT8P(instance->ier_save, portmap->ier_dlh);   \
} while (0)
/*****/

/****id* uart.data/dw_uart_state
 * DESCRIPTION
 *  These are the states used internally by the driver when managing
 *  interrupt-driven transfers.
 * SOURCE
 */
enum dw_uart_state {
    Uart_state_idle = 0,    // driver is idle
    Uart_state_tx,          // driver is transmitting data
    Uart_state_rx,          // driver is receiving data
    Uart_state_rx_req,      // user must call dw_uart_receive()
    Uart_state_tx_rx,       // driver is transceiving data
    Uart_state_tx_rx_req    // ditto as above; the user must call
                            // dw_uart_receive() to handle Rx transfer
};
/*****/

/****is* uart.data/dw_uart_param
 * DESCRIPTION
 *  This structure comprises the uart hardware parameters that affect
 *  the software driver.  This structure needs to be initialized with
 *  the correct values and be pointed to by the (struct dw_device).cc
 *  member of the relevant uart device structure.
 * SOURCE
 */
struct dw_uart_param {
    bool afce_mode;         // automatic flow control
    bool dma_extra;         // DW_ahb_dmac compatibility signals
    bool fifo_access;       // programmable FIFO access mode
    bool fifo_stat;         // additional FIFO status registers
    bool new_feat;          // new v3 features (shadow registers, etc.)
    bool shadow;            // include shadow registers
    bool sir_lp_mode;       // low-power IrDA SIR mode
    bool sir_mode;          // serial infrared
    bool thre_mode;         // programmable thre interrupts
    uint8_t fifo_mode;      // FIFO depth
};
/*****/

/****is* uart.data/dw_uart_portmap
 * DESCRIPTION
 *  This is the structure used for accessing the uart register memory
 *  map.
 * EXAMPLE
 *  struct dw_uart_portmap *portmap;
 *  portmap = (struct dw_uart_portmap *) DW_APB_UART_BASE;
 *  lineStatus = INP(portmap->lsr);
 * SOURCE
 */
struct dw_uart_portmap {
    volatile uint32_t rbr_thr_dll;  // receive buffer register
                                    // receive buffer register
                                    // divisor latch low          (0x00)
    volatile uint32_t ier_dlh;      // interrupt enable register
                                    // divisor latch high         (0x04)
    volatile uint32_t iir_fcr;      // interrupt identity register
                                    // FIFO control register      (0x08)
    volatile uint32_t lcr;          // line control register      (0x0c)
    volatile uint32_t mcr;          // modem control register     (0x10)
    volatile uint32_t lsr;          // line status register       (0x14)
    volatile uint32_t msr;          // modem status register      (0x18)
    volatile uint32_t scr;          // scratch register           (0x1c)
    volatile uint32_t reserved1[4]; // reserved              (0x20-0x2c)
    // This is deliberately not marked as volatile as the address needs
    // to be used with memcpy, which won't accept a volatile pointer.
    uint32_t srbr_sthr[16];         // shadow receive buffer and  (0x30-
                                    // transmit holding registers  0x6c)
    volatile uint32_t far;          // FIFO access register       (0x70)
    volatile uint32_t tfr;          // transmit FIFO read         (0x74)
    volatile uint32_t rfw;          // receiver FIFO write        (0x78)
    volatile uint32_t usr;          // uart status register       (0x7c)
    volatile uint32_t tfl;          // transmit FIFO level        (0x80)
    volatile uint32_t rfl;          // receive FIFO level         (0x84)
    volatile uint32_t srr;          // software reset register    (0x88)
    volatile uint32_t srts;         // shadow request to send     (0x8c)
    volatile uint32_t sbcr;         // shadow break control       (0x90)
    volatile uint32_t sdmam;        // shadow dma mode            (0x94)
    volatile uint32_t sfe;          // shadow FIFO enable         (0x98)
    volatile uint32_t srt;          // shadow receiver trigger    (0x9c)
    volatile uint32_t stet;         // shadow transmitter trigger (0xa0)
    volatile uint32_t htx;          // halt Tx                    (0xa4)
    volatile uint32_t dmasa;        // dma software acknowledge   (0xa8)
    volatile uint32_t reserved2[18];// reserved              (0xac-0xf0)
    volatile uint32_t comp_param_1; // component parameters       (0xf4)
    volatile uint32_t comp_version; // component version          (0xf8)
    volatile uint32_t comp_type;    // component type             (0xfc)
};
/*****/

/****is* uart.data/dw_uart_instance
 * DESCRIPTION
 *  This structure contains variables which relate to each individual
 *  uart instance.  Cumulatively, they can be thought of as the "state
 *  variables" for each distinct uart device.
 * SOURCE
 */
struct dw_uart_instance {
    enum dw_uart_state state;           // uart driver state
    uint32_t value_in_fcr;              // FCR value (write only reg)
    uint32_t ier_save;                  // saved IER value
    uint32_t lsr_save;                  // saved line status errors
    dw_callback listener;          	// user event listener
    dw_callback txCallback;        	// user Tx callback function
    dw_callback rxCallback;        	// user Rx callback function
    enum dw_uart_callback_mode callbackMode;    // Rx callback mode
    uint32_t *txBuffer;                 // pointer to user Tx buffer
    uint32_t txHold;                    // Tx holding register
    unsigned txIdx;                     // Tx holding register index
    unsigned txLength;                  // user Tx buffer length
    unsigned txRemain;                  // chars left to send in buffer
    uint32_t *rxBuffer;                 // pointer to user Rx buffer
    uint32_t rxHold;                    // Rx holding register
    unsigned rxIdx;                     // Rx holding register index
    unsigned rxLength;                  // user Rx buffer length
    unsigned rxRemain;                  // space left in buffer
    bool rxAlign;                       // is Rx buffer aligned?
    bool fifos_enabled;                 // are FIFOs enabled? (cached)
    enum dw_uart_tx_trigger txTrigger;  // saved user Tx trigger
    struct dw_dma_config dmaTx;         // DMA Tx configuration
    struct dw_dma_config dmaRx;         // DMA Rx configuration
};
/*****/

/****if* uart.functions/dw_uart_resetInstance
 * DESCRIPTION
 *  Initializes the uart instance structure members.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  none
 * NOTES
 *  Interrupt *must* be disabled before calling this function to avoid
 *  shared data issues.
 * SEE ALSO
 *  dw_uart_instance
 * SOURCE
 */
void dw_uart_resetInstance(struct dw_device *dev);
/*****/

/****if* uart.functions/dw_uart_flushRxFifo
 * DESCRIPTION
 *  Flushes any characters present in the Rx FIFO to the user Rx buffer.
 * ARGUMENTS
 *  dev         DW_apb_uart device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -EPERM      -- no Rx transfer is currently in progress
 * NOTES
 *  Interrupt *must* be disabled before calling this function to avoid
 *  shared data issues.
 * SEE ALSO
 *  dw_uart_flushRxHold()
 * SOURCE
 */
int dw_uart_flushRxFifo(struct dw_device *dev);
/*****/

/****if* uart.functions/dw_uart_flushRxHold
 * DESCRIPTION
 *  Flushes any characters present in the instance->rxHold variable to
 *  the user Rx buffer.
 * ARGUMENTS
 *  dev         UART device handle
 * RETURN VALUE
 *  none
 * NOTES
 *  Interrupt *must* be disabled before calling this function to avoid
 *  shared data issues.
 * SEE ALSO
 *  dw_uart_flushRxFifo()
 * SOURCE
 */
void dw_uart_flushRxHold(struct dw_device *dev);
/*****/

/****if* uart.functions/dw_uart_autoCompParams
 * DESCRIPTION
 *  This function attempts to automatically discover the component
 *  hardware parameters, if possible.  If this is not supported, an
 *  error code is returned.
 * ARGUMENTS
 *  dev         uart device handle
 * RETURN VALUE
 *  0           if successful
 *  -ENOSYS     -- auto-configuration is not supported
 * NOTES
 *  This function is affected by the ADD_ENCODED_PARAMS hardware
 *  parameter.
 *  Interrupt *must* be disabled before calling this function to avoid
 *  shared data issues.
 * SEE ALSO
 *  dw_uart_param
 * SOURCE
 */
int dw_uart_autoCompParams(struct dw_device *dev);
/*****/



#ifdef __cplusplus
}
#endif

#endif  // DW_UART_PRIVATE_H

