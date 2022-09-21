/*************************************************************************
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies or
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
************************************************************************/

/*! \def PLL_REG_INDEX_REG_0
    \brief index register number
*/
#define PLL_REG_INDEX_REG_0 0

/*! \def PLL_REG_INDEX_REG_FCW_INT
    \brief int multiplier
*/
#define PLL_REG_INDEX_REG_FCW_INT 0x02

/*! \def PLL_REG_INDEX_REG_FCW_FRAC
    \brief frac multiplier
*/
#define PLL_REG_INDEX_REG_FCW_FRAC 0x03

/*! \def PLL_REG_INDEX_REG_POST_DIVIDER
    \brief post divider
*/
#define PLL_REG_INDEX_REG_POST_DIVIDER 0x0E

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

/*! \def DCO_NORMALIZATION_ENABLE_OFFSET
    \brief DCO normalization enable offset
*/
#define DCO_NORMALIZATION_ENABLE_OFFSET 7u

/*! \def DCO_NORMALIZATION_ENABLE_MASK
    \brief DCO normalization enable mask
*/
#define DCO_NORMALIZATION_ENABLE_MASK (1u << DCO_NORMALIZATION_ENABLE_OFFSET)

/*! \def STROBE_UPDATE_OFFSET
    \brief Strobe update offset
*/
#define STROBE_UPDATE_OFFSET 0u

/*! \def PLL_ENABLE_OFFSET
    \brief PLL enable offset
*/
#define PLL_ENABLE_OFFSET 3u

/*! \def PLL_ENABLE_MASK
    \brief PLL enable mask
*/
#define PLL_ENABLE_MASK (1u << PLL_ENABLE_OFFSET)

/*! \def LDO_POWER_DOWN_OFFSET
    \brief LDO power down offset
*/
#define LDO_POWER_DOWN_OFFSET 1u

/*! \def LDO_BYPASS_OFFSET
    \brief LDO bypass offset
*/
#define LDO_BYPASS_OFFSET 0u