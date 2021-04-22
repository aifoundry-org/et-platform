#ifndef MM_TO_CM_IFACE_H
#define MM_TO_CM_IFACE_H

#include "message_types.h"

/*! \def TIMEOUT_MM_CM_MSG
    \brief Timeout value (in mS) for MM->CM messages
*/
#define TIMEOUT_MM_CM_MSG    1

int8_t MM_To_CM_Iface_Multicast_Send(uint64_t dest_shire_mask, cm_iface_message_t *const message);
void MM_to_CM_Iface_Multicast_Timeout_Cb(uint8_t arg);

#endif
