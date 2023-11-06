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
        asm volatile("mv %[ptr], sp  \n"
                    : [ptr] "=r"(ptr));

        et_printf("Stack size received: 0x%lx Stack ptr:0x%lx\r\n", kernel_params->stack_size, (uint64_t)(uintptr_t)ptr);

        /* Allocate on stack and do a dummy copy
        64-bytes of stack are consumed for this kernel, hence allocate rest */
        char arr[kernel_params->stack_size - 64];
        et_memset(arr, 'A', kernel_params->stack_size - 64);
    }
    else
    {
        et_printf("Stack size null\r\n");
    }

    return 0;
}
