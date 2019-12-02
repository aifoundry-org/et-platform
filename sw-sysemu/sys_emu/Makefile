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
LD       := ld
CPPFLAGS := -MMD -MP -I.. -I. -DSYS_EMU
CXXFLAGS := -Wall -Wextra -Werror -pedantic-errors -fPIC -std=c++11
CFLAGS   := -Wall -Wextra -Werror -pedantic-errors -fPIC -std=c11
LDLIBS   := -lm

ifeq ($(DEBUG),)
    CXXFLAGS += -g -O2
    CFLAGS += -g -O2
else
    CPPFLAGS += -D_GLIBCXX_DEBUG
    CXXFLAGS += -g3 -O1 -fno-inline
    CFLAGS += -g3 -O1 -fno-inline
endif

ifneq ($(COVERAGE),)
  CXXFLAGS += --coverage
  CFLAGS   += --coverage
  LDLIBS   += --coverage
endif

ifneq ($(PROFILING),)
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
	$(MAKE) -C ../fpu libfpu_OBJDIR=$(abspath $(libfpu_OBJDIR)) DEBUG=$(DEBUG) all

%.o: %.xml
	$(SILENT)xxd -i $< | $(CC) -c -x c - -o $@

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

$(sysemu_OBJDIR)/obj/%.xml: ../%.xml | $$(@D)/.
	cp -f $< $@

$(sysemu_OBJDIR)/obj/sys_emu/gdb_target_csr.xml: ../csrs.h | $$(@D)/.
	@echo "<?xml version=\"1.0\"?>" > $@
	@echo "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">" >> $@
	@echo "<feature name=\"org.gnu.gdb.riscv.csr\">" >> $@
	@echo -e \
	'#define CSRDEF(num, lower, upper) <reg name= #lower bitsize="64"/>\n'\
	'#include "csrs.h"\n'\
	'#undef CSRDEF' | $(CXX) -P -E  -I.. -I. - >> $@
	@echo "</feature>" >> $@

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
