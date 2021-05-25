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
#ifndef __BL2_BUILD_CONFIGURATION_H__
#define __BL2_BUILD_CONFIGURATION_H__

#include <stdint.h>
#include "esperanto_signed_image_format/executable_image.h"
#include "../../ServiceProcessorBL2/include/build_configuration.h"

const IMAGE_VERSION_INFO_t *get_image_version_info(void);

#endif
