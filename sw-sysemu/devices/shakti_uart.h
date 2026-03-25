/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef BEMU_SHAKTI_UART_H
#define BEMU_SHAKTI_UART_H

#include <cerrno>
#include <cstdint>
#include <system_error>
#include <unistd.h>
#include <sys/select.h>
#include "agent.h"
#include "system.h"
#include "memory/memory_error.h"
#include "memory/memory_region.h"

namespace bemu {

template <unsigned long long Base, size_t N>
struct ShaktiUart : public MemoryRegion {
    using addr_type     = typename MemoryRegion::addr_type;
    using size_type     = typename MemoryRegion::size_type;
    using value_type    = typename MemoryRegion::value_type;
    using pointer       = typename MemoryRegion::pointer;
    using const_pointer = typename MemoryRegion::const_pointer;

    // Shakti UART register offsets (64-bit aligned)
    enum : size_type {
        SHAKTI_UART_BAUD         = 0x00,
        SHAKTI_UART_TX_REG       = 0x08,
        SHAKTI_UART_RCV_REG      = 0x10,
        SHAKTI_UART_STATUS       = 0x18,
        SHAKTI_UART_DELAY        = 0x20,
        SHAKTI_UART_CONTROL      = 0x28,
        SHAKTI_UART_IEN          = 0x30,
        SHAKTI_UART_RX_THRESHOLD = 0x40,
    };

    // STATUS register bits
    enum : uint32_t {
        STATUS_TX_EMPTY     = (1u << 0),
        STATUS_TX_FULL      = (1u << 1),
        STATUS_RX_NOT_EMPTY = (1u << 2),
        STATUS_RX_FULL      = (1u << 3),
    };

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        (void) n;

        switch (pos) {
        case SHAKTI_UART_TX_REG:
            *reinterpret_cast<uint32_t*>(result) = 0;
            break;
        case SHAKTI_UART_RCV_REG: {
            uint8_t data = 0;
            if (agent.chip->is_uart_enabled() && rx_has_byte) {
                data = rx_byte_buf;
                rx_has_byte = false;
            }
            *reinterpret_cast<uint32_t*>(result) = data;
            break;
        }
        case SHAKTI_UART_STATUS: {
            uint32_t status = STATUS_TX_EMPTY;
            if (agent.chip->is_uart_enabled() && rx_data_available()) {
                status |= STATUS_RX_NOT_EMPTY;
            }
            *reinterpret_cast<uint32_t*>(result) = status;
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
            if (agent.chip->is_uart_enabled() && (tx_fd != -1) && (::write(tx_fd, source, 1) < 0)) {
                auto error = std::error_code(errno, std::system_category());
                throw std::system_error(error, "bemu::ShaktiUart::write()");
            }
            break;
        case SHAKTI_UART_BAUD:
            reg_baud = value & 0xFFFFu;
            break;
        case SHAKTI_UART_DELAY:
            reg_delay = value & 0xFFFFu;
            break;
        case SHAKTI_UART_CONTROL:
            reg_control = value & 0x07FEu;  // charsize, parity, stopbits
            break;
        case SHAKTI_UART_IEN:
            reg_ien = value & 0x01FFu;  // bits [8:0]
            break;
        case SHAKTI_UART_RX_THRESHOLD:
            reg_rx_threshold = value & 0xFFu;
            break;
        case SHAKTI_UART_STATUS:
            if (value & (1u << 5))  // OVERRUN write-to-clear
                error_overrun = false;
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

    int tx_fd = -1;
    int rx_fd = -1;

private:
    uint32_t reg_baud = 0;
    uint32_t reg_delay = 0;
    uint32_t reg_control = 0;
    uint32_t reg_ien = 0;
    uint32_t reg_rx_threshold = 0;
    bool     rx_has_byte = false;
    uint8_t  rx_byte_buf = 0;
    bool     error_overrun = false;

    // select() with timeout=0 checks if read() would block. However, it
    // returns "readable" both for actual data and for EOF — so select()
    // alone cannot tell them apart. We follow up with read() to distinguish:
    //   r == 1 : real byte  — buffer it for the guest
    //   r == 0 : EOF        — set rx_fd = -1, stop polling
    //   r <  0 : error      — no data
    bool rx_data_available() {
        if (rx_has_byte) return true;
        if (rx_fd == -1) return false;

        fd_set rfds;
        struct timeval tv = {0, 0};
        FD_ZERO(&rfds);
        FD_SET(rx_fd, &rfds);
        if (select(rx_fd + 1, &rfds, nullptr, nullptr, &tv) <= 0)
            return false;

        uint8_t b;
        ssize_t r = ::read(rx_fd, &b, 1);
        if (r == 1) { rx_byte_buf = b; rx_has_byte = true; return true; }
        if (r == 0) { rx_fd = -1; return false; }
        return false;
    }
};


} // namespace bemu

#endif // BEMU_SHAKTI_UART_H
