#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include "etsoc_hal/inc/hal_device.h"

#define SP_UART0 R_SP_UART0_BASEADDR 
#define SP_UART1 R_SP_UART1_BASEADDR  
#define PU_UART0 R_PU_UART_BASEADDR
#define PU_UART1 R_PU_UART1_BASEADDR

int SERIAL_init(uintptr_t uartRegs);
int SERIAL_write(uintptr_t uartRegs, const char *const string, int length);
int SERIAL_puts(uintptr_t uartRegs, const char *const string);
void SERIAL_getchar(uintptr_t uartRegs, char *c);

#endif // SERIAL_H
