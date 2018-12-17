#ifndef _EMU_GIO_H
#define _EMU_GIO_H

#include <cstdio>
#include "testLog.h"
#include "emu_defines.h"


#define LOG(severity, format, ...) do { \
    extern uint32_t current_thread; \
    extern int32_t minion_only_log; \
    extern thread_local char emu_log_buffer[]; \
    if ( (LOG_##severity >= LOG_ERR) || \
         ((LOG_##severity >= emu_log().getLogLevel()) && \
          ((minion_only_log < 0) || int32_t(current_thread / EMU_THREADS_PER_MINION) == minion_only_log)) ) \
    { \
        (void) snprintf(emu_log_buffer, 4096, "[EMU Curr%u S%u->N%u->M%u->T%u] " format, \
                        			(current_thread), \
                        			(current_thread /35), \
						(current_thread %32)/8, \
						(current_thread %32)%8, \
						(current_thread % EMU_THREADS_PER_MINION), \
                        ##__VA_ARGS__); \
        emu_log() << LOG_##severity << emu_log_buffer << endm; \
    } \
} while (0)


#define LOG_ALL_MINIONS(severity, format, ...) do { \
    extern int32_t minion_only_log; \
    int32_t old_minion_only_log = minion_only_log; \
    minion_only_log = -1; \
    LOG(severity, format, ##__VA_ARGS__); \
    minion_only_log = old_minion_only_log; \
} while(0)


#if VL == 4
#define MASK2BYTE(_MR) (_MR.b[3]<<3|_MR.b[2]<<2|_MR.b[1]<<1|_MR.b[0])
#else
#define MASK2BYTE(_MR) (_MR.b[7]<<7|_MR.b[6]<<6|_MR.b[5]<<5|_MR.b[4]<<4|_MR.b[3]<<3|_MR.b[2]<<2|_MR.b[1]<<1|_MR.b[0])
#endif

#define DEBUG_MASK(_MR) LOG(DEBUG, "\tmask = 0x%02x",MASK2BYTE(_MR));


extern testLog& emu_log();

#endif // _EMU_GIO_H
