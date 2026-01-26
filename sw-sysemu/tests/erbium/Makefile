# Erbium Tests Makefile

VERBOSE ?= 0

# Paths
BUILD_DIR := build
EMU       := ../../build/erbium_emu
RISCV     ?= /opt/et/bin

# Verbosity
VERBOSE_0 := @
ECHO := $(VERBOSE_$(VERBOSE))

# RISC-V toolchain
CC := $(RISCV)/riscv64-unknown-elf-gcc
OD := $(RISCV)/riscv64-unknown-elf-objdump

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

.PHONY: all build clean run list-tests

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
		$(EMU) -l -elf_load $(BUILD_DIR)/$$test.elf > $(BUILD_DIR)/$$test.out 2>&1; \
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
	@$(EMU) -l -elf_load $< > $(BUILD_DIR)/$*.out 2>&1; \
	if grep -q "Signal end test with FAIL" $(BUILD_DIR)/$*.out; then \
		echo "  FAIL"; exit 1; \
	elif grep -q "Signal end test with PASS" $(BUILD_DIR)/$*.out; then \
		echo "  PASS"; \
	else \
		echo "  UNKNOWN"; exit 1; \
	fi

clean:
	rm -rf $(BUILD_DIR)
