/*-------------------------------------------------------------------------
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef __ERROR_H__
#define __ERROR_H__

// Generic errors: [0, -999]
#define ERROR_NO_ERROR         0
#define ERROR_INVALID_ARGUMENT -1

// SPI FLASH
#define ERROR_SPI_FLASH_RDSR_FAILED                  -4000
#define ERROR_SPI_FLASH_TIMEOUT_WAITING_MEM_READY    -4001
#define ERROR_SPI_FLASH_WREN_FAILED                  -4002
#define ERROR_SPI_FLASH_MEM_NOT_READY                -4003
#define ERROR_SPI_FLASH_INVALID_ARGUMENTS            -4004
#define ERROR_SPI_FLASH_RDID_FAILED                  -4005
#define ERROR_SPI_FLASH_RDSFDP_FAILED                -4006
#define ERROR_SPI_FLASH_NORMAL_RD_FAILED             -4007
#define ERROR_SPI_FLASH_FAST_RD_FAILED               -4008
#define ERROR_SPI_FLASH_PP_FAILED                    -4009
#define ERROR_SPI_FLASH_BE_FAILED                    -4010
#define ERROR_SPI_FLASH_SE_FAILED                    -4011
#define ERROR_SPI_FLASH_BOOT_COUNTER_REGION_FULL     -4012
#define ERROR_SPI_FLASH_REGION_CRC_MISMATCH          -4013
#define ERROR_SPI_FLASH_INVALID_REGION_SIZE_OFFSET   -4014
#define ERROR_SPI_FLASH_INVALID_REGION_ID            -4015
#define ERROR_SPI_FLASH_FS_INIT_FAILED               -4016
#define ERROR_SPI_FLASH_NO_VALID_PARTITION           -4017
#define ERROR_SPI_FLASH_INVALID_FILE_INFO            -4018
#define ERROR_SPI_FLASH_LOAD_FILE_INFO_FAILED        -4019
#define ERROR_SPI_FLASH_INVALID_FILE_SIZE_OFFSET     -4020
#define ERROR_SPI_FLASH_PARTITION_ERASE_FAILED       -4021
#define ERROR_SPI_FLASH_PARTITION_PROGRAM_FAILED     -4022
#define ERROR_SPI_FLASH_BL2_INFO_IS_NULL             -4023

// PMIC I2C
#define ERROR_PMIC_I2C_READ_FAILED                   -5000
#define ERROR_PMIC_I2C_WRITE_FAILED                  -5001
#define ERROR_PMIC_I2C_INVALID_ARGUMENTS             -5002
#define ERROR_PMIC_I2C_INVALID_VOLTAGE_TYPE          -5003
#define ERROR_PMIC_I2C_INVALID_MINION_GROUP          -5004
#define ERROR_PMIC_INIT_FAILED                       -5005

#endif
