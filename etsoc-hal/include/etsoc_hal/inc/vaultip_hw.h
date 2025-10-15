/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef __VAULTIP_HW_H__
#define __VAULTIP_HW_H__

#include <stdint.h>

#define VAULTIP_MAILBOX_SIZE 256

typedef union _AIC_REGISTER_s {
    uint32_t R;
    struct  {
        uint32_t mbx1_in_free : 1;
        uint32_t mbx1_token_done : 1;
        uint32_t mbx2_in_free : 1;
        uint32_t mbx2_token_done : 1;
        uint32_t mbx3_in_free : 1;
        uint32_t mbx3_token_done : 1;
        uint32_t mbx4_in_free : 1;
        uint32_t mbx4_token_done : 1;
        uint32_t mailbox_linkable : 1;
        uint32_t reserved : 23;
    } B;
} AIC_REGISTER_t;

typedef union _AIC_OPTIONS_s {
    uint32_t R;
    struct {
        uint32_t number_of_inputs : 6;
        uint32_t reserved : 26;
    } B;
} AIC_OPTIONS_t;

typedef union _AIC_VERSION_s {
    uint32_t R;
    struct {
        uint32_t EIP_number : 8;
        uint32_t EIP_number_complement : 8;
        uint32_t patch_level : 4;
        uint32_t minor_version : 4;
        uint32_t major_version : 4;
        uint32_t reserved : 4;
    } B;
} AIC_VERSION_t;

typedef union _MAILBOX_STAT_s {
    uint32_t R;
    struct {
        uint32_t mbx1_in_full : 1;
        uint32_t mbx1_out_full : 1;
        uint32_t mbx1_linked : 1;
        uint32_t mbx1_available : 1;
        uint32_t mbx2_in_full : 1;
        uint32_t mbx2_out_full : 1;
        uint32_t mbx2_linked : 1;
        uint32_t mbx2_available : 1;
        uint32_t mbx3_in_full : 1;
        uint32_t mbx3_out_full : 1;
        uint32_t mbx3_linked : 1;
        uint32_t mbx3_available : 1;
        uint32_t mbx4_in_full : 1;
        uint32_t mbx4_out_full : 1;
        uint32_t mbx4_linked : 1;
        uint32_t mbx4_available : 1;
        uint32_t reserved : 16;
    } B;
} MAILBOX_STAT_t;

typedef union _MAILBOX_CTRL_s {
    uint32_t R;
    struct {
        uint32_t mbx1_in_full : 1;
        uint32_t mbx1_out_empty : 1;
        uint32_t mbx1_link : 1;
        uint32_t mbx1_unlink : 1;
        uint32_t mbx2_in_full : 1;
        uint32_t mbx2_out_empty : 1;
        uint32_t mbx2_link : 1;
        uint32_t mbx2_unlink : 1;
        uint32_t mbx3_in_full : 1;
        uint32_t mbx3_out_empty : 1;
        uint32_t mbx3_link : 1;
        uint32_t mbx3_unlink : 1;
        uint32_t mbx4_in_full : 1;
        uint32_t mbx4_out_empty : 1;
        uint32_t mbx4_link : 1;
        uint32_t mbx4_unlink : 1;
        uint32_t reserved : 16;
    } B;
} MAILBOX_CTRL_t;

typedef union _MAILBOX_RAWSTAT_s {
    uint32_t R;
    struct {
        uint32_t mbx1_in_full : 1;
        uint32_t mbx1_out_full : 1;
        uint32_t mbx1_linked : 1;
        uint32_t reserved1 : 1;
        uint32_t mbx2_in_full : 1;
        uint32_t mbx2_out_full : 1;
        uint32_t mbx2_linked : 1;
        uint32_t reserved2 : 1;
        uint32_t mbx3_in_full : 1;
        uint32_t mbx3_out_full : 1;
        uint32_t mbx3_linked : 1;
        uint32_t reserved3 : 1;
        uint32_t mbx4_in_full : 1;
        uint32_t mbx4_out_full : 1;
        uint32_t mbx4_linked : 1;
        uint32_t reserved4 : 17;
    } B;
} MAILBOX_RAWSTAT_t;

typedef union _MAILBOX_RESET_s {
    uint32_t R;
    struct {
        uint32_t reserved1 : 1;
        uint32_t mbx1_out_empty : 1;
        uint32_t reserved2 : 1;
        uint32_t mbx1_unlink : 1;
        uint32_t reserved3 : 1;
        uint32_t mbx2_out_empty : 1;
        uint32_t reserved4 : 1;
        uint32_t mbx2_unlink : 1;
        uint32_t reserved5 : 1;
        uint32_t mbx3_out_empty : 1;
        uint32_t reserved6 : 1;
        uint32_t mbx3_unlink : 1;
        uint32_t reserved7 : 1;
        uint32_t mbx4_out_empty : 1;
        uint32_t reserved8 : 1;
        uint32_t mbx4_unlink : 1;
        uint32_t reserved9 : 16;
    } B;
} MAILBOX_RESET_t;

typedef union _MAILBOX_LINKID_s {
    uint32_t R;
    struct {
        uint32_t mbx1_link_id : 3;
        uint32_t mbx1_out_prot_acc : 1;
        uint32_t mbx2_link_id : 3;
        uint32_t mbx2_out_prot_acc : 1;
        uint32_t mbx3_link_id : 3;
        uint32_t mbx3_out_prot_acc : 1;
        uint32_t mbx4_link_id : 3;
        uint32_t mbx4_out_prot_acc : 1;
        uint32_t reserved : 16;
    } B;
} MAILBOX_LINKID_t;

typedef union _MAILBOX_OUTID_s {
    uint32_t R;
    struct {
        uint32_t mbx1_out_id : 3;
        uint32_t mbx1_out_prot_acc : 1;
        uint32_t mbx2_out_id : 3;
        uint32_t mbx2_out_prot_acc : 1;
        uint32_t mbx3_out_id : 3;
        uint32_t mbx3_out_prot_acc : 1;
        uint32_t mbx4_out_id : 3;
        uint32_t mbx4_out_prot_acc : 1;
        uint32_t reserved : 16;
    } B;
} MAILBOX_OUTID_t;

typedef union _MAILBOX_LOCKOUT_s {
    uint32_t R;
    struct {
        uint32_t mbx1_lockout : 8;
        uint32_t mbx2_lockout : 8;
        uint32_t mbx3_lockout : 8;
        uint32_t mbx4_lockout : 8;
    } B;
} MAILBOX_LOCKOUT_t;

typedef union _MODULE_STATUS_s {
    uint32_t R;
    struct {
        uint32_t FIPS_mode : 1;
        uint32_t Non_FIPS_mode : 1;
        uint32_t reserved1 : 6;
        uint32_t CRC24_Busy : 1;
        uint32_t CRC24_OK : 1;
        uint32_t CRC24_Error : 1;
        uint32_t reserved2 : 9;
        uint32_t fw_image_written : 1;
        uint32_t reserved3 : 1;
        uint32_t fw_image_checks_done : 1;
        uint32_t fw_image_accepted : 1;
        uint32_t reserved4 : 7;
        uint32_t FatalError : 1;
    } B;
} MODULE_STATUS_t;

typedef union _EIP_OPTIONS2_s {
    uint32_t R;
    struct {
        uint32_t des_aes : 1;
        uint32_t arc4 : 1;
        uint32_t md5_sha : 1;
        uint32_t trng : 1;
        uint32_t crc : 1;
        uint32_t pkcp : 1;
        uint32_t reserved1 : 2;
        uint32_t c_cpu : 1;
        uint32_t reserved2 : 3;
        uint32_t bus_ifc : 1;
        uint32_t reserved3 : 3;
        uint32_t add_ce1 : 1;
        uint32_t add_ce2 : 1;
        uint32_t add_ce3 : 1;
        uint32_t add_ce4 : 1;
        uint32_t add_ce5 : 1;
        uint32_t add_ce6 : 1;
        uint32_t add_ce7 : 1;
        uint32_t add_ce8 : 1;
        uint32_t add_ce9 : 1;
        uint32_t add_ce10 : 1;
        uint32_t reserved4 : 6;
    } B;
} EIP_OPTIONS2_t;

typedef union _EIP_OPTIONS_s {
    uint32_t R;
    struct {
        uint32_t nr_of_mailboxes : 4;
        uint32_t mailbox_size : 2;
        uint32_t reserved : 2;
        uint32_t host_id_0 : 1;
        uint32_t host_id_1 : 1;
        uint32_t host_id_2 : 1;
        uint32_t host_id_3 : 1;
        uint32_t host_id_4 : 1;
        uint32_t host_id_5 : 1;
        uint32_t host_id_6 : 1;
        uint32_t host_id_7 : 1;
        uint32_t master_id : 3;
        uint32_t prot_av : 1;
        uint32_t my_id : 3;
        uint32_t prot : 1;
        uint32_t host_id_sec_0 : 1;
        uint32_t host_id_sec_1 : 1;
        uint32_t host_id_sec_2 : 1;
        uint32_t host_id_sec_3 : 1;
        uint32_t host_id_sec_4 : 1;
        uint32_t host_id_sec_5 : 1;
        uint32_t host_id_sec_6 : 1;
        uint32_t host_id_sec_7 : 1;
    } B;
} EIP_OPTIONS_t;

typedef union _EIP_VERSION_s {
    uint32_t R;
    struct {
        uint32_t EIP_number : 8;
        uint32_t EIP_number_complement : 8;
        uint32_t patch_level : 4;
        uint32_t minor_version : 4;
        uint32_t major_version : 4;
        uint32_t reserved : 4;
    } B;
} EIP_VERSION_t;

typedef struct _VAULTIP_HW_REGS_s {
    uint32_t MAILBOX1[VAULTIP_MAILBOX_SIZE/sizeof(uint32_t)];

    uint32_t reserved1[(0x0400 - 0x0100) / sizeof(uint32_t)];

    uint32_t MAILBOX2[VAULTIP_MAILBOX_SIZE/sizeof(uint32_t)];

    uint32_t reserved2[(0x0800 - 0x0500) / sizeof(uint32_t)];

    uint32_t MAILBOX3[VAULTIP_MAILBOX_SIZE/sizeof(uint32_t)];

    uint32_t reserved3[(0x0C00 - 0x0900) / sizeof(uint32_t)];

    uint32_t MAILBOX4[VAULTIP_MAILBOX_SIZE/sizeof(uint32_t)];

    uint32_t reserved4[(0x3E00 - 0x0D00) / sizeof(uint32_t)];

    AIC_REGISTER_t AIC_POL_CTRL;
    AIC_REGISTER_t AIC_TYPE_CTRL;
    AIC_REGISTER_t AIC_ENABLE_CTRL;
    union {
        AIC_REGISTER_t AIC_RAW_STAT;
        AIC_REGISTER_t AIC_ENABLE_SET;
    };
    union {
        AIC_REGISTER_t AIC_ENABLED_STAT;
        AIC_REGISTER_t AIC_ACK;
    };
    AIC_REGISTER_t AIC_ENABLE_CLR;
    AIC_OPTIONS_t AIC_OPTIONS;
    AIC_VERSION_t AIC_VERSION;

    uint32_t reserved5[(0x3F00 - 0x3E20) / sizeof(uint32_t)];

    union {
        MAILBOX_STAT_t MAILBOX_STAT;
        MAILBOX_CTRL_t MAILBOX_CTRL;
    };
    union {
        MAILBOX_RAWSTAT_t MAILBOX_RAWSTAT;
        MAILBOX_RESET_t MAILBOX_RESET;
    };
    MAILBOX_LINKID_t MAILBOX_LINKID;
    MAILBOX_OUTID_t MAILBOX_OUTID;
    MAILBOX_LOCKOUT_t MAILBOX_LOCKOUT;

    uint32_t reserved6[(0x3FE0 - 0x3F14) / sizeof(uint32_t)];

    MODULE_STATUS_t MODULE_STATUS;

    uint32_t reserved7[(0x3FF4 - 0x3FE4) / sizeof(uint32_t)];

    EIP_OPTIONS2_t EIP_OPTIONS2;
    EIP_OPTIONS_t EIP_OPTIONS;
    EIP_VERSION_t EIP_VERSION;
    uint32_t FWRAM_IN[(0x1C000-0x4000)/sizeof(uint32_t)];

    uint32_t reserved8[(0x20000 - 0x1C000) / sizeof(uint32_t)];
} VAULTIP_HW_REGS_t;

#define VAULTIP_MAXIMUM_FIRMWARE_SIZE       0x18000

#define VAULTIP_DMAC_MST_RUNPARAMS          0xF878
#define VAULTIP_DMAC_OPTIONS2               0xF8DC

#endif // __VAULT_IP_H__
