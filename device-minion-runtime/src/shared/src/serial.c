#include "serial.h"

static unsigned int calc_baud_divisor(unsigned int baudRate, unsigned int clkFreq);

int SERIAL_init(SPIO_UART_t* const uartRegs)
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

    /* Write to DLL, DLH to set up divisor for required baud rate */
    const unsigned int baud_divisor = calc_baud_divisor(57600U, 100000000U);
    uartRegs->RBR_THR_DLL.R = (SPIO_UART_RBR_THR_DLL_t){.B = {.RBR_THR_DLL = baud_divisor & 0xff}}.R;
	uartRegs->IER_DLH.R = (SPIO_UART_IER_DLH_t){.B_DLH = {.DLH = (baud_divisor >> 8) & 0xff}}.R;

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
void SERIAL_write(SPIO_UART_t* const uartRegs, const char* const string, unsigned int length)
{
    for (unsigned int i = 0; i < length; i++)
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

static unsigned int calc_baud_divisor(unsigned int baudRate, unsigned int clkFreq)
{
    return clkFreq/(16U*baudRate);
}
