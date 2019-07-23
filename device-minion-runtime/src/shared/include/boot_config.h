#ifndef __BOOT_CONFIG_H__
#define __BOOT_CONFIG_H__

#include <stdint.h>

typedef struct CONFIG_COMMAND_u {
    union {
        struct {
            uint32_t offset : 24;
            uint32_t memSpace : 4;
            uint32_t opCode : 4;
        } B;
        uint32_t R;
    } dw0;
    union {
        struct {
            uint32_t value : 32;
        } B;
        uint32_t R;
    } dw1;
    union {
        struct {
            uint32_t mask : 32;
        } B;
        uint32_t R;
    } dw2;
} CONFIG_COMMAND_t;

/* 
 * Executes @commands. Intended to store a set of register accesses needed for very
 * early power-on configuration that can be executed safely before the firmware is
 * crypto validated and loaded.
 *
 * @commands commands to execute
 * @commands_count number of commands in the list
 * @addr_white_list list of addresses valid to access. Commands issued to other addresses
   will be considered invalid. Pass NULL to bypass white list check.
 * @add_white_list_size number of elements in @commands_count
 * @return 0 on success
 */
int boot_config_execute(
    const CONFIG_COMMAND_t * commands,
    uint32_t commands_count,
    const uint64_t * addr_white_list,
    uint32_t addr_white_list_size);

#ifdef BOOT_CONFIG_SELF_TEST
/* 
 * Tests boot_config_execute
 * @return 0 on test pass, non-zero on test fail
 */
int boot_config_self_test(void);
#endif

#endif
