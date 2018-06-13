target: build libchecker build/sys_emu

GCC=g++
CFLAGS=-g -O3 -DCHECKER -I../checker -I.. -I$(TESTROOT)/libs -I$(SGPU)/include -I -DEMU_DEBUG -DSYS_EMU $(HASH_FLAGS)
HASH_FLAGS=-std=c++11
LIBS=../checker/build/libchecker.a
LIBS_LINK=-ldl -lboost_regex
OBJS=build/testLog.o build/net_emulator.o build/rboxSysEmu.o build/trace.o

build/sys_emu: $(OBJS) $(LIBS) sys_emu.cpp
	$(GCC) $(CFLAGS) $(HASH_FLAGS) $(LIBS_LINK) $(OBJS) sys_emu.cpp -o build/sys_emu $(LIBS)

build/testLog.o: $(TESTROOT)/libs/testLog.cc $(TESTROOT)/libs/testLog.h
	$(GCC) -c $(CFLAGS) $(TESTROOT)/libs/testLog.cc -o build/testLog.o

build/net_emulator.o: net_emulator.cpp net_emulator.h
	$(GCC) -c $(CFLAGS) net_emulator.cpp -o build/net_emulator.o

build/rboxSysEmu.o: rboxSysEmu.cpp rboxSysEmu.h
	$(GCC) -c $(CFLAGS) rboxSysEmu.cpp -o build/rboxSysEmu.o

build/sys_emu.o: sys_emu.cpp
	$(GCC) -c $(CFLAGS) sys_emu.cpp -o build/sys_emu.o

build/trace.o: $(SGPU)/src/trace.cpp
	$(GCC) -c $(CFLAGS) $(SGPU)/src/trace.cpp -o build/trace.o

build:
	mkdir build

libchecker:
	make -C $(BEMU)/checker/

clean:
	make -C $(BEMU)/checker/ clean
	rm -rf build

