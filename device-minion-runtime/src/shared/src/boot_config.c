#include "boot_config.h"

#include <stddef.h>
#include <stdbool.h>

#include "hal_device.h"

static uint32_t test_mem[2];
#define TEST_MEM_SIZE sizeof(test_mem) / sizeof(test_mem[0]);

typedef enum {
    CONFIG_MEM_SPACE_RESERVED0 = 0,
    CONFIG_MEM_SPACE_R_SP_CRU,
    CONFIG_MEM_SPACE_R_PCIE_ESR,
    CONFIG_MEM_SPACE_R_PCIE_APB_SUBSYS,
    CONFIG_MEM_SPACE_R_PCIE0_DBI_SLV,
    CONFIG_MEM_SPACE_TEST,
    CONFIG_MEM_SPACE_EOL
} CONFIG_MEM_SPACE_t;

//Keep this in sync with config_rom_assembler.py
static const uint64_t MEM_SPACE_LUT[16] = {
    [CONFIG_MEM_SPACE_RESERVED0]         = 0ULL,
    [CONFIG_MEM_SPACE_R_SP_CRU]          = R_SP_CRU_BASEADDR,
    [CONFIG_MEM_SPACE_R_PCIE_ESR]        = R_PCIE_ESR_BASEADDR,
    [CONFIG_MEM_SPACE_R_PCIE_APB_SUBSYS] = R_PCIE_APB_SUBSYS_BASEADDR,
    [CONFIG_MEM_SPACE_R_PCIE0_DBI_SLV]   = R_PCIE0_DBI_SLV_BASEADDR,
    [CONFIG_MEM_SPACE_TEST]              = (uint64_t)&test_mem[0],
    [6] = 0ULL, //others reserved for future use
    [7] = 0ULL,
    [8] = 0ULL,
    [9] = 0ULL,
    [10] = 0ULL,
    [11] = 0ULL,
    [12] = 0ULL,
    [13] = 0ULL,
    [14] = 0ULL,
    [15] = 0ULL
};

#define CONFIG_OP_CODE_TERMINATOR0    0
#define CONFIG_OP_CODE_WRITE          1
#define CONFIG_OP_CODE_READ_MOD_WRITE 2
#define CONFIG_OP_CODE_POLL           3
#define CONFIG_OP_CODE_WAIT           4
#define CONFIG_OP_CODE_TERMINATOR1    0xF

static const uint64_t MAX_WAIT_LOOPS = 100000;

static volatile uint32_t* getRegPtr(const CONFIG_COMMAND_t * cmd, const uint64_t * addr_white_list, uint32_t addr_white_list_size);

static volatile uint32_t* getRegPtr(const CONFIG_COMMAND_t * cmd, const uint64_t * addr_white_list, uint32_t addr_white_list_size)
{
    if (cmd->dw0.B.memSpace <= CONFIG_MEM_SPACE_RESERVED0 || cmd->dw0.B.memSpace >= CONFIG_MEM_SPACE_EOL) {
        return NULL;
    }

    uint64_t base_addr = MEM_SPACE_LUT[cmd->dw0.B.memSpace];
    if (base_addr == 0) return NULL;

    uint64_t target_addr = base_addr + cmd->dw0.B.offset;

    // Check for int overflow
    if (target_addr < base_addr) return NULL;

    if (addr_white_list) {
        bool white_listed = false;

        for (size_t i = 0; i < addr_white_list_size; ++i) {
            if (addr_white_list[i] == target_addr) {
                white_listed = true;
                break; 
            }
        }

        if (!white_listed) return NULL;
    }

    return (volatile uint32_t*)target_addr;
}

int boot_config_execute(const CONFIG_COMMAND_t * commands, uint32_t commands_count, const uint64_t * addr_white_list, uint32_t addr_white_list_size)
{
    if (commands == NULL || commands_count == 0) return 0;

    for (size_t i = 0; i < commands_count; ++i) {
        const CONFIG_COMMAND_t * cmd = &commands[i];
       
        switch (cmd->dw0.B.opCode) {
            case CONFIG_OP_CODE_TERMINATOR0:
            case CONFIG_OP_CODE_TERMINATOR1:
                return 0;

            case CONFIG_OP_CODE_WRITE:
            {
                volatile uint32_t* regPtr = getRegPtr(cmd, addr_white_list, addr_white_list_size);
                if (!regPtr) return -1;

                *regPtr = cmd->dw1.B.value & cmd->dw2.B.mask;
            }   
            break;

            case CONFIG_OP_CODE_READ_MOD_WRITE:
            {
                volatile uint32_t* regPtr = getRegPtr(cmd, addr_white_list, addr_white_list_size);
                if (!regPtr) return -1;

                uint32_t readVal = *regPtr;
                *regPtr = (readVal & cmd->dw2.B.mask) | (cmd->dw1.B.value & cmd->dw2.B.mask);
            }   
            break;

            case CONFIG_OP_CODE_POLL:
            {
                volatile uint32_t* regPtr = getRegPtr(cmd, addr_white_list, addr_white_list_size);
                if (!regPtr) return -1;

                uint64_t loopCounter = MAX_WAIT_LOOPS;

                while (((*regPtr & cmd->dw2.B.mask) != cmd->dw1.B.value) && loopCounter) {
                    --loopCounter;
                }

                //Timeouts should never happen. Fail the sequence on a timeout.
                if (!loopCounter) return -1;
            }
            break;

            case CONFIG_OP_CODE_WAIT:
            {
                if (cmd->dw1.B.value > MAX_WAIT_LOOPS) return -1;

                uint64_t loopCounter = cmd->dw1.B.value;

                while (loopCounter) --loopCounter;
            }
            break;

            //Unsupported OpCode. Assume sequence tainted.
            default: return -1;
        }
    }

    //Executed commands_count number of instructions without seeing a terminator
    return -1;
}

#ifdef BOOT_CONFIG_SELF_TEST

static const uint32_t test_seq_invalid_mem_space_0[] = {
    //mem write an invalid mem space
    0x1 << 28 | CONFIG_MEM_SPACE_RESERVED0 << 24 | 0, //offset = 0
    0x00000000,                                       //writeVal
    0x00000000,                                       //mask
    //terminator
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF
};

static const uint32_t test_seq_invalid_mem_space_1[] = {
    //mem write an invalid mem space
    0x1 << 28 | CONFIG_MEM_SPACE_EOL << 24 | 0,       //offset = 0
    0x00000000,                                       //writeVal
    0x00000000,                                       //mask
    //terminator
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF
};

static const uint32_t test_seq_invalid_op_code[] = {
    //invalid op code
    0x5 << 28,
    0x00000000,                                       //writeVal
    0x00000000,                                       //mask
    //terminator
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF
};

static const uint32_t test_seq_no_terminator[] = {
    //write test_mem[0]
    0x1 << 28 | CONFIG_MEM_SPACE_TEST << 24 | 0, //offset = 0
    0xFFFFFFFF,                                  //writeVal
    0xFFFFFFFF,                                  //mask
    //no terminator
};

static const uint32_t test_seq_infinite_poll[] = {
    //write test_mem[0]
    0x1 << 28 | CONFIG_MEM_SPACE_TEST << 24 | 0, //offset = 0
    0xFFFFFFFF,                                  //writeVal
    0xFFFFFFFF,                                  //mask
    //poll test_mem[0]
    //poll bits at 0xAAAAAAAA are 0
    //should never be satisfied, time out
    0x3 << 28 | CONFIG_MEM_SPACE_TEST << 24 | 0, //offset = 0
    0x00000000,                                  //desiredVal
    0xAAAAAAAA,                                  //mask
    //terminator
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF
};

static const uint32_t test_seq_wait_too_big[] = {
    //wait
    0x4 << 28,
    MAX_WAIT_LOOPS + 1,                          //tick count
    0,                                           //reserved
    //terminator
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF
};

static const uint32_t test_seq_valid_mem0[] = {
    //write test_mem[0]
    //test_mem[0] = 0x11000000
    0x1 << 28 | CONFIG_MEM_SPACE_TEST << 24 | 0, //offset = 0
    0xFF000000,                                  //writeVal
    0x11000000,                                  //mask
    //rmw write test_mem[0]
    //Should write bit 0, clear bit 28, leave bit 24 untouched
    //test_mem[0] = 0x01000001
    0x2 << 28 | CONFIG_MEM_SPACE_TEST << 24 | 0, //offset = 0
    0x00000001,                                  //writeVal
    0x01000001,                                  //mask
    //poll test_mem[0]
    //check bit 24 == 1, ignore others
    //should immediately return true
    0x3 << 28 | CONFIG_MEM_SPACE_TEST << 24 | 0, //offset = 0
    0x01000000,                                  //desiredVal
    0x01000000,                                  //mask
    //poll test_mem[0]
    //check bit 28 == 0, ignore others
    //should immediately return true
    0x3 << 28 | CONFIG_MEM_SPACE_TEST << 24 | 0, //offset = 0
    0x00000000,                                  //desiredVal
    0x10000000,                                  //mask
    //wait 1 tick
    0x4 << 28,
    1,                                           //tick count
    0,                                           //reserved
    //terminator
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF
};

//Similar to seq 0 but writes test_mem[1] (not on white list) instead
static const uint32_t test_seq_valid_mem1[] = {
    //write test_mem[1]
    //test_mem[0] = 0x00110000
    0x1 << 28 | CONFIG_MEM_SPACE_TEST << 24 | 4, //offset = 4
    0x00FF0000,                                  //writeVal
    0x00110000,                                  //mask
    //rmw write test_mem[1]
    //test_mem[0] = 0x00010001
    0x2 << 28 | CONFIG_MEM_SPACE_TEST << 24 | 4, //offset = 4
    0x00000001,                                  //writeVal
    0x00010001,                                  //mask
    //poll test_mem[1]
    //check bit == 1, ignore others
    //should immediately return true
    0x3 << 28 | CONFIG_MEM_SPACE_TEST << 24 | 4, //offset = 4
    0x00010000,                                  //desiredVal
    0x00010000,                                  //mask
    //poll test_mem[1]
    //check bit == 0, ignore others
    //should immediately return true
    0x3 << 28 | CONFIG_MEM_SPACE_TEST << 24 | 4, //offset = 4
    0x00000000,                                  //desiredVal
    0x00100000,                                  //mask
    //wait 1 tick
    0x4 << 28,
    1,                                           //tick count
    0,                                           //reserved
    //terminator
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF
};

static const uint64_t test_white_list[] = {
    (uint64_t)&test_mem[0]
};

int boot_config_self_test(void)
{
    //Try accessing invalid memory spaces
    int test_result = boot_config_execute((const CONFIG_COMMAND_t*)test_seq_invalid_mem_space_0, 2, NULL, 0);
    if (test_result != -1) {
        return -2;
    }

    test_result = boot_config_execute((const CONFIG_COMMAND_t*)test_seq_invalid_mem_space_1, 2, NULL, 0);
    if (test_result != -1) {
        return -3;
    }

    //Test invalid opcode returns -1
    test_result = boot_config_execute((const CONFIG_COMMAND_t*)test_seq_invalid_op_code, 2, NULL, 0);
    if (test_result != -1) {
        return -4;
    }

    //Test no terminator
    test_result = boot_config_execute((const CONFIG_COMMAND_t*)test_seq_no_terminator, 1, NULL, 0);
    if (test_result != -1) {
        return -5;
    }

    //poll on value that never happens - make sure it times out
    test_result = boot_config_execute((const CONFIG_COMMAND_t*)test_seq_infinite_poll, 3, test_white_list, 1);
    if (test_result != -1) {
        return -6;
    }

    //wait too long
    test_result = boot_config_execute((const CONFIG_COMMAND_t*)test_seq_wait_too_big, 2, test_white_list, 1);
    if (test_result != -1) {
        return -7;
    }

    //This test exercises all of the valid op codes
    test_result = boot_config_execute((const CONFIG_COMMAND_t*)test_seq_valid_mem0, 6, test_white_list, 1);
    if (test_result != 0) {
        return -8;
    }
    else if (test_mem[0] != 0x01000001) {
        return -9;
    }

    //This sequence uses a memory address not on the white list. It should fail.
    test_result = boot_config_execute((const CONFIG_COMMAND_t*)test_seq_valid_mem1, 6, test_white_list, 1);
    if (test_result != -1) {
        return -10;
    }

    //Try again, but this time allow non-white-listed addresses
    test_result = boot_config_execute((const CONFIG_COMMAND_t*)test_seq_valid_mem1, 6, NULL, 0);
    if (test_result != 0) {
        return -11;
    }
    else if (test_mem[1] != 0x00010001) {
        return -12;
    }

    //All tests passed
    return 0;
}

#endif