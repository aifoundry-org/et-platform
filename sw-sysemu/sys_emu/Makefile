target: build libchecker build/sys_emu

CXX=g++
CXXFLAGS=-g -O3 -DCHECKER -I../checker -I.. -I$(TESTROOT)/libs -I$(SGPU)/include -I -DEMU_DEBUG -DSYS_EMU $(HASH_FLAGS)
HASH_FLAGS=-std=c++11
LIBS=../checker/build/libchecker.a
LIBS_LINK=-ldl -lboost_regex
OBJS=build/testLog.o build/net_emulator.o build/rboxSysEmu.o build/trace.o

build/sys_emu: $(OBJS) $(LIBS) sys_emu.cpp
	$(CXX) $(CXXFLAGS) $(LIBS_LINK) $(OBJS) sys_emu.cpp -o build/sys_emu $(LIBS)

build/testLog.o: $(TESTROOT)/libs/testLog.cc $(TESTROOT)/libs/testLog.h
	$(CXX) -c $(CXXFLAGS) $(TESTROOT)/libs/testLog.cc -o build/testLog.o

build/net_emulator.o: net_emulator.cpp net_emulator.h
	$(CXX) -c $(CXXFLAGS) net_emulator.cpp -o build/net_emulator.o

build/rboxSysEmu.o: rboxSysEmu.cpp rboxSysEmu.h
	$(CXX) -c $(CXXFLAGS) rboxSysEmu.cpp -o build/rboxSysEmu.o

build/sys_emu.o: sys_emu.cpp
	$(CXX) -c $(CXXFLAGS) sys_emu.cpp -o build/sys_emu.o

build/trace.o: $(SGPU)/src/trace.cpp
	$(CXX) -c $(CXXFLAGS) $(SGPU)/src/trace.cpp -o build/trace.o

build:
	mkdir build

libchecker:
	make -C $(BEMU)/checker/

clean:
	make -C $(BEMU)/checker/ clean
	rm -rf build

