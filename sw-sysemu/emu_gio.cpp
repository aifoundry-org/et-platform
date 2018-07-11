// g[s]printf functions for emu shared library used by checker in RTL tests

#include <cstdio>
#include <stdarg.h>
#include <inttypes.h>
#include <string>
#include <sstream>

#include "emu_gio.h"

#define GPRINTF_BUF_SIZE 1024

using namespace std;

extern uint32_t current_thread;

int print_debug = 0;

namespace emu {

    static void puts_stamp( char * msg)
    {
        ostringstream s_stamp;
        s_stamp<<"[EMU " << (current_thread>>1) << "." << (current_thread & 1) << "] ";
        string stamp = s_stamp.str();
        string::size_type i;
        string s(stamp + string(msg));

        for (;;){
            i = s.find("\n", i);
            if (i == string::npos) break;
            if (s[i+1] == 0) break; // do not replace last \n
            s.replace(i, 1, stamp);
        }
        printf("%s", s.c_str());
    }

    void gprintf(const char* format, ...)
    {
        va_list argptr;
        va_start(argptr, format);
        char buf[GPRINTF_BUF_SIZE];
        int size = vsnprintf(buf, GPRINTF_BUF_SIZE, format, argptr);
        if ( GPRINTF_BUF_SIZE <= size) {
            char* new_buf = new char [ size + 1 ];
            vsprintf(new_buf, format, argptr);
            puts_stamp(new_buf);
            delete [] new_buf;
        } else {
            puts_stamp(buf);
        }
        va_end(argptr);
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
