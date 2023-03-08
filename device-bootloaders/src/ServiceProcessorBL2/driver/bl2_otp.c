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
#include "log.h"

/*! \def OTP_ENTRY_SIZE_BYTES
    \brief size of single OTP entry
*/
#define OTP_ENTRY_SIZE_BYTES 4u

/*! \def OTP_BANK_SIZE_BYTES
    \brief defining OTP bank sizes
*/
#define OTP_BANK_SIZE_BYTES 16u

/*! \def OTP_BANK_SIZE_ENTRIES
    \brief macro to calculate total bank size entries
*/
#define OTP_BANK_SIZE_ENTRIES (OTP_BANK_SIZE_BYTES / OTP_ENTRY_SIZE_BYTES)

/*! \def OTP_CALC_START_BANK_INDEX
    \brief macro to calculate bank start index based on entry index
*/
#define OTP_CALC_START_BANK_INDEX(entry_index) ((entry_index) / OTP_BANK_SIZE_ENTRIES)

/*! \def OTP_CALC_START_BANK_INDEX
    \brief macro to calculate bank start index based on entry index
*/
#define OTP_CALC_END_BANK_INDEX(entry_index, entry_count, entry_size) \
    ((((entry_index)*OTP_ENTRY_SIZE_BYTES) + (entry_count) * (entry_size)-1) / OTP_BANK_SIZE_BYTES)

/*! \def WRCK_TIMEOUT
    \brief macro to define timeout for operation
*/
#define WRCK_TIMEOUT 1000

static uint32_t gs_sp_otp_lock_bits[2];
static bool gs_is_otp_available;

static inline bool otp_is_bank_locked(uint32_t bank_index)
{
    uint32_t reg_index = bank_index / 32;
    uint32_t bit_index = bank_index & 0x1Fu;
    uint32_t mask = 1u << bit_index;

    if (gs_sp_otp_lock_bits[reg_index] & mask)
    {
        return false;
    }
    else
    {
        return true;
    }
}

uint32_t OTP_Read_Word(uint32_t bank, uint32_t row, uint32_t mask)
{
    const volatile uint32_t *sp_otp_data = (uint32_t *)R_SP_EFUSE_BASEADDR;
    uint32_t index = bank + row;
    if (index >= 256)
    {
        return 1;
    }

    if (!gs_is_otp_available)
    {
        return 0xFFFFFFFF;
    }
    else
    {
        return (sp_otp_data[index] & mask);
    }
}

int OTP_Write_Word(uint32_t bank, uint32_t row, uint32_t mask, uint32_t value)
{
    uint32_t offset = bank + row;
    volatile uint32_t *sp_otp_data = (uint32_t *)R_SP_EFUSE_BASEADDR;
    uint32_t bank_index = OTP_CALC_START_BANK_INDEX(offset);
    uint32_t old_value;
    uint32_t new_value;

    if (!gs_is_otp_available)
    {
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if (otp_is_bank_locked(bank_index))
    {
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    old_value = sp_otp_data[offset];
    value = (value & old_value) & mask;
    sp_otp_data[offset] = value;
    new_value = sp_otp_data[offset];

    if (SP_OTP_INDEX_LOCK_REG_BITS_31_00_OFFSET == offset)
    {
        gs_sp_otp_lock_bits[0] = new_value;
    }
    else if (SP_OTP_INDEX_LOCK_REG_BITS_63_32_OFFSET == offset)
    {
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
    int status;
    uint64_t chip_revision;
    OTP_SILICON_REVISION_t silicon_revision;

    status = sp_otp_get_silicon_revision(&silicon_revision);
    if (status != 0)
    {
        Log_Write(LOG_LEVEL_ERROR, "sp_otp_get_silicon_revision() failed!\n");
        return status;
    }

    chip_revision =
        (uint32_t)((silicon_revision.B.si_major_rev << 4) | silicon_revision.B.si_minor_rev);

    sprintf(chip_rev, "%ld", chip_revision);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       otp_get_master_shire_id
*
*   DESCRIPTION
*
*       This function reads Master shire ID from OTP memory
*
*   INPUTS
*
*       *mm_id    Pointer to MM ID variable
*
*   OUTPUTS
*
*       int          Return status
*
***********************************************************************/
int otp_get_master_shire_id(uint8_t *mm_id)
{
    int status;
    OTP_NEIGHBORHOOD_STATUS_NH128_NH135_OTHER_t neigh_config;

    status = sp_otp_get_neighborhood_status_nh128_nh135_other(&neigh_config);
    if (status != 0)
    {
        Log_Write(LOG_LEVEL_ERROR, "sp_otp_get_neighborhood_status_nh128_nh135_other() failed!\n");
        return status;
    }

    *mm_id = neigh_config.B.shire_master_id;

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       otp_get_shire_speed
*
*   DESCRIPTION
*
*       This function reads Shire Speed from OTP memory
*
*   INPUTS
*
*       shire_num   Index of Shire
*       *speed    Pointer to speed variable
*
*   OUTPUTS
*
*       int          Return status
*
***********************************************************************/
int otp_get_shire_speed(uint8_t shire_num, uint8_t *speed)
{
    int status;

    status = sp_otp_get_shire_speed(shire_num, speed);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*      read_ecid 
*
*   DESCRIPTION
*
*       This function reads the Electronic Chip ID
*       This information is fused into OTP with 
*       the following encoding: 
*        - TSMC Lot ID: 36bit. bits:1152:1187
*        - Wafer ID: 5bit. bits: 1188:1192 
*        - X coord: 8bit. bits: 1193:1200 
*        - Y coord: 8bit. bits: 1201:1208
*
*   INPUTS
*   
*       None
* 
*   OUTPUTS
*
*       int          Return status
*
***********************************************************************/
int read_ecid(ecid_t *ecid)
{
    const volatile uint32_t *sp_otp_data = (uint32_t *)R_SP_EFUSE_BASEADDR;

    if (ecid == NULL)
        return -1;

    ecid->lot_id = (((uint64_t)sp_otp_data[37] & 0xf) << 32) | sp_otp_data[36];
    ecid->wafer_id = (uint8_t)((sp_otp_data[37] >> 8) & 0x1f);
    ecid->x_coordinate = (uint8_t)((sp_otp_data[37] >> 16) & 0xff);
    ecid->y_coordinate = (uint8_t)((sp_otp_data[37] >> 24) & 0xff);

    // ECID encoding according to fuse map
    uint64_t lot_id = ecid->lot_id;
    char *str = ecid->lot_id_str;
    int i = ECID_LOT_ID_LENGTH;
    str[i--] = '\0';
    while (lot_id > 0 && i >= 0)
    {
        str[i] = lot_id & 0x3f;
        if (str[i] < 10)
            str[i] = (char)(str[i] + '0');
        else if (str[i] < 36)
            str[i] = (char)(str[i] - 10 + 'A');
        else
            str[i] = '_';
        --i;
        lot_id >>= 6;
    }

    Log_Write(LOG_LEVEL_INFO, "ECID:\n");
    Log_Write(LOG_LEVEL_INFO, "  OTP row 36 = 0x%08x\n", sp_otp_data[36]);
    Log_Write(LOG_LEVEL_INFO, "  OTP row 37 = 0x%08x\n", sp_otp_data[37]);
    Log_Write(LOG_LEVEL_INFO, "  Lot ID       = %s (0x%016lx)\n", ecid->lot_id_str, ecid->lot_id);
    Log_Write(LOG_LEVEL_INFO, "  Wafer ID     = 0x%02x (%d)\n", ecid->wafer_id, ecid->wafer_id);
    Log_Write(LOG_LEVEL_INFO, "  X Coordinate = 0x%02x (%d)\n", ecid->x_coordinate,
              ecid->x_coordinate);
    Log_Write(LOG_LEVEL_INFO, "  Y Coordinate = 0x%02x (%d)\n", ecid->y_coordinate,
              ecid->y_coordinate);

    return 0;
}
