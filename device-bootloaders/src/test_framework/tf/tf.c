#include <stdio.h>
#include "tf.h"
#include "etsoc/drivers/serial/serial.h"
#include "hwinc/hal_device.h"
#include <string.h>

/* Globals */
static char Input_Cmd_Buffer[TF_MAX_CMD_SIZE];
static char Output_Rsp_Buffer[TF_MAX_RSP_SIZE];

/* Initialize to earliest interception point in BL1 */
static uint8_t TF_Interception_Point = TF_DEFAULT_ENTRY;

extern int8_t (*TF_Test_Cmd_Handler[TF_NUM_COMMANDS])(void *test_cmd);

uint8_t TF_Set_Entry_Point(uint8_t intercept)
{
    uint8_t retval = TF_Get_Entry_Point();

    if((intercept > retval) && (intercept <= TF_BL2_ENTRY_FOR_SP_MM))
    {
        TF_Interception_Point = retval = intercept;
    }

    return retval;
}

uint8_t TF_Get_Entry_Point(void)
{
    return TF_Interception_Point;
}


#define TF_CMD_HDR_BYTES    (TF_HEADER_ID_BYTES + TF_HEADER_FLAGS_BYTES + TF_HEADER_PLAYLOADSIZE_BYTES)

int8_t TF_Wait_And_Process_TF_Cmds(int8_t intercept)
{
    char            c;
    int8_t          rtn_arg;
    char            *p_buff;
    uint32_t        bytes_received;
    uint32_t        cmd_bytes_received;
    struct header_t tf_cmd_hdr;
    struct header_t *p_tf_cmd_hdr;
    bool            tf_prot_start_found=false;
    bool            tf_cmd_size_available=false;

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
        p_buff = &Input_Cmd_Buffer[0];
        p_tf_cmd_hdr = (void*)&Input_Cmd_Buffer[0];
        bytes_received = 0;
        cmd_bytes_received = 0;
        tf_prot_start_found = false;
        tf_cmd_size_available = false;

        printf("Getting into TF RX loop \r\n");

        printf("command received\r\n");
        while(true)
        {
            SERIAL_getchar(SP_UART1, &c);

            if((c == TF_CMD_START) && (!tf_prot_start_found))
            {
                tf_prot_start_found = true;
            }
            else
            {
                *p_buff = c;

                printf("0x");
                printf("%02X", *p_buff);
                printf("%s", ",");

                p_buff++;
                bytes_received++;

                if(bytes_received == TF_CMD_HDR_BYTES)
                {

                    memcpy(&tf_cmd_hdr, p_tf_cmd_hdr, sizeof(tf_cmd_hdr));
                    printf("\r\n");
                    printf("command_id = %d\r\n", tf_cmd_hdr.id);
                    printf("command_flags = %d\r\n", tf_cmd_hdr.flags);
                    printf("command_size = %d\r\n", tf_cmd_hdr.payload_size);
                    tf_cmd_size_available = true;
                }

                if(tf_cmd_size_available)
                {
                    cmd_bytes_received++;

                    if(cmd_bytes_received == (tf_cmd_hdr.payload_size + TF_CHECKSUM_SIZE))
                    {
                        printf("command fully received, size = %d \r\n", cmd_bytes_received);
                        tf_prot_start_found = false;
                        tf_cmd_size_available = false;
                        bytes_received = 0;
                        cmd_bytes_received = 0;
                        p_buff = &Input_Cmd_Buffer[0];
                        p_tf_cmd_hdr = (void*)&Input_Cmd_Buffer[0];
                        break;
                    }
                }
            }
        }


        rtn_arg = TF_Test_Cmd_Handler[tf_cmd_hdr.id](p_tf_cmd_hdr);

        if(rtn_arg == TF_EXIT_FROM_TF_LOOP && tf_cmd_hdr.id == TF_CMD_SET_INTERCEPT)
        {
            break;
        }
    }

    return 0;
}


static void fill_rsp_buffer(uint32_t *buf_size, const void *buffer,
    uint32_t rsp_size)
{
    uint32_t start_idx = TF_MAX_RSP_SIZE - *buf_size;
    const char* src_buff = buffer;
    char* p_rsp = &Output_Rsp_Buffer[start_idx];

    while(rsp_size)
    {
        if (rsp_size > *buf_size)
        {
            memcpy(p_rsp, src_buff, *buf_size);
            SERIAL_write(SP_UART1, &Output_Rsp_Buffer[0], TF_MAX_RSP_SIZE);

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
    uint32_t checksum = 0;
    uint32_t buf_size = TF_MAX_RSP_SIZE;
    uint32_t bytes_to_transmit=0;
    char* p_rsp = &Output_Rsp_Buffer[0];

    printf("Response being tyransmitted\r\n");

    Output_Rsp_Buffer[0] = TF_CMD_START;
    buf_size--;

    fill_rsp_buffer(&buf_size, rsp, rsp_size);

    if (additional_rsp_size)
    {
        fill_rsp_buffer(&buf_size, additional_rsp, additional_rsp_size);
    }

    bytes_to_transmit = (TF_MAX_RSP_SIZE - buf_size);

    if(bytes_to_transmit <= TF_MAX_RSP_SIZE)
    {
        for(uint32_t i = 0; i < bytes_to_transmit; i++)
        {
            checksum += *p_rsp;
            p_rsp++;
        }

        /* Add checksum to response */
        fill_rsp_buffer(&buf_size, &checksum, 4);
        bytes_to_transmit += 4;

#if 1
        p_rsp = &Output_Rsp_Buffer[0];
        for(uint32_t i =0; i < bytes_to_transmit; i++)
        {
            printf("0x");
            printf("%02X", *p_rsp);
            printf("%s", ",");

            p_rsp++;
        }
        printf("\r\n");
#endif

        SERIAL_write(SP_UART1, &Output_Rsp_Buffer[0], (int)bytes_to_transmit);
    }
    else
    {
        printf("ERROR: TF protocol error: Host trying to move data greater than supported 4K byte size \r\n");
        while(1);
    }

    return 0;
}

int8_t TF_Send_Response(void* rsp, uint32_t rsp_size)
{
    return TF_Send_Response_With_Payload(rsp, rsp_size, NULL, 0);
}

