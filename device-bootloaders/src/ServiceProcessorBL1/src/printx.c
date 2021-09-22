/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#include "etsoc/drivers/serial/serial.h"

#include "printx.h"
#include "stdbool.h"

#define COMPILER_PROMOTES_VAR_ARGS_TO_32_BIT

//#define SUPPORT_SSIZE
//#define SUPPORT_ALTERNATE_FORM
//#define SUPPORT_LEFT_ADJUSTED

#ifdef SUPPORT_SSIZE
#include </usr/include/unistd.h>
#endif

#define UNUSED_ARGUMENT(x) (void)x

typedef int (*PRINTF_CHAR_FN)(void *context, char c);

// Although the C99 spec clearly states that it is OK to pass the VA by pointer.
// ISO/IEC 9899:1999 - paragraph 7.15 'Variable arguments <stdarg.h>, footnote 212
// states:
// "It is permitted to create a pointer to a va_list and pass that pointer to
// another function, in which case the original function may make further use
// of the original list after the other function returns."
//
// However, this fails on the GCC x86 compilers with the following message:
// passing argument ... from incompatible pointer type [-Werror=incompatible-pointer-types]
// expected ‘__va_list_tag (*)[1]’ but argument is of type ‘__va_list_tag **
//
//
// As a work around for this problem, we will wrap the va_list argument in a structure,
// and pass the structure by pointer.

typedef struct VA_LIST_STRUCT {
    va_list ap;
} VA_LIST_STRUCT_t;

typedef enum PRINTX_MODIFIER {
    PRINTX_MODIFIER_DEFAULT = 0,
    PRINTX_MODIFIER_HH,
    PRINTX_MODIFIER_H,
    PRINTX_MODIFIER_L,
    PRINTX_MODIFIER_LL,
    PRINTX_MODIFIER_Z
} PRINTX_MODIFIER_t;

typedef struct PRINTX_SPECIFIER {
#ifdef SUPPORT_ALTERNATE_FORM
    bool alternate_form;
#endif
    bool zero_padded;
#ifdef SUPPORT_LEFT_ADJUSTED
//    bool left_adjusted;
#endif
    bool space_before_positive;
    bool print_sign;

    bool width_specified;
    int32_t width;

    PRINTX_MODIFIER_t modifier;

    union {
        uint8_t u8;
        int8_t s8;
        uint16_t u16;
        int16_t s16;
        uint32_t u32;
        int32_t s32;
        uint64_t u64;
        int64_t s64;
        size_t size;
#ifdef SUPPORT_SSIZE
        ssize_t ssize;
#endif
        const void *ptr;
        const char *str;
    } val;
} PRINTX_SPECIFIER_t;

static int print_impl_char(PRINTF_CHAR_FN print_char_pfn, void *context,
                           const PRINTX_SPECIFIER_t *pprintx_specifier)
{
    int count;
    int padding;

    count = 1;
    if (pprintx_specifier->width_specified && pprintx_specifier->width > 1) {
        padding = pprintx_specifier->width - 1;
    } else {
        padding = 0;
    }
    count += padding;

#ifdef SUPPORT_LEFT_ADJUSTED
    if (false == pprintx_specifier->left_adjusted) {
#endif
        while (padding > 0) {
            if (0 != print_char_pfn(context, ' ')) {
                return -1;
            }
            padding--;
        }
#ifdef SUPPORT_LEFT_ADJUSTED
    }
#endif
    if (0 != print_char_pfn(context, (char)(pprintx_specifier->val.s8))) {
        return -1;
    }
#ifdef SUPPORT_LEFT_ADJUSTED
    if (true == pprintx_specifier->left_adjusted) {
        while (padding > 0) {
            if (0 != print_char_pfn(context, ' ')) {
                return -1;
            }
            padding--;
        }
    }
#endif
    return count;
}
static int print_impl_string_len(const char *str)
{
    const char *str_end = str;
    while (*str_end) {
        str_end++;
    }
    return (int)(str_end - str);
}

static int print_impl_string(PRINTF_CHAR_FN print_char_pfn, void *context,
                             const PRINTX_SPECIFIER_t *pprintx_specifier)
{
    int count = 0;
    const char *str = pprintx_specifier->val.str;
    int str_len;
    int padding = 0;

    if (pprintx_specifier->width_specified) {
        str_len = print_impl_string_len(str);
        if (pprintx_specifier->width > str_len) {
            padding = pprintx_specifier->width - str_len;
        }
    }

#ifdef SUPPORT_LEFT_ADJUSTED
    if (false == pprintx_specifier->left_adjusted) {
#endif
        while (padding > 0) {
            if (0 != print_char_pfn(context, ' ')) {
                return -1;
            }
            count++;
            padding--;
        }
#ifdef SUPPORT_LEFT_ADJUSTED
    }
#endif
    while (0 != *str) {
        if (0 != print_char_pfn(context, *str)) {
            return -1;
        }
        count++;
        str++;
    }
#ifdef SUPPORT_LEFT_ADJUSTED
    if (true == pprintx_specifier->left_adjusted) {
        while (padding > 0) {
            if (0 != print_char_pfn(context, ' ')) {
                return -1;
            }
            count++;
            padding--;
        }
    }
#endif
    return count;
}
// maximum value: 18446744073709551615
static int print_impl_unsigned_integer(PRINTF_CHAR_FN print_char_pfn, void *context,
                                       const PRINTX_SPECIFIER_t *pprintx_specifier)
{
    char buffer[20];
    int count = 0;
    int offset = 0;
    int padding;
    uint32_t digit;
    uint64_t val = pprintx_specifier->val.u64;
    do {
        digit = (uint32_t)(val % 10);
        buffer[offset] = (char)('0' + digit);
        offset++;
        val = val / 10u;
    } while (0 != val);

    if (pprintx_specifier->zero_padded && pprintx_specifier->width_specified &&
        pprintx_specifier->width > offset) {
        padding = pprintx_specifier->width - offset;
        while (padding > 0) {
            buffer[offset] = '0';
            offset++;
            padding--;
        }
    }

    if (!pprintx_specifier->zero_padded && pprintx_specifier->width_specified &&
        pprintx_specifier->width > offset) {
        padding = pprintx_specifier->width - offset;
    } else {
        padding = 0;
    }

    count = offset + padding;

#ifdef SUPPORT_LEFT_ADJUSTED
    if (false == pprintx_specifier->left_adjusted) {
#endif
        while (padding) {
            if (0 != print_char_pfn(context, ' ')) {
                return -1;
            }
            padding--;
        }
#ifdef SUPPORT_LEFT_ADJUSTED
    }
#endif
    while (offset > 0) {
        offset--;
        if (0 != print_char_pfn(context, buffer[offset])) {
            return -1;
        }
    }

#ifdef SUPPORT_LEFT_ADJUSTED
    if (true == pprintx_specifier->left_adjusted) {
        while (padding) {
            if (0 != print_char_pfn(context, ' ')) {
                return -1;
            }
            padding--;
        }
    }
#endif
    return count;
}

// maximum value: FFFFFFFFFFFFFFFF
static int print_impl_hex(PRINTF_CHAR_FN print_char_pfn, void *context,
                          const PRINTX_SPECIFIER_t *pprintx_specifier, bool upper_case)
{
    char buffer[18];
    int count = 0;
    int offset = 0;
    int padding;
    uint32_t digit;
    uint64_t val = pprintx_specifier->val.u64;
    do {
        digit = (uint32_t)(val % 16);
        if (digit < 10) {
            buffer[offset] = (char)('0' + digit);
        } else {
            if (upper_case) {
                buffer[offset] = (char)('A' + digit - 10);
            } else {
                buffer[offset] = (char)('a' + digit - 10);
            }
        }
        offset++;
        val = val / 16;
    } while (0 != val);

    if (pprintx_specifier->zero_padded && pprintx_specifier->width_specified &&
        pprintx_specifier->width > offset) {
        padding = pprintx_specifier->width - offset;
#ifdef SUPPORT_ALTERNATE_FORM
        if (pprintx_specifier->alternate_form) {
            if (padding > 2) {
                padding -= 2;
            } else {
                padding = 0;
            }
        }
#endif
        while (padding > 0) {
            buffer[offset] = '0';
            offset++;
            padding--;
        }
    }

#ifdef SUPPORT_ALTERNATE_FORM
    if (pprintx_specifier->alternate_form) {
        buffer[offset] = upper_case ? 'X' : 'x';
        offset++;
        buffer[offset] = '0';
        offset++;
    }
#endif
    if (!pprintx_specifier->zero_padded && pprintx_specifier->width_specified &&
        pprintx_specifier->width > offset) {
        padding = pprintx_specifier->width - offset;
    } else {
        padding = 0;
    }

    count = offset + padding;

#ifdef SUPPORT_LEFT_ADJUSTED
    if (false == pprintx_specifier->left_adjusted) {
#endif
        while (padding) {
            if (0 != print_char_pfn(context, ' ')) {
                return -1;
            }
            padding--;
        }
#ifdef SUPPORT_LEFT_ADJUSTED
    }
#endif
    while (offset > 0) {
        offset--;
        if (0 != print_char_pfn(context, buffer[offset])) {
            return -1;
        }
    }

#ifdef SUPPORT_LEFT_ADJUSTED
    if (true == pprintx_specifier->left_adjusted) {
        while (padding) {
            if (0 != print_char_pfn(context, ' ')) {
                return -1;
            }
            padding--;
        }
    }
#endif
    return count;
}

// minimum value: -9223372036854775808
// maximum value: 9223372036854775807
static int print_impl_signed_integer(PRINTF_CHAR_FN print_char_pfn, void *context,
                                     const PRINTX_SPECIFIER_t *pprintx_specifier)
{
    char buffer[20];
    int count = 0;
    int offset = 0;
    bool negative;
    int padding;
    uint32_t digit;
    int64_t val = pprintx_specifier->val.s64;
    if (val < 0) {
        negative = true;
        val = 0 - val;
    } else {
        negative = false;
    }
    do {
        digit = (uint32_t)(val % 10);
        buffer[offset] = (char)('0' + digit);
        offset++;
        val = val / 10u;
    } while (0 != val);

    if (pprintx_specifier->zero_padded && pprintx_specifier->width_specified &&
        pprintx_specifier->width > offset) {
        padding = pprintx_specifier->width - offset;
        if (negative || pprintx_specifier->print_sign || pprintx_specifier->space_before_positive) {
            padding--;
        }
        while (padding > 0) {
            buffer[offset] = '0';
            offset++;
            padding--;
        }
    }
    if (pprintx_specifier->print_sign) {
        buffer[offset] = negative ? '-' : '+';
        offset++;
    } else if (negative) {
        buffer[offset] = '-';
        offset++;
    } else if (pprintx_specifier->space_before_positive) {
        buffer[offset] = ' ';
        offset++;
    }

    if (!pprintx_specifier->zero_padded && pprintx_specifier->width_specified &&
        pprintx_specifier->width > offset) {
        padding = pprintx_specifier->width - offset;
    } else {
        padding = 0;
    }

    count = offset + padding;

#ifdef SUPPORT_LEFT_ADJUSTED
    if (false == pprintx_specifier->left_adjusted) {
#endif
        while (padding) {
            if (0 != print_char_pfn(context, ' ')) {
                return -1;
            }
            padding--;
        }
#ifdef SUPPORT_LEFT_ADJUSTED
    }
#endif
    while (offset > 0) {
        offset--;
        if (0 != print_char_pfn(context, buffer[offset])) {
            return -1;
        }
    }
#ifdef SUPPORT_LEFT_ADJUSTED
    if (true == pprintx_specifier->left_adjusted) {
        while (padding) {
            if (0 != print_char_pfn(context, ' ')) {
                return -1;
            }
            padding--;
        }
    }
#endif
    return count;
}

static void print_conversion_get_arg_val(VA_LIST_STRUCT_t *args,
                                         PRINTX_SPECIFIER_t *pprintx_specifier, bool sign_extend)
{
    if (sign_extend) {
        switch (pprintx_specifier->modifier) {
        case PRINTX_MODIFIER_H:
#ifdef COMPILER_PROMOTES_VAR_ARGS_TO_32_BIT
            pprintx_specifier->val.s64 = va_arg(args->ap, int);
#else
            pprintx_specifier->val.s64 = va_arg(args->ap, char);
#endif
            return;
        case PRINTX_MODIFIER_HH:
#ifdef COMPILER_PROMOTES_VAR_ARGS_TO_32_BIT
            pprintx_specifier->val.s64 = va_arg(args->ap, int);
#else
            pprintx_specifier->val.s64 = va_arg(args->ap, short);
#endif
            return;
        case PRINTX_MODIFIER_DEFAULT:
        default:
            pprintx_specifier->val.s64 = va_arg(args->ap, int);
            return;
        case PRINTX_MODIFIER_L:
            pprintx_specifier->val.s64 = va_arg(args->ap, long int);
            return;
        case PRINTX_MODIFIER_LL:
            pprintx_specifier->val.s64 = va_arg(args->ap, long long int);
            return;
        case PRINTX_MODIFIER_Z:
#ifdef SUPPORT_SSIZE
            pprintx_specifier->val.s64 = va_arg(args->ap, ssize_t);
#else
            pprintx_specifier->val.u64 = va_arg(args->ap, size_t);
#endif
            return;
        }
    } else {
        switch (pprintx_specifier->modifier) {
        case PRINTX_MODIFIER_H:
#ifdef COMPILER_PROMOTES_VAR_ARGS_TO_32_BIT
            pprintx_specifier->val.u64 = va_arg(args->ap, unsigned int);
#else
            pprintx_specifier->val.u64 = va_arg(args->ap, unsigned char);
#endif
            return;
        case PRINTX_MODIFIER_HH:
#ifdef COMPILER_PROMOTES_VAR_ARGS_TO_32_BIT
            pprintx_specifier->val.u64 = va_arg(args->ap, unsigned int);
#else
            pprintx_specifier->val.u64 = va_arg(args->ap, unsigned short);
#endif
            return;
        case PRINTX_MODIFIER_DEFAULT:
        default:
            pprintx_specifier->val.u64 = va_arg(args->ap, unsigned int);
            return;
        case PRINTX_MODIFIER_L:
            pprintx_specifier->val.u64 = va_arg(args->ap, unsigned long int);
            return;
        case PRINTX_MODIFIER_LL:
            pprintx_specifier->val.u64 = va_arg(args->ap, unsigned long long int);
            return;
        case PRINTX_MODIFIER_Z:
            pprintx_specifier->val.u64 = va_arg(args->ap, size_t);
            return;
        }
    }
}

static int print_conversion_specifier(PRINTF_CHAR_FN print_char_pfn, void *context,
                                      const char **pformat, VA_LIST_STRUCT_t *args)
{
    PRINTX_SPECIFIER_t printx_specifier = { 0 };
    bool allow_flags;
    bool allow_width;

    // assert(print_char_pfn);
    // assert(pformat);
    // assert(pap);

    allow_flags = true;
    allow_width = true;
    for (;;) {
        switch (**pformat) {
        case '%':
            if (0 != print_char_pfn(context, '%')) {
                return -1;
            }
            return 1;
#ifdef SUPPORT_ALTERNATE_FORM
        case '#':
            if (false == allow_flags) {
                return -1;
            }
            printx_specifier.alternate_form = true;
            break;
#endif
#ifdef SUPPORT_LEFT_ADJUSTED
        case '-':
            if (false == allow_flags) {
                return -1;
            }
            printx_specifier.left_adjusted = true;
            break;
#endif
        case ' ':
            if (false == allow_flags) {
                return -1;
            }
            printx_specifier.space_before_positive = true;
            break;
        case '+':
            if (false == allow_flags) {
                return -1;
            }
            printx_specifier.print_sign = true;
            break;
        case '0':
            if (true == allow_flags && false == printx_specifier.zero_padded) {
                printx_specifier.zero_padded = true;
                break;
            } else if (false == allow_width) {
                return -1;
            }
            allow_flags = false;
            printx_specifier.width_specified = true;
            printx_specifier.width = 10 * printx_specifier.width;
            if (printx_specifier.width > 999) {
                return -1;
            }
            break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if (false == allow_width) {
                return -1;
            }
            allow_flags = false;
            printx_specifier.width_specified = true;
            printx_specifier.width = 10 * printx_specifier.width + (int32_t)(**pformat - '0');
            if (printx_specifier.width > 999) {
                return -1;
            }
            break;
        case '*':
            if (false == allow_width) {
                return -1;
            }
            allow_flags = false;
            printx_specifier.width_specified = true;
            printx_specifier.width = va_arg(args->ap, int32_t);
            if (printx_specifier.width < -999 || 999 < printx_specifier.width) {
                return -1;
            }
            allow_width = false;
            break;
        case 'h':
            allow_flags = false;
            allow_width = false;
            if (PRINTX_MODIFIER_DEFAULT == printx_specifier.modifier) {
                printx_specifier.modifier = PRINTX_MODIFIER_H;
            } else if (PRINTX_MODIFIER_H == printx_specifier.modifier) {
                printx_specifier.modifier = PRINTX_MODIFIER_HH;
            } else {
                return -1;
            }
            break;
        case 'l':
            allow_flags = false;
            allow_width = false;
            if (PRINTX_MODIFIER_DEFAULT == printx_specifier.modifier) {
                printx_specifier.modifier = PRINTX_MODIFIER_L;
            } else if (PRINTX_MODIFIER_L == printx_specifier.modifier) {
                printx_specifier.modifier = PRINTX_MODIFIER_LL;
            } else {
                return -1;
            }
            break;
        case 'z':
            allow_flags = false;
            allow_width = false;
            if (PRINTX_MODIFIER_DEFAULT == printx_specifier.modifier) {
                printx_specifier.modifier = PRINTX_MODIFIER_Z;
            } else {
                return -1;
            }
            break;
        case 'c':
            print_conversion_get_arg_val(args, &printx_specifier, true);
            return print_impl_char(print_char_pfn, context, &printx_specifier);
        case 's':
            printx_specifier.val.str = va_arg(args->ap, const char *);
            return print_impl_string(print_char_pfn, context, &printx_specifier);
        case 'i':
        case 'd':
            print_conversion_get_arg_val(args, &printx_specifier, true);
            return print_impl_signed_integer(print_char_pfn, context, &printx_specifier);
        case 'u':
            print_conversion_get_arg_val(args, &printx_specifier, false);
            return print_impl_unsigned_integer(print_char_pfn, context, &printx_specifier);
        case 'x':
            print_conversion_get_arg_val(args, &printx_specifier, false);
            return print_impl_hex(print_char_pfn, context, &printx_specifier, false);
        case 'X':
            print_conversion_get_arg_val(args, &printx_specifier, false);
            return print_impl_hex(print_char_pfn, context, &printx_specifier, true);
        case 'p':
            printx_specifier.val.ptr = va_arg(args->ap, const void *);
            return print_impl_hex(print_char_pfn, context, &printx_specifier, false);
        case 'P':
            printx_specifier.val.ptr = va_arg(args->ap, const void *);
            return print_impl_hex(print_char_pfn, context, &printx_specifier, true);
        default:
            return -1;
        }
        (*pformat)++;
    }
}

static int print_impl(PRINTF_CHAR_FN print_char_pfn, void *context, const char *format,
                      VA_LIST_STRUCT_t *args)
{
    char c, h0, h1;
    int r;
    int count = 0;

    while (0 != *format) {
        switch (*format) {
        case '\\': // process special characters
            format++;
            switch (*format) {
            case '\\':
                c = '\\';
                break;
            case '"':
                c = '"';
                break;
            case 'n':
                c = '\n';
                break;
            case 'x':
                format++;
                h1 = 0;
                if ('0' <= *format && *format <= '9') {
                    h0 = (char)(*format - '0');
                } else if ('A' <= *format && *format <= 'F') {
                    h0 = (char)(10 + *format - 'A');
                } else if ('a' <= *format && *format <= 'f') {
                    h0 = (char)(10 + *format - 'a');
                } else {
                    return -1;
                }
                format++;
                if ('0' <= *format && *format <= '9') {
                    h1 = h0;
                    h0 = (char)(*format - '0');
                    format++;
                } else if ('A' <= *format && *format <= 'F') {
                    h1 = h0;
                    h0 = (char)(10 + *format - 'A');
                    format++;
                } else if ('a' <= *format && *format <= 'f') {
                    h1 = h0;
                    h0 = (char)(10 + *format - 'a');
                    format++;
                }
                c = (char)((h1 << 4) | h0);
                break;
            default:
                return -1;
            }
            if (0 != print_char_pfn(context, c)) {
                return -1;
            }
            count++;
            break;
        case '%': // process conversion specifier
            format++;
            r = print_conversion_specifier(print_char_pfn, context, &format, args);
            if (r < 0) {
                return r;
            }
            count += r;
            break;
        default: // print the characters
            if (0 != print_char_pfn(context, *format)) {
                return -1;
            }
            count++;
            break;
        } // switch
        format++;
    } // while

    return count;
}

static int print_to_serial(void *context, char c)
{
    UNUSED_ARGUMENT(context);

    SERIAL_write(UART0, &c, 1);

    return 0;
}

typedef struct SPRINTX_CONTEXT {
    char *buffer;
    size_t buffer_size;
    size_t current_offset;
} SPRINTX_CONTEXT_t;

static int print_to_string(void *context, char c)
{
    SPRINTX_CONTEXT_t *sprintx_context = (SPRINTX_CONTEXT_t *)context;
    if (sprintx_context->current_offset < sprintx_context->buffer_size) {
        sprintx_context->buffer[sprintx_context->current_offset] = c;
        sprintx_context->current_offset++;
        return 0;
    }
    return -1;
}

static int vprintx(const char *format, VA_LIST_STRUCT_t *args)
{
    return print_impl(print_to_serial, NULL, format, args);
}

static int vsnprintx(char *str, size_t size, const char *format, VA_LIST_STRUCT_t *args)
{
    int count;
    SPRINTX_CONTEXT_t context;
    if (NULL == str) {
        return -1;
    }
    context.buffer = str;
    context.buffer_size = size;
    context.current_offset = 0;
    count = print_impl(print_to_string, &context, format, args);
    if (count < 0) {
        return count;
    }
    // is there enough room for a terminating NULL?
    if (((uint32_t)count) >= size) {
        return -1;
    }
    // add terminating NULL, but don't include it in the returned count
    str[count] = 0;
    return count;
}

int printx(const char *format, ...)
{
    VA_LIST_STRUCT_t args;
    int i;

    va_start(args.ap, format);
    i = vprintx(format, &args);
    va_end(args.ap);

    return i;
}

int snprintx(char *str, size_t size, const char *format, ...)
{
    VA_LIST_STRUCT_t args;
    int i;

    va_start(args.ap, format);
    i = vsnprintx(str, size, format, &args);
    va_end(args.ap);

    return i;
}
