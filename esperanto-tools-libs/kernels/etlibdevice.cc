#include <stdint.h>
#include "etlibdevice.h"
#include "sys_inc.h"

extern "C" void KernelDispatchFunction(void* args, KernelRuntimeState_t* state_p, KernelWorkItemFunction_t kernel_wi_fun)
{
    uint64_t thread_id = get_minion_id();

#if 1
    /*
     * Note: we always launch all MINIONS_COUNT hw threads for each kernel launch.
     *
     * Each CUDA Thread Block will take whole number of shires. So if block size is 1 then only one hw thread in
     * a shire will be active, i.e. execute some work item.
     * All work items in a block should run simultaneously in order to be possible to implement intra-block
     * synchronization, i.e. CUDA __syncthreads().
     * So in each point of time some last shires may by completely inactive and some last hw threads in
     * each-block-last-shire may also be inactive.
     */
    unsigned grid_size = state_p->grid_size_x * state_p->grid_size_y * state_p->grid_size_z;
    unsigned block_size = state_p->block_size_x * state_p->block_size_y * state_p->block_size_z;
    unsigned shires_per_block = align_up(block_size, (unsigned)MINIONS_PER_SHIRE) / MINIONS_PER_SHIRE;
    unsigned blocks_per_device = SHIRES_COUNT / shires_per_block; // or simul_blocks
    unsigned active_shires = shires_per_block * blocks_per_device;
    unsigned non_active_minions_per_block = (shires_per_block * MINIONS_PER_SHIRE) - block_size;

    unsigned thisThreadShireId = thread_id / MINIONS_PER_SHIRE;
    if (thisThreadShireId < active_shires)
    {
        unsigned thisThreadSimulBlockId = thisThreadShireId / shires_per_block;
        unsigned thisThreadLocalId = thread_id % (shires_per_block * MINIONS_PER_SHIRE);
        unsigned thisThreadLocalShireId = thisThreadLocalId / MINIONS_PER_SHIRE;
        unsigned thisShireActiveMinions = (thisThreadLocalShireId == shires_per_block - 1) // last shire in block?
                                          ? MINIONS_PER_SHIRE - non_active_minions_per_block
                                          : MINIONS_PER_SHIRE;
        if (thisThreadLocalId < block_size)
        {
            for (unsigned block_id = thisThreadSimulBlockId; block_id < grid_size; block_id += blocks_per_device)
            {
                unsigned local_id = thisThreadLocalId; // index inside the block
                KernelWorkItemState_t wi_state;

                wi_state.local_id_x = (local_id % state_p->block_size_x);
                wi_state.local_id_y = (local_id / state_p->block_size_x) % state_p->block_size_y;
                wi_state.local_id_z = (local_id / state_p->block_size_x) / state_p->block_size_y;

                wi_state.block_id_x = (block_id % state_p->grid_size_x);
                wi_state.block_id_y = (block_id / state_p->grid_size_x) % state_p->grid_size_y;
                wi_state.block_id_z = (block_id / state_p->grid_size_x) / state_p->grid_size_y;

                wi_state.shires_per_block = shires_per_block;
                wi_state.this_shire_active_minions = thisShireActiveMinions;
                
                kernel_wi_fun(args, state_p, &wi_state);
            }
        }
    }
#else
    /*
     * Note: we always use all MINIONS_COUNT hw threads for each kernel launch.
     * Each hw thread will execute wiPerThread work items (CUDA threads), except maybe the last one or several.
     */
    unsigned grid_size = state_p->grid_size_x * state_p->grid_size_y * state_p->grid_size_z;
    unsigned block_size = state_p->block_size_x * state_p->block_size_y * state_p->block_size_z;
    unsigned global_size = grid_size * block_size;
    unsigned wiPerThread = (global_size + MINIONS_COUNT - 1) / MINIONS_COUNT;
    //assert(MINIONS_COUNT * (wiPerThread-1) < global_size <= MINIONS_COUNT * wiPerThread)

    unsigned thisThreadGlobalIdFirst = thread_id * wiPerThread;
    unsigned thisThreadGlobalIdEnd = thisThreadGlobalIdFirst + wiPerThread;
    thisThreadGlobalIdEnd = (global_size < thisThreadGlobalIdEnd) ? global_size : thisThreadGlobalIdEnd;

    KernelWorkItemState_t wi_state;

    for (unsigned global_id = thisThreadGlobalIdFirst; global_id < thisThreadGlobalIdEnd; global_id++)
    {
        unsigned local_id = global_id % block_size; // index inside the block
        unsigned block_id = global_id / block_size; // index of the block

        wi_state.local_id_x = (local_id % state_p->block_size_x);
        wi_state.local_id_y = (local_id / state_p->block_size_x) % state_p->block_size_y;
        wi_state.local_id_z = (local_id / state_p->block_size_x) / state_p->block_size_y;

        wi_state.block_id_x = (block_id % state_p->grid_size_x);
        wi_state.block_id_y = (block_id / state_p->grid_size_x) % state_p->grid_size_y;
        wi_state.block_id_z = (block_id / state_p->grid_size_x) / state_p->grid_size_y;

        kernel_wi_fun(args, state_p, &wi_state);
    }
#endif
}


// Returns storage for '__shared__' variables in kernel
extern "C" char* __et_get_smem_vars_storage(KernelRuntimeState_t *rt_p, KernelWorkItemState_t *wi_p)
{
    // for each block we use block leading shire "per-shire shared memory"
    unsigned leading_shire_id = align_down(get_shire_id(), wi_p->shires_per_block);
    uint64_t block_smem_addr = BLOCK_SHARED_REGION + BLOCK_SHARED_REGION_SIZE_PER_SHIRE * leading_shire_id;

    // if kernel uses block shared mem (and therefore unconditionally called this function
    // at the beginnig) then we must assure that threads from consecutive blocks
    // (binned to this leading shire) will not execute simultaneously
    llvm_nvvm_barrier0(rt_p, wi_p);

    // first 3x8 bytes are used for inter-shire synchronization
    // for kernel '__shared__' memory we use from second cache line
    return (char*)(block_smem_addr + 64);
}

// Implementation semantic of '__syncthreads()'
extern "C" void llvm_nvvm_barrier0(KernelRuntimeState_t *rt_p, KernelWorkItemState_t *wi_p)
{
    // for each block we use block leading shire "per-shire shared memory"
    unsigned leading_shire_id = align_down(get_shire_id(), wi_p->shires_per_block);
    uint64_t block_smem_addr = BLOCK_SHARED_REGION + BLOCK_SHARED_REGION_SIZE_PER_SHIRE * leading_shire_id;

    // first 3x8 bytes are used for inter-shire synchronization
    uint64_t block_inter_shire_smem_addr = block_smem_addr;


    if (intra_shire_barrier(FLB_SWI, wi_p->this_shire_active_minions, 0))
    {
        // only one minion from each shire is here (other minions are sleep)

        // synchronize them on block shared memory (if there are more than one shire in block)
        if (wi_p->shires_per_block > 1)
        {
            uint64_t barrier0_addr = block_inter_shire_smem_addr;
            uint64_t barrier1_addr = block_inter_shire_smem_addr + 8;
            uint64_t curr_idx_addr = block_inter_shire_smem_addr + 16;

            /*
             * We use 2 barrier addrs consecutively swapping them because of
             * using same barrier addr consecutively may lead to deadlock if
             * some threads will (indefinitelly) wait zero in counter while some another
             * threads may get ahead and manage to +1 in next logical barrier.
             */
            asm volatile("fence");
            uint64_t curr_idx = AmoLoad64Global(curr_idx_addr);
            uint64_t addr = (curr_idx & 1) ? barrier1_addr : barrier0_addr;
            if (AmoAdd64Global(addr, 1) == wi_p->shires_per_block - 1) {
                // Last guy
                AmoStore64Global(curr_idx_addr, curr_idx + 1);
                asm volatile("fence");
                AmoStore64Global(addr, 0);
                asm volatile("fence");
            } else {
                // All others a waiting Last guy writes 0
                while (AmoLoad64Global(addr) != 0) {
                    asm volatile("fence");
                }
            }
        }

        // wake all other minions
        intra_shire_wake_but_self(wi_p->this_shire_active_minions, 0);
    }
}


extern "C" unsigned llvm_nvvm_read_ptx_sreg_tid_x(KernelRuntimeState_t *rt_p, KernelWorkItemState_t *wi_p)
{
    return wi_p->local_id_x;
}
extern "C" unsigned llvm_nvvm_read_ptx_sreg_tid_y(KernelRuntimeState_t *rt_p, KernelWorkItemState_t *wi_p)
{
    return wi_p->local_id_y;
}
extern "C" unsigned llvm_nvvm_read_ptx_sreg_tid_z(KernelRuntimeState_t *rt_p, KernelWorkItemState_t *wi_p)
{
    return wi_p->local_id_z;
}

extern "C" unsigned llvm_nvvm_read_ptx_sreg_ctaid_x(KernelRuntimeState_t *rt_p, KernelWorkItemState_t *wi_p)
{
    return wi_p->block_id_x;
}
extern "C" unsigned llvm_nvvm_read_ptx_sreg_ctaid_y(KernelRuntimeState_t *rt_p, KernelWorkItemState_t *wi_p)
{
    return wi_p->block_id_y;
}
extern "C" unsigned llvm_nvvm_read_ptx_sreg_ctaid_z(KernelRuntimeState_t *rt_p, KernelWorkItemState_t *wi_p)
{
    return wi_p->block_id_z;
}

extern "C" unsigned llvm_nvvm_read_ptx_sreg_ntid_x(KernelRuntimeState_t *rt_p, KernelWorkItemState_t *wi_p)
{
    return rt_p->block_size_x;
}
extern "C" unsigned llvm_nvvm_read_ptx_sreg_ntid_y(KernelRuntimeState_t *rt_p, KernelWorkItemState_t *wi_p)
{
    return rt_p->block_size_y;
}
extern "C" unsigned llvm_nvvm_read_ptx_sreg_ntid_z(KernelRuntimeState_t *rt_p, KernelWorkItemState_t *wi_p)
{
    return rt_p->block_size_z;
}

extern "C" unsigned llvm_nvvm_read_ptx_sreg_nctaid_x(KernelRuntimeState_t *rt_p, KernelWorkItemState_t *wi_p)
{
    return rt_p->grid_size_x;
}
extern "C" unsigned llvm_nvvm_read_ptx_sreg_nctaid_y(KernelRuntimeState_t *rt_p, KernelWorkItemState_t *wi_p)
{
    return rt_p->grid_size_y;
}
extern "C" unsigned llvm_nvvm_read_ptx_sreg_nctaid_z(KernelRuntimeState_t *rt_p, KernelWorkItemState_t *wi_p)
{
    return rt_p->grid_size_z;
}


#define ETCC_STUB() assert_unreachable()

extern "C" float llvm_nvvm_atomic_load_add_f32_p0f32(float *address, float val)
{
    ETCC_STUB();
    return 0;
}


/*
 * Implementation of some functions from CUDA libdevice.10.bc
 */

extern "C" int __nv_min(int a, int b)
{
    return device_min(a, b);
}

extern "C" int __nv_max(int a, int b)
{
    return device_max(a, b);
}

extern "C" float __nv_fabsf(float a)
{
    ETCC_STUB();
    return 0;
}

extern "C" float __nv_floorf(float a)
{
    ETCC_STUB();
    return 0;
}

extern "C" float __nv_ceilf(float a)
{
    ETCC_STUB();
    return 0;
}

extern "C" float __nv_roundf(float a)
{
    ETCC_STUB();
    return 0;
}

extern "C" float __nv_fminf(float a, float b)
{
    ETCC_STUB();
    return 0;
}

extern "C" float __nv_fmaxf(float a, float b)
{
    ETCC_STUB();
    return 0;
}

extern "C" float __nv_fast_expf(float a)
{
    ETCC_STUB();
    return 0;
}

extern "C" float __nv_expf(float a)
{
    return device_expf(a);
}

extern "C" float __nv_logf(float a)
{
    ETCC_STUB();
    return 0;
}

extern "C" float __nv_powf(float a, float b)
{
    ETCC_STUB();
    return 0;
}

extern "C" float __nv_powif(float a, int b)
{
    ETCC_STUB();
    return 0;
}

extern "C" float __nv_rsqrtf(float a)
{
    ETCC_STUB();
    return 0;
}

extern "C" float __nv_sqrtf(float a)
{
    ETCC_STUB();
    return 0;
}

extern "C" float __nv_sinf(float a)
{
    ETCC_STUB();
    return 0;
}

extern "C" float __nv_cosf(float a)
{
    ETCC_STUB();
    return 0;
}

extern "C" void __nv_sincosf(float a, float *o1_p, float *o2_p)
{
    ETCC_STUB();
}

extern "C" float __nv_tanhf(float a)
{
    ETCC_STUB();
    return 0;
}



