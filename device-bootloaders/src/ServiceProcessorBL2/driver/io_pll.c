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
#include "interrupt.h"
#include "usdelay.h"

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

/*! \def PLL_REG_INDEX_REG_LOCK_THRESHOLD
    \brief lock threshold
*/
#define PLL_REG_INDEX_REG_LOCK_THRESHOLD               0x0B

/*! \def PLL_REG_INDEX_REG_LDO_CONTROL
    \brief LDO control
*/
#define PLL_REG_INDEX_REG_LDO_CONTROL                  0x18

/*! \def PLL_REG_INDEX_REG_LOCK_MONITOR_CONTROL
    \brief Lock monitor sample strobe and clear
*/
#define PLL_REG_INDEX_REG_LOCK_MONITOR_CONTROL      0x19

/*! \def PLL_REG_INDEX_REG_FREQ_MONITOR_CONTROL
    \brief Freq montior control
*/
#define PLL_REG_INDEX_REG_FREQ_MONITOR_CONTROL         0x23

/*! \def PLL_REG_INDEX_REG_FREQ_MONITOR_CLK_REF_COUNT
    \brief Freq montior clock ref count
*/
#define PLL_REG_INDEX_REG_FREQ_MONITOR_CLK_REF_COUNT   0x24

/*! \def PLL_REG_INDEX_REG_FREQ_MONITOR_CLK_COUNT_0
    \brief Freq montior clock count 0
*/
#define PLL_REG_INDEX_REG_FREQ_MONITOR_CLK_COUNT_0     0x25

/*! \def PLL_REG_INDEX_REG_LOCK_MONITOR
    \brief Lock montior
*/
#define PLL_REG_INDEX_REG_LOCK_MONITOR              0x30

/*! \def PLL_LOCK_MONITOR_MASK
    \brief Lock montior mask
*/
#define PLL_LOCK_MONITOR_MASK                       0x3F

/*! \def PLL_REG_INDEX_REG_DCO_CODE_LOOP_FILTER
    \brief DCO code aquired
*/
#define PLL_REG_INDEX_REG_DCO_CODE_LOOP_FILTER         0x32

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

/*! \def PLL_ENABLE__SHIFT
    \brief PLL enable shift
*/
#define PLL_ENABLE__SHIFT      3u

/*! \def PLL_ENABLE__MASK
    \brief PLL enable mask
*/
#define PLL_ENABLE__MASK       (1u << PLL_ENABLE__SHIFT)

/*! \def FREQUENCY_HZ_TO_MHZ(x)
    \brief Converts HZ to MHZ
*/
#define FREQUENCY_HZ_TO_MHZ(x) ((x) / 1000000u)

/*! \def PLL_LOCK_INT_DIS_LOST_INT_EN(PLL_ID)
    \brief Disables lock interrupt, switch to PLL clock, clears status
*          and enables loss interrupt
*/
#define PLL_LOCK_INT_DIS_LOST_INT_EN(PLL_ID)                 \
    clock_manager_set_pll_lock_int_enable(PLL_ID, false);    \
    clock_manager_pll_bypass(PLL_ID, false);                 \
    clock_manager_clear_pll_status(PLL_ID);                  \
    clock_manager_set_pll_loss_int_enable(PLL_ID, true);

/*! \def PLL_LOSS_INT_DIS_LOCK_INT_EN(PLL_ID)
    \brief Disables loss interrupt, switch to ref clock, clears status
*          and enables lock interrupt
*/
#define PLL_LOSS_INT_DIS_LOCK_INT_EN(PLL_ID)                 \
    clock_manager_set_pll_loss_int_enable(PLL_ID, false);    \
    clock_manager_pll_bypass(PLL_ID, true);                  \
    clock_manager_clear_pll_status(PLL_ID);                  \
    clock_manager_set_pll_lock_int_enable(PLL_ID, true);

/*! \def CHECK_AND_HANDLE_PLL_STATUS(PLL_ID, REG_MACRO)
    \brief Check and handle PLL status
*/
#define CHECK_AND_HANDLE_PLL_STATUS(PLL_ID, REG_MACRO, PLL_LOSS_COUNTER)     \
    clock_manager_get_pll_status(PLL_ID, &cm_pll_status);                             \
    if ( REG_MACRO(cm_pll_status) )                                                   \
    {                                                                                 \
        PLL_LOSS_INT_DIS_LOCK_INT_EN(PLL_ID)                                          \
        PLL_LOSS_COUNTER++;                                                                \
    }                                                                                 \
    else                                                                              \
    {                                                                                 \
        PLL_LOCK_INT_DIS_LOST_INT_EN(PLL_ID)                                          \
    }

static uint32_t gs_sp_pll_0_frequency;
static uint32_t gs_sp_pll_1_frequency;
static uint32_t gs_sp_pll_2_frequency;
static uint32_t gs_sp_pll_4_frequency;
static uint32_t gs_pcie_pll_0_frequency;
static uint32_t gs_maxion_pll_core_frequency;
static uint32_t gs_maxion_pll_uncore_frequency;
static uint32_t cm_pll_0_lock_loss_count = 0;
static uint32_t cm_pll_1_lock_loss_count = 0;
static uint32_t cm_pll_2_lock_loss_count = 0;
static uint32_t cm_pll_4_lock_loss_count = 0;

uint32_t get_input_clock_index(void)
{
    uint32_t rm_status2 = ioread32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_STATUS2_ADDRESS);
    return RESET_MANAGER_RM_STATUS2_STRAP_IN_GET(rm_status2);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wswitch-enum"

static int clock_manager_set_pll_lock_int_enable(PLL_ID_t pll, bool inth_enable)
{
    uint32_t reg_value;

    switch (pll)
    {
        case PLL_ID_SP_PLL_0:
            reg_value = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL0_CTRL_ADDRESS);
            reg_value = (uint32_t)CLOCK_MANAGER_CM_PLL0_CTRL_LOCKINTR_EN_MODIFY(reg_value, 
                                                                        (inth_enable ? 1 : 0));
            iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL0_CTRL_ADDRESS, reg_value);
            break;
        case PLL_ID_SP_PLL_1:
            reg_value = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL1_CTRL_ADDRESS);
            reg_value = (uint32_t)CLOCK_MANAGER_CM_PLL1_CTRL_LOCKINTR_EN_MODIFY(reg_value, 
                                                                        (inth_enable ? 1 : 0));
            iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL1_CTRL_ADDRESS, reg_value);
            break;
        case PLL_ID_SP_PLL_2:
            reg_value = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL2_CTRL_ADDRESS);
            reg_value = (uint32_t)CLOCK_MANAGER_CM_PLL2_CTRL_LOCKINTR_EN_MODIFY(reg_value, 
                                                                        (inth_enable ? 1 : 0));
            iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL2_CTRL_ADDRESS, reg_value);
            break;
        case PLL_ID_SP_PLL_4:
            reg_value = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL4_CTRL_ADDRESS);
            reg_value = (uint32_t)CLOCK_MANAGER_CM_PLL4_CTRL_LOCKINTR_EN_MODIFY(reg_value, 
                                                                        (inth_enable ? 1 : 0));
            iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL4_CTRL_ADDRESS, reg_value);
            break;
        default:
            return ERROR_SP_PLL_INVALID_PLL_ID;
    }

    return 0;
}

static int clock_manager_set_pll_loss_int_enable(PLL_ID_t pll, bool inth_enable)
{
    uint32_t reg_value;

    switch (pll)
    {
        case PLL_ID_SP_PLL_0:
            reg_value = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL0_CTRL_ADDRESS);
            reg_value = (uint32_t)CLOCK_MANAGER_CM_PLL0_CTRL_LOSSINTR_EN_MODIFY(reg_value, 
                                                                        (inth_enable ? 1 : 0));
            iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL0_CTRL_ADDRESS, reg_value);
            break;
        case PLL_ID_SP_PLL_1:
            reg_value = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL1_CTRL_ADDRESS);
            reg_value = (uint32_t)CLOCK_MANAGER_CM_PLL1_CTRL_LOSSINTR_EN_MODIFY(reg_value, 
                                                                        (inth_enable ? 1 : 0));
            iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL1_CTRL_ADDRESS, reg_value);
            break;
        case PLL_ID_SP_PLL_2:
            reg_value = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL2_CTRL_ADDRESS);
            reg_value = (uint32_t)CLOCK_MANAGER_CM_PLL2_CTRL_LOSSINTR_EN_MODIFY(reg_value, 
                                                                        (inth_enable ? 1 : 0));
            iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL2_CTRL_ADDRESS, reg_value);
            break;
        case PLL_ID_SP_PLL_4:
            reg_value = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL4_CTRL_ADDRESS);
            reg_value = (uint32_t)CLOCK_MANAGER_CM_PLL4_CTRL_LOSSINTR_EN_MODIFY(reg_value, 
                                                                        (inth_enable ? 1 : 0));
            iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL4_CTRL_ADDRESS, reg_value);
            break;
        default:
            return ERROR_SP_PLL_INVALID_PLL_ID;
    }

    return 0;
}

static int clock_manager_get_pll_status(PLL_ID_t pll, uint32_t *cm_pll_status)
{
    switch (pll)
    {
        case PLL_ID_SP_PLL_0:
            *cm_pll_status = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL0_STATUS_ADDRESS);
            break;
        case PLL_ID_SP_PLL_1:
            *cm_pll_status = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL1_STATUS_ADDRESS);
            break;
        case PLL_ID_SP_PLL_2:
            *cm_pll_status = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL2_STATUS_ADDRESS);
            break;
        case PLL_ID_SP_PLL_4:
            *cm_pll_status = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL4_STATUS_ADDRESS);
            break;
        default:
            return ERROR_SP_PLL_INVALID_PLL_ID;
    }

    return 0;
}

static int clock_manager_clear_pll_status(PLL_ID_t pll)
{
    switch (pll)
    {
        case PLL_ID_SP_PLL_0:
            iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL0_STATUS_ADDRESS, 0);
            break;
        case PLL_ID_SP_PLL_1:
            iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL1_STATUS_ADDRESS, 0);
            break;
        case PLL_ID_SP_PLL_2:
            iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL2_STATUS_ADDRESS, 0);
            break;
        case PLL_ID_SP_PLL_4:
            iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_PLL4_STATUS_ADDRESS, 0);
            break;
        default:
            return ERROR_SP_PLL_INVALID_PLL_ID;
    }

    return 0;
}

static inline void clear_lock_monitor(volatile uint32_t *pll_registers)
{
    // PLL_REG_INDEX_REG_LOCK_MONITOR_CONTROL[0]: sample_strobe
    // PLL_REG_INDEX_REG_LOCK_MONITOR_CONTROL[1]: lock_monitor_clear
    pll_registers[PLL_REG_INDEX_REG_LOCK_MONITOR_CONTROL] = 0x3;
    __asm volatile ( " nop " );
    pll_registers[PLL_REG_INDEX_REG_LOCK_MONITOR_CONTROL] = 0x0;
}

static inline uint32_t get_lock_monitor(volatile uint32_t *pll_registers)
{
    // PLL_REG_INDEX_REG_LOCK_MONITOR_CONTROL[0]: sample_strobe
    pll_registers[PLL_REG_INDEX_REG_LOCK_MONITOR_CONTROL] = 0x1;
    __asm volatile ( " nop " );
    pll_registers[PLL_REG_INDEX_REG_LOCK_MONITOR_CONTROL] = 0x0;
    return (pll_registers[PLL_REG_INDEX_REG_LOCK_MONITOR] & PLL_LOCK_MONITOR_MASK);
}

void spio_pll_clear_lock_monitor(PLL_ID_t pll)
{
    switch (pll)
    {
        case PLL_ID_SP_PLL_0:
            clear_lock_monitor((uint32_t *)R_SP_PLL0_BASEADDR);
            break;
        case PLL_ID_SP_PLL_1:
            clear_lock_monitor((uint32_t *)R_SP_PLL1_BASEADDR);
            break;
        case PLL_ID_SP_PLL_2:
            clear_lock_monitor((uint32_t *)R_SP_PLL2_BASEADDR);
            break;
        case PLL_ID_SP_PLL_4:
            clear_lock_monitor((uint32_t *)R_SP_PLL4_BASEADDR);
            break;
        case PLL_ID_PSHIRE:
            clear_lock_monitor((uint32_t *)R_PCIE_PLLP0_BASEADDR);
            break;
        case PLL_ID_MAXION_CORE:
            clear_lock_monitor((uint32_t *)R_SP_PLLMX0_BASEADDR);
            break;
        case PLL_ID_MAXION_UNCORE:
            clear_lock_monitor((uint32_t *)R_SP_PLLMX1_BASEADDR);
            break;
        default:
            break;
    }

}

uint32_t spio_pll_get_lock_monitor(PLL_ID_t pll)
{
    switch (pll)
    {
        case PLL_ID_SP_PLL_0:
            return get_lock_monitor((uint32_t *)R_SP_PLL0_BASEADDR);
        case PLL_ID_SP_PLL_1:
            return get_lock_monitor((uint32_t *)R_SP_PLL1_BASEADDR);
        case PLL_ID_SP_PLL_2:
            return get_lock_monitor((uint32_t *)R_SP_PLL2_BASEADDR);
        case PLL_ID_SP_PLL_4:
            return get_lock_monitor((uint32_t *)R_SP_PLL4_BASEADDR);
        case PLL_ID_PSHIRE:
            return get_lock_monitor((uint32_t *)R_PCIE_PLLP0_BASEADDR);
        case PLL_ID_MAXION_CORE:
            return get_lock_monitor((uint32_t *)R_SP_PLLMX0_BASEADDR);
        case PLL_ID_MAXION_UNCORE:
            return get_lock_monitor((uint32_t *)R_SP_PLLMX1_BASEADDR);
        default:
            return 0;
    }
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

#pragma GCC diagnostic pop

static int spio_pll_wait_for_lock(volatile const uint32_t *pll_registers)
{
    uint32_t timeout = PLL_LOCK_TIMEOUT;

    /* Give some time for the clock to be stable
       Document suggest 65 ref clock.
       Use 3us for the worst case with 24Mhz ref
    */
    usdelay(3);

    /* Wait for the PLL to lock within a given timeout */
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

static int configure_pll_off(volatile uint32_t *pll_registers)
{
    /* disable the PLL */
    pll_registers[0] = 0;

    /* set reg_update strobe to update registers */
    pll_registers[PLL_REG_INDEX_REG_UPDATE_STROBE] = 1;

    return 0;
}

static int configure_pll(volatile uint32_t *pll_registers, uint8_t mode, uint32_t* target_freq,
                            HPDPLL_LDO_UPDATE_t ldo_update, uint32_t threshold_multiplier)
{
    uint8_t register_index;
    uint16_t register_value;
    uint8_t entry_index;
    uint32_t pll_settings_index;
    uint32_t reg0;
    uint16_t ldo_reg = 0;
    int status;
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
        /* Change LDO configuration if necessary */
        if(register_index == PLL_REG_INDEX_REG_LDO_CONTROL)
        {
            if (ldo_update == HPDPLL_LDO_DISABLE || ldo_update == HPDPLL_LDO_KICK)
            {
                register_value |= 0x2;     // Turn off LDO
                ldo_reg = register_value;
            }
            if (ldo_update == HPDPLL_LDO_BYPASS)
            {
                register_value |= 0x1;     // Bypass LDO
            }
        }
        /* Increase lock threshold if necessary */
        if(register_index == PLL_REG_INDEX_REG_LOCK_THRESHOLD)
        {
            if ((register_value * threshold_multiplier) > 0x0FFFFu)
            {
                register_value = (uint16_t)(0xFFFF);
            }
            else
            {
                register_value = (uint16_t)(register_value * threshold_multiplier);
            }
        }
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
    status = spio_pll_wait_for_lock(pll_registers);
    if(0 != status)
    {
        return status;
    }

    if (ldo_update == HPDPLL_LDO_KICK)
    {
        ldo_reg &= (uint16_t)(~(0x2));  // Turn on LDO
        pll_registers[PLL_REG_INDEX_REG_LDO_CONTROL] = ldo_reg;
        update_pll_registers(pll_registers); // Update PLL registers reg_update

        usdelay(3);

        reg0 = reg0 & ~PLL_ENABLE__MASK; // Disable PLL
        pll_registers[PLL_REG_INDEX_REG_0] = reg0;
        update_pll_registers(pll_registers);

        reg0 = reg0 | PLL_ENABLE__MASK; // Enable PLL
        pll_registers[PLL_REG_INDEX_REG_0] = reg0;
        update_pll_registers(pll_registers);

        status = spio_pll_wait_for_lock(pll_registers);
        if(0 != status)
        {
            return status;
        }
    }

    return 0;
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

    rv = configure_pll((uint32_t *)R_SP_PLL0_BASEADDR, mode, &gs_sp_pll_0_frequency,
                            HPDPLL_LDO_BYPASS, 1);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = clock_manager_pll_bypass(PLL_ID_SP_PLL_0, false);
    if (0 != rv)
    {
        goto ERROR;
    }

    spio_pll_clear_lock_monitor(PLL_ID_SP_PLL_0);

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

    rv = configure_pll((uint32_t *)R_SP_PLL1_BASEADDR, mode, &gs_sp_pll_1_frequency,
                            HPDPLL_LDO_BYPASS, 1);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = clock_manager_pll_bypass(PLL_ID_SP_PLL_1, false);
    if (0 != rv)
    {
        goto ERROR;
    }

    spio_pll_clear_lock_monitor(PLL_ID_SP_PLL_1);

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

    rv = configure_pll((uint32_t *)R_SP_PLL2_BASEADDR, mode, &gs_sp_pll_2_frequency,
                            HPDPLL_LDO_BYPASS, 1);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = clock_manager_pll_bypass(PLL_ID_SP_PLL_2, false);
    if (0 != rv)
    {
        goto ERROR;
    }

    spio_pll_clear_lock_monitor(PLL_ID_SP_PLL_2);

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

    rv = configure_pll((uint32_t *)R_SP_PLL4_BASEADDR, mode, &gs_sp_pll_4_frequency,
                            HPDPLL_LDO_BYPASS, 1);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = clock_manager_pll_bypass(PLL_ID_SP_PLL_4, false);
    if (0 != rv)
    {
        goto ERROR;
    }

    spio_pll_clear_lock_monitor(PLL_ID_SP_PLL_4);

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

    rv = configure_pll((uint32_t *)R_PCIE_PLLP0_BASEADDR, mode, &gs_pcie_pll_0_frequency,
                            HPDPLL_LDO_BYPASS, 1);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = clock_manager_pll_bypass(PLL_ID_PSHIRE, false);
    if (0 != rv)
    {
        goto ERROR;
    }

    spio_pll_clear_lock_monitor(PLL_ID_PSHIRE);

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

    rv = configure_pll((uint32_t *)R_SP_PLLMX0_BASEADDR, mode, &gs_maxion_pll_core_frequency,
                            HPDPLL_LDO_BYPASS, 1);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = clock_manager_pll_bypass(PLL_ID_MAXION_CORE, false);
    if (0 != rv)
    {
        goto ERROR;
    }

    spio_pll_clear_lock_monitor(PLL_ID_MAXION_CORE);

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

    rv = configure_pll((uint32_t *)R_SP_PLLMX1_BASEADDR, mode, &gs_maxion_pll_uncore_frequency,
                            HPDPLL_LDO_BYPASS, 1);
    if (0 != rv)
    {
        goto ERROR;
    }

    rv = clock_manager_pll_bypass(PLL_ID_MAXION_UNCORE, false);
    if (0 != rv)
    {
        goto ERROR;
    }

    spio_pll_clear_lock_monitor(PLL_ID_MAXION_UNCORE);

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

void pll_lock_loss_isr(void)
{
    uint32_t cm_pll_status = 0;
    
    // Check PLL0 status
    CHECK_AND_HANDLE_PLL_STATUS(PLL_ID_SP_PLL_0, CLOCK_MANAGER_CM_PLL0_STATUS_LOSS_GET,
                                cm_pll_0_lock_loss_count)

    // Check PLL1 status
    CHECK_AND_HANDLE_PLL_STATUS(PLL_ID_SP_PLL_1, CLOCK_MANAGER_CM_PLL1_STATUS_LOSS_GET,
                                cm_pll_1_lock_loss_count)

    // Check PLL2 status
    CHECK_AND_HANDLE_PLL_STATUS(PLL_ID_SP_PLL_2, CLOCK_MANAGER_CM_PLL2_STATUS_LOSS_GET,
                                cm_pll_2_lock_loss_count)

    // Check PLL3 status
    CHECK_AND_HANDLE_PLL_STATUS(PLL_ID_SP_PLL_4, CLOCK_MANAGER_CM_PLL4_STATUS_LOSS_GET,
                                cm_pll_4_lock_loss_count)
}

void enable_spio_pll_lock_loss_interrupt(void)
{
    PLL_LOCK_INT_DIS_LOST_INT_EN(PLL_ID_SP_PLL_0)
    PLL_LOCK_INT_DIS_LOST_INT_EN(PLL_ID_SP_PLL_1)
    PLL_LOCK_INT_DIS_LOST_INT_EN(PLL_ID_SP_PLL_2)
    PLL_LOCK_INT_DIS_LOST_INT_EN(PLL_ID_SP_PLL_4)
    INT_enableInterrupt(SPIO_PLIC_CRU_INTR_ID, 1, pll_lock_loss_isr);
}

void print_spio_lock_loss_counters(void)
{
    Log_Write(LOG_LEVEL_CRITICAL, "PLL SPIO LOCK LOSS CNT: %d\n", cm_pll_0_lock_loss_count);
    Log_Write(LOG_LEVEL_CRITICAL, "PLL PU LOCK LOSS CNT: %d\n", cm_pll_1_lock_loss_count);
    Log_Write(LOG_LEVEL_CRITICAL, "PLL NOC LOCK LOSS CNT: %d\n", cm_pll_2_lock_loss_count);
    Log_Write(LOG_LEVEL_CRITICAL, "PLL STEP LOCK LOSS CNT: %d\n", cm_pll_4_lock_loss_count);
}

void print_spio_lock_loss_monitors(void)
{
    uint32_t lock_monitor;

    lock_monitor = spio_pll_get_lock_monitor(PLL_ID_SP_PLL_0);
    if (0 != lock_monitor)
    {
        Log_Write(LOG_LEVEL_WARNING, "PLL0 lock monitor: %d\n", lock_monitor);
    }
    lock_monitor = spio_pll_get_lock_monitor(PLL_ID_SP_PLL_1);
    if (0 != lock_monitor)
    {
        Log_Write(LOG_LEVEL_WARNING, "PLL1 lock monitor: %d\n", lock_monitor);
    }
    lock_monitor = spio_pll_get_lock_monitor(PLL_ID_SP_PLL_2);
    if (0 != lock_monitor)
    {
        Log_Write(LOG_LEVEL_WARNING, "PLL2 lock monitor: %d\n", lock_monitor);
    }
    lock_monitor = spio_pll_get_lock_monitor(PLL_ID_SP_PLL_4);
    if (0 != lock_monitor)
    {
        Log_Write(LOG_LEVEL_WARNING, "PLL4 lock monitor: %d\n", lock_monitor);
    }
    lock_monitor = spio_pll_get_lock_monitor(PLL_ID_PSHIRE);
    if (0 != lock_monitor)
    {
        Log_Write(LOG_LEVEL_WARNING, "PSHIRE PLL lock monitor: %d\n", lock_monitor);
    }
}

void clear_spio_lock_loss_monitors(void)
{
    spio_pll_clear_lock_monitor(PLL_ID_SP_PLL_0);
    spio_pll_clear_lock_monitor(PLL_ID_SP_PLL_1);
    spio_pll_clear_lock_monitor(PLL_ID_SP_PLL_2);
    spio_pll_clear_lock_monitor(PLL_ID_SP_PLL_4);
    spio_pll_clear_lock_monitor(PLL_ID_PSHIRE);
}

int pll_init(uint32_t sp_pll_0_frequency, uint32_t sp_pll_1_frequency,
             uint32_t pcie_pll_0_frequency)
{
    gs_sp_pll_0_frequency = sp_pll_0_frequency;
    gs_sp_pll_1_frequency = sp_pll_1_frequency;
    gs_sp_pll_2_frequency = 0;
    gs_sp_pll_4_frequency = 0;
    gs_pcie_pll_0_frequency = pcie_pll_0_frequency;

    /* Clear lock monitors of PLLs configured during Bootrom */
    spio_pll_clear_lock_monitor(PLL_ID_SP_PLL_0);
    spio_pll_clear_lock_monitor(PLL_ID_SP_PLL_1);
    spio_pll_clear_lock_monitor(PLL_ID_PSHIRE);

    return 0;
}