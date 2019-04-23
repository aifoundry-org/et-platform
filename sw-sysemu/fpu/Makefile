include fpu.mk
include ../softfloat/softfloat.mk

libfpu_OBJDIR ?= build

libfpu_SRCS := $(fpu_cpp_srcs) $(softfloat_c_srcs)
libfpu_OBJS := $(patsubst %,$(libfpu_OBJDIR)/obj/%.o,$(basename $(libfpu_SRCS)))
libfpu_DEPS := $(libfpu_OBJS:.o=.d)


AR       := ar
CC       := gcc
CXX      := g++
ARFLAGS  := rvs
CPPFLAGS := -MMD -MP -I..
CFLAGS   := -Wall -Werror -fPIC -g -O2 -std=c11
CXXFLAGS := -Wall -Werror -Wextra -pedantic-errors -fPIC -g -O2 -std=c++11
LDFLAGS  := -shared

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

$(libfpu_OBJDIR)/obj/%.o: ../%.cpp | $$(@D)/.
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<

$(libfpu_OBJDIR)/obj/%.o: ../%.c | $$(@D)/.
	$(COMPILE.c) $(OUTPUT_OPTION) $<
