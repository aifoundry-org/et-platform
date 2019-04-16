#ifndef LAYOUT_H
#define LAYOUT_H

#define FW_MACHINE_MMODE_ENTRY 0x8000001000
#define FW_MACHINE_MMODE_STACK_BASE 0x80001FFE00

// TODO FIXME can't fetch instructions from L3_DRAM see RTLMIN-3674
// #define FW_MASTER_SMODE_ENTRY  0x8100000000
// #define FW_WORKER_SMODE_ENTRY  0x8100200000
// Working around this for now by placing all code in L3_Linux region starting at 0x8000200000


#define FW_MASTER_SMODE_ENTRY  0x8000200000
#define FW_WORKER_SMODE_ENTRY  0x8000400000
#define FW_SMODE_STACK_BASE    0x8000800000

#define KERNEL_STACK_BASE      0x8001000000
#define KERNEL_STACK_SIZE      4096 // bytes

#endif
