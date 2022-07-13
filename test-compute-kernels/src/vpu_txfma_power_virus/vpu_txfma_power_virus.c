#include <etsoc/isa/hart.h>
#include "kernel.h"

typedef struct {
    uint64_t loop_size;
} Parameters;

int main(const Parameters* const kernel_params_ptr) {
    uint64_t ret = vpu_power_virus(kernel_params_ptr->loop_size);
    if (ret == 0) return 0;
    else return -1;
}

