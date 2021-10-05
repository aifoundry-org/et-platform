# -------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
# -------------------------------------------------------------------------

include fpu.mk
include ../softfloat/softfloat.mk

libfpu_OBJDIR ?= build
DEBUG ?= 0
GPROF ?= 0

libfpu_SRCS := $(fpu_cpp_srcs) $(fpu_c_srcs) $(softfloat_c_srcs)
libfpu_OBJS := $(patsubst %,$(libfpu_OBJDIR)/fpu_obj/%.o,$(basename $(libfpu_SRCS)))
libfpu_DEPS := $(libfpu_OBJS:.o=.d)

EXTRA_MAKE_OPTS ?= 

ARFLAGS  := rvs
CPPFLAGS := -MMD -MP -I..
CFLAGS   := -Wall -Werror -fPIC -std=c11
CXXFLAGS := -Wall -Werror -Wextra -pedantic-errors -fPIC -std=c++11 $(EXTRA_MAKE_OPTS)
LDFLAGS  := -shared
DEBUG ?= 0

ifeq ($(DEBUG),0)
    CXXFLAGS += -g -O2
    CFLAGS += -g -O2
else
    CPPFLAGS += -D_GLIBCXX_DEBUG
    CXXFLAGS += -g3 -Og
    CFLAGS += -g3 -Og
endif

ifneq ($(GPROF),0)
  CXXFLAGS += -pg
  CFLAGS   += -pg
  LDFLAGS  += -pg
endif

# FIXME: Remove the following when we convert everything away from C++ floats
CXXFLAGS += -mf16c -fno-fast-math


# ----- phony targets ---------------------------------------------------------

.PHONY: all
all: $(libfpu_OBJDIR)/libfpu.so $(libfpu_OBJDIR)/libfpu.a

.PHONY: clean
clean:
	$(RM) -r $(libfpu_OBJDIR)

.PHONY: distclean
distclean: clean

# ----- rules for building targets --------------------------------------------

$(libfpu_OBJDIR)/libfpu.so: $(libfpu_OBJS)
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(libfpu_OBJDIR)/libfpu.a: $(libfpu_OBJS)
	$(AR) $(ARFLAGS) $@ $^ 

# ----- dependencies ----------------------------------------------------------

$(libfpu_OBJS): $(MAKEFILE_LIST)

-include $(libfpu_DEPS)

# ----- rules for creating directories ----------------------------------------

.PRECIOUS: $(libfpu_OBJDIR)/.
$(libfpu_OBJDIR)/.:
	  mkdir -p $@

.PRECIOUS: $(libfpu_OBJDIR)%/.
$(libfpu_OBJDIR)%/.:
	  mkdir -p $@

# ----- rules for compiling objects -------------------------------------------

.SECONDEXPANSION:

$(libfpu_OBJDIR)/fpu_obj/%.o: ../%.cpp | $$(@D)/.
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<

$(libfpu_OBJDIR)/fpu_obj/%.o: ../%.c | $$(@D)/.
	$(COMPILE.c) $(OUTPUT_OPTION) $<
