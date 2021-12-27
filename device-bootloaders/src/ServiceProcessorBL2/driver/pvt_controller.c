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
#include "bl2_pvt_controller.h"
#include "log.h"
#include <hwinc/sp_pvt0.h>
#include "hwinc/hal_device.h"

/*! \def MAX(x,y)
    \brief Returns max
*/
#define MAX(x,y)       \
        x > y ? x : y

/*! \def MIN(x,y)
    \brief Returns min
*/
#define MIN(x,y)       \
        x < y ? x : y

/* PVT controllers reg pointers */
static Pvtc *pReg_Pvtc[PVTC_NUM];

/* Map of unused sensors */
static PVTC_IP_disable_mask pvtc_ip_disable_mask[PVTC_NUM] = {
    { .ts_disable_mask = 0x00, .pd_disable_mask = 0x00, .vm_disable_mask = 0xFC },
    { .ts_disable_mask = 0x00, .pd_disable_mask = 0x00, .vm_disable_mask = 0xFC },
    { .ts_disable_mask = 0x00, .pd_disable_mask = 0x00, .vm_disable_mask = 0xFC },
    { .ts_disable_mask = 0x00, .pd_disable_mask = 0x00, .vm_disable_mask = 0xFC },
    { .ts_disable_mask = 0xF8, .pd_disable_mask = 0xF8, .vm_disable_mask = 0xFF }
};

/* Minion shires always takes 3 consecutive VM channels [SRAM, NOC, MNN],
   So we are mapping just first one.
*/
static PVTC_VM_mapping pvtc_minion_shire_vm_map[PVTC_MINION_SHIRE_NUM] = {
    { // MinShire 0
        .pvtc_id = PVTC_2, .vm_id = VM_5, .ch_id = CHAN_7 },
    { // MinShire 1
        .pvtc_id = PVTC_2, .vm_id = VM_5, .ch_id = CHAN_10 },
    { // MinShire 2
        .pvtc_id = PVTC_3, .vm_id = VM_7, .ch_id = CHAN_4 },
    { // MinShire 3
        .pvtc_id = PVTC_3, .vm_id = VM_6, .ch_id = CHAN_1 },
    { // MinShire 4
        .pvtc_id = PVTC_0, .vm_id = VM_1, .ch_id = CHAN_13 },
    { // MinShire 5
        .pvtc_id = PVTC_0, .vm_id = VM_0, .ch_id = CHAN_4 },
    { // MinShire 6
        .pvtc_id = PVTC_1, .vm_id = VM_3, .ch_id = CHAN_10 },
    { // MinShire 7
        .pvtc_id = PVTC_1, .vm_id = VM_2, .ch_id = CHAN_1 },
    { // MinShire 8
        .pvtc_id = PVTC_2, .vm_id = VM_5, .ch_id = CHAN_13 },
    { // MinShire 9
        .pvtc_id = PVTC_2, .vm_id = VM_4, .ch_id = CHAN_13 },
    { // MinShire 10
        .pvtc_id = PVTC_3, .vm_id = VM_6, .ch_id = CHAN_4 },
    { // MinShire 11
        .pvtc_id = PVTC_3, .vm_id = VM_6, .ch_id = CHAN_7 },
    { // MinShire 12
        .pvtc_id = PVTC_2, .vm_id = VM_5, .ch_id = CHAN_1 },
    { // MinShire 13
        .pvtc_id = PVTC_0, .vm_id = VM_0, .ch_id = CHAN_7 },
    { // MinShire 14
        .pvtc_id = PVTC_1, .vm_id = VM_3, .ch_id = CHAN_13 },
    { // MinShire 15
        .pvtc_id = PVTC_1, .vm_id = VM_2, .ch_id = CHAN_13 },
    { // MinShire 16
        .pvtc_id = PVTC_2, .vm_id = VM_5, .ch_id = CHAN_4 },
    { // MinShire 17
        .pvtc_id = PVTC_2, .vm_id = VM_4, .ch_id = CHAN_10 },
    { // MinShire 18
        .pvtc_id = PVTC_3, .vm_id = VM_7, .ch_id = CHAN_7 },
    { // MinShire 19
        .pvtc_id = PVTC_3, .vm_id = VM_6, .ch_id = CHAN_10 },
    { // MinShire 20
        .pvtc_id = PVTC_0, .vm_id = VM_1, .ch_id = CHAN_7 },
    { // MinShire 21
        .pvtc_id = PVTC_0, .vm_id = VM_0, .ch_id = CHAN_10 },
    { // MinShire 22
        .pvtc_id = PVTC_1, .vm_id = VM_3, .ch_id = CHAN_7 },
    { // MinShire 23
        .pvtc_id = PVTC_1, .vm_id = VM_2, .ch_id = CHAN_10 },
    { // MinShire 24
        .pvtc_id = PVTC_2, .vm_id = VM_4, .ch_id = CHAN_4 },
    { // MinShire 25
        .pvtc_id = PVTC_2, .vm_id = VM_4, .ch_id = CHAN_7 },
    { // MinShire 26
        .pvtc_id = PVTC_3, .vm_id = VM_7, .ch_id = CHAN_10 },
    { // MinShire 27
        .pvtc_id = PVTC_3, .vm_id = VM_7, .ch_id = CHAN_13 },
    { // MinShire 28
        .pvtc_id = PVTC_0, .vm_id = VM_1, .ch_id = CHAN_10 },
    { // MinShire 29
        .pvtc_id = PVTC_0, .vm_id = VM_0, .ch_id = CHAN_13 },
    { // MinShire 30
        .pvtc_id = PVTC_1, .vm_id = VM_3, .ch_id = CHAN_4 },
    { // MinShire 31
        .pvtc_id = PVTC_1, .vm_id = VM_2, .ch_id = CHAN_4 },
    { // MinShire 32
        .pvtc_id = PVTC_3, .vm_id = VM_6, .ch_id = CHAN_13 },
    { // MinShire 33
        .pvtc_id = PVTC_1, .vm_id = VM_2, .ch_id = CHAN_7 }
};

/* MEM shires always takes 2 consecutive VM channels [MS, NOC],
   But due to bellow errata we will map both.

   NOTE: There is a known problem with the memshire east voltage sensor.
   Please see page 55 of the A0 Errata:
   https://docs.google.com/document/d/1JCTKjSTspS4Fj7WezyilNgonfKyLIKgcV18QwJBm3SA

   Workaround:

   A close approximation of the NoC voltage at these Memshires can be determined by
   reading the NoC voltage from Minion Shires that are physically adjacent as follows:

    Memshire #     Adjacent Minion Shire (Virtual ID)
        4             5
        5            13
        6             6
        7            14

   So, East Memshires NOC voltage will be mapped to nearest Minion Shire.
*/
static PVTC_VM_mapping pvtc_memshire_vm_map[PVTC_MEM_SHIRE_NUM][2] = {
    { // Memshire 0
        {.pvtc_id = PVTC_2, .vm_id = VM_4, .ch_id = CHAN_0},
        {.pvtc_id = PVTC_2, .vm_id = VM_4, .ch_id = CHAN_1} },
    { // Memshire 1
        {.pvtc_id = PVTC_2, .vm_id = VM_4, .ch_id = CHAN_2},
        {.pvtc_id = PVTC_2, .vm_id = VM_4, .ch_id = CHAN_3} },
    { // Memshire 2
        {.pvtc_id = PVTC_3, .vm_id = VM_7, .ch_id = CHAN_2},
        {.pvtc_id = PVTC_3, .vm_id = VM_7, .ch_id = CHAN_3} },
    { // Memshire 3
        {.pvtc_id = PVTC_3, .vm_id = VM_7, .ch_id = CHAN_0},
        {.pvtc_id = PVTC_3, .vm_id = VM_7, .ch_id = CHAN_1} },
    { // Memshire 4
        {.pvtc_id = PVTC_0, .vm_id = VM_0, .ch_id = CHAN_0},
        {.pvtc_id = PVTC_0, .vm_id = VM_0, .ch_id = CHAN_5} },
    { // Memshire 5
        {.pvtc_id = PVTC_0, .vm_id = VM_0, .ch_id = CHAN_2},
        {.pvtc_id = PVTC_0, .vm_id = VM_0, .ch_id = CHAN_8} },
    { // Memshire 6
        {.pvtc_id = PVTC_1, .vm_id = VM_3, .ch_id = CHAN_2},
        {.pvtc_id = PVTC_1, .vm_id = VM_3, .ch_id = CHAN_11} },
    { // Memshire 7
        {.pvtc_id = PVTC_1, .vm_id = VM_3, .ch_id = CHAN_0},
        {.pvtc_id = PVTC_1, .vm_id = VM_3, .ch_id = CHAN_14} }
};

/* IOSHIRE takes 3 consecutive VM channels [MXN, PU, NOC],
   So we are mapping just first one.
*/
static PVTC_VM_mapping pvtc_ioshire_vm_map = { .pvtc_id = PVTC_0, .vm_id = VM_1, .ch_id = CHAN_1 };

/* PSHIRE takes 2 consecutive VM channels [PSHR, NOC],
   So we are mapping just first one.
*/
static PVTC_VM_mapping pvtc_pshire_vm_map = { .pvtc_id = PVTC_0, .vm_id = VM_1, .ch_id = CHAN_5 };

/* External analog inputs
*/
static PVTC_VM_mapping pvtc_ext_analog_vm_map[PVTC_EXT_ANALOG_VM_NUM] = {
    { // External analog 0
        .pvtc_id = PVTC_0, .vm_id = VM_1, .ch_id = CHAN_0 },
    { // External analog 1
        .pvtc_id = PVTC_2, .vm_id = VM_5, .ch_id = CHAN_0 }
};

static void pvt_confirm_alive(uint8_t pvt_id)
{
    uint32_t pvt_comp_id = 0;
    uint32_t pvt_id_num = 0;
    uint32_t pvt_tm_scratch = 0x0Fu;

    /* Check component ID */
    pvt_comp_id = pReg_Pvtc[pvt_id]->common.pvt_comp_id;
    if (pvt_comp_id != COMMON_PVT_COMP_ID_RESET_VALUE)
    {
        Log_Write(LOG_LEVEL_WARNING, "pvt_comp_id for PVTC%d is wrong: 0x%x\r\n", pvt_id,
                  pvt_comp_id);
    }

    pvt_id_num = pReg_Pvtc[pvt_id]->common.pvt_ip_num;
    if (pvt_id_num != pvt_id)
    {
        Log_Write(LOG_LEVEL_WARNING, "pvt_id_num for PVTC%d is wrong: 0x%x\r\n", pvt_id,
                  pvt_id_num);
    }

    /* Checking the Alive status for each pvtc instance by w/r PVT_TM_SCRATCH register */

    /* Verify the default reset value of the scratch register for each instance equals 0x0 */
    pvt_tm_scratch = pReg_Pvtc[pvt_id]->common.pvt_tm_scratch;
    if (pvt_tm_scratch != COMMON_PVT_TM_SCRATCH_RESET_VALUE)
    {
        Log_Write(LOG_LEVEL_WARNING, "pvt_tm_scratch reset value for PVTC%d is wrong: 0x%x\r\n",
                  pvt_id, pvt_tm_scratch);
    }

    /* Write an appropriate test value to the scratch register */
    pReg_Pvtc[pvt_id]->common.pvt_tm_scratch = 0xA5A5A5A5;
    pvt_tm_scratch = pReg_Pvtc[pvt_id]->common.pvt_tm_scratch;
    if (pvt_tm_scratch != 0xA5A5A5A5)
    {
        Log_Write(LOG_LEVEL_WARNING, "pvt_tm_scratch write failed for PVTC%d: 0x%x\r\n", pvt_id,
                  pvt_tm_scratch);
    }

    /* Write an appropriate test value to the scratch register */
    pReg_Pvtc[pvt_id]->common.pvt_tm_scratch = 0x5A5A5A5A;
    pvt_tm_scratch = pReg_Pvtc[pvt_id]->common.pvt_tm_scratch;
    if (pvt_tm_scratch != 0x5A5A5A5A)
    {
        Log_Write(LOG_LEVEL_WARNING, "pvt_tm_scratch write failed for PVTC%d: 0x%x\r\n", pvt_id,
                  pvt_tm_scratch);
    }
}

static void pvt_configure_controller(uint8_t pvt_id)
{
    /* Disable unsused TS IPs */
    pReg_Pvtc[pvt_id]->ts.ts_common.sdif_disable = pvtc_ip_disable_mask[pvt_id].ts_disable_mask;

    /* Disable unsused PD IPs */
    pReg_Pvtc[pvt_id]->pd.pd_common.sdif_disable = pvtc_ip_disable_mask[pvt_id].pd_disable_mask;

    /* Disable unused VM IPs */
    pReg_Pvtc[pvt_id]->vm.vm_common.sdif_disable = pvtc_ip_disable_mask[pvt_id].vm_disable_mask;
}

static void pvt_configure_clock_synth(uint8_t pvt_id)
{
    uint32_t clock_synth;

    /* Configure clock synth, same value used for all IPs */
    clock_synth = TS_COMMON_IP_CLK_SYNTH_CLK_SYNTH_EN_SET(0x1u) |
                  TS_COMMON_IP_CLK_SYNTH_CLK_SYNTH_HOLD_SET(PVTC_CLK_SYNTH_STROBE) |
                  TS_COMMON_IP_CLK_SYNTH_CLK_SYNTH_HI_SET(PVTC_CLK_SYNTH_HI) |
                  TS_COMMON_IP_CLK_SYNTH_CLK_SYNTH_LO_SET(PVTC_CLK_SYNTH_LO);

    pReg_Pvtc[pvt_id]->ts.ts_common.clk_synth = clock_synth;

    pReg_Pvtc[pvt_id]->pd.pd_common.clk_synth = clock_synth;

    pReg_Pvtc[pvt_id]->vm.vm_common.clk_synth = clock_synth;
}

static void pvt_ts_sdif_program(uint8_t pvt_id, uint8_t inhibit, uint8_t sdif_addr,
                                uint32_t sdif_data)
{
    uint32_t sdif_value;

    /* Set inhibit */
    pReg_Pvtc[pvt_id]->ts.ts_common.sdif_ctrl = inhibit;

    sdif_value = (uint32_t)(
        TS_COMMON_IP_SDIF_SDIF_PROG_SET(0x1u) | TS_COMMON_IP_SDIF_SDIF_WRN_SET(0x1u) |
        TS_COMMON_IP_SDIF_SDIF_ADDR_SET(sdif_addr) | TS_COMMON_IP_SDIF_SDIF_DATA_SET(sdif_data));

    /* Wait SDIF not busy */
    while (TS_COMMON_IP_SDIF_STATUS_SDIF_BUSY_GET(pReg_Pvtc[pvt_id]->ts.ts_common.sdif_status))
    {
        /* Waiting for SDIF to be free */
    }

    /* Perform SDIF write */
    pReg_Pvtc[pvt_id]->ts.ts_common.sdif = sdif_value;
}

static void pvt_pd_sdif_program(uint8_t pvt_id, uint8_t inhibit, uint8_t sdif_addr,
                                uint32_t sdif_data)
{
    uint32_t sdif_value;

    /* Set inhibit */
    pReg_Pvtc[pvt_id]->pd.pd_common.sdif_ctrl = inhibit;

    sdif_value = (uint32_t)(
        PD_COMMON_IP_SDIF_SDIF_PROG_SET(0x1u) | PD_COMMON_IP_SDIF_SDIF_WRN_SET(0x1u) |
        PD_COMMON_IP_SDIF_SDIF_ADDR_SET(sdif_addr) | PD_COMMON_IP_SDIF_SDIF_DATA_SET(sdif_data));

    /* Wait SDIF not busy */
    while (PD_COMMON_IP_SDIF_STATUS_SDIF_BUSY_GET(pReg_Pvtc[pvt_id]->pd.pd_common.sdif_status))
    {
        /* Waiting for SDIF to be free */
    }

    /* Perform SDIF write */
    pReg_Pvtc[pvt_id]->pd.pd_common.sdif = sdif_value;
}

static void pvt_vm_sdif_program(uint8_t pvt_id, uint8_t inhibit, uint8_t sdif_addr,
                                uint32_t sdif_data)
{
    uint32_t sdif_value;

    /* Set inhibit */
    pReg_Pvtc[pvt_id]->vm.vm_common.sdif_ctrl = inhibit;

    sdif_value = (uint32_t)(
        VM_COMMON_IP_SDIF_SDIF_PROG_SET(0x1u) | VM_COMMON_IP_SDIF_SDIF_WRN_SET(0x1u) |
        VM_COMMON_IP_SDIF_SDIF_ADDR_SET(sdif_addr) | VM_COMMON_IP_SDIF_SDIF_DATA_SET(sdif_data));

    /* Wait SDIF not busy */
    while (VM_COMMON_IP_SDIF_STATUS_SDIF_BUSY_GET(pReg_Pvtc[pvt_id]->vm.vm_common.sdif_status))
    {
        /* Waiting for SDIF to be free */
    }

    /* Perform SDIF write */
    pReg_Pvtc[pvt_id]->vm.vm_common.sdif = sdif_value;
}

static void pvt_configure_sda_regs(uint8_t pvt_id)
{
    uint32_t cfg_value;
    uint8_t cfg_0;
    uint8_t cfg_1;
    uint8_t cfg_2;

    /* Program TS SDA TMR reg */
    pvt_ts_sdif_program(pvt_id, pvtc_ip_disable_mask[pvt_id].ts_disable_mask,
                        PVTC_SDA_IP_TMR_ADDRESS, PVTC_TS_PWR_UP_DELAY);

    /* Program PD SDA TMR reg */
    pvt_pd_sdif_program(pvt_id, pvtc_ip_disable_mask[pvt_id].pd_disable_mask,
                        PVTC_SDA_IP_TMR_ADDRESS, PVTC_PD_PWR_UP_DELAY);

    /* Program VM SDA TMR reg */
    pvt_vm_sdif_program(pvt_id, pvtc_ip_disable_mask[pvt_id].vm_disable_mask,
                        PVTC_SDA_IP_TMR_ADDRESS, PVTC_VM_PWR_UP_DELAY);

    /* Program VM SDA POLLING reg */
    pvt_vm_sdif_program(pvt_id, pvtc_ip_disable_mask[pvt_id].vm_disable_mask,
                        PVTC_SDA_IP_POL_ADDRESS, 0x0000FFFFu);

    /* Program TS CFG regs */
    cfg_0 = PVT_TS_CFG0_RESOLUTION_SET(PVT_TS_RESOLUTION) |
            PVT_TS_CFG0_RUN_MODE_SET(PVT_TS_RUN_MODE);
    cfg_value = cfg_0;
    pvt_ts_sdif_program(pvt_id, pvtc_ip_disable_mask[pvt_id].ts_disable_mask,
                        PVTC_SDA_IP_CFG_ADDRESS, cfg_value);

    /* Program PD CFG regs */
    cfg_0 = PVT_PD_CFG0_RUN_MODE_SET(PVT_PD_RUN_MODE);
    cfg_1 =
        PVT_PD_CFG1_INTERTNAL_DELAY_CHAIN_21_ALWAYS_ON_SET(PVT_PD_INT_DELAY_CHAIN_21_ALWAYS_ON) |
        PVT_PD_CFG1_INTERTNAL_DELAY_CHAIN_20_ALWAYS_ON_SET(PVT_PD_INT_DELAY_CHAIN_20_ALWAYS_ON) |
        PVT_PD_CFG1_INTERTNAL_DELAY_CHAIN_19_ALWAYS_ON_SET(PVT_PD_INT_DELAY_CHAIN_19_ALWAYS_ON) |
        PVT_PD_CFG1_OSCILLATOR_SELECT_SET(PVT_PD_OSC_SELECT);
    cfg_2 = PVT_PD_CFG2_OSC_CNT_GATE_VALUE_SET(PVT_PD_OSC_CNT_GATE) |
            PVT_PD_CFG2_OSC_CNT_PRE_SCALER_SET(PVT_PD_OSC_CNT_PRE_SCALER);
    cfg_value = (uint32_t)((cfg_2 << 16) | (cfg_1 << 8) | cfg_0);
    pvt_pd_sdif_program(pvt_id, pvtc_ip_disable_mask[pvt_id].pd_disable_mask,
                        PVTC_SDA_IP_CFG_ADDRESS, cfg_value);

    /* Program VM CFG regs */
    cfg_0 = PVT_VM_CFG0_RESOLUTION_SET(PVT_VM_RESOLUTION) |
            PVT_VM_CFG0_RUN_MODE_SET(PVT_VM_RUN_MODE);
    cfg_1 = PVT_VM_CFG1_CHANNEL_SELECT_SET(PVT_VM_CH_SELECT);
    cfg_value = (uint32_t)((cfg_1 << 8) | cfg_0);
    pvt_vm_sdif_program(pvt_id, pvtc_ip_disable_mask[pvt_id].vm_disable_mask,
                        PVTC_SDA_IP_CFG_ADDRESS, cfg_value);
}

static void pvt_trigger_single_sample_run(uint8_t pvt_id)
{
    uint32_t ip_ctrl_value;

    /* Trigger TS sampling */
    ip_ctrl_value = PVTC_SDA_IP_CTRL_IP_AUTO_SET(0x1u) | PVTC_SDA_IP_CTRL_IP_RUN_ONCE_SET(0x1u);
    pvt_ts_sdif_program(pvt_id, pvtc_ip_disable_mask[pvt_id].ts_disable_mask,
                        PVTC_SDA_IP_CTRL_ADDRESS, ip_ctrl_value);

    /* Trigger PD sampling */
    ip_ctrl_value = PVTC_SDA_IP_CTRL_IP_AUTO_SET(0x1u) | PVTC_SDA_IP_CTRL_IP_RUN_ONCE_SET(0x1u);
    pvt_pd_sdif_program(pvt_id, pvtc_ip_disable_mask[pvt_id].pd_disable_mask,
                        PVTC_SDA_IP_CTRL_ADDRESS, ip_ctrl_value);

    /* Trigger VM sampling */
    ip_ctrl_value = PVTC_SDA_IP_CTRL_IP_VM_MODE_SET(0x1u) | PVTC_SDA_IP_CTRL_IP_AUTO_SET(0x1u) |
                    PVTC_SDA_IP_CTRL_IP_RUN_ONCE_SET(0x1u);
    pvt_vm_sdif_program(pvt_id, pvtc_ip_disable_mask[pvt_id].vm_disable_mask,
                        PVTC_SDA_IP_CTRL_ADDRESS, ip_ctrl_value);
}

void pvt_single_sample_run(void)
{
    for (uint8_t i = 0; i < PVTC_NUM; i++)
    {
        pvt_trigger_single_sample_run(i);
    }
}

static void pvt_trigger_continuous_sample_run(uint8_t pvt_id)
{
    uint32_t ip_ctrl_value;

    /* Trigger TS sampling */
    ip_ctrl_value = PVTC_SDA_IP_CTRL_IP_AUTO_SET(0x1u) | PVTC_SDA_IP_CTRL_IP_RUN_CONT_SET(0x1u);
    pvt_ts_sdif_program(pvt_id, pvtc_ip_disable_mask[pvt_id].ts_disable_mask,
                        PVTC_SDA_IP_CTRL_ADDRESS, ip_ctrl_value);

    /* Trigger PD sampling */
    ip_ctrl_value = PVTC_SDA_IP_CTRL_IP_AUTO_SET(0x1u) | PVTC_SDA_IP_CTRL_IP_RUN_CONT_SET(0x1u);
    pvt_pd_sdif_program(pvt_id, pvtc_ip_disable_mask[pvt_id].pd_disable_mask,
                        PVTC_SDA_IP_CTRL_ADDRESS, ip_ctrl_value);

    /* Trigger VM sampling */
    ip_ctrl_value = PVTC_SDA_IP_CTRL_IP_VM_MODE_SET(0x1u) | PVTC_SDA_IP_CTRL_IP_AUTO_SET(0x1u) |
                    PVTC_SDA_IP_CTRL_IP_RUN_CONT_SET(0x1u);
    pvt_vm_sdif_program(pvt_id, pvtc_ip_disable_mask[pvt_id].vm_disable_mask,
                        PVTC_SDA_IP_CTRL_ADDRESS, ip_ctrl_value);
}

void pvt_continuous_sample_run(void)
{
    for (uint8_t i = 0; i < PVTC_NUM; i++)
    {
        pvt_trigger_continuous_sample_run(i);
    }
}

static int pvt_ts_conversion(uint16_t ts_sample)
{
    int result;
    uint8_t run_mode = PVT_TS_RUN_MODE;

    if (run_mode == PVT_TS_RUN_MODE_RUN_1)
    {
        int g = PVT_TS_G_PARAMETER;
        int h = PVT_TS_H_PARAMETER;
        int cal5 = PVT_TS_CAL5_PARAMETER;
        result = (g + ((h * ts_sample) / cal5 - h / 2)) / 1000;
    }
    else
    {
        result = ts_sample;
    }

    return result;
}

static int pvt_pd_conversion(uint16_t pd_sample)
{
    int result;

    result = pd_sample;

    return result;
}

static uint16_t pvt_vm_conversion(uint16_t vm_sample)
{
    uint16_t result;
    uint8_t run_mode = PVT_VM_RUN_MODE;

    if (run_mode == PVT_VM_RUN_MODE_RUN_0)
    {
        int vref = PVT_VM_VREF_PARAMETER;
        int r = 8 + (3 - PVT_VM_RESOLUTION) * 2;

        int x = vref / 5;
        int y = (6000 * vm_sample) / (1 << 14) - 3000 / (1 << r) - 1000;
        result = (uint16_t)((x * y) / 1000000);
    }
    else
    {
        result = vm_sample;
    }

    return result;
}

int pvt_get_min_shire_ts_sample(PVTC_MINSHIRE_e min_id, int *ts_sample)
{
    uint8_t pvt_id = (uint8_t)(min_id / PVTC_TS_NUM);
    uint8_t ts_id = (uint8_t)(min_id % PVTC_TS_NUM);
    uint32_t sample_data;

    if (min_id > PVTC_MAX_MINSHIRE_ID)
    {
        Log_Write(LOG_LEVEL_WARNING, "MINION SHIRE id (%d) out of range\r\n", min_id);
        return ERROR_PVT_MINION_ID_OUT_OF_RANGE;
    }

    sample_data = pReg_Pvtc[pvt_id]->ts.ts_individual[ts_id].sdif_data;
    if (TS_PD_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_FAULT_GET(sample_data))
    {
        Log_Write(LOG_LEVEL_WARNING, "Fault occured during TS sampling, sample data invalid\r\n");
        return ERROR_PVT_SAMPLE_FAULT;
    }

    *ts_sample = pvt_ts_conversion(TS_PD_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));

    return 0;
}

int pvt_get_min_shire_ts_sample_hilo(PVTC_MINSHIRE_e min_id, int *ts_sample_high,
                                     int *ts_sample_low)
{
    uint8_t pvt_id = (uint8_t)(min_id / PVTC_TS_NUM);
    uint8_t ts_id = (uint8_t)(min_id % PVTC_TS_NUM);
    uint32_t hilo;

    if (min_id > PVTC_MAX_MINSHIRE_ID)
    {
        Log_Write(LOG_LEVEL_WARNING, "MINION SHIRE id (%d) out of range\r\n", min_id);
        return ERROR_PVT_MINION_ID_OUT_OF_RANGE;
    }

    hilo = pReg_Pvtc[pvt_id]->ts.ts_individual[ts_id].smpl_hilo;

    *ts_sample_high = pvt_ts_conversion((uint16_t)TS_PD_INDIVIDUAL_IP_SMPL_HILO_SMPL_HI_GET(hilo));
    *ts_sample_low = pvt_ts_conversion(TS_PD_INDIVIDUAL_IP_SMPL_HILO_SMPL_LO_GET(hilo));

    return 0;
}

int pvt_get_ioshire_ts_sample(int *ts_sample)
{
    uint8_t pvt_id = PVTC_IOSHIRE_TS_PD_ID / PVTC_TS_NUM;
    uint8_t ts_id = PVTC_IOSHIRE_TS_PD_ID % PVTC_TS_NUM;
    uint32_t sample_data;

    /* Check sample done */
    if (TS_PD_INDIVIDUAL_IP_SDIF_DONE_SDIF_SMPL_DONE_GET(
            pReg_Pvtc[pvt_id]->ts.ts_individual[ts_id].sdif_done) == 0)
    {
        Log_Write(LOG_LEVEL_INFO, "PVT TS sample for IOSHIRE is not done yet\r\n");
        return ERROR_PVT_SAMPLE_NOT_DONE;
    }

    sample_data = pReg_Pvtc[pvt_id]->ts.ts_individual[ts_id].sdif_data;
    if (TS_PD_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_FAULT_GET(sample_data))
    {
        Log_Write(LOG_LEVEL_WARNING, "Fault occured during TS sampling, sample data invalid\r\n");
        return ERROR_PVT_SAMPLE_FAULT;
    }

    *ts_sample = pvt_ts_conversion(TS_PD_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));

    return 0;
}

int pvt_get_ioshire_ts_sample_hilo(int *ts_sample_high, int *ts_sample_low)
{
    uint8_t pvt_id = PVTC_IOSHIRE_TS_PD_ID / PVTC_TS_NUM;
    uint8_t ts_id = PVTC_IOSHIRE_TS_PD_ID % PVTC_TS_NUM;
    uint32_t hilo;

    hilo = pReg_Pvtc[pvt_id]->ts.ts_individual[ts_id].smpl_hilo;

    *ts_sample_high = pvt_ts_conversion((uint16_t)TS_PD_INDIVIDUAL_IP_SMPL_HILO_SMPL_HI_GET(hilo));
    *ts_sample_low = pvt_ts_conversion(TS_PD_INDIVIDUAL_IP_SMPL_HILO_SMPL_LO_GET(hilo));

    return 0;
}

int pvt_get_min_shire_pd_sample(PVTC_MINSHIRE_e min_id, int *pd_sample)
{
    uint8_t pvt_id = (uint8_t)(min_id / PVTC_PD_NUM);
    uint8_t pd_id = (uint8_t)(min_id % PVTC_PD_NUM);
    uint32_t sample_data;

    if (min_id > PVTC_MAX_MINSHIRE_ID)
    {
        Log_Write(LOG_LEVEL_WARNING, "MINION SHIRE id (%d) out of range\r\n", min_id);
        return ERROR_PVT_MINION_ID_OUT_OF_RANGE;
    }

    /* Check sample done */
    if (TS_PD_INDIVIDUAL_IP_SDIF_DONE_SDIF_SMPL_DONE_GET(
            pReg_Pvtc[pvt_id]->pd.pd_individual[pd_id].sdif_done) == 0)
    {
        Log_Write(LOG_LEVEL_INFO, "PVT PD sample for MINION SHIRE %d is not done yet\r\n", min_id);
        return ERROR_PVT_SAMPLE_NOT_DONE;
    }

    sample_data = pReg_Pvtc[pvt_id]->pd.pd_individual[pd_id].sdif_data;
    if (TS_PD_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_FAULT_GET(sample_data))
    {
        Log_Write(LOG_LEVEL_WARNING, "Fault occured during PD sampling, sample data invalid\r\n");
        return ERROR_PVT_SAMPLE_FAULT;
    }

    *pd_sample = pvt_pd_conversion(TS_PD_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));

    return 0;
}

int pvt_get_min_shire_pd_sample_hilo(PVTC_MINSHIRE_e min_id, int *pd_sample_high,
                                     int *pd_sample_low)
{
    uint8_t pvt_id = (uint8_t)(min_id / PVTC_PD_NUM);
    uint8_t pd_id = (uint8_t)(min_id % PVTC_PD_NUM);
    uint32_t hilo;

    if (min_id > PVTC_MAX_MINSHIRE_ID)
    {
        Log_Write(LOG_LEVEL_WARNING, "MINION SHIRE id (%d) out of range\r\n", min_id);
        return ERROR_PVT_MINION_ID_OUT_OF_RANGE;
    }

    hilo = pReg_Pvtc[pvt_id]->pd.pd_individual[pd_id].smpl_hilo;

    *pd_sample_high = pvt_pd_conversion((uint16_t)TS_PD_INDIVIDUAL_IP_SMPL_HILO_SMPL_HI_GET(hilo));
    *pd_sample_low = pvt_pd_conversion((uint16_t)TS_PD_INDIVIDUAL_IP_SMPL_HILO_SMPL_LO_GET(hilo));

    return 0;
}

int pvt_get_ioshire_pd_sample(int *pd_sample)
{
    uint8_t pvt_id = PVTC_IOSHIRE_TS_PD_ID / PVTC_PD_NUM;
    uint8_t pd_id = PVTC_IOSHIRE_TS_PD_ID % PVTC_PD_NUM;
    uint32_t sample_data;

    /* Check sample done */
    if (TS_PD_INDIVIDUAL_IP_SDIF_DONE_SDIF_SMPL_DONE_GET(
            pReg_Pvtc[pvt_id]->pd.pd_individual[pd_id].sdif_done) == 0)
    {
        Log_Write(LOG_LEVEL_INFO, "PVT PD sample for IOSHIRE is not done yet\r\n");
        return ERROR_PVT_SAMPLE_NOT_DONE;
    }

    sample_data = pReg_Pvtc[pvt_id]->pd.pd_individual[pd_id].sdif_data;
    if (TS_PD_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_FAULT_GET(sample_data))
    {
        Log_Write(LOG_LEVEL_WARNING, "Fault occured during PD sampling, sample data invalid\r\n");
        return ERROR_PVT_SAMPLE_FAULT;
    }

    *pd_sample = pvt_pd_conversion(TS_PD_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));

    return 0;
}

int pvt_get_ioshire_pd_sample_hilo(int *pd_sample_high, int *pd_sample_low)
{
    uint8_t pvt_id = PVTC_IOSHIRE_TS_PD_ID / PVTC_PD_NUM;
    uint8_t pd_id = PVTC_IOSHIRE_TS_PD_ID % PVTC_PD_NUM;
    uint32_t hilo;

    hilo = pReg_Pvtc[pvt_id]->pd.pd_individual[pd_id].smpl_hilo;

    *pd_sample_high = pvt_pd_conversion((uint16_t)TS_PD_INDIVIDUAL_IP_SMPL_HILO_SMPL_HI_GET(hilo));
    *pd_sample_low = pvt_pd_conversion((uint16_t)TS_PD_INDIVIDUAL_IP_SMPL_HILO_SMPL_LO_GET(hilo));

    return 0;
}

int pvt_get_min_shire_vm_sample(PVTC_MINSHIRE_e min_id, MinShire_VM_sample *vm_sample)
{
    if (min_id > PVTC_MAX_MINSHIRE_ID)
    {
        Log_Write(LOG_LEVEL_WARNING, "MINION SHIRE id (%d) out of range\r\n", min_id);
        return ERROR_PVT_MINION_ID_OUT_OF_RANGE;
    }

    uint8_t pvt_id = pvtc_minion_shire_vm_map[min_id].pvtc_id;
    uint8_t vm_id = pvtc_minion_shire_vm_map[min_id].vm_id;
    uint8_t ch_id = pvtc_minion_shire_vm_map[min_id].ch_id;
    uint32_t sample_data;

    for (int i = 0; i < 3; i++)
    {
        sample_data = pReg_Pvtc[pvt_id]->vm.vm_individual[vm_id].sdif_data[ch_id + i];
        if (VM_INDIVIDUAL_IP_SDIF_DATA_FAULT_GET(sample_data))
        {
            Log_Write(LOG_LEVEL_WARNING,
                      "Fault occured during VM sampling, sample data invalid\r\n");
            return ERROR_PVT_SAMPLE_FAULT;
        }

        switch (i)
        {
            case 0:
                vm_sample->vdd_sram =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
                break;
            case 1:
                vm_sample->vdd_noc =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
                break;
            case 2:
                vm_sample->vdd_mnn =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
                break;
            default:
                break;
        }
    }

    return 0;
}

int pvt_get_min_shire_vm_sample_hilo(PVTC_MINSHIRE_e min_id, MinShire_VM_sample *vm_sample_high,
                                     MinShire_VM_sample *vm_sample_low)
{
    if (min_id > PVTC_MAX_MINSHIRE_ID)
    {
        Log_Write(LOG_LEVEL_WARNING, "MINION SHIRE id (%d) out of range\r\n", min_id);
        return ERROR_PVT_MINION_ID_OUT_OF_RANGE;
    }

    uint8_t pvt_id = pvtc_minion_shire_vm_map[min_id].pvtc_id;
    uint8_t vm_id = pvtc_minion_shire_vm_map[min_id].vm_id;
    uint8_t ch_id = pvtc_minion_shire_vm_map[min_id].ch_id;
    uint32_t hilo;

    for (int i = 0; i < 3; i++)
    {
        hilo = pReg_Pvtc[pvt_id]->vm.vm_individual[vm_id].alarm_and_hilo[ch_id + i].smpl_hilo;

        switch (i)
        {
            case 0:
                vm_sample_high->vdd_sram = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
                vm_sample_low->vdd_sram =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_LO_GET(hilo));
                break;
            case 1:
                vm_sample_high->vdd_noc = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
                vm_sample_low->vdd_noc =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_LO_GET(hilo));
                break;
            case 2:
                vm_sample_high->vdd_mnn = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
                vm_sample_low->vdd_mnn =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_LO_GET(hilo));
                break;
            default:
                break;
        }
    }

    return 0;
}

int pvt_get_memshire_vm_sample(PVTC_MEMSHIRE_e memshire_id, MemShire_VM_sample *vm_sample)
{
    if (memshire_id > PVTC_MAX_MEMSHIRE_ID)
    {
        Log_Write(LOG_LEVEL_WARNING, "MEM SHIRE id (%d) out of range\r\n", memshire_id);
        return ERROR_PVT_MEM_ID_OUT_OF_RANGE;
    }

    uint32_t sample_data;

    for (int i = 0; i < 2; i++)
    {
        uint8_t pvt_id = pvtc_memshire_vm_map[memshire_id][i].pvtc_id;
        uint8_t vm_id = pvtc_memshire_vm_map[memshire_id][i].vm_id;
        uint8_t ch_id = pvtc_memshire_vm_map[memshire_id][i].ch_id;

        sample_data = pReg_Pvtc[pvt_id]->vm.vm_individual[vm_id].sdif_data[ch_id];
        if (VM_INDIVIDUAL_IP_SDIF_DATA_FAULT_GET(sample_data))
        {
            Log_Write(LOG_LEVEL_WARNING,
                      "Fault occured during VM sampling, sample data invalid\r\n");
            return ERROR_PVT_SAMPLE_FAULT;
        }

        switch (i)
        {
            case 0:
                vm_sample->vdd_ms =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
                break;
            case 1:
                vm_sample->vdd_noc =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
                break;
            default:
                break;
        }
    }

    return 0;
}

int pvt_get_memshire_vm_sample_hilo(PVTC_MEMSHIRE_e memshire_id, MemShire_VM_sample *vm_sample_high,
                                    MemShire_VM_sample *vm_sample_low)
{
    if (memshire_id > PVTC_MAX_MEMSHIRE_ID)
    {
        Log_Write(LOG_LEVEL_WARNING, "MEM SHIRE id (%d) out of range\r\n", memshire_id);
        return ERROR_PVT_MEM_ID_OUT_OF_RANGE;
    }

    uint32_t hilo;

    for (int i = 0; i < 2; i++)
    {
        uint8_t pvt_id = pvtc_memshire_vm_map[memshire_id][i].pvtc_id;
        uint8_t vm_id = pvtc_memshire_vm_map[memshire_id][i].vm_id;
        uint8_t ch_id = pvtc_memshire_vm_map[memshire_id][i].ch_id;

        hilo = pReg_Pvtc[pvt_id]->vm.vm_individual[vm_id].alarm_and_hilo[ch_id].smpl_hilo;

        switch (i)
        {
            case 0:
                vm_sample_high->vdd_ms = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
                vm_sample_low->vdd_ms = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_LO_GET(hilo));
                break;
            case 1:
                vm_sample_high->vdd_noc = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
                vm_sample_low->vdd_noc = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_LO_GET(hilo));
                break;
            default:
                break;
        }
    }

    return 0;
}

int pvt_get_ioshire_vm_sample(IOShire_VM_sample *vm_sample)
{
    uint8_t pvt_id = pvtc_ioshire_vm_map.pvtc_id;
    uint8_t vm_id = pvtc_ioshire_vm_map.vm_id;
    uint8_t ch_id = pvtc_ioshire_vm_map.ch_id;
    uint32_t sample_data;

    for (int i = 0; i < 3; i++)
    {
        sample_data = pReg_Pvtc[pvt_id]->vm.vm_individual[vm_id].sdif_data[ch_id + i];
        if (VM_INDIVIDUAL_IP_SDIF_DATA_FAULT_GET(sample_data))
        {
            Log_Write(LOG_LEVEL_WARNING,
                      "Fault occured during VM sampling, sample data invalid\r\n");
            return ERROR_PVT_SAMPLE_FAULT;
        }

        switch (i)
        {
            case 0:
                vm_sample->vdd_noc =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
                break;
            case 1:
                vm_sample->vdd_pu =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
                break;
            case 2:
                vm_sample->vdd_mxn =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
                break;
            default:
                break;
        }
    }

    return 0;
}

int pvt_get_ioshire_vm_sample_hilo(IOShire_VM_sample *vm_sample_high,
                                   IOShire_VM_sample *vm_sample_low)
{
    uint8_t pvt_id = pvtc_ioshire_vm_map.pvtc_id;
    uint8_t vm_id = pvtc_ioshire_vm_map.vm_id;
    uint8_t ch_id = pvtc_ioshire_vm_map.ch_id;
    uint32_t hilo;

    for (int i = 0; i < 3; i++)
    {
        hilo = pReg_Pvtc[pvt_id]->vm.vm_individual[vm_id].alarm_and_hilo[ch_id + i].smpl_hilo;

        switch (i)
        {
            case 0:
                vm_sample_high->vdd_noc = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
                vm_sample_low->vdd_noc = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_LO_GET(hilo));
                break;
            case 1:
                vm_sample_high->vdd_pu = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
                vm_sample_low->vdd_pu = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_LO_GET(hilo));
                break;
            case 2:
                vm_sample_high->vdd_mxn = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
                vm_sample_low->vdd_mxn = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_LO_GET(hilo));
                break;
            default:
                break;
        }
    }

    return 0;
}

int pvt_get_pshire_vm_sample(PShire_VM_sample *vm_sample)
{
    uint8_t pvt_id = pvtc_pshire_vm_map.pvtc_id;
    uint8_t vm_id = pvtc_pshire_vm_map.vm_id;
    uint8_t ch_id = pvtc_pshire_vm_map.ch_id;
    uint32_t sample_data;

    for (int i = 0; i < 2; i++)
    {
        sample_data = pReg_Pvtc[pvt_id]->vm.vm_individual[vm_id].sdif_data[ch_id + i];
        if (VM_INDIVIDUAL_IP_SDIF_DATA_FAULT_GET(sample_data))
        {
            Log_Write(LOG_LEVEL_WARNING,
                      "Fault occured during VM sampling, sample data invalid\r\n");
            return ERROR_PVT_SAMPLE_FAULT;
        }

        switch (i)
        {
            case 0:
                vm_sample->vdd_pshr =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
                break;
            case 1:
                vm_sample->vdd_noc =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
                break;
            default:
                break;
        }
    }

    return 0;
}

int pvt_get_pshire_vm_sample_hilo(PShire_VM_sample *vm_sample_high, PShire_VM_sample *vm_sample_low)
{
    uint8_t pvt_id = pvtc_pshire_vm_map.pvtc_id;
    uint8_t vm_id = pvtc_pshire_vm_map.vm_id;
    uint8_t ch_id = pvtc_pshire_vm_map.ch_id;
    uint32_t hilo;

    for (int i = 0; i < 2; i++)
    {
        hilo = pReg_Pvtc[pvt_id]->vm.vm_individual[vm_id].alarm_and_hilo[ch_id + i].smpl_hilo;

        switch (i)
        {
            case 0:
                vm_sample_high->vdd_pshr = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
                vm_sample_low->vdd_pshr = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_LO_GET(hilo));
                break;
            case 1:
                vm_sample_high->vdd_noc = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
                vm_sample_low->vdd_noc = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_LO_GET(hilo));
                break;
            default:
                break;
        }
    }

    return 0;
}

int pvt_get_ext_analog_vm_sample(PVTC_EXT_ANALOG_e ext_an_id, ExtAnalog_VM_sample *vm_sample)
{
    if (ext_an_id > PVTC_MAX_EXT_ANALOG_ID)
    {
        Log_Write(LOG_LEVEL_WARNING, "EXT ANALOG id (%d) out of range\r\n", ext_an_id);
        return ERROR_PVT_EXT_AN_ID_OUT_OF_RANGE;
    }

    uint8_t pvt_id = pvtc_ext_analog_vm_map[ext_an_id].pvtc_id;
    uint8_t vm_id = pvtc_ext_analog_vm_map[ext_an_id].vm_id;
    uint8_t ch_id = pvtc_ext_analog_vm_map[ext_an_id].ch_id;
    uint32_t sample_data;

    sample_data = pReg_Pvtc[pvt_id]->vm.vm_individual[vm_id].sdif_data[ch_id];
    if (VM_INDIVIDUAL_IP_SDIF_DATA_FAULT_GET(sample_data))
    {
        Log_Write(LOG_LEVEL_WARNING, "Fault occured during VM sampling, sample data invalid\r\n");
        return ERROR_PVT_SAMPLE_FAULT;
    }

    vm_sample->vdd_ext_analog =
        pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));

    return 0;
}

int pvt_get_ext_analog_vm_sample_hilo(PVTC_EXT_ANALOG_e ext_an_id,
                                      ExtAnalog_VM_sample *vm_sample_high,
                                      ExtAnalog_VM_sample *vm_sample_low)
{
    if (ext_an_id > PVTC_MAX_EXT_ANALOG_ID)
    {
        Log_Write(LOG_LEVEL_WARNING, "EXT ANALOG id (%d) out of range\r\n", ext_an_id);
        return ERROR_PVT_EXT_AN_ID_OUT_OF_RANGE;
    }

    uint8_t pvt_id = pvtc_ext_analog_vm_map[ext_an_id].pvtc_id;
    uint8_t vm_id = pvtc_ext_analog_vm_map[ext_an_id].vm_id;
    uint8_t ch_id = pvtc_ext_analog_vm_map[ext_an_id].ch_id;
    uint32_t hilo;

    hilo = pReg_Pvtc[pvt_id]->vm.vm_individual[vm_id].alarm_and_hilo[ch_id].smpl_hilo;

    vm_sample_high->vdd_ext_analog =
        pvt_vm_conversion((uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
    vm_sample_low->vdd_ext_analog =
        pvt_vm_conversion((uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_LO_GET(hilo));

    return 0;
}

static void pvt_ts_hilo_reset(uint8_t pvt_id)
{
    uint32_t hilo_rst;

    hilo_rst = TS_PD_INDIVIDUAL_IP_HILO_RESET_SMPL_HI_CLR_SET(0x1u) |
               TS_PD_INDIVIDUAL_IP_HILO_RESET_SMPL_LO_SET_SET(0x1u);

    for (uint8_t i = 0; i < PVTC_TS_NUM; i++)
    {
        pReg_Pvtc[pvt_id]->ts.ts_individual[i].hilo_reset = hilo_rst;
    }
}

static void pvt_pd_hilo_reset(uint8_t pvt_id)
{
    uint32_t hilo_rst;

    hilo_rst = TS_PD_INDIVIDUAL_IP_HILO_RESET_SMPL_HI_CLR_SET(0x1u) |
               TS_PD_INDIVIDUAL_IP_HILO_RESET_SMPL_LO_SET_SET(0x1u);

    for (uint8_t i = 0; i < PVTC_PD_NUM; i++)
    {
        pReg_Pvtc[pvt_id]->pd.pd_individual[i].hilo_reset = hilo_rst;
    }
}

static void pvt_vm_hilo_reset(uint8_t pvt_id)
{
    uint32_t hilo_rst;

    hilo_rst = VM_INDIVIDUAL_IP_ALARM_AND_HILO_HILO_RESET_SMPL_HI_CLR_SET(0x1u) |
               VM_INDIVIDUAL_IP_ALARM_AND_HILO_HILO_RESET_SMPL_LO_SET_SET(0x1u);

    for (uint8_t i = 0; i < PVTC_VM_NUM; i++)
    {
        for (uint8_t j = 0; j < PVTC_VM_CH_NUM; j++)
        {
            pReg_Pvtc[pvt_id]->vm.vm_individual[i].alarm_and_hilo[j].hilo_reset = hilo_rst;
        }
    }
}

int pvt_hilo_reset(void)
{
    for (uint8_t i = 0; i < PVTC_NUM; i++)
    {
        pvt_ts_hilo_reset(i);
        pvt_pd_hilo_reset(i);
        pvt_vm_hilo_reset(i);
    }

    return 0;
}

int pvt_init(void)
{
    /* Initialize reg pointers */
    pReg_Pvtc[0] = (Pvtc *)(R_SP_PVT0_BASEADDR);
    pReg_Pvtc[1] = (Pvtc *)(R_SP_PVT1_BASEADDR);
    pReg_Pvtc[2] = (Pvtc *)(R_SP_PVT2_BASEADDR);
    pReg_Pvtc[3] = (Pvtc *)(R_SP_PVT3_BASEADDR);
    pReg_Pvtc[4] = (Pvtc *)(R_SP_PVT4_BASEADDR);

    for (uint8_t i = 0; i < PVTC_NUM; i++)
    {
        pvt_confirm_alive(i);
        pvt_configure_controller(i);
        pvt_configure_clock_synth(i);
        pvt_configure_sda_regs(i);
    }

    pvt_continuous_sample_run();

    return 0;
}

static void pvt_print_ioshire_temperature_sampled_values(void)
{
    int ret;
    int sample;
    int sample_hi;
    int sample_lo;

    pvt_get_ioshire_ts_sample_hilo(&sample_hi, &sample_lo);
    ret = pvt_get_ioshire_ts_sample(&sample);
    if (0 != ret) {
        Log_Write(LOG_LEVEL_CRITICAL, "IOSHIRE Temp [C]: Sample fault [%d, %d]\n",
                sample_lo, sample_hi);
    }
    else {
        Log_Write(LOG_LEVEL_CRITICAL, "IOSHIRE Temp [C]: %d [%d, %d]\n",
                sample, sample_lo, sample_hi);
    }
}

static void pvt_print_ioshire_voltage_sampled_values(void)
{
    int ret;

    IOShire_VM_sample ioshire_sample = {0, 0, 0};
    IOShire_VM_sample ioshire_sample_hi = {0, 0, 0};
    IOShire_VM_sample ioshire_sample_lo = {0, 0, 0};
    pvt_get_ioshire_vm_sample_hilo(&ioshire_sample_hi, &ioshire_sample_lo);
    ret = pvt_get_ioshire_vm_sample(&ioshire_sample);
    if (0 != ret) {
        Log_Write(LOG_LEVEL_CRITICAL, "IOSHIRE Voltage [mV]: VDD_PU: Sample fault [%d, %d]"
                " VDD_NOC: Sample fault [%d, %d]\n",
                ioshire_sample_lo.vdd_pu, ioshire_sample_hi.vdd_pu,
                ioshire_sample_lo.vdd_noc, ioshire_sample_hi.vdd_noc);
    }
    else {
        Log_Write(LOG_LEVEL_CRITICAL, "IOSHIRE Voltage [mV]: VDD_PU: %d [%d, %d]"
                " VDD_NOC: %d [%d, %d]\n",
                ioshire_sample.vdd_pu, ioshire_sample_lo.vdd_pu, ioshire_sample_hi.vdd_pu,
                ioshire_sample.vdd_noc, ioshire_sample_lo.vdd_noc, ioshire_sample_hi.vdd_noc);
    }
}

static void pvt_print_pshire_voltage_sampled_values(void)
{
    int ret;

    PShire_VM_sample pshire_sample = {0, 0};
    PShire_VM_sample pshire_sample_hi = {0, 0};
    PShire_VM_sample pshire_sample_lo = {0, 0};
    pvt_get_pshire_vm_sample_hilo(&pshire_sample_hi, &pshire_sample_lo);
    ret = pvt_get_pshire_vm_sample(&pshire_sample);
    if (0 != ret) {
        Log_Write(LOG_LEVEL_CRITICAL, "PSHIRE Voltage [mV]:"
                " VDD_PSHR: Sample fault [%d, %d] VDD_NOC: Sample fault [%d, %d]\n",
                pshire_sample_lo.vdd_pshr, pshire_sample_hi.vdd_pshr,
                pshire_sample_lo.vdd_noc, pshire_sample_hi.vdd_noc);
    }
    else {
        Log_Write(LOG_LEVEL_CRITICAL, "PSHIRE Voltage [mV]:"
                " VDD_PSHR: %d [%d, %d] VDD_NOC: %d [%d, %d]\n",
                pshire_sample.vdd_pshr, pshire_sample_lo.vdd_pshr, pshire_sample_hi.vdd_pshr,
                pshire_sample.vdd_noc, pshire_sample_lo.vdd_noc, pshire_sample_hi.vdd_noc);
    }
}

static void pvt_print_memshire_voltage_sampled_values(void)
{
    int ret;
    MemShire_VM_sample memshire_sample_avg = {0, 0};
    MemShire_VM_sample memshire_sample_hi_max = {0, 0};
    MemShire_VM_sample memshire_sample_lo_min = {0xFFFF, 0xFFFF};
    int valid_samples_num = 0;

    for(int mem = 0; mem < PVTC_MEM_SHIRE_NUM; mem++)
    {
        MemShire_VM_sample memshire_sample = {0, 0};
        MemShire_VM_sample memshire_sample_hi = {0, 0};
        MemShire_VM_sample memshire_sample_lo = {0, 0};

        pvt_get_memshire_vm_sample_hilo(mem, &memshire_sample_hi, &memshire_sample_lo);
        ret = pvt_get_memshire_vm_sample(mem, &memshire_sample);
        if (0 != ret) {
            Log_Write(LOG_LEVEL_DEBUG, "MEM %d Voltage [mV]:"
                " VDD_MS: Sample fault [%d, %d] VDD_NOC: Sample fault [%d, %d]\n",
                mem, memshire_sample_lo.vdd_ms, memshire_sample_hi.vdd_ms,
                memshire_sample_lo.vdd_noc, memshire_sample_hi.vdd_noc);
        }
        else {
            Log_Write(LOG_LEVEL_DEBUG, "MEM %d Voltage [mV]:"
                " VDD_MS: %d [%d, %d] VDD_NOC: %d [%d, %d]\n",
                mem, memshire_sample.vdd_ms, memshire_sample_lo.vdd_ms, memshire_sample_hi.vdd_ms,
                memshire_sample.vdd_noc, memshire_sample_lo.vdd_noc, memshire_sample_hi.vdd_noc);

            memshire_sample_avg.vdd_ms =
                (uint16_t)(memshire_sample_avg.vdd_ms + memshire_sample.vdd_ms);
            memshire_sample_avg.vdd_noc =
                (uint16_t)(memshire_sample_avg.vdd_noc + memshire_sample.vdd_noc);
            valid_samples_num++;
        }

        memshire_sample_hi_max.vdd_ms =
            MAX(memshire_sample_hi_max.vdd_ms, memshire_sample_hi.vdd_ms);
        memshire_sample_hi_max.vdd_noc =
            MAX(memshire_sample_hi_max.vdd_noc, memshire_sample_hi.vdd_noc);
        memshire_sample_lo_min.vdd_ms =
            MIN(memshire_sample_lo_min.vdd_ms, memshire_sample_lo.vdd_ms);
        memshire_sample_lo_min.vdd_noc =
            MIN(memshire_sample_lo_min.vdd_noc, memshire_sample_lo.vdd_noc);
    }

    memshire_sample_avg.vdd_ms = (uint16_t)(memshire_sample_avg.vdd_ms / valid_samples_num);
    memshire_sample_avg.vdd_noc = (uint16_t)(memshire_sample_avg.vdd_noc / valid_samples_num);

    Log_Write(LOG_LEVEL_CRITICAL, "MemShire Average Volt[mV]:"
                " VDD_MS: %d [%d, %d] VDD_NOC: %d [%d, %d]\n",
                memshire_sample_avg.vdd_ms, memshire_sample_lo_min.vdd_ms,
                memshire_sample_hi_max.vdd_ms,  memshire_sample_avg.vdd_noc,
                memshire_sample_lo_min.vdd_noc, memshire_sample_hi_max.vdd_noc);
}

static void pvt_print_min_shire_temperature_sampled_values(void)
{
    int ret;
    int sample;
    int sample_hi;
    int sample_lo;
    int sample_avg = 0;
    int sample_hi_max = 0;
    int sample_lo_min = 0xFFFF;
    int valid_samples_num = 0;

    for(int min = 0; min < PVTC_MINION_SHIRE_NUM; min++) {
        pvt_get_min_shire_ts_sample_hilo(min, &sample_hi, &sample_lo);
        ret = pvt_get_min_shire_ts_sample(min, &sample);
        if (0 != ret) {
            Log_Write(LOG_LEVEL_DEBUG, "MS %2d Temp [C]: Sample fault [%d, %d]\n",
                min, sample_lo, sample_hi);
        }
        else {
            Log_Write(LOG_LEVEL_DEBUG, "MS %2d Temp [C]: %d [%d, %d]\n",
                min, sample, sample_lo, sample_hi);
            sample_avg = sample_avg + sample;
            valid_samples_num++;
        }

        sample_hi_max = MAX(sample_hi_max, sample_hi);
        sample_lo_min = MIN(sample_lo_min, sample_lo);
    }

    sample_avg = sample_avg / valid_samples_num;
    
    Log_Write(LOG_LEVEL_CRITICAL, "MinShire Average Temp[C]: %d [%d, %d]\n",
                sample_avg, sample_lo_min, sample_hi_max);
}

static void pvt_print_min_shire_voltage_sampled_values(void)
{
    int ret;
    MinShire_VM_sample minshire_sample_avg = {0, 0, 0};
    MinShire_VM_sample minshire_sample_hi_max = {0, 0, 0};
    MinShire_VM_sample minshire_sample_lo_min = {0xFFFF, 0xFFFF, 0xFFFF};
    int valid_samples_num = 0;

    for(int min = 0; min < PVTC_MINION_SHIRE_NUM; min++) {

        MinShire_VM_sample minshire_sample = {0, 0, 0};
        MinShire_VM_sample minshire_sample_hi = {0, 0, 0};
        MinShire_VM_sample minshire_sample_lo = {0, 0, 0};

        pvt_get_min_shire_vm_sample_hilo(min, &minshire_sample_hi, &minshire_sample_lo);
        ret = pvt_get_min_shire_vm_sample(min, &minshire_sample);
        if (0 != ret) {
            Log_Write(LOG_LEVEL_DEBUG, "MS %2d Voltage [mV]: VDD_MNN: Sample fault [%d, %d]"
                " VDD_SRAM: Sample fault [%d, %d] VDD_NOC: Sample fault [%d, %d]\n",
                min, minshire_sample_lo.vdd_mnn, minshire_sample_hi.vdd_mnn,
                minshire_sample_lo.vdd_sram, minshire_sample_hi.vdd_sram,
                minshire_sample_lo.vdd_noc, minshire_sample_hi.vdd_noc);
        }
        else {
            Log_Write(LOG_LEVEL_DEBUG, "MS %2d Voltage [mV]: VDD_MNN: %d [%d, %d]"
                " VDD_SRAM: %d [%d, %d] VDD_NOC: %d [%d, %d]\n",
                min,minshire_sample.vdd_mnn, minshire_sample_lo.vdd_mnn, minshire_sample_hi.vdd_mnn,
                minshire_sample.vdd_sram, minshire_sample_lo.vdd_sram, minshire_sample_hi.vdd_sram,
                minshire_sample.vdd_noc, minshire_sample_lo.vdd_noc, minshire_sample_hi.vdd_noc);

            minshire_sample_avg.vdd_mnn =
                (uint16_t)(minshire_sample_avg.vdd_mnn + minshire_sample.vdd_mnn);
            minshire_sample_avg.vdd_sram =
                (uint16_t)(minshire_sample_avg.vdd_sram + minshire_sample.vdd_sram);
            minshire_sample_avg.vdd_noc =
                (uint16_t)(minshire_sample_avg.vdd_noc + minshire_sample.vdd_noc);
            valid_samples_num++;
        }

        minshire_sample_hi_max.vdd_mnn =
            MAX(minshire_sample_hi_max.vdd_mnn, minshire_sample_hi.vdd_mnn);
        minshire_sample_hi_max.vdd_sram =
            MAX(minshire_sample_hi_max.vdd_sram, minshire_sample_hi.vdd_sram);
        minshire_sample_hi_max.vdd_noc =
            MAX(minshire_sample_hi_max.vdd_noc, minshire_sample_hi.vdd_noc);
        minshire_sample_lo_min.vdd_mnn =
            MIN(minshire_sample_lo_min.vdd_mnn, minshire_sample_lo.vdd_mnn);
        minshire_sample_lo_min.vdd_sram =
            MIN(minshire_sample_lo_min.vdd_sram, minshire_sample_lo.vdd_sram);
        minshire_sample_lo_min.vdd_noc =
            MIN(minshire_sample_lo_min.vdd_noc, minshire_sample_lo.vdd_noc);
    }

    minshire_sample_avg.vdd_mnn = (uint16_t)(minshire_sample_avg.vdd_mnn / valid_samples_num);
    minshire_sample_avg.vdd_sram = (uint16_t)(minshire_sample_avg.vdd_sram / valid_samples_num);
    minshire_sample_avg.vdd_noc = (uint16_t)(minshire_sample_avg.vdd_noc / valid_samples_num);

    Log_Write(LOG_LEVEL_CRITICAL, "MinShire Average Volt[mV]: VDD_MNN: %d [%d, %d]"
                " VDD_SRAM: %d [%d, %d] VDD_NOC: %d [%d, %d]\n",
                minshire_sample_avg.vdd_mnn, minshire_sample_lo_min.vdd_mnn,
                minshire_sample_hi_max.vdd_mnn, minshire_sample_avg.vdd_sram,
                minshire_sample_lo_min.vdd_sram, minshire_sample_hi_max.vdd_sram,
                minshire_sample_avg.vdd_noc, minshire_sample_lo_min.vdd_noc,
                minshire_sample_hi_max.vdd_noc);
}

void pvt_print_voltage_sampled_values(pvtc_shire_type_t shire_type)
{
    switch(shire_type)
    {
        case PVTC_IOSHIRE: pvt_print_ioshire_voltage_sampled_values();
            break;
        case PVTC_PSHIRE: pvt_print_pshire_voltage_sampled_values();
            break;
        case PVTC_MEMSHIRE: pvt_print_memshire_voltage_sampled_values();
            break;
        case PVTC_MINION_SHIRE: pvt_print_min_shire_voltage_sampled_values();
            break;
        default: break;
    }
}

void pvt_print_temperature_sampled_values(pvtc_shire_type_t shire_type)
{
    switch(shire_type)
    {
        case PVTC_IOSHIRE: pvt_print_ioshire_temperature_sampled_values();
            break;
        case PVTC_MINION_SHIRE: pvt_print_min_shire_temperature_sampled_values();
            break;
        case PVTC_PSHIRE:
        case PVTC_MEMSHIRE:
            break;
        default:
            break;
    }
}

int pvt_get_minion_avg_temperature(uint8_t* avg_temp)
{
    int sample;
    int status;
    int avg = 0;
    int valid_samples_num = 0;
    
    for(int min = 0; min < PVTC_MINION_SHIRE_NUM; min++) {
        status = pvt_get_min_shire_ts_sample(min, &sample);
        if (0 == status) {
            avg = avg + sample;
            valid_samples_num++;
        }
    }

    if (valid_samples_num == 0)
    {
        Log_Write(LOG_LEVEL_WARNING, "There were no valid Minion Shire TS samples\r\n");
        return ERROR_PVT_NO_VALID_TS_SAMPLES;
    }

    avg = avg / valid_samples_num;
    *avg_temp = (uint8_t)avg;

    return 0;
}

void pvt_print_all(void)
{
    /* Print Temperatures */
    pvt_print_temperature_sampled_values(PVTC_IOSHIRE);
    pvt_print_temperature_sampled_values(PVTC_MINION_SHIRE);

    /* Print Temperatures */
    pvt_print_voltage_sampled_values(PVTC_IOSHIRE);
    pvt_print_voltage_sampled_values(PVTC_PSHIRE);
    pvt_print_voltage_sampled_values(PVTC_MEMSHIRE);
    pvt_print_voltage_sampled_values(PVTC_MINION_SHIRE);
}
