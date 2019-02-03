include emu.mk
include fpu.mk
include softfloat.mk

libemu_SRCS := $(emu_cpp_srcs)

libfpu_SRCS := $(fpu_cpp_srcs) \
	       $(softfloat_c_srcs)

AR       := ar
CC       := gcc
CXX      := g++
ARFLAGS  := rvs
common_CPPFLAGS := -I..
CXXFLAGS := -MMD -MP -Wall -Werror -fPIC -g -O2 -std=c++11
CFLAGS   := -MMD -MP -Wall -Werror -fPIC -g -O2 -std=c11
LDFLAGS  := -shared

# FIXME: Remove the following when we convert everything away from C++ floats
CXXFLAGS += -mf16c -fno-fast-math

.PHONY: all
all: libfpu libemu

.PHONY: clean
clean: clean_libfpu clean_libemu

# ---------- libfpu -----------------------------------------------------------

libfpu_OBJS := $(addprefix libfpu/,$(addsuffix .o,$(basename $(libfpu_SRCS))))
libfpu_DEPS := $(addsuffix .d,$(basename $(libfpu_OBJS)))

libfpu_DIRS := $(sort $(dir $(libfpu_OBJS)))

.PHONY: libfpu
libfpu: libfpu.so libfpu.a

.PHONY: clean_libfpu
clean_libfpu:
	$(RM) -r libfpu.* libfpu/

libfpu.so libfpu.a: CPPFLAGS := $(common_CPPFLAGS)
libfpu.so libfpu.a: LDLIBS :=

libfpu.so: $(libfpu_OBJS)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

libfpu.a: $(libfpu_OBJS)
	$(AR) $(ARFLAGS) $@ $^ 

$(libfpu_DIRS):
	mkdir -p $@

libfpu/%.o: ../%.cpp | $(libfpu_DIRS)
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<

libfpu/%.o: ../%.c | $(libfpu_DIRS)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(libfpu_OBJS): Makefile

-include $(libfpu_DEPS)

# ---------- libemu -----------------------------------------------------------

libemu_OBJS := $(addprefix libemu/,$(addsuffix .o,$(basename $(libemu_SRCS))))
libemu_DEPS := $(addsuffix .d,$(basename $(libemu_OBJS)))
libemu_DIRS := $(sort $(dir $(libemu_OBJS)))

.PHONY: libemu
libemu: libemu.so libemu.a

.PHONY: clean_libemu
clean_libemu:
	$(RM) -r libemu.* libemu/

libemu.so libemu.a: CPPFLAGS := $(common_CPPFLAGS) -I$(TESTROOT)/libs -I$(TESTROOT)/shire/shiretop/csrc
libemu.so libemu.a: LDLIBS := -L. -Wl,-rpath=$(shell readlink -f .) -lfpu

libemu.so: $(libemu_OBJS) libfpu.so
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $(libemu_OBJS) $(LOADLIBES) $(LDLIBS) -o $@

libemu.a: $(libemu_OBJS) libfpu.a
	$(AR) $(ARFLAGS) $@ $(OBJS) 

$(libemu_DIRS):
	mkdir -p $@

libemu/%.o: ../%.cpp | $(libemu_DIRS)
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<

vlog_define:
	$(TESTROOT)/shire/shiretop/scripts/gen_defines.sh

$(libemu_OBJS): Makefile vlog_define

-include $(libemu_DEPS)
