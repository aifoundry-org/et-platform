#ifndef __VAULTIP_SW_H__
#define __VAULTIP_SW_H__

#include <stdint.h>

typedef struct VAULTIP_INPUT_TOKEN_WORD_0_s {
    uint32_t TokenID : 16;
    uint32_t Priority : 2;
    uint32_t WrTokenID : 1;
    uint32_t Reserved : 5;
    uint32_t OpCode : 4;
    uint32_t SubCode : 4;
} VAULTIP_INPUT_TOKEN_WORD_0_t;

typedef struct VAULTIP_INPUT_TOKEN_WORD_1_s {
    uint32_t Identity : 32;
} VAULTIP_INPUT_TOKEN_WORD_1_t;

typedef struct VAULTIP_OUTPUT_TOKEN_WORD_0_s {
    uint32_t TokenID : 16;
    uint32_t Reserved : 8;
    uint32_t Result : 5;
    uint32_t ResultSrc : 2;
    uint32_t Error : 1;
} VAULTIP_OUTPUT_TOKEN_WORD_0_t;

typedef struct VAULTIP_INPUT_TOKEN_DATA_LENGTH_s {
    uint32_t DataLength : 32;
} VAULTIP_INPUT_TOKEN_DATA_LENGTH_t;

typedef struct VAULTIP_INPUT_TOKEN_INPUT_DATA_ADDRESS_LO_s {
    uint32_t InputDataAddress_31_00 : 32;
} VAULTIP_INPUT_TOKEN_INPUT_DATA_ADDRESS_LO_t;

typedef struct VAULTIP_INPUT_TOKEN_INPUT_DATA_ADDRESS_HI_s {
    uint32_t InputDataAddress_63_32 : 32;
} VAULTIP_INPUT_TOKEN_INPUT_DATA_ADDRESS_HI_t;

typedef struct VAULTIP_INPUT_TOKEN_INPUT_DATA_LENGTH_s {
    uint32_t InputDataLength : 21;
    uint32_t Reserved : 11;
} VAULTIP_INPUT_TOKEN_INPUT_DATA_LENGTH_t;

typedef struct VAULTIP_INPUT_TOKEN_OUTPUT_DATA_ADDRESS_LO_s {
    uint32_t OutputDataAddress_31_00 : 32;
} VAULTIP_INPUT_TOKEN_OUTPUT_DATA_ADDRESS_LO_t;

typedef struct VAULTIP_INPUT_TOKEN_OUTPUT_DATA_ADDRESS_HI_s {
    uint32_t OutputDataAddress_63_32 : 32;
} VAULTIP_INPUT_TOKEN_OUTPUT_DATA_ADDRESS_HI_t;

typedef struct VAULTIP_INPUT_TOKEN_OUTPUT_DATA_LENGTH_s {
    uint32_t OutputDataLength : 21;
    uint32_t Reserved : 11;
} VAULTIP_INPUT_TOKEN_OUTPUT_DATA_LENGTH_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct VAULTIP_INPUT_TOKEN_NOP_s {
    VAULTIP_INPUT_TOKEN_DATA_LENGTH_t               dw_02;
    VAULTIP_INPUT_TOKEN_INPUT_DATA_ADDRESS_LO_t     dw_03;
    VAULTIP_INPUT_TOKEN_INPUT_DATA_ADDRESS_HI_t     dw_04;
    VAULTIP_INPUT_TOKEN_INPUT_DATA_LENGTH_t         dw_05;
    VAULTIP_INPUT_TOKEN_OUTPUT_DATA_ADDRESS_LO_t    dw_06;
    VAULTIP_INPUT_TOKEN_OUTPUT_DATA_ADDRESS_HI_t    dw_07;
    VAULTIP_INPUT_TOKEN_OUTPUT_DATA_LENGTH_t        dw_08;
} VAULTIP_INPUT_TOKEN_NOP_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_9_s {
    uint32_t AssociatedDataAddress_31_00 : 32;
} VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_9_t;

typedef struct VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_10_s {
        uint32_t AssociatedDataAddress_63_32 : 32;
} VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_10_t;


typedef struct VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_11_s {
    uint32_t Algorithm : 4;
    uint32_t Mode : 4;
    uint32_t AS_LoadKey : 1;
    uint32_t AS_LoadIV : 1;
    uint32_t Reserved : 1;
    uint32_t LoadParam : 1;
    uint32_t AS_SaveIV : 1;
    uint32_t GCM_Mode : 1;
    uint32_t Encrypt : 1;
    uint32_t KeyLength : 4;
    uint32_t NonceLength : 4;
    uint32_t TagLength_or_F8_SaltKeyLength : 5;
    uint32_t Reserved2 : 3;
} VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_11_t;

typedef struct VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_12_s {
    uint32_t SaveIV_AS_ID : 32;
} VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_12_t;

typedef struct VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_13_16_s {
    uint8_t IV[4];
} VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_13_16_t;

typedef struct VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_24_17_s {
    uint8_t Key[4];
} VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_24_17_t;

typedef struct VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_25_s {
    uint32_t AssociatedDataLength;
    uint8_t Key[4];
    uint8_t f8_IV[4];
} VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_25_t;

typedef struct VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_26_28_s {
    uint8_t Key[4];
    uint8_t f8_IV[4];
    uint8_t Hash_Key[4];
} VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_26_28_t;

typedef struct VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_29_31_s {
    uint8_t Key[4];
    uint8_t Hash_Key[4];
    uint8_t Nonce[4];
    uint8_t f8_SaltKey[4];
} VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_29_31_t;

typedef struct VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_32_s {
    uint8_t Key[4];
    uint8_t f8_SaltKey[4];
    struct {
        uint8_t Nonce[1];
        uint8_t Reserved[3];
    };
} VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_32_t;

typedef struct VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_33_36_s {
    uint8_t TagToVerify[4];
} VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_33_36_t;

typedef struct VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_37_40_s {
    uint8_t Parameter[4];
} VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_37_40_t;

typedef struct VAULTIP_INPUT_TOKEN_ENCRYPTION_s {
    VAULTIP_INPUT_TOKEN_DATA_LENGTH_t               dw_02;
    VAULTIP_INPUT_TOKEN_INPUT_DATA_ADDRESS_LO_t     dw_03;
    VAULTIP_INPUT_TOKEN_INPUT_DATA_ADDRESS_HI_t     dw_04;
    VAULTIP_INPUT_TOKEN_INPUT_DATA_LENGTH_t         dw_05;
    VAULTIP_INPUT_TOKEN_OUTPUT_DATA_ADDRESS_LO_t    dw_06;
    VAULTIP_INPUT_TOKEN_OUTPUT_DATA_ADDRESS_HI_t    dw_07;
    VAULTIP_INPUT_TOKEN_OUTPUT_DATA_LENGTH_t        dw_08;
    VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_9_t         dw_09;
    VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_10_t        dw_10;
    VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_11_t        dw_11;
    VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_12_t        dw_12;
    VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_13_16_t     dw_16_13[4];
    VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_24_17_t     dw_24_17[8];
    VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_25_t        dw_25;
    VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_26_28_t     dw_28_26[3];
    VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_29_31_t     dw_31_29[3];
    VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_32_t        dw_32;
    VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_33_36_t     dw_36_33[4];
    VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_37_40_t     dw_40_37[4];
} VAULTIP_INPUT_TOKEN_ENCRYPTION_t;

typedef struct VAULTIP_OUTPUT_TOKEN_ENCRYPTION_WORD_2_5_s {
    uint8_t IV[4];
} VAULTIP_OUTPUT_TOKEN_ENCRYPTION_WORD_2_5_t;

typedef struct VAULTIP_OUTPUT_TOKEN_ENCRYPTION_WORD_6_9_s {
    uint8_t Tag[4];
} VAULTIP_OUTPUT_TOKEN_ENCRYPTION_WORD_6_9_t;

typedef struct VAULTIP_OUTPUT_TOKEN_ENCRYPTION_s {
    uint32_t                                        reserved;
    VAULTIP_OUTPUT_TOKEN_ENCRYPTION_WORD_2_5_t      dw_05_02[4];
    VAULTIP_OUTPUT_TOKEN_ENCRYPTION_WORD_6_9_t      dw_09_06[4];
} VAULTIP_OUTPUT_TOKEN_ENCRYPTION_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct VAULTIP_INPUT_TOKEN_HASH_WORD_6_s {
    uint32_t Algorithm : 4;
    uint32_t Mode : 2;
    uint32_t Reserved : 26;
} VAULTIP_INPUT_TOKEN_HASH_WORD_6_t;

typedef struct VAULTIP_INPUT_TOKEN_HASH_WORD_7_s {
    uint32_t Digest_AS_ID : 32;
} VAULTIP_INPUT_TOKEN_HASH_WORD_7_t;

typedef struct VAULTIP_INPUT_TOKEN_HASH_WORD_8_23_s {
    uint8_t Digest[4];
} VAULTIP_INPUT_TOKEN_HASH_WORD_8_23_t;

typedef struct VAULTIP_INPUT_TOKEN_HASH_WORD_24_s {
    uint32_t TotalMessageLength_31_00 : 32;
} VAULTIP_INPUT_TOKEN_HASH_WORD_24_t;

typedef struct VAULTIP_INPUT_TOKEN_HASH_WORD_25_s {
    uint32_t TotalMessageLength_60_32 : 29;
    uint32_t Reserved : 3;
} VAULTIP_INPUT_TOKEN_HASH_WORD_25_t;

typedef struct VAULTIP_INPUT_TOKEN_HASH_s {
    VAULTIP_INPUT_TOKEN_DATA_LENGTH_t           dw_02;
    VAULTIP_INPUT_TOKEN_INPUT_DATA_ADDRESS_LO_t dw_03;
    VAULTIP_INPUT_TOKEN_INPUT_DATA_ADDRESS_HI_t dw_04;
    VAULTIP_INPUT_TOKEN_INPUT_DATA_LENGTH_t     dw_05;
    VAULTIP_INPUT_TOKEN_HASH_WORD_6_t           dw_06;
    VAULTIP_INPUT_TOKEN_HASH_WORD_7_t           dw_07;
    VAULTIP_INPUT_TOKEN_HASH_WORD_8_23_t        dw_08_23[16];
    VAULTIP_INPUT_TOKEN_HASH_WORD_24_t          dw_24;
    VAULTIP_INPUT_TOKEN_HASH_WORD_25_t          dw_25;
} VAULTIP_INPUT_TOKEN_HASH_t;


typedef union VAULTIP_OUTPUT_TOKEN_HASH_WORD_2_17_u {
    uint32_t u32;
    uint8_t u8[4];
} VAULTIP_OUTPUT_TOKEN_HASH_WORD_2_17_t;

typedef struct VAULTIP_OUTPUT_TOKEN_HASH_s {
    uint32_t                                    dw_01;
    VAULTIP_OUTPUT_TOKEN_HASH_WORD_2_17_t       dw_02_17[16];
} VAULTIP_OUTPUT_TOKEN_HASH_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct VAULTIP_INPUT_TOKEN_MAC_WORD_6_s {
    uint32_t Algorithm : 4;
    uint32_t Mode : 2;
    uint32_t Reserved : 2;
    uint32_t AS_LoadKey : 1;
    uint32_t AS_LoadMAC : 1;
    uint32_t Reserved2 : 6;
    uint32_t KeyLength : 8;
    uint32_t Reserved3 : 8;
} VAULTIP_INPUT_TOKEN_MAC_WORD_6_t;

typedef struct VAULTIP_INPUT_TOKEN_MAC_WORD_8_23_s {
    uint8_t MAC[4];
} VAULTIP_INPUT_TOKEN_MAC_WORD_8_23_t;

typedef struct VAULTIP_INPUT_TOKEN_MAC_WORD_24_s {
    uint32_t TotalMessageLength_31_00 : 32;
} VAULTIP_INPUT_TOKEN_MAC_WORD_24_t;

typedef struct VAULTIP_INPUT_TOKEN_MAC_WORD_25_s {
    uint32_t TotalMessageLength_60_32 : 29;
    uint32_t Reserved : 3;
} VAULTIP_INPUT_TOKEN_MAC_WORD_25_t;

typedef struct VAULTIP_INPUT_TOKEN_MAC_WORD_28_59_s {
    uint8_t Key[4];
} VAULTIP_INPUT_TOKEN_MAC_WORD_28_59_t;

typedef struct VAULTIP_INPUT_TOKEN_MAC_s {
    VAULTIP_INPUT_TOKEN_DATA_LENGTH_t           dw_02;
    VAULTIP_INPUT_TOKEN_INPUT_DATA_ADDRESS_LO_t dw_03;
    VAULTIP_INPUT_TOKEN_INPUT_DATA_ADDRESS_HI_t dw_04;
    VAULTIP_INPUT_TOKEN_INPUT_DATA_LENGTH_t     dw_05;
    VAULTIP_INPUT_TOKEN_MAC_WORD_6_t            dw_06;
    VAULTIP_INPUT_TOKEN_MAC_WORD_8_23_t         dw_23_08[16];
    VAULTIP_INPUT_TOKEN_MAC_WORD_24_t           dw_24;
    VAULTIP_INPUT_TOKEN_MAC_WORD_25_t           dw_25;
    uint32_t                                    reserved[2];
    VAULTIP_INPUT_TOKEN_MAC_WORD_28_59_t        dw_59_28[32];
} VAULTIP_INPUT_TOKEN_MAC_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_WORD_1_s {
    uint32_t FWVersionPatch : 8;
    uint32_t FWVersionMinor : 8;
    uint32_t FWVersionMajor : 8;
    uint32_t Reserved : 8;
} VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_WORD_1_t;

typedef struct VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_WORD_2_s {
    uint32_t HWVersionPatch : 8;
    uint32_t HWVersionMinor : 8;
    uint32_t HWVersionMajor : 8;
    uint32_t Reserved : 8;
} VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_WORD_2_t;

typedef struct VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_WORD_3_s {
    uint32_t MemorySize : 16;
    uint32_t HostId : 3;
    uint32_t NS : 1;
    uint32_t Reserved : 7;
    uint32_t CO : 1;
    uint32_t FIPS : 4;
} VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_WORD_3_t;

typedef struct VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_WORD_4_s {
    uint32_t Identity : 32;
} VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_WORD_4_t;

typedef struct VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_WORD_5_s {
    uint32_t OTPAnomalyLocation : 12;
    uint32_t OTPAnomaly : 4;
    uint32_t Reserved : 16;
} VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_WORD_5_t;

typedef struct VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_s {
    VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_WORD_1_t      dw_01;
    VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_WORD_2_t      dw_02;
    VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_WORD_3_t      dw_03;
    VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_WORD_4_t      dw_04;
    VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_WORD_5_t      dw_05;
} VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct VAULTIP_INPUT_TOKEN_REGISTER_READ_WORD_2_s {
    uint32_t Number : 6;
    uint32_t Mode : 1;
    uint32_t Reserved : 25;
} VAULTIP_INPUT_TOKEN_REGISTER_READ_WORD_2_t;

typedef struct VAULTIP_INPUT_TOKEN_REGISTER_READ_WORD_3_63_s {
    uint32_t Address : 16;
    uint32_t Reserved : 16;
} VAULTIP_INPUT_TOKEN_REGISTER_READ_WORD_3_63_t;

#define VAULTIP_INPUT_TOKEN_REGISTER_READ_MAXIMUM_NUMBER 61
typedef struct VAULTIP_INPUT_TOKEN_REGISTER_READ_s {
    VAULTIP_INPUT_TOKEN_REGISTER_READ_WORD_2_t      dw_02;
    VAULTIP_INPUT_TOKEN_REGISTER_READ_WORD_3_63_t   dw_03_63[VAULTIP_INPUT_TOKEN_REGISTER_READ_MAXIMUM_NUMBER];
} VAULTIP_INPUT_TOKEN_REGISTER_READ_t;

typedef struct VAULTIP_OUTPUT_TOKEN_REGISTER_READ_WORD_1_61_s {
    uint32_t ReadData;
} VAULTIP_OUTPUT_TOKEN_REGISTER_READ_WORD_1_61_t;

typedef struct VAULTIP_OUTPUT_TOKEN_REGISTER_READ_s {
    VAULTIP_OUTPUT_TOKEN_REGISTER_READ_WORD_1_61_t  dw_01_61[VAULTIP_INPUT_TOKEN_REGISTER_READ_MAXIMUM_NUMBER];
} VAULTIP_OUTPUT_TOKEN_REGISTER_READ_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct VAULTIP_INPUT_TOKEN_REGISTER_WRITE_WORD_2_s {
    uint32_t Number : 6;
    uint32_t Mode : 1;
    uint32_t Reserved : 25;
} VAULTIP_INPUT_TOKEN_REGISTER_WRITE_WORD_2_t;

#define VAULTIP_INPUT_TOKEN_REGISTER_WRITE_INCREMENTAL_MAXIMUM_NUMBER 59
#define VAULTIP_INPUT_TOKEN_REGISTER_WRITE_NON_INCREMENTAL_MAXIMUM_NUMBER 20

typedef struct VAULTIP_INPUT_TOKEN_REGISTER_WRITE_ADDRESS_WORD_s {
    uint32_t Address : 16;
    uint32_t Reserved : 16;
} VAULTIP_INPUT_TOKEN_REGISTER_WRITE_ADDRESS_WORD_t;

typedef struct VAULTIP_INPUT_TOKEN_REGISTER_WRITE_INCREMENTAL_s {
    uint32_t Mask;
    struct {
        uint32_t Address : 16;
        uint32_t Reserved : 16;
    };
    uint32_t WriteData[VAULTIP_INPUT_TOKEN_REGISTER_WRITE_INCREMENTAL_MAXIMUM_NUMBER];
} VAULTIP_INPUT_TOKEN_REGISTER_WRITE_INCREMENTAL_t;

typedef struct VAULTIP_INPUT_TOKEN_REGISTER_WRITE_NON_INCREMENTAL_ENTRY_s {
    uint32_t Mask;
    struct {
        uint32_t Address : 16;
        uint32_t Reserved : 16;
    };
    uint32_t WriteData;
} VAULTIP_INPUT_TOKEN_REGISTER_WRITE_NON_INCREMENTAL_ENTRY_t;

typedef struct VAULTIP_INPUT_TOKEN_REGISTER_WRITE_NON_INCREMENTAL_s {
    VAULTIP_INPUT_TOKEN_REGISTER_WRITE_NON_INCREMENTAL_ENTRY_t entries[VAULTIP_INPUT_TOKEN_REGISTER_WRITE_NON_INCREMENTAL_MAXIMUM_NUMBER];
} VAULTIP_INPUT_TOKEN_REGISTER_WRITE_NON_INCREMENTAL_t;

typedef struct VAULTIP_INPUT_TOKEN_REGISTER_WRITE_s {
    VAULTIP_INPUT_TOKEN_REGISTER_WRITE_WORD_2_t      dw_02;
    union {
        VAULTIP_INPUT_TOKEN_REGISTER_WRITE_INCREMENTAL_t incremental;
        VAULTIP_INPUT_TOKEN_REGISTER_WRITE_NON_INCREMENTAL_t non_incremental;
    };
} VAULTIP_INPUT_TOKEN_REGISTER_WRITE_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct VAULTIP_INPUT_TOKEN_TRNG_CONFIGURATION_WORD_2_s {
    uint32_t LST : 1;
    uint32_t RRD : 1;
    uint32_t Reserved : 6;
    uint32_t AutoSeed : 8;
    uint32_t FroBlockKey : 16;
} VAULTIP_INPUT_TOKEN_TRNG_CONFIGURATION_WORD_2_t;

typedef struct VAULTIP_INPUT_TOKEN_TRNG_CONFIGURATION_WORD_3_s {
    uint32_t NoiseBlocks : 5;
    uint32_t Reserved : 1;
    uint32_t Scale : 2;
    uint32_t SampleDiv : 4;
    uint32_t Reserved2 : 4;
    uint32_t SampleCycles : 16;
} VAULTIP_INPUT_TOKEN_TRNG_CONFIGURATION_WORD_3_t;

typedef struct VAULTIP_INPUT_TOKEN_TRNG_CONFIGURATION_s {
    VAULTIP_INPUT_TOKEN_TRNG_CONFIGURATION_WORD_2_t     dw_02;
    VAULTIP_INPUT_TOKEN_TRNG_CONFIGURATION_WORD_3_t     dw_03;
} VAULTIP_INPUT_TOKEN_TRNG_CONFIGURATION_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct VAULTIP_INPUT_TOKEN_PROVISION_HUK_WORD_2_s {
    uint32_t AssetNumber : 5;
    uint32_t Reserved : 3;
    uint32_t AutoSeed : 8;
    uint32_t Size_128bit : 1;
    uint32_t Size_256bit : 1;
    uint32_t Reserved2 : 12;
    uint32_t CRC : 1;
    uint32_t KeyBlob : 1;
} VAULTIP_INPUT_TOKEN_PROVISION_HUK_WORD_2_t;

typedef struct VAULTIP_INPUT_TOKEN_PROVISION_HUK_WORD_3_s {
    uint32_t NoiseBlocks : 8;
    uint32_t SampleDiv : 4;
    uint32_t Reserved : 4;
    uint32_t SampleCycles : 16;
} VAULTIP_INPUT_TOKEN_PROVISION_HUK_WORD_3_t;

typedef struct VAULTIP_INPUT_TOKEN_PROVISION_HUK_WORD_4_s {
    uint32_t OutputDataLength : 10;
    uint32_t Reserved : 6;
    uint32_t AssociatedDataLength : 8;
    uint32_t Reserved2 : 8;
} VAULTIP_INPUT_TOKEN_PROVISION_HUK_WORD_4_t;

typedef struct VAULTIP_INPUT_TOKEN_PROVISION_HUK_WORD_7_8_s {
    uint8_t AssociatedData[4];
} VAULTIP_INPUT_TOKEN_PROVISION_HUK_WORD_7_8_t;

typedef struct VAULTIP_INPUT_TOKEN_PROVISION_HUK_s {
    VAULTIP_INPUT_TOKEN_PROVISION_HUK_WORD_2_t          dw_02;
    VAULTIP_INPUT_TOKEN_PROVISION_HUK_WORD_3_t          dw_03;
    VAULTIP_INPUT_TOKEN_PROVISION_HUK_WORD_4_t          dw_04;
    VAULTIP_INPUT_TOKEN_OUTPUT_DATA_ADDRESS_LO_t        dw_05;
    VAULTIP_INPUT_TOKEN_OUTPUT_DATA_ADDRESS_HI_t        dw_06;
    VAULTIP_INPUT_TOKEN_PROVISION_HUK_WORD_7_8_t        dw_07;
    VAULTIP_INPUT_TOKEN_PROVISION_HUK_WORD_7_8_t        dw_08;
} VAULTIP_INPUT_TOKEN_PROVISION_HUK_t;

typedef struct VAULTIP_OUTPUT_TOKEN_PROVISION_HUK_WORD_1_s {
    uint32_t KeyBlobLength : 10;
    uint32_t Reserved : 22;
} VAULTIP_OUTPUT_TOKEN_PROVISION_HUK_WORD_1_t;

typedef struct VAULTIP_OUTPUT_TOKEN_PROVISION_HUK_s {
    VAULTIP_OUTPUT_TOKEN_PROVISION_HUK_WORD_1_t     dw_01;
} VAULTIP_OUTPUT_TOKEN_PROVISION_HUK_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef union VAULTIP_INPUT_TOKEN_s {
    uint32_t dw[64];
    struct {
        VAULTIP_INPUT_TOKEN_WORD_0_t                    dw_00;
        VAULTIP_INPUT_TOKEN_WORD_1_t                    dw_01;
        union {
            VAULTIP_INPUT_TOKEN_NOP_t                   nop;
            VAULTIP_INPUT_TOKEN_ENCRYPTION_t            encryption;
            VAULTIP_INPUT_TOKEN_HASH_t                  hash;
            VAULTIP_INPUT_TOKEN_MAC_t                   mac;
            VAULTIP_INPUT_TOKEN_REGISTER_READ_t         register_read;
            VAULTIP_INPUT_TOKEN_REGISTER_WRITE_t        register_write;
            VAULTIP_INPUT_TOKEN_TRNG_CONFIGURATION_t    trng_configuration;
            VAULTIP_INPUT_TOKEN_PROVISION_HUK_t         provision_huk;
        };
    };
} VAULTIP_INPUT_TOKEN_t;

typedef union VAULTIP_OUTPUT_TOKEN_s {
    uint32_t dw[64];
    struct {
        VAULTIP_OUTPUT_TOKEN_WORD_0_t                   dw_00;
        union { 
            VAULTIP_OUTPUT_TOKEN_ENCRYPTION_t           encryption;
            VAULTIP_OUTPUT_TOKEN_HASH_t                 hash;
            VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_t          system_info;
            VAULTIP_OUTPUT_TOKEN_REGISTER_READ_t        register_read;
            VAULTIP_OUTPUT_TOKEN_PROVISION_HUK_t        provision_huk;
        };
    };
} VAULTIP_OUTPUT_TOKEN_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#define VAULTIP_TOKEN_OPCODE_NOP                    0x0
#define VAULTIP_TOKEN_OPCODE_ENCRYPTION             0x1
#define VAULTIP_TOKEN_OPCODE_HASH                   0x2
#define VAULTIP_TOKEN_OPCODE_MAC                    0x3
#define VAULTIP_TOKEN_OPCODE_TRNG                   0x4
#define VAULTIP_TOKEN_OPCODE_SPECIAL_FUNCTIONS      0x5
#define VAULTIP_TOKEN_OPCODE_AES_WRAP               0x6
#define VAULTIP_TOKEN_OPCODE_ASSET_MANAGEMENT       0x7
#define VAULTIP_TOKEN_OPCODE_AUTHENTICATION_UNLOCK  0x8
#define VAULTIP_TOKEN_OPCODE_PUBLIC_KEY_OPERATION   0x9
#define VAULTIP_TOKEN_OPCODE_EMMC_ACCESS            0xA
#define VAULTIP_TOKEN_OPCODE_SERVICE                0xE
#define VAULTIP_TOKEN_OPCODE_SYSTEM                 0xF

#define VAULTIP_TOKEN_TRNG_SUBCODE_GET_RANDOM_NUMBER    0x0
#define VAULTIP_TOKEN_TRNG_SUBCODE_CONFIGURE_NRBG       0x1
#define VAULTIP_TOKEN_TRNG_SUBCODE_VERIFY_DBRG          0x2
#define VAULTIP_TOKEN_TRNG_SUBCODE_VERIFY_NRBG          0x3

#define VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_ASSET_SEARCH                 0x0
#define VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_ASSET_CREATE                 0x1
#define VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_ASSET_LOAD                   0x2
#define VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_ASSET_DELETE                 0x3
#define VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_PUBLIC_DATA_READ             0x4
#define VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_MONOTONIC_COUNTER_READ       0x5
#define VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_MONOTONIC_COUNTER_INCREMENT  0x6
#define VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_OTP_DATA_WRITE               0x7
#define VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_SECURE_TIMER                 0x8
#define VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_PROVISION_RANDOM_HUK         0x0

#define VAULTIP_TOKEN_SYSTEM_SUBCODE_SYSTEM_INFORMATION         0x0
#define VAULTIP_TOKEN_SYSTEM_SUBCODE_SELF_TEST                  0x1
#define VAULTIP_TOKEN_SYSTEM_SUBCODE_RESET                      0x2
#define VAULTIP_TOKEN_SYSTEM_SUBCODE_DEFINE_USERS               0x3
#define VAULTIP_TOKEN_SYSTEM_SUBCODE_SLEEP_MODE                 0x4
#define VAULTIP_TOKEN_SYSTEM_SUBCODE_RESUME_FROM_SLEEP_MODE     0x5
#define VAULTIP_TOKEN_SYSTEM_SUBCODE_HIBERNATION                0x6
#define VAULTIP_TOKEN_SYSTEM_SUBCODE_RESUME_FROM_HIBERNATION    0x7
#define VAULTIP_TOKEN_SYSTEM_SUBCODE_SET_SYSTEM_TIME            0x8
#define VAULTIP_TOKEN_SYSTEM_SUBCODE_RAM_FIRMWARE_BOOT          0x9

#define VAULTIP_TOKEN_SERVICE_SUBCODE_REGISTER_READ                         0x0
#define VAULTIP_TOKEN_SERVICE_SUBCODE_REGISTER_WRITE                        0x1
#define VAULTIP_TOKEN_SERVICE_SUBCODE_CLOCK_SWITCH                          0x2
#define VAULTIP_TOKEN_SERVICE_SUBCODE_ZEROISE_OUTPUT_MAILBOX                0x3
#define VAULTIP_TOKEN_SERVICE_SUBCODE_SELECT_ONE_TIME_PROGRAMMABLE_ZEROIZE  0x4
#define VAULTIP_TOKEN_SERVICE_SUBCODE_ZEROIZE_ONE_TIME_PROGRAMMABLE         0x5

#endif
