target: libchecker build/sys_emu

CXX=g++
CXXFLAGS=-g -O3 -DCHECKER -I../checker -I.. -I$(TESTROOT)/libs -I$(SGPU)/include -I -DEMU_DEBUG -DSYS_EMU $(HASH_FLAGS)
HASH_FLAGS=-std=c++11
LIBS_DIR=$(shell pwd)/../checker/build
LIBS_DEP=$(LIBS_DIR)/libchecker.so
LIBS_LINK=-ldl -lboost_regex -L$(LIBS_DIR) -Wl,-rpath=$(LIBS_DIR) -lchecker

OBJS=build/testLog.o build/net_emulator.o build/rboxSysEmu.o build/trace.o build/sys_emu.o

build/sys_emu: $(OBJS) | $(LIBS_DEP) build
	$(CXX) -o $@ $(CXXFLAGS) $^ $(LIBS_LINK)

build/testLog.o: $(TESTROOT)/libs/testLog.cc $(TESTROOT)/libs/testLog.h | build
	$(CXX) -c $(CXXFLAGS) $< -o $@

build/net_emulator.o: net_emulator.cpp net_emulator.h | build
	$(CXX) -c $(CXXFLAGS) $< -o $@

build/rboxSysEmu.o: rboxSysEmu.cpp rboxSysEmu.h | build
	$(CXX) -c $(CXXFLAGS) $< -o $@

build/sys_emu.o: sys_emu.cpp | build
	$(CXX) -c $(CXXFLAGS) $< -o $@

build/trace.o: $(SGPU)/src/trace.cpp | build
	$(CXX) -c $(CXXFLAGS) $< -o $@

build:
	mkdir build

libchecker:
	make -C $(BEMU)/checker/

clean:
	make -C $(BEMU)/checker/ clean
	rm -rf build

