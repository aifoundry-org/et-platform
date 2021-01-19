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
#define SUCCESS   0

/*! \def Main::NOC Setup Error Return code  */
#define NOC_MAIN_CLOCK_CONFIGURE_ERROR    -1000

/*! \def Main::MEMSHIRE Setup Error Return code  */
#define MEMSHIRE_COLD_RESET_CONFIG_ERROR -2000
#define MEMSHIRE_PLL_CONFIG_ERROR        -2001
#define MEMSHIRE_DDR_CONFIG_ERROR        -2002

/*! \def Main::Minion Setup Error Return code  */
#define MINION_STEP_CLOCK_CONFIGURE_ERROR -3000
#define MINION_COLD_RESET_CONFIG_ERROR    -3001
#define MINION_WARM_RESET_CONFIG_ERROR    -3002
#define MINION_PLL_DLL_CONFIG_ERROR       -3003

/*! \def Main::FW Load and Authenticate Setup Error Return code  */
#define FW_SW_CERTS_LOAD_ERROR  -4000
#define FW_MACH_LOAD_ERROR      -4001
#define FW_MM_LOAD_ERROR        -4002
#define FW_CM_LOAD_ERROR        -4003

#endif
