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
#ifdef SYS_EMU
#include "sys_emu.h"
#endif

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

    void read(const Agent&, size_type pos, size_type n, pointer result) override {
        uint32_t *result32 = reinterpret_cast<uint32_t *>(result);

        LOG_NOTHREAD(DEBUG, "PcieNoPcieEsrRegion::read(pos=0x%llx)", pos);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        default:
          *result32 = 0;
          break;
        }
    }

    void write(const Agent&, size_type pos, size_type n, const_pointer source) override {
        const uint32_t *source32 = reinterpret_cast<const uint32_t *>(source);
        (void) source32;

        LOG_NOTHREAD(DEBUG, "PcieNoPcieEsrRegion::write(pos=0x%llx)", pos);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        case MSI_TX_VEC:
#ifdef SYS_EMU
            if (*source32 != 0) {
                if (sys_emu::get_api_communicate())
                    sys_emu::get_api_communicate()->raise_host_interrupt(*source32);
                else
                    LOG_NOTHREAD(WARN, "%s", "API Communicate is NULL!");
            }
#endif
            break;
        }
    }

    void init(const Agent&, size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::PcieNoPcieEsrRegion::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(std::ostream&, size_type, size_type) const override { }
};

} // namespace bemu

#endif // BEMU_PCIE_NOPCIE_ESR_H
