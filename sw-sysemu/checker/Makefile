BUILD_DIR=$(shell pwd)/build
BUILD_DIR_NN=$(BUILD_DIR)_nn
CHECKER_LIBNAME=$(BUILD_DIR)/libchecker.so
CHECKER_LIBNAME_NN=$(BUILD_DIR_NN)/libchecker.so
EMU_LIBNAME=$(BUILD_DIR)/libemu.so
EMU_LIBNAME_NN=$(BUILD_DIR_NN)/libemu.so

# setting list of files to compile
CHECKER_SOURCES= checker.cpp \
                 instruction_cache.cpp \
                 instruction.cpp \
                 main_memory.cpp \
                 main_memory_region.cpp \
                 main_memory_region_atomic.cpp \
                 main_memory_region_tbox.cpp \
                 main_memory_region_rbox.cpp \

EMU_SOURCES= ../emu.cpp \
	     ../log.cpp \
	     ../cvt.cpp \
	     ../txs.cpp \
	     ../ttrans.cpp \
	     ../tbox_emu.cpp \
	     ../ipc.cpp \
	     ../emu_gio.cpp \
	     $(TESTROOT)/libs/testLog.cc

# setting include paths
CXXINC= -I.. -I$(TESTROOT)/libs

# Variables
EMU_DEBUG=-DEMU_DEBUG # To enable debug
#EMU_DEBUG=            # To disable debug
MIN_PER_N=$(shell grep  "define MIN_PER_N " $(RTLROOT)/inc/shire_defines.vh | sed s/".*MIN_PER_N \+\([0-9]\+\).*"/"\1"/ )
NUM_NEIGH=$(shell grep  "define NUM_NEIGH " $(RTLROOT)/inc/shire_defines.vh | sed s/".*NUM_NEIGH \+\([0-9]\+\).*"/"\1"/ )

# tool configuration
CXX=g++
CXXFLAGS=-mf16c -Wall -Werror -g -DCHECKER -fPIC -fno-fast-math $(EMU_DEBUG) -O2 -std=c++11
CXXFLAGS_NN=-mf16c -Wall -Werror -g -DCHECKER -fPIC -fno-fast-math $(EMU_DEBUG) -O2 -std=c++11 -DUSE_FAKE_TXFMA
DEPDIR = .depend
AR=ar
ARFLAGS=rvs
SHARED_OBJ_FLAGS=-shared
HASH_FLAGS=-std=c++11
CHECKER_LIBS=-L$(BUILD_DIR) -Wl,-rpath=$(BUILD_DIR) -lemu
CHECKER_LIBS_NN=-L$(BUILD_DIR_NN) -Wl,-rpath=$(BUILD_DIR_NN) -lemu

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
$(call OBJ_NAME,$1): $1 | $(BUILD_DIR)  $(BUILD_DIR)/.depend
	$(CXX) $(CXXINC) $(CXXFLAGS)  $(EMU_DEBUG) -c $$< -o $$@ -MMD -MF $$(@D)/.depend/$$(notdir $$(@:%.o=%.d))

CHECKER_OBJECTS += $(call OBJ_NAME,$1)
endef

define CHECKER_OBJ_RULE_NN =
$(call OBJ_NAME_NN,$1): $1 | $(BUILD_DIR_NN)  $(BUILD_DIR_NN)/.depend
	$(CXX) $(CXXINC) $(CXXFLAGS_NN)  $(EMU_DEBUG) -c $$< -o $$@ -MMD -MF $$(@D)/.depend/$$(notdir $$(@:%.o=%.d))

CHECKER_OBJECTS_NN += $(call OBJ_NAME_NN,$1)
endef

define EMU_OBJ_RULE =
$(call OBJ_NAME,$1): $1 | $(BUILD_DIR)  $(BUILD_DIR)/.depend
	$(CXX) $(CXXINC) $(CXXFLAGS) $(EMU_DEBUG) -DMIN_PER_N=$(MIN_PER_N) -DNUM_NEIGH=$(NUM_NEIGH) -c $$< -o $$@ -MMD -MF $$(@D)/.depend/$$(notdir $$(@:%.o=%.d))

EMU_OBJECTS += $(call OBJ_NAME,$1)
endef

define EMU_OBJ_RULE_NN =
$(call OBJ_NAME_NN,$1): $1 | $(BUILD_DIR_NN)  $(BUILD_DIR_NN)/.depend
	$(CXX) $(CXXINC) $(CXXFLAGS_NN) $(EMU_DEBUG) -DMIN_PER_N=$(MIN_PER_N) -DNUM_NEIGH=$(NUM_NEIGH) -c $$< -o $$@ -MMD -MF $$(@D)/.depend/$$(notdir $$(@:%.o=%.d))

EMU_OBJECTS_NN += $(call OBJ_NAME_NN,$1)
endef

$(foreach src,$(CHECKER_SOURCES), $(eval $(call CHECKER_OBJ_RULE,$(src))))
$(foreach src,$(EMU_SOURCES), $(eval $(call EMU_OBJ_RULE,$(src))))

$(foreach src,$(CHECKER_SOURCES), $(eval $(call CHECKER_OBJ_RULE_NN,$(src))))
$(foreach src,$(EMU_SOURCES), $(eval $(call EMU_OBJ_RULE_NN,$(src))))

include $(wildcard $(BUILD_DIR)/$1/$(BUILD_DIR_CSRC_LIB)/.depend/*.d)
include $(wildcard $(BUILD_DIR_NN)/$1/$(BUILD_DIR_CSRC_LIB)/.depend/*.d)

$(CHECKER_LIBNAME): $(CHECKER_OBJECTS) $(EMU_OBJECTS)
	$(CXX) -o $@ $(CXXFLAGS) $(SHARED_OBJ_FLAGS) $^

$(CHECKER_LIBNAME_NN): $(CHECKER_OBJECTS_NN) $(EMU_OBJECTS_NN)
	$(CXX) -o $@ $(CXXFLAGS) $(SHARED_OBJ_FLAGS) $^

$(EMU_LIBNAME): $(EMU_OBJECTS)
	$(CXX) -o $@ $(CXXFLAGS) $(SHARED_OBJ_FLAGS) $^

$(EMU_LIBNAME_NN): $(EMU_OBJECTS_NN)
	$(CXX) -o $@ $(CXXFLAGS) $(SHARED_OBJ_FLAGS) $^

$(BUILD_DIR) $(BUILD_DIR_NN) $(BUILD_DIR)/.depend $(BUILD_DIR_NN)/.depend:
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(BUILD_DIR_NN)

# manual rule to force regeneration of emu.o if # minions changed in RTL
$(BUILD_DIR)build/emu.o: $(RTLROOT)/inc/shire_defines.vh
$(BUILD_DIR_NN)build/emu.o: $(RTLROOT)/inc/shire_defines.vh

