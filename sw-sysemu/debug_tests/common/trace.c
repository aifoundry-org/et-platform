#include "diag.h"

#define assert(x)

#define DIAG_PUTCHAR (1ull << 56)

static void print_hex(unsigned long long n)
{
    if (n == 0) {
        putchar('0');
        return;
    }
    int i = 0;
    int buf[64];
    while (n > 0) {
        buf[i++] = n % 16;
        n /= 16;
    }
    while (i > 0) {
        --i;
        char o = buf[i] < 10 ? '0' : ('a' - 10);
        putchar(o + buf[i]);
    }
}


static void print_uint(unsigned long long n)
{
    if (n == 0) {
        putchar('0');
        return;
    }
    int i = 0;
    int buf[64];
    while (n > 0) {
        buf[i++] = n % 10;
        n /= 10;
    }
    while (i > 0)
        putchar('0' + buf[--i]);
}


static void print_int(long long n)
{
    if (n < 0) {
        putchar('-');
        n = -n;
    }
    print_uint(n);
}


static void print_str(const char* s)
{
    while (*s) {
        putchar(*s++);
    }
}


int putchar(int c)
{
    uint64_t csr_enc = DIAG_PUTCHAR | c;
    asm volatile("csrw validation1, %[csr_enc]\n" : : [ csr_enc ] "r"(csr_enc) :);
    return 0;
}


int puts(const char* s)
{
    print_str(s);
    putchar('\n');
}


int vprintf(const char* fmt, va_list ap)
{
    while (*fmt) {
        char c = *fmt++;
        if (c != '%') {
            putchar(c);
        }
        else {
            int l = 0;
            c     = *fmt++;
            while (c == 'l') {
                ++l;
                c = *fmt++;
            }
            switch (c) {
            case 'd':
            case 'i':
                switch (l) {
                case 0: print_int(va_arg(ap, int)); break;
                case 1: print_int(va_arg(ap, long int)); break;
                case 2: print_int(va_arg(ap, long long int)); break;
                default: assert(0 && "Invalid number of l's");
                }
                break;
            case 'u':
                switch (l) {
                case 0: print_uint(va_arg(ap, unsigned)); break;
                case 1: print_uint(va_arg(ap, long unsigned)); break;
                case 2: print_uint(va_arg(ap, long long unsigned)); break;
                default: assert(0 && "Invalid number of l's");
                }
                break;
            case 'x':
            case 'X':
                switch (l) {
                case 0: print_hex(va_arg(ap, unsigned)); break;
                case 1: print_hex(va_arg(ap, long unsigned)); break;
                case 2: print_hex(va_arg(ap, long long unsigned)); break;
                default: assert(0 && "Invalid number of l's");
                }
                break;
            case 's': print_str(va_arg(ap, const char*)); break;
            case 'p':
                putchar('0');
                putchar('x');
                print_hex(va_arg(ap, long long unsigned));
                break;
            default: putchar(c);
            }
        }
    }

    return 0;
}


int printf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int rv = vprintf(fmt, ap);
    va_end(ap);
    return rv;
}


void tracef(const char* fmt, ...)
{
    uint64_t hart = get_hart_id();
    printf("TRACE: [H%u] ", hart);

    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    putchar('\n');
}

void failf(const char* fmt, ...)
{
    uint64_t hart = get_hart_id();
    printf("FAIL: [H%u] ", hart);

    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    putchar('\n');

    C_TEST_FAIL;
}
