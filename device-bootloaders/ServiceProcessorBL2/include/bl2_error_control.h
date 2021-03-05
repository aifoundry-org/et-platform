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
/*! \file bl2_error_control.h
    \brief A C header that defines the Error control service's
    public interfaces. These interfaces provide services using which
    the host can query device for error control details.
*/
/***********************************************************************/
#ifndef __BL2_ERROR_CONTROL_H__
#define __BL2_ERROR_CONTROL_H__

#include <stdint.h>
#include "dm.h"

/*! \fn void error_control_process_request(tag_id_t tag_id, msg_id_t msg_id)
    \brief Interface to process the error control command request
     by the msg_id
    \param tag_id Tag ID
    \param msg_id ID of the command received
    \returns none
*/
void error_control_process_request(tag_id_t tag_id, msg_id_t msg_id );
#endif
