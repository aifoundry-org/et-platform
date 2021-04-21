#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "error.h"
#include "io.h"
#include "bl2_sp_pll.h"
#include "bl2_main.h"

#include "etsoc_hal/inc/rm_esr.h"
#include "etsoc_hal/inc/cm_esr.h"
#include "etsoc_hal/inc/pshire_esr.h"
#include "etsoc_hal/inc/hal_device.h"

#include "etsoc_hal/inc/movellus_hpdpll_modes_config.h"

#define PLL_LOCK_TIMEOUT 10000

#define INPUT_CLK_CONFIG_COUNT 3
#define PLL_COUNT              3
#define PLL_CONFIG_COUNT       4

#define PLL_REG_INDEX_REG_0                  0
#define PLL_REG_INDEX_REG_UPDATE_STROBE      0x38
#define PLL_REG_INDEX_REG_LOCK_DETECT_STATUS 0x39
#define DCO_NORMALIZATION_ENABLE__SHIFT      7u
#define DCO_NORMALIZATION_ENABLE__MASK       (1u << DCO_NORMALIZATION_ENABLE__SHIFT)

static uint32_t gs_sp_pll_0_frequency;
static uint32_t gs_sp_pll_1_frequency;
static uint32_t gs_sp_pll_2_frequency;
static uint32_t gs_sp_pll_4_frequency;
static uint32_t gs_pcie_pll_0_frequency;

uint32_t get_input_clock_index(void)
{
    uint32_t rm_status2 = ioread32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_STATUS2_ADDRESS);
    return RESET_MANAGER_RM_STATUS2_STRAP_IN_GET(rm_status2);
}

static int configure_pll_off(volatile uint32_t *pll_registers)
{
    // disable the PLL
    pll_registers[0] = 0;

    // set reg_update strobe to update registers
    pll_registers[PLL_REG_INDEX_REG_UPDATE_STROBE] = 1;

    return 0;
}

static void update_pll_registers(volatile uint32_t *pll_registers)
{
    uint32_t strobe;

    strobe = pll_registers[PLL_REG_INDEX_REG_UPDATE_STROBE];
    strobe = strobe & 0xFFFFFFFE;
    pll_registers[PLL_REG_INDEX_REG_UPDATE_STROBE] = strobe;

    strobe = pll_registers[PLL_REG_INDEX_REG_UPDATE_STROBE];
    strobe = strobe | 1;
    pll_registers[PLL_REG_INDEX_REG_UPDATE_STROBE] = strobe;
}

static int configure_pll(volatile uint32_t *pll_registers, uint8_t mode)
{
    uint32_t timeout = PLL_LOCK_TIMEOUT;
    uint8_t register_index;
    uint16_t register_value;
    uint8_t entry_index;
    uint32_t pll_settings_index;
    uint32_t reg0;
    static const uint32_t pll_settings_count =
        sizeof(gs_hpdpll_settings) / sizeof(HPDPLL_SETTING_t);

    if (0 == mode) {
        return 0;
    }

    for (pll_settings_index = 0; pll_settings_index < pll_settings_count; pll_settings_index++) {
        if (gs_hpdpll_settings[pll_settings_index].mode == mode) {
            goto FOUND_CONFIG_DATA;
        }
    }

    return ERROR_SP_PLL_CONFIG_DATA_NOT_FOUND;

FOUND_CONFIG_DATA:

    // program the PLL registers using generated configuration data
    for (entry_index = 0; entry_index < gs_hpdpll_settings[pll_settings_index].count;
         entry_index++) {
        register_index = gs_hpdpll_settings[pll_settings_index].offsets[entry_index];
        register_value = gs_hpdpll_settings[pll_settings_index].values[entry_index];
        pll_registers[register_index] = register_value;
    }

    // Update PLL registers
    update_pll_registers(pll_registers);

    ///////////////////////////////////////////////////////////////////////////////////////
    // POTENTIAL BUG WORKAROUND BEGIN
    ///////////////////////////////////////////////////////////////////////////////////////

    // Toggle the DCO_NORMALIZATION_ENABLE 1 -> 0 -> 1 in order for PLL to acquire lock
    // this is required to work around potential HW bugs in Movellus PLL

    // if the DCO_NORMALIZATION_ENABLE bit is NOT 1, set it to 1
    reg0 = pll_registers[PLL_REG_INDEX_REG_0];
    if (!(reg0 & DCO_NORMALIZATION_ENABLE__MASK)) {
        reg0 = reg0 | DCO_NORMALIZATION_ENABLE__MASK;
        pll_registers[PLL_REG_INDEX_REG_0] = reg0;
        update_pll_registers(pll_registers);
    }

    // set the DCO_NORMALIZATION_ENABLE bit to 0
    reg0 = pll_registers[PLL_REG_INDEX_REG_0];
    reg0 = reg0 & ~DCO_NORMALIZATION_ENABLE__MASK;
    pll_registers[PLL_REG_INDEX_REG_0] = reg0;
    update_pll_registers(pll_registers);

    // set the DCO_NORMALIZATION_ENABLE bit to 1
    reg0 = pll_registers[PLL_REG_INDEX_REG_0];
    reg0 = reg0 | DCO_NORMALIZATION_ENABLE__MASK;
    pll_registers[PLL_REG_INDEX_REG_0] = reg0;
    update_pll_registers(pll_registers);

    ///////////////////////////////////////////////////////////////////////////////////////
    // POTENTIAL BUG WORKAROUND END
    ///////////////////////////////////////////////////////////////////////////////////////

    // Wait for the PLL to lock within a given timeout
    while (timeout > 0) {
        if (pll_registers[PLL_REG_INDEX_REG_LOCK_DETECT_STATUS] & 1) {
            return 0;
        }
        --timeout;
    }

    return ERROR_SP_PLL_PLL_LOCK_TIMEOUT;
}

static int clock_manager_pll_bypass(PLL_ID_t pll, bool bypass_enable)
{
    switch (pll) {
    case PLL_ID_SP_PLL_0:
        iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_IOS_CTRL_ADDRESS,
                  CLOCK_MANAGER_CM_IOS_CTRL_MISSION_SET(bypass_enable ? 0 : 1));
        break;
    case PLL_ID_SP_PLL_1:
        iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL1_CTRL_ADDRESS,
                  CLOCK_MANAGER_CM_PLL1_CTRL_ENABLE_SET(bypass_enable ? 0 : 1));
        break;
    case PLL_ID_SP_PLL_2:
        iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL2_CTRL_ADDRESS,
                  CLOCK_MANAGER_CM_PLL2_CTRL_ENABLE_SET(bypass_enable ? 0 : 1));
        break;
    case PLL_ID_SP_PLL_4:
        iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL4_CTRL_ADDRESS,
                  CLOCK_MANAGER_CM_PLL4_CTRL_ENABLE_SET(bypass_enable ? 0 : 1));
        break;
    case PLL_ID_PSHIRE:
        iowrite32(R_PCIE_ESR_BASEADDR + PSHIRE_PSHIRE_CTRL_ADDRESS,
                  PSHIRE_PSHIRE_CTRL_PLL0_BYP_SET(bypass_enable ? 1 : 0));
        break;

    case PLL_ID_SP_PLL_3:
    case PLL_ID_INVALID:
    default:
        return ERROR_SP_PLL_INVALID_PLL_ID;
    }

    return 0;
}

static int configure_sp_pll(PLL_ID_t pll_id, volatile uint32_t *pll_registers,
                            const uint8_t mode[INPUT_CLK_CONFIG_COUNT])
{
    int rv;

    rv = configure_pll(pll_registers, mode[get_input_clock_index()]);
    if (0 != rv) {
        goto ERROR;
    }

    if (0 != clock_manager_pll_bypass(pll_id, false)) {
        goto ERROR;
    }

    return 0;

ERROR:
    clock_manager_pll_bypass(pll_id, true);
    configure_pll_off(pll_registers);

    return rv;
}

int configure_sp_pll_2(void)
{
    int rv;
    static const uint8_t mode[INPUT_CLK_CONFIG_COUNT] = { 5, 11, 17 };

    rv = configure_sp_pll(PLL_ID_SP_PLL_2, (uint32_t *)R_SP_PLL2_BASEADDR, mode);
    if (0 == rv) {
        gs_sp_pll_2_frequency = 500;
    } else {
        gs_sp_pll_2_frequency = 0;
    }

    return rv;
}

int configure_sp_pll_4(void)
{
    int rv;
    static const uint8_t mode[INPUT_CLK_CONFIG_COUNT] = { 3, 9, 15 };

    rv = configure_sp_pll(PLL_ID_SP_PLL_4, (uint32_t *)R_SP_PLL4_BASEADDR, mode);
    if (0 == rv) {
        gs_sp_pll_2_frequency = 1000;
    } else {
        gs_sp_pll_2_frequency = 0;
    }

    return rv;
}

int configure_pcie_pll(void)
{
    int rv;
    static const uint8_t mode[INPUT_CLK_CONFIG_COUNT] = { 6, 12, 18 };

    rv = configure_sp_pll(PLL_ID_PSHIRE, (uint32_t *)R_PCIE_PLLP0_BASEADDR, mode);
    if (0 == rv) {
        gs_pcie_pll_0_frequency = 1010;
    } else {
        gs_pcie_pll_0_frequency = 0;
    }

    return rv;
}

int configure_pshire_pll(const uint8_t mode)
{

    int rv;

    rv = configure_pll((uint32_t *)R_PCIE_PLLP0_BASEADDR, mode);
    if (0 != rv) {
        goto ERROR;
    }

    if (0 != clock_manager_pll_bypass(PLL_ID_PSHIRE, false)) {
        goto ERROR;
    }

    if (0 == rv) {
        gs_pcie_pll_0_frequency = 1010;
    } else {
        gs_pcie_pll_0_frequency = 0;
    }

    return 0;

ERROR:
    clock_manager_pll_bypass(PLL_ID_PSHIRE, true);
    configure_pll_off((uint32_t *)R_PCIE_PLLP0_BASEADDR);

    return rv;
}

int get_pll_frequency(PLL_ID_t pll_id, uint32_t *frequency)
{
    if (NULL == frequency) {
        return ERROR_INVALID_ARGUMENT;
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
        return ERROR_SP_PLL_INVALID_PLL_ID;
    }
}

int pll_init(uint32_t sp_pll_0_frequency, uint32_t sp_pll_1_frequency,
             uint32_t pcie_pll_0_frequency)
{
    gs_sp_pll_0_frequency = sp_pll_0_frequency;
    gs_sp_pll_1_frequency = sp_pll_1_frequency;
    gs_sp_pll_2_frequency = 0;
    gs_sp_pll_4_frequency = 0;
    gs_pcie_pll_0_frequency = pcie_pll_0_frequency;
    return 0;
}
