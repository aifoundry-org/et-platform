#ifndef CM_TO_MM_IFACE_H
#define CM_TO_MM_IFACE_H

#include "message_types.h"

// Thread safe. Can be called by multiple threads (takes a lock)
int8_t CM_To_MM_Iface_Unicast_Send(uint64_t ms_thread_id, uint64_t cb_idx, const cm_iface_message_t *const message);

#endif
