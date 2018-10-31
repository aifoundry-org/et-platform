ifndef BEMU
    $(error "Please execute 'source ./env.sh' or 'source ./env_csh.sh' in the repository root directory before calling make")
endif
ifndef TESTROOT
    $(error "Please execute 'source ./env.sh' or 'source ./env_csh.sh' in the repository root directory before calling make")
endif

SRCS := $(BEMU)/emu.cpp \
        $(BEMU)/fpu.cpp \
        $(BEMU)/cvt.cpp \
        $(BEMU)/txs.cpp \
        $(BEMU)/ttrans.cpp \
        $(BEMU)/tbox_emu.cpp \
        $(BEMU)/log.cpp \
        $(BEMU)/ipc.cpp \
        $(BEMU)/emu_gio.cpp \
        $(BEMU)/instruction_cache.cpp \
        $(BEMU)/instruction.cpp

include softfloat.mk
SRCS += $(softfloat_c_srcs)

vpath %.cpp $(BEMU)
vpath %.cc  $(TESTROOT)/libs
vpath %.c   $(BEMU)/softfloat

OBJS := $(addsuffix .o,$(basename $(notdir $(SRCS))))
DEPS := $(addsuffix .d,$(basename $(OBJS)))

CC       := gcc
CXX 	 := g++
CPPFLAGS := -I$(BEMU) -I$(TESTROOT)/libs
CXXFLAGS := -MMD -MP -Wall -Werror -fPIC -g -O2 -std=c++11
CFLAGS   := -MMD -MP -Wall -Werror -fPIC -g -O2 -std=c11
LDLIBS   :=
LDFLAGS  := -shared

# FIXME: Remove the following when we convert everything away from C++ floats
CXXFLAGS += -mf16c -fno-fast-math


.PHONY: all
all: libemu.so

.PHONY: clean
clean:
	$(RM) libemu.so *.o *.d

testLog.o: $(TESTROOT)/libs/testLog.cc
	$(COMPILE.cc) -DCHECKER $(OUTPUT_OPTION) $<

libemu.so: $(OBJS)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

-include $(DEPS)
