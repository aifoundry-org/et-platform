#include <stdio.h>
#include "tf.h"
#include "serial.h"
#include "hwinc/hal_device.h"
#include <string.h>

/* Globals */
static char Input_Cmd_Buffer[TF_MAX_CMD_SIZE];
static char Output_Rsp_Buffer[TF_MAX_RSP_SIZE];

/* Initialize to earliest interception point in BL1 */
static uint8_t TF_Interception_Point = TF_DEFAULT_ENTRY;

extern int8_t (*TF_Test_Cmd_Handler[TF_NUM_COMMANDS])(void *test_cmd);

uint32_t byte_copy(char* src, char* dst, uint32_t size);

uint32_t byte_copy(char* src, char* dst, uint32_t size)
{
    uint32_t retval=0;
    char *p_src=src;
    char *p_dst=dst;

    while(size--)
    {
        *p_src = *p_dst;
        retval++;

        p_src++;
        p_dst++;
    }

    return retval;
}

uint8_t TF_Set_Entry_Point(uint8_t intercept)
{
    uint8_t retval = TF_DEFAULT_ENTRY;

    if((intercept > TF_DEFAULT_ENTRY) && (intercept <= TF_BL2_ENTRY_FOR_SP_MM))
    {
        TF_Interception_Point = retval = intercept;
    }

    return retval;
}

uint8_t TF_Get_Entry_Point(void)
{
    return TF_Interception_Point;
}

int8_t TF_Wait_And_Process_TF_Cmds(int8_t intercept)
{
    char c;
    char *p_buff = &Input_Cmd_Buffer[0];
    void *p_hdr=0;
    struct header_t cmd_hdr;
    int8_t rtn_arg;

    /* First entry unconditionally hook-in */
    /* Subsequent entries fall thru if current intercept
    is not equal to TF_Interception_Point set by host */
    if(TF_Interception_Point != TF_DEFAULT_ENTRY)
    {
        if(intercept != TF_Interception_Point)
        {
            return 0;
        }
    }

    for(;;)
    {
        do
        {
            SERIAL_getchar(UART1, &c);

            if(c == TF_CMD_START)
            {
                p_hdr = (void*) (p_buff+1);
            }

            *p_buff = c;
            p_buff++;

        } while (c != TF_CHECKSUM_END);

        byte_copy((char*)&cmd_hdr, (char*)p_hdr,
            sizeof(struct header_t));

        rtn_arg = TF_Test_Cmd_Handler[cmd_hdr.id](p_hdr);

        if(rtn_arg == TF_EXIT_FROM_TF_LOOP)
        {
            break;
        }
    }

    return 0;
}

static void fill_rsp_buffer(uint32_t *buf_size, void *buffer,
    uint32_t rsp_size)
{
    uint32_t start_idx = TF_MAX_RSP_SIZE - *buf_size;
    char* src_buff = buffer;
    char* p_rsp = &Output_Rsp_Buffer[start_idx];

    while(rsp_size)
    {
        if (rsp_size > *buf_size)
        {
            memcpy(p_rsp, src_buff, *buf_size);
            SERIAL_write(UART1, &Output_Rsp_Buffer[0], TF_MAX_RSP_SIZE);

            src_buff += *buf_size;
            p_rsp = &Output_Rsp_Buffer[0];
            rsp_size -= *buf_size;
            *buf_size = TF_MAX_RSP_SIZE;
        }
        else
        {
            memcpy(p_rsp, src_buff, rsp_size);
            p_rsp += rsp_size;
            *buf_size -= rsp_size;
            rsp_size -= rsp_size;
        }
    }

    if (*buf_size == 0)
        *buf_size = TF_MAX_RSP_SIZE;
}

int8_t TF_Send_Response_With_Payload(void *rsp, uint32_t rsp_size,
    void *additional_rsp, uint32_t additional_rsp_size)
{
    uint32_t magic=0xA5A5A5A5;
    uint32_t buf_size = TF_MAX_RSP_SIZE;

    Output_Rsp_Buffer[0] = TF_CMD_START;
    buf_size--;

    fill_rsp_buffer(&buf_size, rsp, rsp_size);

    if (additional_rsp_size)
    {
        fill_rsp_buffer(&buf_size, additional_rsp, additional_rsp_size);
    }

    // Append END and CHECKSUM delimiter
    if (buf_size < TF_CHECKSUM_SIZE + 2)
    {
        SERIAL_write(UART1, &Output_Rsp_Buffer[0], (int)(TF_MAX_RSP_SIZE - buf_size));
        buf_size = TF_MAX_RSP_SIZE;
    }

    char* p_rsp = &Output_Rsp_Buffer[TF_MAX_RSP_SIZE - buf_size];
    *p_rsp = TF_CMD_END;
    p_rsp++;
    buf_size--;
    memcpy(p_rsp, (char*)&magic, TF_CHECKSUM_SIZE);
    p_rsp += TF_CHECKSUM_SIZE;
    buf_size -= TF_CHECKSUM_SIZE;
    *p_rsp = TF_CHECKSUM_END;
    buf_size--;
    SERIAL_write(UART1, &Output_Rsp_Buffer[0], (int)(TF_MAX_RSP_SIZE - buf_size));

    return 0;
}

int8_t TF_Send_Response(void* rsp, uint32_t rsp_size)
{
    return TF_Send_Response_With_Payload(rsp, rsp_size, NULL, 0);
}

