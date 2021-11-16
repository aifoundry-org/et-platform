/*-------------------------------------------------------------------------
* Copyright (C) 2021, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_PVTC_H
#define BEMU_PVTC_H

#include <array>
#include <cstdint>
#include "memory/memory_error.h"
#include "memory/memory_region.h"
#include "literals.h"
#include "emu_gio.h"

namespace bemu {

template<unsigned long long Base, unsigned long long N, int ID>
struct PVTC : public MemoryRegion {
    using addr_type     = typename MemoryRegion::addr_type;
    using size_type     = typename MemoryRegion::size_type;
    using pointer       = typename MemoryRegion::pointer;
    using const_pointer = typename MemoryRegion::const_pointer;

    static_assert(N == 64_KiB, "bemu::PVTC has illegal size");

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        uint32_t *result32 = reinterpret_cast<uint32_t *>(result);

        LOG_AGENT(DEBUG, agent, "PVTC%d::read(pos=0x%llx)", ID, pos);

        if (n != 4)
            throw memory_error(first() + pos);

        *result32 = pvtc_read(pos);
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const uint32_t *source32 = reinterpret_cast<const uint32_t *>(source);
        (void) source32;

        LOG_AGENT(DEBUG, agent, "PVTC%d::write(pos=0x%llx)", ID, pos);

        if (n != 4)
            throw memory_error(first() + pos);

        pvtc_write(pos, source);
    }

    void init(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const uint8_t *src = reinterpret_cast<const uint8_t *>(source);
        (void) src;
        LOG_AGENT(DEBUG, agent, "PVTC%d::init(pos=0x%llx, n=0x%llx)", ID, pos, n);
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }

protected:
    enum {
        PVT_COMP_ID = 0x00,
        PVT_ID_NUM = 0x08,
        PVT_TM_SCRATCH = 0x0C
    };

    uint32_t pvtc_tm_scratch = 0;

    void pvtc_write(size_type reg_addr, const_pointer wr_data) {
        uint32_t* data = (uint32_t*)wr_data;
        switch (reg_addr) {
        case PVT_TM_SCRATCH:
            pvtc_tm_scratch = *data;
            break;
        default:
            break;
        }
    }

    uint32_t pvtc_read(size_type reg_addr) {
        switch (reg_addr) {
        case PVT_COMP_ID: return 0x9b487062ul;
        case PVT_ID_NUM: return ID;
        case PVT_TM_SCRATCH: return pvtc_tm_scratch;
        default: return 0;
        }
    }

};

} // namespace bemu

#endif // BEMU_PVTC_H
