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

/*! \fn int32_t NOC_Remap_Shire_Id(int displace, int spare)
    \brief This funtion remaps shires
    \param displace the ID of the shire to be displaced
    \param spare the ID of the spare shire
    \return Status indicating success or negative error
*/
int32_t NOC_Remap_Shire_Id(int displace, int spare);

/*! \fn int32_t Set_Displace_Shire_Id(uint64_t shire_mask)
    \brief This funtion sets the global displace shire ID value
    \param shire_mask shire mask to calculate the ID of the shire to be displaced
    \return Status indicating success or negative error
*/
int32_t Set_Displace_Shire_Id(uint64_t shire_mask);

/*! \fn int32_t Set_Displace_Shire_Id(uint64_t shire_mask)
    \brief This funtion gets the displace shire ID value
    \param n/a
    \return Status indicating success or negative error
*/
int Get_Displace_Shire_Id(void);

/*! \fn int32_t Set_Displace_Shire_Id(uint64_t shire_mask)
    \brief This funtion gets the spare shire ID value
    \param n/a
    \return Status indicating success or negative error
*/
int Get_Spare_Shire_Id(void);

#endif
