/*-------------------------------------------------------------------------
* Copyright (C) 2021, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_DW_APB_TIMERS_H
#define BEMU_DW_APB_TIMERS_H

#include <array>
#include <cstdint>
#include "memory/memory_error.h"
#include "memory/memory_region.h"

namespace bemu {

template<unsigned long long Base, unsigned long long N>
struct DW_apb_timers : public MemoryRegion {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    static_assert(N == 4_KiB, "bemu::DW_apb_timers has illegal size");

    enum : size_type {
        NUM_TIMERS = 8,
    };

    // Stride in bytes between registers of different timers
    enum : size_type {
        REG_STRIDE = 0x14u,
    };

    // Registers
    enum : size_type {
        LOADCOUNT_OFFSET = 0x0u,
        CURRENTVAL_OFFSET = 0x4u,
        CONTROLREG_OFFSET = 0x8u,
        EOI_OFFSET = 0xCu,
        INTSTAT_OFFSET = 0x10u,
    };

    // TimerNControlReg bit fields
    #define CONTROLREG_TIMER_ENABLE_GET(x)         ((x) & 0x00000001ul)
    #define CONTROLREG_TIMER_ENABLE_SET(x)         ((x) & 0x00000001ul)
    #define CONTROLREG_TIMER_MODE_GET(x)           (((x) & 0x00000002ul) >> 1)
    #define CONTROLREG_TIMER_MODE_SET(x)           (((x) << 1) & 0x00000002ul)
    #define CONTROLREG_TIMER_INTERRUPT_MASK_GET(x) (((x) & 0x00000004ul) >> 2)
    #define CONTROLREG_TIMER_INTERRUPT_MASK_SET(x) (((x) << 2) & 0x00000004ul)

    void read(const Agent&, size_type pos, size_type n, pointer result) override {
        uint32_t *result32 = reinterpret_cast<uint32_t *>(result);

        LOG_NOTHREAD(DEBUG, "DW_apb_timers::read(pos=0x%llx)", pos);

        if (n != 4)
            throw memory_error(first() + pos);

        if (pos >= (NUM_TIMERS * REG_STRIDE))
            return;

        size_type timer = pos / REG_STRIDE;
        size_type timer_pos = pos % REG_STRIDE;

        switch (timer_pos) {
        case LOADCOUNT_OFFSET:
            *result32 = loadcount[timer];
            break;
        case CURRENTVAL_OFFSET:
            *result32 = currentvalue[timer];
            break;
        case CONTROLREG_OFFSET:
            *result32 = controlreg[timer];
            break;
        case INTSTAT_OFFSET:
            *result32 = intstatus[timer];
            break;
        }
    }

    void write(const Agent&, size_type pos, size_type n, const_pointer source) override {
        const uint32_t *source32 = reinterpret_cast<const uint32_t *>(source);

        LOG_NOTHREAD(DEBUG, "DW_apb_timers::write(pos=0x%llx)", pos);

        if (n != 4)
            throw memory_error(first() + pos);

        if (pos >= (NUM_TIMERS * REG_STRIDE))
            return;

        size_type timer = pos / REG_STRIDE;
        size_type timer_pos = pos % REG_STRIDE;

        switch (timer_pos) {
        case LOADCOUNT_OFFSET:
            loadcount[timer] = *source32;
            break;
        case CURRENTVAL_OFFSET:
            currentvalue[timer] = *source32;
            break;
        case CONTROLREG_OFFSET:
            controlreg[timer] = *source32;
            break;
        case INTSTAT_OFFSET:
            intstatus[timer] = *source32;
            break;
        }
    }

    // Clock tick
    void update(System& chip, int64_t cycle) {
        (void) chip;
        (void) cycle;
    }

    void init(const Agent&, size_type pos, size_type n, const_pointer source) override {
        const uint8_t *src = reinterpret_cast<const uint8_t *>(source);
        (void) src;
        LOG_NOTHREAD(DEBUG, "DW_apb_timers::init(pos=0x%llx, n=0x%llx)", pos, n);
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }

private:
    std::array<uint32_t, NUM_TIMERS> loadcount;
    std::array<uint32_t, NUM_TIMERS> loadcount2;
    std::array<uint32_t, NUM_TIMERS> currentvalue;
    std::array<uint32_t, NUM_TIMERS> controlreg;
    std::array<uint32_t, NUM_TIMERS> intstatus;
};

} // namespace bemu

#endif // BEMU_DW_APB_TIMERS_H
