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