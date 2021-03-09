/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
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
        mtimecmp = val;
        interrupt = (mtime >= mtimecmp);
        if (had_interrupt && !interrupt) {
            agent.chip->clear_timer_interrupt(interrupt_shire_mask);
        }
    }

    void clock_tick(const Agent& agent)
    {
        if (++mtime >= mtimecmp) {
            if (!interrupt) {
                agent.chip->raise_timer_interrupt(interrupt_shire_mask);
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
