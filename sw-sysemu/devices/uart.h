/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
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
        DW_APB_UART_RBR_THR = 0x00,
        DW_APB_UART_LSR = 0x14,
    };

    // LSR register fields
    enum : size_type {
        DW_APB_UART_LSR_DR_SHIFT = 0,
        DW_APB_UART_LSR_THRE_SHIFT = 5,
    };

    void read(const Agent&, size_type pos, size_type n, pointer result) override {
        (void) n;
        assert(n == 4);

        switch (pos) {
        case DW_APB_UART_RBR_THR: {
            char data = 0;
            if (rx_fd != -1 && fd_read_data_available(rx_fd)) {
                ::read(rx_fd, &data, 1);
            }
           *reinterpret_cast<uint32_t*>(result) = data;
            break;
        }
        case DW_APB_UART_LSR: {
            uint32_t dr = 0;
            if (rx_fd != -1 && fd_read_data_available(rx_fd)) {
                dr = 1;
            }
            *reinterpret_cast<uint32_t*>(result) =
                (dr << DW_APB_UART_LSR_DR_SHIFT) |  // Data Ready in the RBR or the receiver FIFO
                (0  << DW_APB_UART_LSR_THRE_SHIFT); // Transmit FIFO always empty
            break;
        }
        default:
            *reinterpret_cast<uint32_t*>(result) = 0;
            break;
        }
    }

    void write(const Agent&, size_type pos, size_type n, const_pointer source) override {
        (void) n;
        assert(n == 4);

        switch (pos) {
        case DW_APB_UART_RBR_THR:
            if ((tx_fd != -1) && (::write(tx_fd, source, 1) < 0)) {
                auto error = std::error_code(errno, std::system_category());
                throw std::system_error(error, "bemu::Uart::write()");
            }
            break;
        default:
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
};


} // namespace bemu

#endif // BEMU_UART_H
