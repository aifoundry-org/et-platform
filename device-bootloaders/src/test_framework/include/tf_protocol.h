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
    /* HW TF commands from 0 - 69 */
    TF_CMD_OFFSET=0,
    TF_CMD_SP_FW_VERSION=1,
    TF_CMD_UNUSED1=2,
    TF_CMD_ECHO_TO_SP=3,
    TF_CMD_UNUSED2=4,
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
    TF_CMD_MEM_SUBSYSTEM_CONFIG=40,
    TF_CMD_MEM_DDR_READ=41,
    TF_CMD_MEM_DDR_WRITE=42,
    TF_CMD_MINION_STEP_CLOCK_PLL_PROGRAM=43,
    TF_CMD_MINION_VOLTAGE_UPDATE=44,
    TF_CMD_MINION_SHIRE_ENABLE=45,
    TF_CMD_MINION_SHIRE_BOOT=46,
    TF_CMD_MINION_KERNEL_LAUNCH=47,
    TF_CMD_MINION_ESR_READ=48,
    TF_CMD_MINION_ESR_WRITE=49,
    TF_CMD_MINION_ESR_RMW=50,
    TF_CMD_SPIO_PLL_PROGRAM=51,
    TF_CMD_PU_PLL_PROGRAM=52,
    TF_CMD_SPIO_IO_READ=53,
    TF_CMD_SPIO_IO_WRITE=54,
    TF_CMD_SPIO_IO_RMW=55,
    TF_CMD_MAXION_RESET_DEASSERT=56,
    TF_CMD_MAXION_CORE_PLL_PROGRAM=57,
    TF_CMD_MAXION_UNCORE_PLL_PROGRAM=58,
    TF_CMD_MAXION_INTERNAL_INIT=59,
    TF_CMD_PMIC_MODULE_TEMPERATURE=60,
    TF_CMD_PMIC_MODULE_POWER=61,
    /* SW TF commands from 97 - 255 */
    TF_CMD_AT_MANUFACTURER_NAME=97,
    TF_CMD_AT_PART_NUMBER=98,
    TF_CMD_AT_SERIAL_NUMBER=99,
    TF_CMD_AT_CHIP_REVISION=100,
    TF_CMD_AT_PCIE_MAX_SPEED=101,
    TF_CMD_AT_MODULE_REVISION=102,
    TF_CMD_AT_FORM_FACTOR=103,
    TF_CMD_AT_MEMORY_DETAILS=104,
    TF_CMD_AT_MEMORY_SIZE_MB=105,
    TF_CMD_AT_MEMORY_TYPE=106,
    TF_CMD_MM_CMD_SHELL=107,
    TF_CMD_MM_CMD_SHELL_DEBUG_PRINT=108,
    TF_CMD_UNSUPPORTED=109
}tf_cmd_t;

typedef enum {
    /* HW TF response from 256 - 351 */
    TF_RSP_OFFSET=256,
    TF_RSP_SP_FW_VERSION=257,
    TF_RSP_UNUSED1=258,
    TF_RSP_ECHO_TO_SP=259,
    TF_RSP_UNUSED2=260,
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
    TF_RSP_MEM_SUBSYSTEM_CONFIG=296,
    TF_RSP_MEM_DDR_READ=297,
    TF_RSP_MEM_DDR_WRITE=298,
    TF_RSP_MINION_STEP_CLOCK_PLL_PROGRAM=299,
    TF_RSP_MINION_VOLTAGE_UPDATE=300,
    TF_RSP_MINION_SHIRE_ENABLE=301,
    TF_RSP_MINION_SHIRE_BOOT=302,
    TF_RSP_MINION_KERNEL_LAUNCH=303,
    TF_RSP_MINION_ESR_READ=304,
    TF_RSP_MINION_ESR_WRITE=305,
    TF_RSP_MINION_ESR_RMW=306,
    TF_RSP_SPIO_PLL_PROGRAM=307,
    TF_RSP_PU_PLL_PROGRAM=308,
    TF_RSP_SPIO_IO_READ=309,
    TF_RSP_SPIO_IO_WRITE=310,
    TF_RSP_SPIO_IO_RMW=311,
    TF_RSP_MAXION_INIT=312,
    TF_RSP_MAXION_CORE_PLL_PROGRAM=313,
    TF_RSP_MAXION_UNCORE_PLL_PROGRAM=314,
    TF_RSP_MAXION_INTERNAL_INIT=315,
    TF_RSP_PMIC_MODULE_TEMPERATURE=316,
    TF_RSP_PMIC_MODULE_POWER=317,
    /* SW TF response from 352 - 511 */
    TF_RSP_AT_MANUFACTURER_NAME=352,
    TF_RSP_AT_PART_NUMBER=353,
    TF_RSP_AT_SERIAL_NUMBER=354,
    TF_RSP_AT_CHIP_REVISION=355,
    TF_RSP_AT_PCIE_MAX_SPEED=356,
    TF_RSP_AT_MODULE_REVISION=357,
    TF_RSP_AT_FORM_FACTOR=358,
    TF_RSP_AT_MEMORY_DETAILS=359,
    TF_RSP_AT_MEMORY_SIZE_MB=360,
    TF_RSP_AT_MEMORY_TYPE=361,
    TF_RSP_MM_CMD_SHELL=362,
    TF_RSP_UNUSED3=363,
    TF_RSP_UNSUPPORTED=364,
}tf_rsp_t;

#define TF_NUM_COMMANDS (TF_CMD_UNSUPPORTED - TF_CMD_OFFSET)
#define TF_NUM_CMD_RSPONSES (TF_RSP_UNSUPPORTED - TF_RSP_OFFSET)

#define TF_GET_PAYLOAD_SIZE(cmd_struct)  (sizeof(cmd_struct)-sizeof(struct header_t))

/* Test Command Header */
struct header_t {
    uint16_t id;
    uint16_t flags;
    uint32_t payload_size;
};
typedef struct header_t tf_cmd_hdr_t;
typedef struct header_t tf_rsp_hdr_t;

/* Generic test command and response prototypes */
struct tf_cmd_rsp_t {
    tf_rsp_hdr_t  rsp_hdr;
    uint32_t      status;
};

struct tf_echo_cmd_t {
    tf_cmd_hdr_t  cmd_hdr;
    uint32_t      cmd_payload;
};
struct tf_echo_rsp_t {
    tf_rsp_hdr_t  rsp_hdr;
    uint32_t      rsp_payload;
};

struct tf_flash_cmd_t {
    tf_cmd_hdr_t  cmd_hdr;
    uint8_t      flash_id;
};
struct tf_pll_cmd_t {
    tf_cmd_hdr_t  cmd_hdr;
    uint8_t      cmd_payload;
};
struct __attribute__((__packed__)) tf_shire_cmd_t {
    tf_cmd_hdr_t  cmd_hdr;
    uint64_t      shire_mask;
    uint8_t      pll4_mode;
};

struct __attribute__((__packed__)) tf_esr_cmd_t {
    tf_cmd_hdr_t  cmd_hdr;
    uint32_t      addr;
    uint64_t      value;
    uint64_t      mask;
};
struct __attribute__((__packed__)) tf_io_cmd_t {
    tf_cmd_hdr_t  cmd_hdr;
    uint8_t       width;
    uint32_t      addr;
    uint64_t      value;
    uint64_t      mask;
};
struct tf_read_rsp_t {
    tf_rsp_hdr_t  rsp_hdr;
    uint64_t      value;
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

struct tf_mm_cmd_shell_cmd_t {
    tf_cmd_hdr_t  cmd_hdr;
    uint32_t      size;
    char          data[];
};

struct tf_mm_rsp_shell_rsp_t {
    tf_rsp_hdr_t  rsp_hdr;
    uint32_t      size;
    char          data[256];
};

struct tf_move_data_to_device_cmd_t {
    tf_cmd_hdr_t  cmd_hdr;
    uint64_t      dst_addr;
    uint32_t      size;
    char          data[TF_MAX_CMD_SIZE];
};
struct tf_move_data_to_device_rsp_t {
    tf_rsp_hdr_t  rsp_hdr;
    uint32_t      bytes_written;
};

struct tf_move_data_to_host_cmd_t {
    tf_cmd_hdr_t  cmd_hdr;
    uint64_t      src_addr;
    uint32_t      size;
};
struct tf_move_data_to_host_rsp_t {
    tf_rsp_hdr_t  rsp_hdr;
    uint32_t      bytes_read;
    char          data[TF_MAX_RSP_SIZE];
};

/* DM test command and response prototypes */
struct tf_asset_tracking_rsp_t {
    tf_rsp_hdr_t rsp_hdr;
    char         data[8];
};

struct tf_pmic_rsp_t {
    tf_rsp_hdr_t rsp_hdr;
    uint8_t      value;
};

#endif
