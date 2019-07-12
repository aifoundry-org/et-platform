/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "serial.h"
#include "printx.h"
#include "crc32.h"

#include "vaultip_hw.h"
#include "vaultip_sw.h"
#include "vaultip_sw_asset.h"
#include "vaultip_static_assets.h"
#include "system_registers.h"
#include "bl1_flash_fs.h"
#include "cache_flush_ops.h"

#include "bl1_vaultip_controller.h"

#include "hal_device.h"

#pragma GCC diagnostic ignored "-Wswitch-enum"

//#define VAULTIP_CRYPTO_OFFICER_IDENTITY 0x4F435445
#define VAULTIP_CRYPTO_OFFICER_IDENTITY 0

//#define VERBOSE_FW_LOAD
//#define TEST_RAM_FIRMWARE
//#define LOAD_FIRMWARE_FROM_SRAM

#ifdef LOAD_FIRMWARE_FROM_SRAM

#define VIP_FW_RAM_ADDR                      0x40410000
#define VIP_FW_RAM_SIZE                      82240

#endif

#define DEFAULT_READ_TOKEN_TIMEOUT 1000000
#define LONG_READ_TOKEN_TIMEOUT 100000000
#ifdef TEST_RAM_FIRMWARE
#define USE_CRC32
#define VIP_FW_RAM_CRC32                     0xd5fa7e38

#define VIP_FW_RAM_DW_0                      0xcf000000 // 00 00 00 cf 
#define VIP_FW_RAM_DW_1                      0x02775746 // 46 57 77 02
#define VIP_FW_RAM_DW_n_minus_1              0x6778a107 // 07 a1 78 67
#define VIP_FW_RAM_DW_n                      0x65073e1c // 1c 3e 07 65
#endif

#define PTR232LO(x) ((uint32_t)(((size_t)x) & 0xFFFFFFFFu))
#define PTR232HI(x) ((uint32_t)((((size_t)x) >> 32u) & 0xFFFFFFFFu))

static uint16_t gs_next_token_id = 0x1000;

static VAULTIP_INPUT_TOKEN_t input_token;
static VAULTIP_OUTPUT_TOKEN_t output_token;

void memory_copy_8x32(volatile uint32_t * dst, const volatile uint32_t * src, const volatile uint32_t * src_end);

static inline void * volatile_cast(volatile void * vp) {
    union {
        volatile void * vp;
        void * p;
    } u;

    u.vp = vp;
    return u.p;
}

static inline void * const_cast(const void * vp) {
    union {
        const void * vp;
        void * p;
    } u;

    u.vp = vp;
    return u.p;
}

static uint16_t get_next_token_id(void) {
    return gs_next_token_id++;
}

#ifdef TEST_RAM_FIRMWARE
static int test_ram_firmware(const uint32_t * firmware_data, uint32_t firmware_size) {
#ifdef USE_CRC32
    uint32_t crc = 0;
    crc32(firmware_data, firmware_size, &crc);
    if (VIP_FW_RAM_CRC32 != crc) {
        printx("VaultIP firmware RAM image CRC mismatch!\n");
        return -1;
    }
    printx("VaultIP firmware RAM Image CRC OK.\n");
#else
    uint32_t last_index = (firmware_size / 4) - 1;
    if (VIP_FW_RAM_DW_0 != firmware_data[0]) {
        return -1;
    }
    if (VIP_FW_RAM_DW_1 != firmware_data[1]) {
        return -1;
    }
    if (VIP_FW_RAM_DW_n_minus_1 != firmware_data[last_index-1]) {
        return -1;
    }
    if (VIP_FW_RAM_DW_n != firmware_data[last_index]) {
        return -1;
    }
#endif
    return 0;
}
#endif

int vaultip_test_initial_state(void) {
    volatile VAULTIP_HW_REGS_t * vaultip_regs = (VAULTIP_HW_REGS_t *)R_SP_VAULT_BASEADDR;
    EIP_VERSION_t eip_version;
    MODULE_STATUS_t module_status;

    eip_version.R = vaultip_regs->EIP_VERSION.R;
    if (0x82 != eip_version.B.EIP_number || 0x7D != eip_version.B.EIP_number_complement) {
        printx("EIP_VERSION mismatch!\n");
        return -1;
    }

    module_status.R = vaultip_regs->MODULE_STATUS.R;
    if (0 == module_status.B.CRC24_OK) {
        printx("MODULE_STATUS CRC24_OK is not 1!\n");
        return -1;
    }
    if (0 != module_status.B.FatalError) {
        printx("MODULE_STATUS FatalError is 1!\n");
        return -1;
    }
    if (0 != module_status.B.fw_image_written) {
        printx("MODULE_STATUS fw_image_written is 1!\n");
        return -1;
    }
    if (0 != module_status.B.fw_image_checks_done) {
        printx("MODULE_STATUS fw_image_checks_done is 1!\n");
        return -1;
    }
    if (0 != module_status.B.fw_image_accepted) {
        printx("MODULE_STATUS fw_image_accepted is 1!\n");
        return -1;
    }

    return 0;
}

void dump_vip_regs(void) {
    volatile VAULTIP_HW_REGS_t * vaultip_regs = (VAULTIP_HW_REGS_t *)R_SP_VAULT_BASEADDR;
    uint32_t val;

    val = vaultip_regs->EIP_VERSION.R;
    printx("EIP_VERSION: 0x%x\n", val);

    val = vaultip_regs->EIP_OPTIONS2.R;
    printx("EIP_OPTIONS2: 0x%x\n", val);

    val = vaultip_regs->EIP_OPTIONS.R;
    printx("EIP_OPTIONS: 0x%x\n", val);

    val = vaultip_regs->MODULE_STATUS.R;
    printx("MODULE_STATUS: 0x%x\n", val);

    val = vaultip_regs->MAILBOX_STAT.R;
    printx("MAILBOX_STAT: 0x%x\n", val);

    val = vaultip_regs->MAILBOX_RAWSTAT.R;
    printx("MAILBOX_RAWSTAT: 0x%x\n", val);

    val = vaultip_regs->MAILBOX_LINKID.R;
    printx("MAILBOX_LINKID: 0x%x\n", val);

    val = vaultip_regs->MAILBOX_OUTID.R;
    printx("MAILBOX_OUTID: 0x%x\n", val);

    val = vaultip_regs->MAILBOX_LOCKOUT.R;
    printx("MAILBOX_LOCKOUT: 0x%x\n", val);
}

int vaultip_send_input_token(const VAULTIP_INPUT_TOKEN_t * pinput_token) {
    volatile VAULTIP_HW_REGS_t * vaultip_regs = (VAULTIP_HW_REGS_t *)R_SP_VAULT_BASEADDR;
    MODULE_STATUS_t module_status;
    MAILBOX_STAT_t mailbox_stat;
    //uint32_t n;

    if (NULL == pinput_token) {
        return -1;
    }

    module_status.R = vaultip_regs->MODULE_STATUS.R;
    if (0 != module_status.B.FatalError || 0 == module_status.B.fw_image_checks_done || 0 == module_status.B.fw_image_accepted) {
        return -1;
    }

    mailbox_stat.R = vaultip_regs->MAILBOX_STAT.R;
    if (0 != mailbox_stat.B.mbx1_linked || 0 == mailbox_stat.B.mbx1_available) {
        return -1;
    }

    // link mailbox

    vaultip_regs->MAILBOX_CTRL.R = (MAILBOX_CTRL_t){ .B.mbx1_link = 1 }.R;
    mailbox_stat.R = vaultip_regs->MAILBOX_STAT.R;
    if (0 == mailbox_stat.B.mbx1_linked) {
        printx("MAILBOX_STAT mbx1_linked is still 0!\n");
        return -1;
    }
    if (0 != mailbox_stat.B.mbx1_in_full) {
        printx("MAILBOX_STAT mbx1_in_full is not 0!\n");
        return -1;
    }

    // write the input token

    memory_copy_8x32(vaultip_regs->MAILBOX1, &pinput_token->dw[0], &pinput_token->dw[64]);

    // report that the input token was written

    vaultip_regs->MAILBOX_CTRL.R = (MAILBOX_CTRL_t){ .B.mbx1_in_full = 1 }.R;
    mailbox_stat.R = vaultip_regs->MAILBOX_STAT.R;
    if (0 == mailbox_stat.B.mbx1_in_full) {
        printx("MAILBOX_STAT mbx1_in_full is not 1!\n");
        return -1;
    }
/*
    printx("Wrote token:\n");
    for (n = 0; n < 64; n+=8) {
        printx("  %08x %08x %08x %08x %08x %08x %08x %08x\n", 
               pinput_token->dw[n+0], pinput_token->dw[n+1], pinput_token->dw[n+2], pinput_token->dw[n+3],
               pinput_token->dw[n+4], pinput_token->dw[n+5], pinput_token->dw[n+6], pinput_token->dw[n+7]);
    }
*/
    // if (!suppress_token_send_diagnostics) {
    //     printx("SI[0] = 0x%08x\n", pinput_token->dw_00);
    // }

    return 0;
}

int vaultip_read_output_token(VAULTIP_OUTPUT_TOKEN_t * poutput_token, uint32_t timeout) {
    volatile VAULTIP_HW_REGS_t * vaultip_regs = (VAULTIP_HW_REGS_t *)R_SP_VAULT_BASEADDR;
    MODULE_STATUS_t module_status;
    MAILBOX_STAT_t mailbox_stat;

    if (NULL == poutput_token) {
        return -1;
    }

    module_status.R = vaultip_regs->MODULE_STATUS.R;
    if (0 != module_status.B.FatalError || 0 == module_status.B.fw_image_checks_done || 0 == module_status.B.fw_image_accepted) {
        printx("vaultip_read_output_token: Error 1: MODULE_STATS=%08x\n", module_status.R );
        return -1;
    }

    mailbox_stat.R = vaultip_regs->MAILBOX_STAT.R;
    if (0 == mailbox_stat.B.mbx1_linked || 0 != mailbox_stat.B.mbx1_available) {
        printx("vaultip_read_output_token: Error 2: mailbox_stat=%08x\n", mailbox_stat.R );
        return -1;
    }

    // wait for the output token to become available

    mailbox_stat.R = vaultip_regs->MAILBOX_STAT.R;
    while (0 == mailbox_stat.B.mbx1_out_full) {
        if (timeout > 0) {
            timeout--;
            if (0 == timeout) {
                printx("vaultip_read_output_token: Timeout waiting for output token!\n");

                printx("MAILBOX_STAT = 0x%08x\n", mailbox_stat.R);
                module_status.R = vaultip_regs->MODULE_STATUS.R;
                printx("MODULE_STATUS = 0x%08x\n", module_status.R);
                return -1;
            }
            module_status.R = vaultip_regs->MODULE_STATUS.R;
            if (0 != module_status.B.FatalError) {
                printx("vaultip_read_output_token: Fatal error!\n");
                printx("MAILBOX_STAT = 0x%08x\n", mailbox_stat.R);
                printx("MODULE_STATUS = 0x%08x\n", module_status.R);
                return -1;
            }
        }
        mailbox_stat.R = vaultip_regs->MAILBOX_STAT.R;
    }

    // read the output token

    memory_copy_8x32(poutput_token->dw, &vaultip_regs->MAILBOX1[0], &vaultip_regs->MAILBOX1[64]);

    // report that the output token was read

    vaultip_regs->MAILBOX_CTRL.R = (MAILBOX_CTRL_t){ .B.mbx1_out_empty = 1 }.R;
    mailbox_stat.R = vaultip_regs->MAILBOX_STAT.R;
    if (0 != mailbox_stat.B.mbx1_out_full) {
        printx("MAILBOX_STAT mbx1_out_full is 1!\n");
        return -1;
    }

    // unlink mailbox

    vaultip_regs->MAILBOX_CTRL.R = (MAILBOX_CTRL_t){ .B.mbx1_unlink = 1 }.R;
    mailbox_stat.R = vaultip_regs->MAILBOX_STAT.R;
    if (1 == mailbox_stat.B.mbx1_linked) {
        printx("MAILBOX_STAT mbx1_linked is still 1!\n");
        return -1;
    }

    // if (!suppress_token_read_diagnostics) {
    //     printx("SO[0] = 0x%08x\n", poutput_token->dw_00);
    // }

    return 0;
}

static void print_failed_input_token_info(const uint32_t * input_token_words, uint32_t words_count) {
    uint32_t n;
    printx("Input token:\n");
    for (n = 0; n < words_count; n++) {
        printx("  si[%u] = %08x\n", n, input_token_words[n]);
    }
    printx("\n");
}

int vaultip_get_system_information(VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_t * system_info) {
    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_SYSTEM;
    input_token.dw_00.SubCode = VAULTIP_TOKEN_SYSTEM_SUBCODE_SYSTEM_INFORMATION;

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        *system_info = output_token.system_info;
        return 0;
    } else {
        return -1;
    }
}

int vaultip_register_read(bool incremental_read, uint32_t number, const uint32_t * address, uint32_t * result) {
    uint32_t n;

    if (number > VAULTIP_INPUT_TOKEN_REGISTER_READ_MAXIMUM_NUMBER) {
        printx("Invalid register number!\n");
        return -1;
    }
    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_01.Identity = VAULTIP_CRYPTO_OFFICER_IDENTITY;

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_SERVICE;
    input_token.dw_00.SubCode = VAULTIP_TOKEN_SERVICE_SUBCODE_REGISTER_READ;

    input_token.register_read.dw_02.Number = number & 0x3F;
    if (incremental_read) {
        input_token.register_read.dw_02.Mode = 1;
        input_token.register_read.dw_03_63[0].Address = address[0] & 0xFFFFu;
    } else {
        input_token.register_read.dw_02.Mode = 0;
        for (n = 0; n < number; n++) {
            input_token.register_read.dw_03_63[n].Address = address[n] & 0xFFFFu;
        }
    }

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        for (n = 0; n < number; n++) {
            result[n] = output_token.register_read.dw_01_61[n].ReadData;
        }
        return 0;
    } else {
        return -1;
    }
}

int vaultip_register_write(bool incremental_write, uint32_t number, const uint32_t * mask, const uint32_t * address, const uint32_t * value) {
    uint32_t n;

    if (number > VAULTIP_INPUT_TOKEN_REGISTER_READ_MAXIMUM_NUMBER) {
        printx("Invalid register number!\n");
        return -1;
    }
    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_01.Identity = VAULTIP_CRYPTO_OFFICER_IDENTITY;

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_SERVICE;
    input_token.dw_00.SubCode = VAULTIP_TOKEN_SERVICE_SUBCODE_REGISTER_WRITE;

    input_token.register_write.dw_02.Number = number & 0x3F;
    if (incremental_write) {
        if (number > VAULTIP_INPUT_TOKEN_REGISTER_WRITE_INCREMENTAL_MAXIMUM_NUMBER) {
            return -1;
        }
        input_token.register_write.dw_02.Mode = 1;
        input_token.register_write.incremental.Mask = mask[0];
        input_token.register_write.incremental.Address = address[0] & 0xFFFFu;
        for (n = 0; n < number; n++) {
            input_token.register_write.incremental.WriteData[n] = value[n];
        }
    } else {
        if (number > VAULTIP_INPUT_TOKEN_REGISTER_WRITE_NON_INCREMENTAL_MAXIMUM_NUMBER) {
            return -1;
        }
        input_token.register_write.dw_02.Mode = 0;
        for (n = 0; n < number; n++) {
            input_token.register_write.non_incremental.entries[n].Mask = mask[0];
            input_token.register_write.non_incremental.entries[n].Address = address[0] & 0xFFFFu;
            input_token.register_write.non_incremental.entries[n].Address = address[n] & 0xFFFFu;
        }
    }

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        return 0;
    } else {
        return -1;
    }
}

int vaultip_trng_configuration(void) {
    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_TRNG;
    input_token.dw_00.SubCode = VAULTIP_TOKEN_TRNG_SUBCODE_CONFIGURE_NRBG;

    input_token.trng_configuration.dw_02.LST = 1;
    input_token.trng_configuration.dw_02.RRD = 1;
    input_token.trng_configuration.dw_02.AutoSeed = 32;
    input_token.trng_configuration.dw_02.FroBlockKey = 0;
    input_token.trng_configuration.dw_03.NoiseBlocks = 8;
    input_token.trng_configuration.dw_03.Scale = 0;
    input_token.trng_configuration.dw_03.SampleDiv = 0;
    input_token.trng_configuration.dw_03.SampleCycles = 8;
    input_token.trng_configuration.dw_03.Scale = 0;

    //suppress_token_read_diagnostics = true;

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_send_input_token() failed!\n");
        return -1;
    }

    //suppress_token_read_diagnostics = false;

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        return 0;
    } else {
        return -1;
    }
}

int vaultip_provision_huk(void) {
    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_ASSET_MANAGEMENT;
    input_token.dw_00.SubCode = VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_PROVISION_RANDOM_HUK;
    
    input_token.dw_01.Identity = VAULTIP_CRYPTO_OFFICER_IDENTITY;

    input_token.provision_huk.dw_02.AssetNumber = VAULTIP_STATIC_ASSET_HUK;
    input_token.provision_huk.dw_02.AutoSeed = 0;
    input_token.provision_huk.dw_02.Size_128bit = 0;
    input_token.provision_huk.dw_02.Size_256bit = 1;
    input_token.provision_huk.dw_02.CRC = 1;
    input_token.provision_huk.dw_02.KeyBlob = 0;

    input_token.provision_huk.dw_03.NoiseBlocks = 8;
    input_token.provision_huk.dw_03.SampleDiv = 0;
    input_token.provision_huk.dw_03.SampleCycles = 16;

    input_token.provision_huk.dw_04.AssociatedDataLength = 0;
    input_token.provision_huk.dw_04.OutputDataLength = 0;

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, LONG_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        return 0;
    } else {
        return -1;
    }
}

int vaultip_reset(void) {
    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_SYSTEM;
    input_token.dw_00.SubCode = VAULTIP_TOKEN_SYSTEM_SUBCODE_RESET;

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        return 0;
    } else {
        return -1;
    }
}

int vaultip_hash(HASH_ALG_t hash_alg, const void * msg, size_t msg_size, uint8_t * hash) {
    void * temp_msg;
    uint32_t hash_size;
    uint32_t hash_type;

    switch(hash_alg) {
    case HASH_ALG_SHA2_256:
        hash_type = VAULTIP_HASH_ALGORITHM_SHA_256;
        hash_size = 256 / 8;
        break;
    case HASH_ALG_SHA2_384:
        hash_type = VAULTIP_HASH_ALGORITHM_SHA_384;
        hash_size = 384 / 8;
        break;
    case HASH_ALG_SHA2_512:
        hash_type = VAULTIP_HASH_ALGORITHM_SHA_512;
        hash_size = 512 / 8;
        break;
    default:
        printx("vaultip_hash: invalid hash_alg!\n");
        return -1;
    }

    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_HASH;
    input_token.hash.dw_02.DataLength = (uint32_t)msg_size;
    temp_msg = const_cast(msg);
    input_token.hash.dw_03.InputDataAddress_31_00 = (uint32_t)(size_t)temp_msg;
    input_token.hash.dw_04.InputDataAddress_63_32 = 0;
    input_token.hash.dw_05.InputDataLength = msg_size & 0x1FFFFFu;
    input_token.hash.dw_06.Algorithm = hash_type & 0xFu;
    input_token.hash.dw_06.Mode = VAULTIP_HASH_MODE_INITIAL_FINAL;
    input_token.hash.dw_24.TotalMessageLength_31_00 = (uint32_t)msg_size;
    input_token.hash.dw_25.TotalMessageLength_60_32 = 0;

    l1_data_cache_flush_region(msg, msg_size);

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_hash: vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_hash: vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        memcpy(hash, &output_token.hash.dw_02_17, hash_size);
        return 0;
    } else {
        printx("vaultip_hash: output_token = 0x%x\n", output_token.dw[0]);
        print_failed_input_token_info(input_token.dw, 26);
        return -1;
    }
}

int vaultip_hash_update(HASH_ALG_t hash_alg, uint32_t digest_asset_id, const void * msg, size_t msg_size, bool init) {
    uint32_t hash_type;

    switch(hash_alg) {
    case HASH_ALG_SHA2_256:
        hash_type = VAULTIP_HASH_ALGORITHM_SHA_256;
        break;
    case HASH_ALG_SHA2_384:
        hash_type = VAULTIP_HASH_ALGORITHM_SHA_384;
        break;
    case HASH_ALG_SHA2_512:
        hash_type = VAULTIP_HASH_ALGORITHM_SHA_512;
        break;
    default:
        printx("vaultip_hash_update: invalid hash_alg!\n");
        return -1;
    }

    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_HASH;
    input_token.hash.dw_02.DataLength = (uint32_t)msg_size;
    input_token.hash.dw_03.InputDataAddress_31_00 = PTR232LO(msg);
    input_token.hash.dw_04.InputDataAddress_63_32 = PTR232HI(msg);
    input_token.hash.dw_05.InputDataLength = msg_size & 0x1FFFFFu;
    input_token.hash.dw_06.Algorithm = hash_type & 0xFu;
    input_token.hash.dw_06.Mode = init ? VAULTIP_HASH_MODE_INITIAL_NOT_FINAL : VAULTIP_HASH_MODE_CONTINUED_NOT_FINAL;
    input_token.hash.dw_07.Digest_AS_ID = digest_asset_id;
    input_token.hash.dw_24.TotalMessageLength_31_00 = 0;
    input_token.hash.dw_25.TotalMessageLength_60_32 = 0;

    l1_data_cache_flush_region(msg, msg_size);

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_hash_update: vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_hash_update: vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        return 0;
    } else {
        printx("vaultip_hash_update: output_token = 0x%x\n", output_token.dw[0]);
        print_failed_input_token_info(input_token.dw, 26);
        return -1;
    }
}

int vaultip_hash_final(HASH_ALG_t hash_alg, uint32_t digest_asset_id, const void * msg, size_t msg_size, bool init, size_t total_msg_length, uint8_t * hash) {
    uint32_t hash_size;
    uint32_t hash_type;

    switch(hash_alg) {
    case HASH_ALG_SHA2_256:
        hash_type = VAULTIP_HASH_ALGORITHM_SHA_256;
        hash_size = 256 / 8;
        break;
    case HASH_ALG_SHA2_384:
        hash_type = VAULTIP_HASH_ALGORITHM_SHA_384;
        hash_size = 384 / 8;
        break;
    case HASH_ALG_SHA2_512:
        hash_type = VAULTIP_HASH_ALGORITHM_SHA_512;
        hash_size = 512 / 8;
        break;
    default:
        printx("vaultip_hash_final: invalid hash_alg!\n");
        return -1;
    }

    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_HASH;
    input_token.hash.dw_02.DataLength = (uint32_t)msg_size;
    input_token.hash.dw_03.InputDataAddress_31_00 = PTR232LO(msg);
    input_token.hash.dw_04.InputDataAddress_63_32 = PTR232HI(msg);
    input_token.hash.dw_05.InputDataLength = msg_size & 0x1FFFFFu;
    input_token.hash.dw_06.Algorithm = hash_type & 0xFu;
    input_token.hash.dw_06.Mode = init ? VAULTIP_HASH_MODE_INITIAL_FINAL : VAULTIP_HASH_MODE_CONTINUED_FINAL;
    input_token.hash.dw_07.Digest_AS_ID = digest_asset_id;
    input_token.hash.dw_24.TotalMessageLength_31_00 = (uint32_t)total_msg_length;
    input_token.hash.dw_25.TotalMessageLength_60_32 = (total_msg_length >> 32u) & 0x1FFFFFFFu;

    l1_data_cache_flush_region(msg, msg_size);

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_hash_final: vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_hash_final: vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        memcpy(hash, output_token.hash.dw_02_17, hash_size);
        return 0;
    } else {
        printx("vaultip_hash_final: output_token = 0x%x\n", output_token.dw[0]);
        print_failed_input_token_info(input_token.dw, 26);
        return -1;
    }
}

int vaultip_mac_generate(ESPERANTO_MAC_TYPE_t mac_alg, uint32_t key_asset_id, const void * msg, size_t msg_size, uint8_t * mac) {
    uint32_t mac_type;
    uint32_t mac_size;

    switch(mac_alg) {
    case ESPERANTO_MAC_TYPE_AES_CMAC:
        mac_type = VAULTIP_MAC_ALGORITHM_AES_CMAC;
        mac_size = 128 / 8;
        break;
    case ESPERANTO_MAC_TYPE_HMAC_SHA2_256:
        mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_256;
        mac_size = 256 / 8;
        break;
    case ESPERANTO_MAC_TYPE_HMAC_SHA2_384:
        mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_384;
        mac_size = 384 / 8;
        break;
    case ESPERANTO_MAC_TYPE_HMAC_SHA2_512:
        mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_512;
        mac_size = 512 / 8;
        break;
    default:
        printx("vaultip_mac_generate: invalid mac_alg!\n");
        return -1;
    }

    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_MAC;
    input_token.mac.dw_02.DataLength = (uint32_t)msg_size;
    input_token.mac.dw_03.InputDataAddress_31_00 = PTR232LO(msg);
    input_token.mac.dw_04.InputDataAddress_63_32 = PTR232HI(msg);
    input_token.mac.dw_05.InputDataLength = msg_size & 0x1FFFFFu;
    input_token.mac.dw_06.Algorithm = mac_type & 0xFu;
    input_token.mac.dw_06.Mode = VAULTIP_MAC_MODE_INITIAL_FINAL;
    input_token.mac.dw_06.AS_LoadKey = 1u;
    input_token.mac.dw_06.AS_LoadMAC = 0u;
    input_token.mac.dw_06.KeyLength = 0u;
 
    input_token.mac.dw_07.MAC_AS_ID = 0;
    input_token.dw[28] = key_asset_id;

    input_token.mac.dw_24.TotalMessageLength_31_00 = msg_size & 0xFFFFFFFF;
    input_token.mac.dw_25.TotalMessageLength_60_32 = 0;

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_mac_generate: vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_mac_generate: vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        memcpy(mac, output_token.mac.dw_02_17, mac_size);
        return 0;
    } else {
        printx("vaultip_mac_generate: output_token = 0x%x\n", output_token.dw[0]);
        print_failed_input_token_info(input_token.dw, 60);
        return -1;
    }
}

int vaultip_mac_verify(ESPERANTO_MAC_TYPE_t mac_alg, uint32_t key_asset_id, const void * msg, size_t msg_size, const uint8_t * mac) {
    uint32_t mac_type;
    uint32_t mac_size;

    switch(mac_alg) {
    case ESPERANTO_MAC_TYPE_AES_CMAC:
        mac_type = VAULTIP_MAC_ALGORITHM_AES_CMAC;
        mac_size = 128 / 8;
        break;
    case ESPERANTO_MAC_TYPE_HMAC_SHA2_256:
        mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_256;
        mac_size = 256 / 8;
        break;
    case ESPERANTO_MAC_TYPE_HMAC_SHA2_384:
        mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_384;
        mac_size = 384 / 8;
        break;
    case ESPERANTO_MAC_TYPE_HMAC_SHA2_512:
        mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_512;
        mac_size = 512 / 8;
        break;
    default:
        printx("vaultip_mac_verify: invalid mac_alg!\n");
        return -1;
    }

    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_MAC;
    input_token.mac.dw_02.DataLength = (uint32_t)msg_size;
    input_token.mac.dw_03.InputDataAddress_31_00 = PTR232LO(msg);
    input_token.mac.dw_04.InputDataAddress_63_32 = PTR232HI(msg);
    input_token.mac.dw_05.InputDataLength = msg_size & 0x1FFFFFu;
    input_token.mac.dw_06.Algorithm = mac_type & 0xFu;
    input_token.mac.dw_06.Mode = VAULTIP_MAC_MODE_INITIAL_FINAL;
    input_token.mac.dw_06.AS_LoadKey = 1u;
    input_token.mac.dw_06.AS_LoadMAC = 1u;
    input_token.mac.dw_06.KeyLength = 0u;
 
    input_token.mac.dw_07.MAC_AS_ID = 0;
    memcpy(input_token.mac.dw_23_08, mac, mac_size);

    input_token.mac.dw_24.TotalMessageLength_31_00 = msg_size & 0xFFFFFFFF;
    input_token.mac.dw_25.TotalMessageLength_60_32 = 0;

    input_token.dw[28] = key_asset_id;

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_mac_verify: vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_mac_verify: vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        return 0;
    } else {
        printx("vaultip_mac_verify: output_token = 0x%x\n", output_token.dw[0]);
        print_failed_input_token_info(input_token.dw, 60);
        return -1;
    }
}

int vaultip_mac_update(ESPERANTO_MAC_TYPE_t mac_alg, uint32_t mac_asset_id, uint32_t key_asset_id, const void * msg, size_t msg_size, bool init) {
    uint32_t mac_type;

    switch(mac_alg) {
    case ESPERANTO_MAC_TYPE_AES_CMAC:
        mac_type = VAULTIP_MAC_ALGORITHM_AES_CMAC;
        break;
    case ESPERANTO_MAC_TYPE_HMAC_SHA2_256:
        mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_256;
        break;
    case ESPERANTO_MAC_TYPE_HMAC_SHA2_384:
        mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_384;
        break;
    case ESPERANTO_MAC_TYPE_HMAC_SHA2_512:
        mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_512;
        break;
    default:
        printx("vaultip_mac_update: invalid mac_alg!\n");
        return -1;
    }

    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_MAC;
    input_token.mac.dw_02.DataLength = (uint32_t)msg_size;
    input_token.mac.dw_03.InputDataAddress_31_00 = PTR232LO(msg);
    input_token.mac.dw_04.InputDataAddress_63_32 = PTR232HI(msg);
    input_token.mac.dw_05.InputDataLength = msg_size & 0x1FFFFFu;
    input_token.mac.dw_06.Algorithm = mac_type & 0xFu;
    input_token.mac.dw_06.Mode = init ? VAULTIP_MAC_MODE_INITIAL_FINAL : VAULTIP_MAC_MODE_CONTINUED_FINAL;
    input_token.mac.dw_06.AS_LoadKey = 1u;
    input_token.mac.dw_06.AS_LoadMAC = 0u;
    input_token.mac.dw_06.KeyLength = 0u;
 
    input_token.mac.dw_07.MAC_AS_ID = mac_asset_id;
    input_token.dw[28] = key_asset_id;

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_mac_update: vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_mac_update: vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        return 0;
    } else {
        printx("vaultip_mac_update: output_token = 0x%x\n", output_token.dw[0]);
        print_failed_input_token_info(input_token.dw, 60);
        return -1;
    }
}

int vaultip_mac_final_generate(ESPERANTO_MAC_TYPE_t mac_alg, uint32_t mac_asset_id, uint32_t key_asset_id, const void * msg, size_t msg_size, size_t total_msg_size, bool init, uint8_t * mac) {
    uint32_t mac_type;
    uint32_t mac_size;

    switch(mac_alg) {
    case ESPERANTO_MAC_TYPE_AES_CMAC:
        mac_type = VAULTIP_MAC_ALGORITHM_AES_CMAC;
        mac_size = 128 / 8;
        break;
    case ESPERANTO_MAC_TYPE_HMAC_SHA2_256:
        mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_256;
        mac_size = 256 / 8;
        break;
    case ESPERANTO_MAC_TYPE_HMAC_SHA2_384:
        mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_384;
        mac_size = 384 / 8;
        break;
    case ESPERANTO_MAC_TYPE_HMAC_SHA2_512:
        mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_512;
        mac_size = 512 / 8;
        break;
    default:
        printx("vaultip_mac_final_generate: invalid mac_alg!\n");
        return -1;
    }

    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_MAC;
    input_token.mac.dw_02.DataLength = (uint32_t)msg_size;
    input_token.mac.dw_03.InputDataAddress_31_00 = PTR232LO(msg);
    input_token.mac.dw_04.InputDataAddress_63_32 = PTR232HI(msg);
    input_token.mac.dw_05.InputDataLength = msg_size & 0x1FFFFFu;
    input_token.mac.dw_06.Algorithm = mac_type & 0xFu;
    input_token.mac.dw_06.Mode = init ? VAULTIP_MAC_MODE_INITIAL_FINAL : VAULTIP_MAC_MODE_CONTINUED_FINAL;
    input_token.mac.dw_06.AS_LoadKey = 1u;
    input_token.mac.dw_06.AS_LoadMAC = 0u;
    input_token.mac.dw_06.KeyLength = 0u;
 
    input_token.mac.dw_07.MAC_AS_ID = mac_asset_id;

    input_token.mac.dw_24.TotalMessageLength_31_00 = total_msg_size & 0xFFFFFFFF;
    input_token.mac.dw_25.TotalMessageLength_60_32 = 0;

    input_token.dw[28] = key_asset_id;

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_mac_final_generate: vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_mac_final_generate: vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        memcpy(mac, output_token.mac.dw_02_17, mac_size);
        return 0;
    } else {
        printx("vaultip_mac_final_generate: output_token = 0x%x\n", output_token.dw[0]);
        print_failed_input_token_info(input_token.dw, 60);
        return -1;
    }
}

int vaultip_mac_final_verify(ESPERANTO_MAC_TYPE_t mac_alg, uint32_t mac_asset_id, uint32_t key_asset_id, const void * msg, size_t msg_size, size_t total_msg_size, bool init, const uint8_t * mac) {
    uint32_t mac_type;
    uint32_t mac_size;

    switch(mac_alg) {
    case ESPERANTO_MAC_TYPE_AES_CMAC:
        mac_type = VAULTIP_MAC_ALGORITHM_AES_CMAC;
        mac_size = 128 / 8;
        break;
    case ESPERANTO_MAC_TYPE_HMAC_SHA2_256:
        mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_256;
        mac_size = 256 / 8;
        break;
    case ESPERANTO_MAC_TYPE_HMAC_SHA2_384:
        mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_384;
        mac_size = 384 / 8;
        break;
    case ESPERANTO_MAC_TYPE_HMAC_SHA2_512:
        mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_512;
        mac_size = 512 / 8;
        break;
    default:
        printx("vaultip_mac_final_verify: invalid mac_alg!\n");
        return -1;
    }

    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_MAC;
    input_token.mac.dw_02.DataLength = (uint32_t)msg_size;
    input_token.mac.dw_03.InputDataAddress_31_00 = PTR232LO(msg);
    input_token.mac.dw_04.InputDataAddress_63_32 = PTR232HI(msg);
    input_token.mac.dw_05.InputDataLength = msg_size & 0x1FFFFFu;
    input_token.mac.dw_06.Algorithm = mac_type & 0xFu;
    input_token.mac.dw_06.Mode = init ? VAULTIP_MAC_MODE_INITIAL_FINAL : VAULTIP_MAC_MODE_CONTINUED_FINAL;
    input_token.mac.dw_06.AS_LoadKey = 1u;
    input_token.mac.dw_06.AS_LoadMAC = 1u;
    input_token.mac.dw_06.KeyLength = 0u;
 
    input_token.mac.dw_07.MAC_AS_ID = mac_asset_id;
    memcpy(input_token.mac.dw_23_08, mac, mac_size);

    input_token.mac.dw_24.TotalMessageLength_31_00 = total_msg_size & 0xFFFFFFFF;
    input_token.mac.dw_25.TotalMessageLength_60_32 = 0;

    input_token.dw[28] = key_asset_id;

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_mac_final_verify: vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_mac_final_verify: vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        return 0;
    } else {
        printx("vaultip_mac_final_verify: output_token = 0x%x\n", output_token.dw[0]);
        print_failed_input_token_info(input_token.dw, 60);
        return -1;
    }
}

static int vaultip_aes_cbc(uint32_t identity, uint32_t key_asset_id, uint8_t * IV, void * data, size_t data_size, bool encrypt) {
    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_ENCRYPTION;
    input_token.dw_01.Identity = identity;
    input_token.encryption.dw_02.DataLength = (uint32_t)data_size;
    input_token.encryption.dw_03.InputDataAddress_31_00 = PTR232LO(data);
    input_token.encryption.dw_04.InputDataAddress_63_32 = PTR232HI(data);
    input_token.encryption.dw_05.InputDataLength = data_size & 0x1FFFFFu;
    input_token.encryption.dw_06.OutputDataAddress_31_00 = PTR232LO(data);
    input_token.encryption.dw_07.OutputDataAddress_63_32 = PTR232HI(data);
    input_token.encryption.dw_08.OutputDataLength = data_size & 0x1FFFFFu;
    //input_token.encryption.dw_09.AssociatedDataAddress_31_00 = 0;
    //input_token.encryption.dw_10.AssociatedDataAddress_63_32 = 0;
    input_token.encryption.dw_11.Algorithm = VAULTIP_ENCRYPT_ALGORITHM_AES;
    input_token.encryption.dw_11.Mode = VAULTIP_ENCRYPT_MODE_CBC;
    input_token.encryption.dw_11.AS_LoadKey = 1;
    input_token.encryption.dw_11.AS_LoadIV = 0;
    //input_token.encryption.dw_11.LoadParam = 0;
    //input_token.encryption.dw_11.AS_SaveIV = 1;
    //input_token.encryption.dw_11.GCM_Mode = 0;
    input_token.encryption.dw_11.Encrypt = encrypt ? 1 : 0;
    input_token.encryption.dw_11.KeyLength = VAULTIP_ENCRYPT_KEY_LENGTH_256;
    //input_token.encryption.dw_11.NonceLength = 0;
    //input_token.encryption.dw_11.TagLength_or_F8_SaltKeyLength = 0;
    input_token.encryption.dw_12.SaveIV_AS_ID = 0;
    memcpy(input_token.encryption.dw_16_13, IV, 16);
    input_token.encryption.dw_24_17[0].Key32 = key_asset_id;

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_aes_cbc: vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_aes_cbc: vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        memcpy(IV, output_token.encryption.dw_05_02, 16);
        return 0;
    } else {
        printx("vaultip_aes_cbc: output_token = 0x%x\n", output_token.dw[0]);
        print_failed_input_token_info(input_token.dw, 41);
        return -1;
    }
}

int vaultip_aes_cbc_encrypt(uint32_t identity, uint32_t key_asset_id, uint8_t * IV, void * data, size_t data_size) {
    return vaultip_aes_cbc(identity, key_asset_id, IV, data, data_size, true);
}

int vaultip_aes_cbc_decrypt(uint32_t identity, uint32_t key_asset_id, uint8_t * IV, void * data, size_t data_size) {
    return vaultip_aes_cbc(identity, key_asset_id, IV, data, data_size, false);
}

int vaultip_asset_create(uint32_t identity, uint32_t policy_31_00, uint32_t policy_63_32, VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t other_settings, uint32_t lifetime, uint32_t * asset_id) {
    // uint32_t n;

    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_ASSET_MANAGEMENT;
    input_token.dw_00.SubCode = VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_ASSET_CREATE;
    input_token.dw_01.Identity = identity;
    input_token.asset_create.dw_02.Policy_31_00 = policy_31_00;
    input_token.asset_create.dw_03.Policy_63_32 = policy_63_32;
    input_token.asset_create.dw_04 = other_settings;

    input_token.asset_create.dw_05.Lifetime = lifetime;

    // printx("vaultip_asset_create: policy=0x%08x_%08x, settings=0x%08x\n", policy_63_32, policy_31_00, other_settings);
    // for (n = 0; n < 6; n++) {
    //     printx("asset_create[%u]=%08x\n", n, input_token.dw[n]);
    // }

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        *asset_id = output_token.asset_create.dw_01.AS_ID;
        printx("Created asset id 0x%08x\n", output_token.asset_create.dw_01.AS_ID);
        return 0;
    } else {
        printx("vaultip_asset_create: output_token = 0x%x\n", output_token.dw[0]);
        print_failed_input_token_info(input_token.dw, 6);
        return -1;
    }
}

int vaultip_asset_load_plaintext(uint32_t identity, uint32_t asset_id, const void * data, uint32_t data_size) {
    // uint32_t n;

    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_ASSET_MANAGEMENT;
    input_token.dw_00.SubCode = VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_ASSET_LOAD;
    input_token.dw_01.Identity = identity;
    input_token.asset_load.dw_02.AS_ID = asset_id;
    input_token.asset_load.dw_03.PlainText = 1;
    //input_token.asset_load.dw_03.RFC5869 = 1; // todo: verify if this is required
    input_token.asset_load.dw_03.InputDataLength = data_size & 0x3FFu;
    input_token.asset_load.dw_04.InputDataAddress_31_00 = PTR232LO(data);
    input_token.asset_load.dw_05.InputDataAddress_63_32 = PTR232HI(data);

    // for (n = 0; n < 6; n++) {
    //     printx("asset_load[%u]=%08x\n", n, input_token.dw[n]);
    // }

    printx("vaultip_asset_load_plaintext: assetid=0x%08x, length=0x%08x\n", asset_id, data_size);

    l1_data_cache_flush_region(data, data_size);

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        return 0;
    } else {
        printx("vaultip_asset_load_plaintext: output_token[0]=%08x\n", output_token.dw[0]);
        return -1;
    }
}

int vaultip_asset_load_derive(uint32_t identity, uint32_t asset_id, uint32_t kdk_asset_id, const uint8_t * key_expansion_IV, uint32_t key_expansion_IV_length, const uint8_t * associated_data, uint32_t associated_data_size, const uint8_t * salt, uint32_t salt_size) {
    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_ASSET_MANAGEMENT;
    input_token.dw_00.SubCode = VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_ASSET_LOAD;
    input_token.dw_01.Identity = identity;
    input_token.asset_load.dw_02.AS_ID = asset_id;
    input_token.asset_load.dw_03.Derive = 1;
    input_token.asset_load.dw_03.Counter = 1; // todo: verify if this is required or desired
    input_token.asset_load.dw_03.RFC5869 = 1; // todo: verify if this is required or desired
    input_token.asset_load.dw_03.AssociatedDataLength = associated_data_size & 0xFFu;
    input_token.asset_load.dw_03.InputDataLength = salt_size & 0x3FFu;
    input_token.asset_load.dw_04.InputDataAddress_31_00 = PTR232LO(salt);
    input_token.asset_load.dw_05.InputDataAddress_63_32 = PTR232HI(salt);
    input_token.asset_load.dw_06.OutputDataAddress_31_00 = PTR232LO(key_expansion_IV);
    input_token.asset_load.dw_07.OutputDataAddress_63_32 = PTR232HI(key_expansion_IV);
    input_token.asset_load.dw_08.OutputDataLength = key_expansion_IV_length & 0x7FF;
    input_token.asset_load.dw_09.Key_AS_ID = kdk_asset_id;
    memcpy(input_token.asset_load.dw_10_63[0].AssociatedData, associated_data, associated_data_size);

    l1_data_cache_flush_region(salt, salt_size);
    l1_data_cache_flush_region(key_expansion_IV, key_expansion_IV_length);

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        return 0;
    } else {
        return -1;
    }
}

int vaultip_asset_delete(uint32_t identity, uint32_t asset_id) {
    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_ASSET_MANAGEMENT;
    input_token.dw_00.SubCode = VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_ASSET_DELETE;
    input_token.dw_01.Identity = identity;
    input_token.asset_delete.dw_02.AS_ID = asset_id;

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        return 0;
    } else {
        return -1;
    }
}

int vaultip_static_asset_search(uint32_t identity, VAULTIP_STATIC_ASSET_ID_t asset_number, uint32_t * asset_id, uint32_t * data_length) {
    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_ASSET_MANAGEMENT;
    input_token.dw_00.SubCode = VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_ASSET_SEARCH;
    input_token.dw_01.Identity = identity;
    input_token.static_asset_search.dw_04.AssetNumber = asset_number & 0x3Fu;

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        *asset_id = output_token.static_asset_search.dw_01.AS_ID;
        *data_length = output_token.static_asset_search.dw_02.DataLength;
        return 0;
    } else {
        return -1;
    }
}

int vaultip_public_key_ecdsa_verify(EC_KEY_CURVE_ID_t curve_id, uint32_t identity, uint32_t public_key_asset_id, 
                                 uint32_t curve_parameters_asset_id, uint32_t temp_message_digest_asset_id, 
                                 const void * message, uint32_t message_size, uint32_t hash_data_length, 
                                 const void * sig_data_address, uint32_t sig_data_size) {
    uint32_t modulus_size;
    uint32_t modulus_words;
    // uint32_t n;

    switch (curve_id) {
    case EC_KEY_CURVE_NIST_P256:
        modulus_size = 256;
        break;
    case EC_KEY_CURVE_NIST_P384:
        modulus_size = 384;
        break;
    case EC_KEY_CURVE_NIST_P521:
        modulus_size = 521;
        break;
    default:
        printx("vaultip_public_key_ec_verify: invalid curve_id!\n");
        return -1;
    }
    modulus_words = (modulus_size + 31) / 32;

    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_PUBLIC_KEY_OPERATION;
    input_token.dw_00.SubCode = VAULTIP_TOKEN_PUBLIC_KEY_SUBCODE_USES_ASSETS;
    input_token.dw_01.Identity = identity;
    input_token.public_key.dw_02.Command = VAULTIP_PUBLIC_KEY_COMMAND_ECDSA_VERIFY;
    input_token.public_key.dw_02.Mwords = (uint8_t)modulus_words;
    input_token.public_key.dw_02.Nwords = (uint8_t)modulus_words;
    input_token.public_key.dw_04.KeyAssetRef = public_key_asset_id;
    input_token.public_key.dw_05.ParamAssetRef = curve_parameters_asset_id;
    input_token.public_key.dw_06.IOAssetRef = temp_message_digest_asset_id;
    input_token.public_key.dw_07.InputDataSize = message_size & 0xFFFu;
    input_token.public_key.dw_07.OutputDataSize_or_SigDataSize = sig_data_size & 0xFFFu;
    input_token.public_key.dw_08.InputDataAddress_31_00 = PTR232LO(message);
    input_token.public_key.dw_09.InputDataAddress_63_32 = PTR232HI(message);
    input_token.public_key.dw_10.SigDataAddress_31_00 = PTR232LO(sig_data_address);
    input_token.public_key.dw_11.SigDataAddress_63_32 = PTR232HI(sig_data_address);
    input_token.public_key.dw_12_63.HashDataLength = hash_data_length;

    // for (n = 0; n < 13; n++) {
    //     printx("ec_verify[%u]=%08x\n", n, input_token.dw[n]);
    // }


    l1_data_cache_flush_region(message, message_size);
    l1_data_cache_flush_region(sig_data_address, sig_data_size);

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        return 0;
    } else {
        printx("vaultip_public_key_ecdsa_verify: vaultip_read_output_token() output token[0] = 0x%08x\n", output_token.dw_00);
        return -1;
    }
}

int vaultip_public_key_rsa_pss_verify(uint32_t modulus_size, uint32_t identity, uint32_t public_key_asset_id, 
                                      uint32_t temp_message_digest_asset_id, const void * message, uint32_t message_size, 
                                      uint32_t hash_data_length, const void * sig_data_address, uint32_t sig_data_size,
                                      uint32_t salt_length) {
    uint32_t modulus_words;
    //uint32_t n;

    switch (modulus_size) {
    case 4096:
    case 3072:
    case 2048:
        break;
    default:
        printx("vaultip_public_key_rsa_pss_verify: invalid modulus_size!\n");
        return -1;
    }
    modulus_words = (modulus_size + 31) / 32;

    memset(&input_token, 0, sizeof(input_token));
    memset(&output_token, 0, sizeof(output_token));

    input_token.dw_00.TokenID = get_next_token_id();
    input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_PUBLIC_KEY_OPERATION;
    input_token.dw_00.SubCode = VAULTIP_TOKEN_PUBLIC_KEY_SUBCODE_USES_ASSETS;
    input_token.dw_01.Identity = identity;
    input_token.public_key.dw_02.Command = VAULTIP_PUBLIC_KEY_COMMAND_RSA_PSS_VERIFY;
    input_token.public_key.dw_02.Mwords = (uint8_t)modulus_words;
    input_token.public_key.dw_02.Nwords = (uint8_t)modulus_words;
    input_token.public_key.dw_03.OtherLen = salt_length & 0xFFu;
    input_token.public_key.dw_04.KeyAssetRef = public_key_asset_id;
    input_token.public_key.dw_06.IOAssetRef = temp_message_digest_asset_id;
    input_token.public_key.dw_07.InputDataSize = message_size & 0xFFFu;
    input_token.public_key.dw_07.OutputDataSize_or_SigDataSize = sig_data_size & 0xFFFu;
    input_token.public_key.dw_08.InputDataAddress_31_00 = PTR232LO(message);
    input_token.public_key.dw_09.InputDataAddress_63_32 = PTR232HI(message);
    input_token.public_key.dw_10.SigDataAddress_31_00 = PTR232LO(sig_data_address);
    input_token.public_key.dw_11.SigDataAddress_63_32 = PTR232HI(sig_data_address);
    input_token.public_key.dw_12_63.HashDataLength = hash_data_length;

    // for (n = 0; n < 13; n++) {
    //     printx("rsa_verify[%u]=%08x\n", n, input_token.dw[n]);
    // }

    l1_data_cache_flush_all();
    // l1_data_cache_flush_region(message, message_size);
    // l1_data_cache_flush_region(sig_data_address, sig_data_size);

    if (0 != vaultip_send_input_token(&input_token)) {
        printx("vaultip_send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&output_token, DEFAULT_READ_TOKEN_TIMEOUT)) {
        printx("vaultip_read_output_token() failed!\n");
        return -1;
    }

    if (0 == output_token.dw_00.Error) {
        return 0;
    } else {
        return -1;
    }
}
