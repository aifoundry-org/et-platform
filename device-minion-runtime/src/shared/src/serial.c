#include "serial.h"

static void set_baud_divisor(volatile SPIO_UART_t* const uartRegs, unsigned int baudRate, unsigned int clkFreq);

int SERIAL_init(volatile SPIO_UART_t* const uartRegs)
{
    /* Steps from Figure 6-1 "Flowchart for DW_apb_uart Transmit Programming Example"
       from Synopsys DesignWare DW_apb_uart Databook 4.02a July 2018 */

    /* Write to Modem Control Register (MCR) to program SIR mode,
       auto flow, loopback, modem control outputs */
    uartRegs->MCR.R = (SPIO_UART_MCR_t){.B = {.DTR = 1,
                                              .RTS = 1,
                                              .OUT1 = 0,
                                              .OUT2 = 0,
                                              .LOOPBACK = 0,
                                              .AFCE = 0,     // no automatic flow control
                                              .SIRE = 0}}.R;

    /* Write "1" to LCR[7] (DLAB) bit */
    uartRegs->LCR.B.DLAB = 1;

    // For ZeBu, clk_100 is forced to 1GHz for now - it will reduce to 100MHz eventually.
    // UART SCLK = clk_100/4 = 250MHz out of reset for now, 25MHz when clk_100 reduces to 100MHz.
    // configuring UART to 1.5Mbaud - fastest possible with 25MHz SCLK, doable with 250Mhz SCLK.
    // 1GHz / 1500000 = 667 so use setRatio(667) for ZeBu transactor
    set_baud_divisor(uartRegs, 1500000, 250000000); // for now

    /* Write "0" to LCR[7] (DLAB) bit */
    uartRegs->LCR.B.DLAB = 0;

    /* Write to LCR to set up transfer characteristics such as data length,
       number of stop bits, parity bits, and so on */
    uartRegs->LCR.R = (SPIO_UART_LCR_t){.B = {.DLS = 3,      // 8 data bits per character
                                              .STOP = 0,     // 1 stop bit
                                              .PEN = 0,      // no parity
                                              .EPS = 0,
                                              .SP = 0,
                                              .BC = 0,
                                              .DLAB = 0}}.R;

    /* If FIFO_MODE != NONE (FIFO mode enabled), write to FCR to enable
       FIFOs and set Transmit FIFO threshold level */
    uartRegs->FCR.R = (SPIO_UART_FCR_t){.B = {.FIFOE = 1,      // FIFO_MODE = enabled
                                              .RT = 2,         // rx fifo half full
                                              .TET = 1,        // 2 characters in tx fifo
                                              .DMAM = 0,
                                              .XFIFOR = 1,     // reset the tx fifo
                                              .RFIFOR = 1}}.R; // reset the rx fifo

    /* Write to IER to enable required interrupts */
    uartRegs->IER_DLH.R = (SPIO_UART_IER_DLH_t){.B_IER = {.ERBFI = 0,     // receive data available
                                                          .ETBEI = 0,     // transmit holding register empty
                                                          .ELSI =  0,     // receiver line status
                                                          .EDSSI = 0,     // modem status
                                                          .ELCOLR = 0,    // error bits cleared by reading LSR register or Rx FIFO (RBR read)
                                                          .PTIME = 1}}.R; // Programmable Thre Interrupt Mode Enable

    return 0;
}

// Blocks until the entire string has been written to the fifo
void SERIAL_write(volatile SPIO_UART_t* const uartRegs, const char* const string, int length)
{
    for (int i = 0; i < length; i++)
    {
        /* if THRE_MODE_USER == Enabled AND FIFO_MODE != NONE and both modes are active
           (IER[7] set to one and FCR[0] set to one respectively), the functionality is
           switched to indicate the transmitter FIFO is full, and no longer controls THRE
           interrupts, which are then controlled by the FCR[5:4] threshold setting. */
        while (uartRegs->LSR.B.THRE == 1U) {}

        /* Write characters to be transmitted to transmit FIFO by writing to THR */
        uartRegs->RBR_THR_DLL.R = (SPIO_UART_RBR_THR_DLL_t){.B = {.RBR_THR_DLL = string[i]}}.R;
    }
}

static void set_baud_divisor(volatile SPIO_UART_t* const uartRegs, unsigned int baudRate, unsigned int clkFreq)
{
    // See Synopsys DesignWare DW_apb_uart Databook 4.02a July 2018 section 2.4 Fractional Baud Rate Support

    // Divisor
    const unsigned int baud_divisor = clkFreq/(16U*baudRate);

    // Divisor Latch Fractional Value, rounded to nearest 16th given DLF_SIZE = 4
    const unsigned int baud_dlf = ((((2U*clkFreq)/baudRate) + 1U) / 2U) - (16U*baud_divisor);

    uartRegs->RBR_THR_DLL.R = (SPIO_UART_RBR_THR_DLL_t){.B = {.RBR_THR_DLL = baud_divisor & 0xff}}.R;
	uartRegs->IER_DLH.R = (SPIO_UART_IER_DLH_t){.B_DLH = {.DLH = (baud_divisor >> 8) & 0xff}}.R;
    uartRegs->DLF.R = (SPIO_UART_DLF_t){.B = {.DLF = baud_dlf & 0xf}}.R;
}
