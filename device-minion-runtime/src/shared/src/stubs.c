// stubs for minimal bare metal newlib-nano printf() implementation
// __attribute__((used)) because LTO, hmm

#include "serial.h"

#include <sys/stat.h>
#include <errno.h>

#undef errno
extern int errno;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wunused-parameter"

void* __attribute__((used)) _sbrk (int incr)
{
	extern char __heap_start; //set by linker
	extern char __heap_end; //set by linker

	static char *heap_end; // Previous end of heap or 0 if none
	char        *prev_heap_end;

	if (0 == heap_end)
    {
		heap_end = &__heap_start; // Initialize first time round
	}

	prev_heap_end  = heap_end;
	heap_end      += incr;

	if (heap_end >= (&__heap_end))
    {
		errno = ENOMEM;
		return (char*)-1;
	}
    else
    {
        return (void*)prev_heap_end;
    }
}

int __attribute__((used)) _write(int file, char *ptr, int len)
{
    SERIAL_write(UART0, ptr, len);
    return len;
}

int __attribute__((used)) _isatty(int file)
{
    return 1;
}

int __attribute__((used)) _close(int file)
{
    errno = EBADF;
    return -1;
}

int __attribute__((used)) _lseek(int file, int offset, int whence)
{
    return 0;
}

int __attribute__((used)) _read(int file, char *ptr, int len)
{
    return 0;
}

int __attribute__((used)) _fstat(int file, struct stat *st)
{
    st->st_mode = S_IFCHR;
    return 0;
}

#pragma GCC diagnostic pop
