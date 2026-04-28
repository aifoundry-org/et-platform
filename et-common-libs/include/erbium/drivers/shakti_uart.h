/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-----------------------------------------------------------------------*/

/* Shakti UART — stateless inline register-access wrappers. The IP is
 * generic (open-source RISC-V third-party block); this driver is
 * parameterized by a uintptr_t base and OS-agnostic. MMIO goes through
 * common/mmio.h; bitfield extraction uses the GET/SET/MODIFY macros
 * from the auto-generated hwinc/uart.h. Used by erbium/drivers/uart.h. */

#ifndef _ERBIUM_DRIVERS_SHAKTI_UART_H_
#define _ERBIUM_DRIVERS_SHAKTI_UART_H_

#include <stdbool.h>
#include <stdint.h>

#include "common/mmio.h"
#include "hwinc/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Error flag bits returned by shakti_uart_errors() */
#define SHAKTI_UART_ERR_PARITY   (1U << 0)
#define SHAKTI_UART_ERR_OVERRUN  (1U << 1)
#define SHAKTI_UART_ERR_FRAME    (1U << 2)
#define SHAKTI_UART_ERR_BREAK    (1U << 3)

static inline __attribute__((always_inline))
uint32_t shakti_uart_status(uintptr_t base)
{
    return reg_read32(base + UART_STATUSREG_ADDRESS);
}

static inline __attribute__((always_inline))
bool shakti_uart_tx_ready(uintptr_t base)
{
    return !UART_STATUSREG_TX_FULL_GET(shakti_uart_status(base));
}

static inline __attribute__((always_inline))
bool shakti_uart_tx_empty(uintptr_t base)
{
    return !!UART_STATUSREG_TX_EMPTY_GET(shakti_uart_status(base));
}

static inline __attribute__((always_inline))
bool shakti_uart_rx_ready(uintptr_t base)
{
    return !!UART_STATUSREG_RX_NOTEMPTY_GET(shakti_uart_status(base));
}

static inline __attribute__((always_inline))
void shakti_uart_tx_byte(uintptr_t base, uint8_t c)
{
    reg_write32(base + UART_TXREG_ADDRESS, (uint32_t)c);
}

static inline __attribute__((always_inline))
uint8_t shakti_uart_rx_byte(uintptr_t base)
{
    return (uint8_t)(reg_read32(base + UART_RXREG_ADDRESS) & 0xffU);
}

/* V3 TX trigger: after writing TXREG, must write BAUD, DELAY, IEN,
 * RX_THRESHOLD to start transmission. */
static inline __attribute__((always_inline))
void shakti_uart_tx_trigger(uintptr_t base, uint16_t baud, uint32_t ien)
{
    reg_write32(base + UART_BAUDREG_ADDRESS,      (uint32_t)baud);
    reg_write32(base + UART_DELAYREG_ADDRESS,     0U);
    reg_write32(base + UART_INTERRUPTEN_ADDRESS,  ien);
    reg_write32(base + UART_RX_THRESHOLD_ADDRESS, 0U);
}

static inline __attribute__((always_inline))
uint32_t shakti_uart_errors(uintptr_t base)
{
    uint32_t s = shakti_uart_status(base);
    uint32_t err = 0U;

    if (UART_STATUSREG_PARITY_ERROR_GET(s))   err |= SHAKTI_UART_ERR_PARITY;
    if (UART_STATUSREG_OVERRUN_ERROR_GET(s))  err |= SHAKTI_UART_ERR_OVERRUN;
    if (UART_STATUSREG_FRAME_ERROR_GET(s))    err |= SHAKTI_UART_ERR_FRAME;
    if (UART_STATUSREG_BREAK_ERROR_GET(s))    err |= SHAKTI_UART_ERR_BREAK;

    return err;
}

static inline __attribute__((always_inline))
void shakti_uart_int_set(uintptr_t base, uint32_t val)
{
    reg_write32(base + UART_INTERRUPTEN_ADDRESS, val);
}

static inline __attribute__((always_inline))
void shakti_uart_int_enable(uintptr_t base, uint32_t *shadow, uint32_t mask)
{
    *shadow |= mask;
    shakti_uart_int_set(base, *shadow);
}

static inline __attribute__((always_inline))
void shakti_uart_int_disable(uintptr_t base, uint32_t *shadow, uint32_t mask)
{
    *shadow &= ~mask;
    shakti_uart_int_set(base, *shadow);
}

static inline __attribute__((always_inline))
bool shakti_uart_irq_pending(uintptr_t base, uint32_t ien_mask)
{
    return !!(shakti_uart_status(base) & ien_mask);
}

static inline __attribute__((always_inline))
void shakti_uart_baud_set(uintptr_t base, uint16_t divisor)
{
    reg_write32(base + UART_BAUDREG_ADDRESS, (uint32_t)divisor);
}

static inline __attribute__((always_inline))
uint16_t shakti_uart_baud_get(uintptr_t base)
{
    return (uint16_t)reg_read32(base + UART_BAUDREG_ADDRESS);
}

static inline __attribute__((always_inline))
void shakti_uart_control_set(uintptr_t base, uint32_t val)
{
    reg_write32(base + UART_CONTROLREG_ADDRESS, val);
}

static inline __attribute__((always_inline))
void shakti_uart_delay_set(uintptr_t base, uint16_t delay)
{
    reg_write32(base + UART_DELAYREG_ADDRESS, (uint32_t)delay);
}

static inline __attribute__((always_inline))
void shakti_uart_rx_threshold_set(uintptr_t base, uint8_t level)
{
    reg_write32(base + UART_RX_THRESHOLD_ADDRESS, (uint32_t)level);
}

#ifdef __cplusplus
}
#endif

#endif /* _ERBIUM_DRIVERS_SHAKTI_UART_H_ */
