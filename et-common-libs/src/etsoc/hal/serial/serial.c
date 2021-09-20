#include "etsoc/drivers/serial/serial.h"
#include "etsoc/isa/mem-access/io.h"

#include "etsoc_hal/inc/DW_apb_uart.h"

static void set_baud_divisor(uintptr_t uartRegs, unsigned int baudRate, unsigned int clkFreq);

int SERIAL_init(uintptr_t uartRegs)
{
    uint32_t tmp;

    /* Steps from Figure 6-1 "Flowchart for DW_apb_uart Transmit Programming Example"
       from Synopsys DesignWare DW_apb_uart Databook 4.02a July 2018 */

    /* Write to Modem Control Register (MCR) to program SIR mode,
       auto flow, loopback, modem control outputs */
    iowrite32(uartRegs + UART_MCR_ADDRESS,
              UART_MCR_DTR_SET(UART_MCR_DTR_DTR_ACTIVE) |
                  UART_MCR_RTS_SET(UART_MCR_RTS_RTS_ACTIVE) |
                  UART_MCR_OUT1_SET(UART_MCR_OUT1_OUT1_OUT1_0) |
                  UART_MCR_OUT2_SET(UART_MCR_OUT2_OUT2_OUT2_0) |
                  UART_MCR_LOOPBACK_SET(UART_MCR_LOOPBACK_LOOPBACK_DISABLED) |
                  UART_MCR_AFCE_SET(UART_MCR_AFCE_AFCE_DISABLED) | // no automatic flow control
                  UART_MCR_SIRE_SET(UART_MCR_SIRE_SIRE_DISABLED));

    /* Write "1" to LCR[7] (DLAB) bit */
    tmp = ioread32(uartRegs + UART_LCR_ADDRESS);
    tmp = UART_LCR_DLAB_MODIFY(tmp, UART_LCR_DLAB_DLAB_ENABLED);
    iowrite32(uartRegs + UART_LCR_ADDRESS, tmp);

    // For ZeBu, clk_100 is forced to 1GHz for now - it will reduce to 100MHz eventually.
    // UART SCLK = clk_100/4 = 250MHz out of reset for now, 25MHz when clk_100 reduces to 100MHz.
    // configuring UART to 115200baud - fastest possible with 25MHz SCLK, doable with 250Mhz SCLK.
    // 1GHz / 115200 = 868 so use setRatio(868) for ZeBu transactor
    set_baud_divisor(uartRegs, 115200, 25000000); // for now, when PLLs are ON

    /* Write "0" to LCR[7] (DLAB) bit */
    tmp = ioread32(uartRegs + UART_LCR_ADDRESS);
    tmp = UART_LCR_DLAB_MODIFY(tmp, UART_LCR_DLAB_DLAB_DISABLED);
    iowrite32(uartRegs + UART_LCR_ADDRESS, tmp);

    /* Write to LCR to set up transfer characteristics such as data length,
       number of stop bits, parity bits, and so on */
    iowrite32(uartRegs + UART_LCR_ADDRESS,
              UART_LCR_DLS_SET(UART_LCR_DLS_DLS_CHAR_8BITS) |
                  UART_LCR_STOP_SET(UART_LCR_STOP_STOP_STOP_1BIT) |
                  UART_LCR_PEN_SET(UART_LCR_PEN_PEN_DISABLED) | // no parity
                  UART_LCR_EPS_SET(UART_LCR_EPS_EPS_ODD_PARITY) |
                  UART_LCR_SP_SET(UART_LCR_SP_SP_DISABLED) |
                  UART_LCR_BC_SET(UART_LCR_BC_BC_DISABLED) |
                  UART_LCR_DLAB_SET(UART_LCR_DLAB_DLAB_DISABLED));

    /* If FIFO_MODE != NONE (FIFO mode enabled), write to FCR to enable
       FIFOs and set Transmit FIFO threshold level */
    iowrite32(uartRegs + UART_IIR_FCR_ADDRESS,
              UART_IIR_FCR_FIFOE_SET(UART_IIR_FCR_FIFOE_FIFOE_ENABLED) |
                  UART_IIR_FCR_RT_SET(
                      UART_IIR_FCR_RT_RT_FIFO_HALF_FULL) | // rx threshold = RX FIFO half full
                  UART_IIR_FCR_TET_SET(
                      UART_IIR_FCR_TET_TET_FIFO_CHAR_2) | // tx threshold = 2 characters in TX FIFO
                  UART_IIR_FCR_DMAM_SET(UART_IIR_FCR_DMAM_DMAM_MODE0) |
                  UART_IIR_FCR_XFIFOR_SET(UART_IIR_FCR_XFIFOR_XFIFOR_RESET) |
                  UART_IIR_FCR_RFIFOR_SET(UART_IIR_FCR_RFIFOR_RFIFOR_RESET));

    /* Write to IER to enable required interrupts */
    iowrite32(
        uartRegs + UART_IER_IER_ADDRESS,
        UART_IER_IER_ERBFI_SET(UART_IER_IER_ERBFI_ERBFI_DISABLED) | // receive data available
            UART_IER_IER_ETBEI_SET(
                UART_IER_IER_ETBEI_ETBEI_DISABLED) | // transmit holding register empty
            UART_IER_IER_ELSI_SET(UART_IER_IER_ELSI_ELSI_DISABLED) | // receiver line status
            UART_IER_IER_EDSSI_SET(UART_IER_IER_EDSSI_EDSSI_DISABLED) | // modem status
            UART_IER_IER_ELCOLR_SET(
                UART_IER_IER_ELCOLR_ELCOLR_DISABLED) | // error bits cleared by reading LSR register or Rx FIFO (RBR read)
            UART_IER_IER_PTIME_SET(
                UART_IER_IER_PTIME_PTIME_ENABLED)); // Programmable Thre Interrupt Mode Enable

    return 0;
}

// Blocks until the entire string has been written to the fifo
int SERIAL_write(uintptr_t uartRegs, const char *const string, int length)
{
    int i;
    for (i = 0; i < length; i++) {
        /* if THRE_MODE_USER == Enabled AND FIFO_MODE != NONE and both modes are active
           (IER[7] set to one and FCR[0] set to one respectively), the functionality is
           switched to indicate the transmitter FIFO is full, and no longer controls THRE
           interrupts, which are then controlled by the FCR[5:4] threshold setting. */
        while (UART_LSR_THRE_GET(ioread32(uartRegs + UART_LSR_ADDRESS)) == 1U) {
        }

        /* Write characters to be transmitted to transmit FIFO by writing to THR */
        iowrite32(uartRegs + UART_RBR_THR_ADDRESS, UART_RBR_THR_THR_SET(string[i]));
    }
    return i;
}

// Blocks until the entire null-terminated string has been written to the fifo
int SERIAL_puts(uintptr_t uartRegs, const char *const string)
{
    int i;
    for (i = 0; string[i]; i++) {
        /* if THRE_MODE_USER == Enabled AND FIFO_MODE != NONE and both modes are active
           (IER[7] set to one and FCR[0] set to one respectively), the functionality is
           switched to indicate the transmitter FIFO is full, and no longer controls THRE
           interrupts, which are then controlled by the FCR[5:4] threshold setting. */
        while (UART_LSR_THRE_GET(ioread32(uartRegs + UART_LSR_ADDRESS)) == 1U) {
        }

        /* Write characters to be transmitted to transmit FIFO by writing to THR */
        iowrite32(uartRegs + UART_RBR_THR_ADDRESS, UART_RBR_THR_THR_SET(string[i]));
    }
    return i;
}

// Spins until a character is obtained
void SERIAL_getchar(uintptr_t uartRegs, char *c)
{
    while(UART_LSR_DR_GET(ioread32(uartRegs + UART_LSR_ADDRESS)) != 1U);
    *c = UART_RBR_RBR_RBR_GET(ioread32((uartRegs + UART_RBR_ADDRESS)));
}

static void set_baud_divisor(uintptr_t uartRegs, unsigned int baudRate, unsigned int clkFreq)
{
    // See Synopsys DesignWare DW_apb_uart Databook 4.02a July 2018 section 2.4 Fractional Baud Rate Support

    // Divisor
    const unsigned int baud_divisor = clkFreq / (16U * baudRate);

    // Divisor Latch Fractional Value, rounded to nearest 16th given DLF_SIZE = 4
    const unsigned int baud_dlf = ((((2U * clkFreq) / baudRate) + 1U) / 2U) - (16U * baud_divisor);

    iowrite32(uartRegs + UART_RBR_DLL_ADDRESS, UART_RBR_DLL_DLL_SET(baud_divisor & 0xff));
    iowrite32(uartRegs + UART_IER_DLH_ADDRESS, UART_IER_DLH_DLH_SET((baud_divisor >> 8) & 0xff));
    iowrite32(uartRegs + UART_DLF_ADDRESS, UART_DLF_DLF_SET(baud_dlf & 0xf));
}
