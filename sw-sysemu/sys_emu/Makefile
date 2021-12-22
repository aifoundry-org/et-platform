# -------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
# -------------------------------------------------------------------------

sysemu_OBJDIR ?= build
libfpu_OBJDIR ?= $(sysemu_OBJDIR)
DEBUG     ?= 0
GPROF     ?= 0
COVERAGE  ?= 0
PROFILING ?= 0
BACKTRACE ?= $(DEBUG)
SMB_SIZE  ?= 0

include ../emu.mk
include sysemu.mk

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

CPPFLAGS := -MMD -MP -I.. -I. -DSYS_EMU
CXXFLAGS := -Wall -Wextra -Werror -pedantic-errors -fPIC -std=c++11
CFLAGS   := -Wall -Wextra -Werror -pedantic-errors -fPIC -std=c11
LDLIBS   := -lm

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

ifneq ($(BACKTRACE),0)
  CPPFLAGS += -DHAVE_BACKTRACE
  LDLIBS += -lunwind
endif

ifneq ($(COVERAGE),0)
  CXXFLAGS += --coverage
  CFLAGS   += --coverage
  LDFLAGS  += --coverage
endif

ifneq ($(PROFILING),0)
  CPPFLAGS += -DBEMU_PROFILING -DSYSEMU_PROFILING
endif

ifneq ($(SMB_SIZE),0)
  ifdef SMB_ADDR
    $(sysemu_OBJS): CPPFLAGS += -DSMB_SIZE=$(SMB_SIZE) -DSMB_ADDR=$(SMB_ADDR)
  else
    $(error Must define SMB_ADDR when SMB_SIZE is not 0)
  endif
else
  ifdef SMB_ADDR
    $(warning Ignoring SMB_ADDR because SMB_SIZE is 0)
  endif
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
	$(MAKE) -C ../fpu libfpu_OBJDIR=$(abspath $(libfpu_OBJDIR)) DEBUG=$(DEBUG) GPROF=$(GPROF) $(abspath $(libfpu_OBJDIR))/libfpu.a

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

# ----- rules for generating coverage report ----------------------------------

coverage:
	@echo "Make sure that you passed COVERAGE=1 when compiling"
	@if [[ $$(which lcov) ]]; then \
	  cd ..; \
	  lcov --no-external -c --directory . --output-file sysemu.info; \
	  genhtml sysemu.info --output-directory sysemu-coverage-full; \
	  lcov -o sysemu-insns-regs.info -e sysemu.info \
            '*bemu/insns/arith.cpp' \
            '*bemu/insns/arith_atomic.cpp' \
            '*bemu/insns/arith_loadstore.cpp' \
            '*bemu/insns/branch.cpp' \
            '*bemu/insns/coherent_arith_loadstore.cpp' \
            '*bemu/insns/coherent_packed_loadstore.cpp' \
            '*bemu/insns/float.cpp' \
            '*bemu/insns/float_loadstore.cpp' \
            '*bemu/insns/muldiv.cpp' \
            '*bemu/insns/packed_arith.cpp' \
            '*bemu/insns/packed_atomic.cpp' \
            '*bemu/insns/packed_float.cpp' \
            '*bemu/insns/packed_loadstore.cpp' \
            '*bemu/insns/packed_mask.cpp' \
            '*bemu/insns/packed_trans.cpp' \
            '*bemu/insns/zifencei.cpp' \
            '*bemu/emu.cpp'; \
	  genhtml sysemu-insns-regs.info --output-directory sysemu-coverage-insns-regs; \
	  ./sys_emu/augment-lcov-report.py > sysemu-csr-coverage.txt; \
	  mkdir -p ~/public_html; \
	  tar cf - sysemu-coverage-insns-regs | (cd ~/public_html && tar xf - && chmod -R go=u-w sysemu-coverage-insns-regs); \
	  tar cf - sysemu-coverage-full | (cd ~/public_html && tar xf - && chmod -R go=u-w sysemu-coverage-full); \
	else \
	  echo "Cannot find lcov. Do you need to run 'yum install lcov'?"; \
	fi
