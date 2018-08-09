ifndef BEMU
    $(error "Please execute 'source ./env.sh' or 'source ./env_csh.sh' in the repository root directory before calling make")
endif
ifndef TESTROOT
    $(error "Please execute 'source ./env.sh' or 'source ./env_csh.sh' in the repository root directory before calling make")
endif

SRCS := $(BEMU)/emu.cpp \
        $(BEMU)/log.cpp \
        $(BEMU)/cvt.cpp \
        $(BEMU)/txs.cpp \
        $(BEMU)/ttrans.cpp \
        $(BEMU)/tbox_emu.cpp \
        $(BEMU)/ipc.cpp \
        $(BEMU)/emu_gio.cpp \
        $(BEMU)/instruction_cache.cpp \
        $(BEMU)/instruction.cpp \
        $(BEMU)/main_memory.cpp \
        $(BEMU)/main_memory_region.cpp \
        $(BEMU)/main_memory_region_atomic.cpp \
        $(BEMU)/main_memory_region_tbox.cpp \
        $(BEMU)/main_memory_region_rbox.cpp \
        $(TESTROOT)/libs/testLog.cc

vpath %.cpp $(BEMU)
vpath %.cc  $(TESTROOT)/libs

OBJS := $(addsuffix .o,$(basename $(notdir $(SRCS))))
DEPS := $(addsuffix .d,$(basename $(OBJS)))

CC       := gcc
CXX 	 := g++
CPPFLAGS := -I$(BEMU) -I$(TESTROOT)/libs -DCHECKER
CXXFLAGS := -MMD -Wall -Werror -fPIC -std=c++11
ifeq ($(SOFTFLOAT), 1)
    CXXFLAGS += -g -O2
else
    CXXFLAGS += -g -O2 -mf16c -fno-fast-math
endif
LDFLAGS := -shared

.PHONY: all
all: libemu.so

.PHONY: clean
clean:
	$(RM) libemu.so $(OBJS) $(DEPS)

libemu.so: $(OBJS)
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

-include $(DEPS)
