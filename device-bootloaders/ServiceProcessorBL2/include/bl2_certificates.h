/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#ifndef __BL2_SP_CERTIFICATES_H__
#define __BL2_SP_CERTIFICATES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "service_processor_BL2_data.h"

int verify_esperanto_image_certificate(const ESPERANTO_IMAGE_TYPE_t image_type,
                                       const ESPERANTO_CERTIFICATE_t *certificate);

int load_sw_certificates_chain(void);

#endif
