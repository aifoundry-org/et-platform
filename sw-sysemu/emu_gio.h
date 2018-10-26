#ifndef _EMU_GIO_H
#define _EMU_GIO_H

#include "emu_defines.h"
#include <cstdarg>
#include "testLog.h"

#ifdef DEBUG_EMU
#undef DEBUG_EMU
#define DEBUG_EMU(a) do { if (print_debug) { a } } while (0)
#else
#define DEBUG_EMU(a) do { } while (0)
#endif

#ifdef DEBUG_MASK
#undef DEBUG_MASK
#define DEBUG_MASK(_MR) LOG(DEBUG, "\tmask = 0x%02x\n",MASK2BYTE(_MR));
#else
#define DEBUG_MASK(a)
#endif

extern int print_debug;

#define LOG(severity, format, ...)    log_printf(LOG_##severity, format, ##__VA_ARGS__)
#define LOG_ALL_MINIONS( severity, format, ...) do {	\
    int32_t old_minion_only_log = minion_only_log;	\
    minion_only_log = -1;				\
    LOG(severity, format, ##__VA_ARGS__);		\
    minion_only_log = old_minion_only_log;		\
  } while(0)						\

void log_printf(enum logLevel level, const char* format, ...);
testLog& emu_log();

namespace emu {
    extern void gprintf(const char* format, ...);
    extern void gsprintf(char* str, const char* format, ...);
    extern void gfprintf(FILE *stream, const char* format, ...);
}

#endif // _EMU_GIO_H
