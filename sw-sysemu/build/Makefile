ifndef BEMU
    $(error "Please execute 'source ./env.sh' or 'source ./env_csh.sh' in the repository root directory before calling make")
endif
ifndef TESTROOT
    $(error "Please execute 'source ./env.sh' or 'source ./env_csh.sh' in the repository root directory before calling make")
endif

SRCS := $(BEMU)/emu.cpp \
        $(BEMU)/fpu/fpu.cpp \
        $(BEMU)/fpu/cvt.cpp \
        $(BEMU)/txs.cpp \
        $(BEMU)/fpu/ttrans.cpp \
        $(BEMU)/tbox_emu.cpp \
        $(BEMU)/log.cpp \
        $(BEMU)/emu_gio.cpp \
        $(BEMU)/insn_dec_func.cpp \
        $(BEMU)/insn_exec_func.cpp

include softfloat.mk
SRCS += $(softfloat_c_srcs)

vpath %.cpp $(BEMU)
vpath %.cpp $(BEMU)/fpu
vpath %.c   $(BEMU)/softfloat

OBJS := $(addsuffix .o,$(basename $(notdir $(SRCS))))
DEPS := $(addsuffix .d,$(basename $(OBJS)))

AR       := ar
CC       := gcc
CXX      := g++
ARFLAGS  := rvs
CPPFLAGS := -I$(BEMU) -I$(TESTROOT)/libs
CXXFLAGS := -MMD -MP -Wall -Werror -g -O2 -std=c++11
CFLAGS   := -MMD -MP -Wall -Werror -g -O2 -std=c11
LDLIBS   :=
LDFLAGS  := -shared

ifndef LIBEMU_STATIC
CXXFLAGS += -fPIC
CFLAGS   += -fPIC
endif

# FIXME: Remove the following when we convert everything away from C++ floats
CXXFLAGS += -mf16c -fno-fast-math

.PHONY: all
ifdef LIBEMU_STATIC
all: libemu.a
else
all: libemu.so
endif

.PHONY: clean
clean:
	$(RM) libemu.{a,so} *.o *.d

libemu.so: $(OBJS)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

libemu.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

-include $(DEPS)
