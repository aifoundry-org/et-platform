ifndef BEMU
    $(error "Please execute 'source ./env.sh' or 'source ./env_csh.sh' in the repository root directory before calling make")
endif
ifndef TESTROOT
    $(error "Please execute 'source ./env.sh' or 'source ./env_csh.sh' in the repository root directory before calling make")
endif

SRCS := $(BEMU)/checker/checker.cpp \
        $(BEMU)/common/riscv_disasm.cpp \
        $(BEMU)/common/main_memory.cpp \
        $(BEMU)/common/main_memory_region.cpp \
        $(BEMU)/common/main_memory_region_atomic.cpp \
        $(BEMU)/common/main_memory_region_tbox.cpp \
        $(BEMU)/common/main_memory_region_rbox.cpp

vpath %.cpp $(BEMU)/checker
vpath %.cpp $(BEMU)/common

OBJS := $(addsuffix .o,$(basename $(notdir $(SRCS))))
DEPS := $(addsuffix .d,$(basename $(OBJS)))

BFDROOT := $(RISCV)/x86_64-pc-linux-gnu/riscv64-unknown-elf

CC	 := gcc
CXX 	 := g++
CPPFLAGS := -I$(BEMU) -I$(BEMU)/checker -I$(TESTROOT)/libs
CXXFLAGS := -MMD -MP -Wall -Werror -fPIC -std=c++11 -g -O2
LDFLAGS  := -shared
LDLIBS   := -L$(BEMU)/build -Wl,-rpath=$(BEMU)/build -lemu

CPPFLAGS += -I$(BFDROOT)/include
LDLIBS   += -L$(BFDROOT)/lib -Wl,-rpath=$(BFDROOT)/lib \
	    -lbfd -lopcodes

NO_EMU_DEBUG ?= 0
ifeq ($(NO_EMU_DEBUG), 0)
    CPPFLAGS += -DEMU_DEBUG
endif

.PHONY: all
all: libchecker.so libchecker.a

.PHONY: libemu_all
libemu_all:
	$(MAKE) -C $(BEMU)/build all

.PHONY: libemu_clean
libemu_clean:
	$(MAKE) -C $(BEMU)/build clean

.PHONY: clean
clean: libemu_clean
	$(RM) *.so *.a *.o *.d

libchecker.so: $(OBJS) | libemu_all
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

libchecker.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $^ 

-include $(DEPS)
