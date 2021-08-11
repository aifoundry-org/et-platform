#ifndef TF_DEFS_H
#define TF_DEFS_H

#include "common_defs.h"
#include <esperanto/tf-protocol/tf_protocol.h>

enum interception_points_t {
    TF_DEFAULT_ENTRY = 0,
    TF_BL1_ENTRY = 1,
    TF_BL2_ENTRY_FOR_HW = 2,
    TF_BL2_ENTRY_FOR_DM = 3,
    TF_BL2_ENTRY_FOR_DM_WITH_PCIE = 4,
    TF_BL2_ENTRY_FOR_SP_MM = 5
};

uint8_t TF_Set_Entry_Point(uint8_t intercept);
uint8_t TF_Get_Entry_Point(void);
int8_t TF_Wait_And_Process_TF_Cmds(int8_t intercept);
int8_t TF_Send_Response(void* rsp, uint32_t rsp_size);

#endif /* TF_DEFS_H */
