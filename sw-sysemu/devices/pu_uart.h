/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_PU_UART_H
#define BEMU_PU_UART_H

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
struct PU_Uart : public MemoryRegion
{
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    // Registers from DW_apb_uart.csr
    enum : size_type {
        DW_APB_UART_RBR         = 0x0,
        DW_APB_UART_IER         = 0x4,
        DW_APB_UART_IIR         = 0x8,
        DW_APB_UART_LCR         = 0xc,
        DW_APB_UART_MCR         = 0x10,
        DW_APB_UART_LSR         = 0x14,
        DW_APB_UART_MSR         = 0x18,
        DW_APB_UART_SCR         = 0x1c,
        DW_APB_UART_SRBR0       = 0x30,
        DW_APB_UART_SRBR1       = 0x34,
        DW_APB_UART_SRBR2       = 0x38,
        DW_APB_UART_SRBR3       = 0x3c,
        DW_APB_UART_SRBR4       = 0x40,
        DW_APB_UART_SRBR5       = 0x44,
        DW_APB_UART_SRBR6       = 0x48,
        DW_APB_UART_SRBR7       = 0x4c,
        DW_APB_UART_SRBR8       = 0x50,
        DW_APB_UART_SRBR9       = 0x54,
        DW_APB_UART_SRBR10      = 0x58,
        DW_APB_UART_SRBR11      = 0x5c,
        DW_APB_UART_SRBR12      = 0x60,
        DW_APB_UART_SRBR13      = 0x64,
        DW_APB_UART_SRBR14      = 0x68,
        DW_APB_UART_SRBR15      = 0x6c,
        DW_APB_UART_FAR         = 0x70,
        DW_APB_UART_TFR         = 0x74,
        DW_APB_UART_RFW         = 0x78,
        DW_APB_UART_USR         = 0x7c,
        DW_APB_UART_TFL         = 0x80,
        DW_APB_UART_RFL         = 0x84,
        DW_APB_UART_SRR         = 0x88,
        DW_APB_UART_SRTS        = 0x8c,
        DW_APB_UART_SBCR        = 0x90,
        DW_APB_UART_SDMAM       = 0x94,
        DW_APB_UART_SFE         = 0x98,
        DW_APB_UART_SRT         = 0x9c,
        DW_APB_UART_STET        = 0xa0,
        DW_APB_UART_HTX         = 0xa4,
        DW_APB_UART_DMASA       = 0xa8,
        DW_APB_UART_DLF         = 0xc0,
        DW_APB_UART_TIMEOUT_RST = 0xd4,
        DW_APB_UART_CPR         = 0xf4,
        DW_APB_UART_UCV         = 0xf8,
        DW_APB_UART_CTR         = 0xfc
    };

    void read(size_type pos, size_type n, pointer result) override {
        switch (pos) {
        case DW_APB_UART_LSR:
            assert(n == 4);
            *reinterpret_cast<uint32_t*>(result) = 0;
            break;
        case DW_APB_UART_RBR:
        case DW_APB_UART_IER:
        case DW_APB_UART_IIR:
        case DW_APB_UART_LCR:
        case DW_APB_UART_MCR:
        case DW_APB_UART_MSR:
        case DW_APB_UART_SCR:
        case DW_APB_UART_SRBR0:
        case DW_APB_UART_SRBR1:
        case DW_APB_UART_SRBR2:
        case DW_APB_UART_SRBR3:
        case DW_APB_UART_SRBR4:
        case DW_APB_UART_SRBR5:
        case DW_APB_UART_SRBR6:
        case DW_APB_UART_SRBR7:
        case DW_APB_UART_SRBR8:
        case DW_APB_UART_SRBR9:
        case DW_APB_UART_SRBR10:
        case DW_APB_UART_SRBR11:
        case DW_APB_UART_SRBR12:
        case DW_APB_UART_SRBR13:
        case DW_APB_UART_SRBR14:
        case DW_APB_UART_SRBR15:
        case DW_APB_UART_FAR:
        case DW_APB_UART_TFR:
        case DW_APB_UART_RFW:
        case DW_APB_UART_USR:
        case DW_APB_UART_TFL:
        case DW_APB_UART_RFL:
        case DW_APB_UART_SRR:
        case DW_APB_UART_SRTS:
        case DW_APB_UART_SBCR:
        case DW_APB_UART_SDMAM:
        case DW_APB_UART_SFE:
        case DW_APB_UART_SRT:
        case DW_APB_UART_STET:
        case DW_APB_UART_HTX:
        case DW_APB_UART_DMASA:
        case DW_APB_UART_DLF:
        case DW_APB_UART_TIMEOUT_RST:
        case DW_APB_UART_CPR:
        case DW_APB_UART_UCV:
        case DW_APB_UART_CTR:
            break;
        default:
            throw memory_error(first() + pos);
        }
    }

    void write(size_type pos, size_type n, const_pointer source) override {
        switch (pos) {
        case DW_APB_UART_RBR:
            assert(n == 4);
            if ((fd != -1) && (::write(fd, source, 1) < 0)) {
                auto error = std::error_code(errno, std::system_category());
                throw std::system_error(error, "bemu::PU_Uart::write()");
            }
            break;
        case DW_APB_UART_IER:
        case DW_APB_UART_IIR:
        case DW_APB_UART_LCR:
        case DW_APB_UART_MCR:
        case DW_APB_UART_LSR:
        case DW_APB_UART_MSR:
        case DW_APB_UART_SCR:
        case DW_APB_UART_SRBR0:
        case DW_APB_UART_SRBR1:
        case DW_APB_UART_SRBR2:
        case DW_APB_UART_SRBR3:
        case DW_APB_UART_SRBR4:
        case DW_APB_UART_SRBR5:
        case DW_APB_UART_SRBR6:
        case DW_APB_UART_SRBR7:
        case DW_APB_UART_SRBR8:
        case DW_APB_UART_SRBR9:
        case DW_APB_UART_SRBR10:
        case DW_APB_UART_SRBR11:
        case DW_APB_UART_SRBR12:
        case DW_APB_UART_SRBR13:
        case DW_APB_UART_SRBR14:
        case DW_APB_UART_SRBR15:
        case DW_APB_UART_FAR:
        case DW_APB_UART_TFR:
        case DW_APB_UART_RFW:
        case DW_APB_UART_USR:
        case DW_APB_UART_TFL:
        case DW_APB_UART_RFL:
        case DW_APB_UART_SRR:
        case DW_APB_UART_SRTS:
        case DW_APB_UART_SBCR:
        case DW_APB_UART_SDMAM:
        case DW_APB_UART_SFE:
        case DW_APB_UART_SRT:
        case DW_APB_UART_STET:
        case DW_APB_UART_HTX:
        case DW_APB_UART_DMASA:
        case DW_APB_UART_DLF:
        case DW_APB_UART_TIMEOUT_RST:
        case DW_APB_UART_CPR:
        case DW_APB_UART_UCV:
        case DW_APB_UART_CTR:
            break;
        default:
            throw memory_error(first() + pos);
        }
    }

    void init(size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::PU_Uart::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(std::ostream&, size_type, size_type) const override { }

    // For exposition only
    int fd = -1;
};


} // namespace bemu

#endif // BEMU_PU_UART_H
