#include <etsoc/isa/hart.h>

#define bool uint64_t
#define false 0x0ULL
#define true 0x1ULL

#include "utils.h"
#include "kernel.h"

typedef struct {
    uint64_t loop_size;
} Parameters;

int main(const Parameters* const kernel_params_ptr) {
    uint64_t loop_size = kernel_params_ptr->loop_size;
    loop_size = 1;
    uint64_t ret = shire_cache_power_virus(loop_size);
    if (ret == 0) return 0;
    else return -1;
}
