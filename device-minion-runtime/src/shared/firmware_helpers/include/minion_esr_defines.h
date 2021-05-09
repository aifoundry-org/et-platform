/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

typedef enum { PP_USER = 0, PP_SUPERVISOR = 1, PP_MESSAGES = 2, PP_MACHINE = 3 } esr_protection_t;

typedef enum {
    REGION_MINION = 0, // HART ESR
    REGION_NEIGHBOURHOOD = 1, // Neighbor ESR
    REGION_TBOX = 2, //
    REGION_OTHER = 3 // Shire Cache ESR and Shire Other ESR
} esr_regions_t;

const uint64_t ESR_MEMORY_REGION = 0x0100000000UL; // [32]=1

static inline volatile uint64_t *__attribute__((always_inline))
esr_address(esr_protection_t pp, uint8_t shire_id, esr_regions_t region, uint32_t address)
{
    volatile uint64_t *p =
        (uint64_t *)(ESR_MEMORY_REGION | ((uint64_t)(pp & 0x03) << 30) |
                     ((uint64_t)(shire_id & 0xff) << 22) | ((uint64_t)(region & 0x03) << 20) |
                     ((uint64_t)(address & 0x01ffff) << 3));
    return p;
}

static inline uint64_t __attribute__((always_inline))
read_esr(esr_protection_t pp, uint8_t shire_id, esr_regions_t region, uint32_t address)
{
    volatile uint64_t *p = esr_address(pp, shire_id, region, address);
    return *p;
}

static inline void __attribute__((always_inline))
write_esr(esr_protection_t pp, uint8_t shire_id, esr_regions_t region, uint32_t address,
          uint64_t value)
{
    volatile uint64_t *p = esr_address(pp, shire_id, region, address);
    *p = value;
}

// Virtual Master Minion Shire NOC ID
#define MM_SHIRE_ID 32

// Virtual Master Minion Shire NOC ID Mask
#define MM_SHIRE_ID_MASK 0x100000000U

// Virtual Compute Shire NOC ID Mask
#define CM_SHIRE_ID_MASK 0xFFFFFFFFU

// Define for All Threads available in a Minion Shire
#define MM_ALL_THREADS 0xFFFFFFFFU

// Define for Threads which participate in the Device Runtime FW management.
// Currently only lower 16 Minions (64 Harts) of the whole Minion Shire which
// participate in the Device Runtime. This might change with Virtual Queue
// implementation
#define MM_RT_THREADS 0x0000FFFFU

// Define for Threads which participate in the Device Runtime FW management.
// Currently the upper 16 Minions (64 Harts) of the whole Minion Shire
// participates in Compute Minion Kernel execution.
#define MM_COMPUTE_THREADS 0xFFFF0000U

/*typedef struct packed {
   logic [1:0] tbox3_id;
   logic [1:0] tbox2_id;
   logic [1:0] tbox1_id;
   logic [1:0] tbox0_id;
   logic rbox_en;
   logic [3:0] tbox_en;
   logic [3:0] neigh_en;
   logic cache_en;
   logic [7:0] shire_id;
} esr_shire_config_t;*/
/* ESR values */
#define ESR_SHIRE_CONFIG_32_EN    0x001f20
#define ESR_SHIRE_CONFIG_32_REMAP 0x001f00
#define ESR_SHIRE_CONFIG_32_DIS   0x000120
#define ESR_SHIRE_CONFIG_0_EN     0x001f00
#define ESR_SHIRE_CONFIG_0_REMAP  0x001f20
#define ESR_SHIRE_CONFIG_0_DIS    0x000100
#define ESR_SHIRE_CONFIG_EN       0x000100
/* ESR addresses: REGION_NEIGH */
#define MINION_BOOT 0x1e003
/* ESR addresses: REGION_OTHER */
#define SHIRE_OTHER_CONFIG            0x08001
#define SHIRE_PLL_READ_DATA           0x08051
#define SHIRE_DLL_READ_DATA           0x0805c
#define SHIRE_OTHER_THREAD0_DISABLE   0x08048
#define SHIRE_OTHER_THREAD1_DISABLE   0x08002
#define SHIRE_OTHER_CTRL_CLOCKMUX     0x08053
#define SHIRE_OTHER_PLL_AUTO_CONFIG   0x0804a
#define SHIRE_OTHER_DLL_AUTO_CONFIG   0x08059
#define SHIRE_OTHER_PLL_CONFIG_DATA_0 0x0804b
#define SHIRE_OTHER_PLL_CONFIG_DATA_1 0x0804c
#define SHIRE_OTHER_PLL_CONFIG_DATA_2 0x0804d
#define SHIRE_OTHER_PLL_CONFIG_DATA_3 0x0804e
#define SHIRE_OTHER_DLL_CONFIG_DATA_0 0x0805a
#define SHIRE_OTHER_DLL_CONFIG_DATA_1 0x0805b

#define SHIRE_OTHER_PLL_AUTO_CONFIG_PCLK_SEL_OFF  15
#define SHIRE_OTHER_PLL_AUTO_CONFIG_LOCK_RST_OFF  14
#define SHIRE_OTHER_PLL_AUTO_CONFIG_REG_NUM_OFF   10
#define SHIRE_OTHER_PLL_AUTO_CONFIG_REG_FIRST_OFF 4
#define SHIRE_OTHER_PLL_AUTO_CONFIG_WRITE_OFF     3
#define SHIRE_OTHER_PLL_AUTO_CONFIG_RUN_OFF       2
#define SHIRE_OTHER_PLL_AUTO_CONFIG_ENABLE_OFF    1
#define SHIRE_OTHER_PLL_AUTO_CONFIG_RESET_OFF     0

#define SHIRE_OTHER_DLL_AUTO_CONFIG_DLL_EN_OFF    13
#define SHIRE_OTHER_DLL_AUTO_CONFIG_PCLK_SEL_OFF  11
#define SHIRE_OTHER_DLL_AUTO_CONFIG_LOCK_RST_OFF  10
#define SHIRE_OTHER_DLL_AUTO_CONFIG_REG_NUM_OFF   7
#define SHIRE_OTHER_DLL_AUTO_CONFIG_REG_FIRST_OFF 4
#define SHIRE_OTHER_DLL_AUTO_CONFIG_WRITE_OFF     3
#define SHIRE_OTHER_DLL_AUTO_CONFIG_RUN_OFF       2
#define SHIRE_OTHER_DLL_AUTO_CONFIG_ENABLE_OFF    1
#define SHIRE_OTHER_DLL_AUTO_CONFIG_RESET_OFF     0

#define SHIRE_OTHER_PLL_AUTO_CONFIG_PCLK_SEL (2 << SHIRE_OTHER_PLL_AUTO_CONFIG_PCLK_SEL_OFF)
#define SHIRE_OTHER_DLL_AUTO_CONFIG_PCLK_SEL (2 << SHIRE_OTHER_DLL_AUTO_CONFIG_PCLK_SEL_OFF)

/*typedef struct packed {
      logic [1:0] pclk_sel = 0
      logic lock_reset_disable = 0
      logic [3:0] reg_num = 0x16
      logic [5:0] reg_first = 00
      logic write = 0
      logic run = 0
      logic enable = 0
      logic reset = 1
} esr_pll_auto_config_t;*/

/*typedef struct packed {
      logic dll_enable;
      logic [1:0] pclk_sel;
      logic lock_reset_disable;
      logic [2:0] reg_last;
      logic [2:0] reg_first;
      logic write;
      logic run;
      logic enable;
      logic reset;
   } esr_dll_auto_config_t;
*/
