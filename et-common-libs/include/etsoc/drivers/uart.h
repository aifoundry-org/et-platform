/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-----------------------------------------------------------------------*/

/* etsoc UART driver — DesignWare-style register layout at the
 * processing-unit UART base R_PU_UART_BASEADDR.
 *
 * The sys_emu UART model used by Minion tests exposes a simple DW
 * UART view — RX/TX data at offset 0x0, line status at offset 0x14.
 * Baud programming and pin-mux gating are not exposed by the
 * emulator model, so the matching helpers report unsupported and
 * are no-ops.
 *
 * The stateful rx_cache workaround exists because sys_emu reports
 * LSR.DR=1 at file-descriptor EOF — polling LSR.DR directly can
 * return a stale "data ready" after the last byte. Caching one
 * byte normalizes poll-mode semantics for tests. */

#ifndef _ETSOC_DRIVERS_UART_H_
#define _ETSOC_DRIVERS_UART_H_

#include <stdbool.h>
#include <stdint.h>

#include "common/mmio.h"
#include "hwinc/hal_device.h"   /* R_PU_UART_BASEADDR */

#ifdef __cplusplus
extern "C" {
#endif

#define ETSOC_UART0_BASE          ((uintptr_t)R_PU_UART_BASEADDR)

#define ETSOC_UART_RBR_THR_OFFSET 0x00u
#define ETSOC_UART_LSR_OFFSET     0x14u

#define ETSOC_UART_LSR_DR         (1u << 0)
#define ETSOC_UART_LSR_THRE       (1u << 5)

static bool etsoc_uart_rx_cache_valid;
static uint8_t etsoc_uart_rx_cache_byte;

static inline __attribute__((always_inline))
bool etsoc_uart_fill_rx_cache(void)
{
    uint32_t lsr;
    uint8_t byte;

    if (etsoc_uart_rx_cache_valid)
    {
        return true;
    }

    lsr = reg_read32(ETSOC_UART0_BASE + ETSOC_UART_LSR_OFFSET);
    if ((lsr & ETSOC_UART_LSR_DR) == 0U)
    {
        return false;
    }

    byte = (uint8_t)(reg_read32(ETSOC_UART0_BASE + ETSOC_UART_RBR_THR_OFFSET) & 0xffU);
    if (byte == 0U)
    {
        return false;
    }

    etsoc_uart_rx_cache_byte = byte;
    etsoc_uart_rx_cache_valid = true;
    return true;
}

static inline __attribute__((always_inline))
bool uart_supports_pinmux_gate(void)
{
    return false;
}

static inline __attribute__((always_inline))
void uart_enable_pinmux(void)
{
}

static inline __attribute__((always_inline))
bool uart_supports_baud_roundtrip(void)
{
    return false;
}

static inline __attribute__((always_inline))
uint32_t uart_baud_get(void)
{
    return 0U;
}

static inline __attribute__((always_inline))
void uart_baud_set(uint32_t val)
{
    (void)val;
}

static inline __attribute__((always_inline))
bool uart_tx_ready(void)
{
    uint32_t lsr = reg_read32(ETSOC_UART0_BASE + ETSOC_UART_LSR_OFFSET);

    return (lsr & ETSOC_UART_LSR_THRE) == 0U;
}

static inline __attribute__((always_inline))
bool uart_tx_empty(void)
{
    return uart_tx_ready();
}

static inline __attribute__((always_inline))
bool uart_rx_ready(void)
{
    return etsoc_uart_fill_rx_cache();
}

static inline __attribute__((always_inline))
uint8_t uart_rx_byte(void)
{
    if (!etsoc_uart_fill_rx_cache())
    {
        return 0U;
    }

    etsoc_uart_rx_cache_valid = false;
    return etsoc_uart_rx_cache_byte;
}

static inline __attribute__((always_inline))
void uart_tx_byte(uint8_t c)
{
    reg_write32(ETSOC_UART0_BASE + ETSOC_UART_RBR_THR_OFFSET, (uint32_t)c);
}

#ifdef __cplusplus
}
#endif

#endif /* _ETSOC_DRIVERS_UART_H_ */
