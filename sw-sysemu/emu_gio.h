#ifndef _EMU_GIO_H
#define _EMU_GIO_H

#include "emu_defines.h"

#ifdef DEBUG_EMU
#undef DEBUG_EMU
#define DEBUG_EMU(a) do { if (print_debug) { a } } while (0)
#else
#define DEBUG_EMU(a) do { } while (0)
#endif

#ifdef DEBUG_MASK
#undef DEBUG_MASK
#define DEBUG_MASK(_MR) DEBUG_EMU(gprintf("\tmask = 0x%02x\n",MASK2BYTE(_MR)););
#else
#define DEBUG_MASK(a)
#endif

extern int print_debug;

namespace emu {
    extern void gprintf(const char* format, ...);
    extern void gsprintf(char* str, const char* format, ...);
    extern void gfprintf(FILE *stream, const char* format, ...);
}

#endif // _EMU_GIO_H
