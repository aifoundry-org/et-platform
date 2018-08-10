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
	$(SGPU)/src/trace.cpp

vpath %.cpp $(BEMU)/sys_emu
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

.PHONY: libemu
libemu:
	$(MAKE) -C $(BEMU)/build all

.PHONY: clean
clean:
	$(RM) sys_emu $(OBJS) $(DEPS)
	$(MAKE) -C $(BEMU)/build clean

trace.o: $(SGPU)/src/trace.cpp
	$(COMPILE.cpp) -DSYS_EMU $(OUTPUT_OPTION) $<

sys_emu: $(OBJS) | libemu
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

-include $(DEPS)
