SYSEMU_REL_LIB_DIR?=../../build
BEMU_BUILD_DIR?=

include ../../build/emu.mk

sysemu_hdrs := \
	common/main_memory.h \
	common/main_memory_region.h \
	common/main_memory_region_atomic.h \
	common/main_memory_region_esr.h \
	common/main_memory_region_rbox.h \
	common/main_memory_region_scp_linear.h \
	sys_emu/api_communicate.h \
	sys_emu/sys_emu.h \
	sys_emu/log.h \
	sys_emu/testLog.h

sysemu_cpp_srcs := \
	common/main_memory.cpp \
	common/main_memory_region.cpp \
	common/main_memory_region_atomic.cpp \
	common/main_memory_region_esr.cpp \
	common/main_memory_region_rbox.cpp \
	common/main_memory_region_scp_linear.cpp \
	sys_emu/api_communicate.cpp \
	sys_emu/log.cpp \
	sys_emu/net_emulator.cpp \
	sys_emu/sys_emu.cpp \
	sys_emu/sys_emu_main.cpp \
	sys_emu/sys_emu_log.cpp

sysemu_SRCS := $(sysemu_cpp_srcs) \
	       $(emu_cpp_srcs)

sysemu_OBJS := $(addprefix $(BEMU_BUILD_DIR)sys_emu_obj/,$(addsuffix .o,$(basename $(sysemu_SRCS))))
# Remove the file that implements the main function
libsysemu_OBJS := $(filter-out $(BEMU_BUILD_DIR)sys_emu_obj/sys_emu/sys_emu_main.o,$(sysemu_OBJS))
sysemu_DEPS := $(addsuffix .d,$(basename $(sysemu_OBJS)))
sysemu_DIRS := $(sort $(dir $(sysemu_OBJS)))

CC       := gcc
CXX      := g++
CPPFLAGS := -I../.. -I.. -DSYS_EMU
CXXFLAGS := -MMD -MP -Wall -Wextra -Wpedantic -Werror -fPIC -g -O2 -std=c++11
CFLAGS   := -MMD -MP -Wall -Wextra -Wpedantic -Werror -fPIC -g -O2 -std=c11
LDLIBS   := $(SYSEMU_REL_LIB_DIR)/libfpu.a -lm

# FIXME: Remove the following when we convert everything away from C++ floats
CXXFLAGS += -mf16c -fno-fast-math

.PHONY: all
all: $(BEMU_BUILD_DIR)sys_emu $(BEMU_BUILD_DIR)libsysemu.a

.PHONY: static
static: LDFLAGS := -static
static: sys_emu
	strip $<

.PHONY: debug
debug: CPPFLAGS += -DSYSEMU_DEBUG
debug: sys_emu

.PHONY: profiling
profiling: CPPFLAGS += -DBEMU_PROFILING -DSYSEMU_PROFILING
profiling: sys_emu

.PHONY: clean
clean:
	$(MAKE) -C ../../build BEMU_BUILD_DIR=$(BEMU_BUILD_DIR) clean_libfpu
	$(RM) -r $(BEMU_BUILD_DIR)sys_emu $(BEMU_BUILD_DIR)sys_emu_obj/ *.a sysemu-artifacts*

$(SYSEMU_REL_LIB_DIR)/libfpu.a: force
	$(MAKE) -C ../../build BEMU_BUILD_DIR=$(BEMU_BUILD_DIR) $(addprefix $(BEMU_BUILD_DIR),$(notdir $@))

artifacts_dir:
	mkdir -p sysemu-artifacts

.PHONY: install_headers
install_headers : $(addprefix ../../,$(sysemu_hdrs)) | artifacts_dir
	for i in $^; do \
	  FILE=sysemu-artifacts/`echo $$i | sed -e 's/..\/..\///' ` ; \
	    mkdir -p `dirname $$FILE` \
	  && cp -f $$i $$FILE  \
	; done

.PHONY: install_binaries
install_binaries: $(BEMU_BUILD_DIR)libsysemu.a $(BEMU_BUILD_DIR)sys_emu $(SYSEMU_REL_LIB_DIR)/libfpu.a | artifacts_dir
	for i in $^; do cp -f $$i sysemu-artifacts/; done

sysemu-artifacts.tar.gz: install_binaries install_headers | artifacts_dir
	tar zcvf sysemu-artifacts.tar.gz sysemu-artifacts

$(BEMU_BUILD_DIR)libsysemu.a:  $(libsysemu_OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(BEMU_BUILD_DIR)sys_emu: $(sysemu_OBJS) $(SYSEMU_REL_LIB_DIR)/libfpu.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(TARGET_ARCH) $(sysemu_OBJS) $(LOADLIBES) $(LDLIBS) -o $@
	-if [ "$(BEMU_BUILD_DIR)sys_emu" != "sys_emu" ]  ; then /bin/cp -n $(BEMU_BUILD_DIR)sys_emu sys_emu; fi

$(sysemu_DIRS):
	mkdir -p $@

$(BEMU_BUILD_DIR)sys_emu_obj/%.o: ../../%.cpp | $(sysemu_DIRS)
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<

$(sysemu_OBJS): Makefile

-include $(sysemu_DEPS)

.PHONY: force
force:
