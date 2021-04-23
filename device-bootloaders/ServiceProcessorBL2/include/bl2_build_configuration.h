#ifndef __BL2_BUILD_CONFIGURATION_H__
#define __BL2_BUILD_CONFIGURATION_H__

#include <stdint.h>

#include "esperanto_signed_image_format/executable_image.h"

#include "../../ServiceProcessorBL2/include/build_configuration.h"

const IMAGE_VERSION_INFO_t *get_image_version_info(void);

#endif
