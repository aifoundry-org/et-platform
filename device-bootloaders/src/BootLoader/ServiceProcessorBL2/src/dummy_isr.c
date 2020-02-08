#include "dummy_isr.h"
#include "serial.h"

void dummy_isr1(void)
{
    SERIAL_write(UART0, "isr1\r\n", 6);
}

void dummy_isr2(void)
{
    SERIAL_write(UART0, "isr2\r\n", 6);
}
