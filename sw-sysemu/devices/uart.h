/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef BEMU_UART_H
#define BEMU_UART_H

#include <cassert>
#include <cerrno>
#include <cinttypes>
#include <cstdint>
#include <system_error>
#include <unistd.h>
#include "memory/memory_error.h"
#include "memory/memory_region.h"

namespace bemu {

template <unsigned long long Base, size_t N>
struct Uart : public MemoryRegion {
    using addr_type     = typename MemoryRegion::addr_type;
    using size_type     = typename MemoryRegion::size_type;
    using value_type    = typename MemoryRegion::value_type;
    using pointer       = typename MemoryRegion::pointer;
    using const_pointer = typename MemoryRegion::const_pointer;

    // Registers from DW_apb_uart.csr
    enum : size_type {
        DW_APB_UART_RBR_THR = 0x00,  // Receiver Buffer / Transmitter Holding
        DW_APB_UART_IER     = 0x04,  // Interrupt Enable Register
        DW_APB_UART_IIR     = 0x08,  // Interrupt Identification Register (read)
        DW_APB_UART_FCR     = 0x08,  // FIFO Control Register (write)
        DW_APB_UART_LCR     = 0x0C,  // Line Control Register
        DW_APB_UART_MCR     = 0x10,  // Modem Control Register
        DW_APB_UART_LSR     = 0x14,  // Line Status Register
        DW_APB_UART_MSR     = 0x18,  // Modem Status Register
    };

    // LSR register fields
    enum : size_type {
        DW_APB_UART_LSR_DR_SHIFT = 0,    // Data Ready
        DW_APB_UART_LSR_THRE_SHIFT = 5,  // Transmit Holding Register Empty
    };

    void read(const Agent&, size_type pos, size_type n, pointer result) override {
        // Support 1-byte, 2-byte, and 4-byte accesses (real hardware supports all sizes)
        if (n != 1 && n != 2 && n != 4) {
            throw std::runtime_error("UART: Unsupported access size");
        }

        switch (pos) {
        case DW_APB_UART_RBR_THR: {
            char data = 0;
            if (rx_fd != -1 && fd_read_data_available(rx_fd)) {
                auto res = ::read(rx_fd, &data, 1);
                (void)res;
            }
            write_value_by_size(result, n, static_cast<uint32_t>(static_cast<uint8_t>(data)));
            break;
        }
        case DW_APB_UART_LSR: {
            uint32_t dr = 0;
            if (rx_fd != -1 && fd_read_data_available(rx_fd)) {
                dr = 1;
            }
            uint32_t lsr =
                (dr << DW_APB_UART_LSR_DR_SHIFT) |   // Data Ready in the RBR or the receiver FIFO
                (1  << DW_APB_UART_LSR_THRE_SHIFT);  // Transmit FIFO always empty (ready to accept data)
            write_value_by_size(result, n, lsr);
            break;
        }
        case DW_APB_UART_IER:
        case DW_APB_UART_LCR:
        case DW_APB_UART_MCR:
            // Return stored/default values for control registers
            write_value_by_size(result, n, 0);
            break;
        case DW_APB_UART_IIR:
            // IIR bit 0 = 1 means no interrupt pending
            write_value_by_size(result, n, 0x01);
            break;
        case DW_APB_UART_MSR:
            // Modem status - indicate ready state
            write_value_by_size(result, n, 0xF0);
            break;
        default:
            // Unknown registers return 0
            write_value_by_size(result, n, 0);
            break;
        }
    }

    void write(const Agent&, size_type pos, size_type n, const_pointer source) override {
        // Support 1-byte, 2-byte, and 4-byte accesses (real hardware supports all sizes)
        if (n != 1 && n != 2 && n != 4) {
            throw std::runtime_error("UART: Unsupported access size");
        }

        switch (pos) {
        case DW_APB_UART_RBR_THR: {
            // Extract the byte to write (lower 8 bits regardless of access size)
            uint8_t byte_to_write = static_cast<uint8_t>(read_value_by_size(source, n));

            if ((tx_fd != -1) && (::write(tx_fd, &byte_to_write, 1) < 0)) {
                auto error = std::error_code(errno, std::system_category());
                throw std::system_error(error, "bemu::Uart::write()");
            }
            break;
        }
        case DW_APB_UART_IER:
        case DW_APB_UART_FCR:
        case DW_APB_UART_LCR:
        case DW_APB_UART_MCR:
            // Accept writes to control registers but don't implement functionality
            // (polling mode doesn't need these to work)
            break;
        default:
            // Silently ignore writes to other/unknown registers
            break;
        }
    }

    void init(const Agent&, size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::Uart::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }

    // For exposition only
    int tx_fd = -1;
    int rx_fd = -1;

private:
    static bool fd_read_data_available(int fd) {
        fd_set rfds;
        struct timeval tv;
        tv.tv_sec = 0; //  If both fields of tv are zero, returns immediately
        tv.tv_usec = 0;
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        if (select(fd + 1, &rfds, nullptr, nullptr, &tv) < 0) {
            return false;
        }
        return FD_ISSET(fd, &rfds);
    }

    // Helper function to write a value to result buffer based on access size
    static void write_value_by_size(pointer dest, size_type n, uint32_t value) {
        switch (n) {
        case 1:
            *reinterpret_cast<uint8_t*>(dest) = static_cast<uint8_t>(value);
            break;
        case 2:
            *reinterpret_cast<uint16_t*>(dest) = static_cast<uint16_t>(value);
            break;
        case 4:
            *reinterpret_cast<uint32_t*>(dest) = value;
            break;
        }
    }

    // Helper function to read a value from source buffer based on access size
    static uint32_t read_value_by_size(const_pointer src, size_type n) {
        switch (n) {
        case 1:
            return *reinterpret_cast<const uint8_t*>(src);
        case 2:
            return *reinterpret_cast<const uint16_t*>(src);
        case 4:
            return *reinterpret_cast<const uint32_t*>(src);
        default:
            return 0;
        }
    }
};


} // namespace bemu

#endif // BEMU_UART_H
