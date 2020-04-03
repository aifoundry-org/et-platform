# -------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
# -------------------------------------------------------------------------

sysemu_hdrs := \
    sys_emu/api_communicate.h \
    sys_emu/gdbstub.h \
    sys_emu/log.h \
    sys_emu/sys_emu.h \
    sys_emu/testLog.h \
    sys_emu/utils.h \
    sys_emu/checkers/mem_checker.h \
    sys_emu/checkers/l1_scp_checker.h \
    sys_emu/checkers/l2_scp_checker.h

sysemu_cpp_srcs := \
    sys_emu/gdbstub.cpp \
    sys_emu/log.cpp \
    sys_emu/sys_emu.cpp \
    sys_emu/sys_emu_debug.cpp \
    sys_emu/sys_emu_main.cpp \
    sys_emu/sys_emu_parse_args.cpp \
    sys_emu/testLog.cpp \
    sys_emu/utils.cpp \
    sys_emu/checkers/mem_checker.cpp \
    sys_emu/checkers/l1_scp_checker.cpp \
    sys_emu/checkers/l2_scp_checker.cpp

ifeq ($(PROFILING), 1)
  sysemu_hdrs     += sys_emu/profiling.h
  sysemu_cpp_srcs += sys_emu/profiling.cpp
endif
