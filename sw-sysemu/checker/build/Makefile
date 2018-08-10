ifndef BEMU
    $(error "Please execute 'source ./env.sh' or 'source ./env_csh.sh' in the repository root directory before calling make")
endif
ifndef TESTROOT
    $(error "Please execute 'source ./env.sh' or 'source ./env_csh.sh' in the repository root directory before calling make")
endif

SRCS := checker.cpp

vpath %.cpp $(BEMU)/checker

OBJS := $(addsuffix .o,$(basename $(notdir $(SRCS))))
DEPS := $(addsuffix .d,$(basename $(OBJS)))

CC	 := gcc
CXX 	 := g++
CPPFLAGS := -I$(BEMU) -I$(BEMU)/checker -I$(TESTROOT)/libs -DCHECKER
CXXFLAGS := -MMD -Wall -Werror -fPIC -std=c++11 -g -O2
LDFLAGS  := -shared
LDLIBS   := -L$(BEMU)/build -Wl,-rpath=$(BEMU)/build -lemu

NO_EMU_DEBUG ?= 0
ifeq ($(NO_EMU_DEBUG), 0)
    CPPFLAGS += -DEMU_DEBUG
endif

.PHONY: all
all: libchecker.so

.PHONY: libemu
libemu:
	$(MAKE) -C $(BEMU)/build all

.PHONY: clean
clean:
	$(RM) libchecker.so $(OBJS) $(DEPS)
	$(MAKE) -C $(BEMU)/build clean

libchecker.so: $(OBJS) | libemu
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

-include $(DEPS)
