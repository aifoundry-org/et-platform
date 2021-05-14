#ifndef TF_PROTOCOL_H
#define TF_PROTOCOL_H

#include "common_defs.h"

/* THIS FILE IS MANUALLY WRITTEN FOR NOW
THIS FILE IS GOING TO BE AUTO GENERATED
FROM THE JSON TEST SPECIFICATION USED BY 
TF */

/* command/payload format */ 
/* -----$<command header 64 bytes><payload..>#<checksum>% -----*/

/* Protocol delimiters and size parameters */
#define TF_CMD_START                    0x24    /* ASCII-$ */
#define TF_CMD_END                      0x23    /* ASCII-# */
#define TF_CHECKSUM_END                 0x25    /* ASCII-% */
#define TF_CHECKSUM_SIZE                4       /* Bytes */
#define TF_MAX_CMD_SIZE                 4096    /* Total size of test command + payload */
#define TF_MAX_RSP_SIZE                 4096    /* Total size of test command + payload */

/* Command and response flags */
#define TF_CMD_ONLY                     (0x01 << 0)
#define TF_CMD_WITH_PAYLOAD             (0x01 << 1)
#define TF_CMD_WITH_PAYLOAD_NEXT        (0x01 << 2)
#define TF_RSP_ONLY                     (0x01 << 3)
#define TF_RSP_WITH_PAYLOAD             (0x01 << 4)
#define TF_RSP_WITH_PAYLOAD_NEXT        (0x01 << 5)

typedef enum {
    TF_CMD_OFFSET=0,
    TF_CMD_SP_FW_VERSION=1,
    TF_CMD_MM_FW_VERSION=2,
    TF_CMD_ECHO_TO_SP=3,
    TF_CMD_ECHO_TO_MM=4,
    TF_CMD_MOVE_DATA_TO_DEVICE=5,
    TF_CMD_MOVE_DATA_To_HOST=6,
    TF_CMD_SPIO_RAM_READ_WORD=7,
    TF_CMD_SPIO_RAM_WRITE_WORD=8,
    TF_CMD_SPIO_SPI_FLASH_INIT=9,
    TF_CMD_SPIO_SPI_FLASH_READ_WORD=10,
    TF_CMD_SPIO_SPI_FLASH_WRITE_WORD=11,
    TF_CMD_SPIO_OTP_INIT=12,
    TF_CMD_SPIO_OTP_READ_WORD=13,
    TF_CMD_SPIO_OTP_WRITE_WORD=14,
    TF_CMD_SPIO_VAULT_INIT=15,
    TF_CMD_SPIO_VAULT_COMMAND_ISSUE=16,
    TF_CMD_SPIO_I2C_INIT=17,
    TF_CMD_SPIO_I2C_PMIC_READ=18,
    TF_CMD_SPIO_I2C_PMIC_WRITE=19,
    TF_CMD_PU_UART_INIT=20,
    TF_CMD_PU_SRAM_READ_WORD=21,
    TF_CMD_PU_SRAM_WRITE_WORD=22,
    TF_CMD_PCIE_PSHIRE_INIT=23,
    TF_CMD_PCIE_PSHIRE_VOLTAGE_UPDATE=24,
    TF_CMD_PCIE_PSHIRE_PLL_PROGRAM=25,
    TF_CMD_PCIE_PSHIRE_NOC_UPDATE_ROUTING_TABLE=26,
    TF_CMD_PCIE_CNTR_INIT_BAR_MAPPING=27,
    TF_CMD_PCIE_CNTR_INIT_INTERRUPTS=28,
    TF_CMD_PCIE_CNTR_INIT_LINK_PARAMS=29,
    TF_CMD_PCIE_CNTR_ATU_INIT=30,
    TF_CMD_PCIE_PHY_INIT=31,
    TF_CMD_PCIE_PHY_FW_INIT=32,
    TF_CMD_NOC_VOLTAGE_UPDATE=33,
    TF_CMD_NOC_PLL_PROGRAM=34,
    TF_CMD_NOC_ROUTING_TABLE_UPDATE=35,
    TF_CMD_MEM_MEMSHIRE_PLL_PROGRAM=36,
    TF_CMD_MEM_MEMSHIRE_VOLTAGE_UPDATE=37,
    TF_CMD_MEM_MEMSHIRE_INIT=38,
    TF_CMD_MEM_DDR_CNTR_INIT=39,
    TF_CMD_MEM_DDR_READ=40,
    TF_CMD_MEM_DDR_WRITE=42,
    TF_CMD_MINION_STEP_CLOCK_PLL_PROGRAM=43,
    TF_CMD_MINION_VOLTAGE_UPDATE=44,
    TF_CMD_MINION_SHIRE_ENABLE=45,
    TF_CMD_MINION_SHIRE_BOOT=46,
    TF_CMD_MINION_KERNEL_LAUNCH=47,
    TF_CMD_MINION_ESR_READ=48,
    TF_CMD_MINION_ESR_WRITE=49,
    TF_CMD_MM_TESTS_OFFSET=50
}tf_cmd_t;

typedef enum {
    TF_RSP_OFFSET=256,
    TF_RSP_SP_FW_VERSION=257,
    TF_RSP_MM_FW_VERSION=258,
    TF_RSP_ECHO_TO_SP=259,
    TF_RSP_ECHO_TO_MM=260,
    TF_RSP_MOVE_DATA_TO_DEVICE=261,
    TF_RSP_MOVE_DATA_To_HOST=262,
    TF_RSP_SPIO_RAM_READ_WORD=263,
    TF_RSP_SPIO_RAM_WRITE_WORD=264,
    TF_RSP_SPIO_SPI_FLASH_INIT=265,
    TF_RSP_SPIO_SPI_FLASH_READ_WORD=266,
    TF_RSP_SPIO_SPI_FLASH_WRITE_WORD=267,
    TF_RSP_SPIO_OTP_INIT=268,
    TF_RSP_SPIO_OTP_READ_WORD=269,
    TF_RSP_SPIO_OTP_WRITE_WORD=270,
    TF_RSP_SPIO_VAULT_INIT=271,
    TF_RSP_SPIO_VAULT_COMMAND_ISSUE=272,
    TF_RSP_SPIO_I2C_INIT=273,
    TF_RSP_SPIO_I2C_PMIC_READ=274,
    TF_RSP_SPIO_I2C_PMIC_WRITE=275,
    TF_RSP_PU_UART_INIT=276,
    TF_RSP_PU_SRAM_READ_WORD=277,
    TF_RSP_PU_SRAM_WRITE_WORD=278,
    TF_RSP_PCIE_PSHIRE_INIT=279,
    TF_RSP_PCIE_PSHIRE_VOLTAGE_UPDATE=280,
    TF_RSP_PCIE_PSHIRE_PLL_PROGRAM=281,
    TF_RSP_PCIE_PSHIRE_NOC_UPDATE_ROUTING_TABLE=282,
    TF_RSP_PCIE_CNTR_INIT_BAR_MAPPING=283,
    TF_RSP_PCIE_CNTR_INIT_INTERRUPTS=284,
    TF_RSP_PCIE_CNTR_INIT_LINK_PARAMS=285,
    TF_RSP_PCIE_CNTR_ATU_INIT=286,
    TF_RSP_PCIE_PHY_INIT=287,
    TF_RSP_PCIE_PHY_FW_INIT=288,
    TF_RSP_NOC_VOLTAGE_UPDATE=289,
    TF_RSP_NOC_PLL_PROGRAM=290,
    TF_RSP_NOC_ROUTING_TABLE_UPDATE=291,
    TF_RSP_MEM_MEMSHIRE_PLL_PROGRAM=292,
    TF_RSP_MEM_MEMSHIRE_VOLTAGE_UPDATE=293,
    TF_RSP_MEM_MEMSHIRE_INIT=294,
    TF_RSP_MEM_DDR_CNTR_INIT=295,
    TF_RSP_MEM_DDR_READ=296,
    TF_RSP_MEM_DDR_WRITE=297,
    TF_RSP_MINION_STEP_CLOCK_PLL_PROGRAM=298,
    TF_RSP_MINION_VOLTAGE_UPDATE=299,
    TF_RSP_MINION_SHIRE_ENABLE=300,
    TF_RSP_MINION_SHIRE_BOOT=301,
    TF_RSP_MINION_KERNEL_LAUNCH=302,
    TF_RSP_MINION_ESR_READ=303,
    TF_RSP_MINION_ESR_WRITE=304,
    TF_RSP_MM_TESTS_OFFSET=305,
    TF_RSP_UNSUPPORTED_COMMAND=512
}tf_rsp_t;

#define TF_NUM_COMMANDS (TF_CMD_MM_TESTS_OFFSET - TF_CMD_OFFSET)
#define TF_NUM_CMD_RSPONSES (TF_CMD_MM_TESTS_OFFSET - TF_RSP_OFFSET)

#define TF_GET_PAYLOAD_SIZE(cmd_struct)  (sizeof(cmd_struct)-sizeof(struct header_t))

/* Test Command Header */
struct header_t {
    uint16_t id;
    uint16_t flags;
    uint32_t payload_size;
};
typedef struct header_t tf_cmd_hdr_t;
typedef struct header_t tf_rsp_hdr_t;

/* Test Commands and responses */
struct tf_echo_cmd_t {
    tf_cmd_hdr_t  cmd_hdr;
    uint32_t      cmd_payload;
};
struct tf_echo_rsp_t {
    tf_rsp_hdr_t  rsp_hdr;
    uint32_t      rsp_payload;
};

struct tf_get_spfw_ver_cmd_t {
    tf_cmd_hdr_t  cmd_hdr;
};
struct tf_get_spfw_ver_rsp_t {
    tf_rsp_hdr_t  rsp_hdr;    
    uint32_t      major;
    uint32_t      minor;
    uint32_t      version;
};

struct tf_get_mmfw_ver_cmd_t {
    tf_cmd_hdr_t  cmd_hdr;
};
struct tf_get_mmfw_ver_rsp_t {
    tf_rsp_hdr_t  rsp_hdr;    
    uint32_t      major;
    uint32_t      minor;
    uint32_t      version;
};

#endif
