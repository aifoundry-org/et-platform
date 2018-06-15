BUILD_DIR=build
CHECKER_LIBNAME=$(BUILD_DIR)/libchecker.a
EMU_LIBNAME=$(BUILD_DIR)/emu.so

# setting list of files to compile
CHECKER_SOURCES= checker.cpp \
                 function_pointer_cache.cpp \
                 instruction_cache_block.cpp \
                 instruction_cache.cpp \
                 instruction.cpp \
                 main_memory.cpp \
                 main_memory_region.cpp \
                 main_memory_region_atomic.cpp \
                 main_memory_region_tbox.cpp \
                 main_memory_region_rbox.cpp

EMU_SOURCES= ../emu.c \
             ../log.c \
             ../cvt.c \
             ../txs.c \
             ../ttrans.c \
             ../tbox_emu.c \
             ../ipc.c \
             gio.cpp \
             $(TESTROOT)/libs/testLog.cc

# setting include paths
CXXINC= -I.. -I$(TESTROOT)/libs

# Variables
EMU_DEBUG=-DEMU_DEBUG # To enable debug
#EMU_DEBUG=            # To disable debug
MIN_PER_N=$(shell grep  "define MIN_PER_N " $(RTLROOT)/inc/shire_defines.vh | sed s/".*MIN_PER_N \+\([0-9]\+\).*"/"\1"/ )
NUM_NEIGH=$(shell grep  "define NUM_NEIGH " $(RTLROOT)/inc/shire_defines.vh | sed s/".*NUM_NEIGH \+\([0-9]\+\).*"/"\1"/ )

ifeq ($(NEW_TRANS_UNIT), 1)
NEW_TRANS_UNIT_DEF=-DNEW_TRANS_UNIT
endif

ifeq ($(USE_FAKE_TXFMA), 1)
	USE_FAKE_TXFMA_DEF=-DUSE_FAKE_TXFMA
endif
# tool configuration
CXX=g++
CXXFLAGS=-mf16c -g -DCHECKER -fPIC -fno-fast-math $(EMU_DEBUG) -O2 $(NEW_TRANS_UNIT_DEF) $(USE_FAKE_TXFMA_DEF)
DEPDIR = .depend
AR=ar
ARFLAGS=rvs
SHARED_OBJ_FLAGS=-shared -fPIC
HASH_FLAGS=-std=c++11

all: $(CHECKER_LIBNAME) $(EMU_LIBNAME)

define OBJ_NAME = 
$(addsuffix .o,$(basename $(addprefix $(BUILD_DIR)/,$(notdir $1))))
endef

define CHECKER_OBJ_RULE =
$(call OBJ_NAME,$1): $1 | $(BUILD_DIR)  $(BUILD_DIR)/.depend/
	$(CXX) $(CXXINC) $(CXXFLAGS)  $(HASH_FLAGS) $(EMU_DEBUG) -c $$< -o $$@ -MMD -MF $$(@D)/.depend/$$(notdir $$(@:%.o=%.d))

CHECKER_OBJECTS += $(call OBJ_NAME,$1)
endef

define EMU_OBJ_RULE =
$(call OBJ_NAME,$1): $1 | $(BUILD_DIR)  $(BUILD_DIR)/.depend/
	$(CXX) $(CXXINC) $(CXXFLAGS) $(SHARED_OBJ_FLAGS) $(EMU_DEBUG) -DMIN_PER_N=$(MIN_PER_N) -DNUM_NEIGH=$(NUM_NEIGH) -c $$< -o $$@ -MMD -MF $$(@D)/.depend/$$(notdir $$(@:%.o=%.d))

EMU_OBJECTS += $(call OBJ_NAME,$1)
endef

$(foreach src,$(CHECKER_SOURCES), $(eval $(call CHECKER_OBJ_RULE,$(src))))
$(foreach src,$(EMU_SOURCES), $(eval $(call EMU_OBJ_RULE,$(src))))

include $(wildcard $(BUILD_DIR)/$1/$(BUILD_DIR_CSRC_LIB)/.depend/*.d)

$(CHECKER_LIBNAME): $(CHECKER_OBJECTS)
	$(AR) $(ARFLAGS) $(CHECKER_LIBNAME) $(CHECKER_OBJECTS)

$(EMU_LIBNAME): $(EMU_OBJECTS)
	$(CXX) $(CXXFLAGS) $(SHARED_OBJ_FLAGS) $(EMU_OBJECTS) -o $(EMU_LIBNAME)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/.depend/:
	mkdir -p $(BUILD_DIR)/.depend/
clean:
	rm -rf $(BUILD_DIR)

# manual rule to force regeneration of emu.o if # minions changed in RTL
$(BUILD_DIR)build/emu.o: $(RTLROOT)/inc/shire_defines.vh

