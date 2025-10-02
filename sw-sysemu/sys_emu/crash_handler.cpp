/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include <climits>
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#define UNW_LOCAL_ONLY
#include <libunwind.h>

#include "crash_handler.h"


// Minimal printing functionality that does not use any dynamic memory and
// does not throw any exceptions, so it can be used inside a signal handler
template<size_t N>
struct Mem_stream {
    char     data[N];
    size_t   pos;

    void reset() {
        pos = 0;
    }

    void write_char(char c)
    {
        data[pos] = c;
        pos = (pos < N - 1) ? (pos + 1) : 0;
    }

    void write_hex(uint64_t value, const char fillchar = '\0', bool ignore_zero = true)
    {
        if (!ignore_zero || (ignore_zero && fillchar == '0')) {
            write_char('0');
            write_char('x');
        }
        for (size_t shamt = sizeof(value) * CHAR_BIT - 4; shamt; shamt -= 4) {
            uint64_t nibble = (value >> shamt) & 0xF;
            if (ignore_zero) {
                if (!nibble) {
                    if (fillchar != '\0')
                        write_char(fillchar);
                    continue;
                }
                if (fillchar != '0') {
                    write_char('0');
                    write_char('x');
                }
            }
            write_char("0123456789abcdef"[nibble]);
            ignore_zero = false;
        }
        if (value <= 0xF && ignore_zero && fillchar != '0') {
            write_char('0');
            write_char('x');
        }
        write_char("0123456789abcdef"[value & 0xF]);
    }

    void write_cstr(const char* str)
    {
        while (*str) {
            write_char(*str++);
        }
    }

    void flush(int fd) const
    {
        (void) write(fd, data, pos);
    }
};


static void abort_with_backtrace(int sig)
{
    static Mem_stream<512> mbuf;
    static char sym[256];
    unw_cursor_t cursor;
    unw_context_t context;

    mbuf.reset();
    mbuf.write_cstr("*** ");
    switch (sig) {
    case SIGABRT: mbuf.write_cstr("SIGABRT"); break;
    case SIGFPE : mbuf.write_cstr("SIGFPE");  break;
    case SIGILL : mbuf.write_cstr("SIGILL");  break;
    case SIGSEGV: mbuf.write_cstr("SIGSEGV"); break;
    default     : mbuf.write_cstr("SIG???");  break;
    }
    mbuf.write_cstr(" received; stack trace: ***\n");
    mbuf.flush(STDERR_FILENO);

    // Initialize cursor to current frame for local unwinding.
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);

    // Unwind frames one by one, going up the frame stack.
    while (unw_step(&cursor) > 0) {
        unw_word_t offset, pc;
        unw_get_reg(&cursor, UNW_REG_IP, &pc);
        if (pc == 0)
            break;

        mbuf.reset();
        mbuf.write_char('@');
        mbuf.write_hex(pc, ' ');

        if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
            mbuf.write_char(' ');
            mbuf.write_cstr(sym);
            //mbuf.write_char('+');
            //mbuf.write_hex(offset);
        }
        mbuf.write_char('\n');
        mbuf.flush(STDERR_FILENO);
    }

    (void) signal(SIGABRT, SIG_DFL);
    abort();
}


Crash_handler::Crash_handler()
{
    (void) signal(SIGABRT, abort_with_backtrace);
    (void) signal(SIGFPE,  abort_with_backtrace);
    (void) signal(SIGILL,  abort_with_backtrace);
    (void) signal(SIGSEGV, abort_with_backtrace);
}


Crash_handler::~Crash_handler()
{
    (void) signal(SIGABRT, SIG_DFL);
    (void) signal(SIGFPE,  SIG_DFL);
    (void) signal(SIGILL,  SIG_DFL);
    (void) signal(SIGSEGV, SIG_DFL);
}
