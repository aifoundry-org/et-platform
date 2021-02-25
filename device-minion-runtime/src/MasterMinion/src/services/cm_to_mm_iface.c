#include "layout.h"
#include "circbuff.h"
#include "cm_mm_defines.h"
#include "services/cm_to_mm_iface.h"

int8_t CM_To_MM_Iface_Unicast_Receive(uint64_t cb_idx, cm_iface_message_t *const message)
{
    int8_t status;
    circ_buff_cb_t *cb = (circ_buff_cb_t *)(CM_MM_IFACE_UNICAST_CIRCBUFFERS_BASE_ADDR +
                                            cb_idx * CM_MM_IFACE_CIRCBUFFER_SIZE);

    /* Peek the command size to pop */
    status = Circbuffer_Peek(cb, (void *)&message->header, 0, sizeof(message->header), L3_CACHE);

    if (status == STATUS_SUCCESS)
    {
        /* Pop the command from circular buffer */
        status = Circbuffer_Pop(cb, message, sizeof(*message), L3_CACHE);
    }

    return status;
}
