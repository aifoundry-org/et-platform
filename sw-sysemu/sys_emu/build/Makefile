include ../../build/emu.mk

sysemu_hdrs := \
	common/main_memory.h \
	common/main_memory_region.h \
	common/main_memory_region_atomic.h \
	common/main_memory_region_esr.h \
	common/main_memory_region_rbox.h \
	sys_emu/api_communicate.h \
	sys_emu/et_emulator.h \
	sys_emu/testLog.h

sysemu_cpp_srcs := \
	common/main_memory.cpp \
	common/main_memory_region.cpp \
	common/main_memory_region_atomic.cpp \
	common/main_memory_region_esr.cpp \
	common/main_memory_region_rbox.cpp \
	sys_emu/api_communicate.cpp \
	sys_emu/net_emulator.cpp \
	sys_emu/sys_emu.cpp \
	sys_emu/sys_emu_log.cpp

sysemu_SRCS := $(sysemu_cpp_srcs) \
	       $(emu_cpp_srcs)

sysemu_OBJS := $(addprefix obj/,$(addsuffix .o,$(basename $(sysemu_SRCS))))
sysemu_DEPS := $(addsuffix .d,$(basename $(sysemu_OBJS)))
sysemu_DIRS := $(sort $(dir $(sysemu_OBJS)))

CC	 := gcc
CXX 	 := g++
CPPFLAGS := -I../.. -I.. -DSYS_EMU
CXXFLAGS := -MMD -MP -Wall -Werror -fPIC -g -O2 -std=c++11
CFLAGS   := -MMD -MP -Wall -Werror -fPIC -g -O2 -std=c11
LDLIBS   := ../../build/libfpu.a -lm

# FIXME: Remove the following when we convert everything away from C++ floats
CXXFLAGS += -mf16c -fno-fast-math

.PHONY: all
all: sys_emu

.PHONY: static
static: LDFLAGS := -static
static: sys_emu
	strip $<

.PHONY: debug
debug: CPPFLAGS += -DSYSEMU_DEBUG
debug: sys_emu

.PHONY: clean
clean:
	$(MAKE) -C ../../build clean_libfpu
	$(RM) -r sys_emu obj/

../../build/libfpu.a:
	$(MAKE) -C ../../build $(notdir $@)

sys_emu: $(sysemu_OBJS) ../../build/libfpu.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(TARGET_ARCH) $(sysemu_OBJS) $(LOADLIBES) $(LDLIBS) -o $@

$(sysemu_DIRS):
	mkdir -p $@

obj/%.o: ../../%.cpp | $(sysemu_DIRS)
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<

$(sysemu_OBJS): Makefile

-include $(sysemu_DEPS)
