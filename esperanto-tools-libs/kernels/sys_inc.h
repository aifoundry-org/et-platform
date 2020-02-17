#ifndef SYS_INC_H
#define SYS_INC_H

// FIXME the follwing is used by SimulatorUtils.cpp:255:46
#define FIRMWARE_LOAD_ADDR 0x8000001000

#define SHIRES_COUNT 32

#define CACHE_LINE_BYTES 64

// FIXME this should move inside NEuralizer
/// Scratchpad bytes allocated for activation outputs soc-wise (32 Mb)
static const size_t SOC_ACT_OUT_L2SCP_BYTES = 0x2000000;

/// Scratchpad lines allocated for activation outputs soc-wise (32 Mb)
static const size_t SOC_ACT_OUT_L2SCP_LINES = SOC_ACT_OUT_L2SCP_BYTES / CACHE_LINE_BYTES;

/// Scratchpad bytes allocated for activation prefetch soc-wise (16 Mb)
static const size_t SOC_ACT_PREF_L2SCP_BYTES = 0x1000000;

/// Scratchpad lines allocated for activation prefetch soc-wise (16 Mb)
static const size_t SOC_ACT_PREF_L2SCP_LINES = SOC_ACT_PREF_L2SCP_BYTES / CACHE_LINE_BYTES;

/// Scratchpad bytes allocated for weight prefetch soc-wise (32 Mb)
static const size_t SOC_W_PREF_L2SCP_BYTES = 0x2000000;

/// Scratchpad lines allocated for weight prefetch soc-wise (32 Mb)
static const size_t SOC_W_PREF_L2SCP_LINES = SOC_W_PREF_L2SCP_BYTES / CACHE_LINE_BYTES;

/// Scratchpad bytes allocated for activation outputs shire-wise (1 Mb)
static const size_t SHIRE_ACT_OUT_L2SCP_BYTES = SOC_ACT_OUT_L2SCP_BYTES / SHIRES_COUNT;

/// Scratchpad lines allocated for activation outputs shire-wise (1 Mb)
static const size_t SHIRE_ACT_OUT_L2SCP_LINES = SOC_ACT_OUT_L2SCP_LINES / SHIRES_COUNT;

/// Scratchpad bytes allocated for activation prefetch shire-wise (0.5 Mb)
static const size_t SHIRE_ACT_PREF_L2SCP_BYTES = SOC_ACT_PREF_L2SCP_BYTES / SHIRES_COUNT;

/// Scratchpad lines allocated for activation prefetch shire-wise (0.5 Mb)
static const size_t SHIRE_ACT_PREF_L2SCP_LINES = SOC_ACT_PREF_L2SCP_LINES / SHIRES_COUNT;

/// Scratchpad bytes allocated for weight prefetch shire-wise (1 Mb)
static const size_t SHIRE_W_PREF_L2SCP_BYTES = SOC_W_PREF_L2SCP_BYTES / SHIRES_COUNT;

/// Scratchpad lines allocated for weight prefetch shire-wise (1 Mb)
static const size_t SHIRE_W_PREF_L2SCP_LINES = SOC_W_PREF_L2SCP_LINES / SHIRES_COUNT;

// The following should be removed
// ETSOC map
#define STACK_ADDR_START    0x8100000000
#define ELF_ADDR_START      0x8180000000
#define LAYER_ADDR_START_1  0x8201000000
#define DATA_ADDR_START     0x8210000000
#define L2SCP_ADDR_START    0x00C0000000

// FIXME remove SimulatorUtils.cpp:266:4
// Address where the host will place the kernel launch information (synced with fw_common.h)
#define RT_HOST_KERNEL_LAUNCH_INFO 0x8200000000ULL

// RAM memory region
//#define RAM_MEMORY_REGION 0x8100000000
#define RAM_MEMORY_REGION 0x8000000000

// FIXME Neuralizer/JitterData.h
//#define LAUNCH_PARAMS_AREA_BASE (RAM_MEMORY_REGION + 0x200000000) // +8G
#define LAUNCH_PARAMS_AREA_BASE LAYER_ADDR_START_1
#define LAUNCH_PARAMS_AREA_SIZE 0x10000
#define LAYER_ADDR_START LAUNCH_PARAMS_AREA_BASE + 0x28


//#define GLOBAL_MEM_REGION_BASE  (RAM_MEMORY_REGION + 0x400000000)      // +16GB
#define GLOBAL_MEM_REGION_BASE DATA_ADDR_START
//#define GLOBAL_MEM_REGION_SIZE  (512*1024*1024) // + 512MB
#define GLOBAL_MEM_REGION_SIZE  0x0200000000 // +8GB
#define EXEC_MEM_REGION_BASE    ELF_ADDR_START
#define EXEC_MEM_REGION_SIZE    (256*1024*1024) // + 256MB


#endif /* SYS_INC_H */
