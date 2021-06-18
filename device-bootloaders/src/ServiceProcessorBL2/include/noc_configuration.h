/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file noc_configuration.h
    \brief A C header that defines the NOC configuration service's
    public interfaces.
*/
/***********************************************************************/
#ifndef __NOC_CONFIGURATION_H__
#define __NOC_CONFIGURATION_H__

/*! \fn int32_t NOC_Configure(uint8_t mode)
    \brief This configures the Main NOC
    \param PLL mode to be used 
    \return Status indicating success or negative error
*/

int32_t NOC_Configure(uint8_t mode);

#endif
