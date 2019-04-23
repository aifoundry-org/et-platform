include ../emu.mk
include sysemu.mk

libfpu_OBJDIR ?= ../fpu/build
sysemu_OBJDIR ?= build

sysemu_SRCS := $(sysemu_cpp_srcs) $(emu_cpp_srcs)
sysemu_OBJS := $(patsubst %,$(sysemu_OBJDIR)/obj/%.o,$(basename $(sysemu_SRCS)))
sysemu_DEPS := $(sysemu_OBJS:.o=.d)

sysemu_artifact_HDRS := $(addprefix sysemu-artifacts/include/,$(sysemu_hdrs))
sysemu_artifact_TGTS := \
    sysemu-artifacts/libfpu.a \
    sysemu-artifacts/libsysemu.a \
    sysemu-artifacts/sys_emu

# Remove the file that implements the main function
libsysemu_OBJS := $(filter-out $(sysemu_OBJDIR)/sys_emu/sys_emu_main.o,$(sysemu_OBJS))

CC       := gcc
CXX      := g++
CPPFLAGS := -MMD -MP -I.. -I. -DSYS_EMU
CXXFLAGS := -Wall -Wextra -Werror -pedantic-errors -fPIC -g -O2 -std=c++11
CFLAGS   := -Wall -Wextra -Werror -pedantic-errors -fPIC -g -O2 -std=c11
LDLIBS   := -lm

ifeq ($(PROFILING), 1)
  CPPFLAGS += -DBEMU_PROFILING -DSYSEMU_PROFILING
endif


# ----- phony targets ---------------------------------------------------------

.PHONY: all
all: $(sysemu_OBJDIR)/sys_emu $(sysemu_OBJDIR)/libsysemu.a

.PHONY: sysemu
sysemu: $(sysemu_OBJDIR)/sys_emu

.PHONY: libsysemu
libsysemu: $(sysemu_OBJDIR)/libsysemu.a

.PHONY: static
static: LDFLAGS := -static
static: $(sysemu_OBJDIR)/sys_emu
	strip $<

.PHONY: debug
debug: CPPFLAGS += -DSYSEMU_DEBUG
debug: $(sysemu_OBJDIR)/sys_emu

.PHONY: clean
clean:
	$(RM) -r $(sysemu_OBJDIR) sysemu-artifacts*

.PHONY: distclean
distclean: clean
	$(MAKE) -C ../fpu libfpu_OBJDIR=$(abspath $(libfpu_OBJDIR)) clean

.PHONY: force
force:

# ----- rules for building targets --------------------------------------------

$(sysemu_OBJDIR)/sys_emu: $(sysemu_OBJS) $(libfpu_OBJDIR)/libfpu.a
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(sysemu_OBJDIR)/libsysemu.a: $(libsysemu_OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(libfpu_OBJDIR)/libfpu.a: force
	$(MAKE) -C ../fpu libfpu_OBJDIR=$(abspath $(libfpu_OBJDIR)) all

sysemu-artifacts.tar.gz: $(sysemu_artifact_HDRS) $(sysemu_artifact_TGTS)
	tar zcvf sysemu-artifacts.tar.gz sysemu-artifacts

# ----- dependencies ----------------------------------------------------------

$(sysemu_OBJS): $(MAKEFILE_LIST)

-include $(sysemu_DEPS)

# ----- rules for creating directories ----------------------------------------

.PRECIOUS: $(sysemu_OBJDIR)/.
$(sysemu_OBJDIR)/.:
	  mkdir -p $@

.PRECIOUS: $(sysemu_OBJDIR)%/.
$(sysemu_OBJDIR)%/.:
	  mkdir -p $@

sysemu-artifacts/.:
	mkdir -p $@

sysemu-artifacts%/.:
	mkdir -p $@

# ----- rules for generating files --------------------------------------------

.SECONDEXPANSION:

$(sysemu_OBJDIR)/obj/%.o: ../%.cpp | $$(@D)/.
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<

sysemu-artifacts/include/%.h: ../%.h | $$(@D)/.
	cp -f $< $@

sysemu-artifacts/libfpu.a: $(libfpu_OBJDIR)/libfpu.a | $$(@D)/.
	cp -f $< $@

sysemu-artifacts/libsysemu.a: $(sysemu_OBJDIR)/libsysemu.a | $$(@D)/.
	cp -f $< $@

sysemu-artifacts/sys_emu: $(sysemu_OBJDIR)/sys_emu | $$(@D)/.
	cp -f $< $@
