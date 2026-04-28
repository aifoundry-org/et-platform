/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-----------------------------------------------------------------------*/

/* Erbium UART driver. Wires the Shakti UART IP at UART0
 * (ERBIUM_TOP_UART_REGISTERS_BASE) to a small portable surface
 * (uart_tx_byte / _rx_byte / _tx_ready / _baud_set / etc.). The
 * pinmux gate lives in system_registers.SystemConfig.UART_ENABLE
 * and must be enabled before the UART can drive the pins. */

#ifndef _ERBIUM_DRIVERS_UART_H_
#define _ERBIUM_DRIVERS_UART_H_

#include <stdbool.h>
#include <stdint.h>

#include "common/mmio.h"
#include "erbium/drivers/shakti_uart.h"
#include "hwinc/system.h"   /* SYSTEM_SYSTEMCONFIG_* */
#include "hwinc/top.h"      /* ERBIUM_TOP_UART_REGISTERS_BASE, *_SYSTEM_REGISTERS_BASE */
#include "hwinc/uart.h"     /* UART_BAUDREG_ADDRESS */

#ifdef __cplusplus
extern "C" {
#endif

#define ERBIUM_UART0_BASE   ERBIUM_TOP_UART_REGISTERS_BASE
#define ERBIUM_SYSREG_BASE  ERBIUM_TOP_SYSTEM_REGISTERS_BASE

static inline __attribute__((always_inline))
bool uart_supports_pinmux_gate(void)
{
    return true;
}

static inline __attribute__((always_inline))
void uart_enable_pinmux(void)
{
    uintptr_t const addr = ERBIUM_SYSREG_BASE + SYSTEM_SYSTEMCONFIG_ADDRESS;
    uint32_t cfg = reg_read32(addr);

    cfg = SYSTEM_SYSTEMCONFIG_UART_ENABLE_MODIFY(cfg, 1U);
    reg_write32(addr, cfg);
}

static inline __attribute__((always_inline))
bool uart_supports_baud_roundtrip(void)
{
    return true;
}

static inline __attribute__((always_inline))
uint32_t uart_baud_get(void)
{
    return shakti_uart_baud_get(ERBIUM_UART0_BASE);
}

static inline __attribute__((always_inline))
void uart_baud_set(uint32_t val)
{
    shakti_uart_baud_set(ERBIUM_UART0_BASE, (uint16_t)val);
}

static inline __attribute__((always_inline))
bool uart_tx_ready(void)
{
    return shakti_uart_tx_ready(ERBIUM_UART0_BASE);
}

static inline __attribute__((always_inline))
bool uart_tx_empty(void)
{
    return shakti_uart_tx_empty(ERBIUM_UART0_BASE);
}

static inline __attribute__((always_inline))
bool uart_rx_ready(void)
{
    return shakti_uart_rx_ready(ERBIUM_UART0_BASE);
}

static inline __attribute__((always_inline))
uint8_t uart_rx_byte(void)
{
    return shakti_uart_rx_byte(ERBIUM_UART0_BASE);
}

static inline __attribute__((always_inline))
void uart_tx_byte(uint8_t c)
{
    shakti_uart_tx_byte(ERBIUM_UART0_BASE, c);
}

#ifdef __cplusplus
}
#endif

#endif /* _ERBIUM_DRIVERS_UART_H_ */
