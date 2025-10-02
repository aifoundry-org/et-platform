#include <stdint.h>
#include <stddef.h>
#include <etsoc/common/utils.h>
#include <trace/trace_umode.h>
#include <system/abi.h>

typedef struct {
  uint64_t shire_mask;
} kernel_args_t;

int64_t entry_point(const kernel_args_t*, const kernel_environment_t*);

int64_t entry_point(const kernel_args_t *kernel_args, const kernel_environment_t *kernel_env)
{
    /* Print the args and env */
    et_printf("Kernel Args:shire_mask: 0x%lx\r\n", kernel_args->shire_mask);
    et_printf("Kernel Environment:ABI ver:%u.%u.%u:shire_mask:0x%lx:frequency:%u\n\r",
        kernel_env->version.major, kernel_env->version.minor, kernel_env->version.patch,
        kernel_env->shire_mask, kernel_env->frequency);

    /* Verify the kernel env parameters */
    if (kernel_args->shire_mask != kernel_env->shire_mask)
    {
        return -1;
    }
    else if ((kernel_env->version.major == 0) && (kernel_env->version.minor == 0) && (kernel_env->version.patch == 0))
    {
        return -1;
    }

    return 0;
}
