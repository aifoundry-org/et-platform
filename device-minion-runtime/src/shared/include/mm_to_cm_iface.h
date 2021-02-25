#ifndef MM_TO_CM_IFACE_H
#define MM_TO_CM_IFACE_H

#include "message_types.h"

/*
 * Called by Master Firmware
 */
int64_t MM_To_CM_Iface_Multicast_Send(uint64_t dest_shire_mask, cm_iface_message_t *const message);

/*
 * Called by Compute Firmware
 */
bool MM_To_CM_Iface_Multicast_Receive_Message_Available(cm_iface_message_number_t previous_broadcast_message_number);
cm_iface_message_number_t MM_To_CM_Iface_Multicast_Receive(cm_iface_message_t *const message);

#endif
