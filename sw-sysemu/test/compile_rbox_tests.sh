#!/usr/bin/bash

echo "First build libfpu.a"

pushd ../build
make
popd

echo "Then compile the test binary"

# NOTE : There is no static version of libemu!!

EMU_FILES="../emu.cpp ../emu_gio.cpp ../insn_dec_func.cpp ../insn_exec_func.cpp ../log.cpp ../tbox_emu.cpp ../txs.cpp ../rbox.cpp"
INCLUDE_DIRS="-I ../ -I ../sys_emu/ -I ../common/"
SOURCE_FILES="../sys_emu/sys_emu_log.cpp ../common/main_memory.cpp ../common/main_memory_region.cpp \
              ../common/main_memory_region_atomic.cpp ../common/main_memory_region_esr.cpp \
              ../common/main_memory_region_rbox.cpp rbox_test.c"
LIBRARY_FILES=" ../build/libfpu.a -lm"

g++ -mf16c -std=c++11 -MMD -MP -Wall -Werror -fPIC -static -O2 $INCLUDE_DIRS $EMU_FILES $SOURCE_FILES $LIBRARY_FILES -o rbox_test

