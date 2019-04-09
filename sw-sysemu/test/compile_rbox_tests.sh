#!/usr/bin/bash

echo "First build libfpu.a"

pushd ../build
make
popd

echo "Then compile the test binary"

# NOTE : There is no static version of libemu!!

INCLUDE_DIRS="-I ../ -I ../sys_emu/ -I ../common/"
SOURCE_FILES="../sys_emu/sys_emu_log.cpp ../common/main_memory.cpp ../common/main_memory_region.cpp \
              ../common/main_memory_region_atomic.cpp ../common/main_memory_region_esr.cpp \
              ../common/main_memory_region_rbox.cpp ../common/main_memory_region_scp_linear.cpp \
			  rbox_test.c"
LIBRARY_FILES=" ../build/libfpu.a ../build/libemu.a -lm"

g++ -mf16c -std=c++11 -MMD -MP -Wall -Werror -fPIC -static -O2 $INCLUDE_DIRS $EMU_FILES $SOURCE_FILES $LIBRARY_FILES -o rbox_test

