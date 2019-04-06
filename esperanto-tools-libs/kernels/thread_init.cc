// clang-format off
// Note include order matters
#include <stdint.h>
#include "sys_inc.h"
#include "etlibdevice.h"
// clang-format on

///////////////////////////////////////////////////////////////////////////////
// This is the function called after the threads finish the bootrom. They set
// up the mask, barriers and scratchpad and do a WFI and wait for a Maxion to
// send an IPI with the PC of the operation to execute
///////////////////////////////////////////////////////////////////////////////

extern "C" void thread_init() {
  bool is_t0 = ((get_hart_id() & 1) == 0);

  // If minion0-thread0 within shire, initialize barriers
  if (get_lane_id() == 0 && is_t0) {
    // Resets the barriers
    // Atomic region is from ESR area
    for (uint64_t barrier_addr = ATOMIC_REGION;
         barrier_addr < ATOMIC_REGION + 0x100; barrier_addr += 8) {
      *(volatile uint64_t *)barrier_addr = 0;
    }

    // Init first 3x8 bytes of per-shire block shared memory
    unsigned shire_id = get_shire_id();
    uint64_t shire_smem_addr =
        BLOCK_SHARED_REGION + BLOCK_SHARED_REGION_SIZE_PER_SHIRE * shire_id;
    // AmoStore64Global(shire_smem_addr + 0, 0);
    // AmoStore64Global(shire_smem_addr + 8, 0);
    // AmoStore64Global(shire_smem_addr + 16, 0);
    *(volatile uint64_t *)(shire_smem_addr + 0) = 0;
    *(volatile uint64_t *)(shire_smem_addr + 8) = 0;
    *(volatile uint64_t *)(shire_smem_addr + 16) = 0;
    asm volatile("fence");

    // Wake up the other threads but current one
    intra_shire_wake_but_self(MINIONS_PER_SHIRE, MINIONS_PER_SHIRE);
  } else {
    // Other threads go to sleep and wait for initialization
    intra_shire_sleep();
  }

  // Enables 1 elements of FPU
  __asm__ __volatile__("mov.m.x m0, zero, 0x0f\n");

  // Set RM to 4
  __asm__ __volatile__("addi x31, zero, 4\n"
                       "fsrm zero, x31\n"
                       :
                       :
                       : "x31");

  if (is_t0) {
    evict_range_wait((void *)LAUNCH_PARAMS_AREA_BASE, CACHE_LINE_SIZE,
                     CACHEOP_DEST_LVL_L2);
  }

  if (intra_shire_barrier(FLB_BOOT, MINIONS_PER_SHIRE, MINIONS_PER_SHIRE)) {
    intra_shire_wake_but_self(MINIONS_PER_SHIRE, MINIONS_PER_SHIRE);

    evict_range_wait((void *)LAUNCH_PARAMS_AREA_BASE, CACHE_LINE_SIZE,
                     CACHEOP_DEST_LVL_MEM);

    // Inform maxion about initialization completion. Only one thread is doing
    // this
    send_ready_to_maxion();
  }
}

extern "C" void thread_launch(LaunchParams_t *launch_params_p) {
  bool is_raw_kernel = (launch_params_p->state.grid_size_x == 0);
  bool is_t0 = ((get_hart_id() & 1) == 0);

  if (is_raw_kernel) {
    /**
     * This is a so called raw-kernel. Call it as is.
     */
    typedef void (*RawKernelFunction_t)(void *args);
    RawKernelFunction_t raw_kernel_fun =
        (RawKernelFunction_t)launch_params_p->kernel_pc;

    raw_kernel_fun(launch_params_p->args);
  } else {
    /**
     * This is a regular kernel.
     */
    if (is_t0) {
      KernelWorkItemFunction_t kernel_wi_fun =
          (KernelWorkItemFunction_t)launch_params_p->kernel_pc;

      KernelDispatchFunction(launch_params_p->args, &launch_params_p->state,
                             kernel_wi_fun);
    }
  }

  if (is_t0) {
    evict_range_wait(launch_params_p,
                     sizeof(LaunchParams_t) + launch_params_p->args_size,
                     CACHEOP_DEST_LVL_L2);
  }

  // We need a separate barrier for the completely done barrier, as some
  // non-active minions might be done at some point before the other ones,
  // which may perform another barrier "inside a work item function".
  if (intra_shire_barrier(FLB_FINAL, MINIONS_PER_SHIRE, MINIONS_PER_SHIRE)) {
    intra_shire_wake_but_self(MINIONS_PER_SHIRE, MINIONS_PER_SHIRE);

    evict_range_wait(launch_params_p,
                     sizeof(LaunchParams_t) + launch_params_p->args_size,
                     CACHEOP_DEST_LVL_MEM);

    // Sends an IPI to maxion. Only one thread is doing this
    send_ipi_to_maxion();
  }
}

extern "C" uint64_t mtrap_handler(uint64_t cause, uint64_t epc, uint64_t mtval,
                                  uint64_t *regs) {
  // only handling reading of mhartid
  if (cause == CAUSE_ILLEGAL_INSTRUCTION &&
      (mtval & INST_WRITE_TXSLEEP27_MASK) == INST_WRITE_TXSLEEP27_MATCH) {
    uint64_t rd, rs, val;
    rd = (mtval >> 7) & 0x1F;
    rs = (mtval >> 15) & 0x1F;
    __asm__ __volatile__("csrrw %[val], 0x7d1, %[rs] \n"
                         : [val] "=r"(val)
                         : [rs] "r"(regs[rs]));
    regs[rd] = val;
  } else {
    // any other cause => wfi and expect timeout in test
    __asm__ __volatile__("wfi\n");
  }

  return epc + 4; // should return +2 if compressed... because only implementing
                  // csrXX, return 4 directly
}
