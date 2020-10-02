/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_SYSREG_REGION_H
#define BEMU_SYSREG_REGION_H

#include <cassert>
#include <cstdint>
#include "esrs.h"
#include "memory_region.h"
#include "devices/rvtimer.h"

namespace bemu {


template <unsigned long long Base, unsigned long long N>
struct SysregRegion : public MemoryRegion {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    static_assert(Base == ESR_REGION_BASE,
                  "bemu::SysregRegion has illegal base address");
    static_assert(N == ESR_REGION_SIZE,
                  "bemu::SysregRegion has illegal size");

    void read(const Agent& agent, size_type pos, size_type count, pointer result) override {
        assert(count == 8);
        *reinterpret_cast<uint64_t*>(result) = esr_read(agent, first() + pos);
    }

    void write(const Agent& agent, size_type pos, size_type count, const_pointer source) override {
        assert(count == 8);
        esr_write(agent, first() + pos, *reinterpret_cast<const uint64_t*>(source));
    }

    void init(const Agent&, size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::SysregRegion::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(std::ostream&, size_type, size_type) const override { }

    RVTimer<(1ull << EMU_NUM_MINION_SHIRES) - 1> ioshire_pu_rvtimer;
};


} // namesapce bemu

#endif // BEMU_SYSREG_REGION_H
