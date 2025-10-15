/* Copyright (c) 2025 Ainekko, Co. */
/* SPDX-License-Identifier: Apache-2.0 */
/*                                                                         */
// From commit 6b2223de8e142bc49820a0b3b81badfa05f5b67b in esperanto-soc repository

#ifndef LVDPLL_DEFINES_H
#define LVDPLL_DEFINES_H

#ifndef LOCK_TIMEOUT
    #define LOCK_TIMEOUT 2000
#endif

#ifndef BUSY_TIMEOUT
    #define BUSY_TIMEOUT 1000
#endif

#ifndef REG_STROBE_UPDATE
    #define REG_STROBE_UPDATE 0x1
#endif

#ifndef SELECT_REF_CLOCK
    #define SELECT_REF_CLOCK 0x8
#endif

#ifndef SELECT_STEP_CLOCK
    #define SELECT_STEP_CLOCK 0xb
#endif

#ifndef SELECT_PLL_CLOCK_0
    #define SELECT_PLL_CLOCK_0 0xc
#endif

#ifndef FCW_INT_OFFSET
    #define FCW_INT_OFFSET 0x02
#endif

#ifndef FCW_FRAC_OFFSET
    #define FCW_FRAC_OFFSET 0x03
#endif

#ifndef POSTDIV0_OFFSET
    #define POSTDIV0_OFFSET 0x0e
#endif

#ifndef REG_STROBE_UPDATE_OFFSET
    #define REG_STROBE_UPDATE_OFFSET 0x38
#endif

#ifndef LOCAL_SHIRE_ID
    #define LOCAL_SHIRE_ID 0xff
#endif

#ifndef AUTO_NORMALIZATION_ON
    #define AUTO_NORMALIZATION_ON 0x01b8
#endif

#ifndef AUTO_NORMALIZATION_OFF
    #define AUTO_NORMALIZATION_OFF 0x81b8
#endif

#ifndef MODE_NUMBER_OF_MODES
    #define MODE_NUMBER_OF_MODES 45
#endif

#ifndef MODE_FREQUENCY_STEP_SIZE
    #define MODE_FREQUENCY_STEP_SIZE 25
#endif

#ifndef MODE_FREQUENCY_STEP_BASE
    #define MODE_FREQUENCY_STEP_BASE 275
#endif

#define WAIT_DLL_LOCK(shire_id)                                                                                        \
    int timeout = LOCK_TIMEOUT;                                                                                        \
    while (timeout > 0) {                                                                                              \
        if (ETSOC_SHIRE_OTHER_ESR_SHIRE_DLL_READ_DATA_LOCKED_GET(read_esr_new(PP_MACHINE, shire_id,                    \
                REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_DLL_READ_DATA_ADDRESS, 0))) {     \
            return 0;                                                                                                  \
        }                                                                                                              \
        --timeout;                                                                                                     \
    }                                                                                                                  \
    return -1;

#define WAIT_PLL_LOCK(shire_id)                                                                                        \
    int timeout = LOCK_TIMEOUT;                                                                                        \
    while (timeout > 0) {                                                                                              \
        if (ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_READ_DATA_LOCKED_GET(read_esr_new(PP_MACHINE, shire_id,                    \
                REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_READ_DATA_ADDRESS, 0))) {     \
            return 0;                                                                                                  \
        }                                                                                                              \
        --timeout;                                                                                                     \
    }                                                                                                                  \
    return -1;

#define WAIT_BUSY(shire_id)                                                                                            \
    int timeout = BUSY_TIMEOUT;                                                                                        \
    while (timeout > 0) {                                                                                              \
        if (!(ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_READ_DATA_BUSY_GET(read_esr_new(PP_MACHINE, shire_id,                    \
                REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_READ_DATA_ADDRESS, 0)))) {    \
            break;                                                                                                     \
        }                                                                                                              \
        --timeout;                                                                                                     \
    }                                                                                                                  \
    if(timeout == 0) return -1;

#define SWITCH_CLOCK_MUX(shire_id, clock)                                                                              \
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER,                                       \
                    ETSOC_SHIRE_OTHER_ESR_SHIRE_CTRL_CLOCKMUX_ADDRESS, clock, 0);


// Bellow defines must be set according to characterization

#ifndef DVFS_LOCK_TIME_WAIT
    #define LVDPLL_LOCK_TIME_WAIT 120
#endif

#ifndef DVFS_BUSY_TIME_WAIT
    #define LVDPLL_BUSY_TIME_WAIT 10
#endif

#ifndef DVFS_POLL_FOR_LOCK
    #define DVFS_POLL_FOR_LOCK false
#endif

#ifndef DVFS_POLL_FOR_BUSY
    #define DVFS_POLL_FOR_BUSY false
#endif

#ifndef DVFS_NORMALIZATION_OFF
    #define DVFS_NORMALIZATION_OFF 0
#endif

#ifndef DVFS_USE_FCW
    #define DVFS_USE_FCW 0
#endif

typedef enum {
    LVDPLL_LDO_NO_UPDATE,
    LVDPLL_LDO_ENABLE,
    LVDPLL_LDO_DISABLE,
    LVDPLL_LDO_BYPASS,
    LVDPLL_LDO_KICK
} LVDPLL_LDO_UPDATE_t;

typedef enum lvdpllMode
{
    MODE_i100_o300MHz = 0x1,
    MODE_i100_o325MHz,
    MODE_i100_o350MHz,
    MODE_i100_o375MHz,
    MODE_i100_o400MHz,
    MODE_i100_o425MHz,
    MODE_i100_o450MHz,
    MODE_i100_o475MHz,
    MODE_i100_o500MHz,
    MODE_i100_o525MHz,
    MODE_i100_o550MHz,
    MODE_i100_o575MHz,
    MODE_i100_o600MHz,
    MODE_i100_o625MHz,
    MODE_i100_o650MHz,
    MODE_i100_o675MHz,
    MODE_i100_o700MHz,
    MODE_i100_o725MHz,
    MODE_i100_o750MHz,
    MODE_i100_o775MHz,
    MODE_i100_o800MHz,
    MODE_i100_o825MHz,
    MODE_i100_o850MHz,
    MODE_i100_o875MHz,
    MODE_i100_o900MHz,
    MODE_i100_o925MHz,
    MODE_i100_o950MHz,
    MODE_i100_o975MHz,
    MODE_i100_o1000MHz,
    MODE_i100_o1025MHz,
    MODE_i100_o1050MHz,
    MODE_i100_o1075MHz,
    MODE_i100_o1100MHz,
    MODE_i100_o1125MHz,
    MODE_i100_o1150MHz,
    MODE_i100_o1175MHz,
    MODE_i100_o1200MHz,
    MODE_i100_o1225MHz,
    MODE_i100_o1250MHz,
    MODE_i100_o1275MHz,
    MODE_i100_o1300MHz,
    MODE_i100_o1325MHz,
    MODE_i100_o1350MHz,
    MODE_i100_o1375MHz,
    MODE_i100_o1400MHz,
    MODE_i24_o300MHz,
    MODE_i24_o325MHz,
    MODE_i24_o350MHz,
    MODE_i24_o375MHz,
    MODE_i24_o400MHz,
    MODE_i24_o425MHz,
    MODE_i24_o450MHz,
    MODE_i24_o475MHz,
    MODE_i24_o500MHz,
    MODE_i24_o525MHz,
    MODE_i24_o550MHz,
    MODE_i24_o575MHz,
    MODE_i24_o600MHz,
    MODE_i24_o625MHz,
    MODE_i24_o650MHz,
    MODE_i24_o675MHz,
    MODE_i24_o700MHz,
    MODE_i24_o725MHz,
    MODE_i24_o750MHz,
    MODE_i24_o775MHz,
    MODE_i24_o800MHz,
    MODE_i24_o825MHz,
    MODE_i24_o850MHz,
    MODE_i24_o875MHz,
    MODE_i24_o900MHz,
    MODE_i24_o925MHz,
    MODE_i24_o950MHz,
    MODE_i24_o975MHz,
    MODE_i24_o1000MHz,
    MODE_i24_o1025MHz,
    MODE_i24_o1050MHz,
    MODE_i24_o1075MHz,
    MODE_i24_o1100MHz,
    MODE_i24_o1125MHz,
    MODE_i24_o1150MHz,
    MODE_i24_o1175MHz,
    MODE_i24_o1200MHz,
    MODE_i24_o1225MHz,
    MODE_i24_o1250MHz,
    MODE_i24_o1275MHz,
    MODE_i24_o1300MHz,
    MODE_i24_o1325MHz,
    MODE_i24_o1350MHz,
    MODE_i24_o1375MHz,
    MODE_i24_o1400MHz,
    MODE_i40_o300MHz,
    MODE_i40_o325MHz,
    MODE_i40_o350MHz,
    MODE_i40_o375MHz,
    MODE_i40_o400MHz,
    MODE_i40_o425MHz,
    MODE_i40_o450MHz,
    MODE_i40_o475MHz,
    MODE_i40_o500MHz,
    MODE_i40_o525MHz,
    MODE_i40_o550MHz,
    MODE_i40_o575MHz,
    MODE_i40_o600MHz,
    MODE_i40_o625MHz,
    MODE_i40_o650MHz,
    MODE_i40_o675MHz,
    MODE_i40_o700MHz,
    MODE_i40_o725MHz,
    MODE_i40_o750MHz,
    MODE_i40_o775MHz,
    MODE_i40_o800MHz,
    MODE_i40_o825MHz,
    MODE_i40_o850MHz,
    MODE_i40_o875MHz,
    MODE_i40_o900MHz,
    MODE_i40_o925MHz,
    MODE_i40_o950MHz,
    MODE_i40_o975MHz,
    MODE_i40_o1000MHz,
    MODE_i40_o1025MHz,
    MODE_i40_o1050MHz,
    MODE_i40_o1075MHz,
    MODE_i40_o1100MHz,
    MODE_i40_o1125MHz,
    MODE_i40_o1150MHz,
    MODE_i40_o1175MHz,
    MODE_i40_o1200MHz,
    MODE_i40_o1225MHz,
    MODE_i40_o1250MHz,
    MODE_i40_o1275MHz,
    MODE_i40_o1300MHz,
    MODE_i40_o1325MHz,
    MODE_i40_o1350MHz,
    MODE_i40_o1375MHz,
    MODE_i40_o1400MHz
} LVDPLL_MODE_e;

#endif