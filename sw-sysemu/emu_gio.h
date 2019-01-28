#ifndef _EMU_GIO_H
#define _EMU_GIO_H

#include <cstdio>
#include "testLog.h"
#include "emu_defines.h"


#define _LOG_IMPL(severity, cond, thread, format, ...) do { \
    extern thread_local char emu_log_buffer[]; \
    if ((LOG_##severity >= emu_log().getLogLevel()) && (cond)) \
    { \
        (void) snprintf(emu_log_buffer, 4096, "[H%u S%u:N%u:M%u:T%u] " format, \
                        thread, \
                        thread / EMU_THREADS_PER_SHIRE, \
                        (thread / EMU_THREADS_PER_NEIGH) % EMU_NEIGH_PER_SHIRE, \
                        (thread / EMU_THREADS_PER_MINION) % EMU_MINIONS_PER_NEIGH, \
                        thread % EMU_THREADS_PER_MINION, \
                        ##__VA_ARGS__); \
        emu_log() << LOG_##severity << emu_log_buffer << endm; \
    } \
} while (0)


#define LOG_NOTHREAD(severity, format, ...) do { \
    extern thread_local char emu_log_buffer[]; \
    if (LOG_##severity >= emu_log().getLogLevel()) \
    { \
        (void) snprintf(emu_log_buffer, 4096, format, ##__VA_ARGS__); \
        emu_log() << LOG_##severity << emu_log_buffer << endm; \
    } \
} while (0)

#define LOG(severity, format, ...) do { \
    extern uint32_t current_thread; \
    extern int32_t minion_only_log; \
    _LOG_IMPL(severity, \
              (minion_only_log < 0) || int32_t(current_thread / EMU_THREADS_PER_MINION) == minion_only_log, \
              current_thread, format, ##__VA_ARGS__); \
} while (0)

#define LOG_OTHER(severity, thread, format, ...) do { \
    extern int32_t minion_only_log; \
    _LOG_IMPL(severity, \
              (minion_only_log < 0) || int32_t(thread / EMU_THREADS_PER_MINION) == minion_only_log, \
              thread, format, ##__VA_ARGS__); \
} while (0)


#define LOG_ALL_MINIONS(severity, format, ...) do { \
    extern uint32_t current_thread; \
    _LOG_IMPL(severity, true, current_thread, format, ##__VA_ARGS__); \
} while(0)


#if VL == 4
#define MASK2BYTE(_MR) (_MR.b[3]<<3|_MR.b[2]<<2|_MR.b[1]<<1|_MR.b[0])
#else
#define MASK2BYTE(_MR) (_MR.b[7]<<7|_MR.b[6]<<6|_MR.b[5]<<5|_MR.b[4]<<4|_MR.b[3]<<3|_MR.b[2]<<2|_MR.b[1]<<1|_MR.b[0])
#endif

#define DEBUG_MASK(_MR) LOG(DEBUG, "\tmask = 0x%02x",MASK2BYTE(_MR));


extern testLog& emu_log();
extern void log_only_minion(int32_t);

#endif // _EMU_GIO_H
