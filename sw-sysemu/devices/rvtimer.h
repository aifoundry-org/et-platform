/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef BEMU_RVTIMER_H
#define BEMU_RVTIMER_H

#include <cstdint>
#include <limits>
#include "agent.h"
#include "system.h"

namespace bemu {


template <uint64_t interrupt_shire_mask>
struct RVTimer
{
    RVTimer() {
        reset();
    }

    void reset() {
        mtime = 0;
        mtimecmp = std::numeric_limits<uint64_t>::max();
        interrupt = false;
    }

    bool is_active() const {
        return mtimecmp != std::numeric_limits<uint64_t>::max();
    }

    uint64_t read_mtime() const {
        return mtime;
    }

    void write_mtime(const Agent&, uint64_t val) {
        mtime = val;
    }

    uint64_t read_mtimecmp() const {
        return mtimecmp;
    }

    void write_mtimecmp(const Agent& agent, uint64_t val)
    {
        bool had_interrupt = interrupt;
        mtimecmp = mtime+val;
        interrupt = (mtime >= mtimecmp);
        if (had_interrupt && !interrupt) {
            for (unsigned shire = 0; shire < EMU_NUM_SHIRES; ++shire) {
                if ((interrupt_shire_mask >> shire) & 1) {
                    agent.chip->clear_machine_timer_interrupt(shire);
                }
            }
        }
    }

    void clock_tick(const Agent& agent)
    {
        if (++mtime >= mtimecmp) {
            if (!interrupt) {
                for (unsigned shire = 0; shire < EMU_NUM_SHIRES; ++shire) {
                    if ((interrupt_shire_mask >> shire) & 1) {
                        agent.chip->raise_machine_timer_interrupt(shire);
                    }
                }
                interrupt = true;
            }
        }
    }

private:
    uint64_t mtime;
    uint64_t mtimecmp;
    bool interrupt;
};


} // namespace bemu

#endif
