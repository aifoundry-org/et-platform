#ifndef __VAULTIP_STATIC_ASSETS_H__
#define __VAULTIP_STATIC_ASSETS_H__

typedef enum VAULTIP_STATIC_ASSET_ID_e {
    VAULTIP_STATIC_ASSET_HUK = 0,                           // AES-256 key
    VAULTIP_STATIC_ASSET_SP_ROOT_CA_HASH,                   // SHA2-512 hash of SP ROOT CA certificate
    VAULTIP_STATIC_ASSET_VAULTIP_SFWPK,                     // SHA2-256 hash of the Secure Firmware EC-P256 signing key
    VAULTIP_STATIC_ASSET_VAULTIP_SFWBCR,                    // AES-256 Secure Firmware encryption key
    VAULTIP_STATIC_ASSET_VAULTIP_SECURE_JTAG_KEY,           // EC-P256 Secure JTAG public key
    VAULTIP_STATIC_ASSET_AUTHKEY_DYNAMIC_ASSET_UNWRAP_KEY,  // AES-256 Secure Debug Auth Key Dynamic Asset unwrap key
    VAULTIP_STATIC_ASSET_COUNT
} VAULTIP_STATIC_ASSET_ID_t;

#endif
