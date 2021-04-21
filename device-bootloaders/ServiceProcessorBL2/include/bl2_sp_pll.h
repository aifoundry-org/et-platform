#ifndef __BL2_SP_PLL_H__
#define __BL2_SP_PLL_H__

#include <stdint.h>

#define ERROR_SP_PLL_CONFIG_DATA_NOT_FOUND -1000
#define ERROR_SP_PLL_PLL_LOCK_TIMEOUT      -1001
#define ERROR_SP_PLL_INVALID_PLL_ID        -1002

typedef enum PLL_ID_e {
    PLL_ID_INVALID,
    PLL_ID_SP_PLL_0,
    PLL_ID_SP_PLL_1,
    PLL_ID_SP_PLL_2,
    PLL_ID_SP_PLL_3,
    PLL_ID_SP_PLL_4,
    PLL_ID_PSHIRE
} PLL_ID_t;

typedef enum SP_PLL_STATE_e {
    SP_PLL_STATE_INVALID,
    SP_PLL_STATE_OFF,
    SP_PLL_STATE_50_PER_CENT,
    SP_PLL_STATE_75_PER_CENT,
    SP_PLL_STATE_100_PER_CENT
} SP_PLL_STATE_t;

uint32_t get_input_clock_index(void);
int configure_sp_pll_2(void);
int configure_sp_pll_4(void);
int configure_pcie_pll(void);
int configure_pshire_pll(const uint8_t mode);
int configure_minion_plls(void);
int get_pll_frequency(PLL_ID_t pll_id, uint32_t *frequency);
int pll_init(uint32_t sp_pll_0_frequency, uint32_t sp_pll_1_frequency,
             uint32_t pcie_pll_0_frequency);

#endif
