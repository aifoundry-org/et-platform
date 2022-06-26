# ========
# Settings

# Set to show compile commands
VERBOSE        ?= 0
# Address for _boot symbol / start PC
BOOT_ADDR      ?= 0x8000001000
# Top of the stack
STACK_TOP      ?= 0x80A1FFC000 
# Architecture / ABI
MINION_MARCH   ?= rv64imf
MINION_MABI    ?= lp64f
# Enable features (not recommended)
USE_EXCEPTIONS ?= 0
USE_RTTI       ?= 0
USE_STDLIB     ?= 0
# Other compile flags
COMPILE_OPT    ?= -O2
EXTRA_CFLAGS   ?=
# Extra paths/sources
RISCV          ?=
BUILD_DIR      ?= ../build/examples
BOOT_SRC       ?= common/boot.S
CRT_SRC        ?= common/crt.S

# ================
# Derived settings
# (do not modify!)

VERBOSE_0 = @
ECHO = $(VERBOSE_$(VERBOSE))

# RISC-V Executables
AS  = riscv64-unknown-elf-as
CC  = riscv64-unknown-elf-gcc
CXX = riscv64-unknown-elf-g++
OD  = riscv64-unknown-elf-objdump

CFLAGS_STDLIB_0 = -nostdlib
CXXFLAGS_RTTI_0 = -fno-rtti
CXXFLAGS_EXCEPTIONS_0 = -fno-exceptions -fno-unwind-tables

# Compile flags (C)
CFLAGS += -Wall \
	     -Wextra \
	     -Werror \
	     -Wcast-align \
	     -Wcast-qual \
	     -Wmissing-include-dirs \
	     -Wlogical-op \
	     -Wredundant-decls \
	     -Wno-unknown-pragmas \
	     -Wundef \
	     -Wunused \
	     -fdiagnostics-show-option \
	     -fomit-frame-pointer \
	     $(COMPILE_OPT) -g \
	     -I$(CURDIR) \
	     -mcmodel=medany \
	     -march=$(MINION_MARCH) \
	     -mabi=$(MINION_MABI) \
	     -Wa,-march=$(MINION_MARCH),-mabi=$(MINION_MABI)

CFLAGS += $(CFLAGS_STDLIB_$(USE_STDLIB))

# Compile flags (C++)
CXXFLAGS += $(CFLAGS) \
	   -Woverloaded-virtual \
	   -Wno-unused-local-typedefs \
	   -Wctor-dtor-privacy \
	   -Wstrict-null-sentinel \
	   -Wnoexcept \
	   -std=c++11

CXXFLAGS += $(CXXFLAGS_RTTI_$(USE_RTTI))
CXXFLAGS += $(CXXFLAGS_EXCEPTIONS_$(USE_EXCEPTIONS))

# Linker flags
LDFLAGS += -Wl,--section-start=bootrom=$(BOOT_ADDR),--section-start=stack=$(STACK_TOP)

# Assembly sources
EXTRA_SRCS = $(BOOT_SRC)
ifeq ($(USE_STDLIB),0)
EXTRA_SRCS += $(CRT_SRC)
endif

# ===========
# Build rules

$(BUILD_DIR)/objs/%.c.o: %.c
	@echo "-- Compiling $<"
	@mkdir -p $(@D)
	$(ECHO)$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

$(BUILD_DIR)/objs/%.cpp.o: %.cpp
	@echo "-- Compiling $<"
	@mkdir -p $(@D)
	$(ECHO)$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c -o $@ $<

$(BUILD_DIR)/objs/%.cc.o: %.cc
	@echo "-- Compiling $<"
	@mkdir -p $(@D)
	$(ECHO)$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c -o $@ $<

$(BUILD_DIR)/%.elf: $(EXTRA_SRCS)
	@echo "-- Linking $@"
	@mkdir -p $(@D)
	$(ECHO)$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^


