#ifndef TF_DEFS_H
#define TF_DEFS_H

#include "etsoc/common/common_defs.h"
#include <esperanto/tf-protocol/tf_protocol.h>

#define TF_EXIT_FROM_TF_LOOP    -1

uint8_t TF_Set_Entry_Point(uint8_t intercept);
uint8_t TF_Get_Entry_Point(void);
int8_t TF_Wait_And_Process_TF_Cmds(int8_t intercept);
int8_t TF_Send_Response(void* rsp, uint32_t rsp_size);
int8_t TF_Send_Response_With_Payload(void *rsp, uint32_t rsp_size,
    void *additional_rsp, uint32_t additional_rsp_size);

#endif /* TF_DEFS_H */
