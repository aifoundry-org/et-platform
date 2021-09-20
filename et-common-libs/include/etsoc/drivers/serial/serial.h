#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include "etsoc_hal/inc/hal_device.h"

#define UART0 R_PU_UART_BASEADDR
#define UART1 R_PU_UART1_BASEADDR

int SERIAL_init(uintptr_t uartRegs);
int SERIAL_write(uintptr_t uartRegs, const char *const string, int length);
int SERIAL_puts(uintptr_t uartRegs, const char *const string);
void SERIAL_getchar(uintptr_t uartRegs, char *c);

#endif // SERIAL_H
