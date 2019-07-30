#ifndef __BOOT_CONFIG_H__
#define __BOOT_CONFIG_H__

#include <stdint.h>

typedef enum {
    BOOT_CONFIG_MEM_SPACE_RESERVED0 = 0,
    BOOT_CONFIG_MEM_SPACE_R_SP_CRU,
    BOOT_CONFIG_MEM_SPACE_R_PCIE_ESR,
    BOOT_CONFIG_MEM_SPACE_R_PCIE_APB_SUBSYS,
    BOOT_CONFIG_MEM_SPACE_R_PCIE0_DBI_SLV,
    BOOT_CONFIG_MEM_SPACE_TEST,
    BOOT_CONFIG_MEM_SPACE_EOL
} BOOT_CONFIG_MEM_SPACE_t;

typedef struct CONFIG_COMMAND_u {
    union {
        struct {
            uint32_t reserved : 2; //reserved for OTP flags when address stored there
            uint32_t offset_24_2 : 22;
            uint32_t memSpace : 4;
            uint32_t opCode : 4;
        } __attribute__ ((__packed__)) B;
        uint32_t R;
    } dw0;
    union {
        struct {
            uint32_t value : 32;
        } __attribute__ ((__packed__)) B;
        uint32_t R;
    } dw1;
    union {
        struct {
            uint32_t mask : 32;
        } __attribute__ ((__packed__)) B;
        uint32_t R;
    } dw2;
} __attribute__ ((__packed__)) CONFIG_COMMAND_t;

/*
 * White list for CONFIG_COMMANT_t commands. If used, all of the addresses passed to
 * boot_config_execute must appear on the white list.
 * @addr address to allow
 * @mask bits of the address to allow modifying. Other bits in CONFIG_COMMAND_t will
 * be ignored.
 */
typedef struct ADDR_WHITE_LIST_s {
    union {
        struct {
            uint32_t reserved0 : 2; //reserved for OTP flags when address stored there
            uint32_t offset_24_2 : 22;
            uint32_t memSpace : 4;
            uint32_t reserved1 : 4;
        } __attribute__ ((__packed__)) B;
        uint32_t R;
    } addr;
    uint32_t mask;
} __attribute__ ((__packed__)) ADDR_WHITE_LIST_t;

/* Returns the base address for the namespace or NULL on error */
volatile uint32_t* get_namespace_address(BOOT_CONFIG_MEM_SPACE_t memSpace);

/* 
 * Executes @commands. Intended to store a set of register accesses needed for very
 * early power-on configuration that can be executed safely before the firmware is
 * crypto validated and loaded.
 *
 * @commands commands to execute
 * @commands_count number of commands in the list
 * @addr_white_list list of addresses valid to access. Commands issued to other addresses
   will be considered invalid. Pass NULL to bypass white list check.
 * @addr_white_list_count number of elements in @addr_white_list
 * @return 0 on success
 */
int boot_config_execute(
    const CONFIG_COMMAND_t * commands,
    uint32_t commands_count,
    const ADDR_WHITE_LIST_t * addr_white_list,
    uint32_t addr_white_list_count);

#ifdef BOOT_CONFIG_SELF_TEST
/* 
 * Tests boot_config_execute
 * @return 0 on test pass, non-zero on test fail
 */
int boot_config_self_test(void);
#endif

#endif
