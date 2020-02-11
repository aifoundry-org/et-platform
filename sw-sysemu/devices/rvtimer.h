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

#include <cinttypes>
#include <limits>
#include "emu_defines.h"

struct RVTimer
{
    enum {
        tick_freq = 100,
    };

    RVTimer() {
        reset();
    }

    void reset() {
        mtime = 0;
        mtimecmp = std::numeric_limits<uint64_t>::max();
        interrupt = false;
    }

    void update(uint64_t cycle);

    bool is_active() const {
        return mtimecmp != std::numeric_limits<uint64_t>::max();
    }

    uint64_t read_mtime() const {
        return mtime;
    }

    uint64_t read_mtimecmp() const {
        return mtimecmp;
    }

    void write_mtime(uint64_t val) {
        mtime = val;
    }

    void write_mtimecmp(uint64_t val);

private:
    uint64_t mtime;
    uint64_t mtimecmp;
    bool interrupt;
};

#endif
