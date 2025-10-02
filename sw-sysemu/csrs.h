/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

//
// RISCV registers
//

CSRDEF(0x000, ustatus, USTATUS)         // not implemented
CSRDEF(0x001, fflags, FFLAGS)
CSRDEF(0x002, frm, FRM)
CSRDEF(0x003, fcsr, FCSR)
CSRDEF(0x004, uie, UIE)                 // not implemented
CSRDEF(0x005, utvec, UTVEC)             // not implemented
CSRDEF(0x040, uscratch, USCRATCH)       // not implemented
CSRDEF(0x041, uepc, UEPC)               // not implemented
CSRDEF(0x042, ucause, UCAUSE)           // not implemented
CSRDEF(0x043, utval, UTVAL)             // not implemented
CSRDEF(0x044, uip, UIP)                 // not implemented
CSRDEF(0x100, sstatus, SSTATUS)
CSRDEF(0x102, sedeleg, SEDELEG)         // not implemented
CSRDEF(0x103, sideleg, SIDELEG)         // not implemented
CSRDEF(0x104, sie, SIE)
CSRDEF(0x105, stvec, STVEC)
CSRDEF(0x106, scounteren, SCOUNTEREN)
CSRDEF(0x140, sscratch, SSCRATCH)
CSRDEF(0x141, sepc, SEPC)
CSRDEF(0x142, scause, SCAUSE)
CSRDEF(0x143, stval, STVAL)
CSRDEF(0x144, sip, SIP)
CSRDEF(0x180, satp, SATP)
CSRDEF(0x300, mstatus, MSTATUS)
CSRDEF(0x301, misa, MISA)
CSRDEF(0x302, medeleg, MEDELEG)
CSRDEF(0x303, mideleg, MIDELEG)
CSRDEF(0x304, mie, MIE)
CSRDEF(0x305, mtvec, MTVEC)
CSRDEF(0x306, mcounteren, MCOUNTEREN)
CSRDEF(0x320, mcountinhibit, MCOUNTINHIBIT)
CSRDEF(0x323, mhpmevent3, MHPMEVENT3)
CSRDEF(0x324, mhpmevent4, MHPMEVENT4)
CSRDEF(0x325, mhpmevent5, MHPMEVENT5)
CSRDEF(0x326, mhpmevent6, MHPMEVENT6)
CSRDEF(0x327, mhpmevent7, MHPMEVENT7)
CSRDEF(0x328, mhpmevent8, MHPMEVENT8)
CSRDEF(0x329, mhpmevent9, MHPMEVENT9)
CSRDEF(0x32a, mhpmevent10, MHPMEVENT10)
CSRDEF(0x32b, mhpmevent11, MHPMEVENT11)
CSRDEF(0x32c, mhpmevent12, MHPMEVENT12)
CSRDEF(0x32d, mhpmevent13, MHPMEVENT13)
CSRDEF(0x32e, mhpmevent14, MHPMEVENT14)
CSRDEF(0x32f, mhpmevent15, MHPMEVENT15)
CSRDEF(0x330, mhpmevent16, MHPMEVENT16)
CSRDEF(0x331, mhpmevent17, MHPMEVENT17)
CSRDEF(0x332, mhpmevent18, MHPMEVENT18)
CSRDEF(0x333, mhpmevent19, MHPMEVENT19)
CSRDEF(0x334, mhpmevent20, MHPMEVENT20)
CSRDEF(0x335, mhpmevent21, MHPMEVENT21)
CSRDEF(0x336, mhpmevent22, MHPMEVENT22)
CSRDEF(0x337, mhpmevent23, MHPMEVENT23)
CSRDEF(0x338, mhpmevent24, MHPMEVENT24)
CSRDEF(0x339, mhpmevent25, MHPMEVENT25)
CSRDEF(0x33a, mhpmevent26, MHPMEVENT26)
CSRDEF(0x33b, mhpmevent27, MHPMEVENT27)
CSRDEF(0x33c, mhpmevent28, MHPMEVENT28)
CSRDEF(0x33d, mhpmevent29, MHPMEVENT29)
CSRDEF(0x33e, mhpmevent30, MHPMEVENT30)
CSRDEF(0x33f, mhpmevent31, MHPMEVENT31)
CSRDEF(0x340, mscratch, MSCRATCH)
CSRDEF(0x341, mepc, MEPC)
CSRDEF(0x342, mcause, MCAUSE)
CSRDEF(0x343, mtval, MTVAL)
CSRDEF(0x344, mip, MIP)
CSRDEF(0x3a0, pmpcfg0, PMPCFG0)         // not implemented
CSRDEF(0x3a1, pmpcfg1, PMPCFG1)         // not implemented
CSRDEF(0x3a2, pmpcfg2, PMPCFG2)         // not implemented
CSRDEF(0x3a3, pmpcfg3, PMPCFG3)         // not implemented
CSRDEF(0x3b0, pmpaddr0, PMPADDR0)       // not implemented
CSRDEF(0x3b1, pmpaddr1, PMPADDR1)       // not implemented
CSRDEF(0x3b2, pmpaddr2, PMPADDR2)       // not implemented
CSRDEF(0x3b3, pmpaddr3, PMPADDR3)       // not implemented
CSRDEF(0x3b4, pmpaddr4, PMPADDR4)       // not implemented
CSRDEF(0x3b5, pmpaddr5, PMPADDR5)       // not implemented
CSRDEF(0x3b6, pmpaddr6, PMPADDR6)       // not implemented
CSRDEF(0x3b7, pmpaddr7, PMPADDR7)       // not implemented
CSRDEF(0x3b8, pmpaddr8, PMPADDR8)       // not implemented
CSRDEF(0x3b9, pmpaddr9, PMPADDR9)       // not implemented
CSRDEF(0x3ba, pmpaddr10, PMPADDR10)     // not implemented
CSRDEF(0x3bb, pmpaddr11, PMPADDR11)     // not implemented
CSRDEF(0x3bc, pmpaddr12, PMPADDR12)     // not implemented
CSRDEF(0x3bd, pmpaddr13, PMPADDR13)     // not implemented
CSRDEF(0x3be, pmpaddr14, PMPADDR14)     // not implemented
CSRDEF(0x3bf, pmpaddr15, PMPADDR15)     // not implemented
CSRDEF(0x7a0, tselect, TSELECT)
CSRDEF(0x7a1, tdata1, TDATA1)
CSRDEF(0x7a2, tdata2, TDATA2)
CSRDEF(0x7a3, tdata3, TDATA3)           // not implemented
CSRDEF(0x7a4, tinfo, TINFO)             // not implemented
CSRDEF(0x7b0, dcsr, DCSR)
CSRDEF(0x7b1, dpc, DPC)
CSRDEF(0x7b2, dscratch0, DSCRATCH0)     // not implemented
CSRDEF(0x7b3, dscratch1, DSCRATCH1)     // not implemented
CSRDEF(0xb00, mcycle, MCYCLE)
CSRDEF(0xb02, minstret, MINSTRET)
CSRDEF(0xb03, mhpmcounter3, MHPMCOUNTER3)
CSRDEF(0xb04, mhpmcounter4, MHPMCOUNTER4)
CSRDEF(0xb05, mhpmcounter5, MHPMCOUNTER5)
CSRDEF(0xb06, mhpmcounter6, MHPMCOUNTER6)
CSRDEF(0xb07, mhpmcounter7, MHPMCOUNTER7)
CSRDEF(0xb08, mhpmcounter8, MHPMCOUNTER8)
CSRDEF(0xb09, mhpmcounter9, MHPMCOUNTER9)
CSRDEF(0xb0a, mhpmcounter10, MHPMCOUNTER10)
CSRDEF(0xb0b, mhpmcounter11, MHPMCOUNTER11)
CSRDEF(0xb0c, mhpmcounter12, MHPMCOUNTER12)
CSRDEF(0xb0d, mhpmcounter13, MHPMCOUNTER13)
CSRDEF(0xb0e, mhpmcounter14, MHPMCOUNTER14)
CSRDEF(0xb0f, mhpmcounter15, MHPMCOUNTER15)
CSRDEF(0xb10, mhpmcounter16, MHPMCOUNTER16)
CSRDEF(0xb11, mhpmcounter17, MHPMCOUNTER17)
CSRDEF(0xb12, mhpmcounter18, MHPMCOUNTER18)
CSRDEF(0xb13, mhpmcounter19, MHPMCOUNTER19)
CSRDEF(0xb14, mhpmcounter20, MHPMCOUNTER20)
CSRDEF(0xb15, mhpmcounter21, MHPMCOUNTER21)
CSRDEF(0xb16, mhpmcounter22, MHPMCOUNTER22)
CSRDEF(0xb17, mhpmcounter23, MHPMCOUNTER23)
CSRDEF(0xb18, mhpmcounter24, MHPMCOUNTER24)
CSRDEF(0xb19, mhpmcounter25, MHPMCOUNTER25)
CSRDEF(0xb1a, mhpmcounter26, MHPMCOUNTER26)
CSRDEF(0xb1b, mhpmcounter27, MHPMCOUNTER27)
CSRDEF(0xb1c, mhpmcounter28, MHPMCOUNTER28)
CSRDEF(0xb1d, mhpmcounter29, MHPMCOUNTER29)
CSRDEF(0xb1e, mhpmcounter30, MHPMCOUNTER30)
CSRDEF(0xb1f, mhpmcounter31, MHPMCOUNTER31)
CSRDEF(0xb80, mcycleh, MCYCLEH)                 // not implemented
CSRDEF(0xb82, minstreth, MINSTRETH)             // not implemented
CSRDEF(0xb83, mhpmcounter3h, MHPMCOUNTER3H)     // not implemented
CSRDEF(0xb84, mhpmcounter4h, MHPMCOUNTER4H)     // not implemented
CSRDEF(0xb85, mhpmcounter5h, MHPMCOUNTER5H)     // not implemented
CSRDEF(0xb86, mhpmcounter6h, MHPMCOUNTER6H)     // not implemented
CSRDEF(0xb87, mhpmcounter7h, MHPMCOUNTER7H)     // not implemented
CSRDEF(0xb88, mhpmcounter8h, MHPMCOUNTER8H)     // not implemented
CSRDEF(0xb89, mhpmcounter9h, MHPMCOUNTER9H)     // not implemented
CSRDEF(0xb8a, mhpmcounter10h, MHPMCOUNTER10H)   // not implemented
CSRDEF(0xb8b, mhpmcounter11h, MHPMCOUNTER11H)   // not implemented
CSRDEF(0xb8c, mhpmcounter12h, MHPMCOUNTER12H)   // not implemented
CSRDEF(0xb8d, mhpmcounter13h, MHPMCOUNTER13H)   // not implemented
CSRDEF(0xb8e, mhpmcounter14h, MHPMCOUNTER14H)   // not implemented
CSRDEF(0xb8f, mhpmcounter15h, MHPMCOUNTER15H)   // not implemented
CSRDEF(0xb90, mhpmcounter16h, MHPMCOUNTER16H)   // not implemented
CSRDEF(0xb91, mhpmcounter17h, MHPMCOUNTER17H)   // not implemented
CSRDEF(0xb92, mhpmcounter18h, MHPMCOUNTER18H)   // not implemented
CSRDEF(0xb93, mhpmcounter19h, MHPMCOUNTER19H)   // not implemented
CSRDEF(0xb94, mhpmcounter20h, MHPMCOUNTER20H)   // not implemented
CSRDEF(0xb95, mhpmcounter21h, MHPMCOUNTER21H)   // not implemented
CSRDEF(0xb96, mhpmcounter22h, MHPMCOUNTER22H)   // not implemented
CSRDEF(0xb97, mhpmcounter23h, MHPMCOUNTER23H)   // not implemented
CSRDEF(0xb98, mhpmcounter24h, MHPMCOUNTER24H)   // not implemented
CSRDEF(0xb99, mhpmcounter25h, MHPMCOUNTER25H)   // not implemented
CSRDEF(0xb9a, mhpmcounter26h, MHPMCOUNTER26H)   // not implemented
CSRDEF(0xb9b, mhpmcounter27h, MHPMCOUNTER27H)   // not implemented
CSRDEF(0xb9c, mhpmcounter28h, MHPMCOUNTER28H)   // not implemented
CSRDEF(0xb9d, mhpmcounter29h, MHPMCOUNTER29H)   // not implemented
CSRDEF(0xb9e, mhpmcounter30h, MHPMCOUNTER30H)   // not implemented
CSRDEF(0xb9f, mhpmcounter31h, MHPMCOUNTER31H)   // not implemented
CSRDEF(0xc00, cycle, CYCLE)
CSRDEF(0xc01, time, TIME)                       // not implemented
CSRDEF(0xc02, instret, INSTRET)
CSRDEF(0xc03, hpmcounter3, HPMCOUNTER3)
CSRDEF(0xc04, hpmcounter4, HPMCOUNTER4)
CSRDEF(0xc05, hpmcounter5, HPMCOUNTER5)
CSRDEF(0xc06, hpmcounter6, HPMCOUNTER6)
CSRDEF(0xc07, hpmcounter7, HPMCOUNTER7)
CSRDEF(0xc08, hpmcounter8, HPMCOUNTER8)
CSRDEF(0xc09, hpmcounter9, HPMCOUNTER9)
CSRDEF(0xc0a, hpmcounter10, HPMCOUNTER10)
CSRDEF(0xc0b, hpmcounter11, HPMCOUNTER11)
CSRDEF(0xc0c, hpmcounter12, HPMCOUNTER12)
CSRDEF(0xc0d, hpmcounter13, HPMCOUNTER13)
CSRDEF(0xc0e, hpmcounter14, HPMCOUNTER14)
CSRDEF(0xc0f, hpmcounter15, HPMCOUNTER15)
CSRDEF(0xc10, hpmcounter16, HPMCOUNTER16)
CSRDEF(0xc11, hpmcounter17, HPMCOUNTER17)
CSRDEF(0xc12, hpmcounter18, HPMCOUNTER18)
CSRDEF(0xc13, hpmcounter19, HPMCOUNTER19)
CSRDEF(0xc14, hpmcounter20, HPMCOUNTER20)
CSRDEF(0xc15, hpmcounter21, HPMCOUNTER21)
CSRDEF(0xc16, hpmcounter22, HPMCOUNTER22)
CSRDEF(0xc17, hpmcounter23, HPMCOUNTER23)
CSRDEF(0xc18, hpmcounter24, HPMCOUNTER24)
CSRDEF(0xc19, hpmcounter25, HPMCOUNTER25)
CSRDEF(0xc1a, hpmcounter26, HPMCOUNTER26)
CSRDEF(0xc1b, hpmcounter27, HPMCOUNTER27)
CSRDEF(0xc1c, hpmcounter28, HPMCOUNTER28)
CSRDEF(0xc1d, hpmcounter29, HPMCOUNTER29)
CSRDEF(0xc1e, hpmcounter30, HPMCOUNTER30)
CSRDEF(0xc1f, hpmcounter31, HPMCOUNTER31)
CSRDEF(0xc80, cycleh, CYCLEH)                   // not implemented
CSRDEF(0xc81, timeh, TIMEH)                     // not implemented
CSRDEF(0xc82, instreth, INSTRETH)               // not implemented
CSRDEF(0xc83, hpmcounter3h, HPMCOUNTER3H)       // not implemented
CSRDEF(0xc84, hpmcounter4h, HPMCOUNTER4H)       // not implemented
CSRDEF(0xc85, hpmcounter5h, HPMCOUNTER5H)       // not implemented
CSRDEF(0xc86, hpmcounter6h, HPMCOUNTER6H)       // not implemented
CSRDEF(0xc87, hpmcounter7h, HPMCOUNTER7H)       // not implemented
CSRDEF(0xc88, hpmcounter8h, HPMCOUNTER8H)       // not implemented
CSRDEF(0xc89, hpmcounter9h, HPMCOUNTER9H)       // not implemented
CSRDEF(0xc8a, hpmcounter10h, HPMCOUNTER10H)     // not implemented
CSRDEF(0xc8b, hpmcounter11h, HPMCOUNTER11H)     // not implemented
CSRDEF(0xc8c, hpmcounter12h, HPMCOUNTER12H)     // not implemented
CSRDEF(0xc8d, hpmcounter13h, HPMCOUNTER13H)     // not implemented
CSRDEF(0xc8e, hpmcounter14h, HPMCOUNTER14H)     // not implemented
CSRDEF(0xc8f, hpmcounter15h, HPMCOUNTER15H)     // not implemented
CSRDEF(0xc90, hpmcounter16h, HPMCOUNTER16H)     // not implemented
CSRDEF(0xc91, hpmcounter17h, HPMCOUNTER17H)     // not implemented
CSRDEF(0xc92, hpmcounter18h, HPMCOUNTER18H)     // not implemented
CSRDEF(0xc93, hpmcounter19h, HPMCOUNTER19H)     // not implemented
CSRDEF(0xc94, hpmcounter20h, HPMCOUNTER20H)     // not implemented
CSRDEF(0xc95, hpmcounter21h, HPMCOUNTER21H)     // not implemented
CSRDEF(0xc96, hpmcounter22h, HPMCOUNTER22H)     // not implemented
CSRDEF(0xc97, hpmcounter23h, HPMCOUNTER23H)     // not implemented
CSRDEF(0xc98, hpmcounter24h, HPMCOUNTER24H)     // not implemented
CSRDEF(0xc99, hpmcounter25h, HPMCOUNTER25H)     // not implemented
CSRDEF(0xc9a, hpmcounter26h, HPMCOUNTER26H)     // not implemented
CSRDEF(0xc9b, hpmcounter27h, HPMCOUNTER27H)     // not implemented
CSRDEF(0xc9c, hpmcounter28h, HPMCOUNTER28H)     // not implemented
CSRDEF(0xc9d, hpmcounter29h, HPMCOUNTER29H)     // not implemented
CSRDEF(0xc9e, hpmcounter30h, HPMCOUNTER30H)     // not implemented
CSRDEF(0xc9f, hpmcounter31h, HPMCOUNTER31H)     // not implemented
CSRDEF(0xf11, mvendorid, MVENDORID)
CSRDEF(0xf12, marchid, MARCHID)
CSRDEF(0xf13, mimpid, MIMPID)
CSRDEF(0xf14, mhartid, MHARTID)

//
// Esperanto registers
//

CSRDEF(0x7b8, ddata0, DDATA0)
CSRDEF(0x7c0, matp, MATP)
CSRDEF(0x7cd, minstmask, MINSTMASK)
CSRDEF(0x7ce, minstmatch, MINSTMATCH)
CSRDEF(0x7d0, cache_invalidate, CACHE_INVALIDATE)
CSRDEF(0x7d2, menable_shadows, MENABLE_SHADOWS)
CSRDEF(0x7d3, excl_mode, EXCL_MODE)
CSRDEF(0x7d5, mbusaddr, MBUSADDR)
CSRDEF(0x7e0, mcache_control, MCACHE_CONTROL)
CSRDEF(0x7f9, evict_sw, EVICT_SW)
CSRDEF(0x7fb, flush_sw, FLUSH_SW)
CSRDEF(0x7fd, lock_sw, LOCK_SW)
CSRDEF(0x7ff, unlock_sw, UNLOCK_SW)
CSRDEF(0x800, tensor_reduce, TENSOR_REDUCE)
CSRDEF(0x801, tensor_fma, TENSOR_FMA)
CSRDEF(0x802, tensor_conv_size, TENSOR_CONV_SIZE)
CSRDEF(0x803, tensor_conv_ctrl, TENSOR_CONV_CTRL)
CSRDEF(0x804, tensor_coop, TENSOR_COOP)
CSRDEF(0x805, tensor_mask, TENSOR_MASK)
CSRDEF(0x806, tensor_quant, TENSOR_QUANT)
CSRDEF(0x807, tex_send, TEX_SEND)
CSRDEF(0x808, tensor_error, TENSOR_ERROR)
CSRDEF(0x810, ucache_control, UCACHE_CONTROL)
CSRDEF(0x81f, prefetch_va, PREFETCH_VA)
CSRDEF(0x820, flb, FLB)
CSRDEF(0x821, fcc, FCC)
CSRDEF(0x822, stall, STALL)
CSRDEF(0x830, tensor_wait, TENSOR_WAIT)
CSRDEF(0x83f, tensor_load, TENSOR_LOAD)
CSRDEF(0x840, gsc_progress, GSC_PROGRESS)
CSRDEF(0x85f, tensor_load_l2, TENSOR_LOAD_L2)
CSRDEF(0x87f, tensor_store, TENSOR_STORE)
CSRDEF(0x89f, evict_va, EVICT_VA)
CSRDEF(0x8bf, flush_va, FLUSH_VA)
CSRDEF(0x8d0, validation0, VALIDATION0)
CSRDEF(0x8d1, validation1, VALIDATION1)
CSRDEF(0x8d2, validation2, VALIDATION2)
CSRDEF(0x8d3, validation3, VALIDATION3)
CSRDEF(0x8df, lock_va, LOCK_VA)
CSRDEF(0x8ff, unlock_va, UNLOCK_VA)
CSRDEF(0x9cc, portctrl0, PORTCTRL0)
CSRDEF(0x9cd, portctrl1, PORTCTRL1)
CSRDEF(0x9ce, portctrl2, PORTCTRL2)
CSRDEF(0x9cf, portctrl3, PORTCTRL3)
CSRDEF(0xcc0, fccnb, FCCNB)
CSRDEF(0xcc8, porthead0, PORTHEAD0)
CSRDEF(0xcc9, porthead1, PORTHEAD1)
CSRDEF(0xcca, porthead2, PORTHEAD2)
CSRDEF(0xccb, porthead3, PORTHEAD3)
CSRDEF(0xccc, portheadnb0, PORTHEADNB0)
CSRDEF(0xccd, portheadnb1, PORTHEADNB1)
CSRDEF(0xcce, portheadnb2, PORTHEADNB2)
CSRDEF(0xccf, portheadnb3, PORTHEADNB3)
CSRDEF(0xcd0, hartid, HARTID)
CSRDEF(0xfc0, dcache_debug, DCACHE_DEBUG)
