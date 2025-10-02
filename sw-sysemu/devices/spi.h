/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_SPI_H
#define BEMU_SPI_H

#include <array>
#include <cstdint>
#include <vector>
#include "literals.h"
#include "memory/memory_error.h"
#include "memory/memory_region.h"
#include "emu_gio.h"

namespace bemu {

template<unsigned long long Base, unsigned long long N, int ID>
struct Spi : public MemoryRegion {
    using addr_type     = typename MemoryRegion::addr_type;
    using size_type     = typename MemoryRegion::size_type;
    using pointer       = typename MemoryRegion::pointer;
    using const_pointer = typename MemoryRegion::const_pointer;

    enum : unsigned long long {
        SSI_RXFLR_ADDRESS = 0x24,
        SSI_SR_ADDRESS = 0x28,
        SSI_DR0_ADDRESS = 0x60,
    };
    static_assert(N == 4_KiB, "bemu::Spi has illegal size");
    std::vector<std::uint32_t> data;

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        uint32_t *result32 = reinterpret_cast<uint32_t *>(result);

        LOG_AGENT(DEBUG, agent, "Spi%d::read(pos=0x%llx)", ID, pos);

        if (n != 4)
            throw memory_error(first() + pos);

        switch(pos)
        {
            case SSI_SR_ADDRESS:
                *result32 = 0x04; /* Emulating Empty FIFO (Bit[2]==1) and busy (BIT[0] == 0) */
                break;
            case SSI_RXFLR_ADDRESS:
                *result32 = 0x01; /* Emulate that RX FIFO has data */
                break;
            default:
                *result32 = 0;
        }
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const uint32_t *source32 = reinterpret_cast<const uint32_t *>(source);

        LOG_AGENT(DEBUG, agent, "Spi%d::write(pos=0x%llx)", ID, pos);

        if (n != 4)
            throw memory_error(first() + pos);

        if(pos == SSI_DR0_ADDRESS)
        {
            data.push_back(*source32);
        }
    }

    void init(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const uint8_t *src = reinterpret_cast<const uint8_t *>(source);
        (void) src;
        LOG_AGENT(DEBUG, agent, "Spi%d::init(pos=0x%llx, n=0x%llx)", ID, pos, n);
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }
};

} // namespace bemu

#endif // BEMU_SPI_H
