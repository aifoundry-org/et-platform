emu_hdrs := \
	emu.h \
	emu_casts.h \
	emu_defines.h \
	emu_gio.h \
	emu_memop.h \
	gold.h \
	insn.h \
	insn_exec_func.h \
	rbox.h \
	tbox_emu.h \
	txs.h \
	common/main_memory.h \
	common/main_memory_region.h \
	common/main_memory_region_atomic.h \
	common/main_memory_region_esr.h \
	common/main_memory_region_rbox.h \
	common/main_memory_region_scp_linear.h

emu_cpp_srcs := \
	decode.cpp \
	emu.cpp \
	emu_gio.cpp \
	gold.cpp \
	insn_exec_func.cpp \
	tbox_emu.cpp \
	txs.cpp \
	rbox.cpp \
	common/main_memory.cpp \
	common/main_memory_region.cpp \
	common/main_memory_region_atomic.cpp \
	common/main_memory_region_esr.cpp \
	common/main_memory_region_rbox.cpp \
	common/main_memory_region_scp.cpp \
	common/main_memory_region_scp_linear.cpp
