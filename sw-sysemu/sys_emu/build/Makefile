ifndef BEMU
    $(error "Please execute 'source ./env.sh' or 'source ./env_csh.sh' in the repository root directory before calling make")
endif
ifndef SGPU
    $(error "Please execute 'source ./env.sh' or 'source ./env_csh.sh' in the repository root directory before calling make")
endif
ifndef TESTROOT
    $(error "Please execute 'source ./env.sh' or 'source ./env_csh.sh' in the repository root directory before calling make")
endif

SRCS := $(BEMU)/sys_emu/net_emulator.cpp \
	$(BEMU)/sys_emu/rboxSysEmu.cpp \
	$(BEMU)/sys_emu/sys_emu.cpp \
        $(BEMU)/common/main_memory.cpp \
        $(BEMU)/common/main_memory_region.cpp \
        $(BEMU)/common/main_memory_region_atomic.cpp \
        $(BEMU)/common/main_memory_region_tbox.cpp \
        $(BEMU)/common/main_memory_region_rbox.cpp \
	$(SGPU)/src/trace.cpp

vpath %.cpp $(BEMU)/sys_emu
vpath %.cpp $(BEMU)/common
vpath %.cpp $(SGPU)/src

OBJS := $(addsuffix .o,$(basename $(notdir $(SRCS))))
DEPS := $(addsuffix .d,$(basename $(OBJS)))

CC	 := gcc
CXX 	 := g++
CPPFLAGS := -I$(BEMU) -I$(TESTROOT)/libs -I$(SGPU)/include
CXXFLAGS := -MMD -Wall -Werror -std=c++11 -g -O2
LDLIBS   := -lboost_regex -L$(BEMU)/build -Wl,-rpath=$(BEMU)/build -lemu

.PHONY: all
all: sys_emu

.PHONY: libemu_all
libemu_all:
	$(MAKE) -C $(BEMU)/build all

.PHONY: libemu_clean
libemu_clean:
	$(MAKE) -C $(BEMU)/build clean

.PHONY: clean
clean: libemu_clean
	$(RM) sys_emu *.o *.d

trace.o: $(SGPU)/src/trace.cpp
	$(COMPILE.cpp) -DSYS_EMU $(OUTPUT_OPTION) $<

sys_emu: $(OBJS) | libemu_all
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

-include $(DEPS)
