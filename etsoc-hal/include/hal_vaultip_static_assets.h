/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef __VAULTIP_STATIC_ASSETS_H__
#define __VAULTIP_STATIC_ASSETS_H__

typedef enum VAULTIP_STATIC_ASSET_ID_e {
    // following are entries used by the Vault IP
    VAULTIP_STATIC_ASSET_HUK = 0,                           // AES-256 key
    VAULTIP_STATIC_ASSET_VAULTIP_SFWBCR,                    // AES-256 Secure Firmware encryption key
    VAULTIP_STATIC_ASSET_VAULTIP_SFWPKD,                    // SHA2-256 Digest (hash) of the Secure Firmware EC-P256 signing key
    VAULTIP_STATIC_ASSET_VAULTIP_SECURE_JTAG_KEY,           // EC-P256 Secure JTAG public key
    VAULTIP_STATIC_ASSET_AUTHKEY_DYNAMIC_ASSET_UNWRAP_KEY,  // AES-256 Secure Debug Auth Key Dynamic Asset unwrap key
    // following are entries used by the SP
    VAULTIP_STATIC_ASSET_SP_ROOT_CA_HASH,                   // SHA2-512 hash of SP ROOT CA certificate
    VAULTIP_STATIC_ASSET_PCIE_CFG_DATA_REVOCATION_COUNTER,  // monotonic counter - minimum version of PCIe PHY config data
    VAULTIP_STATIC_ASSET_SP_BL1_REVOCATION_COUNTER,         // monotonic counter - minimum version of SP BL1 image
    // the following values will be used by later stage firmware (post SP-ROM)
    VAULTIP_STATIC_ASSET_SP_BL2_REVOCATION_COUNTER,         // monotonic counter - minimum version of SP BL2 image
    VAULTIP_STATIC_ASSET_MACHINE_MINION_REVOCATION_COUNTER, // monotonic counter - minimum version of Machine Minion image
    VAULTIP_STATIC_ASSET_MASTER_MINION_REVOCATION_COUNTER,  // monotonic counter - minimum version of Master Minion image
    VAULTIP_STATIC_ASSET_WORKER_MINION_REVOCATION_COUNTER,  // monotonic counter - minimum version of Worker Minion image
    VAULTIP_STATIC_ASSET_MAXION_BL1_REVOCATION_COUNTER,     // monotonic counter - minimum version of Maxion BL1 image
    VAULTIP_STATIC_ASSET_COUNT
} VAULTIP_STATIC_ASSET_ID_t;

#endif
