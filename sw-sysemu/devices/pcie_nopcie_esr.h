/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_PCIE_NOPCIE_ESR_H
#define BEMU_PCIE_NOPCIE_ESR_H

#include <array>
#include <cstdint>
#include "memory/memory_error.h"
#include "memory/memory_region.h"
#include "system.h"
#include "emu_gio.h"

namespace bemu {

template<unsigned long long Base, unsigned long long N>
struct PcieNoPcieEsrRegion : public MemoryRegion {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    enum : unsigned long long {
        MSI_TX_VEC = 0x18,
    };

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        uint32_t *result32 = reinterpret_cast<uint32_t *>(result);

        LOG_AGENT(DEBUG, agent, "PcieNoPcieEsrRegion::read(pos=0x%llx)", pos);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        default:
          *result32 = 0;
          break;
        }
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const uint32_t *source32 = reinterpret_cast<const uint32_t *>(source);

        LOG_AGENT(DEBUG, agent, "PcieNoPcieEsrRegion::write(pos=0x%llx)", pos);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        case MSI_TX_VEC:
            agent.chip->raise_host_interrupt(*source32);
            break;
        }
    }

    void init(const Agent&, size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::PcieNoPcieEsrRegion::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }
};

} // namespace bemu

#endif // BEMU_PCIE_NOPCIE_ESR_H
