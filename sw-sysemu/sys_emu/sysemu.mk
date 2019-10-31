sysemu_hdrs := \
    sys_emu/api_communicate.h \
    sys_emu/log.h \
    sys_emu/mem_directory.h \
    sys_emu/scp_directory.h \
    sys_emu/sys_emu.h \
    sys_emu/testLog.h \
    sys_emu/utils.h

sysemu_cpp_srcs := \
    sys_emu/log.cpp \
    sys_emu/mem_directory.cpp \
    sys_emu/scp_directory.cpp \
    sys_emu/sys_emu.cpp \
    sys_emu/sys_emu_debug.cpp \
    sys_emu/sys_emu_main.cpp \
    sys_emu/sys_emu_parse_args.cpp \
    sys_emu/testLog.cpp \
    sys_emu/utils.cpp

ifeq ($(PROFILING), 1)
  sysemu_hdrs     += sys_emu/profiling.h
  sysemu_cpp_srcs += sys_emu/profiling.cpp
endif
