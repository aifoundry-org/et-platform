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
	devices/plic.h \
	devices/pu_uart.h \
	devices/rvtimer.h \
	emu_defines.h \
	emu_gio.h \
	esrs.h \
	gold.h \
	insn.h \
	insn_func.h \
	insn_util.h \
	insns/tensor_error.h \
	lazy_array.h \
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
	sysreg_error.h \
	system.h \
	tensor.h \
	traps.h \
	utility.h

emu_cpp_srcs := \
	devices/pcie_dma.cpp \
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
	system.cpp \
	traps.cpp
