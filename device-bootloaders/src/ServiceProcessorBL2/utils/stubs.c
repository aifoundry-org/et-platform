// stubs for minimal bare metal newlib-nano in FreeRTOS
// with configUSE_NEWLIB_REENTRANT set so each task gets its own
// _reent struc and _impure_ptr is updated on context switch
// See https://www.freertos.org/a00110.html#configUSE_NEWLIB_REENTRANT
// See https://www.sourceware.org/newlib/libc.html#Reentrancy
// See https://www.sourceware.org/newlib/libc.html#g_t_005f_005fmalloc_005flock
// See http://www.nadler.com/embedded/newlibAndFreeRTOS.html
// lots of __attribute__((used)) because LTO is being overly-aggressive, hmm

#include "etsoc/drivers/serial/serial.h"

#include "FreeRTOS.h"
#include "task.h"

#include <sys/stat.h>
#include <errno.h>

#if (!defined(configUSE_NEWLIB_REENTRANT) || (configUSE_NEWLIB_REENTRANT == 0))
#error "FreeRTOS must be configured with configUSE_NEWLIB_REENTRANT = 1"
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wunused-parameter"

void *__attribute__((used)) _sbrk(int incr)
{
    extern char __heap_start; //set by linker
    extern char __heap_end; //set by linker

    static char *heap_end; // Previous end of heap or 0 if none
    char *prev_heap_end;

    if (0 == heap_end) {
        heap_end = &__heap_start; // Initialize first time round
    }

    prev_heap_end = heap_end;
    heap_end += incr;

    if (heap_end >= (&__heap_end)) {
        errno = ENOMEM;
        return (char *)-1;
    } else {
        return (void *)prev_heap_end;
    }
}

void __malloc_lock(void)
{
    taskENTER_CRITICAL();
}

void __malloc_unlock(void)
{
    taskEXIT_CRITICAL();
}

void __env_lock(void)
{
    taskENTER_CRITICAL();
}

void __env_unlock(void)
{
    taskEXIT_CRITICAL();
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
