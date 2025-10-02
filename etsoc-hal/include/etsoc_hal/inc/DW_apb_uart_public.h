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

#ifndef DW_UART_PUBLIC_H
#define DW_UART_PUBLIC_H

#ifdef __cplusplus
extern "C" {    // allow C++ to use these headers
#endif

/****h* drivers.uart/uart.api
 * NAME
 *  DW_apb_uart API overview
 * DESCRIPTION
 *  This section gives an overview of the DW_apb_uart software driver
 *  Application Programming Interface (API).
 * SEE ALSO
 *  uart.data, uart.functions
 ***/

/****h* drivers.uart/uart.data
 * NAME
 *  DW_apb_uart driver data types and definitions
 * DESCRIPTION
 *  This section details all the public data types and definitions used
 *  with the DW_apb_uart software driver.
 * SEE ALSO
 *  uart.api, uart.functions
 ***/

/****h* drivers.uart/uart.functions
 * NAME
 *  DW_apb_uart driver data types and definitions
 * DESCRIPTION
 *  This section details all the public functions available for use with
 *  the DW_apb_uart software driver.
 * SEE ALSO
 *  uart.api, uart.data
 ***/

/****h* uart.api/uart.data_types
 * NAME
 *  DW_apb_uart driver data types and definitions
 * DESCRIPTION
 *  enum dw_uart_event          -- occurrence of an interrupt
 *  enum dw_uart_irq            -- enabling/disabling interrupts
 *  enum dw_uart_line_status    -- value of line status register
 *  enum dw_uart_modem_status   -- value of modem status register
 *  enum dw_uart_modem_line     -- modem control settings
 *  enum dw_uart_rx_trigger     -- Rx FIFO full trigger level
 *  enum dw_uart_tx_trigger     -- Tx FIFO empty trigger level
 *  enum dw_uart_dma_mode       -- single- or multi- DMA mode
 *  enum dw_uart_stop_bits      -- number of stop bits
 *  enum dw_uart_parity         -- odd, even or no parity
 *  enum dw_uart_cls            -- number of data bits
 *  enum dw_uart_line_control   -- line control settings
 *  enum dw_uart_callback_mode  -- Rx callback mode (used w/interrupts)
 *
 * SEE ALSO
 *  uart.configuration, uart.command, uart.status, uart.interrupt,
 *  uart.debug
 ***/

/****h* uart.api/uart.configuration
 * NAME
 *  DW_apb_uart driver configuration functions
 * DESCRIPTION
 *  dw_uart_enableFifos         -- enable FIFOs
 *  dw_uart_disableFifos        -- disable FIFOs
 *  dw_uart_enablePtime         -- enable Programmable THRE IRQ Mode
 *  dw_uart_disablePtime        -- disable Programmable THRE IRQ Mode
 *  dw_uart_setDmaMode          -- single- or multi- DMA mode
 *  dw_uart_setTxTrigger        -- set the Tx FIFO empty IRQ trigger
 *  dw_uart_setRxTrigger        -- set the Rx FIFO full IRQ trigger
 *  dw_uart_setClockDivisor     -- set the clock divisor/baud rate
 *  dw_uart_setLineControl      -- set the line control register
 *  dw_uart_enableLoopback      -- enable loopback mode
 *  dw_uart_disableLoopback     -- disable loopback mode
 *  dw_uart_enableAfc           -- enable AFC mode
 *  dw_uart_disableAfc          -- disable AFC mode
 *  dw_uart_enableSir           -- enable serial infra-red mode
 *  dw_uart_disableSir          -- disable serial infra-red mode
 *  dw_uart_setScratchpad       -- set the scratchpad value
 *  dw_uart_enableIrq           -- enable interrupt(s)
 *  dw_uart_disableIrq          -- disable interrupt(s)
 *
 * SEE ALSO
 *  uart.data_types, uart.command, uart.status, uart.interrupt,
 *  uart.debug
 ***/

/****h* uart.api/uart.command
 * NAME
 *  DW_apb_uart driver command functions
 * DESCRIPTION
 *  dw_uart_init                -- initialize the device driver
 *  dw_uart_reset               -- reset the device
 *  dw_uart_resetTxFifo         -- reset the Tx FIFO
 *  dw_uart_resetRxFifo         -- reset the Rx FIFO
 *  dw_uart_dmaSwAck            -- DMA software acknowledge
 *  dw_uart_setBreak            -- send a break condition
 *  dw_uart_setModemLine        -- set modem line(s)
 *  dw_uart_clearModemLine      -- clear modem line(s)
 *  dw_uart_getLineStatus       -- get value of line status register
 *  dw_uart_getModemStatus      -- get value of modem status register
 *  dw_uart_read                -- read from the Rx FIFO
 *  dw_uart_write               -- write to the Tx FIFO
 *  dw_uart_burstRead           -- burst read from the Rx FIFO
 *  dw_uart_burstWrite          -- burst write to the Tx FIFO
 *
 * SEE ALSO
 *  uart.data_types, uart.configuration, uart.status, uart.interrupt,
 *  uart.debug
 ***/

/****h* uart.api/uart.status
 * NAME
 *  DW_apb_uart driver status functions
 * DESCRIPTION
 *  dw_uart_areFifosEnabled     -- are FIFOs enabled?
 *  dw_uart_isTxFifoFull        -- is the Tx FIFO full?
 *  dw_uart_isTxFifoEmpty       -- is the Tx FIFO empty?
 *  dw_uart_isRxFifoFull        -- is the Rx FIFO full?
 *  dw_uart_isRxFifoEmpty       -- is the Rx FIFO empty?
 *  dw_uart_isBusy              -- is the device busy?
 *  dw_uart_isPtimeEnabled      -- is PTIME enabled?
 *  dw_uart_getTxFifoLevel      -- get the Tx FIFO level
 *  dw_uart_getRxFifoLevel      -- get the Rx FIFO level
 *  dw_uart_getDmaMode          -- get the DMA mode
 *  dw_uart_getTxTrigger        -- get the Tx FIFO full trigger level
 *  dw_uart_getRxTrigger        -- get the Rx FIFO full trigger level
 *  dw_uart_getClockDivisor     -- get the clock divisor/baud rate
 *  dw_uart_getLineControl      -- get the line control register value
 *  dw_uart_getDataBits         -- get the number of data bits (5/6/7/8)
 *  dw_uart_getParity           -- get the parity (odd/even/none)
 *  dw_uart_getStopBits         -- get the bumber of stop bits (1/1.5/2)
 *  dw_uart_getBreak            -- get the value of the break bit
 *  dw_uart_getModemLine        -- get the modem line register value
 *  dw_uart_isLoopbackEnabled   -- is loopback mode enabled?
 *  dw_uart_isAfcEnabled        -- is AFC mode enabled?
 *  dw_uart_isSirEnabled        -- is serial infra-red mode enabled?
 *  dw_uart_getScratchpad       -- get the value of the scratchpad
 *  dw_uart_getActiveIrq        -- get the current active interrupt
 *  dw_uart_isIrqEnabled        -- is specified interrupt enabled?
 *  dw_uart_getIrqMask          -- get the current interrupt mask
 *  dw_uart_getFifoDepth        -- get the FIFO depth
 *
 * SEE ALSO
 *  uart.data_types, uart.configuration, uart.command, uart.interrupt,
 *  uart.debug
 ***/

/****h* uart.api/uart.interrupt
 * NAME
 *  DW_apb_uart driver interrupt interface functions
 * DESCRIPTION
 *  dw_uart_setListener         -- set the user listener function
 *  dw_uart_transmit            -- start an interrupt-driven Tx transfer
 *  dw_uart_receive             -- start an interrupt-driven Rx transfer
 *  dw_uart_terminateTx         -- terminate a Tx transfer
 *  dw_uart_terminateRx         -- terminate an Rx transfer
 *  dw_uart_setDmaTxMode        -- set DMA Tx mode (hw/sw handshaking)
 *  dw_uart_getDmaTxMode        -- get the DMA Tx mode
 *  dw_uart_setDmaRxMode        -- set DMA Rx mode (hw/sw handshaking)
 *  dw_uart_getDmaRxMode        -- get the DMA Rx mode
 *  dw_uart_setNotifier_sourceReady         -- set DMA Rx notifier
 *  dw_uart_setNotifier_destinationReady    -- set DMA Tx notifier
 *  dw_uart_setCallbackMode     -- set the Rx callback mode
 *  dw_uart_getCallbackMode     -- get the Rx callback mode
 *  dw_uart_irqHandler          -- device interrupt handler function
 *  dw_uart_userIrqHandler      -- minimal interrupt handler function
 *
 * SEE ALSO
 *  uart.data_types, uart.configuration, uart.command, uart.status,
 *  uart.debug
 ***/

/****h* uart.api/uart.debug
 * NAME
 *  DW_apb_uart driver test/debug functions
 * DESCRIPTION
 *  dw_uart_enableFifoAccess    -- enable FIFO access mode
 *  dw_uart_disableFifoAccess   -- disable FIFO access mode
 *  dw_uart_isFifoAccessEnabled -- is FIFO access mode enabled?
 *  dw_uart_writeRx             -- write to the Rx FIFO
 *  dw_uart_readTx              -- read from the Tx FIFO
 *  dw_uart_haltTx              -- suspend device transfers
 *  dw_uart_resumeTx            -- resume device transfers
 *  dw_uart_isTxHalted          -- are transfer halted?
 *
 * SEE ALSO
 *  uart.data_types, uart.configuration, uart.command, uart.status,
 *  uart.interrupt
 ***/

/****d* uart.data/dw_uart_event
 * DESCRIPTION
 *  This is the data type used for specifying UART events.  An event is
 *  the occurrence of an interrupt which must be handled appropriately.
 *  One of these events is passed at a time to the user listener
 *  function to be processed.  The exception to this are the
 *  Uart_event_thre and Uart_event_timeout interrupts which are handled
 *  internally by the interrupt handler.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - iir/interrupt_id
 * SEE ALSO
 *  dw_uart_setListener(), dw_uart_getActiveIrq()
 * SOURCE
 */
enum dw_uart_event {
    Uart_event_modem = 0x0,     // CTR, DSR, RI or DCD status changed.
    Uart_event_none = 0x1,      // No event/interrupt pending.
    Uart_event_thre = 0x2,      // Transmit holding register empty or TX
                                // FIFO at or below trigger threshold.
    Uart_event_data = 0x4,      // Receive buffer register data
                                // available (non-FIFO mode) or RX FIFO
                                // trigger level reached.
    Uart_event_line = 0x6,      // Overrun/parity/framing error or break
                                // interrupt occurred.
    Uart_event_busy = 0x7,      // Attempt to write to the LCR[7] while
                                // DW_apb_uart was busy (DLAB).
    Uart_event_timeout = 0xc    // No characters in or out of the
                                // receiver FIFO during the last four
                                // character times and there is at least
                                // one character in it during this time.
};
/*****/

/****d* uart.data/dw_uart_irq
 * DESCRIPTION
 *  These are the bit definitions used for managing UART interrupts.
 *  The functionality of ETBEI and ERBFI alters when programmable THRE
 *  interrupt mode is active (dw_uart_enablePtime()).  See the
 *  DW_apb_uart databook for a detailed description.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - ier/erbfi
 *   - ier/etbei
 *   - ier/elsi
 *   - ier/edssi
 * SEE ALSO
 *  dw_uart_enableIrq(), dw_uart_disableIrq(), dw_uart_isIrqEnabled()
 * SOURCE
 */
enum dw_uart_irq {
    Uart_irq_erbfi = 0x01,      // receive data available
    Uart_irq_etbei = 0x02,      // transmitter holding register empty
    Uart_irq_elsi = 0x04,       // receiver line status
    Uart_irq_edssi = 0x08,      // modem status
    Uart_irq_all = 0x0f         // all interrupts
};
/*****/

/****d* uart.data/dw_uart_line_status
 * DESCRIPTION
 *  These are the definitions used for reporting the line status
 *  including errors, if any.  Note that they represent bits of an 8-bit
 *  register and more than one can be active at any time.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - lsr/dr
 *   - lsr/oe
 *   - lsr/pe
 *   - lsr/fe
 *   - lsr/bi
 *   - lsr/thre
 *   - lsr/temt
 *   - lsr/rfe
 * SEE ALSO
 *  dw_uart_getLineStatus()
 * SOURCE
 */
enum dw_uart_line_status {
    Uart_line_dr = 0x01,        // data ready
    Uart_line_oe = 0x02,        // overrun error
    Uart_line_pe = 0x04,        // parity error
    Uart_line_fe = 0x08,        // framing error
    Uart_line_bi = 0x10,        // break interrupt
    Uart_line_thre = 0x20,      // transmit holding register empty
    Uart_line_temt = 0x40,      // transmitter empty
    Uart_line_rfe = 0x80        // receiver FIFO error
};
/*****/

/****d* uart.data/dw_uart_modem_status
 * DESCRIPTION
 *  These are the bit definitions used for reporting the modem status.
 *  Note that they represents bits of an 8-bit register and more than
 *  one can be active at any time.  The delta signals reflect a change
 *  in the relevant pin since the last read of the modem status
 *  register.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - msr/dcts
 *   - msr/ddsr
 *   - msr/teri
 *   - msr/ddcd
 *   - msr/cts
 *   - msr/dsr
 *   - msr/ri
 *   - msr/dcd
 * SEE ALSO
 *  dw_uart_getModemStatus()
 * SOURCE
 */
enum dw_uart_modem_status {
    Uart_modem_dcts = 0x01,     // delta clear to send
    Uart_modem_ddsr = 0x02,     // delta data set ready
    Uart_modem_teri = 0x04,     // ring indicator active->inactive
    Uart_modem_ddcd = 0x08,     // delta data carrier detect
    Uart_modem_cts = 0x10,      // clear to send
    Uart_modem_dsr = 0x20,      // data set ready
    Uart_modem_ri = 0x40,       // ring indicator
    Uart_modem_dcd = 0x80       // data carrier detect
};
/*****/

/****d* uart.data/dw_uart_modem_line
 * DESCRIPTION
 *  These are the bit definitions used for reporting the modem status.
 *  Note that they represents bits of an 4-bit register so one or more
 *  of them may be set/active at any time.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - mcr/dtr
 *   - mcr/rts
 *   - mcr/out1
 *   - mcr/out2
 * SEE ALSO
 *  dw_uart_setModemLine(), dw_uart_clearModemLine(),
 *  dw_uart_getModemLine()
 * SOURCE
 */
enum dw_uart_modem_line {
    Uart_modem_dtr = 0x1,       // data terminal ready
    Uart_modem_rts = 0x2,       // request to send differs since last
                                // read
    Uart_modem_out1 = 0x4,      // user-designated output one
    Uart_modem_out2 = 0x8,      // user-designated output two
    Uart_modem_line_all = 0xf   // all modem control lines
};
/*****/

/****d* uart.data/dw_uart_rx_trigger
 * DESCRIPTION
 *  This is the data type used for the UART receiver FIFO full trigger.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - fcr/rcvr_trigger
 * SEE ALSO
 *  dw_uart_setRxTrigger(), dw_uart_getRxTrigger()
 * SOURCE
 */
enum dw_uart_rx_trigger {
    Uart_one_char_in_fifo = 0x0,
    Uart_fifo_quarter_full = 0x1,
    Uart_fifo_half_full = 0x2,
    Uart_fifo_two_less_full = 0x3
};
/*****/

/****d* uart.data/dw_uart_tx_trigger
 * DESCRIPTION
 *  This is the data type used for the UART transmitter FIFO empty
 *  trigger.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - fcr/tx_empty_trigger
 * SEE ALSO
 *  dw_uart_setTxTrigger(), dw_uart_getTxTrigger()
 * SOURCE
 */
enum dw_uart_tx_trigger {
    Uart_empty_fifo = 0x0,
    Uart_two_chars_in_fifo = 0x1,
    Uart_quarter_full_fifo = 0x2,
    Uart_half_full_fifo = 0x3
};
/*****/

/****d* uart.data/dw_uart_dma_mode
 * DESCRIPTION
 *  This is the data type used for the UART DMA mode.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - fcr/dma_mode
 * SEE ALSO
 *  dw_uart_setDmaMode(), dw_uart_getDmaMode()
 * SOURCE
 */
enum dw_uart_dma_mode {
    Uart_dma_single = 0x0,
    Uart_dma_multi = 0x1
};
/*****/

/****d* uart.data/dw_uart_stop_bits
 * DESCRIPTION
 *  This is the data type used for specifying the number of stop bits.
 *  Note that if that number of bits per character is 5, the actual
 *  number of stop bits transmitted by DW_apb_uart will be either 1
 *  (Uart_one_stop_bit) or 1.5 (Uart_two_stop_bits).
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - lcr/stop
 * SEE ALSO
 *  dw_uart_getStopBits()
 * SOURCE
 */
enum dw_uart_stop_bits {
    Uart_one_stop_bit = 0x0,    // 1 stop bit
    Uart_two_stop_bits = 0x1    // 2 stop bits (or 1.5 stop bits if the
                                // number of bits per character is 5).
};
/*****/

/****d* uart.data/dw_uart_cls
 * DESCRIPTION
 *  This is the data type used for specifying the number of bits per
 *  character (Character Length Select).
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - lcr/cls
 * SEE ALSO
 *  dw_uart_getDataBits()
 * SOURCE
 */
enum dw_uart_cls {
    Uart_five_bits = 0x0,       // 5 bits per character
    Uart_six_bits = 0x1,        // 6 bits per character
    Uart_seven_bits = 0x2,      // 7 bits per character
    Uart_eight_bits = 0x3       // 8 bits per character
};
/*****/

/****d* uart.data/dw_uart_parity
 * DESCRIPTION
 *  This is the data type used for specifying UART parity.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - lcr/pen
 *   - lcr/eps
 * SEE ALSO
 *  dw_uart_getParity()
 * SOURCE
 */
enum dw_uart_parity {
    Uart_no_parity = 0x0,       // no parity
    Uart_odd_parity = 0x1,      // odd parity
    Uart_even_parity = 0x3      // even parity
};
/*****/

/****d* uart.data/dw_uart_line_control
 * DESCRIPTION
 *  This is the data type used for manipulation of the UART line control
 *  settings.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - lcr/cls
 *   - lcr/stop
 *   - lcr/pen
 *   - lcr/eps
 * SEE ALSO
 *  dw_uart_getLineControl(), dw_uart_setLineControl()
 * SOURCE
 */
enum dw_uart_line_control {
    Uart_line_5n1 = 0x00,   // 5 data bits, no parity, 1 stop bit
    Uart_line_5n1_5 = 0x04, // 5 data bits, no parity, 1.5 stop bits
    Uart_line_5e1 = 0x18,   // 5 data bits, even parity, 1 stop bit
    Uart_line_5e1_5 = 0x1c, // 5 data bits, even parity, 1.5 stop bits
    Uart_line_5o1 = 0x08,   // 5 data bits, odd parity, 1 stop bit
    Uart_line_5o1_5 = 0x0c, // 5 data bits, odd parity, 1.5 stop bits
    Uart_line_6n1 = 0x01,   // 6 data bits, no parity, 1 stop bit
    Uart_line_6n2 = 0x05,   // 6 data bits, no parity, 2 stop bits
    Uart_line_6e1 = 0x19,   // 6 data bits, even parity, 1 stop bit
    Uart_line_6e2 = 0x1d,   // 6 data bits, even parity, 2 stop bits
    Uart_line_6o1 = 0x09,   // 6 data bits, odd parity, 1 stop bit
    Uart_line_6o2 = 0x0d,   // 6 data bits, odd parity, 2 stop bits
    Uart_line_7n1 = 0x02,   // 7 data bits, no parity, 1 stop bit
    Uart_line_7n2 = 0x06,   // 7 data bits, no parity, 2 stop bits
    Uart_line_7e1 = 0x1a,   // 7 data bits, even parity, 1 stop bit
    Uart_line_7e2 = 0x1e,   // 7 data bits, even parity, 2 stop bits
    Uart_line_7o1 = 0x0a,   // 7 data bits, odd parity, 1 stop bit
    Uart_line_7o2 = 0x0e,   // 7 data bits, odd parity, 2 stop bits
    Uart_line_8n1 = 0x03,   // 8 data bits, no parity, 1 stop bit
    Uart_line_8n2 = 0x07,   // 8 data bits, no parity, 2 stop bits
    Uart_line_8e1 = 0x1b,   // 8 data bits, even parity, 1 stop bit
    Uart_line_8e2 = 0x1f,   // 8 data bits, even parity, 2 stop bits
    Uart_line_8o1 = 0x0b,   // 8 data bits, odd parity, 1 stop bit
    Uart_line_8o2 = 0x0f    // 8 data bits, odd parity, 2 stop bits
};
/*****/

/****d* uart.data/dw_uart_callback_mode
 * DESCRIPTION
 *  This is the data type used for specifying the Rx callback mode.  The
 *  default behaviour is to call a callback function when the associated
 *  Rx buffer is full.  In addition to this, the Uart_rx_timeout mode
 *  will invoke the callback function for every Uart_event_timeout
 *  interrupt which occurs, with the usual argument of the number of
 *  characters received so far in the Rx buffer.  Note that this is only
 *  relevant when using the Interrupt API.
 * SEE ALSO
 *  dw_uart_setCallbackMode(), dw_uart_getCallbackMode(), uart.interrupt
 * SOURCE
 */
enum dw_uart_callback_mode {
    Uart_buffer_full,   // callback called only when Rx buffer is full
    Uart_rx_timeout     // callback also called for character timeouts
};
/*****/

/****f* uart.functions/dw_uart_init
 * DESCRIPTION
 *  This function initializes a uart.  It disables all interrupts and
 *  resets the Tx and Rx FIFOs.  It also initializes the driver's
 *  internal data structures.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- hardware parameters for the device could not be
 *              automatically determined
 * NOTES
 *  If the DW_apb_uart is independently reset, it must be re-initialized
 *  by calling this function, in order to keep the device and driver in
 *  sync.
 * SEE ALSO
 *  dw_uart_reset()
 * SOURCE
 */
int dw_uart_init(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_isBusy
 * DESCRIPTION
 *  This function returns whether the UART is busy (transmitting and/or
 *  receiving) or not.  If the UART busy bit is unsupported, an error
 *  code is returned.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  true        -- UART is busy
 *  false       -- UART is not busy
 *  -DW_ENOSYS  -- function not supported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - usr/busy
 *
 *  This function is affected by the DW_apb_uart RTL version.  Only
 *  versions >= 3.00 support the busy bit.
 * SOURCE
 */
int dw_uart_isBusy(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_reset
 * DESCRIPTION
 *  This function performs a hardware reset on a DW_apb_uart device.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- function not supported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - srr/ur
 *
 *  This function is affected by the SHADOW hardware parameter.
 *  After resetting the uart, the device driver should be re-initialized
 *  by calling dw_uart_init().
 * SEE ALSO
 *  dw_uart_init()
 * SOURCE
 */
int dw_uart_reset(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_resetTxFifo
 * DESCRIPTION
 *  This funciton resets the transmitter FIFO.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - fcr/xfifor
 *   - srr/xfr
 *
 *  This function is affected by the SHADOW hardware parameter.
 * SEE ALSO
 *  dw_uart_resetRxFifo()
 * SOURCE
 */
void dw_uart_resetTxFifo(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_resetRxFifo
 * DESCRIPTION
 *  This function resets the receiver FIFO.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - fcr/rfifor
 *   - srr/rfr
 *
 *  This function is affected by the SHADOW hardware parameter.
 * SEE ALSO
 *  dw_uart_resetTxFifo()
 * SOURCE
 */
void dw_uart_resetRxFifo(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_setClockDivisor
 * DESCRIPTION
 *  Sets the divisor value used when dividing down sclk, which
 *  determines the baud rate.  See the DW_apb_uart databook for more
 *  information.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  divisor     -- value to set clock divisor
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EBUSY   -- device is busy
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - lcr/dlab
 *   - dll/all bits
 *   - dlh/all bits
 *
 *  A delay is required before using the UART after setting the baud
 *  rate.  The minimum delay needed is 8 clock cycles of the slowest
 *  DW_apb_uart clock.
 * SEE ALSO
 *  dw_uart_getClockDivisor()
 * SOURCE
 */
int dw_uart_setClockDivisor(struct dw_device *dev, uint16_t
        divisor);
/*****/

/****f* uart.functions/dw_uart_getClockDivisor
 * DESCRIPTION
 *  This function returns the clock divisor value.  This is the value
 *  used to divide down sclk, thereby determining the baud rate.  See
 *  the DW_apb_uart databook for more information.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  The clock divisor value.
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - lcr/dlab
 *   - dll/all bits
 *   - dlh/all bits
 *
 *  This function should never be called when the DW_apb_uart is busy as
 *  to do so can result in data loss.  If the value of the divisor is
 *  required while a transfer is in progress, it should be saved to a
 *  local variable before the transfer begins.
 * SEE ALSO
 *  dw_uart_setClockDivisor()
 * SOURCE
 */
uint16_t dw_uart_getClockDivisor(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_setLineControl
 * DESCRIPTION
 *  This function is used to set the parity and the number of data and
 *  stop bits.  The dw_uart_line_control defintions are used to specify
 *  this mode.  The line control settings should not be changed when the
 *  UART is busy.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  mode        -- line control settings
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EBUSY   -- UART is busy
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - lcr/cls
 *   - lcr/stop
 *   - lcr/pen
 *   - lcr/eps
 * SEE ALSO
 *  dw_uart_getLineControl(), dw_uart_line_control, dw_uart_isBusy()
 * SOURCE
 */
int dw_uart_setLineControl(struct dw_device *dev, enum
        dw_uart_line_control mode);
/*****/

/****f* uart.functions/dw_uart_getLineControl
 * DESCRIPTION
 *  Returns the line control settings.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  The current line control settings.
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - lcr/cls
 *   - lcr/stop
 *   - lcr/pen
 *   - lcr/eps
 * SEE ALSO
 *  dw_uart_setLineControl(), dw_uart_line_control
 * SOURCE
 */
enum dw_uart_line_control dw_uart_getLineControl(struct dw_device
        *dev);
/*****/

/****f* uart.functions/dw_uart_setDataBits
 * DESCRIPTION
 *  Sets the number of bits per character (5/6/7/8).
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  cls         -- number of data bits per character
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - lcr/cls
 * SEE ALSO
 *  dw_uart_getDataBits(), dw_uart_cls, dw_uart_setLineControl()
 * SOURCE
 */
void dw_uart_setDataBits(struct dw_device *dev, enum dw_uart_cls
        cls);
/*****/

/****f* uart.functions/dw_uart_getDataBits
 * DESCRIPTION
 *  Returns the number of bits per character setting for data transfers.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  The current number of data bits setting.
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - lcr/cls
 * SEE ALSO
 *  dw_uart_setDataBits(), dw_uart_cls
 * SOURCE
 */
enum dw_uart_cls dw_uart_getDataBits(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_setStopBits
 * DESCRIPTION
 *  Sets the number of stop bits (1/1.5/2).
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  stop        -- number of stop bits
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - lcr/stop
 * SEE ALSO
 *  dw_uart_getStopBits(), dw_uart_stop_bits, dw_uart_setLineControl()
 * SOURCE
 */
void dw_uart_setStopBits(struct dw_device *dev, enum
        dw_uart_stop_bits stop);
/*****/

/****f* uart.functions/dw_uart_getStopBits
 * DESCRIPTION
 *  Returns the number of stop bits setting for data transfers.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  The current number of stop bits setting.
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - lcr/stop
 * SEE ALSO
 *  dw_uart_setStopBits(), dw_uart_stop_bits
 * SOURCE
 */
enum dw_uart_stop_bits dw_uart_getStopBits(struct dw_device
        *dev);
/*****/

/****f* uart.functions/dw_uart_setParity
 * DESCRIPTION
 *  Sets the parity mode (none/odd/even).
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  parity      -- parity to set
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - lcr/pen
 *   - lcr/eps
 * SEE ALSO
 *  dw_uart_getParity(), dw_uart_parity, dw_uart_setLineControl()
 * SOURCE
 */
void dw_uart_setParity(struct dw_device *dev, enum dw_uart_parity
        parity);
/*****/

/****f* uart.functions/dw_uart_getParity
 * DESCRIPTION
 *  Returns the parity setting for data transfers.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  The current parity setting.
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - lcr/pen
 *   - lcr/eps
 * SEE ALSO
 *  dw_uart_setParity(), dw_uart_parity
 * SOURCE
 */
enum dw_uart_parity dw_uart_getParity(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_enableFifos
 * DESCRIPTION
 *  This function enables receive and transmit FIFOs, if they are
 *  available.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- function not supported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - fcr/fifoe
 *   - sfe/fe
 *
 *  This function is affected by the FIFO_MODE and SHADOW hardware
 *  parameter.
 * SEE ALSO
 *  dw_uart_disableFifos(), dw_uart_areFifosEnabled()
 * SOURCE
 */
int dw_uart_enableFifos(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_disableFifos
 * DESCRIPTION
 *  This function disables receive and transmit FIFOs.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- function not supported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - fcr/fifoe
 *   - sfe/fe
 *
 *  This function is affected by the FIFO_MODE and SHADOW hardware
 *  parameter.
 * SEE ALSO
 *  dw_uart_enableFifos(), dw_uart_areFifosEnabled()
 * SOURCE
 */
int dw_uart_disableFifos(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_areFifosEnabled
 * DESCRIPTION
 *  Returns whether the FIFOs and enabled or not.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  true        -- FIFOs are enabled
 *  false       -- FIFOs are disabled or not available
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - iir/fifose
 *   - sfe/fe
 *
 *  This function if affected by the SHADOW hardware parameter.
 * WARNINGS
 *  This function can indirectly clear a THRE interrupt.  When the
 *  SHADOW registers are not available, it reads the IIR register in
 *  order to determine if FIFOs are enabled or not.  Reading the IIR
 *  register clears any pending THRE interrupt.
 * SEE ALSO
 *  dw_uart_enableFifos(), dw_uart_disableFifos()
 * SOURCE
 */
bool dw_uart_areFifosEnabled(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_isTxFifoFull
 * DESCRIPTION
 *  Returns whether the transmitter FIFO is full or not.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  true        -- Tx FIFO is full
 *  false       -- Tx FIFO is not full
 *  -DW_ENOSYS  -- function not supported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - usr/tfnf
 *
 *  This function is affected by the FIFO_STAT hardware parameter.
 * SEE ALSO
 *  dw_uart_isTxFifoEmpty()
 * SOURCE
 */
int dw_uart_isTxFifoFull(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_isTxFifoEmpty
 * DESCRIPTION
 *  Returns whether the transmitter FIFO is empty or not.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  true        -- Tx FIFO is empty
 *  false       -- Tx FIFO is not empty
 *  -DW_ENOSYS  -- function not supported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - usr/tfe
 *
 *  This function is affected by the FIFO_STAT hardware parameter.
 * SEE ALSO
 *  dw_uart_isTxFifoFull()
 * SOURCE
 */
int dw_uart_isTxFifoEmpty(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_isRxFifoFull
 * DESCRIPTION
 *  Returns whether the receiver FIFO is full or not.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  true        -- Rx FIFO is full
 *  false       -- Rx FIFO is not full
 *  -DW_ENOSYS  -- function not supported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - usr/rff
 *
 *  This function is affected by the FIFO_STAT hardware parameter.
 * SEE ALSO
 *  dw_uart_isRxFifoEmpty()
 * SOURCE
 */
int dw_uart_isRxFifoFull(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_isRxFifoEmpty
 * DESCRIPTION
 *  This function returns whether the receiver FIFO is empty or not.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  true        -- Rx FIFO is empty
 *  false       -- Rx FIFO is not empty
 *  -DW_ENOSYS  -- function not supported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - usr/rfne
 *
 *  This function is affected by the FIFO_STAT hardware parameter.
 * SEE ALSO
 *  dw_uart_isRxFifoFull()
 * SOURCE
 */
int dw_uart_isRxFifoEmpty(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_getTxFifoLevel
 * DESCRIPTION
 *  This function returns the number of characters currently present in
 *  the Tx FIFO.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  -DW_ENOSYS  -- function not supported
 *  Otherwise number of characters currently in the Tx FIFO is returned.
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - tfl/all bits
 *
 *  This function is affected by the FIFO_STAT hardware parameter.
 * SEE ALSO
 *  dw_uart_getRxFifoLevel()
 * SOURCE
 */
int dw_uart_getTxFifoLevel(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_getRxFifoLevel
 * DESCRIPTION
 *  This function returns the number of characters currently present in
 *  the Rx FIFO.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  -DW_ENOSYS  -- function not supported
 *  Otherwise number of characters currently in the Rx FIFO is returned.
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - rfl/all bits
 *
 *  This function is affected by the FIFO_STAT hardware parameter.
 * SEE ALSO
 *  dw_uart_getTxFifoLevel()
 * SOURCE
 */
int dw_uart_getRxFifoLevel(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_getFifoDepth
 * DESCRIPTION
 *  Returns how many bytes deep the transmitter and receiver FIFOs are.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  FIFO depth in bytes (from 0 to 2048)
 * NOTES
 *  This function is affected by the FIFO_MODE hardware parameter.
 * SOURCE
 */
unsigned dw_uart_getFifoDepth(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_enablePtime
 * DESCRIPTION
 *  This function enables Programmable Threshold (THRE) Interrupt Mode
 *  (PTIME).  This mode enables triggering of interrupts for different
 *  levels of the Tx/Rx FIFOs.  Enabling PTIME also changes the
 *  functionality of the lsr/thre bit (LSR[5]) to indicate that the Tx
 *  FIFO is full.  See the DW_apb_uart databook for more information.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- function not supported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - ier/ptime
 *
 *  This function is affected by the THRE_MODE hardware parameter.
 * WARNINGS
 *  FIFOs must also be enabled in order to use the Programmable THRE
 *  Interrupt Mode.
 * SEE ALSO
 *  dw_uart_disablePtime(), dw_uart_isPtimeEnabled()
 * SOURCE
 */
int dw_uart_enablePtime(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_disablePtime
 * DESCRIPTION
 *  This function disables Programmable Threshold (THRE) Interrupt Mode
 *  (PTIME).  When PTIME is disabled, the functionality of the lsr/thre
 *  bit (LSR[5]) is normal, indicating that the Tx FIFO/THR is empty.
 *  See the DW_apb_uart databook for more information.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- function not supported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - ier/ptime
 *
 *  This function is affected by the THRE_MODE hardware parameter.
 * SEE ALSO
 *  dw_uart_enablePtime(), dw_uart_isPtimeEnabled()
 * SOURCE
 */
int dw_uart_disablePtime(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_isPtimeEnabled
 * DESCRIPTION
 *  This function returns whether Programmable Threshold (THRE)
 *  Interrupt Mode (PTIME) is enabled or not.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  true        -- PTIME is enabled
 *  false       -- PTIME is disabled
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - ier/ptime
 * SEE ALSO
 *  dw_uart_enablePtime(), dw_uart_disablePtime()
 * SOURCE
 */
bool dw_uart_isPtimeEnabled(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_setBreak
 * DESCRIPTION
 *  Sets the break control bit to 'state'.  When enabled, it causes a
 *  break signal to be generated, by holding the sout line low, until
 *  the break bit is subsequently cleared (with this function).
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  state       -- Set or Clear
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - lcr/break
 *   - sbcr/bcr
 *
 *  This function is affected by the SHADOW hardware parameter.
 * SEE ALSO
 *  dw_uart_getBreak(), dw_state
 * SOURCE
 */
void dw_uart_setBreak(struct dw_device *dev, enum dw_state state);
/*****/

/****f* uart.functions/dw_uart_getBreak
 * DESCRIPTION
 *  Returns the state of the break control bit.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  The current break bit state.
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - lcr/break
 *   - sbcr/bcr
 *
 *  This function is affected by the SHADOW hardware parameter.
 * SEE ALSO
 *  dw_uart_setBreak(), dw_state
 * SOURCE
 */
enum dw_state dw_uart_getBreak(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_setModemLine
 * DESCRIPTION
 *  This function is used to Set specific modem lines.  The lines
 *  argument comprises of one or more bitwise OR'ed dw_uart_modem_line
 *  enumerated values.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  lines       -- modem line(s) to Set
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - mcr/dtr
 *   - mcr/rts
 *   - mcr/out1
 *   - mcr/out2
 * SEE ALSO
 *  dw_uart_clearModemLine(), dw_uart_getModemLine(), dw_uart_modem_line
 * SOURCE
 */
void dw_uart_setModemLine(struct dw_device *dev, enum
        dw_uart_modem_line lines);
/*****/

/****f* uart.functions/dw_uart_clearModemLine
 * DESCRIPTION
 *  This function is used to Clear specific modem lines.  The lines
 *  argument comprises of one or more bitwise OR'ed dw_uart_modem_line
 *  enumerated values.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  lines       -- modem line(s) to Clear
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - mcr/dtr
 *   - mcr/rts
 *   - mcr/out1
 *   - mcr/out2
 * SEE ALSO
 *  dw_uart_setModemLine(), dw_uart_getModemLine(), dw_uart_modem_line
 * SOURCE
 */
void dw_uart_clearModemLine(struct dw_device *dev, enum
        dw_uart_modem_line lines);
/*****/

/****f* uart.functions/dw_uart_getModemLine
 * DESCRIPTION
 *  This function returns the state of the modem control lines.  The
 *  dw_uart_modem_line enumerated values are used with this function's
 *  return value to determine the current state of the modem lines.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  The current value on the modem line control settings.
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - mcr/dtr
 *   - mcr/rts
 *   - mcr/out1
 *   - mcr/out2
 * SEE ALSO
 *  dw_uart_setModemLine(), dw_uart_clearModemLine(), dw_uart_modem_line
 * SOURCE
 */
enum dw_uart_modem_line dw_uart_getModemLine(struct dw_device
        *dev);
/*****/

/****f* uart.functions/dw_uart_enableLoopback
 * DESCRIPTION
 *  Enables loopback mode.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - mcr/loopback
 * SEE ALSO
 *  dw_uart_disableLoopback(), dw_uart_isLoopbackEnabled()
 * SOURCE
 */
void dw_uart_enableLoopback(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_disableLoopback
 * DESCRIPTION
 *  Disables loopback mode.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - mcr/loopback
 * SEE ALSO
 *  dw_uart_enableLoopback(), dw_uart_isLoopbackEnabled()
 * SOURCE
 */
void dw_uart_disableLoopback(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_isLoopbackEnabled
 * DESCRIPTION
 *  Returns whether loopback mode is enabled or not.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  true        -- loopback mode is enabled
 *  false       -- loopback mode is disabled
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - mcr/loopback
 * SEE ALSO
 *  dw_uart_enableLoopback(), dw_uart_disableLoopback()
 * SOURCE
 */
bool dw_uart_isLoopbackEnabled(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_enableAfc
 * DESCRIPTION
 *  Enables Automatic Flow Control mode.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- function not supported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - mcr/afce
 *
 *  This function is affected by the AFCE_MODE hardware parameter.
 * SEE ALSO
 *  dw_uart_disableAfc(), dw_uart_isAfcEnabled()
 * SOURCE
 */
int dw_uart_enableAfc(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_disableAfc
 * DESCRIPTION
 *  Disables Automatic Flow Control mode.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- function not supported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - mcr/afce
 *
 *  This function is affected by the AFCE_MODE hardware parameter.
 * SEE ALSO
 *  dw_uart_enableAfc(), dw_uart_isAfcEnabled()
 * SOURCE
 */
int dw_uart_disableAfc(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_isAfcEnabled
 * DESCRIPTION
 *  Returns whether Automatic Flow Control mode is enabled or not.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  true        -- Automatic Flow Control is enabled
 *  false       -- Automatic Flow Control is disabled
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - mcr/afce
 *
 *  This function is affected by the AFCE_MODE hardware parameter.
 * SEE ALSO
 *  dw_uart_enableAfc(), dw_uart_disableAfc()
 * SOURCE
 */
bool dw_uart_isAfcEnabled(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_enableSir
 * DESCRIPTION
 *  Enables Serial Infra-Red mode.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- function not supported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - mcr/sire
 *
 *  This function is affected by the SIR_MODE hardware parameter.
 * SEE ALSO
 *  dw_uart_disableSir(), dw_uart_isSirEnabled()
 * SOURCE
 */
int dw_uart_enableSir(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_disableSir
 * DESCRIPTION
 *  Disables Serial Infra-Red mode.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- function not supported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - mcr/sire
 *
 *  This function is affected by the SIR_MODE hardware parameter.
 * SEE ALSO
 *  dw_uart_enableSir(), dw_uart_isSirEnabled()
 * SOURCE
 */
int dw_uart_disableSir(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_isSirEnabled
 * DESCRIPTION
 *  Returns whether Serial Infra-Red mode is enabled or not.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  true        -- Serial Infra-Red mode is enabled
 *  false       -- Serial Infra-Red mode is disabled
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - mcr/sire
 *
 *  This function is affected by the SIR_MODE hardware parameter.
 * SEE ALSO
 *  dw_uart_enableSir(), dw_uart_disableSir()
 * SOURCE
 */
bool dw_uart_isSirEnabled(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_getLineStatus
 * DESCRIPTION
 *  This function returns the current line status register value.  This
 *  value is used in conjunction with the dw_uart_line_status enumerated
 *  values to determine the current line status.  See the DW_apb_uart
 *  databook for more information about the line status register.
 *
 *  Reading the line status register can reset the OE, PE, FE and RFE
 *  error bits so these should always be checked to avoid overlooking an
 *  error.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  The current line status register value.
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - lsr/all bits
 *
 *  When using the Interrupt API and the FIFO status registers are not
 *  available, the line status register is read to determine when the
 *  Tx/Rx FIFOs are full/empty.  As this can inadvertently reset an
 *  error bit, any active error bits are temporarily saved.  These bits
 *  are OR'ed with the return value of this function when it is called.
 *
 *  This is to ensure that no errors are missed.  Also note that this
 *  means that reported errors are not necessarily associated with the
 *  character at the top of the Rx FIFO, when using the Interrupt API.
 * SEE ALSO
 *  dw_uart_line_status
 * SOURCE
 */
enum dw_uart_line_status dw_uart_getLineStatus(struct dw_device
        *dev);
/*****/

/****f* uart.functions/dw_uart_getModemStatus
 * DESCRIPTION
 *  This function returns the current modem status register value.  This
 *  value is used in conjunction with the dw_uart_modem_status
 *  enumerated values to determine the current modem status.  See the
 *  DW_apb_uart databook for more information about the modem status
 *  register.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  The current modem status register value.
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - msr/all bits
 * SEE ALSO
 *  dw_uart_modem_status
 * SOURCE
 */
enum dw_uart_modem_status dw_uart_getModemStatus(struct dw_device
        *dev);
/*****/

/****f* uart.functions/dw_uart_setScratchpad
 * DESCRIPTION
 *  Sets the value of the scratchpad register.  This register has no
 *  functional use and is available to a programmer to use at their own
 *  discretion.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  byte        -- value to set
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - scr/all bits
 * SEE ALSO
 *  dw_uart_getScratchpad()
 * SOURCE
 */
void dw_uart_setScratchpad(struct dw_device *dev, uint8_t byte);
/*****/

/****f* uart.functions/dw_uart_getScratchpad
 * DESCRIPTION
 *  Returns the value of the scratchpad register.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  The current scratchpad register value.
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - scr/all bits
 * SEE ALSO
 *  dw_uart_setScratchpad()
 * SOURCE
 */
uint8_t dw_uart_getScratchpad(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_setDmaMode
 * DESCRIPTION
 *  Sets the DMA mode and signalling used by the UART, either single- or
 *  multi- transfer.  This function has no effect when the DMA_EXTRA
 *  hardware parameter is set to true.  See the DW_apb_uart databook for
 *  more information.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  mode        -- DMA mode to set
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - fcr/dmam
 *   - sdmam/mode
 *
 *  This function is affected by the SHADOW hardware parameter.
 * SEE ALSO
 *  dw_uart_getDmaMode(), dw_uart_dma_mode
 * SOURCE
 */
void dw_uart_setDmaMode(struct dw_device *dev, enum
        dw_uart_dma_mode mode);
/*****/

/****f* uart.functions/dw_uart_getDmaMode
 * DESCRIPTION
 *  Returns the DMA mode setting of the UART.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  The DMA mode setting of the DW_apb_uart device.
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - fcr/dmam
 *   - sdmam/mode
 *
 *  This function is affected by the SHADOW hardware parameter.
 * SEE ALSO
 *  dw_uart_setDmaMode(), dw_uart_dma_mode
 * SOURCE
 */
enum dw_uart_dma_mode dw_uart_getDmaMode(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_dmaSwAck
 * DESCRIPTION
 *  This function is used to perform a DMA software acknowledge, if a
 *  transfer needs to be terminated due to an error condition.  See the
 *  DW_apb_uart databook for more information.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- function not supported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - dmasa/ack
 *
 *  This function is affected by the DMA_EXTRA hardware parameter.
 * SOURCE
 */
int dw_uart_dmaSwAck(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_read
 * DESCRIPTION
 *  Reads a single character from the receiver FIFO and returns it.
 *  This function does not check if there is data in the Rx FIFO
 *  beforehand, which is a user responsibility.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  The character read from the Rx FIFO.
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - rbr/all bits
 * SEE ALSO
 *  dw_uart_write(), dw_uart_burstRead()
 * SOURCE
 */
uint8_t dw_uart_read(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_write
 * DESCRIPTION
 *  Writes a single character to the transmitter FIFO.  This function
 *  does not check if there is space in the Tx FIFO beforehand, which is
 *  a user responsibility.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  character   -- character to write
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - thr/all bits
 * SEE ALSO
 *  dw_uart_read(), dw_uart_burstWrite()
 * SOURCE
 */
void dw_uart_write(struct dw_device *dev, uint8_t character);
/*****/

/****f* uart.functions/dw_uart_burstRead
 * DESCRIPTION
 *  This function reads characters from the Rx FIFO, using burst
 *  transactions on the AHB bus.  This function does not check if there
 *  is enough valid data in the Rx FIFO beforehand, which is a user
 *  responsibility.  The length argument should never exceed the FIFO
 *  depth.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  buffer      -- buffer to which data is stored
 *  length      -- number of characters to read
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- function not supported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - srbr/all bits
 *
 *  This function is affected by the SIR_MODE hardware parameter.
 *
 *  The ANSI C memcpy() function is used to read from the shadow receive
 *  buffer registers in bursts of four characters.  When compiled using
 *  the ARM compiler, this function is replaced with efficient LDM/STM
 *  assembler instructions, thus improving AHB bus usage.
 * SEE ALSO
 *  dw_uart_burstWrite(), dw_uart_read()
 * SOURCE
 */
int dw_uart_burstRead(struct dw_device *dev, uint8_t *buffer,
        unsigned length);
/*****/

/****f* uart.functions/dw_uart_burstWrite
 * DESCRIPTION
 *  This function writes a characters to the Tx FIFO, using burst
 *  transfers on the AHB bus.  This function does not check if there is
 *  sufficient spave available in the Tx FIFO beforehand, which is a
 *  user responsibility.  The length argument should never exceed the
 *  FIFO depth.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  buffer      -- buffer from which to send data
 *  length      -- number of characters to write
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- function not supported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - sthr/all bits
 *
 *  This function is affected by the SIR_MODE hardware parameter.
 *  The ANSI C memcpy() function to write to the shadow transmit hold
 *  registers in bursts of four characters.  When compiled using the ARM
 *  compiler, this function is replaced with efficient LDM/STM assembler
 *  instructions, thus improving AHB bus usage.
 * SEE ALSO
 *  dw_uart_burstRead(), dw_uart_write()
 * SOURCE
 */
int dw_uart_burstWrite(struct dw_device *dev, uint8_t *buffer,
        unsigned length);
/*****/

/****f* uart.functions/dw_uart_enableIrq
 * DESCRIPTION
 *  Enables specified interrupt(s).
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  interrupts  -- interrupt(s) to enable
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - ier/all bits
 * SEE ALSO
 *  dw_uart_disableIrq(), dw_uart_isIrqEnabled()
 * SOURCE
 */
void dw_uart_enableIrq(struct dw_device *dev, enum dw_uart_irq
        interrupts);
/*****/

/****f* uart.functions/dw_uart_disableIrq
 * DESCRIPTION
 *  Disables specified interrupt(s).
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  interrupts  -- interrupt(s) to disable
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - ier/all bits
 * SEE ALSO
 *  dw_uart_enableIrq(), dw_uart_isIrqEnabled()
 * SOURCE
 */
void dw_uart_disableIrq(struct dw_device *dev, enum dw_uart_irq
        interrupts);
/*****/

/****f* uart.functions/dw_uart_isIrqEnabled
 * DESCRIPTION
 *  Returns whether the specified interrupt is enabled or not.  Only one
 *  interrupt may be specifed per invocation of this function.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  interrupt   -- interrupt to check
 * RETURN VALUE
 *  true        -- interrupt is enabled
 *  false       -- interrupt is disabled
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - ier/all bits
 * SEE ALSO
 *  dw_uart_enableIrq(), dw_uart_disableIrq(), dw_uart_getIrqMask()
 * SOURCE
 */
bool dw_uart_isIrqEnabled(struct dw_device *dev, enum dw_uart_irq
        interrupt);
/*****/

/****f* uart.functions/dw_uart_getIrqMask
 * DESCRIPTION
 *  Returns the current interrupt mask.  For each bitfield, a value of
 *  '0' indicates that an interrupt is masked while a value of '1'
 *  indicates that an interrupt is enabled.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  The DW_apb_uart interrupt mask.
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - ier/all bits
 * SEE ALSO
 *  dw_uart_enableIrq(), dw_uart_disableIrq(), dw_uart_isIrqEnabled()
 * SOURCE
 */
uint8_t dw_uart_getIrqMask(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_getActiveIrq
 * DESCRIPTION
 *  Returns the event identification number of the highest priority
 *  interrupt that is active.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  The current highest priority active interrupt.
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - iir/iid
 * WARNINGS
 *  This function can indirectly clear a THRE interrupt.  Reading the
 *  IIR register clears any pending THRE interrupt.
 * SEE ALSO
 *  dw_uart_event
 * SOURCE
 */
enum dw_uart_event dw_uart_getActiveIrq(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_setTxTrigger
 * DESCRIPTION
 *  Sets the trigger level of the transmitter FIFO empty interrupt.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  level       -- level at which to set trigger
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - fcr/tet
 *   - stet/trigger
 *
 *  This function is affected by the SHADOW hardware parameter.
 * SEE ALSO
 *  dw_uart_getTxTrigger(), dw_uart_tx_trigger, dw_uart_setRxTrigger()
 * SOURCE
 */
void dw_uart_setTxTrigger(struct dw_device *dev, enum
        dw_uart_tx_trigger level);
/*****/

/****f* uart.functions/dw_uart_getTxTrigger
 * DESCRIPTION
 *  Gets the trigger level of the transmitter FIFO empty interrupt.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  transmitter trigger level
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - fcr/tet
 *   - stet/trigger
 *
 *  This function is affected by the SHADOW hardware parameter.
 * SEE ALSO
 *  dw_uart_setTxTrigger(), dw_uart_tx_trigger
 * SOURCE
 */
enum dw_uart_tx_trigger dw_uart_getTxTrigger(struct dw_device
        *dev);
/*****/

/****f* uart.functions/dw_uart_setRxTrigger
 * DESCRIPTION
 *  Sets the trigger level for the receiver FIFO full interrupt.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  level       -- level at which to set trigger
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - fcr/rt
 *   - srt/trigger
 *
 *  This function is affected by the SHADOW hardware parameter.
 * SEE ALSO
 *  dw_uart_getRxTrigger(), dw_uart_rx_trigger, dw_uart_setTxTrigger()
 * SOURCE
 */
void dw_uart_setRxTrigger(struct dw_device *dev, enum
        dw_uart_rx_trigger level);
/*****/

/****f* uart.functions/dw_uart_getRxTrigger
 * DESCRIPTION
 *  Gets the trigger level of the receiver FIFO full interrupt.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  The receiver empty trigger level.
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - fcr/rt
 *   - srt/trigger
 *
 *  This function is affected by the SHADOW hardware parameter.
 * SEE ALSO
 *  dw_uart_setRxTrigger(), dw_uart_rx_trigger
 * SOURCE
 */
enum dw_uart_rx_trigger dw_uart_getRxTrigger(struct dw_device
        *dev);
/*****/

/****f* uart.functions/dw_uart_setListener
 * DESCRIPTION
 *  This function is used to set a user listener callback.  The listener
 *  function is responsible for handling all events/interrupts that are
 *  not handled internally by the Driver Kit.  This encompasses events
 *  such as errors or receiving data when there is no user Rx buffer set
 *  up.  In this respect, it can be considered as a way of extending the
 *  default interrupt handler.
 *
 *  A listener must be setup up before using any of the other functions
 *  of the Interrupt API.  Note that if the dw_uart_userIrqHandler()
 *  interrupt handler is being used, none of the other Interrupt API
 *  functions can be used with it.  This is because they are symbiotic
 *  with the dw_uart_irqHandler() interrupt handler.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  listener    -- user listener function
 * RETURN VALUE
 *  none
 * NOTES
 *  This function enables the following interrupts:
 *  Uart_irq_erbfi
 *  Uart_irq_elsi
 *
 *  Whether the dw_uart_userIrqHandler() or dw_uart_irqHandler()
 *  interrupt  handler is being used, this function is used to set the
 *  user listener function that is called by both of them.
 * EXAMPLE
 *  In the case of new data being received in the Rx FIFO, the irq
 *  handler (e.g. dw_uart_irqHandler()) would call the listener function
 *  thus:
 *  
 *  userListener(dev, Uart_event_data);
 *  
 *  It is the listener's responsibility to properly handle this, e.g.:
 *
 *  dw_uart_receive(dev, buffer, length, callback);
 * SEE ALSO
 *  dw_uart_irqHandler(), dw_uart_userIrqHandler(), dw_uart_event,
 *  dw_callback
 * SOURCE
 */
void dw_uart_setListener(struct dw_device *dev, dw_callback listener);
/*****/

/****f* uart.functions/dw_uart_transmit
 * DESCRIPTION
 *  This function starts an interrupt-driven Tx transfer.  This function
 *  fills the THR/Tx FIFO and, if there is more data to send, sets up
 *  and enables the Tx interrupt to keep the Tx FIFO filled.  Upon
 *  completion, the callback function is called (if it is not NULL).
 *
 *  A transfer can be stopped at any time by calling
 *  dw_uart_terminateTx(), which returns how many characters were sent
 *  before the transfer was terminated.  A terminated transfer's
 *  callback function is never called.  A listener function must first
 *  be setup before using any of the Interrupt API functions.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  buffer      -- buffer from which to send data
 *  length      -- length of buffer/number of bytes to send
 *  callback    -- function to call when transfer is complete (optional)
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EBUSY   -- Tx transfer is already in progress
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - thr/all bits
 *   - ier/all bits
 *
 *  This function (along with dw_uart_IrqHandler()) manages the proper
 *  enabling/disabling of interrupts to avoid shared data issues.
 *
 *  This function is part of the Interrupt API and should not be called
 *  when using the UART in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_uart_irqHandler().
 * SEE ALSO
 *  dw_uart_receive(), dw_uart_terminateTx(), dw_uart_setListener(),
 *  dw_callback
 * SOURCE
 */
int dw_uart_transmit(struct dw_device *dev, void *buffer, unsigned
        length, dw_callback callback);
/*****/

/****f* uart.functions/dw_uart_receive
 * DESCRIPTION
 *  This function starts an interrupt-driven Rx transfer.  This function
 *  sets up and enables the Rx interrupt to fill the buffer from the
 *  RBR/Rx FIFO.  Upon completion, the callback function is called (if
 *  it is not NULL).  It is also to have the callback function called
 *  upon every occurrence of a character timeout interrupt.  See
 *  dw_uart_setCallbackMode() for more information.
 *
 *  A transfer can be stopped at any time by calling
 *  dw_uart_terminateRx(), which returns how many characters were stored
 *  in the Rx buffer.  A terminated transfer's callback function is
 *  never called.  A listener function must first be setup before using
 *  any of the Interrupt API functions.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  buffer      -- buffer to write received data to
 *  length      -- length of buffer/max number of bytes to receive
 *  callback    -- function to call when transfer is complete (optional)
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EBUSY   -- Rx transfer is already in progress
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - rbr/all bits
 *   - ier/all bits
 *
 *  This function (along with dw_uart_IrqHandler()) manages the proper
 *  enabling/disabling of interrupts to avoid shared data issues.
 *
 *  This function is part of the Interrupt API and should not be called
 *  when using the UART in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_uart_irqHandler().
 * SEE ALSO
 *  dw_uart_transmit(), dw_uart_terminateRx(), dw_uart_setListener(),
 *  dw_callback, dw_uart_setCallbackMode()
 * SOURCE
 */
int dw_uart_receive(struct dw_device *dev, void *buffer, unsigned
        length, dw_callback callback);
/*****/

/****f* uart.functions/dw_uart_setCallbackMode
 * DESCRIPTION
 *  This function is used to set the Rx callback mode.  By default, a
 *  receiver callback function is called when the user Rx buffer is
 *  full.  This behaviour can be changed, though, to also invoke the
 *  callback whenever the Uart_event_timeout (character timeout)
 *  interrupt is triggered.  In this instance, the callback function is
 *  passed, as an argument, the number of bytes received and stored in
 *  the Rx buffer so far.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  mode        -- receive callback mode to set
 * RETURN VALUE
 *  none
 * NOTES
 *  This function is part of the Interrupt API and should not be called
 *  when using the UART in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_uart_irqHandler().
 * SEE ALSO
 *  dw_uart_getCallbackMode(), dw_uart_callback_mode, dw_uart_receive()
 * SOURCE
 */
void dw_uart_setCallbackMode(struct dw_device *dev, enum
        dw_uart_callback_mode mode);
/*****/

/****f* uart.functions/dw_uart_getCallbackMode
 * DESCRIPTION
 *  Returns the current receive callback mode setting.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  The current callback mode
 * NOTES
 *  This function is part of the Interrupt API and should not be called
 *  when using the UART in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_uart_irqHandler().
 * SEE ALSO
 *  dw_uart_setCallbackMode(), dw_uart_callback_mode
 * SOURCE
 */
enum dw_uart_callback_mode dw_uart_getCallbackMode(struct dw_device
        *dev);
/*****/

/****f* uart.functions/dw_uart_terminateTx
 * DESCRIPTION
 *  This function terminates the current interrupt-driven Tx transfer in
 *  progress.  If there is no Tx transfer in progress, this function has
 *  no effect.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  -DW_EPERM   -- no Tx transfer is currently in progress
 *  Otherwise the number of bytes sent before transfer was terminated is
 *  returned.
 * NOTES
 *  This function is part of the Interrupt API and should not be called
 *  when using the UART in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_uart_irqHandler().
 * SEE ALSO
 *  dw_uart_terminateRx()
 * SOURCE
 */
int32_t dw_uart_terminateTx(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_terminateRx
 * DESCRIPTION
 *  This function terminates the current interrupt-driven Rx transfer in
 *  progress.  If there is no Rx transfer in progress, this function has
 *  no effect.  Before a transfer is terminated, the Rx FIFO is flushed
 *  to the user Rx buffer.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  -DW_EPERM   -- no Rx transfer is currently in progress
 *  Otherwise the number of bytes stored in the Rx buffer after the
 *  transfer was terminated is returned.
 * NOTES
 *  This function is part of the Interrupt API and should not be called
 *  when using the UART in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_uart_irqHandler().
 * SEE ALSO
 *  dw_uart_terminateTx()
 * SOURCE
 */
int32_t dw_uart_terminateRx(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_setDmaTxMode
 * DESCRIPTION
 *  This function is used to set the DMA mode for transmitting data.
 *  Possible options are none (DMA disabled), software or hardware
 *  handshaking.
 *
 *  For software handshaking, a Tx notifier function (used to notify the
 *  DMA that the UART is ready to accept more data) must first be setup
 *  by calling the dw_uart_setNotifier_destinationReady() function.  An
 *  assertion within this function will fail otherwise.
 *
 *  The Tx empty interrupt is disabled for hardware handshaking and
 *  enabled (and managed internally by the driver) for software
 *  handshaking or when the DMA mode is set to none.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  mode        -- DMA mode to use for Tx transfers
 * RETURN VALUE
 *  none
 * NOTES
 *  This function is part of the Interrupt API and should not be called
 *  when using the UART in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_uart_irqHandler().
 * SEE ALSO
 *  dw_uart_getDmaTxMode(), dw_uart_setDmaRxMode(), dw_dma_mode
 * SOURCE
 */
void dw_uart_setDmaTxMode(struct dw_device *dev, enum dw_dma_mode mode);
/*****/

/****f* uart.functions/dw_uart_getDmaTxMode
 * DESCRIPTION
 *  This function returns the current DMA mode, if any, being used to
 *  transmit data.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  The current Tx DMA mode.
 * NOTES
 *  This function is part of the Interrupt API and should not be called
 *  when using the UART in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_uart_irqHandler().
 * SEE ALSO
 *  dw_uart_setDmaTxMode(), dw_uart_setDmaRxMode(), dw_dma_mode
 * SOURCE
 */
enum dw_dma_mode dw_uart_getDmaTxMode(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_setDmaRxMode
 * DESCRIPTION
 *  This function is used to set the DMA mode for receiving data.
 *  Possible options are none (DMA disabled), software or hardware
 *  handshaking.
 *
 *  For software handshaking, an Rx notifier function (used to notify
 *  the DMA that the UART has data ready for transfer) must first be
 *  setup by calling the dw_uart_setNotifier_sourceReady() function.  An
 *  assertion within this function will fail otherwise.
 *
 *  The Rx full interrupt is disabled for hardware handshaking and
 *  enabled for software handshaking or when the DMA mode is set to
 *  none.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  mode        -- DMA mode to use for Rx transfers
 * RETURN VALUE
 *  none
 * NOTES
 *  This function is part of the Interrupt API and should not be called
 *  when using the UART in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_uart_irqHandler().
 * SEE ALSO
 *  dw_uart_getDmaRxMode(), dw_uart_setDmaTxMode(), dw_dma_mode
 * SOURCE
 */
void dw_uart_setDmaRxMode(struct dw_device *dev, enum dw_dma_mode mode);
/*****/

/****f* uart.functions/dw_uart_getDmaRxMode
 * DESCRIPTION
 *  This function returns the current DMA mode, if any, being used to
 *  receive data.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  The current Rx DMA mode.
 * NOTES
 *  This function is part of the Interrupt API and should not be called
 *  when using the UART in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_uart_irqHandler().
 * SEE ALSO
 *  dw_uart_setDmaRxMode(), dw_uart_setDmaTxMode(), dw_dma_mode
 * SOURCE
 */
enum dw_dma_mode dw_uart_getDmaRxMode(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_setNotifier_sourceReady
 * DESCRIPTION
 *  This function sets the user DMA Rx notifier function.  This function
 *  is required when the DMA Rx mode is software handshaking.  The UART
 *  driver calls this function at a predefined threshold to inform the
 *  DMA that data is ready to be read from the UART Rx FIFO.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  funcptr     -- function pointer to notifier function
 *  dmac        -- associated DW_ahb_dmac device handle
 *  channel     -- associated DMA channel number
 * RETURN VALUE
 *  none
 * NOTES
 *  This function is part of the Interrupt API and should not be called
 *  when using the UART in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_uart_irqHandler().
 * SEE ALSO
 *  dw_uart_setDmaTxMode(), dw_uart_getDmaRxMode(), dw_dma_notifier_func
 * SOURCE
 */
void dw_uart_setNotifier_sourceReady(struct dw_device *dev,
        dw_dma_notifier_func funcptr, struct dw_device *dmac, unsigned
        channel);
/*****/

/****f* uart.functions/dw_uart_setNotifier_destinationReady
 * DESCRIPTION
 *  This function sets the user DMA Tx notifier function.  This function
 *  is required when the DMA Tx mode is software handshaking.  The UART
 *  driver calls this function at a predefined threshold to request the
 *  DMA to send more data to the UART Tx FIFO.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  funcptr     -- function pointer to notifier function
 *  dmac        -- associated DW_ahb_dmac device handle
 *  channel     -- associated DMA channel number
 * RETURN VALUE
 *  none
 * NOTES
 *  This function is part of the Interrupt API and should not be called
 *  when using the UART in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_uart_irqHandler().
 * SEE ALSO
 *  dw_uart_setDmaTxMode(), dw_uart_getDmaRxMode(), dw_dma_notifier_func
 * SOURCE
 */
void dw_uart_setNotifier_destinationReady(struct dw_device *dev,
        dw_dma_notifier_func funcptr, struct dw_device *dmac, unsigned
        channel);
/*****/

/****f* uart.functions/dw_uart_irqHandler
 * DESCRIPTION
 *  This function handles and processes UART interrupts.  It works in
 *  conjunction with dw_uart_transmit(), dw_uart_receive() and the user
 *  listener function to manage interrupt-driven transfers.  When fully
 *  using the Interrupt API, this function should be called whenever a
 *  DW_apb_uart interrupt occurs.  There is an alternate interrupt
 *  handler available, dw_uart_userIrqHandler(), but this cannot be used
 *  in conjunction with the other Interrupt API functions.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  true        -- an interrupt was processed
 *  false       -- no interrupt was processed
 *  -DW_EIO     -- unrecognized interrupt ID was read
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - iir/all bits
 *   - fcr/all bits
 *   - lsr/all bits
 *   - thr/all bits
 *   - rbr/all bits
 *
 *  This function is part of the Interrupt API and should not be called
 *  when using the UART in a poll-driven manner.
 * WARNINGS
 *  The user listener function is run in interrupt context and, as such,
 *  care must be taken to ensure that any data shared between it and
 *  normal code is adequately protected from corruption.  Depending on
 *  the target platform, spinlocks, mutexes or semaphores may be used to
 *  achieve this.  The other Interrupt API functions disable UART
 *  interrupts before entering critical sections of code to avoid any
 *  shared data issues.
 * SEE ALSO
 *  dw_uart_transmit(), dw_uart_receive(), dw_uart_setDmaTxMode(),
 *  dw_uart_setDmaRxMode(), dw_uart_terminateTx(), dw_uart_terminateRx()
 * SOURCE
 */
int dw_uart_irqHandler(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_userIrqHandler
 * DESCRIPTION
 *  This function identifies the current highest priority active
 *  interrupt, if any, and forwards it to the user-specified listener
 *  function for processing.  This allows a user absolute control over
 *  how each UART interrupt is processed.
 *
 *  None of the other Interrupt API functions can be used with this
 *  interrupt handler.  This is because they are symbiotic with the
 *  dw_uart_irqHandler() interrupt handler.  All Command and Status API
 *  functions, however, can be used within the user listener function.
 *  This is in contrast to dw_uart_irqHandler(), where dw_uart_read(),
 *  dw_uart_write(), dw_uart_burstRead() and dw_uart_burstWrite() cannot
 *  be used within the user listener function.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  true        -- an interrupt was processed
 *  false       -- no interrupt was processed
 *  -DW_EIO     -- unrecognized interrupt ID was read
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - iir/all bits
 *
 *  This function is part of the Interrupt API and should not be called
 *  when using the UART in a poll-driven manner.
 * WARNINGS
 *  The user listener function is run in interrupt context and, as such,
 *  care must be taken to ensure that any data shared between it and
 *  normal code is adequately protected from corruption.  Depending on
 *  the target platform, spinlocks, mutexes or semaphores may be used to
 *  achieve this.
 * SEE ALSO
 *  dw_uart_setListener()
 * SOURCE
 */
int dw_uart_userIrqHandler(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_enableFifoAccess
 * DESCRIPTION
 *  Enables the FIFO access test mode.  See the DW_apb_uart databook for
 *  more information on this mode.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- function not unsupported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - far/fifo_access
 *
 *  This function is affected by the FIFO_ACCESS hardware parameter.
 *
 *  This function is part of the Debug API and is not called in the
 *  normal course of using a DW_apb_uart device.
 * SEE ALSO
 *  dw_uart_disableFifoAccess(), dw_uart_isFifoAccessEnabled(),
 *  dw_uart_writeRx(), dw_uart_readTx(), dw_uart_haltTx()
 * SOURCE
 */
int dw_uart_enableFifoAccess(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_disableFifoAccess
 * DESCRIPTION
 *  This function disables the FIFO access test mode.  See the
 *  DW_apb_uart databook for more information on this mode.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- function not unsupported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - far/fifo_access
 *
 *  This function is affected by the FIFO_ACCESS hardware parameter.
 * SEE ALSO
 *  dw_uart_enableFifoAccess(), dw_uart_isFifoAccessEnabled(),
 *  dw_uart_writeRx(), dw_uart_readTx(), dw_uart_haltTx()
 * SOURCE
 */
int dw_uart_disableFifoAccess(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_isFifoAccessEnabled
 * DESCRIPTION
 *  Returns whether the FIFO access test mode is enabled or not.  See
 *  the DW_apb_uart databook for more information on this mode.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  true        -- FIFO access mode is enabled
 *  false       -- FIFO access mode is disabled
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - far/fifo_access
 *
 *  This function is affected by the FIFO_ACCESS hardware parameter.
 *
 *  This function is part of the Debug API and is not called in the
 *  normal course of using a DW_apb_uart device.
 * SEE ALSO
 *  dw_uart_enableFifoAccess(), dw_uart_disableFifoAccess(),
 *  dw_uart_writeRx(), dw_uart_readTx(), dw_uart_haltTx()
 * SOURCE
 */
bool dw_uart_isFifoAccessEnabled(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_writeRx
 * DESCRIPTION
 *  Writes a character to the Rx FIFO.  This can only be performed when
 *  the DW_apb_uart is in test mode.  See the DW_apb_uart databook for a
 *  complete description of the FIFO access test mode.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 *  character   -- character to write to the Rx FIFO
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- function not unsupported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - rfw/all bits
 *
 *  This function is affected by the FIFO_ACCESS hardware parameter.
 *
 *  This function is part of the Debug API and is not called in the
 *  normal course of using a DW_apb_uart device.
 * SEE ALSO
 *  dw_uart_enableFifoAccess(), dw_uart_readTx(), dw_uart_haltTx()
 * SOURCE
 */
int dw_uart_writeRx(struct dw_device *dev, uint16_t character);
/*****/

/****f* uart.functions/dw_uart_readTx
 * DESCRIPTION
 *  This function reads a character from the Tx FIFO.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  -DW_ENOSYS  -- function not unsupported
 *  Otherwise, the unsigned byte read from the Tx FIFO is returned.
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - tfr/all bits
 *
 *  This function is affected by the FIFO_ACCESS hardware parameter.
 *
 *  This function is part of the Debug API and is not called in the
 *  normal course of using a DW_apb_uart device.
 * SEE ALSO
 *  dw_uart_enableFifoAccess(), dw_uart_writeRx(), dw_uart_haltTx()
 * SOURCE
 */
int dw_uart_readTx(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_haltTx
 * DESCRIPTION
 *  This function is used to halt transmissions for testing.  A complete
 *  description of the FIFO access test mode is available in the
 *  DW_apb_uart databook.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- function not unsupported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - htx/halt_tx
 *
 *  This function is affected by the FIFO_ACCESS hardware parameter.
 *
 *  This function is part of the Debug API and is not called in the
 *  normal course of using a DW_apb_uart device.
 * SEE ALSO
 *  dw_uart_enableFifoAccess(), dw_uart_writeRx(), dw_uart_readTx(),
 *  dw_uart_resumeTx(), dw_uart_isTxHalted()
 * SOURCE
 */
int dw_uart_haltTx(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_resumeTx
 * DESCRIPTION
 *  This function is used to resume transmissions after they have been
 *  halted with dw_uart_haltTx().  A complete description of the FIFO
 *  access test mode is available in the DW_apb_uart databook.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- function not unsupported
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - htx/halt_tx
 *
 *  This function is affected by the FIFO_ACCESS hardware parameter.
 *
 *  This function is part of the Debug API and is not called in the
 *  normal course of using a DW_apb_uart device.
 * SEE ALSO
 *  dw_uart_enableFifoAccess(), dw_uart_writeRx(), dw_uart_readTx(),
 *  dw_uart_haltTx(), dw_uart_isTxHalted()
 * SOURCE
 */
int dw_uart_resumeTx(struct dw_device *dev);
/*****/

/****f* uart.functions/dw_uart_isTxHalted
 * DESCRIPTION
 *  This function returns whether transmissions are currently halted or
 *  not.  A full description of the FIFO access test mode is available
 *  in the DW_apb_uart databook.
 * ARGUMENTS
 *  dev         -- DW_apb_uart device handle
 * RETURN VALUE
 *  true        -- transfers are currently halted
 *  false       -- transfers are not currently halted
 * NOTES
 *  Accesses the following DW_apb_uart register(s)/bit field(s):
 *   - htx/halt_tx
 *
 *  This function is affected by the FIFO_ACCESS hardware parameter.
 *
 *  This function is part of the Debug API and is not called in the
 *  normal course of using a DW_apb_uart device.
 * SEE ALSO
 *  dw_uart_enableFifoAccess(), dw_uart_writeRx(), dw_uart_readTx(),
 *  dw_uart_haltTx(), dw_uart_resumeTx()
 * SOURCE
 */
bool dw_uart_isTxHalted(struct dw_device *dev);
/*****/


#ifdef __cplusplus
}
#endif

#endif  // DW_UART_PUBLIC_H

