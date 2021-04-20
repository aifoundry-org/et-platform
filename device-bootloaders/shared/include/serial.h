#ifndef SERIAL_H
#define SERIAL_H

#include "serial_device.h"

#include <stdint.h>

int SERIAL_init(uintptr_t uartRegs);
void SERIAL_write(uintptr_t uartRegs, const char *const string, int length);
void SERIAL_getchar(uintptr_t uartRegs, char *c);

#endif // SERIAL_H
