# Overwrite settings here, e.g.:
COMPILE_OPT = -O0
VERBOSE ?= 1

# Include common flags and build targets
include common/include.mk

CPPFLAGS += -Icommon
CPPFLAGS += -I$(BUILD_DIR)/../Release/et-common-libs/cm-umode/include

# Custom variables
target = $(BUILD_DIR)/inst_seq.elf
srcs = src/inst_seq.c
objs = $(srcs:%=$(BUILD_DIR)/objs/%.o)
deps = $(objs:%.o=%.d)

.PHONY: all clean
all: $(target)

$(target): $(objs)

-include $(deps)

clean:
	@rm -rf $(BUILD_DIR)



