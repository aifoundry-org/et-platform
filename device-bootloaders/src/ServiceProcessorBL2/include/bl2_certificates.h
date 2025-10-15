/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file bl2_certificates.h
    \brief A C header that defines the secure sign in public interfaces.
*/
/***********************************************************************/
#ifndef __BL2_SP_CERTIFICATES_H__
#define __BL2_SP_CERTIFICATES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "service_processor_BL2_data.h"

/*! \fn int verify_esperanto_image_certificate(const ESPERANTO_IMAGE_TYPE_t image_type,
                                       const ESPERANTO_CERTIFICATE_t *certificate)
    \brief This function compares the esperanto binary image's authenticity by 
           comparing it's certificate.
    \param image_type binary image type
    \param certificate pointer to certificate  
    \return Status indicating success or negative error
*/
int verify_esperanto_image_certificate(const ESPERANTO_IMAGE_TYPE_t image_type,
                                       const ESPERANTO_CERTIFICATE_t *certificate);

/*! \fn int load_sw_certificates_chain(void)
    \brief This function loads certificates chain
    \param None 
    \return Status indicating success or negative error
*/
int load_sw_certificates_chain(void);

#endif
