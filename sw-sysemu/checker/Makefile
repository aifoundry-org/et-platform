BUILD_DIR=build
BUILD_DIR_NN=$(BUILD_DIR)_nn
CHECKER_LIBNAME=$(BUILD_DIR)/libchecker.a
CHECKER_LIBNAME_NN=$(BUILD_DIR_NN)/libchecker.a
EMU_LIBNAME=$(BUILD_DIR)/emu.so
EMU_LIBNAME_NN=$(BUILD_DIR_NN)/emu.so

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

# tool configuration
CXX=g++
CXXFLAGS=-mf16c -g -DCHECKER -fPIC -fno-fast-math $(EMU_DEBUG) -O2 $(NEW_TRANS_UNIT_DEF)
CXXFLAGS_NN=-mf16c -g -DCHECKER -fPIC -fno-fast-math $(EMU_DEBUG) -O2 $(NEW_TRANS_UNIT_DEF) -DUSE_FAKE_TXFMA
DEPDIR = .depend
AR=ar
ARFLAGS=rvs
SHARED_OBJ_FLAGS=-shared -fPIC
HASH_FLAGS=-std=c++11

all: normal nn

normal: $(CHECKER_LIBNAME) $(EMU_LIBNAME)

nn: $(CHECKER_LIBNAME_NN) $(EMU_LIBNAME_NN)

define OBJ_NAME = 
$(addsuffix .o,$(basename $(addprefix $(BUILD_DIR)/,$(notdir $1))))
endef

define OBJ_NAME_NN = 
$(addsuffix .o,$(basename $(addprefix $(BUILD_DIR_NN)/,$(notdir $1))))
endef

define CHECKER_OBJ_RULE =
$(call OBJ_NAME,$1): $1 | $(BUILD_DIR)  $(BUILD_DIR)/.depend/
	$(CXX) $(CXXINC) $(CXXFLAGS)  $(HASH_FLAGS) $(EMU_DEBUG) -c $$< -o $$@ -MMD -MF $$(@D)/.depend/$$(notdir $$(@:%.o=%.d))

CHECKER_OBJECTS += $(call OBJ_NAME,$1)
endef

define CHECKER_OBJ_RULE_NN =
$(call OBJ_NAME_NN,$1): $1 | $(BUILD_DIR_NN)  $(BUILD_DIR_NN)/.depend/
	$(CXX) $(CXXINC) $(CXXFLAGS_NN)  $(HASH_FLAGS) $(EMU_DEBUG) -c $$< -o $$@ -MMD -MF $$(@D)/.depend/$$(notdir $$(@:%.o=%.d))

CHECKER_OBJECTS_NN += $(call OBJ_NAME_NN,$1)
endef

define EMU_OBJ_RULE =
$(call OBJ_NAME,$1): $1 | $(BUILD_DIR)  $(BUILD_DIR)/.depend/
	$(CXX) $(CXXINC) $(CXXFLAGS) $(SHARED_OBJ_FLAGS) $(EMU_DEBUG) -DMIN_PER_N=$(MIN_PER_N) -DNUM_NEIGH=$(NUM_NEIGH) -c $$< -o $$@ -MMD -MF $$(@D)/.depend/$$(notdir $$(@:%.o=%.d))

EMU_OBJECTS += $(call OBJ_NAME,$1)
endef

define EMU_OBJ_RULE_NN =
$(call OBJ_NAME_NN,$1): $1 | $(BUILD_DIR_NN)  $(BUILD_DIR_NN)/.depend/
	$(CXX) $(CXXINC) $(CXXFLAGS_NN) $(SHARED_OBJ_FLAGS) $(EMU_DEBUG) -DMIN_PER_N=$(MIN_PER_N) -DNUM_NEIGH=$(NUM_NEIGH) -c $$< -o $$@ -MMD -MF $$(@D)/.depend/$$(notdir $$(@:%.o=%.d))

EMU_OBJECTS_NN += $(call OBJ_NAME_NN,$1)
endef

$(foreach src,$(CHECKER_SOURCES), $(eval $(call CHECKER_OBJ_RULE,$(src))))
$(foreach src,$(EMU_SOURCES), $(eval $(call EMU_OBJ_RULE,$(src))))

$(foreach src,$(CHECKER_SOURCES), $(eval $(call CHECKER_OBJ_RULE_NN,$(src))))
$(foreach src,$(EMU_SOURCES), $(eval $(call EMU_OBJ_RULE_NN,$(src))))

include $(wildcard $(BUILD_DIR)/$1/$(BUILD_DIR_CSRC_LIB)/.depend/*.d)
include $(wildcard $(BUILD_DIR_NN)/$1/$(BUILD_DIR_CSRC_LIB)/.depend/*.d)

$(CHECKER_LIBNAME): $(CHECKER_OBJECTS)
	$(AR) $(ARFLAGS) $(CHECKER_LIBNAME) $(CHECKER_OBJECTS)

$(CHECKER_LIBNAME_NN): $(CHECKER_OBJECTS_NN)
	$(AR) $(ARFLAGS) $(CHECKER_LIBNAME_NN) $(CHECKER_OBJECTS_NN)

$(EMU_LIBNAME): $(EMU_OBJECTS)
	$(CXX) $(CXXFLAGS) $(SHARED_OBJ_FLAGS) $(EMU_OBJECTS) -o $(EMU_LIBNAME)

$(EMU_LIBNAME_NN): $(EMU_OBJECTS_NN)
	$(CXX) $(CXXFLAGS_NN) $(SHARED_OBJ_FLAGS) $(EMU_OBJECTS_NN) -o $(EMU_LIBNAME_NN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR_NN):
	mkdir -p $(BUILD_DIR_NN)

$(BUILD_DIR)/.depend/:
	mkdir -p $(BUILD_DIR)/.depend/

$(BUILD_DIR_NN)/.depend/:
	mkdir -p $(BUILD_DIR_NN)/.depend/

clean:
	rm -rf $(BUILD_DIR) $(BUILD_DIR_NN)

# manual rule to force regeneration of emu.o if # minions changed in RTL
$(BUILD_DIR)build/emu.o: $(RTLROOT)/inc/shire_defines.vh
$(BUILD_DIR_NN)build/emu.o: $(RTLROOT)/inc/shire_defines.vh

