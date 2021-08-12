#ifndef TF_DEFS_H
#define TF_DEFS_H

#include "common_defs.h"
#include <esperanto/tf-protocol/tf_protocol.h>

int8_t TF_Wait_And_Process_TF_Cmds(void);
int8_t TF_Send_Response(void* rsp, uint32_t rsp_size);

#endif /* TF_DEFS_H */
