#include <stdio.h>
#include "tf.h"
#include "tf_protocol.h"
#include "serial.h"
#include "etsoc_hal/inc/hal_device.h"

static char Input_Cmd_Buffer[TF_MAX_CMD_SIZE];
static char Output_Rsp_Buffer[TF_MAX_RSP_SIZE]; 

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

int8_t TF_Wait_And_Process_TF_Cmds(void)
{
    char c;
    char *p_buff = &Input_Cmd_Buffer[0];
    void *p_hdr=0;
    struct header_t cmd_hdr;
    
    for(;;)
    {
        do 
        {
            SERIAL_getchar(UART1, &c);
            /* printf("%c", c); */
            
            if(c == TF_CMD_START)
            {
                p_hdr = (void*) (p_buff+1);
            }
            
            *p_buff = c;
            p_buff++;

        } while (c != TF_CHECKSUM_END);
        
        /* *p_buff = '\0'; */

        byte_copy((char*)&cmd_hdr, (char*)p_hdr, 
            sizeof(struct header_t));

        TF_Test_Cmd_Handler[cmd_hdr.id](p_hdr);

    }
}

int8_t TF_Send_Response(void* rsp, uint32_t rsp_size)
{
    char* p_rsp = &Output_Rsp_Buffer[0];
    uint32_t magic=0xA5A5A5A5;
    uint32_t size=0;

    *p_rsp = TF_CMD_START;
    p_rsp++; size++;
    byte_copy(p_rsp, rsp, rsp_size);
    p_rsp += rsp_size; size += rsp_size;
    *p_rsp = TF_CMD_END;
    p_rsp++;size++;
    byte_copy(p_rsp, (char*)&magic, TF_CHECKSUM_SIZE);
    p_rsp += TF_CHECKSUM_SIZE;size += TF_CHECKSUM_SIZE;
    *p_rsp = TF_CHECKSUM_END;

    SERIAL_write(UART1, &Output_Rsp_Buffer[0], (int)size);

    return 0;
}
