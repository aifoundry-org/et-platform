# -------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
# -------------------------------------------------------------------------

emu_hdrs := \
	atomics.h \
	cache.h \
	csrs.h \
	decode.h \
	devices/DW_apb_timers.h \
	devices/cru.h \
	devices/efuse.h \
	devices/i2c.h \
	devices/pcie_apb_subsys.h \
	devices/pcie_dbi_slv.h \
	devices/pcie_dma.h \
	devices/pcie_esr.h \
	devices/pcie_nopcie_esr.h \
	devices/pcie_usr_esr.h \
	devices/plic.h \
	devices/pll.h \
	devices/rvtimer.h \
	devices/shire_lpddr.h \
	devices/spi.h \
	devices/spio_misc_region.h \
	devices/spio_rvtimer_region.h \
	devices/uart.h \
	emu_defines.h \
	emu_gio.h \
	esrs.h \
	gold.h \
	insn.h \
	insn_func.h \
	insn_util.h \
	literals.h \
	memmap.h \
	memory/dense_region.h \
	memory/dump_data.h \
	memory/main_memory.h \
	memory/memory_error.h \
	memory/memory_region.h \
	memory/scratch_region.h \
	memory/sparse_region.h \
	memory/sysreg_region.h \
	mmu.h \
	processor.h \
	state.h \
	support/intrusive/detail/member_pointer.h \
	support/intrusive/list.h \
	support/lazy_array.h \
	sysreg_error.h \
	system.h \
	tensor.h \
	traps.h \
	utility.h

emu_cpp_srcs := \
	agent.cpp \
	debugmodule.cpp \
	devices/pcie_dma.cpp \
	devices/spio_misc_region.cpp \
	emu_gio.cpp \
	esrs.cpp \
	flb.cpp \
	gold.cpp \
	insns/arith.cpp \
	insns/arith_atomic.cpp \
	insns/arith_graphics.cpp \
	insns/arith_loadstore.cpp \
	insns/branch.cpp \
	insns/c_arith.cpp \
	insns/c_branch.cpp \
	insns/c_loadstore.cpp \
	insns/cache_control.cpp \
	insns/coherent_arith_loadstore.cpp \
	insns/coherent_packed_loadstore.cpp \
	insns/float.cpp \
	insns/float_loadstore.cpp \
	insns/muldiv.cpp \
	insns/packed_arith.cpp \
	insns/packed_atomic.cpp \
	insns/packed_float.cpp \
	insns/packed_graphics.cpp \
	insns/packed_loadstore.cpp \
	insns/packed_mask.cpp \
	insns/packed_trans.cpp \
	insns/system.cpp \
	insns/tensors.cpp \
	insns/zicsr.cpp \
	insns/zifencei.cpp \
	memory/main_memory.cpp \
	mmu.cpp \
	msgport.cpp \
	processor.cpp \
	system.cpp
