BEMU_BUILD_DIR?=

include emu.mk
include fpu.mk
include softfloat.mk

emu_cpp_hdrs += sys_emu/log.h
emu_cpp_srcs += sys_emu/log.cpp

libemu_SRCS := $(emu_cpp_srcs)

libfpu_SRCS := $(fpu_cpp_srcs) \
	       $(softfloat_c_srcs)

AR       := ar
CC       := gcc
CXX      := g++
ARFLAGS  := rvs
common_CPPFLAGS := -I.. -I../sys_emu
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

libfpu_OBJS := $(addprefix $(BEMU_BUILD_DIR)libfpu/,$(addsuffix .o,$(basename $(libfpu_SRCS))))
libfpu_DEPS := $(addsuffix .d,$(basename $(libfpu_OBJS)))

libfpu_DIRS := $(sort $(dir $(libfpu_OBJS)))

.PHONY: libfpu
libfpu: $(BEMU_BUILD_DIR)libfpu.so $(BEMU_BUILD_DIR)libfpu.a

.PHONY: clean_libfpu
clean_libfpu:
	$(RM) -r $(BEMU_BUILD_DIR)libfpu.* $(BEMU_BUILD_DIR)libfpu/

$(BEMU_BUILD_DIR)libfpu.so $(BEMU_BUILD_DIR)libfpu.a: CPPFLAGS := $(common_CPPFLAGS)
$(BEMU_BUILD_DIR)libfpu.so $(BEMU_BUILD_DIR)libfpu.a: LDLIBS :=

$(BEMU_BUILD_DIR)libfpu.so: $(libfpu_OBJS)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(BEMU_BUILD_DIR)libfpu.a: $(libfpu_OBJS)
	$(AR) $(ARFLAGS) $@ $^ 

$(libfpu_DIRS):
	mkdir -p $@

$(BEMU_BUILD_DIR)libfpu/%.o: ../%.cpp | $(libfpu_DIRS)
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<

$(BEMU_BUILD_DIR)libfpu/%.o: ../%.c | $(libfpu_DIRS)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(libfpu_OBJS): Makefile

-include $(libfpu_DEPS)

# ---------- libemu -----------------------------------------------------------

libemu_OBJS := $(addprefix $(BEMU_BUILD_DIR)libemu/,$(addsuffix .o,$(basename $(libemu_SRCS))))
libemu_DEPS := $(addsuffix .d,$(basename $(libemu_OBJS)))
libemu_DIRS := $(sort $(dir $(libemu_OBJS)))

.PHONY: libemu
libemu: $(BEMU_BUILD_DIR)libemu.so $(BEMU_BUILD_DIR)libemu.a

.PHONY: clean_libemu
clean_libemu:
	$(RM) -r $(BEMU_BUILD_DIR)libemu.* $(BEMU_BUILD_DIR)libemu/

$(BEMU_BUILD_DIR)libemu.so $(BEMU_BUILD_DIR)libemu.a: CPPFLAGS := $(common_CPPFLAGS) -I$(TESTROOT)/libs -I$(TESTROOT)/shire/shiretop/csrc
$(BEMU_BUILD_DIR)libemu.so $(BEMU_BUILD_DIR)libemu.a: LDLIBS := -L. -Wl,-rpath=$(shell readlink -f .) -lfpu

$(BEMU_BUILD_DIR)libemu.so: $(libemu_OBJS) $(BEMU_BUILD_DIR)libfpu.so
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $(libemu_OBJS) $(LOADLIBES) $(LDLIBS) -o $@

$(BEMU_BUILD_DIR)libemu.a: $(libemu_OBJS) $(libfpu_OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(libemu_DIRS):
	mkdir -p $@

$(BEMU_BUILD_DIR)libemu/%.o: ../%.cpp | $(libemu_DIRS)
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<

$(libemu_OBJS): Makefile

-include $(libemu_DEPS)
