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
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       Error Codes for all error condititions within the Service
*       Processor BootLoader 2 code
*
***********************************************************************/

#ifndef __SP_BL2_RETURN_CODE_H__
#define __SP_BL2_RETURN_CODE_H__

/*! \def Default Success Return code  */
#define SUCCESS 0

/*! \def Generic errors: [-1, -999] */
#define ERROR_INVALID_ARGUMENT -1

/*! \def Main::NOC Setup Error Return code  */
#define NOC_MAIN_CLOCK_CONFIGURE_ERROR -1000

/*! \def Main::MEMSHIRE Setup Error Return code  */
#define MEMSHIRE_COLD_RESET_CONFIG_ERROR -2000
#define MEMSHIRE_PLL_CONFIG_ERROR        -2001
#define MEMSHIRE_DDR_CONFIG_ERROR        -2002

/*! \def Main::Minion Setup Error Return code  */
#define MINION_STEP_CLOCK_CONFIGURE_ERROR    -3000
#define MINION_COLD_RESET_CONFIG_ERROR       -3001
#define MINION_WARM_RESET_CONFIG_ERROR       -3002
#define MINION_PLL_DLL_CONFIG_ERROR          -3003
#define MINION_INVALID_SHIRE_MASK            -3004

/*! \def Main::FW Load and Authenticate Setup Error Return code  */
#define FW_SW_CERTS_LOAD_ERROR -4000
#define FW_MACH_LOAD_ERROR     -4001
#define FW_MM_LOAD_ERROR       -4002
#define FW_CM_LOAD_ERROR       -4003

/* APIs Error Return code */

/*! \def SPI FLASH Error Codes. */
#define ERROR_SPI_FLASH_RDSR_FAILED                -5000
#define ERROR_SPI_FLASH_TIMEOUT_WAITING_MEM_READY  -5001
#define ERROR_SPI_FLASH_WREN_FAILED                -5002
#define ERROR_SPI_FLASH_MEM_NOT_READY              -5003
#define ERROR_SPI_FLASH_INVALID_ARGUMENTS          -5004
#define ERROR_SPI_FLASH_RDID_FAILED                -5005
#define ERROR_SPI_FLASH_RDSFDP_FAILED              -5006
#define ERROR_SPI_FLASH_NORMAL_RD_FAILED           -5007
#define ERROR_SPI_FLASH_FAST_RD_FAILED             -5008
#define ERROR_SPI_FLASH_PP_FAILED                  -5009
#define ERROR_SPI_FLASH_BE_FAILED                  -5010
#define ERROR_SPI_FLASH_SE_FAILED                  -5011
#define ERROR_SPI_FLASH_BOOT_COUNTER_REGION_FULL   -5012
#define ERROR_SPI_FLASH_REGION_CRC_MISMATCH        -5013
#define ERROR_SPI_FLASH_INVALID_REGION_SIZE_OFFSET -5014
#define ERROR_SPI_FLASH_INVALID_REGION_ID          -5015
#define ERROR_SPI_FLASH_FS_INIT_FAILED             -5016
#define ERROR_SPI_FLASH_NO_VALID_PARTITION         -5017
#define ERROR_SPI_FLASH_INVALID_FILE_INFO          -5018
#define ERROR_SPI_FLASH_LOAD_FILE_INFO_FAILED      -5019
#define ERROR_SPI_FLASH_INVALID_FILE_SIZE_OFFSET   -5020
#define ERROR_SPI_FLASH_PARTITION_ERASE_FAILED     -5021
#define ERROR_SPI_FLASH_PARTITION_PROGRAM_FAILED   -5022
#define ERROR_SPI_FLASH_BL2_INFO_IS_NULL           -5023

/*! \def PMIC I2C Error Codes. */
#define ERROR_PMIC_I2C_READ_FAILED          -6000
#define ERROR_PMIC_I2C_WRITE_FAILED         -6001
#define ERROR_PMIC_I2C_INVALID_ARGUMENTS    -6002
#define ERROR_PMIC_I2C_INVALID_VOLTAGE_TYPE -6003
#define ERROR_PMIC_I2C_INVALID_MINION_GROUP -6004
#define ERROR_PMIC_INIT_FAILED              -6005

/*! \def VaultIP Error Codes. */
#define ERROR_VAULTIP_COMMAND_FAILED -7000

/*! \def GPIO Error Codes. */
#define ERROR_GPIO_INVALID_ID        -8000
#define ERROR_GPIO_INVALID_ARGUMENTS -8001

/*! \def THERMAL_PWR_MGMT Error Codes. */
#define THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED        -9000
#define THERMAL_PWR_MGMT_EVENT_NOT_INITIALIZED     -9001
#define THERMAL_PWR_MGMT_TASK_CREATION_FAILED      -9002
#define THERMAL_PWR_MGMT_MINION_FREQ_UPDATE_FAILED -9003
#define THERMAL_PWR_MGMT_UNKNOWN_THROTTLE_STATE    -9004
#define THERMAL_PWR_MGMT_UNKNOWN_POWER_STATE       -9005
#define THERMAL_PWR_MGMT_INVALID_TDP_LEVEL         -9006

/*! \def MM IFACE Error Codes. */
#define MM_IFACE_SP2MM_CMD_ERROR        -10000
#define MM_IFACE_SP2MM_TIMEOUT_ERROR    -10001
#define MM_IFACE_SP2MM_INVALID_RESPONSE -10002
#define MM_IFACE_SP2MM_CMD_PUSH_ERROR   -10003

/*! \def PERF MGMT Error Codes. */
#define ERROR_PERF_MGMT_FAILED_TO_GET_DRAM_BW -11000
#define ERROR_PERF_MGMT_FAILED_TO_GET_FREQ    -11001

/*! \def SP OTP Error Codes. */
#define ERROR_SP_OTP_OTP_NOT_AVAILABLE  -12000
#define ERROR_SP_OTP_OTP_READ           -12001
#define ERROR_SP_OTP_BANK_LOCKED        -12002
#define ERROR_SP_OTP_BANK_NOT_LOCKED    -12003
#define ERROR_SP_OTP_SP_WRCK_NOT_100MHZ -12004

#endif
