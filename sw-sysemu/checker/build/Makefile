ifndef TESTROOT
    $(error "Please execute 'source ./env.sh' or 'source ./env_csh.sh' in the repository root directory before calling make")
endif

include ../../build/emu.mk

checker_hdrs := \
	checker/checker.h \
	common/main_memory.h \
	common/main_memory_region.h \
	common/main_memory_region_atomic.h \
	common/main_memory_region_esr.h \
	common/main_memory_region_rbox.h \
	common/riscv_disasm.h

checker_cpp_srcs := \
	checker/checker.cpp \
        common/main_memory.cpp \
        common/main_memory_region.cpp \
        common/main_memory_region_atomic.cpp \
        common/main_memory_region_esr.cpp \
        common/main_memory_region_rbox.cpp \
        common/riscv_disasm.cpp

checker_SRCS := $(checker_cpp_srcs) \
		$(emu_cpp_srcs)

checker_OBJS := $(addprefix obj/,$(addsuffix .o,$(basename $(checker_SRCS))))
checker_DEPS := $(addsuffix .d,$(basename $(checker_OBJS)))
checker_DIRS := $(sort $(dir $(checker_OBJS)))

BFDROOT := $(RISCV)/x86_64-pc-linux-gnu/riscv64-unknown-elf

CC	 := gcc
CXX 	 := g++
CPPFLAGS := -I../.. -I.. -I$(TESTROOT)/libs
CXXFLAGS := -MMD -MP -Wall -Werror -fPIC -g -O2 -std=c++11
CFLAGS   := -MMD -MP -Wall -Werror -fPIC -g -O2 -std=c11
LDLIBS   := -L../../build -Wl,-rpath=$(shell readlink -f ../../build) -lfpu
LDFLAGS  := -shared

CPPFLAGS += -I$(BFDROOT)/include
LDLIBS   += -L$(BFDROOT)/lib -Wl,-rpath=$(BFDROOT)/lib -lbfd -lopcodes

.PHONY: all
all: libchecker.so libchecker.a

.PHONY: clean
clean:
	$(MAKE) -C ../../build clean_libfpu
	$(RM) -r libchecker.* obj/

../../build/libfpu.so ../../build/libfpu.a:
	$(MAKE) -C ../../build $(notdir $@)

libchecker.so: $(checker_OBJS) ../../build/libfpu.so
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) $(checker_OBJS) $(LOADLIBES) $(LDLIBS) -o $@

libchecker.a: $(checker_OBJS) ../../build/libfpu.a
	$(AR) $(ARFLAGS) $@ $(checker_OBJS) 

$(checker_DIRS):
	mkdir -p $@

obj/%.o: ../../%.cpp | $(checker_DIRS)
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<

obj/%.o: ../../%.c | $(checker_DIRS)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(checker_OBJS): Makefile

-include $(checker_DEPS)
