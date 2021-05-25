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
#include "error.h"
#include "io.h"
#include "etsoc_hal/inc/hal_device.h"
#include "etsoc_hal/inc/rm_esr.h"
#include "etsoc_hal/inc/cm_esr.h"

/*! \fn uint8_t* OTP_Initialize()
    \brief Interface to initialize OTP
    \param None
    \returns Status indicating success or negative error
*/
int OTP_Initialize(void);

/*! \fn int otp_get_chip_revision(char *chip_rev)
    \brief Interface read word from OTP
    \param bank 
    \param row 
    \param mask 
    \returns data word
*/
uint32_t OTP_Read_Word (uint32_t bank, uint32_t row, uint32_t mask);

/*! \fn uint8_t* OTP_Write_Word (uint32_t Bank, uint32_t Row, uint32_t mask, uint32_t data)
    \brief Interface read word from OTP
    \param bank 
    \param row 
    \param mask 
    \param data data to be written 
    \returns Status indicating success or negative error
*/
int OTP_Write_Word (uint32_t Bank, uint32_t Row, uint32_t mask, uint32_t data);

/*! \fn int otp_get_chip_revision(char *chip_rev)
    \brief Interface to get the chip revision
    \param *chip_rev  Pointer to chip revision variable
    \returns Status indicating success or negative error
*/
int otp_get_chip_revision(char *chip_rev);
#endif