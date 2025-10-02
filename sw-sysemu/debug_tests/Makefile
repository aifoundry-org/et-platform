arch = rv64imf
abi  = lp64f

VERBOSE ?= 0
DEBUG   ?= 0
LOG     ?= 0
NTRACE  ?= 0

export VERBOSE
export DEBUG

log_0 =
log_1 = -l

SYSEMU = ../sys_emu/build/sys_emu $(log_$(LOG))

CC = riscv64-unknown-elf-gcc
OD = riscv64-unknown-elf-objdump

CFLAGS += -nostdlib \
		  -mcmodel=medany \
		  -march=$(arch) \
		  -mabi=$(abi) \
		  -Wa,-march=$(arch),-mabi=$(abi) \
		  -Iinclude -Os
ifneq ($(NTRACE),0)
CFLAGS += -DNTRACE
endif

unexport CC
unexport CFLAGS

headers = $(wildcard include/*.h)
sources = common/trace.c common/lib.c
sources_spio = $(sources) common/debug.c

tests_src = $(wildcard tests/*.c)
tests     = $(tests_src:%.c=%)
tests_elf = $(tests:%=%.elf) $(tests:%=%.spio.elf)
tests_asm = $(tests_elf:%.elf=%.asm)
tests_out = $(tests:%=%.out)
tests_globals = $(tests:%=%.globals.h)

num_tests = $(shell echo $(tests) | wc -w)

grep_errs = grep -e FAIL -e FATAL -e ERROR

ifeq (n,$(findstring n,$(firstword -$(MAKEFLAGS))))
run_prefix = 
run_suffix =
else
ifeq ($(VERBOSE),0)
run_prefix = @echo "+ running $(@:run/%=%)";
run_suffix = | tee $(@:run/%=%.out) | $(grep_errs) | awk '1{err=1;print "  | "$$0}END{exit err}'
else
run_prefix =
run_suffix =
endif
endif

define TEST_template
$(subst /,_,$(1))_flags?=$$(shell grep -Po 'SYSEMU \K.*' $(1).c | head -n1)
.PHONY: run/$(1)
run/$(1): $(1).elf $(1).spio.elf
	$$(run_prefix)$$(SYSEMU) $$($(subst /,_,$(1))_flags) -elf $(1).elf -elf $(1).spio.elf $$(run_suffix)
endef

.PHONY: all disasm run clean sysemu clean-sysemu list-tests
all: $(tests_elf)

disasm: $(tests_asm)

list-tests:
	@echo "$(tests)" | tr ' ' '\n'

run: $(tests:%=run/%)
	@npass=`for tst in $(tests_out); do \
		$(grep_errs) -q $$tst || echo $$tst; \
		done | wc -l `; nfail=`echo "$(num_tests) - $$npass" | bc`; \
	   	[ $$npass -eq $(num_tests) ] \
		&& echo "SUCCESS: $(num_tests) total | $$npass passed | $$nfail failed" \
		|| echo "FAILURE: $(num_tests) total | $$npass passed | $$nfail failed"

clean: clean-libdebug
	$(RM) tests/*.out tests/*.elf tests/*.globals.h tests/*.asm

sysemu:
	$(MAKE) -C../sys_emu

clean-sysemu:
	$(MAKE) -C../sys_emu clean

$(foreach test,$(tests),$(eval $(call TEST_template,$(test))))

%.asm: %.elf
	$(OD) -j.text -D $< > $@

%.elf: %.c $(headers) $(sources) common/entry.S
	$(LINK.c) -Wl,-Tcommon/link.ld $(filter-out %.h,$^) $(OUTPUT_OPTION) 

.PRECIOUS: $(tests_globals)
%.globals.h: %.elf
	nm -S $< | awk '$$4 ~ /^g_*/{gsub(/^g_/,"ADDR_",$$4); print "#define",$$4,"0x"$$1}' > $@

%.spio.elf: %.c %.globals.h $(headers) $(sources_spio) common/spio_entry.S
	$(LINK.c) -include $(@:%.spio.elf=%.globals.h) -DSPIO -Wl,-Tcommon/spio_link.ld $(filter-out %.h,$^) $(OUTPUT_OPTION) 

libdebug: libdebug.a

clean-libdebug:
	$(RM) libdebug.a common/debug.o

libdebug.a: common/debug.o
	$(AR) $(ARFLAGS) $@ $<
