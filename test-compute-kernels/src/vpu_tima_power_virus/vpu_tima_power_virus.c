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
    int tid = (int)get_thread_id();
    uint64_t ret = 0;
    if (tid == 0) {
        uint64_t loop_size = kernel_params_ptr->loop_size;
        loop_size = 1;
        ret = vpu_tima_power_virus(loop_size);
    }
    if (ret == 0) return 0;
    else return -1;
}
