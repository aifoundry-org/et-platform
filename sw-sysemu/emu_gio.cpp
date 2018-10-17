// g[s]printf functions for emu shared library used by checker in RTL tests

#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <cassert>
#include <cinttypes>
#include <algorithm>

#include "emu_gio.h"

extern uint32_t current_thread;

int print_debug = 0;

namespace emu {

    thread_local char* gio_buf     = nullptr;
    thread_local int   gio_maxsize = 0;
    thread_local char  gio_last    = '\n';

    void gprintf(const char* format, ...)
    {
        va_list argptr;
        if (!gio_buf) {
            // lazily allocate gio_buf
            gio_maxsize = 1024;
            gio_buf = new char[gio_maxsize];
        }
retry_vsnprintf:
        va_start(argptr, format);
        int gio_size = vsnprintf(gio_buf, gio_maxsize, format, argptr);
        if (gio_size >= gio_maxsize) {
            // not enough buffer space; grow gio_buf
            gio_maxsize *= 2;
            delete[] gio_buf;
            gio_buf = new char[gio_maxsize]; // may throw std::bad_alloc
            goto retry_vsnprintf;
        }
        va_end(argptr);
        // add a [EMU N.N] prefix to each line in the gio_buf
        int gio_pos = 0;
        if (gio_last == '\n') {
            printf("[EMU %" PRIu32 ".%" PRIu32 "] ", current_thread>>1, current_thread&1);
        }
        for (int i = 0; i < gio_size-1; ++i) {
            if (gio_buf[i] == '\n') {
                gio_buf[i] = '\0';
                printf("[EMU %" PRIu32 ".%" PRIu32 "] %s\n", current_thread>>1, current_thread&1, &gio_buf[gio_pos]);
                gio_pos = i+1;
            }
        }
        // print remaining line
        assert(gio_buf[gio_size] == '\0');
        gio_last = gio_buf[gio_size-1];
        printf(&gio_buf[gio_pos]);
    }

    void gsprintf(char* str, const char* format, ...)
    {
        va_list argptr;
        va_start(argptr, format);
        vsprintf(str, format, argptr);
        va_end(argptr);
    }

    void gfprintf(FILE *stream, const char* format, ...)
    {
        va_list argptr;
        va_start(argptr, format);
        vfprintf(stream, format, argptr);
        va_end(argptr);
    }

} // namespace emu


// Logging
testLog& emu_log() {
  static testLog l("EMU", LOG_INFO);
  return l;
}

void log_printf(enum logLevel level, const char* format,...)
{

  if ( level >= LOG_ERR || level>= emu_log().getLogLevel() ) {
    va_list args;
    va_start(args, format);
    
    va_list tmp_args; //unfortunately you cannot consume a va_list twice
    va_copy(tmp_args, args); //so we have to copy it
    const int required_len = vsnprintf(nullptr, 0, format, tmp_args) + 1;
    va_end(tmp_args);
    
    std::string buf(required_len, '\0');
    if (std::vsnprintf(&buf[0], buf.size(), format, args) < 0) {
      throw std::runtime_error{"string_vsprintf encoding error"};
    }

    // add [EMU M.T] stamp before each new line
    std::string emu_stamp = "[EMU " + std::to_string(current_thread>>1) + "." + std::to_string(current_thread&1) + "] ";
    emu_log().setName( emu_stamp);
    emu_stamp = "\n" + emu_stamp;
    size_t newline = 0;
    size_t stamp_len = emu_stamp.length();
    while( (newline = buf.find("\n", newline)) != std::string::npos){
      buf.replace(newline, 1, emu_stamp);
      newline+=stamp_len;
    }
    emu_log() << level << buf.c_str() << endm;
  }
}
