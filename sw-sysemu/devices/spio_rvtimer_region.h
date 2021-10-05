/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_SPIO_RVTIMER_REGION_H
#define BEMU_SPIO_RVTIMER_REGION_H

#include <cstdint>
#include "emu_defines.h"
#include "devices/rvtimer.h"
#include "memory/memory_region.h"

namespace bemu {

template<unsigned long long Base, unsigned long long N>
struct SpioRVTimerRegion : public MemoryRegion {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    enum : size_type {
        RVTIMER_REG_MTIME    = 0,
        RVTIMER_REG_MTIMECMP = 8,
    };

    void read(const Agent&, size_type pos, size_type n, pointer result) override {
        uint64_t *result64 = reinterpret_cast<uint64_t *>(result);

        if (n < 8)
            throw memory_error(first() + pos);

        switch (pos) {
        case RVTIMER_REG_MTIME:
            *result64 = rvtimer.read_mtime();
            break;
        case RVTIMER_REG_MTIMECMP:
            *result64 = rvtimer.read_mtimecmp();
            break;
        }
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const uint64_t *source64 = reinterpret_cast<const uint64_t *>(source);

        if (n < 8)
            throw memory_error(first() + pos);

        switch (pos) {
        case RVTIMER_REG_MTIME:
            rvtimer.write_mtime(agent, *source64);
            break;
        case RVTIMER_REG_MTIMECMP:
            rvtimer.write_mtimecmp(agent, *source64);
            break;
        }
    }

    void init(const Agent&, size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::SpioRVTimerRegion::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }

    RVTimer<1ull << EMU_IO_SHIRE_SP> rvtimer;
};

} // namespace bemu

#endif // BEMU_SPIO_RVTIMER_REGION_H
