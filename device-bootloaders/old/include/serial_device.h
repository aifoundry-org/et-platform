// Device-specific serial setings needed by the serial driver

#ifndef SERIAL_DEVICE_H
#define SERIAL_DEVICE_H

#include <stdint.h>
#include "etsoc_hal/inc/DW_apb_uart.h"
#include "etsoc_hal/inc/hal_device.h"

#define UART0    (R_SP_UART0_BASEADDR)
#define UART1    (R_SP_UART1_BASEADDR)
#define PU_UART0 (R_PU_UART_BASEADDR)
#define PU_UART1 (R_PU_UART1_BASEADDR)

#endif
