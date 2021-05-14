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
/*! \file bl2_otp.c
    \brief A C module that implements functions to retrieve 
    the Asset Tracking Metrics

    Public interfaces:
    otp_get_chip_revision

*/
/***********************************************************************/
#include "bl2_otp.h"

#define OTP_ENTRY_SIZE_BYTES  4u
#define OTP_BANK_SIZE_BYTES   16u

#define OTP_BANK_SIZE_ENTRIES (OTP_BANK_SIZE_BYTES / OTP_ENTRY_SIZE_BYTES)

#define OTP_CALC_START_BANK_INDEX(entry_index) \
    ((entry_index) / OTP_BANK_SIZE_ENTRIES)

#define OTP_CALC_END_BANK_INDEX(entry_index, entry_count, entry_size) \
    ((((entry_index) * OTP_ENTRY_SIZE_BYTES) + (entry_count) * (entry_size) - 1) / OTP_BANK_SIZE_BYTES)

#define WRCK_TIMEOUT 1000

static uint32_t gs_sp_otp_lock_bits[2];
static bool gs_is_otp_available;
static OTP_CHICKEN_BITS_t gs_chicken_bits;
static MISC_CONFIGURATION_BITS_t gs_misc_configuration;

static inline bool otp_is_bank_locked(uint32_t bank_index) {
    uint32_t reg_index = bank_index / 32;
    uint32_t bit_index = bank_index & 0x1Fu;
    uint32_t mask = 1u << bit_index;

    if (gs_sp_otp_lock_bits[reg_index] & mask) {
        return false;
    } else {
        return true;
    }
}

int OTP_Initialize(void)
{
    volatile uint32_t * sp_otp_data = (uint32_t*)R_SP_EFUSE_BASEADDR;
    uint32_t main_wrck;
    uint32_t vault_wrck;
    uint32_t timeout;

    // Use the fast clock (100 MHz) to drive the SP WRCK

    main_wrck = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_CLK_MAIN_WRCK_ADDRESS);
    main_wrck = CLOCK_MANAGER_CM_CLK_MAIN_WRCK_SEL_MODIFY(main_wrck, 1);
    iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_CLK_MAIN_WRCK_ADDRESS, main_wrck);
    timeout = WRCK_TIMEOUT;
    do {
        timeout--;
        if (0 == timeout) {
            main_wrck = CLOCK_MANAGER_CM_CLK_MAIN_WRCK_SEL_MODIFY(main_wrck, 0);
            iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_CLK_MAIN_WRCK_ADDRESS, main_wrck);
            break;
        }
        main_wrck = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_CLK_MAIN_WRCK_ADDRESS);
    } while (0 == CLOCK_MANAGER_CM_CLK_MAIN_WRCK_STABLE_GET(main_wrck));


    // Use the fast clock (100 MHz) to drive the VaultIP WRCK
    vault_wrck = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_CLK_VAULT_WRCK_ADDRESS);
    vault_wrck = CLOCK_MANAGER_CM_CLK_VAULT_WRCK_SEL_MODIFY(vault_wrck, 1);
    iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_CLK_VAULT_WRCK_ADDRESS, vault_wrck);
    timeout = WRCK_TIMEOUT;
    do {
        timeout--;
        if (0 == timeout) {
            vault_wrck = CLOCK_MANAGER_CM_CLK_VAULT_WRCK_SEL_MODIFY(vault_wrck, 0);
            iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_CLK_VAULT_WRCK_ADDRESS, vault_wrck);
            break;
        }
        vault_wrck = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_CLK_VAULT_WRCK_ADDRESS);
    } while (0 == CLOCK_MANAGER_CM_CLK_VAULT_WRCK_STABLE_GET(vault_wrck));

    // check the bootstrap pins to test if the OTP is available
    if (RESET_MANAGER_RM_STATUS2_ERROR_SMS_UDR_GET(ioread32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_STATUS2_ADDRESS))) {
        gs_sp_otp_lock_bits[0] = 0xFFFFFFFF;
        gs_sp_otp_lock_bits[1] = 0xFFFFFFFF;
        gs_chicken_bits.R = 0xFFFFFFFF;
        gs_misc_configuration.R = 0xFFFFFFFF;
        gs_is_otp_available = false;
    } else {
        gs_sp_otp_lock_bits[0] = sp_otp_data[SP_OTP_INDEX_LOCK_REG_BITS_31_00_OFFSET];
        gs_sp_otp_lock_bits[1] = sp_otp_data[SP_OTP_INDEX_LOCK_REG_BITS_63_32_OFFSET];
        gs_chicken_bits.R = sp_otp_data[SP_OTP_INDEX_CHICKEN_BITS];
        gs_misc_configuration.R = sp_otp_data[SP_OTP_INDEX_MISC_CONFIGURATION];
        gs_is_otp_available = true;
    }
    
    return 0;
}

uint32_t OTP_Read_Word (uint32_t bank, uint32_t row, uint32_t mask)
{
    volatile uint32_t * sp_otp_data = (uint32_t*)R_SP_EFUSE_BASEADDR;
    uint32_t index = bank + row;
    if (index >= 256) {
        return 1;
    }

    if (!gs_is_otp_available) {
        return  0xFFFFFFFF;
    } else {
        return (sp_otp_data[index] & mask);
    }

}

int OTP_Write_Word (uint32_t bank, uint32_t row, uint32_t mask, uint32_t value)
{
    uint32_t offset = bank + row;
    volatile uint32_t * sp_otp_data = (uint32_t*)R_SP_EFUSE_BASEADDR;
    uint32_t bank_index = OTP_CALC_START_BANK_INDEX(offset);
    uint32_t old_value, new_value;

    if (!gs_is_otp_available) {
        return -1;
    }

    if (otp_is_bank_locked(bank_index)) {
        return -1;
    }

    old_value = sp_otp_data[offset];
    value = (value & old_value) & mask;
    sp_otp_data[offset] = value;
    new_value = sp_otp_data[offset];

    if (SP_OTP_INDEX_LOCK_REG_BITS_31_00_OFFSET == offset) {
        gs_sp_otp_lock_bits[0] = new_value;
    } else if (SP_OTP_INDEX_LOCK_REG_BITS_63_32_OFFSET == offset) {
        gs_sp_otp_lock_bits[1] = new_value;
    }

    return 0;
}
/************************************************************************
*
*   FUNCTION
*
*       otp_get_chip_revision
*
*   DESCRIPTION
*
*       This function reads the chip revision from OTP memory
*
*   INPUTS
*
*       *chip_rev    Pointer to chip rev variable
*
*   OUTPUTS
*
*       int          Return status
*
***********************************************************************/
int otp_get_chip_revision(char *chip_rev)
{
    uint64_t chip_revision;
    OTP_SILICON_REVISION_t silicon_revision;

    if (0 != sp_otp_get_silicon_revision(&silicon_revision)) {
        printf("sp_otp_get_silicon_revision() failed!\n");
        return -1;
    }

    chip_revision =
        (uint32_t)((silicon_revision.B.si_major_rev << 4) | silicon_revision.B.si_minor_rev);

    sprintf(chip_rev, "%ld", chip_revision);

    return 0;
}
