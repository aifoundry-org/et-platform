#ifndef MM_TO_CM_IFACE_H
#define MM_TO_CM_IFACE_H

#include "message_types.h"

int64_t MM_To_CM_Iface_Multicast_Send(uint64_t dest_shire_mask, cm_iface_message_t *const message);

#endif
