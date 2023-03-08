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
#ifndef __BL2_OTP__
#define __BL2_OTP__

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "sp_otp.h"
#include "bl_error_code.h"
#include "etsoc/isa/io.h"
#include "hwinc/hal_device.h"
#include "hwinc/sp_cru_reset.h"
#include "hwinc/sp_cru.h"

/*! \struct ecid_t
    \brief This structure holds the Electronic Chip ID
    \param none
    \returns Status indicating success or negative error
*/
#define ECID_LOT_ID_LENGTH 6
typedef struct
{
    uint64_t lot_id;
    uint8_t wafer_id;
    uint8_t x_coordinate;
    uint8_t y_coordinate;
    char lot_id_str[ECID_LOT_ID_LENGTH + 1];
} ecid_t;

/*! \fn int otp_get_chip_revision(char *chip_rev)
    \brief Interface read word from OTP
    \param bank
    \param row
    \param mask
    \returns data word
*/
uint32_t OTP_Read_Word(uint32_t bank, uint32_t row, uint32_t mask);

/*! \fn uint8_t* OTP_Write_Word (uint32_t Bank, uint32_t Row, uint32_t mask, uint32_t data)
    \brief Interface read word from OTP
    \param bank
    \param row
    \param mask
    \param data data to be written
    \returns Status indicating success or negative error
*/
int OTP_Write_Word(uint32_t Bank, uint32_t Row, uint32_t mask, uint32_t data);

/*! \fn int otp_get_chip_revision(char *chip_rev)
    \brief Interface to get the chip revision
    \param *chip_rev Pointer to chip revision variable
    \returns Status indicating success or negative error
*/
int otp_get_chip_revision(char *chip_rev);

/*! \fn int otp_get_master_shire_id(uint8_t *mm_id)
    \brief Interface to get the chip master shire ID
    \param mm_id Pointer to master shire ID variable
    \returns Status indicating success or negative error
*/
int otp_get_master_shire_id(uint8_t *mm_id);

/*! \fn int otp_get_shire_speed(uint8_t shire_num, uint8_t *speed)
    \brief This function reads Shire Speed from OTP memory
    \param shire_num Index of Shire
    \param speed Pointer to speed variable
    \returns Status indicating success or negative error
*/
int otp_get_shire_speed(uint8_t shire_num, uint8_t *speed);

/*! \fn int read_ecid(void)
    \brief This function reads ECID OTP memory
    \param ecid pointer to a ecid_t structure
    \returns Status indicating success or negative error
*/
int read_ecid(ecid_t *ecid);

#endif
