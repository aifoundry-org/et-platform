#include "printf.h"
#include "serial.h"

inline __attribute__((always_inline)) void _putchar(char character)
{
    SERIAL_write(UART0, &character, 1);
}
