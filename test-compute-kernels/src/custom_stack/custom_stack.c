#include <stdint.h>
#include <stddef.h>
#include <etsoc/common/utils.h>

typedef struct {
    uint64_t stack_size; /* in bytes */
} custom_stack_params_t;

int64_t entry_point(const custom_stack_params_t*);

int64_t entry_point(const custom_stack_params_t *const kernel_params)
{
    if (kernel_params->stack_size > 0)
    {
        uint64_t *ptr;
        uint64_t stack_used;
        asm volatile("mv %[ptr], sp  \n"
                    : [ptr] "=r"(ptr));

        et_printf("Stack size received: 0x%lx Stack ptr:0x%lx\r\n", kernel_params->stack_size, (uint64_t)(uintptr_t)ptr);

        /* Allocate on stack and do a dummy copy */
#ifdef __clang__
        stack_used = 1408;
#else
        stack_used = 64;
#endif
        /* Allocate stack_size - stack already used */
        __attribute__((aligned(64))) char arr[kernel_params->stack_size - stack_used];
        et_memset(arr, 'A', kernel_params->stack_size - stack_used);
    }
    else
    {
        et_printf("Stack size null\r\n");
    }

    return 0;
}
