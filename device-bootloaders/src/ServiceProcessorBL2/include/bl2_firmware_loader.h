/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
************************************************************************/
/*! \file bl2_firmware_loader.h
    \brief A C header that defines the Firmware load service's
    public interfaces. 
*/
/***********************************************************************/

#ifndef __BL2_SP_FIRMWARE_LOADER_H__
#define __BL2_SP_FIRMWARE_LOADER_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "service_processor_BL2_data.h"

/*! \fn int load_firmware(const ESPERANTO_IMAGE_TYPE_t image_type)
    \brief Interface to process the load firmware service 
    \param image_type type of firmware image
    \returns none
*/
int load_firmware(const ESPERANTO_IMAGE_TYPE_t image_type);
ESPERANTO_IMAGE_FILE_HEADER_t *get_master_minion_image_file_header(void);
ESPERANTO_IMAGE_FILE_HEADER_t *get_worker_minion_image_file_header(void);
ESPERANTO_IMAGE_FILE_HEADER_t *get_machine_minion_image_file_header(void);
#endif
