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
        PVT_TM_SCRATCH = 0x0C,
        PVT_TS_00_SDIF_DONE = 0x0D4,
        PVT_TS_00_SDIF_DATA = 0x0D8,
        PVT_TS_01_SDIF_DONE = 0x114,
        PVT_TS_01_SDIF_DATA = 0x118,
        PVT_TS_02_SDIF_DONE = 0x154,
        PVT_TS_02_SDIF_DATA = 0x158,
        PVT_TS_03_SDIF_DONE = 0x194,
        PVT_TS_03_SDIF_DATA = 0x198,
        PVT_TS_04_SDIF_DONE = 0x1D4,
        PVT_TS_04_SDIF_DATA = 0x1D8,
        PVT_TS_05_SDIF_DONE = 0x214,
        PVT_TS_05_SDIF_DATA = 0x218,
        PVT_TS_06_SDIF_DONE = 0x254,
        PVT_TS_06_SDIF_DATA = 0x258,
        PVT_TS_07_SDIF_DONE = 0x294,
        PVT_TS_07_SDIF_DATA = 0x298,
        PVT_VM_00_SDIF_DONE = 0xa34,
        PVT_VM_00_CH_00_SDIF_DATA = 0xa40,
        PVT_VM_00_CH_01_SDIF_DATA = 0xa44,
        PVT_VM_00_CH_02_SDIF_DATA = 0xa48,
        PVT_VM_00_CH_03_SDIF_DATA = 0xa4C,
        PVT_VM_00_CH_04_SDIF_DATA = 0xa50,
        PVT_VM_00_CH_05_SDIF_DATA = 0xa54,
        PVT_VM_00_CH_06_SDIF_DATA = 0xa58,
        PVT_VM_00_CH_07_SDIF_DATA = 0xa5C,
        PVT_VM_00_CH_08_SDIF_DATA = 0xa60,
        PVT_VM_00_CH_09_SDIF_DATA = 0xa64,
        PVT_VM_00_CH_10_SDIF_DATA = 0xa68,
        PVT_VM_00_CH_11_SDIF_DATA = 0xa6C,
        PVT_VM_00_CH_12_SDIF_DATA = 0xa70,
        PVT_VM_00_CH_13_SDIF_DATA = 0xa74,
        PVT_VM_00_CH_14_SDIF_DATA = 0xa78,
        PVT_VM_00_CH_15_SDIF_DATA = 0xa7c,
        PVT_VM_01_SDIF_DONE = 0xc34,
        PVT_VM_01_CH_00_SDIF_DATA = 0xc40,
        PVT_VM_01_CH_01_SDIF_DATA = 0xc44,
        PVT_VM_01_CH_02_SDIF_DATA = 0xc48,
        PVT_VM_01_CH_03_SDIF_DATA = 0xc4C,
        PVT_VM_01_CH_04_SDIF_DATA = 0xc50,
        PVT_VM_01_CH_05_SDIF_DATA = 0xc54,
        PVT_VM_01_CH_06_SDIF_DATA = 0xc58,
        PVT_VM_01_CH_07_SDIF_DATA = 0xc5C,
        PVT_VM_01_CH_08_SDIF_DATA = 0xc60,
        PVT_VM_01_CH_09_SDIF_DATA = 0xc64,
        PVT_VM_01_CH_10_SDIF_DATA = 0xc68,
        PVT_VM_01_CH_11_SDIF_DATA = 0xc6C,
        PVT_VM_01_CH_12_SDIF_DATA = 0xc70,
        PVT_VM_01_CH_13_SDIF_DATA = 0xc74,
        PVT_VM_01_CH_14_SDIF_DATA = 0xc78,
        PVT_VM_01_CH_15_SDIF_DATA = 0xc7c
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
        case PVT_TS_00_SDIF_DONE:
        case PVT_TS_01_SDIF_DONE:
        case PVT_TS_02_SDIF_DONE:
        case PVT_TS_03_SDIF_DONE:
        case PVT_TS_04_SDIF_DONE:
        case PVT_TS_05_SDIF_DONE:
        case PVT_TS_06_SDIF_DONE:
        case PVT_TS_07_SDIF_DONE:
        case PVT_VM_00_SDIF_DONE:
        case PVT_VM_01_SDIF_DONE: return 0x1u;
        case PVT_TS_00_SDIF_DATA:
        case PVT_TS_01_SDIF_DATA:
        case PVT_TS_02_SDIF_DATA:
        case PVT_TS_03_SDIF_DATA:
        case PVT_TS_04_SDIF_DATA:
        case PVT_TS_05_SDIF_DATA:
        case PVT_TS_06_SDIF_DATA:
        case PVT_TS_07_SDIF_DATA: return 0x0799u;
        case PVT_VM_00_CH_00_SDIF_DATA:
        case PVT_VM_00_CH_01_SDIF_DATA:
        case PVT_VM_00_CH_02_SDIF_DATA:
        case PVT_VM_00_CH_03_SDIF_DATA:
        case PVT_VM_00_CH_04_SDIF_DATA:
        case PVT_VM_00_CH_05_SDIF_DATA:
        case PVT_VM_00_CH_06_SDIF_DATA:
        case PVT_VM_00_CH_07_SDIF_DATA:
        case PVT_VM_00_CH_08_SDIF_DATA:
        case PVT_VM_00_CH_09_SDIF_DATA:
        case PVT_VM_00_CH_10_SDIF_DATA:
        case PVT_VM_00_CH_11_SDIF_DATA:
        case PVT_VM_00_CH_12_SDIF_DATA:
        case PVT_VM_00_CH_13_SDIF_DATA:
        case PVT_VM_00_CH_14_SDIF_DATA:
        case PVT_VM_00_CH_15_SDIF_DATA:
        case PVT_VM_01_CH_00_SDIF_DATA:
        case PVT_VM_01_CH_01_SDIF_DATA:
        case PVT_VM_01_CH_02_SDIF_DATA:
        case PVT_VM_01_CH_03_SDIF_DATA:
        case PVT_VM_01_CH_04_SDIF_DATA:
        case PVT_VM_01_CH_05_SDIF_DATA:
        case PVT_VM_01_CH_06_SDIF_DATA:
        case PVT_VM_01_CH_07_SDIF_DATA:
        case PVT_VM_01_CH_08_SDIF_DATA:
        case PVT_VM_01_CH_09_SDIF_DATA:
        case PVT_VM_01_CH_10_SDIF_DATA:
        case PVT_VM_01_CH_11_SDIF_DATA:
        case PVT_VM_01_CH_12_SDIF_DATA:
        case PVT_VM_01_CH_13_SDIF_DATA:
        case PVT_VM_01_CH_14_SDIF_DATA:
        case PVT_VM_01_CH_15_SDIF_DATA: return 0x1C39u;
        default: return 0;
        }
    }

};

} // namespace bemu

#endif // BEMU_PVTC_H
