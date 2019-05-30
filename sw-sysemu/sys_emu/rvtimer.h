#ifndef _SYSEMU_RVTIMER_H_
#define _SYSEMU_RVTIMER_H_

#include <cinttypes>

struct RVTimer
{
    enum {
        tick_freq = 100,
    };

    void reset() {
        mtime = 0;
        mtimecmp = std::numeric_limits<uint64_t>::max();
        active = false;
        interrupt = false;
        clear = false;
    }

    void update(uint64_t cycle) {
        clear = false;
        if ((cycle % tick_freq) == 0) {
            if (++mtime >= mtimecmp) {
                interrupt = true;
            }
        }
    }

    bool is_active() const {
        return active;
    }

    bool interrupt_pending() const {
        return interrupt;
    }

    bool clear_pending() const {
        return clear;
    }

    void write_mtime(uint64_t val) {
        mtime = val;
    }

    void write_mtimecmp(uint64_t val) {
        bool had_interrupt = interrupt;
        mtimecmp = val;
        interrupt = (mtime >= mtimecmp);
        if (had_interrupt)
            clear = !interrupt;
        active = true;
    }

    uint64_t read_mtime() const {
        return mtime;
    }

    uint64_t read_mtimecmp() const {
        return mtimecmp;
    }

private:
    uint64_t mtime;
    uint64_t mtimecmp;
    bool interrupt;
    bool clear;
    bool active;
};

#endif
