/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef BEMU_SHAKTI_UART_H
#define BEMU_SHAKTI_UART_H

#include <array>
#include <cerrno>
#include <cstdint>
#include <system_error>
#include <unistd.h>
#include <sys/select.h>
#include "agent.h"
#include "system.h"
#include "memory/memory_error.h"
#include "memory/memory_region.h"

#include <hwinc/uart.h>
namespace bemu {

template <unsigned long long Base, size_t N, uint32_t PlicSource>
struct ShaktiUart : public MemoryRegion {
    using addr_type     = typename MemoryRegion::addr_type;
    using size_type     = typename MemoryRegion::size_type;
    using value_type    = typename MemoryRegion::value_type;
    using pointer       = typename MemoryRegion::pointer;
    using const_pointer = typename MemoryRegion::const_pointer;

    enum : size_type {
        SHAKTI_UART_BAUD         = UART_BAUDREG_OFFSET,
        SHAKTI_UART_TX_REG       = UART_TXREG_OFFSET,
        SHAKTI_UART_RCV_REG      = UART_RXREG_OFFSET,
        SHAKTI_UART_STATUS       = UART_STATUSREG_OFFSET,
        SHAKTI_UART_DELAY        = UART_DELAYREG_OFFSET,
        SHAKTI_UART_CONTROL      = UART_CONTROLREG_OFFSET,
        SHAKTI_UART_IEN          = UART_INTERRUPTEN_OFFSET,
        SHAKTI_UART_RX_THRESHOLD = UART_RX_THRESHOLD_OFFSET,
    };

    enum : uint32_t {
        STATUS_TX_EMPTY     = UART_STATUSREG_TX_EMPTY_FIELD_MASK,
        STATUS_TX_FULL      = UART_STATUSREG_TX_FULL_FIELD_MASK,
        STATUS_RX_NOT_EMPTY = UART_STATUSREG_RX_NOTEMPTY_FIELD_MASK,
        STATUS_RX_FULL      = UART_STATUSREG_RX_FULL_FIELD_MASK,
        STATUS_PARITY_ERROR = UART_STATUSREG_PARITY_ERROR_FIELD_MASK,
        STATUS_OVERRUN      = UART_STATUSREG_OVERRUN_ERROR_FIELD_MASK,
        STATUS_FRAME_ERROR  = UART_STATUSREG_FRAME_ERROR_FIELD_MASK,
        STATUS_BREAK_ERROR  = UART_STATUSREG_BREAK_ERROR_FIELD_MASK,
        STATUS_RXFIFOTHRE   = UART_STATUSREG_RX_FIFO_THRESHOLD_FIELD_MASK,
    };

    static constexpr size_t FIFO_DEPTH = 16;

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        (void) n;

        switch (pos) {
        case SHAKTI_UART_TX_REG:
            *reinterpret_cast<uint32_t*>(result) = 0;
            break;
        case SHAKTI_UART_RCV_REG: {
            uint8_t data = 0;
            if (agent.chip->is_uart_enabled()) {
                poll_rx_fifo();
                (void)rx_fifo_pop(data);
            }
            *reinterpret_cast<uint32_t*>(result) = data;
            sync_interrupt_line(agent, false);
            break;
        }
        case SHAKTI_UART_STATUS: {
            if (agent.chip->is_uart_enabled()) {
                *reinterpret_cast<uint32_t*>(result) = status_value(true);
            } else {
                *reinterpret_cast<uint32_t*>(result) = STATUS_TX_EMPTY;
            }
            sync_interrupt_line(agent, false);
            break;
        }
        case SHAKTI_UART_BAUD:
            *reinterpret_cast<uint32_t*>(result) = reg_baud;
            break;
        case SHAKTI_UART_DELAY:
            *reinterpret_cast<uint32_t*>(result) = reg_delay;
            break;
        case SHAKTI_UART_CONTROL:
            *reinterpret_cast<uint32_t*>(result) = reg_control;
            break;
        case SHAKTI_UART_IEN:
            *reinterpret_cast<uint32_t*>(result) = reg_ien;
            break;
        case SHAKTI_UART_RX_THRESHOLD:
            *reinterpret_cast<uint32_t*>(result) = reg_rx_threshold;
            break;
        default:
            *reinterpret_cast<uint32_t*>(result) = 0;
            break;
        }
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        (void) n;

        uint32_t value = *reinterpret_cast<const uint32_t*>(source);

        switch (pos) {
        case SHAKTI_UART_TX_REG:
            if (agent.chip->is_uart_enabled()) {
                tx_fifo_push(value & 0xFFu);
            }
            sync_interrupt_line(agent, false);
            break;
        case SHAKTI_UART_BAUD:
            reg_baud = value & UART_BAUDREG_WRITE_MASK;
            break;
        case SHAKTI_UART_DELAY:
            reg_delay = value & UART_DELAYREG_WRITE_MASK;
            break;
        case SHAKTI_UART_CONTROL:
            reg_control = value & UART_CONTROLREG_WRITE_MASK;
            break;
        case SHAKTI_UART_IEN:
            reg_ien = value & UART_INTERRUPTEN_WRITE_MASK;
            sync_interrupt_line(agent, true);
            break;
        case SHAKTI_UART_RX_THRESHOLD:
            reg_rx_threshold = value & UART_RX_THRESHOLD_WRITE_MASK;
            sync_interrupt_line(agent, true);
            break;
        case SHAKTI_UART_STATUS:
            break;
        default:
            break;
        }
    }

    void init(const Agent&, size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::ShaktiUart::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }

    // periph_divider reset: count=15, output clk = sys_clk / (2 * 15) = /30
    static constexpr uint64_t UART_CLK_DIV = 30;

    void clock_tick(const Agent& agent, uint64_t cycle) {
        if ((cycle % UART_CLK_DIV) != 0) return;
        drain_tx_fifo();
        poll_rx_fifo();

        if (reg_ien == 0) return;
        uint32_t pending = status_value(false) & reg_ien;
        if (pending != 0) {
            agent.chip->er_plic_interrupt_pending_set(PlicSource);
            interrupt_asserted = true;
        } else if (interrupt_asserted) {
            agent.chip->er_plic_interrupt_pending_clear(PlicSource);
            interrupt_asserted = false;
        }
    }

    int tx_fd = -1;
    int rx_fd = -1;

private:
    uint32_t reg_baud = 0;
    uint32_t reg_delay = 0;
    uint32_t reg_control = 0;
    uint32_t reg_ien = 0;
    uint32_t reg_rx_threshold = 0;
    std::array<uint8_t, FIFO_DEPTH> tx_fifo{};
    size_t   tx_fifo_head = 0;
    size_t   tx_fifo_count = 0;
    std::array<uint8_t, FIFO_DEPTH> rx_fifo{};
    size_t   rx_fifo_head = 0;
    size_t   rx_fifo_count = 0;

    size_t tx_fifo_tail() const { return (tx_fifo_head + tx_fifo_count) % FIFO_DEPTH; }
    size_t rx_fifo_tail() const { return (rx_fifo_head + rx_fifo_count) % FIFO_DEPTH; }
    bool     error_overrun = false;
    bool     interrupt_asserted = false;

    void sync_interrupt_line(const Agent& agent, bool poll_rx) {
        if (poll_rx) poll_rx_fifo();
        uint32_t pending = status_value(false) & reg_ien;
        bool should_assert = (pending != 0);
        if (should_assert != interrupt_asserted) {
            interrupt_asserted = should_assert;
            if (should_assert)
                agent.chip->er_plic_interrupt_pending_set(PlicSource);
            else
                agent.chip->er_plic_interrupt_pending_clear(PlicSource);
        }
    }

    uint32_t status_value(bool poll_rx) {
        if (poll_rx) {
            poll_rx_fifo();
        }

        uint32_t status = 0;

        // TX status
        if (tx_fifo_count == 0) {
            status |= STATUS_TX_EMPTY;
        }
        if (tx_fifo_count >= FIFO_DEPTH) {
            status |= STATUS_TX_FULL;
        }

        // RX status
        if (rx_fifo_count > 0) {
            status |= STATUS_RX_NOT_EMPTY;
        }
        if (rx_fifo_count >= FIFO_DEPTH) {
            status |= STATUS_RX_FULL;
        }
        if (error_overrun) {
            status |= STATUS_OVERRUN;
        }
        if (rx_threshold_reached()) {
            status |= STATUS_RXFIFOTHRE;
        }
        return status;
    }

    bool rx_threshold_reached() const {
        if (reg_rx_threshold == 0) {
            return rx_fifo_count > 0;
        }
        return rx_fifo_count > reg_rx_threshold;
    }

    void tx_fifo_push(uint8_t value) {
        if (tx_fifo_count >= FIFO_DEPTH) {
            return;
        }
        tx_fifo[tx_fifo_tail()] = value;
        ++tx_fifo_count;
    }

    void drain_tx_fifo() {
        if (tx_fifo_count == 0 || tx_fd == -1) return;
        uint8_t byte = tx_fifo[tx_fifo_head];
        tx_fifo_head = (tx_fifo_head + 1) % FIFO_DEPTH;
        --tx_fifo_count;
        (void)::write(tx_fd, &byte, 1);
    }

    bool rx_fifo_push(uint8_t value) {
        if (rx_fifo_count >= FIFO_DEPTH) {
            error_overrun = true;
            return false;
        }
        rx_fifo[rx_fifo_tail()] = value;
        ++rx_fifo_count;
        return true;
    }

    bool rx_fifo_pop(uint8_t& value) {
        if (rx_fifo_count == 0) {
            return false;
        }
        value = rx_fifo[rx_fifo_head];
        rx_fifo_head = (rx_fifo_head + 1) % FIFO_DEPTH;
        --rx_fifo_count;
        return true;
    }

    void poll_rx_fifo() {
        while (true) {
            uint8_t value = 0;
            const int result = read_rx_byte_nonblocking(value);
            if (result != 1) {
                break;
            }
            (void)rx_fifo_push(value);
        }
    }

    int read_rx_byte_nonblocking(uint8_t& value) {
        if (rx_fd == -1) return 0;

        fd_set rfds;
        struct timeval tv = {0, 0};
        FD_ZERO(&rfds);
        FD_SET(rx_fd, &rfds);
        if (select(rx_fd + 1, &rfds, nullptr, nullptr, &tv) <= 0)
            return 0;

        ssize_t r = ::read(rx_fd, &value, 1);
        if (r == 1) { return 1; }
        if (r == 0) { rx_fd = -1; return 0; }
        return -1;
    }
};


} // namespace bemu

#endif // BEMU_SHAKTI_UART_H
