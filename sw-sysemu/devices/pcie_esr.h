/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_PCIE_ESR_H
#define BEMU_PCIE_ESR_H

#include <array>
#include <cstdint>
#include "memory/memory_error.h"
#include "memory/memory_region.h"
#include "emu_gio.h"

namespace bemu {

template<unsigned long long Base, unsigned long long N>
struct PcieEsr : public MemoryRegion {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    enum : unsigned long long {
        PSHIRE_CTRL  = 0x00,
        PSHIRE_RESET = 0x04,
        PSHIRE_STAT  = 0x08,
    };

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        uint32_t *result32 = reinterpret_cast<uint32_t *>(result);

        LOG_AGENT(DEBUG, agent, "PcieEsr::read(pos=0x%llx)", pos);

        if (n < 4)
            throw memory_error(first() + pos);

        switch (pos) {
        case PSHIRE_STAT:
            *result32 = (1u << 5); /* bit[5]: perst_n: Status of PERST_N pin */
            break;
        default:
            *result32 = 0;
            break;
        }
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const uint32_t *source32 = reinterpret_cast<const uint32_t *>(source);
        (void) source32;

        LOG_AGENT(DEBUG, agent, "PcieEsr::write(pos=0x%llx)", pos);

        if (n < 4)
            throw memory_error(first() + pos);
    }

    void init(const Agent&, size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::PcieEsr::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }
};


} // namespace bemu

#endif // BEMU_PCIE_ESR_H
