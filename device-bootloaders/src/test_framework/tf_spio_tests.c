#include <stdio.h>
#include "etsoc/drivers/serial/serial.h"
#include "tf.h"
#include "bl2_sp_pll.h"
#include "etsoc/isa/io.h"
#include "bl2_spi_flash.h"

int8_t SPIO_RAM_Read_Word_Cmd_Handler(void* test_cmd);
int8_t SPIO_RAM_Write_Word_Cmd_Handler(void* test_cmd);
int8_t SPIO_Flash_Init_Cmd_Handler(void* test_cmd);
int8_t SPIO_Flash_Read_Word_Cmd_Handler(void* test_cmd);
int8_t SPIO_Flash_Write_Word_Cmd_Handler(void* test_cmd);
int8_t SPIO_OTP_Init_Cmd_Handler(void* test_cmd);
int8_t SPIO_OTP_Read_Word_Cmd_Handler(void* test_cmd);
int8_t SPIO_OTP_Write_Word_Cmd_Handler(void* test_cmd);
int8_t SPIO_Vault_Init_Cmd_Handler(void* test_cmd);
int8_t SPIO_Vault_Comand_Issue_Cmd_Handler(void* test_cmd);
int8_t SPIO_I2C_Init_Cmd_Handler(void* test_cmd);
int8_t SPIO_I2C_PMIC_Read_Word_Cmd_Handler(void* test_cmd);
int8_t SPIO_I2C_PMIC_Write_Word_Cmd_Handler(void* test_cmd);
int8_t SPIO_PLL_Program_Cmd_Handler(void* test_cmd);
int8_t SPIO_IO_Read_Cmd_Handler(void* test_cmd);
int8_t SPIO_IO_Write_Cmd_Handler(void* test_cmd);
int8_t SPIO_IO_RMW_Cmd_Handler(void* test_cmd);


int8_t SPIO_RAM_Read_Word_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t SPIO_RAM_Write_Word_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t SPIO_Flash_Init_Cmd_Handler(void* test_cmd)
{
    const struct tf_cmd_spio_spi_flash_init_t* cmd = (struct tf_cmd_spio_spi_flash_init_t*)test_cmd;
    struct tf_rsp_spio_spi_flash_init_t cmd_rsp_hdr;

    cmd_rsp_hdr.rsp_hdr.id = TF_RSP_SPIO_SPI_FLASH_INIT;
    cmd_rsp_hdr.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    cmd_rsp_hdr.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(struct tf_rsp_spio_spi_flash_init_t);

    cmd_rsp_hdr.status = (uint32_t)SPI_Flash_Initialize(cmd->flash_id);

    TF_Send_Response(&cmd_rsp_hdr, sizeof(struct tf_rsp_spio_spi_flash_init_t));

    return 0;
}

int8_t SPIO_Flash_Read_Word_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t SPIO_Flash_Write_Word_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t SPIO_OTP_Init_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t SPIO_OTP_Read_Word_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t SPIO_OTP_Write_Word_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t SPIO_Vault_Init_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t SPIO_Vault_Comand_Issue_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_CMD_OFFSET;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t SPIO_I2C_Init_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t SPIO_I2C_PMIC_Read_Word_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t SPIO_I2C_PMIC_Write_Word_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}
int8_t SPIO_PLL_Program_Cmd_Handler(void* test_cmd)
{
    const struct tf_cmd_spio_pll_program_t* cmd = (struct tf_cmd_spio_pll_program_t*)test_cmd;
    struct tf_rsp_spio_pll_program_t cmd_rsp_hdr;

    cmd_rsp_hdr.rsp_hdr.id = TF_RSP_SPIO_PLL_PROGRAM;
    cmd_rsp_hdr.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    cmd_rsp_hdr.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(struct tf_rsp_spio_pll_program_t);
    printf("\n** SPIOPLL mode = %d **\r\n", cmd->cmd_payload);
    cmd_rsp_hdr.status = (uint32_t)configure_sp_pll_0(cmd->cmd_payload);
    printf("\n** SPIOPLL done **\r\n");
    TF_Send_Response(&cmd_rsp_hdr, sizeof(struct tf_rsp_spio_pll_program_t));

    return 0;
}

int8_t SPIO_IO_Read_Cmd_Handler(void* test_cmd)
{
    const struct tf_cmd_spio_io_read_t* cmd = (struct tf_cmd_spio_io_read_t*)test_cmd;
    struct tf_rsp_spio_io_read_t rsp;

    rsp.rsp_hdr.id = TF_RSP_SPIO_IO_READ;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(struct tf_rsp_spio_io_read_t);
    printf("\n** IO RD width %x adr %x **\r\n", cmd->width, cmd->addr);
    if (cmd->width == 32) {
	rsp.value = ioread32(cmd->addr);
    } else {
	rsp.value = ioread64(cmd->addr);
    }
    printf("\n** IO RD done **\r\n");
    TF_Send_Response(&rsp, sizeof(struct tf_rsp_spio_io_read_t));

    return 0;
}

int8_t SPIO_IO_Write_Cmd_Handler(void* test_cmd)
{
    const struct tf_cmd_spio_io_write_t* cmd = (struct tf_cmd_spio_io_write_t*)test_cmd;
    struct tf_rsp_spio_io_write_t rsp;

    rsp.rsp_hdr.id = TF_RSP_SPIO_IO_WRITE;
    rsp.rsp_hdr.flags = TF_RSP_ONLY;
    rsp.rsp_hdr.payload_size =  0;
    printf("\n** IO WR width %x adr %x val %lx **\r\n", cmd->width, cmd->addr, cmd->value);
    if (cmd->width == 32) {
	iowrite32(cmd->addr, (uint32_t)cmd->value);
    } else {
	iowrite64(cmd->addr, cmd->value);
    }
    printf("\n** IO WR done **\r\n");
    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}


int8_t SPIO_IO_RMW_Cmd_Handler(void* test_cmd)
{
    uint64_t regVal;
    const struct tf_cmd_spio_io_rmw_t* cmd = (struct tf_cmd_spio_io_rmw_t*)test_cmd;
    struct tf_rsp_spio_io_rmw_t rsp;

    rsp.rsp_hdr.id = TF_RSP_SPIO_IO_RMW;
    rsp.rsp_hdr.flags = TF_RSP_ONLY;
    rsp.rsp_hdr.payload_size =  0;
    printf("\n** IO RMW width %x adr %x val %lx mask %lx **\r\n", cmd->width, cmd->addr, cmd->value, cmd->mask);
    if (cmd->width == 32) {
	regVal = ioread32(cmd->addr);
	regVal = (regVal & (~ cmd->mask)) | cmd->value;
	iowrite32(cmd->addr, (uint32_t)regVal);
    } else {
	regVal = ioread64(cmd->addr);
	regVal = (regVal & (~cmd->mask)) | cmd->value;
	iowrite64(cmd->addr, regVal);
    }
    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}
