#include "tf.h"
#include "mm_iface.h"
#include "log.h"

int8_t MM_Cmd_Shell_Cmd_Handler(void* test_cmd);
int8_t MM_Cmd_Shell_Debug_Print_Cmd_Handler(void* test_cmd);

int8_t MM_Cmd_Shell_Debug_Print_Cmd_Handler(void* test_cmd)
{
    const tf_cmd_hdr_t *cmd_hdr = test_cmd;

    struct mm_hdr_template {
        uint16_t size;
        uint16_t tag_id;
        uint16_t msg_id;
        uint16_t flags;
    };

    const struct mm_hdr_template *mm_cmd_hdr =
        (void *)((char*)test_cmd + sizeof(tf_cmd_hdr_t) + sizeof(uint32_t));

    Log_Write(LOG_LEVEL_INFO, "Host2SP:TF SP Command.\r\n");
    Log_Write(LOG_LEVEL_INFO, "Host2SP:tf_cmd.id = %d.\r\n", cmd_hdr->id);
    Log_Write(LOG_LEVEL_INFO, "Host2SP:tf_cmd.flags = %d.\r\n", cmd_hdr->flags);
    Log_Write(LOG_LEVEL_INFO, "Host2SP:tf_cmd.payload_size = %d.\r\n", cmd_hdr->payload_size);

    if(cmd_hdr->id == TF_CMD_MM_CMD_SHELL)
    {
        Log_Write(LOG_LEVEL_INFO, "ToMM:TF MM Command.\r\n");
        Log_Write(LOG_LEVEL_INFO, "ToMM:mm_cmd.size = %d.\r\n", mm_cmd_hdr->size);
        Log_Write(LOG_LEVEL_INFO, "ToMM:mm_cmd.tag_id = %d.\r\n", mm_cmd_hdr->tag_id);
        Log_Write(LOG_LEVEL_INFO, "ToMM:mm_cmd.msg_id = %d.\r\n", mm_cmd_hdr->msg_id);
        Log_Write(LOG_LEVEL_INFO, "ToMM:mm_cmd.flags = %d.\r\n", mm_cmd_hdr->flags);
    }

    return 0;
}

int8_t MM_Cmd_Shell_Cmd_Handler(void* test_cmd)
{
    char sp_rsp_buff[256];
    uint32_t sp_rsp_size = 0;
    int32_t status = 0;
    void *p_mm_cmd_base;
    void *p_mm_cmd_size;
    uint32_t mm_cmd_size = 0;

    struct tf_rsp_mm_cmd_shell_t mm_shell_rsp;

    p_mm_cmd_base = (void *)((char*)test_cmd + sizeof(tf_cmd_hdr_t) + sizeof(uint32_t));
    p_mm_cmd_size = (void *)((char*)test_cmd + sizeof(tf_cmd_hdr_t));
    mm_cmd_size = (uint32_t)*((uint16_t*)p_mm_cmd_size);

    Log_Write(LOG_LEVEL_INFO, "Host2SP:MM_Cmd_Shell_Handler.\r\n");

    status = MM_Iface_MM_Command_Shell(p_mm_cmd_base, mm_cmd_size,
        &sp_rsp_buff[0], &sp_rsp_size);

    if(status == 0)
    {
        mm_shell_rsp.rsp_hdr.id = TF_RSP_MM_CMD_SHELL;
        mm_shell_rsp.rsp_hdr.flags = 2;
        mm_shell_rsp.rsp_hdr.payload_size = (uint32_t)sizeof(uint32_t) + sp_rsp_size;
        mm_shell_rsp.mm_rsp_size = sp_rsp_size;

        const char* src = (char*)(sp_rsp_buff);
        char* dst = (char*)(&mm_shell_rsp.mm_rsp_data);

        while(sp_rsp_size--)
        {
            *dst = *src;
            dst++; src++;
        }

        TF_Send_Response(&mm_shell_rsp, (uint32_t)(sizeof(tf_rsp_hdr_t) + sizeof(uint32_t) + mm_shell_rsp.mm_rsp_size));
    }
    else
    {
        mm_shell_rsp.rsp_hdr.id = TF_RSP_MM_CMD_SHELL;
        mm_shell_rsp.rsp_hdr.flags = 2;
        mm_shell_rsp.rsp_hdr.payload_size = 0;
        mm_shell_rsp.mm_rsp_size = 0;

        TF_Send_Response(&mm_shell_rsp, (uint32_t)(sizeof(tf_rsp_hdr_t) + sizeof(uint32_t)));
    }

    return 0;
}
