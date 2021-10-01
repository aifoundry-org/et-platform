// Local
#include <stdint.h>
#include <hart.h>

#define N_SHIRES_COMPUTE 32

#include "etsoc/common/utils.h"
#include "utils_int.h"
#include "test_common.h"

// Code includes
#include "test_compute.cc"
#include "test_compute_pass0.cc"
#include "test_compute_pass1.cc"
#include "test_compute_pass2.cc"
#include "test_helper_activation.cc"
#include "test_helper_activation_pass0.cc"
#include "test_helper_activation_pass1.cc"
#include "test_helper_activation_pass2.cc"
#include "test_helper_weights.cc"
#include "test_helper_weights_pass0.cc"
#include "test_helper_weights_pass1.cc"
#include "test_helper_weights_pass2.cc"
#include "test_helper_drain.cc"
#include "test_helper_drain_pass0.cc"
#include "test_helper_drain_pass1.cc"
#include "test_helper_drain_pass2.cc"
#include "test_helper_code.cc"
#include "test_helper_code_pass0.cc"
#include "test_helper_code_pass1.cc"
#include "test_helper_code_pass2.cc"
#include "test_sync.cc"

// This is the entry point for the test
int64_t main(void)
{
    // Gets hart id and other ids
    uint32_t hart = get_hart_id();
    uint32_t thread_id = hart & 1;
    uint32_t minion_id = hart >> 1;
    uint32_t shire_id = minion_id >> 5;
    minion_id = minion_id & 0x1F;

    // Selects type of function based on position
    // Compute minions

    uint32_t shire_thr = N_SHIRES_COMPUTE;
    if((shire_id < shire_thr) && (thread_id == 0))
        test_compute(shire_id, minion_id);
    else if((shire_id < shire_thr) && (thread_id == 1) && (HELPER_ACTIVATION_THREADS & (1 << minion_id)))
        test_helper_activation(shire_id, minion_id);
    else if((shire_id < shire_thr) && (thread_id == 1) && (HELPER_WEIGHTS_THREADS    & (1 << minion_id)))
        test_helper_weights(shire_id, minion_id);
    else if((shire_id < shire_thr) && (thread_id == 1) && (HELPER_DRAIN_THREADS      & (1 << minion_id)))
        test_helper_drain(shire_id, minion_id);
    else if((shire_id < shire_thr) && (thread_id == 1) && (HELPER_CODE_THREADS       & (1 << minion_id)))
        test_helper_code(shire_id, minion_id);
    else if(shire_id == 32) {
        test_sync(minion_id, thread_id);
    }

    et_printf("MLP Test done\n");

    return 0;
}

