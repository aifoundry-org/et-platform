#include "serial.h"

static void set_baud_divisor(volatile Uart_t* const uartRegs, unsigned int baudRate, unsigned int clkFreq);

int SERIAL_init(volatile Uart_t* const uartRegs)
{
    /* Steps from Figure 6-1 "Flowchart for DW_apb_uart Transmit Programming Example"
       from Synopsys DesignWare DW_apb_uart Databook 4.02a July 2018 */

    /* Write to Modem Control Register (MCR) to program SIR mode,
       auto flow, loopback, modem control outputs */
    uartRegs->MCR.R = (Uart_MCR_t){.B = {.DTR      = 1,
                                         .RTS      = 1,
                                         .OUT1     = 0,
                                         .OUT2     = 0,
                                         .LoopBack = 0,
                                         .AFCE     = 0, // no automatic flow control
                                         .SIRE     = 0}}.R;

    /* Write "1" to LCR[7] (DLAB) bit */
    uartRegs->LCR.B.DLAB = 1;

    // For ZeBu, clk_100 is forced to 1GHz for now - it will reduce to 100MHz eventually.
    // UART SCLK = clk_100/4 = 250MHz out of reset for now, 25MHz when clk_100 reduces to 100MHz.
    // configuring UART to 115200baud - fastest possible with 25MHz SCLK, doable with 250Mhz SCLK.
    // 1GHz / 115200 = 868 so use setRatio(868) for ZeBu transactor
    set_baud_divisor(uartRegs, 115200, 25000000); // for now, when PLLs are ON

    /* Write "0" to LCR[7] (DLAB) bit */
    uartRegs->LCR.B.DLAB = 0;

    /* Write to LCR to set up transfer characteristics such as data length,
       number of stop bits, parity bits, and so on */
    uartRegs->LCR.R = (Uart_LCR_t){.B = {.DLS  = UART_LCR_DLS_DLS_CHAR_8BITS,
                                         .STOP = UART_LCR_STOP_STOP_STOP_1BIT,
                                         .PEN  = UART_LCR_PEN_PEN_DISABLED, // no parity
                                         .EPS  = UART_LCR_EPS_EPS_ODD_PARITY,
                                         .SP   = UART_LCR_SP_SP_DISABLED,
                                         .BC   = UART_LCR_BC_BC_DISABLED,
                                         .DLAB = UART_LCR_DLAB_DLAB_DISABLED}}.R;

    /* If FIFO_MODE != NONE (FIFO mode enabled), write to FCR to enable
       FIFOs and set Transmit FIFO threshold level */
    uartRegs->IIR.FCR.R = (Uart_IIR_FCR_t){.B = {.FIFOE  = UART_IIR_FCR_FIFOE_FIFOE_ENABLED,
                                                 .RT     = UART_IIR_FCR_RT_RT_FIFO_HALF_FULL, // rx threshold = RX FIFO half full
                                                 .TET    = UART_IIR_FCR_TET_TET_FIFO_CHAR_2, // tx threshold = 2 characters in TX FIFO
                                                 .DMAM   = UART_IIR_FCR_DMAM_DMAM_MODE0,
                                                 .XFIFOR = UART_IIR_FCR_XFIFOR_XFIFOR_RESET,
                                                 .RFIFOR = UART_IIR_FCR_RFIFOR_RFIFOR_RESET}}.R;

    /* Write to IER to enable required interrupts */
    uartRegs->IER.IER.R = (Uart_IER_IER_t){.B = {.ERBFI  = 0, // receive data available
                                                 .ETBEI  = 0, // transmit holding register empty
                                                 .ELSI   = 0, // receiver line status
                                                 .EDSSI  = 0, // modem status
                                                 .ELCOLR = 0, // error bits cleared by reading LSR register or Rx FIFO (RBR read)
                                                 .PTIME  = UART_IER_IER_PTIME_PTIME_ENABLED}}.R; // Programmable Thre Interrupt Mode Enable

    return 0;
}

// Blocks until the entire string has been written to the fifo
void SERIAL_write(volatile Uart_t* const uartRegs, const char* const string, int length)
{
    for (int i = 0; i < length; i++)
    {
        /* if THRE_MODE_USER == Enabled AND FIFO_MODE != NONE and both modes are active
           (IER[7] set to one and FCR[0] set to one respectively), the functionality is
           switched to indicate the transmitter FIFO is full, and no longer controls THRE
           interrupts, which are then controlled by the FCR[5:4] threshold setting. */
        while (uartRegs->LSR.B.THRE == 1U) {}

        /* Write characters to be transmitted to transmit FIFO by writing to THR */
        uartRegs->RBR.THR.R = (Uart_RBR_THR_t){.B = {.THR = string[i]}}.R;
    }
}

static void set_baud_divisor(volatile Uart_t* const uartRegs, unsigned int baudRate, unsigned int clkFreq)
{
    // See Synopsys DesignWare DW_apb_uart Databook 4.02a July 2018 section 2.4 Fractional Baud Rate Support

    // Divisor
    const unsigned int baud_divisor = clkFreq/(16U*baudRate);

    // Divisor Latch Fractional Value, rounded to nearest 16th given DLF_SIZE = 4
    const unsigned int baud_dlf = ((((2U*clkFreq)/baudRate) + 1U) / 2U) - (16U*baud_divisor);

    uartRegs->RBR.DLL.R = (Uart_RBR_DLL_t){.B = {.DLL = baud_divisor & 0xff}}.R;
	uartRegs->IER.DLH.R = (Uart_IER_DLH_t){.B = {.dlh = (baud_divisor >> 8) & 0xff}}.R;
    uartRegs->DLF.R = (Uart_DLF_t){.B = {.DLF = baud_dlf & 0xf}}.R;
}
