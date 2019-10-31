/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

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
