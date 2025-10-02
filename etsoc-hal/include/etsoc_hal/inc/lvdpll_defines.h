/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies or
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#include "et_test_common.h"
#include "minion_esr_defines.h"
#include "movellus_lvdpll_modes_config.h"

#ifndef PLL_LOCK_MASK
    #define PLL_LOCK_MASK 0x20000
#endif

#ifndef PLL_BUSY_MASK
    #define PLL_BUSY_MASK 0x10000
#endif

#ifndef DLL_LOCK_MASK
    #define DLL_LOCK_MASK 0x20000
#endif

#ifndef REG_STROBE_UPDATE
    #define REG_STROBE_UPDATE 0x1
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

// Bellow defines must be set according to characterization

#ifndef LVDPLL_LOCK_TIME_WAIT
    #define LVDPLL_LOCK_TIME_WAIT 120
#endif

#ifndef LVDPLL_BUSY_TIME_WAIT
    #define LVDPLL_BUSY_TIME_WAIT 10
#endif

#ifndef LVDPLL_OSC_NORM_FREQ
    #define LVDPLL_OSC_NORM_FREQ 2000
#endif

#ifndef LVDPLL_NORM_MODE
    #define LVDPLL_NORM_MODE MODE_1000MHz
#endif


typedef enum lvdpllMode 
{
    MODE_300MHz = 0x1,
    MODE_325MHz,
    MODE_350MHz,
    MODE_375MHz,
    MODE_400MHz,
    MODE_425MHz,
    MODE_450MHz,
    MODE_475MHz,
    MODE_500MHz,
    MODE_525MHz,
    MODE_550MHz,
    MODE_575MHz,
    MODE_600MHz,
    MODE_625MHz,
    MODE_650MHz,
    MODE_675MHz,
    MODE_700MHz,
    MODE_725MHz,
    MODE_750MHz,
    MODE_775MHz,
    MODE_800MHz,
    MODE_825MHz,
    MODE_850MHz,
    MODE_875MHz,
    MODE_900MHz,
    MODE_925MHz,
    MODE_950MHz,
    MODE_975MHz,
    MODE_1000MHz,
    MODE_1025MHz,
    MODE_1050MHz,
    MODE_1075MHz,
    MODE_1100MHz,
    MODE_1125MHz,
    MODE_1150MHz,
    MODE_1175MHz,
    MODE_1200MHz,
    MODE_1225MHz,
    MODE_1250MHz,
    MODE_1275MHz,
    MODE_1300MHz,
    MODE_1325MHz,
    MODE_1350MHz,
    MODE_1375MHz,
    MODE_1400MHz
} LVDPLL_MODE_e;