#ifndef BEMU_PU_UART_H
#define BEMU_PU_UART_H

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cinttypes>
#include "memory/memory_region.h"
#include <cstdio>

namespace bemu {


template <unsigned long long Base, size_t N>
struct PU_Uart : public MemoryRegion {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    // Registers from DW_apb_uart.csr
    enum : unsigned long long {
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

    PU_Uart() {
        stream = nullptr;
    }

    void read(size_type offset, size_type count, pointer result) const override {
        (void) offset;
        (void) count;
        (void) result;

        //printf("PU_Uart read: offset: 0x%" PRIx64 ", count: %" PRId64 "\n", offset, count);

        switch (offset) {
        case DW_APB_UART_LSR:
            *reinterpret_cast<uint32_t *>(result) = 0;
            break;
        }
    }

    void write(size_type offset, size_type count, const_pointer source) override {
        (void) offset;
        (void) count;
        (void) source;

        //printf("PU_Uart write: offset: 0x%" PRIx64 ", count: %" PRId64 "\n", offset, count);

        switch (offset) {
        case DW_APB_UART_RBR:
            stream_write(*reinterpret_cast<const char *>(source));
            break;
        }
    }

    void init(size_type, size_type, const_pointer) override {
        std::runtime_error("bemu::PU_Uart::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(std::ostream&, size_type, size_type) const override { }

    void set_stream(std::ostream *s) {
        stream = s;
    }

private:
    std::ostream *stream;

    void stream_write(char c) {
        if (stream)
            *stream << c;
    }

};


} // namesapce bemu

#endif // BEMU_PU_UART_H
