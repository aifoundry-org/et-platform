/***********************************************************************
*
* Copyright (C) 2024 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file fru.h
    \brief A C header that provides abstraction for Field Replaceable Unit (FRU) service's
    interfaces. These interfaces provide services using which
    the host can query device for asset details.
*/
/***********************************************************************/
#ifndef FRU_H
#define FRU_H

#include "dm.h"
#include "sp_host_iface.h"
#include "bl2_pmic_controller.h"

/*! \fn void fru_process_request()
    \brief Interface to process FRU commands from Host
    \param tag_id Transaction ID of the Host command
    \param msg_id Command ID to identify FRU GET/SET
    \param buffer Buffer carrying payload from Host  
*/

void fru_process_request(tag_id_t tag_id, msg_id_t msg_id, const void *buffer);

#endif
