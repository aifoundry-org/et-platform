#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "bl2_sp_pll.h"

#include "bl2_main.h"

#include "etsoc_hal/mvls_tn7_hpdpll.ipxact.h"
#include "etsoc_hal/rm_esr.h"
#include "etsoc_hal/cm_esr.h"
#include "etsoc_hal/pshire_esr.h"
#include "hal_device.h"

#include "movellus_pll_modes_config.h"

#define PLL_LOCK_TIMEOUT 10000

#define INPUT_CLK_CONFIG_COUNT 3
#define PLL_COUNT 3
#define PLL_CONFIG_COUNT 4

#define PLL_REG_INDEX_REG_UPDATE_STROBE 0x38
#define PLL_REG_INDEX_REG_LOCK_DETECT_STATUS 0x39

static uint32_t gs_sp_pll_0_frequency;
static uint32_t gs_sp_pll_1_frequency;
static uint32_t gs_sp_pll_2_frequency;
static uint32_t gs_sp_pll_4_frequency;
static uint32_t gs_pcie_pll_0_frequency;

static int release_pshire_from_reset(void) {
    volatile Reset_Manager_t * reset_manager = (Reset_Manager_t*)R_SP_CRU_BASEADDR;
    reset_manager->rm_pshire_cold.R = (Reset_Manager_rm_pshire_cold_t){ .B = { .rstn = 1 }}.R;
    reset_manager->rm_pshire_warm.R = (Reset_Manager_rm_pshire_warm_t){ .B = { .rstn = 1 }}.R;
    return 0;
}

uint32_t get_input_clock_index(void) {
    volatile Reset_Manager_t * reset_manager = (Reset_Manager_t*)R_SP_CRU_BASEADDR;
    return reset_manager->rm_status2.B.strap_in;
}

static int configure_pll_off(volatile uint32_t * pll_registers) {
    // disable the PLL
    pll_registers[0] = 0;

    // set reg_update strobe to update registers
    pll_registers[PLL_REG_INDEX_REG_UPDATE_STROBE] = 1;

    return 0;
}

static int configure_pll(volatile uint32_t * pll_registers, uint8_t mode, PLL_ID_t pll_id) {
    uint32_t timeout = PLL_LOCK_TIMEOUT;
    uint8_t register_index;
    uint16_t register_value;
    uint8_t entry_index;
    uint32_t pll_settings_index;
    static const uint32_t pll_settings_count = sizeof(gs_pll_settings) / sizeof(PLL_SETTING_t);

    if (0 == mode) {
        return 0;
    }

    for (pll_settings_index = 0; pll_settings_index < pll_settings_count; pll_settings_index++) {
        if (gs_pll_settings[pll_settings_index].mode == mode) {
            goto FOUND_CONFIG_DATA;
        }
    }

    return -1;

FOUND_CONFIG_DATA:

    if (PLL_ID_PSHIRE == pll_id) {
        if (0 != release_pshire_from_reset()) {
            return -1;
        }
    }

    for (entry_index = 0; entry_index < gs_pll_settings[pll_settings_index].count; entry_index++) {
        register_index = gs_pll_settings[pll_settings_index].offsets[entry_index];
        register_value = gs_pll_settings[pll_settings_index].values[entry_index];

        switch (register_index) {
        case PLL_REG_INDEX_REG_UPDATE_STROBE:
        case PLL_REG_INDEX_REG_LOCK_DETECT_STATUS:
            break;

        default:
            pll_registers[register_index] = register_value;
        }
    }

    // set reg_update strobe to update registers
    pll_registers[PLL_REG_INDEX_REG_UPDATE_STROBE] = 1;

    // wait for the PLL to lock
    while (timeout > 0) {
        if (pll_registers[PLL_REG_INDEX_REG_LOCK_DETECT_STATUS] & 1) {
            return 0;
        }
    }
    return -2;
}

static int clock_manager_pll_bypass(PLL_ID_t pll, bool bypass_enable) {
    volatile Clock_Manager_t * sp_clock_manager = (volatile Clock_Manager_t *)R_SP_CRU_BASEADDR;
    volatile Pshire_t * pshire_manager = (volatile Pshire_t *)R_PCIE_ESR_BASEADDR;

    switch (pll) {
    case PLL_ID_SP_PLL_0:
        sp_clock_manager->cm_ios_ctrl.R = ((Clock_Manager_cm_ios_ctrl_t){.B = { .mission = bypass_enable ? 0 : 1 }}).R;
        break;
    case PLL_ID_SP_PLL_1:
        sp_clock_manager->cm_pll1_ctrl.R = ((Clock_Manager_cm_pll1_ctrl_t){.B = { .enable = bypass_enable ? 0 : 1 }}).R;
        break;
    case PLL_ID_SP_PLL_2:
        sp_clock_manager->cm_pll2_ctrl.R = ((Clock_Manager_cm_pll2_ctrl_t){.B = { .enable = bypass_enable ? 0 : 1 }}).R;
        break;
    case PLL_ID_SP_PLL_4:
        sp_clock_manager->cm_pll4_ctrl.R = ((Clock_Manager_cm_pll4_ctrl_t){.B = { .enable = bypass_enable ? 0 : 1 }}).R;
        break;
    case PLL_ID_PSHIRE:
        pshire_manager->pshire_ctrl.R = ((Pshire_pshire_ctrl_t){.B = { .pll0_byp = bypass_enable ? 1 : 0 }}).R;
        break;

    case PLL_ID_SP_PLL_3:
    case PLL_ID_INVALID:
    default:
        return -1;
    }

    return 0;
}

static int configure_sp_pll(PLL_ID_t pll_id, volatile uint32_t * pll_registers, const uint8_t mode[INPUT_CLK_CONFIG_COUNT]) {
    int rv;

    rv = configure_pll(pll_registers, mode[get_input_clock_index()], pll_id);
    if (0 != rv) {
        rv = -1;
        goto ERROR;
    }

    if (0 != clock_manager_pll_bypass(pll_id, false)) {
        rv = -2;
        goto ERROR;
    }

    return 0;

ERROR:
    clock_manager_pll_bypass(pll_id, true);
    configure_pll_off(pll_registers);

    return rv;
}

int configure_sp_pll_2(void) {
    int rv;
    static const uint8_t mode[INPUT_CLK_CONFIG_COUNT] = { 5, 11, 17 };

    rv = configure_sp_pll(PLL_ID_SP_PLL_2, (uint32_t*)R_SP_PLL2_BASEADDR, mode);
    if (0 == rv) {
        gs_sp_pll_2_frequency = 500;
    } else {
        gs_sp_pll_2_frequency = 0;
    }

    return rv;
}

int configure_sp_pll_4(void) {
    int rv;
    static const uint8_t mode[INPUT_CLK_CONFIG_COUNT] = { 3, 9, 15 };

    rv = configure_sp_pll(PLL_ID_SP_PLL_4, (uint32_t*)R_SP_PLL4_BASEADDR, mode);
    if (0 == rv) {
        gs_sp_pll_2_frequency = 1000;
    } else {
        gs_sp_pll_2_frequency = 0;
    }

    return rv;
}

int get_pll_frequency(PLL_ID_t pll_id, uint32_t * frequency) {
    if (NULL == frequency) {
        return -1;
    }

    switch (pll_id) {
    case PLL_ID_SP_PLL_0:
        *frequency = gs_sp_pll_0_frequency;
        return 0;
    case PLL_ID_SP_PLL_1:
        *frequency = gs_sp_pll_1_frequency;
        return 0;
    case PLL_ID_SP_PLL_2:
        *frequency = gs_sp_pll_2_frequency;
        return 0;
    case PLL_ID_SP_PLL_4:
        *frequency = gs_sp_pll_4_frequency;
        return 0;
    case PLL_ID_PSHIRE:
        *frequency = gs_pcie_pll_0_frequency;
        return 0;
    case PLL_ID_SP_PLL_3:
    case PLL_ID_INVALID:
    default:
        return -1;
    }
}

int pll_init(uint32_t sp_pll_0_frequency, uint32_t sp_pll_1_frequency, uint32_t pcie_pll_0_frequency) {
    gs_sp_pll_0_frequency = sp_pll_0_frequency;
    gs_sp_pll_1_frequency = sp_pll_1_frequency;
    gs_sp_pll_2_frequency = 0;
    gs_sp_pll_4_frequency = 0;
    gs_pcie_pll_0_frequency = pcie_pll_0_frequency;
    return 0;
}
