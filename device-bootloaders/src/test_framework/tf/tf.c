#include <stdio.h>
#include "tf.h"
#include "etsoc/drivers/serial/serial.h"
#include "hwinc/hal_device.h"
#include <string.h>

//#define TF_DEBUG

#define TF_UART PU_UART1

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
#if TF_CONFIG_SW_CHECKSUM_ENABLE==1
    uint32_t        computed_checksum = 0;
    uint32_t        rcvd_checksum = 0;
#endif
    struct header_t tf_cmd_hdr = {0};
    bool            tf_prot_start_found=false;
    bool            tf_cmd_size_available=false;
    bool            tf_receive_cmd_data=true;

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
        bytes_received = 0;
        cmd_bytes_received = 0;
#if TF_CONFIG_SW_CHECKSUM_ENABLE==1
        computed_checksum = 0;
#endif
        tf_prot_start_found = false;
        tf_cmd_size_available = false;
        tf_receive_cmd_data = true;

        /* Clear the command input buffer */
        memset(&Input_Cmd_Buffer[0], 0, TF_MAX_CMD_SIZE);

#ifdef  TF_DEBUG
        printf("Getting into TF RX loop \r\n");
#endif
        while(tf_receive_cmd_data)
        {
            SERIAL_getchar(TF_UART, &c);

            /* Look for the command start delimeter */
            if((!tf_prot_start_found) && (c == TF_CMD_START))
            {
#ifdef  TF_DEBUG
                printf("Command start delimeter ($) received\r\n");
#endif
#if TF_CONFIG_SW_CHECKSUM_ENABLE==1
                computed_checksum += (uint8_t)c;
#endif
                tf_prot_start_found = true;
            }
            else
            {
                *p_buff = c;
#if TF_CONFIG_SW_CHECKSUM_ENABLE==1
                computed_checksum += *p_buff;
#endif

#ifdef  TF_DEBUG
                printf("0x");
                printf("%02X", *p_buff);
                printf("%s", ",");
#endif
                p_buff++;
                bytes_received++;

                /* Look for the command header bytes (exclusing the command start delimeter) */
                if(bytes_received == TF_CMD_HDR_BYTES)
                {
                    memcpy(&tf_cmd_hdr, &Input_Cmd_Buffer[0], sizeof(tf_cmd_hdr));
#ifdef  TF_DEBUG
                    printf("\r\n");
                    printf("tf_cmd_hdr.id = %d\r\n", tf_cmd_hdr.id);
                    printf("tf_cmd_hdr.flags = %d\r\n", tf_cmd_hdr.flags);
                    printf("tf_cmd_hdr.payload_size = %d\r\n", tf_cmd_hdr.payload_size);
#endif

                    if(tf_cmd_hdr.payload_size == 0)
                    {
                        tf_receive_cmd_data = false;
                    }
                    else
                    {
                        tf_cmd_size_available = true;
                    }
                }
                else if(tf_cmd_size_available)
                {
                    /* Receiving the command payload */
                    cmd_bytes_received++;

#if TF_CONFIG_SW_CHECKSUM_ENABLE==1
                    if(cmd_bytes_received == (tf_cmd_hdr.payload_size + TF_CHECKSUM_SIZE))
#else
                    if(cmd_bytes_received == tf_cmd_hdr.payload_size)
#endif
                    {
#if TF_CONFIG_SW_CHECKSUM_ENABLE==1
                        /* Verify checksum */
                        const uint8_t* p = (uint8_t*)(&Input_Cmd_Buffer[0] + TF_CMD_HDR_BYTES + tf_cmd_hdr.payload_size);
                        rcvd_checksum = (uint32_t)(p[0] + (p[1] << 8) + (p[2] << 16) + (p[3] << 24));

                        /* subtract the received checksum from checksum for full command length (incl checksum) */
                        computed_checksum -= p[0];
                        computed_checksum -= p[1];
                        computed_checksum -= p[2];
                        computed_checksum -= p[3];
#ifdef  TF_DEBUG
                        printf("\r\nCommand fully received, size = %d \r\n", cmd_bytes_received);
                        printf("bytes_received:%d\r\n", bytes_received);
                        printf("rcvd_checksum:%d \r\n",rcvd_checksum);
                        printf("computed_checksum:%d \r\n",computed_checksum);
#endif
                        if(computed_checksum != rcvd_checksum)
                        {
                            printf("Received command, command checksum failed\r\n");
                        }
#endif
                        /* Command processed */
                        tf_receive_cmd_data = false;
                    }
                }
            }
        }

        /* Invoke the command handler based on ID */
        rtn_arg = TF_Test_Cmd_Handler[tf_cmd_hdr.id]((struct header_t *)&Input_Cmd_Buffer[0]);

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
            SERIAL_write(TF_UART, &Output_Rsp_Buffer[0], TF_MAX_RSP_SIZE);

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
#if TF_CONFIG_SW_CHECKSUM_ENABLE==1
    uint32_t checksum = 0;
#endif
    uint32_t buf_size = TF_MAX_RSP_SIZE;
    uint32_t bytes_to_transmit=0;
    char* p_rsp = &Output_Rsp_Buffer[0];

#ifdef  TF_DEBUG
    printf("Response being transmitted\r\n");
#endif
    Output_Rsp_Buffer[0] = TF_CMD_START;
    buf_size--;

#ifdef  TF_DEBUG
    printf("Response:fill_buffer:rsp_size:%d\r\n", rsp_size);
#endif
    fill_rsp_buffer(&buf_size, rsp, rsp_size);

    if (additional_rsp_size)
    {
#ifdef  TF_DEBUG
        printf("AdditionalResponse:fill_buffer:additional_rsp_size:%d\r\n", additional_rsp_size);
#endif
        fill_rsp_buffer(&buf_size, additional_rsp, additional_rsp_size);
    }

    bytes_to_transmit = (TF_MAX_RSP_SIZE - buf_size);

    if(bytes_to_transmit <= TF_MAX_RSP_SIZE)
    {
#if TF_CONFIG_SW_CHECKSUM_ENABLE==1
        for(uint32_t i = 0; i < bytes_to_transmit; i++)
        {
            checksum += *p_rsp;
            p_rsp++;
        }
        /* Add checksum to response */
        fill_rsp_buffer(&buf_size, &checksum, 4);
        bytes_to_transmit += 4;

#ifdef  TF_DEBUG
        printf("Response Checksum: %d\r\n", checksum);
        p_rsp = &Output_Rsp_Buffer[0];
        for(uint32_t i =0; i < bytes_to_transmit; i++)
        {
            printf("0x");
            printf("%02X", *p_rsp);
            printf("%s", ",");

            p_rsp++;
        }
        printf("\nlength of total response: %d\r\n", bytes_to_transmit);
        printf("\r\n");
#endif

#endif
        p_rsp = &Output_Rsp_Buffer[0];
        SERIAL_write(TF_UART, p_rsp, (int)bytes_to_transmit);
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

