#ifndef CM_TO_MM_IFACE_H
#define CM_TO_MM_IFACE_H

#include "message_types.h"

/*! \def CM_MM_MASTER_HART_UNICAST_BUFF_IDX
    \brief A macro that provides the index of the unicast buffer associated
    with Master HART within the master shire.
*/
#define CM_MM_MASTER_HART_UNICAST_BUFF_IDX  0U

/*! \def CM_MM_KW_HART_UNICAST_BUFF_BASE_IDX
    \brief A macro that provides the base index of the unicast buffer
    associated with Kernel Workers.
*/
#define CM_MM_KW_HART_UNICAST_BUFF_BASE_IDX  1U

// Not thread safe. Only one caller per cb_idx
int8_t CM_To_MM_Iface_Unicast_Receive(uint64_t cb_idx, cm_iface_message_t *const message);

#endif
