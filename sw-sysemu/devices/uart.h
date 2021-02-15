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
struct Uart : public MemoryRegion
{
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    // Registers from DW_apb_uart.csr
    enum : size_type {
        DW_APB_UART_RBR = 0x00,
        DW_APB_UART_LSR = 0x14,
    };

    void read(const Agent&, size_type pos, size_type n, pointer result) override {
        (void) n;
        switch (pos) {
        case DW_APB_UART_LSR:
            assert(n == 4);
            *reinterpret_cast<uint32_t*>(result) = 0;
            break;
        default:
            *reinterpret_cast<uint32_t*>(result) = 0;
            break;
        }
    }

    void write(const Agent&, size_type pos, size_type n, const_pointer source) override {
        (void) n;
        switch (pos) {
        case DW_APB_UART_RBR:
            assert(n == 4);
            if ((fd != -1) && (::write(fd, source, 1) < 0)) {
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
    int fd = -1;
};


} // namespace bemu

#endif // BEMU_UART_H
