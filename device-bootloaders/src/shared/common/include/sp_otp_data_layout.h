/*-------------------------------------------------------------------------
* Copyright (C) 2019,2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef __SP_OTP_DATA_LAYOUT_H__
#define __SP_OTP_DATA_LAYOUT_H__

#include <stdint.h>

typedef struct OTP_NEIGHBORHOOD_STATUS_NH128_NH135_OTHER_s
{
    union
    {
        struct
        {
            uint32_t neighborhood_status : 8;
            uint32_t maxion_status : 4;
            uint32_t graphics_status : 1;
            uint32_t machine_learning : 1;
            uint32_t unused : 2;
            uint32_t shire_master_id : 8;
            uint32_t sku_id : 8;
        } B;
        uint32_t R;
    };
} OTP_NEIGHBORHOOD_STATUS_NH128_NH135_OTHER_t;

// The following struct has two uses:
//   1) Information to append to the PCIe whitelist (MASK and SIZE used)
//   2) Directly patch PCIe registers by writing MASK directly to them
typedef struct OTP_PCIE_WHITELIST_ENTRY_OVERRIDE_0_s
{
    union
    {
        union
        {
            uint32_t MASK; // used when is_region=0
            struct
            { // used when is_region=1
                uint32_t reserved1 : 2;
                uint32_t LIMIT_23_02 : 22; // bits 23:2 of the region limit offset
                uint32_t reserved2 : 8;
            };
        } B;
        uint32_t R;
    };
} OTP_PCIE_WHITELIST_ENTRY_OVERRIDE_0_t;

typedef struct OTP_PCIE_WHITELIST_ENTRY_OVERRIDE_1_s
{
    union
    {
        struct
        {
            uint32_t
                FLAGS : 2; // 0 - add to white list, 1 - apply before PCIe config, 2 - apply after PCIe config, 3 - ignore
            uint32_t OFFSET_23_02 : 22; // bits 23:2 of the offset
            uint32_t MEM_SPACE : 4;     // memory space
            uint32_t IS_REGION : 1;     // when 0: single-register entry, when 1: region entry
            uint32_t reserved : 3;
        } B;
        uint32_t R;
    };
} OTP_PCIE_WHITELIST_ENTRY_OVERRIDE_1_t;

typedef struct OTP_PCIE_WHITELIST_ENTRY_OVERRIDE_s
{
    OTP_PCIE_WHITELIST_ENTRY_OVERRIDE_0_t dw_0;
    OTP_PCIE_WHITELIST_ENTRY_OVERRIDE_1_t dw_1;
} OTP_PCIE_WHITELIST_ENTRY_OVERRIDE_t;

typedef struct OTP_CHICKEN_BITS_s
{
    union
    {
        struct
        {
            uint32_t VaultIP_Chicken_Bit : 2;
            uint32_t VaultIP_FWp_Allowed_Chicken_Bit : 2;
            uint32_t unused : 2;
            uint32_t Signatures_Chicken_Bit : 2;
            uint32_t PCIe_WhiteList_Chicken_Bit : 2;
            uint32_t SP_L1_Cache_Chicken_Bit : 2;
            uint32_t reserved : 20;
        } B;
        uint32_t R;
    };
} OTP_CHICKEN_BITS_t;

typedef struct MISC_CONFIGURATION_BITS_s
{
    union
    {
        struct
        {
            uint32_t ENG : 1;
            uint32_t VaultIP_FIPS : 1;
            uint32_t VaultIP_FCST : 1;
            uint32_t VaultIP_FAT : 1;
            uint32_t VaultIP_ROTT1 : 1;
            uint32_t VaultIP_ROTT2 : 1;
            uint32_t VaultIP_ROTT3 : 1;
            uint32_t VaultIP_ROTT4 : 1;
            uint32_t VaultIP_ROTT5 : 1;
            uint32_t VaultIP_Clock_Switch : 1;
            uint32_t reserved : 22;
        } B;
        uint32_t R;
    };
} MISC_CONFIGURATION_BITS_t;

typedef struct OTP_UART_CONFIGURATION_OVERRIDE_s
{
    union
    {
        struct
        {
            uint32_t RBR_DLL : 8; // overrides the RBR.DLL field (lower 8 bits of the divisor latch)
            uint32_t DLH_DLH : 8; // overrides the DLH.DLH field (upper 8 bits of the divisor latch)
            uint32_t DLF_DLF : 4; // overrides the DLF.DLF field (divisor latch fraction register)
            uint32_t LCR_DLS : 2; // overrides the LCR.DLS field (data length select)
            uint32_t LCR_STOP : 1; // overrides the LCR.STOP field (number of stop bits)
            uint32_t LCR_PEN : 1;  // overrides the LCR.PEN field (parity enable)
            uint32_t LCR_EPS : 1;  // overrides the LCR.EPS field (even parity select)
            uint32_t reserved : 6;
            uint32_t
                IGN : 1; // 1 - ignore OTP override values and use default UART settings instead, 0 - use OTP override values
        } B;
        uint32_t R;
    };
} OTP_UART_CONFIGURATION_OVERRIDE_t;

typedef struct OTP_SPI_CONFIGURATION_OVERRIDE_s
{
    union
    {
        struct
        {
            uint32_t
                BAUDR_SCKDIV_RX : 15; // overrides the BAUDR.SCKDIV field (clock divider) for RX transfers
            uint32_t
                BAUDR_SCKDIV_RX_IGN : 1; // 1 - ignore the BAUDR_SCKDIV_RX field and use default value, 0 - use OTP override value
            uint32_t
                BAUDR_SCKDIV_TX : 15; // overrides the BAUDR.SCKDIV field (clock divider) for TX transfers
            uint32_t
                BAUDR_SCKDIV_TX_IGN : 1; // 1 - ignore the BAUDR_SCKDIV_TX field and use default value, 0 - use OTP override value
        } B;
        uint32_t R;
    };
} OTP_SPI_CONFIGURATION_OVERRIDE_t;

typedef struct OTP_FLASH_CONFIGURATION_OVERRIDE_0_s
{
    union
    {
        struct
        {
            uint32_t FLASH_READ_COMMAND : 8;    // overrides the FLASH READ command
            uint32_t FLASH_PROGRAM_COMMAND : 8; // overrides the FLASH PAGE PROGRAM command
            uint32_t
                READ_DUMMY_BYTES : 2; // overrides the number of dummy bytes following the FLASH READ command (0-3)
            uint32_t
                READ_OVERWRITE : 1; // 1 - ignore FLASH_READ_COMMAND and READ_DUMMY_BYTES fields and use default values, 0 - use OTP override values
            uint32_t
                PROGRAM_OVERWRITE : 1; // 1 - ignore FLASH_PROGRAM_COMMAND and use default value, 0 -use OTP override value
            uint32_t
                PAGE_PROGRAM_TIMEOUT : 3; // 0-6 overrides page program timeout to (1 << PAGE_PROGRAM_TIMEOUT) ms, 0x7 - ignore and use default timeout value
            uint32_t
                STATUS_OVERWRITE : 1; // 1 - ignore the FLASH_GET_PROGRAM_STATUS_xxx fields and use default values, 0 -use OTP ovrride values
            uint32_t
                OVERRIDE_FLASH_SIZE : 3; // 0-6 - override the SPI flash size to (2 << OVERRIDE_FLASH_SIZE) MegaBytes, 0x7 - determine size dynamically
            uint32_t SKIP_RDID : 1;      // 0 - skip the RDID command, 1 - issue the RDID command
            uint32_t SKIP_RDSFDP : 1;    // 0 - skip the RDSFDP command, 1 - issue the RDID command
            uint32_t reserved : 2;
            uint32_t
                IGN : 1; // 1 - ignore OTP_FLASH_CONFIGURATION_OVERRIDE_0_t and OTP_FLASH_CONFIGURATION_OVERRIDE_1_t fields and use default values, 0 - use override values
        } B;
        uint32_t R;
    };
} OTP_FLASH_CONFIGURATION_OVERRIDE_0_t;

typedef struct OTP_FLASH_CONFIGURATION_OVERRIDE_1_s
{
    union
    {
        struct
        {
            uint32_t
                FLASH_GET_PROGRAM_STATUS_COMMAND : 8; // STATUS command code (used to determine if page programming has completed)
            uint32_t FLASH_GET_PROGRAM_STATUS_MASK : 8; // MASK value
            uint32_t unused : 8;                        // Currently unused
            uint32_t
                FLASH_GET_PROGRAM_STATUS_BUSY_VALUE : 8; // (when STATUS & MASK) == BUSY_VALUE, the programming operation is in progress
        } B;
        uint32_t R;
    };
} OTP_FLASH_CONFIGURATION_OVERRIDE_1_t;

typedef struct OTP_FLASH_CONFIGURATION_OVERRIDE_s
{
    OTP_FLASH_CONFIGURATION_OVERRIDE_0_t dw0;
    OTP_FLASH_CONFIGURATION_OVERRIDE_1_t dw1;
} OTP_FLASH_CONFIGURATION_OVERRIDE_t;

typedef struct OTP_PLL_CONFIGURATION_OVERRIDE_s
{
    union
    {
        struct
        {
            uint32_t REGISTER_VALUE : 16; // PLL register value
            uint32_t REGISTER_INDEX : 6;  // PLL register index
            uint32_t reserved : 2;
            uint32_t
                PLL_0 : 1; // 1 - use this entry when programming SP_PLL_0, 0 - ignore this entry when programming SP_PLL_0
            uint32_t
                PLL_1 : 1; // 1 - use this entry when programming SP_PLL_1, 0 - ignore this entry when programming SP_PLL_1
            uint32_t
                PLL_PCIe : 1; // 1 - use this entry when programming PCIe_PLL, 0 - ignore this entry when programming PCIe_PLL
            uint32_t reserved2 : 1;
            uint32_t
                CFG_100 : 1; // 1 - use this entry when configuring the PLLs to 100%, 0 - ignore this entry when configuring the PLLs to 100%
            uint32_t
                CFG_75 : 1; // 1 - use this entry when configuring the PLLs to 75%, 0 - ignore this entry when configuring the PLLs to 75%
            uint32_t
                CFG_50 : 1; // 1 - use this entry when configuring the PLLs to 50%, 0 - ignore this entry when configuring the PLLs to 50%
            uint32_t
                IGN : 1; // 1 - IGNORE this entry, 0 - USE this entry (subject the PLL_x and CFG_x flags)
        } B;
        uint32_t R;
    };
} OTP_PLL_CONFIGURATION_OVERRIDE_t;

typedef struct OTP_CRITICAL_PATCH_ADDRESS_HI_s
{
    union
    {
        struct
        {
            uint32_t ADDRESS_39_32 : 8;
            uint32_t SEQUENCE : 3;
            uint32_t MASK : 12;
            uint32_t MASK_OFFSET : 6;
            uint32_t WRITE_SIZE : 2;
            uint32_t IGN : 1;
        } B;
        uint32_t R;
    };
} OTP_CRITICAL_PATCH_ADDRESS_HI_t;

typedef struct OTP_CRITICAL_PATCH_ADDRESS_LO_s
{
    union
    {
        struct
        {
            uint32_t ADDRESS_31_00 : 32;
        } B;
        uint32_t R;
    };
} OTP_CRITICAL_PATCH_ADDRESS_LO_t;

typedef struct OTP_CRITICAL_PATCH_DATA_HI_s
{
    union
    {
        struct
        {
            uint32_t DATA_63_32 : 32;
        } B;
        uint32_t R;
    };
} OTP_CRITICAL_PATCH_DATA_HI_t;

typedef struct OTP_CRITICAL_PATCH_DATA_LO_s
{
    union
    {
        struct
        {
            uint32_t DATA_31_00 : 32;
        } B;
        uint32_t R;
    };
} OTP_CRITICAL_PATCH_DATA_LO_t;

typedef struct OTP_CRITICAL_PATCH_s
{
    OTP_CRITICAL_PATCH_ADDRESS_HI_t dw0;
    OTP_CRITICAL_PATCH_ADDRESS_LO_t dw1;
    OTP_CRITICAL_PATCH_DATA_HI_t dw2;
    OTP_CRITICAL_PATCH_DATA_LO_t dw3;
} OTP_CRITICAL_PATCH_t;

#define OTP_MAX_CRITICAL_PATCH_COUNT 8

typedef struct OTP_SPECIAL_CUSTOMER_DESIGNATOR_s
{
    union
    {
        struct
        {
            uint32_t special_customer_id : 8;
            uint32_t reserved : 24;
        } B;
        uint32_t R;
    };
} OTP_CRITICAL_PAOTP_SPECIAL_CUSTOMER_DESIGNATOR_t;

typedef struct OTP_PLL_CONFIGURATION_DELAY_s
{
    union
    {
        struct
        {
            uint32_t
                sp_pll_delay_1 : 15; // delay (in iterations) after configuring the PLL and waiting for it to lock
            uint32_t sp_pll_delay_1_ignore : 1;
            uint32_t sp_pll_delay_2 : 15; // delay (in iterations) after switching the PLL MUX
            uint32_t sp_pll_delay_2_ignore : 1;
        } B;
        uint32_t R;
    };
} OTP_PLL_CONFIGURATION_DELAY_t;

typedef struct OTP_PLL_LOCK_TIMEOUT_s
{
    union
    {
        struct
        {
            uint32_t TIMEOUT : 31; // timeout value
            uint32_t IGNORE : 1;   // 1 - IGNORE this entry, 0 - USE this entry
        } B;
        uint32_t R;
    };
} OTP_PLL_LOCK_TIMEOUT_t;

typedef struct OTP_SILICON_REVISION_s
{
    union
    {
        struct
        {
            uint32_t MS_29_H2 : 4;     // MS_29_H2 field
            uint32_t MS_30_H2 : 4;     // MS_30_H2 field
            uint32_t MS_31_H2 : 4;     // MS_31_H2 field
            uint32_t MS_32_H2 : 4;     // MS_32_H2 field
            uint32_t MS_33_H2 : 4;     // MS_33_H2 field
            uint32_t MX_0_H2 : 4;      // MX_0_H2  field
            uint32_t si_major_rev : 4; // si_major_rev field
            uint32_t si_minor_rev : 4; // si_minor_rev field
        } B;
        uint32_t R;
    };
} OTP_SILICON_REVISION_t;

#define SP_OTP_INDEX_SILICON_REVISION 12

#define SP_OTP_MAX_PCIE_CONFIG_ENTRIES_COUNT 16
#define SP_OTP_MAX_PLL_CONFIG_ENTRIES_COUNT  16

#define SP_OTP_INDEX_NEIGHBORHOOD_STATUS_NH128_NH135_OTHER 17

#define SP_OTP_INDEX_SHIRE_STATUS_S0_S31     40
#define SP_OTP_INDEX_SHIRE_STATUS_S32_S33    41
#define SP_OTP_INDEX_SHIRE_STATUS_S0_S31_V2  42
#define SP_OTP_INDEX_SHIRE_STATUS_S32_S33_V2 43

#define SP_OTP_INDEX_SHIRE_SPEED 18

#define SP_OTP_INDEX_PCIE_PHY_CFG_WHITEIST_OVERRIDE 124

#define SP_OTP_INDEX_SPECIAL_CUSTOMER_DESIGNATOR 160

#define SP_OTP_INDEX_CHICKEN_BITS                              161
#define SP_OTP_INDEX_MISC_CONFIGURATION                        162
#define SP_OTP_INDEX_UART_CFG_OVERRIDE                         163
#define SP_OTP_INDEX_SPI_CFG_OVERRIDE                          164
#define SP_OTP_INDEX_FLASH_CFG_OVERRIDE                        168
#define SP_OTP_INDEX_PLL_CFG_OVERRIDE                          172
#define SP_OTP_INDEX_VAULTIP_FIRMWARE_CHECK_START_TIMEOUT      188
#define SP_OTP_INDEX_VAULTIP_FIRMWARE_ACCEPTED_TIMEOUT         189
#define SP_OTP_INDEX_VAULTIP_FIRMWARE_OUTPUT_TOKEN_TIMEOUT_1   190
#define SP_OTP_INDEX_VAULTIP_FIRMWARE_OUTPUT_TOKEN_TIMEOUT_2   191
#define SP_OTP_INDEX_VAULTIP_FIRMWARE_OUTPUT_TOKEN_TIMEOUT_3   192
#define SP_OTP_INDEX_VAULTIP_FIRMWARE_OUTPUT_TOKEN_TIMEOUT_4   193
#define SP_OTP_INDEX_VAULTIP_FIRMWARE_OUTPUT_TOKEN_TIMEOUT_5   194
#define SP_OTP_INDEX_VAULTIP_FIRMWARE_CLOCK_SWITCH_INPUT_TOKEN 195

#define SP_OTP_INDEX_CRITICAL_PATCH_0_ADDRESS_HI 196
#define SP_OTP_INDEX_CRITICAL_PATCH_0_ADDRESS_LO 197
#define SP_OTP_INDEX_CRITICAL_PATCH_0_DATA_HI    198
#define SP_OTP_INDEX_CRITICAL_PATCH_0_DATA_LO    199
#define SP_OTP_INDEX_CRITICAL_PATCH_1_ADDRESS_HI 200
#define SP_OTP_INDEX_CRITICAL_PATCH_1_ADDRESS_LO 201
#define SP_OTP_INDEX_CRITICAL_PATCH_1_DATA_HI    202
#define SP_OTP_INDEX_CRITICAL_PATCH_1_DATA_LO    203
#define SP_OTP_INDEX_CRITICAL_PATCH_2_ADDRESS_HI 204
#define SP_OTP_INDEX_CRITICAL_PATCH_2_ADDRESS_LO 205
#define SP_OTP_INDEX_CRITICAL_PATCH_2_DATA_HI    206
#define SP_OTP_INDEX_CRITICAL_PATCH_2_DATA_LO    207
#define SP_OTP_INDEX_CRITICAL_PATCH_3_ADDRESS_HI 208
#define SP_OTP_INDEX_CRITICAL_PATCH_3_ADDRESS_LO 209
#define SP_OTP_INDEX_CRITICAL_PATCH_3_DATA_HI    210
#define SP_OTP_INDEX_CRITICAL_PATCH_3_DATA_LO    211
#define SP_OTP_INDEX_CRITICAL_PATCH_4_ADDRESS_HI 212
#define SP_OTP_INDEX_CRITICAL_PATCH_4_ADDRESS_LO 213
#define SP_OTP_INDEX_CRITICAL_PATCH_4_DATA_HI    214
#define SP_OTP_INDEX_CRITICAL_PATCH_4_DATA_LO    215
#define SP_OTP_INDEX_CRITICAL_PATCH_5_ADDRESS_HI 216
#define SP_OTP_INDEX_CRITICAL_PATCH_5_ADDRESS_LO 217
#define SP_OTP_INDEX_CRITICAL_PATCH_5_DATA_HI    218
#define SP_OTP_INDEX_CRITICAL_PATCH_5_DATA_LO    219
#define SP_OTP_INDEX_CRITICAL_PATCH_6_ADDRESS_HI 220
#define SP_OTP_INDEX_CRITICAL_PATCH_6_ADDRESS_LO 221
#define SP_OTP_INDEX_CRITICAL_PATCH_6_DATA_HI    222
#define SP_OTP_INDEX_CRITICAL_PATCH_6_DATA_LO    223
#define SP_OTP_INDEX_CRITICAL_PATCH_7_ADDRESS_HI 224
#define SP_OTP_INDEX_CRITICAL_PATCH_7_ADDRESS_LO 225
#define SP_OTP_INDEX_CRITICAL_PATCH_7_DATA_HI    226
#define SP_OTP_INDEX_CRITICAL_PATCH_7_DATA_LO    227

#define SP_OTP_INDEX_PLL_CONFIG_DELAY 228
#define SP_OTP_INDEX_PLL_LOCK_TIMEOUT 229

#define SP_OTP_INDEX_COMM_ISSUING_CA_CERTIFICATE_MONOTONIC_VERSION_COUNTER         236
#define SP_OTP_INDEX_MAXION_BL1_CERTIFICATE_MONOTONIC_VERSION_COUNTER              237
#define SP_OTP_INDEX_COMPUTE_KERNEL_CERTIFICATE_MONOTONIC_VERSION_COUNTER          238
#define SP_OTP_INDEX_WORKER_MINION_CERTIFICATE_MONOTONIC_VERSION_COUNTER           239
#define SP_OTP_INDEX_MASTER_MINION_CERTIFICATE_MONOTONIC_VERSION_COUNTER           240
#define SP_OTP_INDEX_MACHINE_MINION_CERTIFICATE_MONOTONIC_VERSION_COUNTER          241
#define SP_OTP_INDEX_SW_ISSUING_CA_CERTIFICATE_MONOTONIC_VERSION_COUNTER           242
#define SP_OTP_INDEX_DRAM_CONTROLLER_DATA_CERTIFICATE_MONOTONIC_VERSION_COUNTER    243
#define SP_OTP_INDEX_SP_BL2_CERTIFICATE_MONOTONIC_VERSION_COUNTER                  244
#define SP_OTP_INDEX_SP_BL1_CERTIFICATE_MONOTONIC_VERSION_COUNTER                  245
#define SP_OTP_INDEX_SP_PCIE_PHY_CONFIG_DATA_CERTIFICATE_MONOTONIC_VERSION_COUNTER 246
#define SP_OTP_INDEX_SP_ISSUING_CA_CERTIFICATE_MONOTONIC_VERSION_COUNTER           247

#define SP_OTP_INDEX_LOCK_REG_BITS_31_00_OFFSET 252
#define SP_OTP_INDEX_LOCK_REG_BITS_63_32_OFFSET 253

#endif
