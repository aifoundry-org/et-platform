ifndef BEMU
    $(error "Please execute 'source ./env.sh' or 'source ./env_csh.sh' in the repository root directory before calling make")
endif
ifndef TESTROOT
    $(error "Please execute 'source ./env.sh' or 'source ./env_csh.sh' in the repository root directory before calling make")
endif

# Always compile with softfloat
HAVE_SOFTFLOAT := 1

# Use "real" TXFMA; this is incompatible (will be ignored) with HAVE_SOFTFLOAT
USE_REAL_TXFMA := 0

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

ifeq ($(HAVE_SOFTFLOAT), 1)
    include softfloat.mk
    SRCS += $(softfloat_c_srcs)

    vpath %.c $(BEMU)/softfloat
endif

vpath %.cpp $(BEMU)
vpath %.cc  $(TESTROOT)/libs

OBJS := $(addsuffix .o,$(basename $(notdir $(SRCS))))
DEPS := $(addsuffix .d,$(basename $(OBJS)))

CC       := gcc
CXX 	 := g++
CPPFLAGS := -I$(BEMU) -I$(TESTROOT)/libs
CXXFLAGS := -MMD -MP -Wall -Werror -fPIC -g -O2 -std=c++11
CFLAGS   := -MMD -MP -Wall -Werror -fPIC -g -O2
LDLIBS   :=
LDFLAGS  := -shared

ifeq ($(HAVE_SOFTFLOAT), 1)
    CPPFLAGS += -DHAVE_SOFTFLOAT=1
    # FIXME: Remove the following when we convert everything away from C++ floats
    CFLAGS   += -mf16c -fno-fast-math
    CXXFLAGS += -mf16c -fno-fast-math
else
    CFLAGS   += -mf16c -fno-fast-math
    CXXFLAGS += -mf16c -fno-fast-math
endif

ifeq ($(USE_REAL_TXFMA), 1)
    CPPFLAGS += -DUSE_REAL_TXFMA
endif

.PHONY: all
all: libemu.so

.PHONY: clean
clean:
	$(RM) libemu.so $(OBJS) $(DEPS)

testLog.o: $(TESTROOT)/libs/testLog.cc
	$(COMPILE.cc) -DCHECKER $(OUTPUT_OPTION) $<

libemu.so: $(OBJS)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

-include $(DEPS)
