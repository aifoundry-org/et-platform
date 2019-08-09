#ifndef _EMU_GIO_H
#define _EMU_GIO_H

#include <cinttypes>
#include "testLog.h"
#include "emu_defines.h"


namespace emu {
    extern testLog log;

    extern void lprintf(logLevel level, const char* fmt, ...) __attribute__ ((format (printf, 2, 3)));
}

#define _LOG_THREAD(thread) \
    ((thread) == EMU_IO_SHIRE_SP_THREAD \
        ? (IO_SHIRE_ID * EMU_THREADS_PER_SHIRE) \
        : (thread))

#define _LOG_IMPL(severity, cond, thread, format, ...) do { \
    if ((LOG_##severity >= emu::log.getLogLevel()) && (cond)) \
    { \
        emu::lprintf(LOG_##severity, "[H%u S%u:N%u:C%u:T%u] " format, \
                     unsigned(thread), \
                     unsigned(thread / EMU_THREADS_PER_SHIRE), \
                     unsigned((thread / EMU_THREADS_PER_NEIGH) % EMU_NEIGH_PER_SHIRE), \
                     unsigned((thread / EMU_THREADS_PER_MINION) % EMU_MINIONS_PER_NEIGH), \
                     unsigned(thread % EMU_THREADS_PER_MINION), \
                     ##__VA_ARGS__); \
    } \
} while (0)


#define LOG(severity, format, ...) do { \
    _LOG_IMPL(severity, \
              (minion_only_log < 0) || int32_t(current_thread / EMU_THREADS_PER_MINION) == minion_only_log, \
              _LOG_THREAD(current_thread), format, ##__VA_ARGS__); \
} while (0)


#define LOG_OTHER(severity, thread, format, ...) do { \
    _LOG_IMPL(severity, \
              (minion_only_log < 0) || int32_t(thread / EMU_THREADS_PER_MINION) == minion_only_log, \
              _LOG_THREAD(thread), format, ##__VA_ARGS__); \
} while (0)


#define LOG_ALL_MINIONS(severity, format, ...) do { \
    _LOG_IMPL(severity, true, _LOG_THREAD(current_thread), format, ##__VA_ARGS__); \
} while(0)


#define LOG_NOTHREAD(severity, format, ...) do { \
    if (LOG_##severity >= emu::log.getLogLevel()) \
    { \
        emu::lprintf(LOG_##severity, format, ##__VA_ARGS__); \
    } \
} while (0)


#define LOG_IF(severity, cond, format, ...) do { \
    _LOG_IMPL(severity, \
              ((minion_only_log < 0) || int32_t(current_thread / EMU_THREADS_PER_MINION) == minion_only_log) \
               && (cond), \
               _LOG_THREAD(current_thread), format, ##__VA_ARGS__); \
} while (0)


#define DEBUG_MASK(_MR) LOG(DEBUG, "\tmask = 0x%02lx",(_MR).to_ulong())


extern unsigned current_thread;
extern int32_t minion_only_log;

extern void log_only_minion(int32_t);

#endif // _EMU_GIO_H
