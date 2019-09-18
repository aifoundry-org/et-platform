sysemu_hdrs := \
	sys_emu/api_communicate.h \
	sys_emu/sys_emu.h \
	sys_emu/log.h \
	sys_emu/testLog.h \
	sys_emu/rvtimer.h

sysemu_cpp_srcs := \
	sys_emu/api_communicate.cpp \
	sys_emu/log.cpp \
	sys_emu/net_emulator.cpp \
	sys_emu/sys_emu.cpp \
	sys_emu/sys_emu_debug.cpp \
	sys_emu/sys_emu_main.cpp \
	sys_emu/testLog.cpp

ifeq ($(COHERENCY), 1)
  sysemu_hdrs     += sys_emu/mem_directory.h
  sysemu_cpp_srcs += sys_emu/mem_directory.cpp

endif

ifeq ($(PROFILING), 1)
  sysemu_hdrs     += sys_emu/profiling.h
  sysemu_cpp_srcs += sys_emu/profiling.cpp
endif
