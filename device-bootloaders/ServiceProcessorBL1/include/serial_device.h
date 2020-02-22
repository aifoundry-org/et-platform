/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

// Device-specific serial setings needed by the serial driver

#ifndef SERIAL_DEVICE_H
#define SERIAL_DEVICE_H

#include "DW_apb_uart.h"
#include "hal_device.h"

#define UART0 ((volatile Uart_t*)R_SP_UART0_BASEADDR)
#define UART1 ((volatile Uart_t*)R_SP_UART1_BASEADDR)
#define PU_UART0 ((volatile Uart_t*)R_PU_UART_BASEADDR)
#define PU_UART1 ((volatile Uart_t*)R_PU_UART1_BASEADDR)

#endif
