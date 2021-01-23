#ifndef MM_IFACE_H
#define MM_IFACE_H

#include "message.h"

/* MM to CM interface */
void MM_To_CM_Iface_Process(void);

/* CM to MM interface */
int8_t CM_To_MM_Iface_Unicast_Send(uint64_t ms_thread_id, uint64_t cb_idx, const cm_iface_message_t *const message);

#endif
