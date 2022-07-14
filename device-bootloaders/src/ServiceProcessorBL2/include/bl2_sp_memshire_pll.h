/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
#ifndef __BL2_SP_MEMSHIRE_PLL__
#define __BL2_SP_MEMSHIRE_PLL__

#include "hwinc/hpdpll_modes_config.h"
#include "delays.h"

#define MEMSHIRE_PLL_CONFIG_BASE 0x61000000

/*! \def PLL_LOCK_TIMEOUT
    \brief lock timeout for PLL
*/
#define PLL_LOCK_TIMEOUT 10000

/*! \def PLL_REG_INDEX_REG_0
    \brief index register number
*/
#define PLL_REG_INDEX_REG_0 0

/*! \def PLL_REG_INDEX_REG_LOCK_THRESHOLD
    \brief lock threshold
*/
#define PLL_REG_INDEX_REG_LOCK_THRESHOLD 0x0B

/*! \def PLL_REG_INDEX_REG_LDO_CONTROL
    \brief LDO control
*/
#define PLL_REG_INDEX_REG_LDO_CONTROL 0x18

/*! \def PLL_REG_INDEX_REG_LOCK_MONITOR_CONTROL
    \brief Lock monitor sample strobe and clear
*/
#define PLL_REG_INDEX_REG_LOCK_MONITOR_CONTROL 0x19

/*! \def PLL_REG_INDEX_REG_FREQ_MONITOR_CONTROL
    \brief Freq montior control
*/
#define PLL_REG_INDEX_REG_FREQ_MONITOR_CONTROL 0x23

/*! \def PLL_REG_INDEX_REG_FREQ_MONITOR_CLK_REF_COUNT
    \brief Freq montior clock ref count
*/
#define PLL_REG_INDEX_REG_FREQ_MONITOR_CLK_REF_COUNT 0x24

/*! \def PLL_REG_INDEX_REG_FREQ_MONITOR_CLK_COUNT_0
    \brief Freq montior clock count 0
*/
#define PLL_REG_INDEX_REG_FREQ_MONITOR_CLK_COUNT_0 0x25

/*! \def PLL_REG_INDEX_REG_LOCK_MONITOR
    \brief Lock montior
*/
#define PLL_REG_INDEX_REG_LOCK_MONITOR 0x30

/*! \def PLL_LOCK_MONITOR_MASK
    \brief Lock montior mask
*/
#define PLL_LOCK_MONITOR_MASK 0x3F

/*! \def PLL_REG_INDEX_REG_DCO_CODE_ACQUIRED
    \brief DCO code aquired
*/
#define PLL_REG_INDEX_REG_DCO_CODE_ACQUIRED 0x31

/*! \def PLL_REG_INDEX_REG_DCO_CODE_LOOP_FILTER
    \brief DCO code aquired
*/
#define PLL_REG_INDEX_REG_DCO_CODE_LOOP_FILTER 0x32

/*! \def PLL_REG_INDEX_REG_UPDATE_STROBE
    \brief update strobe register index
*/
#define PLL_REG_INDEX_REG_UPDATE_STROBE 0x38

/*! \def PLL_REG_INDEX_REG_LOCK_DETECT_STATUS
    \brief PLL lock detect status
*/
#define PLL_REG_INDEX_REG_LOCK_DETECT_STATUS 0x39

/*! \def DCO_NORMALIZATION_ENABLE__SHIFT
    \brief DCO normalization enable shift
*/
#define DCO_NORMALIZATION_ENABLE__SHIFT 7u

/*! \def DCO_NORMALIZATION_ENABLE__MASK
    \brief DCO normalization enable mask
*/
#define DCO_NORMALIZATION_ENABLE__MASK (1u << DCO_NORMALIZATION_ENABLE__SHIFT)

/*! \def PLL_ENABLE__SHIFT
    \brief PLL enable shift
*/
#define PLL_ENABLE__SHIFT 3u

/*! \def PLL_ENABLE__MASK
    \brief PLL enable mask
*/
#define PLL_ENABLE__MASK (1u << PLL_ENABLE__SHIFT)

/*! \def FREQUENCY_HZ_TO_MHZ(x)
    \brief Converts HZ to MHZ
*/
#define FREQUENCY_HZ_TO_MHZ(x) ((x) / 1000000u)

/**
 * @enum MEM_HPDPLL_LDO_UPDATE_t
 * @brief Enum defining HPDPLL LDO update type
 */
typedef enum MEM_HPDPLL_LDO_UPDATE_e
{
    MEM_HPDPLL_LDO_NO_UPDATE,
    MEM_HPDPLL_LDO_ENABLE,
    MEM_HPDPLL_LDO_DISABLE,
    MEM_HPDPLL_LDO_BYPASS,
    MEM_HPDPLL_LDO_KICK
} MEM_HPDPLL_LDO_UPDATE_t;

static void write_memshire_pll_reg(uint8_t ms_num, uint16_t reg_addr, uint16_t reg_data)
{
    uint64_t address = MEMSHIRE_PLL_CONFIG_BASE;
    volatile uint32_t *p;

    address |= (uint64_t)((ms_num << 26) | (reg_addr << 2));
    p = (uint32_t *)address;
    *p = reg_data;
}

static uint16_t read_memshire_pll_reg(uint8_t ms_num, uint16_t reg_addr)
{
    uint64_t address = MEMSHIRE_PLL_CONFIG_BASE;
    volatile uint32_t *p;

    address |= (uint64_t)((ms_num << 26) | (reg_addr << 2));
    p = (uint32_t *)address;
    return (uint16_t)*p;
}

static void update_memshire_pll_regs(uint8_t ms_num)
{
    write_memshire_pll_reg(ms_num, 0x38, 1);
}

static int memshire_pll_wait_for_lock(uint8_t ms_num)
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
        if (read_memshire_pll_reg(ms_num, PLL_REG_INDEX_REG_LOCK_DETECT_STATUS) & 1)
        {
            return 0;
        }
        --timeout;
    }
    return -1;
}

static int program_memshire_pll(uint8_t ms_num, uint8_t mode, uint32_t *target_freq,
                                MEM_HPDPLL_LDO_UPDATE_t ldo_update, uint32_t threshold_multiplier)
{
    uint16_t reg0;
    uint16_t ldo_reg = 0;
    uint8_t register_index;
    uint16_t register_value;
    int status;

    Log_Write(LOG_LEVEL_DEBUG, "PLL:[txt]program_memshire_pll: start\n");

    /* Start programming PLL to mode N */
    if (!mode)
    {
        return -1;
    }

    for (int i = 0; i < gs_hpdpll_settings[(uint32_t)(mode - 1)].count; i++)
    {
        register_index = gs_hpdpll_settings[mode - 1].offsets[i];
        register_value = gs_hpdpll_settings[mode - 1].values[i];
        /* Change LDO configuration if necessary */
        if (register_index == PLL_REG_INDEX_REG_LDO_CONTROL)
        {
            if (ldo_update == MEM_HPDPLL_LDO_DISABLE || ldo_update == MEM_HPDPLL_LDO_KICK)
            {
                register_value |= 0x2; // Turn off LDO
                ldo_reg = register_value;
            }
            if (ldo_update == MEM_HPDPLL_LDO_BYPASS)
            {
                register_value |= 0x1; // Bypass LDO
            }
        }
        /* Increase lock threshold if necessary */
        if (register_index == PLL_REG_INDEX_REG_LOCK_THRESHOLD)
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
        write_memshire_pll_reg(ms_num, register_index, register_value);
    }
    update_memshire_pll_regs(ms_num);

    /*dco_normalization toggle */
    reg0 = read_memshire_pll_reg(ms_num, PLL_REG_INDEX_REG_0);
    reg0 = reg0 & (uint16_t)~DCO_NORMALIZATION_ENABLE__MASK;
    write_memshire_pll_reg(ms_num, PLL_REG_INDEX_REG_0, reg0);
    update_memshire_pll_regs(ms_num);

    reg0 = read_memshire_pll_reg(ms_num, PLL_REG_INDEX_REG_0);
    reg0 = reg0 | (uint16_t)DCO_NORMALIZATION_ENABLE__MASK;
    write_memshire_pll_reg(ms_num, PLL_REG_INDEX_REG_0, reg0);
    update_memshire_pll_regs(ms_num);

    status = memshire_pll_wait_for_lock(ms_num);
    if (0 != status)
    {
        return status;
    }

    if (ldo_update == MEM_HPDPLL_LDO_KICK)
    {
        ldo_reg &= (uint16_t)(~(0x2)); // Turn on LDO
        write_memshire_pll_reg(ms_num, PLL_REG_INDEX_REG_LDO_CONTROL, ldo_reg);
        update_memshire_pll_regs(ms_num);

        usdelay(3);

        reg0 = reg0 & (uint16_t)~PLL_ENABLE__MASK; // Disable PLL
        write_memshire_pll_reg(ms_num, PLL_REG_INDEX_REG_0, reg0);
        update_memshire_pll_regs(ms_num);

        reg0 = reg0 | (uint16_t)PLL_ENABLE__MASK; // Enable PLL
        write_memshire_pll_reg(ms_num, PLL_REG_INDEX_REG_0, reg0);
        update_memshire_pll_regs(ms_num);

        status = memshire_pll_wait_for_lock(ms_num);
        if (0 != status)
        {
            return status;
        }
    }
    Log_Write(LOG_LEVEL_DEBUG, "PLL:[txt]program_memshire_pll: PLL locked\n");

    *target_freq = FREQUENCY_HZ_TO_MHZ(gs_hpdpll_settings[(mode - 1)].output_frequency);

    return 0;
}

#endif
