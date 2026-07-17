# Erbium Tests Makefile

VERBOSE ?= 0

# Paths
BUILD_DIR := build
EMU       := ../../build/erbium_emu
RISCV     ?= /opt/et/bin
EMU_ARGS  ?= -minions 0xff -mins_dis
INPUT_DIR := inputs

# Verbosity
VERBOSE_0 := @
ECHO := $(VERBOSE_$(VERBOSE))

# RISC-V toolchain
CC  := $(RISCV)/riscv64-unknown-elf-gcc
OD  := $(RISCV)/riscv64-unknown-elf-objdump
GDB := $(RISCV)/riscv64-unknown-elf-gdb

# Compile flags
CFLAGS := -Wall -Wextra -Werror \
          -nostdlib \
          -O2 -g \
          -mcmodel=medany \
          -march=rv64imfc \
          -mabi=lp64f

CPPFLAGS := -Iinclude -Icommon

# Linker flags
# Note: --no-warn-rwx-segments suppresses W^X warning for MRAM region.
# For embedded test code on emulator, single RWX segment is acceptable.
LDFLAGS := -Tcommon/erbium.ld -Wl,--section-start=bootrom=0x200a000 \
           -Wl,--no-warn-rwx-segments

# Sources
BOOT_SRCS := common/boot.S common/crt.S common/trap.S

# Test discovery
tests_src := $(wildcard src/*.c)
tests     := $(tests_src:src/%.c=%)
tests_elf := $(tests:%=$(BUILD_DIR)/%.elf)

num_tests := $(words $(tests))

.PHONY: all build clean run run-gdb list-tests

all: build

build: $(tests_elf)

list-tests:
	@echo "$(tests)" | tr ' ' '\n'

# Build test ELF
$(BUILD_DIR)/%.elf: src/%.c $(BOOT_SRCS)
	@echo "-- Building $@"
	@mkdir -p $(@D)
	$(ECHO)$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^

# Run all tests with summary
run: $(tests_elf)
	@npass=0; nfail=0; \
	for test in $(tests); do \
		echo "+ running $$test"; \
		cat "$(INPUT_DIR)/$$test.in" 2>/dev/null | \
			$(EMU) -l $(EMU_ARGS) -elf_load $(BUILD_DIR)/$$test.elf > $(BUILD_DIR)/$$test.out 2>&1; \
		if grep -q "Signal end test with FAIL" $(BUILD_DIR)/$$test.out; then \
			echo "  FAIL"; \
			nfail=$$((nfail + 1)); \
		elif grep -q "Signal end test with PASS" $(BUILD_DIR)/$$test.out; then \
			echo "  PASS"; \
			npass=$$((npass + 1)); \
		else \
			echo "  UNKNOWN (no pass/fail signal)"; \
			nfail=$$((nfail + 1)); \
		fi; \
	done; \
	echo ""; \
	if [ $$nfail -eq 0 ]; then \
		echo "SUCCESS: $(num_tests) total | $$npass passed | $$nfail failed"; \
	else \
		echo "FAILURE: $(num_tests) total | $$npass passed | $$nfail failed"; \
		exit 1; \
	fi

# Run individual test
run/%: $(BUILD_DIR)/%.elf
	@echo "+ running $*"
	@cat "$(INPUT_DIR)/$*.in" 2>/dev/null | \
		$(EMU) -l $(EMU_ARGS) -elf_load $< > $(BUILD_DIR)/$*.out 2>&1; \
	if grep -q "Signal end test with FAIL" $(BUILD_DIR)/$*.out; then \
		echo "  FAIL"; exit 1; \
	elif grep -q "Signal end test with PASS" $(BUILD_DIR)/$*.out; then \
		echo "  PASS"; \
	else \
		echo "  UNKNOWN"; exit 1; \
	fi

# Boot protocol tests: re-run all tests with --boot_elf
# The ELF entry point is used as the payload PC automatically.
run-boot-protocol: $(tests_elf)
	@npass=0; nfail=0; \
	for test in $(tests); do \
		echo "+ running $$test (boot protocol)"; \
		cat "$(INPUT_DIR)/$$test.in" 2>/dev/null | \
			$(EMU) -l $(EMU_ARGS) --boot_elf $(BUILD_DIR)/$$test.elf > $(BUILD_DIR)/$$test.bp.out 2>&1; \
		if grep -q "Signal end test with FAIL" $(BUILD_DIR)/$$test.bp.out; then \
			echo "  FAIL"; \
			nfail=$$((nfail + 1)); \
		elif grep -q "Signal end test with PASS" $(BUILD_DIR)/$$test.bp.out; then \
			echo "  PASS"; \
			npass=$$((npass + 1)); \
		else \
			echo "  UNKNOWN (no pass/fail signal)"; \
			nfail=$$((nfail + 1)); \
		fi; \
	done; \
	echo ""; \
	if [ $$nfail -eq 0 ]; then \
		echo "SUCCESS: $(num_tests) boot protocol | $$npass passed | $$nfail failed"; \
	else \
		echo "FAILURE: $(num_tests) boot protocol | $$npass passed | $$nfail failed"; \
		exit 1; \
	fi

# GDB tests (explicit list)
gdb_tests     := thread_disable_consistency
num_gdb_tests := $(words $(gdb_tests))

# Run all GDB tests
run-gdb: $(gdb_tests:%=$(BUILD_DIR)/%.elf) $(gdb_tests:%=gdb/%.gdb)
	@if [ -z "$(gdb_tests)" ]; then \
		echo "No GDB tests found (no gdb/*.gdb scripts)"; \
		exit 0; \
	fi; \
	npass=0; nfail=0; \
	for test in $(gdb_tests); do \
		echo "+ running $$test (GDB)"; \
		$(EMU) -l $(EMU_ARGS) -gdb -elf_load $(BUILD_DIR)/$$test.elf > $(BUILD_DIR)/$$test.gdb.out 2>&1 & \
		emu_pid=$$!; \
		ready=0; tries=0; \
		while [ $$tries -lt 50 ]; do \
			if grep -q "Listening on port" $(BUILD_DIR)/$$test.gdb.out; then \
				ready=1; break; \
			fi; \
			if ! kill -0 $$emu_pid 2>/dev/null; then \
				break; \
			fi; \
			tries=$$((tries + 1)); \
			sleep 0.1; \
		done; \
		if [ $$ready -eq 0 ]; then \
			echo "  FAIL (GDB stub did not become ready)"; \
			kill $$emu_pid 2>/dev/null || true; \
			wait $$emu_pid 2>/dev/null || true; \
			nfail=$$((nfail + 1)); \
			continue; \
		fi; \
		gdb_rc=0; \
		$(GDB) -q -batch -x gdb/$$test.gdb $(BUILD_DIR)/$$test.elf > /dev/null 2>&1 || gdb_rc=$$?; \
		sleep 1; \
		kill $$emu_pid 2>/dev/null || true; \
		wait $$emu_pid 2>/dev/null || true; \
		if grep -q "Signal end test with FAIL" $(BUILD_DIR)/$$test.gdb.out; then \
			echo "  FAIL"; \
			nfail=$$((nfail + 1)); \
		elif grep -q "Signal end test with PASS" $(BUILD_DIR)/$$test.gdb.out; then \
			echo "  PASS"; \
			npass=$$((npass + 1)); \
		elif [ $$gdb_rc -ne 0 ]; then \
			echo "  UNKNOWN (GDB command failed: $$gdb_rc)"; \
			nfail=$$((nfail + 1)); \
		else \
			echo "  UNKNOWN (no pass/fail signal)"; \
			nfail=$$((nfail + 1)); \
		fi; \
	done; \
	echo ""; \
	if [ $$nfail -eq 0 ]; then \
		echo "SUCCESS: $(num_gdb_tests) GDB tests | $$npass passed | $$nfail failed"; \
	else \
		echo "FAILURE: $(num_gdb_tests) GDB tests | $$npass passed | $$nfail failed"; \
		exit 1; \
	fi

# Run individual GDB test
run-gdb/%: $(BUILD_DIR)/%.elf gdb/%.gdb
	@echo "+ running $* (GDB)"
	@$(EMU) -l $(EMU_ARGS) -gdb -elf_load $< > $(BUILD_DIR)/$*.gdb.out 2>&1 & \
	emu_pid=$$!; \
	ready=0; tries=0; \
	while [ $$tries -lt 50 ]; do \
		if grep -q "Listening on port" $(BUILD_DIR)/$*.gdb.out; then \
			ready=1; break; \
		fi; \
		if ! kill -0 $$emu_pid 2>/dev/null; then \
			break; \
		fi; \
		tries=$$((tries + 1)); \
		sleep 0.1; \
	done; \
	if [ $$ready -eq 0 ]; then \
		echo "  FAIL (GDB stub did not become ready)"; \
		kill $$emu_pid 2>/dev/null || true; \
		wait $$emu_pid 2>/dev/null || true; \
		exit 1; \
	fi; \
	gdb_rc=0; \
	$(GDB) -q -batch -x gdb/$*.gdb $< > /dev/null 2>&1 || gdb_rc=$$?; \
	sleep 1; \
	kill $$emu_pid 2>/dev/null || true; \
	wait $$emu_pid 2>/dev/null || true; \
	if grep -q "Signal end test with FAIL" $(BUILD_DIR)/$*.gdb.out; then \
		echo "  FAIL"; exit 1; \
	elif grep -q "Signal end test with PASS" $(BUILD_DIR)/$*.gdb.out; then \
		echo "  PASS"; \
	elif [ $$gdb_rc -ne 0 ]; then \
		echo "  UNKNOWN (GDB command failed: $$gdb_rc)"; exit 1; \
	else \
		echo "  UNKNOWN"; exit 1; \
	fi

clean:
	rm -rf $(BUILD_DIR)
