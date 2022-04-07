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
#define MAX(x, y) x > y ? x : y

/*! \def MIN(x,y)
    \brief Returns min
*/
#define MIN(x, y) x < y ? x : y

/*! \def GET_MINION_VM(mishire_sample_avg,valid_samples_num) 
    \brief Reads all MinShire VM values
*/
#define GET_MINION_VM(mishire_sample_avg, valid_samples_num)                                       \
    int ret;                                                                                       \
    for (int min = 0; min < PVTC_MINION_SHIRE_NUM; min++)                                          \
    {                                                                                              \
        MinShire_VM_sample minshire_sample = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };            \
        ret = pvt_get_min_shire_vm_sample(min, &minshire_sample);                                  \
        if (0 != ret)                                                                              \
        {                                                                                          \
            Log_Write(LOG_LEVEL_DEBUG,                                                             \
                      "MS %2d Voltage [mV]: VDD_MNN: Sample fault"                                 \
                      " VDD_SRAM: Sample fault VDD_NOC: Sample fault\n",                           \
                      min);                                                                        \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            Log_Write(LOG_LEVEL_DEBUG,                                                             \
                      "MS %2d Voltage [mV]: VDD_MNN: %d [%d, %d]"                                  \
                      " VDD_SRAM: %d [%d, %d] VDD_NOC: %d [%d, %d]\n",                             \
                      min, minshire_sample.vdd_mnn.current, minshire_sample.vdd_mnn.low,           \
                      minshire_sample.vdd_mnn.high, minshire_sample.vdd_sram.current,              \
                      minshire_sample.vdd_sram.low, minshire_sample.vdd_sram.high,                 \
                      minshire_sample.vdd_noc.current, minshire_sample.vdd_noc.low,                \
                      minshire_sample.vdd_noc.high);                                               \
                                                                                                   \
            minshire_sample_avg.vdd_mnn.current =                                                  \
                (uint16_t)(minshire_sample_avg.vdd_mnn.current + minshire_sample.vdd_mnn.current); \
            minshire_sample_avg.vdd_sram.current = (uint16_t)(                                     \
                minshire_sample_avg.vdd_sram.current + minshire_sample.vdd_sram.current);          \
            minshire_sample_avg.vdd_noc.current =                                                  \
                (uint16_t)(minshire_sample_avg.vdd_noc.current + minshire_sample.vdd_noc.current); \
            valid_samples_num++;                                                                   \
                                                                                                   \
            minshire_sample_avg.vdd_mnn.high =                                                     \
                MAX(minshire_sample_avg.vdd_mnn.high, minshire_sample.vdd_mnn.high);               \
            minshire_sample_avg.vdd_sram.high =                                                    \
                MAX(minshire_sample_avg.vdd_sram.high, minshire_sample.vdd_sram.high);             \
            minshire_sample_avg.vdd_noc.high =                                                     \
                MAX(minshire_sample_avg.vdd_noc.high, minshire_sample.vdd_noc.high);               \
            minshire_sample_avg.vdd_mnn.low =                                                      \
                MIN(minshire_sample_avg.vdd_mnn.low, minshire_sample.vdd_mnn.low);                 \
            minshire_sample_avg.vdd_sram.low =                                                     \
                MIN(minshire_sample_avg.vdd_sram.low, minshire_sample.vdd_sram.low);               \
            minshire_sample_avg.vdd_noc.low =                                                      \
                MIN(minshire_sample_avg.vdd_noc.low, minshire_sample.vdd_noc.low);                 \
        }                                                                                          \
    }                                                                                              \
    if (valid_samples_num != 0)                                                                    \
    {                                                                                              \
        minshire_sample_avg.vdd_mnn.current =                                                      \
            (uint16_t)(minshire_sample_avg.vdd_mnn.current / valid_samples_num);                   \
        minshire_sample_avg.vdd_sram.current =                                                     \
            (uint16_t)(minshire_sample_avg.vdd_sram.current / valid_samples_num);                  \
        minshire_sample_avg.vdd_noc.current =                                                      \
            (uint16_t)(minshire_sample_avg.vdd_noc.current / valid_samples_num);                   \
    }

/*! \def GET_MEMSHIRE_VM(memshire_sample_avg,valid_samples_num) 
    \brief Reads all MemShire VM values
*/
#define GET_MEMSHIRE_VM(memshire_sample_avg, valid_samples_num)                                    \
    int ret;                                                                                       \
    for (int mem = 0; mem < PVTC_MEM_SHIRE_NUM; mem++)                                             \
    {                                                                                              \
        MemShire_VM_sample memshire_sample = { { 0, 0, 0 }, { 0, 0, 0 } };                         \
        ret = pvt_get_memshire_vm_sample(mem, &memshire_sample);                                   \
        if (0 != ret)                                                                              \
        {                                                                                          \
            Log_Write(LOG_LEVEL_DEBUG,                                                             \
                      "MEM %d Voltage [mV]:"                                                       \
                      " VDD_MS: Sample fault VDD_NOC: Sample fault\n",                             \
                      mem);                                                                        \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            Log_Write(LOG_LEVEL_DEBUG,                                                             \
                      "MEM %d Voltage [mV]:"                                                       \
                      " VDD_MS: %d [%d, %d] VDD_NOC: %d [%d, %d]\n",                               \
                      mem, memshire_sample.vdd_ms.current, memshire_sample.vdd_ms.low,             \
                      memshire_sample.vdd_ms.high, memshire_sample.vdd_noc.current,                \
                      memshire_sample.vdd_noc.low, memshire_sample.vdd_noc.high);                  \
                                                                                                   \
            memshire_sample_avg.vdd_ms.current =                                                   \
                (uint16_t)(memshire_sample_avg.vdd_ms.current + memshire_sample.vdd_ms.current);   \
            memshire_sample_avg.vdd_noc.current =                                                  \
                (uint16_t)(memshire_sample_avg.vdd_noc.current + memshire_sample.vdd_noc.current); \
            valid_samples_num++;                                                                   \
                                                                                                   \
            memshire_sample_avg.vdd_ms.high =                                                      \
                MAX(memshire_sample_avg.vdd_ms.high, memshire_sample.vdd_ms.high);                 \
            memshire_sample_avg.vdd_noc.high =                                                     \
                MAX(memshire_sample_avg.vdd_noc.high, memshire_sample.vdd_noc.high);               \
            memshire_sample_avg.vdd_ms.low =                                                       \
                MIN(memshire_sample_avg.vdd_ms.low, memshire_sample.vdd_ms.low);                   \
            memshire_sample_avg.vdd_noc.low =                                                      \
                MIN(memshire_sample_avg.vdd_noc.low, memshire_sample.vdd_noc.low);                 \
        }                                                                                          \
    }                                                                                              \
    if (valid_samples_num != 0)                                                                    \
    {                                                                                              \
        memshire_sample_avg.vdd_ms.current =                                                       \
            (uint16_t)(memshire_sample_avg.vdd_ms.current / valid_samples_num);                    \
        memshire_sample_avg.vdd_noc.current =                                                      \
            (uint16_t)(memshire_sample_avg.vdd_noc.current / valid_samples_num);                   \
    }

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
static PVTC_VM_mapping pvtc_minion_shire_vm_map[PVTC_MINION_SHIRE_NUM] = { { // MinShire 0
                                                                             .pvtc_id = PVTC_2,
                                                                             .vm_id = VM_5,
                                                                             .ch_id = CHAN_7 },
                                                                           { // MinShire 1
                                                                             .pvtc_id = PVTC_2,
                                                                             .vm_id = VM_5,
                                                                             .ch_id = CHAN_10 },
                                                                           { // MinShire 2
                                                                             .pvtc_id = PVTC_3,
                                                                             .vm_id = VM_7,
                                                                             .ch_id = CHAN_4 },
                                                                           { // MinShire 3
                                                                             .pvtc_id = PVTC_3,
                                                                             .vm_id = VM_6,
                                                                             .ch_id = CHAN_1 },
                                                                           { // MinShire 4
                                                                             .pvtc_id = PVTC_0,
                                                                             .vm_id = VM_1,
                                                                             .ch_id = CHAN_13 },
                                                                           { // MinShire 5
                                                                             .pvtc_id = PVTC_0,
                                                                             .vm_id = VM_0,
                                                                             .ch_id = CHAN_4 },
                                                                           { // MinShire 6
                                                                             .pvtc_id = PVTC_1,
                                                                             .vm_id = VM_3,
                                                                             .ch_id = CHAN_10 },
                                                                           { // MinShire 7
                                                                             .pvtc_id = PVTC_1,
                                                                             .vm_id = VM_2,
                                                                             .ch_id = CHAN_1 },
                                                                           { // MinShire 8
                                                                             .pvtc_id = PVTC_2,
                                                                             .vm_id = VM_5,
                                                                             .ch_id = CHAN_13 },
                                                                           { // MinShire 9
                                                                             .pvtc_id = PVTC_2,
                                                                             .vm_id = VM_4,
                                                                             .ch_id = CHAN_13 },
                                                                           { // MinShire 10
                                                                             .pvtc_id = PVTC_3,
                                                                             .vm_id = VM_6,
                                                                             .ch_id = CHAN_4 },
                                                                           { // MinShire 11
                                                                             .pvtc_id = PVTC_3,
                                                                             .vm_id = VM_6,
                                                                             .ch_id = CHAN_7 },
                                                                           { // MinShire 12
                                                                             .pvtc_id = PVTC_2,
                                                                             .vm_id = VM_5,
                                                                             .ch_id = CHAN_1 },
                                                                           { // MinShire 13
                                                                             .pvtc_id = PVTC_0,
                                                                             .vm_id = VM_0,
                                                                             .ch_id = CHAN_7 },
                                                                           { // MinShire 14
                                                                             .pvtc_id = PVTC_1,
                                                                             .vm_id = VM_3,
                                                                             .ch_id = CHAN_13 },
                                                                           { // MinShire 15
                                                                             .pvtc_id = PVTC_1,
                                                                             .vm_id = VM_2,
                                                                             .ch_id = CHAN_13 },
                                                                           { // MinShire 16
                                                                             .pvtc_id = PVTC_2,
                                                                             .vm_id = VM_5,
                                                                             .ch_id = CHAN_4 },
                                                                           { // MinShire 17
                                                                             .pvtc_id = PVTC_2,
                                                                             .vm_id = VM_4,
                                                                             .ch_id = CHAN_10 },
                                                                           { // MinShire 18
                                                                             .pvtc_id = PVTC_3,
                                                                             .vm_id = VM_7,
                                                                             .ch_id = CHAN_7 },
                                                                           { // MinShire 19
                                                                             .pvtc_id = PVTC_3,
                                                                             .vm_id = VM_6,
                                                                             .ch_id = CHAN_10 },
                                                                           { // MinShire 20
                                                                             .pvtc_id = PVTC_0,
                                                                             .vm_id = VM_1,
                                                                             .ch_id = CHAN_7 },
                                                                           { // MinShire 21
                                                                             .pvtc_id = PVTC_0,
                                                                             .vm_id = VM_0,
                                                                             .ch_id = CHAN_10 },
                                                                           { // MinShire 22
                                                                             .pvtc_id = PVTC_1,
                                                                             .vm_id = VM_3,
                                                                             .ch_id = CHAN_7 },
                                                                           { // MinShire 23
                                                                             .pvtc_id = PVTC_1,
                                                                             .vm_id = VM_2,
                                                                             .ch_id = CHAN_10 },
                                                                           { // MinShire 24
                                                                             .pvtc_id = PVTC_2,
                                                                             .vm_id = VM_4,
                                                                             .ch_id = CHAN_4 },
                                                                           { // MinShire 25
                                                                             .pvtc_id = PVTC_2,
                                                                             .vm_id = VM_4,
                                                                             .ch_id = CHAN_7 },
                                                                           { // MinShire 26
                                                                             .pvtc_id = PVTC_3,
                                                                             .vm_id = VM_7,
                                                                             .ch_id = CHAN_10 },
                                                                           { // MinShire 27
                                                                             .pvtc_id = PVTC_3,
                                                                             .vm_id = VM_7,
                                                                             .ch_id = CHAN_13 },
                                                                           { // MinShire 28
                                                                             .pvtc_id = PVTC_0,
                                                                             .vm_id = VM_1,
                                                                             .ch_id = CHAN_10 },
                                                                           { // MinShire 29
                                                                             .pvtc_id = PVTC_0,
                                                                             .vm_id = VM_0,
                                                                             .ch_id = CHAN_13 },
                                                                           { // MinShire 30
                                                                             .pvtc_id = PVTC_1,
                                                                             .vm_id = VM_3,
                                                                             .ch_id = CHAN_4 },
                                                                           { // MinShire 31
                                                                             .pvtc_id = PVTC_1,
                                                                             .vm_id = VM_2,
                                                                             .ch_id = CHAN_4 },
                                                                           { // MinShire 32
                                                                             .pvtc_id = PVTC_3,
                                                                             .vm_id = VM_6,
                                                                             .ch_id = CHAN_13 },
                                                                           { // MinShire 33
                                                                             .pvtc_id = PVTC_1,
                                                                             .vm_id = VM_2,
                                                                             .ch_id = CHAN_7 } };

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
      { .pvtc_id = PVTC_2, .vm_id = VM_4, .ch_id = CHAN_0 },
      { .pvtc_id = PVTC_2, .vm_id = VM_4, .ch_id = CHAN_1 } },
    { // Memshire 1
      { .pvtc_id = PVTC_2, .vm_id = VM_4, .ch_id = CHAN_2 },
      { .pvtc_id = PVTC_2, .vm_id = VM_4, .ch_id = CHAN_3 } },
    { // Memshire 2
      { .pvtc_id = PVTC_3, .vm_id = VM_7, .ch_id = CHAN_2 },
      { .pvtc_id = PVTC_3, .vm_id = VM_7, .ch_id = CHAN_3 } },
    { // Memshire 3
      { .pvtc_id = PVTC_3, .vm_id = VM_7, .ch_id = CHAN_0 },
      { .pvtc_id = PVTC_3, .vm_id = VM_7, .ch_id = CHAN_1 } },
    { // Memshire 4
      { .pvtc_id = PVTC_0, .vm_id = VM_0, .ch_id = CHAN_0 },
      { .pvtc_id = PVTC_0, .vm_id = VM_0, .ch_id = CHAN_5 } },
    { // Memshire 5
      { .pvtc_id = PVTC_0, .vm_id = VM_0, .ch_id = CHAN_2 },
      { .pvtc_id = PVTC_0, .vm_id = VM_0, .ch_id = CHAN_8 } },
    { // Memshire 6
      { .pvtc_id = PVTC_1, .vm_id = VM_3, .ch_id = CHAN_2 },
      { .pvtc_id = PVTC_1, .vm_id = VM_3, .ch_id = CHAN_11 } },
    { // Memshire 7
      { .pvtc_id = PVTC_1, .vm_id = VM_3, .ch_id = CHAN_0 },
      { .pvtc_id = PVTC_1, .vm_id = VM_3, .ch_id = CHAN_14 } }
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
static PVTC_VM_mapping pvtc_ext_analog_vm_map[PVTC_EXT_ANALOG_VM_NUM] = { { // External analog 0
                                                                            .pvtc_id = PVTC_0,
                                                                            .vm_id = VM_1,
                                                                            .ch_id = CHAN_0 },
                                                                          { // External analog 1
                                                                            .pvtc_id = PVTC_2,
                                                                            .vm_id = VM_5,
                                                                            .ch_id = CHAN_0 } };

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

static int16_t pvt_ts_conversion(uint16_t ts_sample)
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

    return (int16_t)result;
}

static uint16_t pvt_pd_conversion(uint16_t pd_sample)
{
    uint16_t result;

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

static int pvt_get_ts_data(uint8_t pvt_id, uint8_t ts_id, TS_Sample *ts_sample)
{
    uint32_t sample_data;
    uint32_t hilo;

    sample_data = pReg_Pvtc[pvt_id]->ts.ts_individual[ts_id].sdif_data;
    if (TS_PD_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_FAULT_GET(sample_data))
    {
        Log_Write(LOG_LEVEL_WARNING, "Fault occured during TS sampling, sample data invalid\r\n");
        return ERROR_PVT_SAMPLE_FAULT;
    }

    ts_sample->current =
        pvt_ts_conversion(TS_PD_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));

    hilo = pReg_Pvtc[pvt_id]->ts.ts_individual[ts_id].smpl_hilo;

    ts_sample->high = pvt_ts_conversion((uint16_t)TS_PD_INDIVIDUAL_IP_SMPL_HILO_SMPL_HI_GET(hilo));
    ts_sample->low = pvt_ts_conversion(TS_PD_INDIVIDUAL_IP_SMPL_HILO_SMPL_LO_GET(hilo));

    return 0;
}

int pvt_get_min_shire_ts_sample(PVTC_MINSHIRE_e min_id, TS_Sample *ts_sample)
{
    uint8_t pvt_id = (uint8_t)(min_id / PVTC_TS_NUM);
    uint8_t ts_id = (uint8_t)(min_id % PVTC_TS_NUM);

    if (min_id > PVTC_MAX_MINSHIRE_ID)
    {
        Log_Write(LOG_LEVEL_WARNING, "MINION SHIRE id (%d) out of range\r\n", min_id);
        return ERROR_PVT_MINION_ID_OUT_OF_RANGE;
    }

    return pvt_get_ts_data(pvt_id, ts_id, ts_sample);
}

int pvt_get_ioshire_ts_sample(TS_Sample *ts_sample)
{
    uint8_t pvt_id = PVTC_IOSHIRE_TS_PD_ID / PVTC_TS_NUM;
    uint8_t ts_id = PVTC_IOSHIRE_TS_PD_ID % PVTC_TS_NUM;

    return pvt_get_ts_data(pvt_id, ts_id, ts_sample);
}

static int pvt_get_pd_data(uint8_t pvt_id, uint8_t pd_id, PD_Sample *pd_sample)
{
    uint32_t sample_data;
    uint32_t hilo;

    sample_data = pReg_Pvtc[pvt_id]->pd.pd_individual[pd_id].sdif_data;
    if (TS_PD_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_FAULT_GET(sample_data))
    {
        Log_Write(LOG_LEVEL_WARNING, "Fault occured during PD sampling, sample data invalid\r\n");
        return ERROR_PVT_SAMPLE_FAULT;
    }

    pd_sample->current =
        pvt_pd_conversion(TS_PD_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));

    hilo = pReg_Pvtc[pvt_id]->pd.pd_individual[pd_id].smpl_hilo;

    pd_sample->high = pvt_pd_conversion((uint16_t)TS_PD_INDIVIDUAL_IP_SMPL_HILO_SMPL_HI_GET(hilo));
    pd_sample->low = pvt_pd_conversion((uint16_t)TS_PD_INDIVIDUAL_IP_SMPL_HILO_SMPL_LO_GET(hilo));

    return 0;
}

int pvt_get_min_shire_pd_sample(PVTC_MINSHIRE_e min_id, PD_Sample *pd_sample)
{
    uint8_t pvt_id = (uint8_t)(min_id / PVTC_PD_NUM);
    uint8_t pd_id = (uint8_t)(min_id % PVTC_PD_NUM);

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

    return pvt_get_pd_data(pvt_id, pd_id, pd_sample);
}

int pvt_get_ioshire_pd_sample(PD_Sample *pd_sample)
{
    uint8_t pvt_id = PVTC_IOSHIRE_TS_PD_ID / PVTC_PD_NUM;
    uint8_t pd_id = PVTC_IOSHIRE_TS_PD_ID % PVTC_PD_NUM;

    /* Check sample done */
    if (TS_PD_INDIVIDUAL_IP_SDIF_DONE_SDIF_SMPL_DONE_GET(
            pReg_Pvtc[pvt_id]->pd.pd_individual[pd_id].sdif_done) == 0)
    {
        Log_Write(LOG_LEVEL_INFO, "PVT PD sample for IOSHIRE is not done yet\r\n");
        return ERROR_PVT_SAMPLE_NOT_DONE;
    }

    return pvt_get_pd_data(pvt_id, pd_id, pd_sample);
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
    uint32_t hilo;

    for (int i = 0; i < 3; i++)
    {
        sample_data = pReg_Pvtc[pvt_id]->vm.vm_individual[vm_id].sdif_data[ch_id + i];
        hilo = pReg_Pvtc[pvt_id]->vm.vm_individual[vm_id].alarm_and_hilo[ch_id + i].smpl_hilo;
        if (VM_INDIVIDUAL_IP_SDIF_DATA_FAULT_GET(sample_data))
        {
            Log_Write(LOG_LEVEL_WARNING,
                      "Fault occured during VM sampling, sample data invalid\r\n");
            return ERROR_PVT_SAMPLE_FAULT;
        }

        switch (i)
        {
            case 0:
                vm_sample->vdd_sram.current =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
                vm_sample->vdd_sram.high = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
                vm_sample->vdd_sram.low =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_LO_GET(hilo));
                break;
            case 1:
                vm_sample->vdd_noc.current =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
                vm_sample->vdd_noc.high = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
                vm_sample->vdd_noc.low =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_LO_GET(hilo));
                break;
            case 2:
                vm_sample->vdd_mnn.current =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
                vm_sample->vdd_mnn.high = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
                vm_sample->vdd_mnn.low =
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
    uint32_t hilo;

    for (int i = 0; i < 2; i++)
    {
        uint8_t pvt_id = pvtc_memshire_vm_map[memshire_id][i].pvtc_id;
        uint8_t vm_id = pvtc_memshire_vm_map[memshire_id][i].vm_id;
        uint8_t ch_id = pvtc_memshire_vm_map[memshire_id][i].ch_id;

        sample_data = pReg_Pvtc[pvt_id]->vm.vm_individual[vm_id].sdif_data[ch_id];
        hilo = pReg_Pvtc[pvt_id]->vm.vm_individual[vm_id].alarm_and_hilo[ch_id].smpl_hilo;
        if (VM_INDIVIDUAL_IP_SDIF_DATA_FAULT_GET(sample_data))
        {
            Log_Write(LOG_LEVEL_WARNING,
                      "Fault occured during VM sampling, sample data invalid\r\n");
            return ERROR_PVT_SAMPLE_FAULT;
        }

        switch (i)
        {
            case 0:
                vm_sample->vdd_ms.current =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
                vm_sample->vdd_ms.high = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
                vm_sample->vdd_ms.low = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_LO_GET(hilo));
                break;
            case 1:
                vm_sample->vdd_noc.current =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
                vm_sample->vdd_noc.high = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
                vm_sample->vdd_noc.low = pvt_vm_conversion(
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
    uint32_t hilo;

    for (int i = 0; i < 3; i++)
    {
        sample_data = pReg_Pvtc[pvt_id]->vm.vm_individual[vm_id].sdif_data[ch_id + i];
        hilo = pReg_Pvtc[pvt_id]->vm.vm_individual[vm_id].alarm_and_hilo[ch_id + i].smpl_hilo;
        if (VM_INDIVIDUAL_IP_SDIF_DATA_FAULT_GET(sample_data))
        {
            Log_Write(LOG_LEVEL_WARNING,
                      "Fault occured during VM sampling, sample data invalid\r\n");
            return ERROR_PVT_SAMPLE_FAULT;
        }

        switch (i)
        {
            case 0:
                vm_sample->vdd_noc.current =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
                vm_sample->vdd_noc.high = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
                vm_sample->vdd_noc.low = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_LO_GET(hilo));
                break;
            case 1:
                vm_sample->vdd_pu.current =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
                vm_sample->vdd_pu.high = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
                vm_sample->vdd_pu.low = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_LO_GET(hilo));
                break;
            case 2:
                vm_sample->vdd_mxn.current =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
                vm_sample->vdd_mxn.high = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
                vm_sample->vdd_mxn.low = pvt_vm_conversion(
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
    uint32_t hilo;

    for (int i = 0; i < 2; i++)
    {
        sample_data = pReg_Pvtc[pvt_id]->vm.vm_individual[vm_id].sdif_data[ch_id + i];
        hilo = pReg_Pvtc[pvt_id]->vm.vm_individual[vm_id].alarm_and_hilo[ch_id + i].smpl_hilo;
        if (VM_INDIVIDUAL_IP_SDIF_DATA_FAULT_GET(sample_data))
        {
            Log_Write(LOG_LEVEL_WARNING,
                      "Fault occured during VM sampling, sample data invalid\r\n");
            return ERROR_PVT_SAMPLE_FAULT;
        }

        switch (i)
        {
            case 0:
                vm_sample->vdd_pshr.current =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
                vm_sample->vdd_pshr.high = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
                vm_sample->vdd_pshr.low = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_LO_GET(hilo));
                break;
            case 1:
                vm_sample->vdd_noc.current =
                    pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
                vm_sample->vdd_noc.high = pvt_vm_conversion(
                    (uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
                vm_sample->vdd_noc.low = pvt_vm_conversion(
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
    uint32_t hilo;

    sample_data = pReg_Pvtc[pvt_id]->vm.vm_individual[vm_id].sdif_data[ch_id];
    hilo = pReg_Pvtc[pvt_id]->vm.vm_individual[vm_id].alarm_and_hilo[ch_id].smpl_hilo;
    if (VM_INDIVIDUAL_IP_SDIF_DATA_FAULT_GET(sample_data))
    {
        Log_Write(LOG_LEVEL_WARNING, "Fault occured during VM sampling, sample data invalid\r\n");
        return ERROR_PVT_SAMPLE_FAULT;
    }

    vm_sample->vdd_ext_analog.current =
        pvt_vm_conversion(VM_INDIVIDUAL_IP_SDIF_DATA_SAMPLE_DATA_GET(sample_data));
    vm_sample->vdd_ext_analog.high =
        pvt_vm_conversion((uint16_t)VM_INDIVIDUAL_IP_ALARM_AND_HILO_SMPL_HILO_SMPL_HI_GET(hilo));
    vm_sample->vdd_ext_analog.low =
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
    TS_Sample sample;

    ret = pvt_get_ioshire_ts_sample(&sample);
    if (0 != ret)
    {
        Log_Write(LOG_LEVEL_CRITICAL, "IOSHIRE Temp [C]: Sample fault\n");
    }
    else
    {
        Log_Write(LOG_LEVEL_CRITICAL, "IOSHIRE Temp [C]: %d [%d, %d]\n", sample.current, sample.low,
                  sample.high);
    }
}

static void pvt_print_ioshire_voltage_sampled_values(void)
{
    int ret;

    IOShire_VM_sample ioshire_sample = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
    ret = pvt_get_ioshire_vm_sample(&ioshire_sample);
    if (0 != ret)
    {
        Log_Write(LOG_LEVEL_CRITICAL, "IOSHIRE Voltage [mV]: VDD_PU: Sample fault"
                                      " VDD_NOC: Sample fault\n");
    }
    else
    {
        Log_Write(LOG_LEVEL_CRITICAL,
                  "IOSHIRE Voltage [mV]: VDD_PU: %d [%d, %d]"
                  " VDD_NOC: %d [%d, %d]\n",
                  ioshire_sample.vdd_pu.current, ioshire_sample.vdd_pu.low,
                  ioshire_sample.vdd_pu.high, ioshire_sample.vdd_noc.current,
                  ioshire_sample.vdd_noc.low, ioshire_sample.vdd_noc.high);
    }
}

static void pvt_print_pshire_voltage_sampled_values(void)
{
    int ret;

    PShire_VM_sample pshire_sample = { { 0, 0, 0 }, { 0, 0, 0 } };
    ret = pvt_get_pshire_vm_sample(&pshire_sample);
    if (0 != ret)
    {
        Log_Write(LOG_LEVEL_WARNING, "PSHIRE Voltage [mV]:"
                                     " VDD_PSHR: Sample fault VDD_NOC: Sample fault \n");
    }
    else
    {
        Log_Write(LOG_LEVEL_CRITICAL,
                  "PSHIRE Voltage [mV]:"
                  " VDD_PSHR: %d [%d, %d] VDD_NOC: %d [%d, %d]\n",
                  pshire_sample.vdd_pshr.current, pshire_sample.vdd_pshr.low,
                  pshire_sample.vdd_pshr.high, pshire_sample.vdd_noc.current,
                  pshire_sample.vdd_noc.low, pshire_sample.vdd_noc.high);
    }
}

static void pvt_print_memshire_voltage_sampled_values(void)
{
    MemShire_VM_sample memshire_sample_avg = { { 0, 0, 0xFFFF }, { 0, 0, 0xFFFF } };
    int valid_samples_num = 0;

    GET_MEMSHIRE_VM(memshire_sample_avg, valid_samples_num)

    Log_Write(LOG_LEVEL_CRITICAL,
              "MemShire Average Volt[mV]:"
              " VDD_MS: %d [%d, %d] VDD_NOC: %d [%d, %d]\n",
              memshire_sample_avg.vdd_ms.current, memshire_sample_avg.vdd_ms.low,
              memshire_sample_avg.vdd_ms.high, memshire_sample_avg.vdd_noc.current,
              memshire_sample_avg.vdd_noc.low, memshire_sample_avg.vdd_noc.high);
}

static void pvt_print_min_shire_temperature_sampled_values(void)
{
    int ret;
    TS_Sample sample;
    TS_Sample sample_avg = { 0, 0, 0x0FFF };
    int valid_samples_num = 0;

    for (int min = 0; min < PVTC_MINION_SHIRE_NUM; min++)
    {
        ret = pvt_get_min_shire_ts_sample(min, &sample);
        if (0 != ret)
        {
            Log_Write(LOG_LEVEL_DEBUG, "MS %2d Temp [C]: Sample fault\n", min);
        }
        else
        {
            Log_Write(LOG_LEVEL_DEBUG, "MS %2d Temp [C]: %d [%d, %d]\n", min, sample.current,
                      sample.low, sample.high);

            sample_avg.current = (int16_t)(sample_avg.current + sample.current);
            valid_samples_num++;

            sample_avg.high = MAX(sample_avg.high, sample.high);
            sample_avg.low = MIN(sample_avg.low, sample.low);
        }
    }

    sample_avg.current = (int16_t)(sample_avg.current / valid_samples_num);

    Log_Write(LOG_LEVEL_CRITICAL, "MinShire Average Temp[C]: %d [%d, %d]\n", sample_avg.current,
              sample_avg.low, sample_avg.high);
}

static void pvt_print_min_shire_voltage_sampled_values(void)
{
    MinShire_VM_sample minshire_sample_avg = { { 0, 0, 0xFFFF },
                                               { 0, 0, 0xFFFF },
                                               { 0, 0, 0xFFFF } };
    int valid_samples_num = 0;

    GET_MINION_VM(mishire_sample_avg, valid_samples_num)

    Log_Write(LOG_LEVEL_CRITICAL,
              "MinShire Average Volt[mV]: VDD_MNN: %d [%d, %d]"
              " VDD_SRAM: %d [%d, %d] VDD_NOC: %d [%d, %d]\n",
              minshire_sample_avg.vdd_mnn.current, minshire_sample_avg.vdd_mnn.low,
              minshire_sample_avg.vdd_mnn.high, minshire_sample_avg.vdd_sram.current,
              minshire_sample_avg.vdd_sram.low, minshire_sample_avg.vdd_sram.high,
              minshire_sample_avg.vdd_noc.current, minshire_sample_avg.vdd_noc.low,
              minshire_sample_avg.vdd_noc.high);
}

void pvt_print_voltage_sampled_values(pvtc_shire_type_t shire_type)
{
    switch (shire_type)
    {
        case PVTC_IOSHIRE:
            pvt_print_ioshire_voltage_sampled_values();
            break;
        case PVTC_PSHIRE:
            pvt_print_pshire_voltage_sampled_values();
            break;
        case PVTC_MEMSHIRE:
            pvt_print_memshire_voltage_sampled_values();
            break;
        case PVTC_MINION_SHIRE:
            pvt_print_min_shire_voltage_sampled_values();
            break;
        default:
            break;
    }
}

void pvt_print_temperature_sampled_values(pvtc_shire_type_t shire_type)
{
    switch (shire_type)
    {
        case PVTC_IOSHIRE:
            pvt_print_ioshire_temperature_sampled_values();
            break;
        case PVTC_MINION_SHIRE:
            pvt_print_min_shire_temperature_sampled_values();
            break;
        case PVTC_PSHIRE:
        case PVTC_MEMSHIRE:
            break;
        default:
            break;
    }
}

int pvt_get_minion_avg_temperature(uint8_t *avg_temp)
{
    TS_Sample sample;
    int status;
    int avg = 0;
    int valid_samples_num = 0;

    for (int min = 0; min < PVTC_MINION_SHIRE_NUM; min++)
    {
        status = pvt_get_min_shire_ts_sample(min, &sample);
        if (0 == status)
        {
            avg = avg + sample.current;
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

int pvt_get_minion_avg_low_high_temperature(TS_Sample *temp)
{
    TS_Sample sample;
    int status;
    int avg = 0;
    int high = 0;
    int low = 0x0FFF;
    int valid_samples_num = 0;

    sample.high = 0;
    sample.low = 0x0FFF;

    for (int min = 0; min < PVTC_MINION_SHIRE_NUM; min++)
    {
        status = pvt_get_min_shire_ts_sample(min, &sample);
        if (0 == status)
        {
            avg = avg + sample.current;
            valid_samples_num++;
            high = MAX(sample.high, high);
            low = MIN(sample.low, low);
        }
    }

    if (valid_samples_num == 0)
    {
        Log_Write(LOG_LEVEL_WARNING, "There were no valid Minion Shire TS samples\r\n");
        return ERROR_PVT_NO_VALID_TS_SAMPLES;
    }

    avg = avg / valid_samples_num;

    temp->current = (int16_t)avg;
    temp->high = (int16_t)high;
    temp->low = (int16_t)low;

    return 0;
}

int pvt_get_minion_avg_low_high_voltage(MinShire_VM_sample *minshire_voltage)
{
    MinShire_VM_sample minshire_sample_avg = { { 0, 0, 0xFFFF },
                                               { 0, 0, 0xFFFF },
                                               { 0, 0, 0xFFFF } };
    int valid_samples_num = 0;

    GET_MINION_VM(mishire_sample_avg, valid_samples_num)

    if (valid_samples_num == 0)
    {
        Log_Write(LOG_LEVEL_WARNING, "There were no valid Minion Shire VM samples\r\n");
        return ERROR_PVT_NO_VALID_VM_SAMPLES;
    }

    *minshire_voltage = minshire_sample_avg;

    return 0;
}

int pvt_get_memshire_avg_low_high_voltage(MemShire_VM_sample *memshire_voltage)
{
    MemShire_VM_sample memshire_sample_avg = { { 0, 0, 0xFFFF }, { 0, 0, 0xFFFF } };
    int valid_samples_num = 0;

    GET_MEMSHIRE_VM(memshire_sample_avg, valid_samples_num)

    if (valid_samples_num == 0)
    {
        Log_Write(LOG_LEVEL_WARNING, "There were no valid Minion Shire TS samples\r\n");
        return ERROR_PVT_NO_VALID_TS_SAMPLES;
    }

    *memshire_voltage = memshire_sample_avg;

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

static int pvt_get_and_print_minshire(uint8_t print_ts, uint8_t print_vm, PVTC_MINSHIRE_e min_id,
                                      MinShire_samples *min_samples)
{
    int status;

    status = pvt_get_min_shire_ts_sample(min_id, &(min_samples->ts));
    if (0 != status)
    {
        Log_Write(LOG_LEVEL_WARNING, "PVT Sample fault!\r\n");
        return ERROR_PVT_SAMPLE_FAULT;
    }
    if (print_ts)
    {
        Log_Write(LOG_LEVEL_CRITICAL, "MS %2d Temp [C]: %d [%d, %d]\n", min_id,
                  min_samples->ts.current, min_samples->ts.low, min_samples->ts.high);
    }
    status = pvt_get_min_shire_vm_sample(min_id, &(min_samples->vm));
    if (0 != status)
    {
        Log_Write(LOG_LEVEL_WARNING, "PVT Sample fault!\r\n");
        return ERROR_PVT_SAMPLE_FAULT;
    }
    if (print_vm)
    {
        Log_Write(LOG_LEVEL_CRITICAL,
                  "MS %2d Voltage [mV]: VDD_MNN: %d [%d, %d]"
                  " VDD_SRAM: %d [%d, %d] VDD_NOC: %d [%d, %d]\n",
                  min_id, min_samples->vm.vdd_mnn.current, min_samples->vm.vdd_mnn.low,
                  min_samples->vm.vdd_mnn.high, min_samples->vm.vdd_sram.current,
                  min_samples->vm.vdd_sram.low, min_samples->vm.vdd_sram.high,
                  min_samples->vm.vdd_noc.current, min_samples->vm.vdd_noc.low,
                  min_samples->vm.vdd_noc.high);
    }

    return 0;
}

static int pvt_get_and_print_memshire(uint8_t print_vm, PVTC_MEMSHIRE_e mem_id,
                                      MemShire_samples *mem_samples)
{
    int status;

    status = pvt_get_memshire_vm_sample(mem_id, &(mem_samples->vm));
    if (0 != status)
    {
        Log_Write(LOG_LEVEL_WARNING, "PVT Sample fault!\r\n");
        return ERROR_PVT_SAMPLE_FAULT;
    }
    if (print_vm)
    {
        Log_Write(LOG_LEVEL_CRITICAL,
                  "MEM %d Voltage [mV]:"
                  " VDD_MS: %d [%d, %d] VDD_NOC: %d [%d, %d]\n",
                  mem_id, mem_samples->vm.vdd_ms.current, mem_samples->vm.vdd_ms.low,
                  mem_samples->vm.vdd_ms.high, mem_samples->vm.vdd_noc.current,
                  mem_samples->vm.vdd_noc.low, mem_samples->vm.vdd_noc.high);
    }

    return 0;
}

static int pvt_get_and_print_ioshire(uint8_t print_ts, uint8_t print_vm,
                                     IOShire_samples *io_samples)
{
    int status;

    status = pvt_get_ioshire_ts_sample(&(io_samples->ts));
    if (0 != status)
    {
        Log_Write(LOG_LEVEL_WARNING, "PVT Sample fault!\r\n");
        return ERROR_PVT_SAMPLE_FAULT;
    }
    if (print_ts)
    {
        Log_Write(LOG_LEVEL_CRITICAL, "IOSHIRE Temp [C]: %d [%d, %d]\n", io_samples->ts.current,
                  io_samples->ts.low, io_samples->ts.high);
    }
    status = pvt_get_ioshire_vm_sample(&(io_samples->vm));
    if (0 != status)
    {
        Log_Write(LOG_LEVEL_WARNING, "PVT Sample fault!\r\n");
        return ERROR_PVT_SAMPLE_FAULT;
    }
    if (print_vm)
    {
        Log_Write(LOG_LEVEL_CRITICAL,
                  "IOSHIRE Voltage [mV]: VDD_PU: %d [%d, %d]"
                  " VDD_NOC: %d [%d, %d]\n",
                  io_samples->vm.vdd_pu.current, io_samples->vm.vdd_pu.low,
                  io_samples->vm.vdd_pu.high, io_samples->vm.vdd_noc.current,
                  io_samples->vm.vdd_noc.low, io_samples->vm.vdd_noc.high);
    }

    return 0;
}

static int pvt_get_and_print_pshire(uint8_t print_vm, PShire_samples *pshr_samples)
{
    int status;

    status = pvt_get_pshire_vm_sample(&(pshr_samples->vm));
    if (0 != status)
    {
        Log_Write(LOG_LEVEL_WARNING, "PVT Sample fault!\r\n");
        return ERROR_PVT_SAMPLE_FAULT;
    }
    if (print_vm)
    {
        Log_Write(LOG_LEVEL_CRITICAL,
                  "PSHIRE Voltage [mV]:"
                  " VDD_PSHR: %d [%d, %d] VDD_NOC: %d [%d, %d]\n",
                  pshr_samples->vm.vdd_pshr.current, pshr_samples->vm.vdd_pshr.low,
                  pshr_samples->vm.vdd_pshr.high, pshr_samples->vm.vdd_noc.current,
                  pshr_samples->vm.vdd_noc.low, pshr_samples->vm.vdd_noc.high);
    }

    return 0;
}

static int pvt_get_and_print_memshire_all(uint8_t print_vm, All_MemShire_samples *mem_samples)
{
    int status;

    for (int mem_id = 0; mem_id < PVTC_MEM_SHIRE_NUM; mem_id++)
    {
        status = pvt_get_and_print_memshire(print_vm, mem_id, &(mem_samples->memshire[mem_id]));
        if (0 != status)
        {
            return status;
        }
    }

    return 0;
}

static int pvt_get_and_print_minshire_all(uint8_t print_ts, uint8_t print_vm,
                                          All_MinShire_samples *min_samples)
{
    int status;

    for (int min_id = 0; min_id < PVTC_MINION_SHIRE_NUM; min_id++)
    {
        status = pvt_get_and_print_minshire(print_ts, print_vm, min_id,
                                            &(min_samples->minshire[min_id]));
        if (0 != status)
        {
            return status;
        }
    }

    return 0;
}

static int pvt_get_and_print_all(uint8_t print_ts, uint8_t print_vm, All_PVT_samples *all_samples)
{
    int status;

    status = pvt_get_and_print_ioshire(print_ts, print_vm, &(all_samples->ioshire));
    if (0 != status)
    {
        return status;
    }

    status = pvt_get_and_print_pshire(print_vm, &(all_samples->pshire));
    if (0 != status)
    {
        return status;
    }

    status = pvt_get_and_print_memshire_all(print_vm,
                                            (All_MemShire_samples *)&(all_samples->memshire[0]));
    if (0 != status)
    {
        return status;
    }

    status = pvt_get_and_print_minshire_all(print_ts, print_vm,
                                            (All_MinShire_samples *)&(all_samples->minshire[0]));
    if (0 != status)
    {
        return status;
    }

    return 0;
}

int pvt_get_and_print(uint8_t print_ts, uint8_t print_vm, PVT_PRINT_e print_select, uint16_t *data,
                      uint32_t *num_bytes)
{
    int status;

    switch (print_select)
    {
        case PVT_PRINT_MINSHIRE_0 ... PVT_PRINT_MINSHIRE_33:
            status = pvt_get_and_print_minshire(print_ts, print_vm, (uint8_t)print_select,
                                                (MinShire_samples *)data);
            *num_bytes = sizeof(MinShire_samples);
            break;
        case PVT_PRINT_MEMSHIRE_232 ... PVT_PRINT_MEMSHIRE_239:
            status = pvt_get_and_print_memshire(print_vm,
                                                (uint8_t)(print_select - PVT_PRINT_MEMSHIRE_232),
                                                (MemShire_samples *)data);
            *num_bytes = sizeof(MemShire_samples);
            break;
        case PVT_PRINT_IOSHIRE_254:
            status = pvt_get_and_print_ioshire(print_ts, print_vm, (IOShire_samples *)data);
            *num_bytes = sizeof(IOShire_samples);
            break;
        case PVT_PRINT_PSHIRE_253:
            status = pvt_get_and_print_pshire(print_vm, (PShire_samples *)data);
            *num_bytes = sizeof(PShire_samples);
            break;
        case PVT_PRINT_MEMSHIRE_ALL:
            status = pvt_get_and_print_memshire_all(print_vm, (All_MemShire_samples *)data);
            *num_bytes = sizeof(All_MemShire_samples);
            break;
        case PVT_PRINT_MINSHIRE_ALL:
            status =
                pvt_get_and_print_minshire_all(print_ts, print_vm, (All_MinShire_samples *)data);
            *num_bytes = sizeof(All_MinShire_samples);
            break;
        case PVT_PRINT_ALL:
            status = pvt_get_and_print_all(print_ts, print_vm, (All_PVT_samples *)data);
            *num_bytes = sizeof(All_PVT_samples);
            break;
        default:
            break;
    }

    return status;
}
