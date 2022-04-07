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
#ifndef __BL2_PVT_CONTROLLER__
#define __BL2_PVT_CONTROLLER__

#include <stdint.h>
#include "bl_error_code.h"
#include <hwinc/pvtc_sda.h>
#include <hwinc/pvt_ts.h>
#include <hwinc/pvt_pd.h>
#include <hwinc/pvt_vm.h>

/*! \def PVTC_NUM
    \brief Number of PVTC controllers
*/
#define PVTC_NUM 5

/*! \def PVTC_TS_NUM
    \brief Number of Temperature Sensors per controller
*/
#define PVTC_TS_NUM 8

/*! \def PVTC_VM_NUM
    \brief Number of Voltage Monitors per controller
*/
#define PVTC_VM_NUM 2

/*! \def PVTC_PD_NUM
    \brief Number of Process Detectors per controller
*/
#define PVTC_PD_NUM 8

/*! \def PVTC_VM_CH_NUM
    \brief Number of channels per VM
*/
#define PVTC_VM_CH_NUM 16

/*! \def PVTC_CLK_SYNTH_STROBE
    \brief Clock synth strobe value for adjusting SDIF drive and sample points
*/
#define PVTC_CLK_SYNTH_STROBE 0x3u

/*! \def PVTC_CLK_SYNTH_HI
    \brief IP clock high period duration in apb_ios (100MHz) cycle units
*/
#define PVTC_CLK_SYNTH_HI 0x06u

/*! \def PVTC_CLK_SYNTH_LO
    \brief IP clock low period duration in apb_ios (100MHz) cycle units
*/
#define PVTC_CLK_SYNTH_LO 0x06u

/*! \def PVTC_TS_PWR_UP_DELAY
    \brief TS power up delay (typical 256 cycles)
*/
#define PVTC_TS_PWR_UP_DELAY 256

/*! \def PVTC_PD_PWR_UP_DELAY
    \brief PD power up delay (typical 0 cycles)
*/
#define PVTC_PD_PWR_UP_DELAY 0

/*! \def PVTC_VM_PWR_UP_DELAY
    \brief VM power up delay (typical 64 cycles)
*/
#define PVTC_VM_PWR_UP_DELAY 64

/*! \def PVTC_MINION_SHIRE_NUM
    \brief Number of minion shires
*/
#define PVTC_MINION_SHIRE_NUM 34

/*! \def PVTC_MEM_SHIRE_NUM
    \brief Number of mamoery shires
*/
#define PVTC_MEM_SHIRE_NUM 8

/*! \def PVTC_EXT_ANALOG_VM_NUM
    \brief Number of external analog VM connections
*/
#define PVTC_EXT_ANALOG_VM_NUM 2

/* PVT sensors configuration */

/*! \def PVT_TS_RESOLUTION
    \brief PVT TS resolution (12, 10 or 8 bits)
*/
#define PVT_TS_RESOLUTION PVT_TS_RESOLUTION_RES_12_BITS

/*! \def PVT_TS_RUN_MODE
    \brief PVT TS run mode (RUN_1 mode doesn't need calibration)
*/
#define PVT_TS_RUN_MODE PVT_TS_RUN_MODE_RUN_1

/*! \def PVT_PD_RUN_MODE
    \brief PVT PD run mode
*/
#define PVT_PD_RUN_MODE PVT_PD_RUN_MODE_SIG

/*! \def PVT_PD_INT_DELAY_CHAIN_21_ALWAYS_ON
    \brief PVT PD internal delay chain 21 always on
*/
#define PVT_PD_INT_DELAY_CHAIN_21_ALWAYS_ON 0

/*! \def PVT_PD_INT_DELAY_CHAIN_20_ALWAYS_ON
    \brief PVT PD internal delay chain 20 always on
*/
#define PVT_PD_INT_DELAY_CHAIN_20_ALWAYS_ON 0

/*! \def PVT_PD_INT_DELAY_CHAIN_19_ALWAYS_ON
    \brief PVT PD internal delay chain 19 always on
*/
#define PVT_PD_INT_DELAY_CHAIN_19_ALWAYS_ON 0

/*! \def PVT_PD_OSC_SELECT
    \brief PVT PD oscillator select
*/
#define PVT_PD_OSC_SELECT PVT_PD_OSC_SELECT_MEASUREMENT_DISABLED

/*! \def PVT_PD_OSC_CNT_GATE
    \brief PVT PD oscillator counter gate value
*/
#define PVT_PD_OSC_CNT_GATE PVT_PD_OSC_CNT_GATE_CYCLES_31

/*! \def PVT_PD_OSC_CNT_PRE_SCALER
    \brief PVT PD oscillator counter prescaler
*/
#define PVT_PD_OSC_CNT_PRE_SCALER PVT_PD_OSC_CNT_DIVIDE_RATIO_1

/*! \def PVT_VM_RESOLUTION
    \brief PVT VM resolution (14, 12, 10 or 8 bits)
*/
#define PVT_VM_RESOLUTION PVT_VM_RESOLUTION_RES_14_BITS

/*! \def PVT_VM_RUN_MODE
    \brief PVT VM run mode (RUN_0 mode doesn't need calibration)
*/
#define PVT_VM_RUN_MODE PVT_VM_RUN_MODE_RUN_0

/*! \def PVT_VM_CH_SELECT
    \brief PVT VM channle select (since we are using channel polling this is not relevant)
*/
#define PVT_VM_CH_SELECT PVT_VM_CH_SELECT_INPUT_0

/*! \def PVT_VM_CH_ENABLE_MASK
    \brief PVT VM channle enable mask
*/
#define PVT_VM_CH_ENABLE_MASK 0x0000FFFFu

/*! \def PVT_TS_G_PARAMETER
    \brief PVT TS G parameter used for conversion of digital output
*/
#define PVT_TS_G_PARAMETER 57400

/*! \def PVT_TS_H_PARAMETER
    \brief PVT TS H parameter used for conversion of digital output
*/
#define PVT_TS_H_PARAMETER 249400

/*! \def PVT_TS_CAL5_PARAMETER
    \brief PVT TS CAL5 parameter used for conversion of digital output
*/
#define PVT_TS_CAL5_PARAMETER 4096

/*! \def PVT_VM_VREF_PARAMETER
    \brief PVT VM VREF parameter used for conversion of digital output
*/
#define PVT_VM_VREF_PARAMETER 1220700

/*! \struct PVTC_IP_disable_mask
    \brief Struct for holding disable masks
*/
typedef struct
{
    uint8_t ts_disable_mask;
    uint8_t pd_disable_mask;
    uint8_t vm_disable_mask;
} PVTC_IP_disable_mask;

/*! \struct PVTC_VM_enable_mask
    \brief Struct for holding VM enable masks
*/
typedef struct
{
    uint16_t vm_enable_mask[PVTC_VM_NUM];
} PVTC_VM_enable_mask;

/*! \struct PVTC_VM_mapping
    \brief Struct for holding VM mapping
*/
typedef struct
{
    uint8_t pvtc_id;
    uint8_t vm_id;
    uint8_t ch_id;
} PVTC_VM_mapping;

/*! \struct TS_Sample
    \brief Struct for holding current, high and low value
*/
typedef struct
{
    int16_t current;
    int16_t high;
    int16_t low;
} TS_Sample;

/*! \struct PD_Sample
    \brief Struct for holding current, high and low value
*/
typedef struct
{
    uint16_t current;
    uint16_t high;
    uint16_t low;
} PD_Sample;

/*! \struct VM_Sample
    \brief Struct for holding VM current, high and low value
*/
typedef struct
{
    uint16_t current;
    uint16_t high;
    uint16_t low;
} VM_Sample;

/*! \struct MinShire_VM_sample
    \brief Struct for holding Minion Voltage sampled values
*/
typedef struct
{
    VM_Sample vdd_sram;
    VM_Sample vdd_noc;
    VM_Sample vdd_mnn;
} MinShire_VM_sample;

/*! \struct IOShire_VM_sample
    \brief Struct for holding IOShire Voltage sampled values
*/
typedef struct
{
    VM_Sample vdd_noc;
    VM_Sample vdd_pu;
    VM_Sample vdd_mxn;
} IOShire_VM_sample;

/*! \struct PShire_VM_sample
    \brief Struct for holding PShire Voltage sampled values
*/
typedef struct
{
    VM_Sample vdd_pshr;
    VM_Sample vdd_noc;
} PShire_VM_sample;

/*! \struct MemShire_VM_sample
    \brief Struct for holding MemShire Voltage sampled values
*/
typedef struct
{
    VM_Sample vdd_ms;
    VM_Sample vdd_noc;
} MemShire_VM_sample;

/*! \struct ExtAnalog_VM_sample
    \brief Struct for holding External Analog Voltage sampled values
*/
typedef struct
{
    VM_Sample vdd_ext_analog;
} ExtAnalog_VM_sample;

/*! \struct IOShire_samples
    \brief Struct for holding IOShire Temperature and Voltage sampled values
*/
typedef struct
{
    TS_Sample ts;
    IOShire_VM_sample vm;
} IOShire_samples;

/*! \struct PShire_samples
    \brief Struct for holding PShire Temperature and Voltage sampled values
*/
typedef struct
{
    PShire_VM_sample vm;
} PShire_samples;

/*! \struct MemShire_samples
    \brief Struct for holding MemShire Temperature and Voltage sampled values
*/
typedef struct
{
    MemShire_VM_sample vm;
} MemShire_samples;

/*! \struct MinShire_samples
    \brief Struct for holding MinShire Temperature and Voltage sampled values
*/
typedef struct
{
    TS_Sample ts;
    MinShire_VM_sample vm;
} MinShire_samples;

/*! \struct All_MinShire_samples
    \brief Struct for holding All MinShire Temperature and Voltage sampled values
*/
typedef struct
{
    MinShire_samples minshire[PVTC_MINION_SHIRE_NUM];
} All_MinShire_samples;

/*! \struct All_MemShire_samples
    \brief Struct for holding All MemShire Temperature and Voltage sampled values
*/
typedef struct
{
    MemShire_samples memshire[PVTC_MEM_SHIRE_NUM];
} All_MemShire_samples;

/*! \struct All_PVT_samples
    \brief Struct for holding All PVT Temperature and Voltage sampled values
*/
typedef struct
{
    IOShire_samples ioshire;
    PShire_samples pshire;
    MemShire_samples memshire[PVTC_MEM_SHIRE_NUM];
    MinShire_samples minshire[PVTC_MINION_SHIRE_NUM];
} All_PVT_samples;

/*! \enum shire_type_t
    \brief Enums for shire types
*/
typedef enum pvtc_shire_type
{
    PVTC_IOSHIRE = 0,
    PVTC_PSHIRE = 1,
    PVTC_MEMSHIRE = 2,
    PVTC_MINION_SHIRE = 3
} pvtc_shire_type_t;

/*! \enum pvtc_id_t
    \brief Enums for pvtc id
*/
typedef enum pvtc_id
{
    PVTC_0 = 0,
    PVTC_1,
    PVTC_2,
    PVTC_3,
    PVTC_4
} pvtc_id_t;

/*! \enum pvtc_vm_id_t
    \brief Enums for pvtc vm id
*/
typedef enum pvtc_vm_id
{
    VM_0 = 0,
    VM_1 = 1,
    VM_2 = 0,
    VM_3 = 1,
    VM_4 = 0,
    VM_5 = 1,
    VM_6 = 0,
    VM_7 = 1
} pvtc_vm_id_t;

/*! \enum pvtc_vm_chan_id_t
    \brief Enums for pvtc vm channel id
*/
typedef enum pvtc_vm_chan_id
{
    CHAN_0 = 0,
    CHAN_1,
    CHAN_2,
    CHAN_3,
    CHAN_4,
    CHAN_5,
    CHAN_6,
    CHAN_7,
    CHAN_8,
    CHAN_9,
    CHAN_10,
    CHAN_11,
    CHAN_12,
    CHAN_13,
    CHAN_14,
    CHAN_15
} pvtc_vm_chan_id_t;

/*! \enum PVTC_MINSHIRE_e
    \brief Enums for MINSHIREs that are monitored by PVTC
*/
typedef enum pvtcMinshire
{
    PVTC_MINSHIRE_0 = 0,
    PVTC_MINSHIRE_1 = 1,
    PVTC_MINSHIRE_2 = 2,
    PVTC_MINSHIRE_3 = 3,
    PVTC_MINSHIRE_4 = 4,
    PVTC_MINSHIRE_5 = 5,
    PVTC_MINSHIRE_6 = 6,
    PVTC_MINSHIRE_7 = 7,
    PVTC_MINSHIRE_8 = 8,
    PVTC_MINSHIRE_9 = 9,
    PVTC_MINSHIRE_10 = 10,
    PVTC_MINSHIRE_11 = 11,
    PVTC_MINSHIRE_12 = 12,
    PVTC_MINSHIRE_13 = 13,
    PVTC_MINSHIRE_14 = 14,
    PVTC_MINSHIRE_15 = 15,
    PVTC_MINSHIRE_16 = 16,
    PVTC_MINSHIRE_17 = 17,
    PVTC_MINSHIRE_18 = 18,
    PVTC_MINSHIRE_19 = 19,
    PVTC_MINSHIRE_20 = 20,
    PVTC_MINSHIRE_21 = 21,
    PVTC_MINSHIRE_22 = 22,
    PVTC_MINSHIRE_23 = 23,
    PVTC_MINSHIRE_24 = 24,
    PVTC_MINSHIRE_25 = 25,
    PVTC_MINSHIRE_26 = 26,
    PVTC_MINSHIRE_27 = 27,
    PVTC_MINSHIRE_28 = 28,
    PVTC_MINSHIRE_29 = 29,
    PVTC_MINSHIRE_30 = 30,
    PVTC_MINSHIRE_31 = 31,
    PVTC_MINSHIRE_32 = 32,
    PVTC_MINSHIRE_33 = 33,
    PVTC_MAX_MINSHIRE_ID = PVTC_MINSHIRE_33,
    PVTC_IOSHIRE_TS_PD_ID = 34
} PVTC_MINSHIRE_e;

/*! \enum PVTC_MEMSHIRE_e
    \brief Enums for MEMSHIREs that are monitored by PVTC
*/
typedef enum pvtcMemshire
{
    PVTC_MEMSHIRE_0 = 0,
    PVTC_MEMSHIRE_1 = 1,
    PVTC_MEMSHIRE_2 = 2,
    PVTC_MEMSHIRE_3 = 3,
    PVTC_MEMSHIRE_4 = 4,
    PVTC_MEMSHIRE_5 = 5,
    PVTC_MEMSHIRE_6 = 6,
    PVTC_MEMSHIRE_7 = 7,
    PVTC_MAX_MEMSHIRE_ID = PVTC_MEMSHIRE_7
} PVTC_MEMSHIRE_e;

/*! \enum PVTC_EXT_ANALOG_e
    \brief Enums for MEMSHIREs that are monitored by PVTC
*/
typedef enum pvtcExtAnalog
{
    PVTC_EXT_ANALOG_0 = 0,
    PVTC_EXT_ANALOG_1 = 1,
    PVTC_MAX_EXT_ANALOG_ID = PVTC_EXT_ANALOG_1
} PVTC_EXT_ANALOG_e;

/*! \enum PVTC_MINSHIRE_e
    \brief Enums for MINSHIREs that are monitored by PVTC
*/
typedef enum PVT_PRINT
{
    PVT_PRINT_MINSHIRE_0 = 0,
    PVT_PRINT_MINSHIRE_1 = 1,
    PVT_PRINT_MINSHIRE_2 = 2,
    PVT_PRINT_MINSHIRE_3 = 3,
    PVT_PRINT_MINSHIRE_4 = 4,
    PVT_PRINT_MINSHIRE_5 = 5,
    PVT_PRINT_MINSHIRE_6 = 6,
    PVT_PRINT_MINSHIRE_7 = 7,
    PVT_PRINT_MINSHIRE_8 = 8,
    PVT_PRINT_MINSHIRE_9 = 9,
    PVT_PRINT_MINSHIRE_10 = 10,
    PVT_PRINT_MINSHIRE_11 = 11,
    PVT_PRINT_MINSHIRE_12 = 12,
    PVT_PRINT_MINSHIRE_13 = 13,
    PVT_PRINT_MINSHIRE_14 = 14,
    PVT_PRINT_MINSHIRE_15 = 15,
    PVT_PRINT_MINSHIRE_16 = 16,
    PVT_PRINT_MINSHIRE_17 = 17,
    PVT_PRINT_MINSHIRE_18 = 18,
    PVT_PRINT_MINSHIRE_19 = 19,
    PVT_PRINT_MINSHIRE_20 = 20,
    PVT_PRINT_MINSHIRE_21 = 21,
    PVT_PRINT_MINSHIRE_22 = 22,
    PVT_PRINT_MINSHIRE_23 = 23,
    PVT_PRINT_MINSHIRE_24 = 24,
    PVT_PRINT_MINSHIRE_25 = 25,
    PVT_PRINT_MINSHIRE_26 = 26,
    PVT_PRINT_MINSHIRE_27 = 27,
    PVT_PRINT_MINSHIRE_28 = 28,
    PVT_PRINT_MINSHIRE_29 = 29,
    PVT_PRINT_MINSHIRE_30 = 30,
    PVT_PRINT_MINSHIRE_31 = 31,
    PVT_PRINT_MINSHIRE_32 = 32,
    PVT_PRINT_MINSHIRE_33 = 33,
    PVT_PRINT_MINSHIRE_ALL = 34,
    PVT_PRINT_MEMSHIRE_232 = 232,
    PVT_PRINT_MEMSHIRE_233 = 233,
    PVT_PRINT_MEMSHIRE_234 = 234,
    PVT_PRINT_MEMSHIRE_235 = 235,
    PVT_PRINT_MEMSHIRE_236 = 236,
    PVT_PRINT_MEMSHIRE_237 = 237,
    PVT_PRINT_MEMSHIRE_238 = 238,
    PVT_PRINT_MEMSHIRE_239 = 239,
    PVT_PRINT_MEMSHIRE_ALL = 240,
    PVT_PRINT_PSHIRE_253 = 253,
    PVT_PRINT_IOSHIRE_254 = 254,
    PVT_PRINT_ALL = 255
} PVT_PRINT_e;

/*! \fn int pvt_init(void)
    \brief This function inits pvt controllers.
    \param none
    \return Status indicating success or negative error
*/
int pvt_init(void);

/*! \fn int pvt_get_min_shire_ts_sample(PVTC_MINSHIRE_e min_id, TS_Sample *ts_sample)
    \brief This function gets TS sample for Minion Shire
    \param min_id Minion Shire ID
    \param ts_sample sample to be stored
    \return Status indicating success or negative error
*/
int pvt_get_min_shire_ts_sample(PVTC_MINSHIRE_e min_id, TS_Sample *ts_sample);

/*! \fn int pvt_get_ioshire_ts_sample(TS_Sample *ts_sample)
    \brief This function gets TS sample for IOShire
    \param ts_sample sample to be stored
    \return Status indicating success or negative error
*/
int pvt_get_ioshire_ts_sample(TS_Sample *ts_sample);

/*! \fn int pvt_get_min_shire_pd_sample(PVTC_MINSHIRE_e min_id, PD_Sample *pd_sample)
    \brief This function gets PD sample for Minion Shire
    \param min_id Minion Shire ID
    \param pd_sample sample to be stored
    \return Status indicating success or negative error
*/
int pvt_get_min_shire_pd_sample(PVTC_MINSHIRE_e min_id, PD_Sample *pd_sample);

/*! \fn int pvt_get_ioshire_pd_sample(PD_Sample *pd_sample)
    \brief This function gets TS sample for IOShire
    \param pd_sample sample to be stored
    \return Status indicating success or negative error
*/
int pvt_get_ioshire_pd_sample(PD_Sample *pd_sample);

/*! \fn int pvt_get_min_shire_vm_sample(PVTC_MINSHIRE_e min_id, MinShire_VM_sample *vm_sample)
    \brief This function gets VM sample for Minion Shire
    \param min_id Minion Shire ID
    \param vm_sample sample to be stored
    \return Status indicating success or negative error
*/
int pvt_get_min_shire_vm_sample(PVTC_MINSHIRE_e min_id, MinShire_VM_sample *vm_sample);

/*! \fn int pvt_get_memshire_vm_sample(PVTC_MEMSHIRE_e memshire_id, MemShire_VM_sample *vm_sample)
    \brief This function gets VM sample for Mem Shire
    \param memshire_id Mem Shire ID
    \param vm_sample sample to be stored
    \return Status indicating success or negative error
*/
int pvt_get_memshire_vm_sample(PVTC_MEMSHIRE_e memshire_id, MemShire_VM_sample *vm_sample);

/*! \fn int pvt_get_ioshire_vm_sample(IOShire_VM_sample *vm_sample)
    \brief This function gets VM sample for IOShire
    \param vm_sample sample to be stored
    \return Status indicating success or negative error
*/
int pvt_get_ioshire_vm_sample(IOShire_VM_sample *vm_sample);

/*! \fn int pvt_get_pshire_vm_sample(PShire_VM_sample *vm_sample)
    \brief This function gets VM sample for PShire
    \param vm_sample sample to be stored
    \return Status indicating success or negative error
*/
int pvt_get_pshire_vm_sample(PShire_VM_sample *vm_sample);

/*! \fn int pvt_get_ext_analog_vm_sample(PVTC_EXT_ANALOG_e ext_an_id, ExtAnalog_VM_sample *vm_sample)
    \brief This function gets VM sample for External Analog
    \param ext_an_id External Analog ID
    \param vm_sample sample to be stored
    \return Status indicating success or negative error
*/
int pvt_get_ext_analog_vm_sample(PVTC_EXT_ANALOG_e ext_an_id, ExtAnalog_VM_sample *vm_sample);

/*! \fn int pvt_single_sample_run(void)
    \brief This function triggers single sample run on all sensors
    \param none
    \return none
*/
void pvt_single_sample_run(void);

/*! \fn int pvt_hilo_reset(void)
    \brief This function reset high and low sampled values of all sensors
    \param none
    \return Status indicating success or negative error
*/
int pvt_hilo_reset(void);

/*! \fn void pvt_continuous_sample_run(void)
    \brief This function triggers continuous sample run on all sensors
    \param none
    \return none
*/
void pvt_continuous_sample_run(void);

/*! \fn void pvt_print_temperature_sampled_values(pvtc_shire_type_t shire_type)
    \brief This function prints TS sampled values from sensors
    \param shire_type Shire type to fetch values
    \return none
*/
void pvt_print_temperature_sampled_values(pvtc_shire_type_t shire_type);

/*! \fn void pvt_print_voltage_sampled_values(pvtc_shire_type_t shire_type)
    \brief This function prints VM sampled values from sensors
    \param shire_type Shire type to fetch values
    \return none
*/
void pvt_print_voltage_sampled_values(pvtc_shire_type_t shire_type);

/*! \fn int pvt_get_minion_avg_temperature(uint8_t* avg_temp)
    \brief This function returns average temperature for Minion shires
    \param avg_temp place holder for return value
    \return Status indicating success or negative error
*/
int pvt_get_minion_avg_temperature(uint8_t *avg_temp);

/*! \fn void pvt_print_all(void)
    \brief This function prints sampled values from all VM and TS
    \param none
    \return none
*/
void pvt_print_all(void);

/*! \fn int pvt_get_and_print(uint8_t print_ts, uint8_t print_vm, PVT_PRINT_e print_select,
*                             uint16_t *data, uint32_t *num_bytes)
    \brief This function gets and prints sampled values from selected VMs and TSs
    \param print_ts      Print TS values
    \param print_vm      Print VM values
    \param print_select  Select what to get/print
    \param data          Placeholder for sampled values
    \param num_bytes     Number of valid bytes in data
    \return Status indicating success or negative error
*/
int pvt_get_and_print(uint8_t print_ts, uint8_t print_vm, PVT_PRINT_e print_select, uint16_t *data,
                      uint32_t *num_bytes);

/*! \fn int pvt_get_minion_avg_low_high_temperature(TS_Sample* temp)
    \brief This function returns average, high and low temperature for Minion shires
    \param temp place holder for return values
    \return Status indicating success or negative error
*/
int pvt_get_minion_avg_low_high_temperature(TS_Sample *temp);

/*! \fn int pvt_get_minion_avg_low_high_voltage(MinShire_VM_sample* minshire_voltage)
    \brief This function returns average, high and low voltage for Minion shires
    \param minshire_voltage place holder for return values
    \return Status indicating success or negative error
*/
int pvt_get_minion_avg_low_high_voltage(MinShire_VM_sample *minshire_voltage);

/*! \fn int pvt_get_memshire_avg_low_high_voltage(MemShire_VM_sample* memshire_voltage)
    \brief This function returns average, high and low voltage for Mem shires
    \param memshire_voltage place holder for return values
    \return Status indicating success or negative error
*/
int pvt_get_memshire_avg_low_high_voltage(MemShire_VM_sample *memshire_voltage);

#endif
