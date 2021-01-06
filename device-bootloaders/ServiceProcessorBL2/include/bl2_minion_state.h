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
/*! \file bl2_minion_state.h
    \brief A C header that defines the Minion State service's
    public interfaces. These interfaces provide services using which
    the host can query the minion/thread states.
*/
/***********************************************************************/
#ifndef __BL2_MINION_STATE_H__
#define __BL2_MINION_STATE_H__

/*! \fn void mm_state_process_request(tag_id_t tag_id)
    \brief Interface to process the master minion state request
    \param tag_id Tag ID
    \returns none
*/
void mm_state_process_request(tag_id_t tag_id);

#endif
