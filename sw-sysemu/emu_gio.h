#ifndef _EMU_GIO_H
#define _EMU_GIO_H

#include "testLog.h"
#include "emu_defines.h"


namespace emu {
    extern testLog log;

    extern void lprintf(logLevel level, const char* fmt, ...) __attribute__ ((format (printf, 2, 3)));
}


#define _LOG_IMPL(severity, cond, thread, format, ...) do { \
    if ((LOG_##severity >= emu::log.getLogLevel()) && (cond)) \
    { \
        emu::lprintf(LOG_##severity, "[H%u S%u:N%u:M%u:T%u] " format, \
                     thread, \
                     thread / EMU_THREADS_PER_SHIRE, \
                     (thread / EMU_THREADS_PER_NEIGH) % EMU_NEIGH_PER_SHIRE, \
                     (thread / EMU_THREADS_PER_MINION) % EMU_MINIONS_PER_NEIGH, \
                     thread % EMU_THREADS_PER_MINION, \
                     ##__VA_ARGS__); \
    } \
} while (0)


#define LOG(severity, format, ...) do { \
    _LOG_IMPL(severity, \
              (minion_only_log < 0) || int32_t(current_thread / EMU_THREADS_PER_MINION) == minion_only_log, \
              current_thread, format, ##__VA_ARGS__); \
} while (0)


#define LOG_OTHER(severity, thread, format, ...) do { \
    _LOG_IMPL(severity, \
              (minion_only_log < 0) || int32_t(thread / EMU_THREADS_PER_MINION) == minion_only_log, \
              thread, format, ##__VA_ARGS__); \
} while (0)


#define LOG_ALL_MINIONS(severity, format, ...) do { \
    _LOG_IMPL(severity, true, current_thread, format, ##__VA_ARGS__); \
} while(0)


#define LOG_NOTHREAD(severity, format, ...) do { \
    if (LOG_##severity >= emu::log.getLogLevel()) \
    { \
        emu::lprintf(LOG_##severity, format, ##__VA_ARGS__); \
    } \
} while (0)


#define DEBUG_MASK(_MR) LOG(DEBUG, "\tmask = 0x%02lx",(_MR).b.to_ulong())


extern uint32_t current_thread;
extern int32_t minion_only_log;

extern void log_only_minion(int32_t);

#endif // _EMU_GIO_H
