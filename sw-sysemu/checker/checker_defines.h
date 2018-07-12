#ifndef _CHECKER_DEFINES_
#define _CHECKER_DEFINES_

#include "log.h"

// Checker results
typedef enum
{
    CHECKER_OK    = 0,
    CHECKER_ERROR = 1,
    CHECKER_WAIT  = 2
} checker_result;

// Defines the function pointer typedef
typedef void (*func_ptr)();
typedef void (*func_ptr_0)(const char* comm);
typedef void (*func_ptr_1)(int arg0, const char* comm);
typedef void (*func_ptr_2)(int arg0, int arg1, const char* comm);
typedef void (*func_ptr_3)(int arg0, int arg1, int arg2, const char* comm);
typedef void (*func_ptr_4)(int arg0, int arg1, int arg2, int arg3, const char* comm);
typedef void (*func_ptr_5)(int arg0, int arg1, int arg2, int arg3, int arg4, const char* comm);

#endif // _CHECKER_DEFINES_
