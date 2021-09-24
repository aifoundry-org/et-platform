/*************************************************************************
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies or
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
************************************************************************/
/*! \file io_pll.c
    \brief A C module that implements the I/O PLL configuration services. It
    provides functionality of configuring I/O PLLs.

    Public interfaces:
        get_input_clock_index
        configure_pcie_pll
        configure_pshire_pll
        configure_minion_plls
        get_pll_frequency
        pll_init
*/
/***********************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "bl_error_code.h"
#include "etsoc/isa/io.h"
#include "bl2_sp_pll.h"
#include "bl2_main.h"

#include "hwinc/sp_cru_reset.h"
#include "hwinc/sp_cru.h"
#include "hwinc/pcie_esr.h"
#include "hwinc/hal_device.h"
#include "hwinc/hpdpll_modes_config.h"

/*! \def PLL_LOCK_TIMEOUT
    \brief lock timeout for PLL
*/
#define PLL_LOCK_TIMEOUT 10000

/*! \def INPUT_CLK_CONFIG_COUNT
    \brief clock configurations
*/
#define INPUT_CLK_CONFIG_COUNT 3

/*! \def PLL_COUNT
    \brief count of PLLs
*/
#define PLL_COUNT              3

/*! \def PLL_CONFIG_COUNT
    \brief PLL configuration count
*/
#define PLL_CONFIG_COUNT       4

/*! \def PLL_REG_INDEX_REG_0
    \brief index register number
*/
#define PLL_REG_INDEX_REG_0                  0

/*! \def PLL_REG_INDEX_REG_UPDATE_STROBE
    \brief update strobe register index
*/
#define PLL_REG_INDEX_REG_UPDATE_STROBE      0x38

/*! \def PLL_REG_INDEX_REG_LOCK_DETECT_STATUS
    \brief PLL lock detect status
*/
#define PLL_REG_INDEX_REG_LOCK_DETECT_STATUS 0x39

/*! \def DCO_NORMALIZATION_ENABLE__SHIFT
    \brief DCO normalization enable shift
*/
#define DCO_NORMALIZATION_ENABLE__SHIFT      7u

/*! \def DCO_NORMALIZATION_ENABLE__MASK
    \brief DCO normalization enable mask
*/
#define DCO_NORMALIZATION_ENABLE__MASK       (1u << DCO_NORMALIZATION_ENABLE__SHIFT)

/*! \def FREQUENCY_HZ_TO_MHZ(x)
    \brief Converts HZ to MHZ
*/
#define FREQUENCY_HZ_TO_MHZ(x) ((x) / 1000000u)

static uint32_t gs_sp_pll_0_frequency;
static uint32_t gs_sp_pll_1_frequency;
static uint32_t gs_sp_pll_2_frequency;
static uint32_t gs_sp_pll_4_frequency;
static uint32_t gs_pcie_pll_0_frequency;
static uint32_t gs_maxion_pll_core_frequency;
static uint32_t gs_maxion_pll_uncore_frequency;

uint32_t get_input_clock_index(void)
{
    uint32_t rm_status2 = ioread32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_STATUS2_ADDRESS);
    return RESET_MANAGER_RM_STATUS2_STRAP_IN_GET(rm_status2);
}

static int configure_pll_off(volatile uint32_t *pll_registers)
{
    /* disable the PLL */
    pll_registers[0] = 0;

    /* set reg_update strobe to update registers */
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

static int configure_pll(volatile uint32_t *pll_registers, uint8_t mode, uint32_t* target_freq)
{
    uint32_t timeout = PLL_LOCK_TIMEOUT;
    uint8_t register_index;
    uint16_t register_value;
    uint8_t entry_index;
    uint32_t pll_settings_index;
    uint32_t reg0;
    static const uint32_t pll_settings_count =
        sizeof(gs_hpdpll_settings) / sizeof(HPDPLL_SETTING_t);

    if (0 == mode)
    {
        return 0;
    }

    for (pll_settings_index = 0; pll_settings_index < pll_settings_count; pll_settings_index++)
    {
        if (gs_hpdpll_settings[pll_settings_index].mode == mode)
        {
            goto FOUND_CONFIG_DATA;
        }
    }

    return ERROR_SP_PLL_CONFIG_DATA_NOT_FOUND;

FOUND_CONFIG_DATA:

    /* program the PLL registers using generated configuration data */
    for (entry_index = 0; entry_index < gs_hpdpll_settings[pll_settings_index].count;
         entry_index++)
    {
        register_index = gs_hpdpll_settings[pll_settings_index].offsets[entry_index];
        register_value = gs_hpdpll_settings[pll_settings_index].values[entry_index];
        pll_registers[register_index] = register_value;
    }

    *target_freq =
        FREQUENCY_HZ_TO_MHZ(gs_hpdpll_settings[pll_settings_index].output_frequency);

    /* Update PLL registers */
    update_pll_registers(pll_registers);

    /*
     POTENTIAL BUG WORKAROUND BEGIN

     Toggle the DCO_NORMALIZATION_ENABLE 1 -> 0 -> 1 in order for PLL to acquire lock
     this is required to work around potential HW bugs in Movellus PLL
    */

    /* if the DCO_NORMALIZATION_ENABLE bit is NOT 1, set it to 1 */
    reg0 = pll_registers[PLL_REG_INDEX_REG_0];
    if (!(reg0 & DCO_NORMALIZATION_ENABLE__MASK))
    {
        reg0 = reg0 | DCO_NORMALIZATION_ENABLE__MASK;
        pll_registers[PLL_REG_INDEX_REG_0] = reg0;
        update_pll_registers(pll_registers);
    }

    /* set the DCO_NORMALIZATION_ENABLE bit to 0 */
    reg0 = pll_registers[PLL_REG_INDEX_REG_0];
    reg0 = reg0 & ~DCO_NORMALIZATION_ENABLE__MASK;
    pll_registers[PLL_REG_INDEX_REG_0] = reg0;
    update_pll_registers(pll_registers);

    /* set the DCO_NORMALIZATION_ENABLE bit to 1 */
    reg0 = pll_registers[PLL_REG_INDEX_REG_0];
    reg0 = reg0 | DCO_NORMALIZATION_ENABLE__MASK;
    pll_registers[PLL_REG_INDEX_REG_0] = reg0;
    update_pll_registers(pll_registers);

    /*
     POTENTIAL BUG WORKAROUND END

     Wait for the PLL to lock within a given timeout */
    while (timeout > 0)
    {
        if (pll_registers[PLL_REG_INDEX_REG_LOCK_DETECT_STATUS] & 1)
        {
            return 0;
        }
        --timeout;
    }

    return ERROR_SP_PLL_PLL_LOCK_TIMEOUT;
}

static int clock_manager_pll_bypass(PLL_ID_t pll, bool bypass_enable)
{
    switch (pll)
    {
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
            iowrite32(R_PCIE_ESR_BASEADDR + PCIE_ESR_PSHIRE_CTRL_ADDRESS,
                    PCIE_ESR_PSHIRE_CTRL_PLL0_BYP_SET(bypass_enable ? 1 : 0));
            break;
        case PLL_ID_MAXION_CORE:
        case PLL_ID_MAXION_UNCORE:
            iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_MAX_ADDRESS,
                   CLOCK_MANAGER_CM_MAX_PLL_SEL_SET(bypass_enable? 0 : 1));
            break;
        case PLL_ID_SP_PLL_3:
        case PLL_ID_INVALID:
        default:
            return ERROR_SP_PLL_INVALID_PLL_ID;
    }

    return 0;
}

int configure_sp_pll_0(uint8_t mode)
{
    int rv;

    rv = clock_manager_pll_bypass(PLL_ID_SP_PLL_0, true);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = configure_pll((uint32_t *)R_SP_PLL0_BASEADDR, mode, &gs_sp_pll_0_frequency);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = clock_manager_pll_bypass(PLL_ID_SP_PLL_0, false);
    if (0 != rv)
    {
        goto ERROR;
    }

    return 0;

ERROR:
    clock_manager_pll_bypass(PLL_ID_SP_PLL_0, true);
    configure_pll_off((uint32_t *)R_SP_PLL0_BASEADDR);
    gs_sp_pll_0_frequency = 100;

    return rv;
}


int configure_sp_pll_1(uint8_t mode)
{
    int rv;

    rv = clock_manager_pll_bypass(PLL_ID_SP_PLL_1, true);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = configure_pll((uint32_t *)R_SP_PLL1_BASEADDR, mode, &gs_sp_pll_1_frequency);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = clock_manager_pll_bypass(PLL_ID_SP_PLL_1, false);
    if (0 != rv)
    {
        goto ERROR;
    }

    return 0;

ERROR:
    clock_manager_pll_bypass(PLL_ID_SP_PLL_1, true);
    configure_pll_off((uint32_t *)R_SP_PLL1_BASEADDR);
    gs_sp_pll_0_frequency = 100;

    return rv;
}


int configure_sp_pll_2(uint8_t mode)
{
    int rv;

    rv = clock_manager_pll_bypass(PLL_ID_SP_PLL_2, true);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = configure_pll((uint32_t *)R_SP_PLL2_BASEADDR, mode, &gs_sp_pll_2_frequency);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = clock_manager_pll_bypass(PLL_ID_SP_PLL_2, false);
    if (0 != rv)
    {
        goto ERROR;
    }

    return 0;

ERROR:
    clock_manager_pll_bypass(PLL_ID_SP_PLL_2, true);
    configure_pll_off((uint32_t *)R_SP_PLL2_BASEADDR);
    gs_sp_pll_2_frequency = 100;

    return rv;
}

int configure_sp_pll_4(uint8_t mode)
{
    int rv;

    rv = clock_manager_pll_bypass(PLL_ID_SP_PLL_4, true);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = configure_pll((uint32_t *)R_SP_PLL4_BASEADDR, mode, &gs_sp_pll_4_frequency);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = clock_manager_pll_bypass(PLL_ID_SP_PLL_4, false);
    if (0 != rv)
    {
        goto ERROR;
    }

    return 0;

ERROR:
    clock_manager_pll_bypass(PLL_ID_SP_PLL_4, true);
    configure_pll_off((uint32_t *)R_SP_PLL4_BASEADDR);
    gs_sp_pll_4_frequency = 100;

    return rv;
}

int configure_pshire_pll(const uint8_t mode)
{

    int rv;

    rv = clock_manager_pll_bypass(PLL_ID_PSHIRE, true);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = configure_pll((uint32_t *)R_PCIE_PLLP0_BASEADDR, mode, &gs_pcie_pll_0_frequency);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = clock_manager_pll_bypass(PLL_ID_PSHIRE, false);
    if (0 != rv)
    {
        goto ERROR;
    }

    return 0;

ERROR:
    clock_manager_pll_bypass(PLL_ID_PSHIRE, true);
    configure_pll_off((uint32_t *)R_PCIE_PLLP0_BASEADDR);
    gs_pcie_pll_0_frequency = 100;

    return rv;
}

int configure_maxion_pll_core(uint8_t mode)
{
    int rv;

    rv = clock_manager_pll_bypass(PLL_ID_MAXION_CORE, true);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = configure_pll((uint32_t *)R_SP_PLLMX0_BASEADDR, mode, &gs_maxion_pll_core_frequency);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = clock_manager_pll_bypass(PLL_ID_MAXION_CORE, false);
    if (0 != rv)
    {
        goto ERROR;
    }

    return 0;

ERROR:
    clock_manager_pll_bypass(PLL_ID_MAXION_CORE, true);
    configure_pll_off((uint32_t *)R_SP_PLLMX0_BASEADDR);
    gs_maxion_pll_core_frequency=100;

    return rv;
}

int configure_maxion_pll_uncore(uint8_t mode)
{
    int rv;

    rv = clock_manager_pll_bypass(PLL_ID_MAXION_UNCORE, true);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = configure_pll((uint32_t *)R_SP_PLLMX1_BASEADDR, mode, &gs_maxion_pll_uncore_frequency);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = clock_manager_pll_bypass(PLL_ID_MAXION_UNCORE, false);
    if (0 != rv)
    {
        goto ERROR;
    }

    return 0;

ERROR:
    clock_manager_pll_bypass(PLL_ID_MAXION_UNCORE, true);
    configure_pll_off((uint32_t *)R_SP_PLLMX1_BASEADDR);
    gs_maxion_pll_uncore_frequency=100;

    return rv;
}

int get_pll_frequency(PLL_ID_t pll_id, uint32_t *frequency)
{
    if (NULL == frequency)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    switch (pll_id)
    {
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
        case PLL_ID_MAXION_CORE:
            *frequency = gs_maxion_pll_core_frequency;
            return 0;
        case PLL_ID_MAXION_UNCORE:
            *frequency = gs_maxion_pll_uncore_frequency;
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
