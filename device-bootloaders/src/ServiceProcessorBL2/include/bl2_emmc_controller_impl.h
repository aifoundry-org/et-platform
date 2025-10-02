/*-------------------------------------------------------------------------
* Copyright (C) 2023, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/
/************************************************************************/
/*! \file bl2_emmc_controller_impl.h
    \brief A C header that defines private implementation primitives
*/
/***********************************************************************/

#ifndef __BL2_EMMC_CONTROLLER_IMPL_H__
#define __BL2_EMMC_CONTROLLER_IMPL_H__

// eMMC HAL and MACROS
#include "etsoc_hal/inc/DWC_mshc_crypto_macro.h"
#include "etsoc_hal/inc/DWC_mshc_crypto.h"

#include "bl2_emmc_controller.h"

#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h"
#include "semphr.h"

#define EMMC_SIZE_BYTES_EXT_CSD_CARD_REGISTER     512 // taken from JEDEC EMMC standard
#define EMMC_EXT_CSD_BYTE_OFFSET_SEC_COUNT_BYTE_3 215 // taken from JEDEC EMMC standard
#define EMMC_EXT_CSD_BYTE_OFFSET_SEC_COUNT_BYTE_2 214 // taken from JEDEC EMMC standard
#define EMMC_EXT_CSD_BYTE_OFFSET_SEC_COUNT_BYTE_1 213 // taken from JEDEC EMMC standard
#define EMMC_EXT_CSD_BYTE_OFFSET_SEC_COUNT_BYTE_0 212 // taken from JEDEC EMMC standard
#define EMMC_EXT_CSD_BYTE_OFFSET_HS_TIMING        185 // taken from JEDEC EMMC standard
#define EMMC_EXT_CSD_BYTE_OFFSET_BUS_WIDTH        183 // taken from JEDEC EMMC standard
#define EMMC_EXT_CSD_BYTE_OFFSET_RST_N_FUNCTION   162 // taken from JEDEC EMMC standard

// The bits in the pointed byte are set, according to the ‘1’ bits in the Value
// field.
#define EMMC_JEDEC_CMD6_ARG_ACCESS_SET_BITS 0x1
// The bits in the pointed byte are cleared, according to the ‘1’ bits in the
// Value field.
#define EMMC_JEDEC_CMD6_ARG_ACCESS_CLEAR_BITS 0x2
// The Value field is written into the pointed byte of EXT_CSD
#define EMMC_JEDEC_CMD6_ARG_ACCESS_WRITE_BYTE 0x3

// defines for EXT_CSD[HS_TIMING]
#define EMMC_HS_TIMING_BACKWARDS_COMPATIBILITY 0x0
#define EMMC_HS_TIMING_HIGH_SPEED              0x1
#define EMMC_HS_TIMING_HS200                   0x2
#define EMMC_HS_TIMING_HS400                   0x3

// defines for EXT_CSD[BUS_WIDTH]
#define EMMC_BUS_WIDTH_STROBE_ENABLE 0x8
#define EMMC_BUS_WIDTH_8_BIT_DDR     0x6
#define EMMC_BUS_WIDTH_4_BIT_DDR     0x5
#define EMMC_BUS_WIDTH_8_BIT_SDR     0x2
#define EMMC_BUS_WIDTH_4_BIT_SDR     0x1
#define EMMC_BUS_WIDTH_1_BIT_SDR     0x0

// define for EXT_CSD[RST_N_FUNCTION]
#define EMMC_EXT_CSD_RST_N_FUNCTION_ENABLE 1

#define EMMC_DEVICE_POWER_IS_BAD 0x0

#define EMMC_GET_MANUFACTURER_ID(x) ((x >> 16) & 0xFF)
#define EMMC_IS_BLOCK_SIZE_VARIABLE(command, operating_mode) \
    ((command != EMMC_JEDEC_CMD21) && (operating_mode != EMMC_MODE_HS400))

#define EMMC_MANUFACTURER_ID_SAMSUNG  0x15
#define EMMC_MANUFACTURER_ID_SWISSBIT 0xFB
#define EMMC_MANUFACTURER_ID_KINGSTON 0x70
#define EMMC_MANUFACTURER_ID_ISSI     0x9D
#define EMMC_MANUFACTURER_ID_TOSHIBA  0x11

typedef struct EMMC_DEVICE
{
    Emmc *regs;
    StaticSemaphore_t lock;
    SemaphoreHandle_t lock_handle;

} ET_EMMC_DEV_t;

typedef struct __attribute__((packed)) desc_line
{
    uint32_t valid : 1;     // bit0
    uint32_t end : 1;       // bit1
    uint32_t interr : 1;    // bit2
    uint32_t act : 3;       // bit3-bit5
    uint32_t len_10b : 10;  // bit6-bit15
    uint32_t len_16b : 16;  // bit16-bit31
    long address : 64;      // bit32-bit95
    uint32_t reserved : 32; // bit96-bit127
} desc_line_t;

enum emmc_command_type
{
    NORMAL_CMD = 0,
    SUSPEND_CMD,
    RESUME_CMD,
    ABORT_CMD
};

// eMMC device OCR register defines
/*! \def EMMC_OCR_1V8_VCCQ
    \brief Indicates that eMMC card uses 1.8V supply, stored in OCR register in the card.
*/
/*! \def EMMC_OCR_2V7_3V6_VCCQ
    \brief Indicates that eMMC card uses 2.7V-3.6V supply, as indicated with OCR register in the card.
*/
/*! \def EMMC_OCR_SECTOR_ACCESS_MODE
    \brief Indicates that we should be accessing card in sector mode, as oposite to byte access. This depends on device capacity (2GB is border point).
*/
/*! \def EMMC_OCR_CARD_READY
    \brief Indicates that the card is in Ready state*/

#define EMMC_OCR_1V8_VCCQ           (0x1u << 7)
#define EMMC_OCR_2V7_3V6_VCCQ       (0x1FFu << 15)
#define EMMC_OCR_SECTOR_ACCESS_MODE (0x2u << 29)
#define EMMC_OCR_CARD_READY         (1u << 31)

#define EMMC_CAPACITY_GRE_2G_READY_STATE                                                    \
    (0x00000000 | EMMC_OCR_1V8_VCCQ | EMMC_OCR_2V7_3V6_VCCQ | EMMC_OCR_SECTOR_ACCESS_MODE | \
     EMMC_OCR_CARD_READY)
#define EMMC_CAPACITY_LEQ_2G_READY_STATE \
    (0x00000000 | EMMC_OCR_1V8_VCCQ | EMMC_OCR_2V7_3V6_VCCQ | EMMC_OCR_CARD_READY)

#define EMMC_STUFF_BITS_32 0x0
#define EMMC_RCA           0x1u
#define EMMC_RCA_31_16     (EMMC_RCA << 16)
#define EMMC_SLEEP_BIT     (0x1u << 15)
#define EMMC_AWAKE_BIT     (0x0u << 15)
#define EMMC_READ          1
#define EMMC_WRITE         0

// Command response defines
#define RESP0  0x0 // no response
#define RESP1  0x2 // 48 bits
#define RESP1b 0x3 // 48 bits with busy check
#define RESP2  0x1 // 136 bits
#define RESP3  RESP1
#define RESP4  RESP1
#define RESP5  RESP1

// SDMA
#define SDMA_SELECT 0x0

// ADMA2 stuff
#define ADMA2_SELECT    0x2
#define ADMA2_ATTR_NOP  0x0
#define ADMA2_ATTR_TRAN 0x4
#define ADMA2_ATTR_LINK 0x6
#define ADMA2_LEN_OFF   0x6
#define ADMA2_ADR_OFF   32

// eMMC frequency and dividers
// for PLL1 = 2GHz
#define EMMC_200_200_MHZ_DIV 0
#define EMMC_200_100_MHZ_DIV 1
#define EMMC_200_50_MHZ_DIV  2
#define EMMC_200_33_MHZ_DIV  3
#define EMMC_200_25_MHZ_DIV  4
#define EMMC_200_20_MHZ_DIV  5
// for PLL1 = 1.5GHz
#define EMMC_150_150_MHZ_DIV 0
#define EMMC_150_75_MHZ_DIV  1
#define EMMC_150_37_MHZ_DIV  2
#define EMMC_150_25_MHZ_DIV  3
#define EMMC_150_18_MHZ_DIV  4
#define EMMC_150_15_MHZ_DIV  5
// for PLL1 = 1GHz
#define EMMC_100_100_MHZ_DIV 0
#define EMMC_100_50_MHZ_DIV  1
#define EMMC_100_25_MHZ_DIV  2
#define EMMC_100_16_MHZ_DIV  3
#define EMMC_100_12_MHZ_DIV  4
#define EMMC_100_10_MHZ_DIV  5

enum emmc_command
{
    EMMC_JEDEC_CMD0,
    EMMC_JEDEC_CMD1,
    EMMC_JEDEC_CMD2,
    EMMC_JEDEC_CMD3,
    EMMC_JEDEC_CMD4,
    EMMC_JEDEC_CMD5,
    EMMC_JEDEC_CMD6,
    EMMC_JEDEC_CMD7,
    EMMC_JEDEC_CMD8,
    EMMC_JEDEC_CMD9,
    EMMC_JEDEC_CMD10,
    EMMC_JEDEC_CMD11,
    EMMC_JEDEC_CMD12,
    EMMC_JEDEC_CMD13,
    EMMC_JEDEC_CMD14,
    EMMC_JEDEC_CMD15,
    EMMC_JEDEC_CMD16,
    EMMC_JEDEC_CMD17,
    EMMC_JEDEC_CMD18,
    EMMC_JEDEC_CMD19,
    EMMC_JEDEC_CMD20,
    EMMC_JEDEC_CMD21,
    EMMC_JEDEC_CMD22,
    EMMC_JEDEC_CMD23,
    EMMC_JEDEC_CMD24,
    EMMC_JEDEC_CMD25,
    EMMC_JEDEC_CMD26,
    EMMC_JEDEC_CMD27,
    EMMC_JEDEC_CMD28,
    EMMC_JEDEC_CMD29,
    EMMC_JEDEC_CMD30,
    EMMC_JEDEC_CMD31,
    EMMC_JEDEC_CMD32,
    EMMC_JEDEC_CMD33,
    EMMC_JEDEC_CMD34,
    EMMC_JEDEC_CMD35,
    EMMC_JEDEC_CMD36,
    EMMC_JEDEC_CMD37,
    EMMC_JEDEC_CMD38,
    EMMC_JEDEC_CMD39,
    EMMC_JEDEC_CMD40,
    EMMC_JEDEC_CMD41,
    EMMC_JEDEC_CMD42,
    EMMC_JEDEC_CMD43,
    EMMC_JEDEC_CMD44,
    EMMC_JEDEC_CMD45,
    EMMC_JEDEC_CMD46,
    EMMC_JEDEC_CMD47,
    EMMC_JEDEC_CMD48,
    EMMC_JEDEC_CMD49,
    EMMC_JEDEC_CMD50,
    EMMC_JEDEC_CMD51,
    EMMC_JEDEC_CMD52,
    EMMC_JEDEC_CMD53,
    EMMC_JEDEC_CMD54,
    EMMC_JEDEC_CMD55,
    EMMC_JEDEC_CMD56,
    EMMC_JEDEC_CMD57,
    EMMC_JEDEC_CMD58,
    EMMC_JEDEC_CMD59,
    EMMC_JEDEC_CMD60,
    EMMC_JEDEC_CMD61,
    EMMC_JEDEC_CMD62,
    EMMC_JEDEC_CMD63
};

typedef enum
{
    EMMC_CMD_READ = EMMC_READ,
    EMMC_CMD_WRITE = EMMC_WRITE
} EMMC_Command_Direction;

#endif /*__BL2_EMMC_CONTROLLER_IMPL_H__*/
