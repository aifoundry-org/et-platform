/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef __VAULTIP_SW_H__
#define __VAULTIP_SW_H__

#include <stdint.h>
#include <assert.h>

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
    uint32_t GCM_Mode : 2;
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

typedef union VAULTIP_INPUT_TOKEN_ENCRYPTION_WORD_24_17_u {
    uint8_t Key[4];
    uint32_t Key32;
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

typedef struct VAULTIP_INPUT_TOKEN_MAC_WORD_7_s {
    uint32_t MAC_AS_ID : 32;
} VAULTIP_INPUT_TOKEN_MAC_WORD_7_t;

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
    VAULTIP_INPUT_TOKEN_MAC_WORD_7_t            dw_07;
    VAULTIP_INPUT_TOKEN_MAC_WORD_8_23_t         dw_23_08[16];
    VAULTIP_INPUT_TOKEN_MAC_WORD_24_t           dw_24;
    VAULTIP_INPUT_TOKEN_MAC_WORD_25_t           dw_25;
    uint32_t                                    reserved[2];
    VAULTIP_INPUT_TOKEN_MAC_WORD_28_59_t        dw_59_28[32];
} VAULTIP_INPUT_TOKEN_MAC_t;

typedef union VAULTIP_OUTPUT_TOKEN_MAC_WORD_2_17_u {
    uint32_t u32;
    uint8_t u8[4];
} VAULTIP_OUTPUT_TOKEN_MAC_WORD_2_17_t;

typedef struct VAULTIP_OUTPUT_TOKEN_MAC_s {
    uint32_t                                    dw_01;
    VAULTIP_OUTPUT_TOKEN_MAC_WORD_2_17_t       dw_02_17[16];
} VAULTIP_OUTPUT_TOKEN_MAC_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_2_s {
    uint32_t Policy_31_00;
} VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_2_t;

typedef struct VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_3_s {
    uint32_t Policy_63_32;
} VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_3_t;

typedef struct VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_s {
    uint32_t DataLength : 10;
    uint32_t Reserved : 14;
    uint32_t RemoveSecure : 1;
    uint32_t AllHosts : 1;
    uint32_t Reserved2 : 2;
    uint32_t NoLdLifetime : 1;
    uint32_t Relative : 1;
    uint32_t LifetimeUse : 2;
} VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t;

typedef struct VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_5_s {
    uint32_t Lifetime;
} VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_5_t;

typedef struct VAULTIP_INPUT_TOKEN_ASSET_CREATE_s {
    VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_2_t   dw_02;
    VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_3_t   dw_03;
    VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t   dw_04;
    VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_5_t   dw_05;
} VAULTIP_INPUT_TOKEN_ASSET_CREATE_t;

typedef struct VAULTIP_OUTPUT_TOKEN_ASSET_CREATE_WORD_1_s {
    uint32_t AS_ID;
} VAULTIP_OUTPUT_TOKEN_ASSET_CREATE_WORD_1_t;

typedef struct VAULTIP_OUTPUT_TOKEN_ASSET_CREATE_s {
    VAULTIP_OUTPUT_TOKEN_ASSET_CREATE_WORD_1_t  dw_01;
} VAULTIP_OUTPUT_TOKEN_ASSET_CREATE_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_2_s {
    uint32_t AS_ID : 32;
} VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_2_t;

typedef struct VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_3_s {
    uint32_t InputDataLength : 10;
    uint32_t Reserved : 4;
    uint32_t Counter : 1;
    uint32_t RFC5869 : 1;
    uint32_t AssociatedDataLength : 8;
    uint32_t Derive : 1;
    uint32_t Random : 1;
    uint32_t Import : 1;
    uint32_t PlainText : 1;
    uint32_t AESUnwrap : 1;
    uint32_t Reserved2 : 2;
    uint32_t KeyBlob : 1;
} VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_3_t;

typedef struct VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_4_s {
    uint32_t InputDataAddress_31_00 : 32;
} VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_4_t;

typedef struct VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_5_s {
    uint32_t InputDataAddress_63_32 : 32;
} VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_5_t;

typedef struct VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_6_s {
    uint32_t OutputDataAddress_31_00 : 32;
} VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_6_t;

typedef struct VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_7_s {
    uint32_t OutputDataAddress_63_32 : 32;
} VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_7_t;

typedef struct VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_8_s {
    uint32_t OutputDataLength : 11;
    uint32_t Reserved : 21;
} VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_8_t;

typedef struct VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_9_s {
    uint32_t Key_AS_ID : 32;
} VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_9_t;

typedef struct VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_10_63_s {
    uint8_t AssociatedData[4];
} VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_10_63_t;

typedef struct VAULTIP_INPUT_TOKEN_ASSET_LOAD_s {
    VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_2_t     dw_02;
    VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_3_t     dw_03;
    VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_4_t     dw_04;
    VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_5_t     dw_05;
    VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_6_t     dw_06;
    VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_7_t     dw_07;
    VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_8_t     dw_08;
    VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_9_t     dw_09;
    VAULTIP_INPUT_TOKEN_ASSET_LOAD_WORD_10_63_t dw_10_63[54];
} VAULTIP_INPUT_TOKEN_ASSET_LOAD_t;

typedef struct VAULTIP_OUTPUT_TOKEN_ASSET_LOAD_WORD_1_s {
    uint32_t KeyBlobLength : 10;
    uint32_t Reserved : 22;
} VAULTIP_OUTPUT_TOKEN_ASSET_LOAD_WORD_1_t;

typedef struct VAULTIP_OUTPUT_TOKEN_ASSET_LOAD_s {
    VAULTIP_OUTPUT_TOKEN_ASSET_LOAD_WORD_1_t    dw_01;
} VAULTIP_OUTPUT_TOKEN_ASSET_LOAD_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct VAULTIP_INPUT_TOKEN_STATIC_ASSET_SEARCH_WORD_4_s {
    uint32_t Reserved : 16;
    uint32_t AssetNumber : 6;
    uint32_t Reserved2 : 10;
} VAULTIP_INPUT_TOKEN_STATIC_ASSET_SEARCH_WORD_4_t;

typedef struct VAULTIP_INPUT_TOKEN_STATIC_ASSET_SEARCH_s {
    uint32_t reserved[2];
    VAULTIP_INPUT_TOKEN_STATIC_ASSET_SEARCH_WORD_4_t     dw_04;
} VAULTIP_INPUT_TOKEN_STATIC_ASSET_SEARCH_t;

typedef struct VAULTIP_OUTPUT_TOKEN_STATIC_ASSET_SEARCH_WORD_1_s {
    uint32_t AS_ID : 32;
} VAULTIP_OUTPUT_TOKEN_STATIC_ASSET_SEARCH_WORD_1_t;

typedef struct VAULTIP_OUTPUT_TOKEN_STATIC_ASSET_SEARCH_WORD_2_s {
    uint32_t DataLength : 10;
    uint32_t Reserved : 22;
} VAULTIP_OUTPUT_TOKEN_STATIC_ASSET_SEARCH_WORD_2_t;

typedef struct VAULTIP_OUTPUT_TOKEN_STATIC_ASSET_SEARCH_s {
    VAULTIP_OUTPUT_TOKEN_STATIC_ASSET_SEARCH_WORD_1_t    dw_01;
    VAULTIP_OUTPUT_TOKEN_STATIC_ASSET_SEARCH_WORD_2_t    dw_02;
} VAULTIP_OUTPUT_TOKEN_STATIC_ASSET_SEARCH_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct VAULTIP_INPUT_TOKEN_ASSET_DELETE_WORD_2_s {
    uint32_t AS_ID : 32;
} VAULTIP_INPUT_TOKEN_ASSET_DELETE_WORD_2_t;

typedef struct VAULTIP_INPUT_TOKEN_ASSET_DELETE_s {
    VAULTIP_INPUT_TOKEN_ASSET_DELETE_WORD_2_t     dw_02;
} VAULTIP_INPUT_TOKEN_ASSET_DELETE_t;

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
    VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_WORD_1_t       dw_01;
    VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_WORD_2_t       dw_02;
    VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_WORD_3_t       dw_03;
    VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_WORD_4_t       dw_04;
    VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_WORD_5_t       dw_05;
} VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_2_s {
    uint32_t Command : 8;
    uint32_t OutputReq : 8;
    uint32_t Nwords : 8;
    uint32_t Mwords : 8;
} VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_2_t;

typedef struct VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_3_s {
    uint32_t AddInputLen : 8;
    uint32_t OtherLen : 8;
    uint32_t Reserved : 15;
    uint32_t SvShrdScrt : 1;
} VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_3_t;

typedef struct VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_4_s {
    uint32_t KeyAssetRef;
} VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_4_t;

typedef struct VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_5_s {
    uint32_t ParamAssetRef;
} VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_5_t;

typedef struct VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_6_s {
    uint32_t IOAssetRef;
} VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_6_t;

typedef struct VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_7_s {
    uint32_t InputDataSize : 12;
    uint32_t Reserved : 4;
    uint32_t OutputDataSize_or_SigDataSize : 12;
    uint32_t Reserved2 : 4;
} VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_7_t;

typedef union VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_10_u {
    uint32_t OutputDataAddress_31_00;
    uint32_t SigDataAddress_31_00;
} VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_10_t;

typedef union VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_11_u {
    uint32_t OutputDataAddress_63_32;
    uint32_t SigDataAddress_63_32;
} VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_11_t;

typedef union VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_12_63_u {
    uint32_t AssetReferences[52];
    uint8_t AdditionalInput[52 * sizeof(uint32_t)];
    uint32_t HashDataLength;
} VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORDS_12_63_t;

typedef struct VAULTIP_INPUT_TOKEN_PUBLIC_KEY_s {
    VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_2_t         dw_02;
    VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_3_t         dw_03;
    VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_4_t         dw_04;
    VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_5_t         dw_05;
    VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_6_t         dw_06;
    VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_7_t         dw_07;
    VAULTIP_INPUT_TOKEN_INPUT_DATA_ADDRESS_LO_t     dw_08;
    VAULTIP_INPUT_TOKEN_INPUT_DATA_ADDRESS_HI_t     dw_09;
    VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_10_t        dw_10;
    VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORD_11_t        dw_11;
    VAULTIP_INPUT_TOKEN_PUBLIC_KEY_WORDS_12_63_t    dw_12_63;
} VAULTIP_INPUT_TOKEN_PUBLIC_KEY_t;

typedef struct VAULTIP_OUTPUT_TOKEN_PUBLIC_KEY_WORD_1_s {
    uint32_t AS_ID;
} VAULTIP_OUTPUT_TOKEN_PUBLIC_KEY_WORD_1_t;

typedef struct VAULTIP_OUTPUT_TOKEN_PUBLIC_KEY_s {
    VAULTIP_OUTPUT_TOKEN_PUBLIC_KEY_WORD_1_t  dw_01;
} VAULTIP_OUTPUT_TOKEN_PUBLIC_KEY_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct VAULTIP_INPUT_TOKEN_PUBLIC_DATA_READ_WORD_2_s {
    uint32_t AS_ID : 32;
} VAULTIP_INPUT_TOKEN_PUBLIC_DATA_READ_WORD_2_t;

typedef struct VAULTIP_INPUT_TOKEN_PUBLIC_DATA_READ_WORD_3_s {
    uint32_t OutputDataLength : 10;
    uint32_t Reserved : 22;
} VAULTIP_INPUT_TOKEN_PUBLIC_DATA_READ_WORD_3_t;

typedef struct VAULTIP_INPUT_TOKEN_PUBLIC_DATA_READ_WORD_4_s {
    uint32_t OutputDataAddress_31_00;
} VAULTIP_INPUT_TOKEN_PUBLIC_DATA_READ_WORD_4_t;

typedef struct VAULTIP_INPUT_TOKEN_PUBLIC_DATA_READ_WORD_5_s {
    uint32_t OutputDataAddress_63_32;
} VAULTIP_INPUT_TOKEN_PUBLIC_DATA_READ_WORD_5_t;

typedef struct VAULTIP_INPUT_TOKEN_PUBLIC_DATA_READ_s {
    VAULTIP_INPUT_TOKEN_PUBLIC_DATA_READ_WORD_2_t      dw_02;
    VAULTIP_INPUT_TOKEN_PUBLIC_DATA_READ_WORD_3_t      dw_03;
    VAULTIP_INPUT_TOKEN_PUBLIC_DATA_READ_WORD_4_t      dw_04;
    VAULTIP_INPUT_TOKEN_PUBLIC_DATA_READ_WORD_5_t      dw_05;
} VAULTIP_INPUT_TOKEN_PUBLIC_DATA_READ_t;

typedef struct VAULTIP_OUTPUT_TOKEN_PUBLIC_DATA_READ_WORD_1_s {
    uint32_t DataLength : 10;
    uint32_t Reserved : 22;
} VAULTIP_OUTPUT_TOKEN_PUBLIC_DATA_READ_WORD_1_t;

typedef struct VAULTIP_OUTPUT_TOKEN_PUBLIC_DATA_READ_s {
    VAULTIP_OUTPUT_TOKEN_PUBLIC_DATA_READ_WORD_1_t  dw_01;
} VAULTIP_OUTPUT_TOKEN_PUBLIC_DATA_READ_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_READ_WORD_2_s {
    uint32_t AS_ID : 32;
} VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_READ_WORD_2_t;

typedef struct VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_READ_WORD_3_s {
    uint32_t OutputDataLength : 10;
    uint32_t Reserved : 22;
} VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_READ_WORD_3_t;

typedef struct VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_READ_WORD_4_s {
    uint32_t OutputDataAddress_31_00;
} VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_READ_WORD_4_t;

typedef struct VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_READ_WORD_5_s {
    uint32_t OutputDataAddress_63_32;
} VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_READ_WORD_5_t;

typedef struct VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_READ_s {
    VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_READ_WORD_2_t      dw_02;
    VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_READ_WORD_3_t      dw_03;
    VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_READ_WORD_4_t      dw_04;
    VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_READ_WORD_5_t      dw_05;
} VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_READ_t;

typedef struct VAULTIP_OUTPUT_TOKEN_MONOTONIC_COUNTER_READ_WORD_1_s {
    uint32_t DataLength : 10;
    uint32_t Reserved : 22;
} VAULTIP_OUTPUT_TOKEN_MONOTONIC_COUNTER_READ_WORD_1_t;

typedef struct VAULTIP_OUTPUT_TOKEN_MONOTONIC_COUNTER_READ_s {
    VAULTIP_OUTPUT_TOKEN_MONOTONIC_COUNTER_READ_WORD_1_t  dw_01;
} VAULTIP_OUTPUT_TOKEN_MONOTONIC_COUNTER_READ_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_INCREMENT_WORD_2_s {
    uint32_t AS_ID : 32;
} VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_INCREMENT_WORD_2_t;

typedef struct VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_INCREMENT_s {
    VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_INCREMENT_WORD_2_t      dw_02;
} VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_INCREMENT_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct VAULTIP_INPUT_TOKEN_OTP_DATA_WRITE_WORD_2_s {
    uint32_t AssetNumber : 5;
    uint32_t Reserved : 11;
    uint32_t PolicyNumber : 5;
    uint32_t Reserved2 : 10;
    uint32_t CRC : 1;
} VAULTIP_INPUT_TOKEN_OTP_DATA_WRITE_WORD_2_t;

typedef struct VAULTIP_INPUT_TOKEN_OTP_DATA_WRITE_WORD_3_s {
    uint32_t InputDataLength : 10;
    uint32_t Reserved : 6;
    uint32_t AssociatedDataLength : 8;
    uint32_t Reserved2 : 8;
} VAULTIP_INPUT_TOKEN_OTP_DATA_WRITE_WORD_3_t;

typedef struct VAULTIP_INPUT_TOKEN_OTP_DATA_WRITE_WORD_4_s {
    uint32_t InputDataAddress_31_00 : 32;
} VAULTIP_INPUT_TOKEN_OTP_DATA_WRITE_WORD_4_t;

typedef struct VAULTIP_INPUT_TOKEN_OTP_DATA_WRITE_WORD_5_s {
    uint32_t InputDataAddress_63_32 : 32;
} VAULTIP_INPUT_TOKEN_OTP_DATA_WRITE_WORD_5_t;

typedef struct VAULTIP_INPUT_TOKEN_OTP_DATA_WRITE_WORDS_6_63_s {
    uint8_t AssociatedData[4];
} VAULTIP_INPUT_TOKEN_OTP_DATA_WRITE_WORDS_6_63_t;

typedef struct VAULTIP_INPUT_TOKEN_OTP_DATA_WRITE_s {
    VAULTIP_INPUT_TOKEN_OTP_DATA_WRITE_WORD_2_t      dw_02;
    VAULTIP_INPUT_TOKEN_OTP_DATA_WRITE_WORD_3_t      dw_03;
    VAULTIP_INPUT_TOKEN_OTP_DATA_WRITE_WORD_4_t      dw_04;
    VAULTIP_INPUT_TOKEN_OTP_DATA_WRITE_WORD_5_t      dw_05;
    VAULTIP_INPUT_TOKEN_OTP_DATA_WRITE_WORDS_6_63_t  dw_63_06[58];
} VAULTIP_INPUT_TOKEN_OTP_DATA_WRITE_t;

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

typedef struct VAULTIP_INPUT_TOKEN_TRNG_GET_RANDOM_NUMBER_WORD_2_s {
    uint32_t Size : 16;
    uint32_t RawKey : 16;
} VAULTIP_INPUT_TOKEN_TRNG_GET_RANDOM_NUMBER_WORD_2_t;

typedef struct VAULTIP_INPUT_TOKEN_TRNG_GET_RANDOM_NUMBER_WORD_3_s {
    uint32_t OutputDataAddress_31_00;
} VAULTIP_INPUT_TOKEN_TRNG_GET_RANDOM_NUMBER_WORD_3_t;

typedef struct VAULTIP_INPUT_TOKEN_TRNG_GET_RANDOM_NUMBER_WORD_4_s {
    uint32_t OutputDataAddress_63_32;
} VAULTIP_INPUT_TOKEN_TRNG_GET_RANDOM_NUMBER_WORD_4_t;

typedef struct VAULTIP_INPUT_TOKEN_TRNG_GET_RANDOM_NUMBER_s {
    VAULTIP_INPUT_TOKEN_TRNG_GET_RANDOM_NUMBER_WORD_2_t     dw_02;
    VAULTIP_INPUT_TOKEN_TRNG_GET_RANDOM_NUMBER_WORD_3_t     dw_03;
    VAULTIP_INPUT_TOKEN_TRNG_GET_RANDOM_NUMBER_WORD_4_t     dw_04;
} VAULTIP_INPUT_TOKEN_TRNG_GET_RANDOM_NUMBER_t;

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

typedef struct VAULTIP_INPUT_TOKEN_CLOCK_SWITCH_WORD_2_s {
    uint32_t EncryptClk_ON : 1;
    uint32_t HashClk_ON : 1;
    uint32_t TRNG_Clk_ON : 1;
    uint32_t PKCP_Clk_ON : 1;
    uint32_t CM_Clk_ON : 1;
    uint32_t Reserved : 3;
    uint32_t CE3_Clk_ON : 1;
    uint32_t CE4_Clk_ON : 1;
    uint32_t CE5_Clk_ON : 1;
    uint32_t CE6_Clk_ON : 1;
    uint32_t CE7_Clk_ON : 1;
    uint32_t CE8_Clk_ON : 1;
    uint32_t CE9_Clk_ON : 1;
    uint32_t CE10_Clk_ON : 1;
    uint32_t EncryptClk_OFF : 1;
    uint32_t HashClk_OFF : 1;
    uint32_t TRNG_Clk_OFF : 1;
    uint32_t PKCP_Clk_OFF : 1;
    uint32_t Reserved2 : 4;
    uint32_t CE3_Clk_OFF : 1;
    uint32_t CE4_Clk_OFF : 1;
    uint32_t CE5_Clk_OFF : 1;
    uint32_t CE6_Clk_OFF : 1;
    uint32_t CE7_Clk_OFF : 1;
    uint32_t CE8_Clk_OFF : 1;
    uint32_t CE9_Clk_OFF : 1;
    uint32_t CE10_Clk_OFF : 1;
} VAULTIP_INPUT_TOKEN_CLOCK_SWITCH_WORD_2_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef union VAULTIP_INPUT_TOKEN_s {
    uint32_t dw[64];
    struct {
        VAULTIP_INPUT_TOKEN_WORD_0_t                                dw_00;
        VAULTIP_INPUT_TOKEN_WORD_1_t                                dw_01;
        union {
            VAULTIP_INPUT_TOKEN_NOP_t                               nop;
            VAULTIP_INPUT_TOKEN_ENCRYPTION_t                        encryption;
            VAULTIP_INPUT_TOKEN_HASH_t                              hash;
            VAULTIP_INPUT_TOKEN_MAC_t                               mac;
            VAULTIP_INPUT_TOKEN_PUBLIC_KEY_t                        public_key;
            VAULTIP_INPUT_TOKEN_REGISTER_READ_t                     register_read;
            VAULTIP_INPUT_TOKEN_REGISTER_WRITE_t                    register_write;
            VAULTIP_INPUT_TOKEN_TRNG_CONFIGURATION_t                trng_configuration;
            VAULTIP_INPUT_TOKEN_TRNG_GET_RANDOM_NUMBER_t            trn_get_random_number;
            VAULTIP_INPUT_TOKEN_ASSET_CREATE_t                      asset_create;
            VAULTIP_INPUT_TOKEN_STATIC_ASSET_SEARCH_t               static_asset_search;
            VAULTIP_INPUT_TOKEN_ASSET_LOAD_t                        asset_load;
            VAULTIP_INPUT_TOKEN_ASSET_DELETE_t                      asset_delete;
            VAULTIP_INPUT_TOKEN_PUBLIC_DATA_READ_t                  public_data_read;
            VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_READ_t            monotonic_counter_read;
            VAULTIP_INPUT_TOKEN_MONOTONIC_COUNTER_INCREMENT_t       monotonic_counter_increment;
            VAULTIP_INPUT_TOKEN_OTP_DATA_WRITE_t                    otp_data_write;
            VAULTIP_INPUT_TOKEN_PROVISION_HUK_t                     provision_huk;
            VAULTIP_INPUT_TOKEN_CLOCK_SWITCH_WORD_2_t               clock_switch;
        };
    };
} VAULTIP_INPUT_TOKEN_t;

typedef union VAULTIP_OUTPUT_TOKEN_s {
    uint32_t dw[64];
    struct {
        VAULTIP_OUTPUT_TOKEN_WORD_0_t                               dw_00;
        union { 
            VAULTIP_OUTPUT_TOKEN_ENCRYPTION_t                       encryption;
            VAULTIP_OUTPUT_TOKEN_HASH_t                             hash;
            VAULTIP_OUTPUT_TOKEN_MAC_t                              mac;
            VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_t                      system_info;
            VAULTIP_OUTPUT_TOKEN_PUBLIC_KEY_t                       public_key;
            VAULTIP_OUTPUT_TOKEN_REGISTER_READ_t                    register_read;
            VAULTIP_OUTPUT_TOKEN_ASSET_CREATE_t                     asset_create;
            VAULTIP_OUTPUT_TOKEN_STATIC_ASSET_SEARCH_t              static_asset_search;
            VAULTIP_OUTPUT_TOKEN_ASSET_LOAD_t                       asset_load;
            VAULTIP_OUTPUT_TOKEN_PUBLIC_DATA_READ_t                 public_data_read;
            VAULTIP_OUTPUT_TOKEN_MONOTONIC_COUNTER_READ_t           monotonic_counter_read;
            VAULTIP_OUTPUT_TOKEN_PROVISION_HUK_t                    provision_huk;
        };
    };
} VAULTIP_OUTPUT_TOKEN_t;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct VAULTIP_FIRMWARE_RAM_TOKEN_WORD_0_s {
    uint32_t TokenID : 16;
    uint32_t Reserved : 8;
    uint32_t OpCode : 4;
    uint32_t SubCode : 4;
} VAULTIP_FIRMWARE_RAM_TOKEN_WORD_0_t;

typedef struct VAULTIP_FIRMWARE_RAM_TOKEN_WORD_1_s {
    uint32_t ImageType : 24;
    uint32_t ImageVersionNumber : 8;
} VAULTIP_FIRMWARE_RAM_TOKEN_WORD_1_t;

typedef struct VAULTIP_FIRMWARE_RAM_TOKEN_WORD_3_18_s {
    uint8_t Signature[4];
} VAULTIP_FIRMWARE_RAM_TOKEN_WORD_3_18_t;

typedef struct VAULTIP_FIRMWARE_RAM_TOKEN_WORD_19_28_s {
    uint8_t WrappedKey[4];
} VAULTIP_FIRMWARE_RAM_TOKEN_WORD_19_28_t;

typedef struct VAULTIP_FIRMWARE_RAM_TOKEN_WORD_29_44_s {
    uint8_t PublicKey[4];
} VAULTIP_FIRMWARE_RAM_TOKEN_WORD_29_44_t;

typedef struct VAULTIP_FIRMWARE_RAM_TOKEN_WORD_45_48_s {
    uint8_t IV[4];
} VAULTIP_FIRMWARE_RAM_TOKEN_WORD_45_48_t;

typedef struct VAULTIP_FIRMWARE_RAM_TOKEN_WORD_49_s {
    uint32_t ImageSize : 17;
    uint32_t Reserved : 15;
} VAULTIP_FIRMWARE_RAM_TOKEN_WORD_49_t;

typedef union VAULTIP_FIRMWARE_RAM_TOKEN_s {
    uint32_t dw[64];
    struct {
        VAULTIP_FIRMWARE_RAM_TOKEN_WORD_0_t             dw_00;
        VAULTIP_FIRMWARE_RAM_TOKEN_WORD_1_t             dw_01;
        uint32_t reserved;
        VAULTIP_FIRMWARE_RAM_TOKEN_WORD_3_18_t          dw_03_18[16];
        VAULTIP_FIRMWARE_RAM_TOKEN_WORD_19_28_t         dw_19_28[10];
        VAULTIP_FIRMWARE_RAM_TOKEN_WORD_29_44_t         dw_29_44[16];
        VAULTIP_FIRMWARE_RAM_TOKEN_WORD_45_48_t         dw_45_48[4];
        VAULTIP_FIRMWARE_RAM_TOKEN_WORD_49_t            dw_49;
        uint32_t reserved2[14];
    };
} VAULTIP_FIRMWARE_RAM_TOKEN_t;

static_assert(256 == sizeof(VAULTIP_FIRMWARE_RAM_TOKEN_t), "sizeof(VAULTIP_FIRMWARE_RAM_TOKEN_s) is not 256!");

#define VAULTIP_FIRMWARE_IMAGE_TYPE_FWp 0x705746 // 'FWp' // plain text image, only allowed if VaultIP OTP is all zero
#define VAULTIP_FIRMWARE_IMAGE_TYPE_FWw 0x775746 // 'FWw' // encrypted image
#define VAULTIP_FIRMWARE_IMAGE_VERSION_NUMBER 2 // first released version number

#define VAULTIP_FIRMWARE_HEADER_SIZE 256
#define VAULTIP_FIRMWARE_IMAGE_MAX_FILE_SIZE (VAULTIP_MAXIMUM_FIRMWARE_SIZE + VAULTIP_FIRMWARE_HEADER_SIZE)

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
#define VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_PROVISION_RANDOM_HUK         0x9

#define VAULTIP_TOKEN_SYSTEM_SUBCODE_SYSTEM_INFORMATION         0x0
#define VAULTIP_TOKEN_SYSTEM_SUBCODE_SELF_TEST                  0x1
#define VAULTIP_TOKEN_SYSTEM_SUBCODE_RESET                      0x2
#define VAULTIP_TOKEN_SYSTEM_SUBCODE_DEFINE_USERS               0x3
#define VAULTIP_TOKEN_SYSTEM_SUBCODE_SLEEP_MODE                 0x4
#define VAULTIP_TOKEN_SYSTEM_SUBCODE_RESUME_FROM_SLEEP_MODE     0x5
#define VAULTIP_TOKEN_SYSTEM_SUBCODE_HIBERNATION                0x6
#define VAULTIP_TOKEN_SYSTEM_SUBCODE_RESUME_FROM_HIBERNATION    0x7
#define VAULTIP_TOKEN_SYSTEM_SUBCODE_SET_SYSTEM_TIME            0x8
#define VAULTIP_TOKEN_SYSTEM_SUBCODE_RAM_FIRMWARE_BOOT          0xC

#define VAULTIP_TOKEN_PUBLIC_KEY_SUBCODE_DOES_NOT_USE_ASSETS    0x0
#define VAULTIP_TOKEN_PUBLIC_KEY_SUBCODE_USES_ASSETS            0x1

#define VAULTIP_TOKEN_SERVICE_SUBCODE_REGISTER_READ             0x0
#define VAULTIP_TOKEN_SERVICE_SUBCODE_REGISTER_WRITE            0x1
#define VAULTIP_TOKEN_SERVICE_SUBCODE_CLOCK_SWITCH              0x2
#define VAULTIP_TOKEN_SERVICE_SUBCODE_ZEROISE_OUTPUT_MAILBOX    0x3
#define VAULTIP_TOKEN_SERVICE_SUBCODE_SELECT_OTP_ZEROIZE        0x4
#define VAULTIP_TOKEN_SERVICE_SUBCODE_ZEROIZE_OTP               0x5

#define VAULTIP_ENCRYPT_ALGORITHM_AES    0x0

#define VAULTIP_ENCRYPT_MODE_ECB    0x0
#define VAULTIP_ENCRYPT_MODE_CBC    0x1
#define VAULTIP_ENCRYPT_MODE_CTR    0x2
#define VAULTIP_ENCRYPT_MODE_ICM    0x3
#define VAULTIP_ENCRYPT_MODE_F8     0x4
#define VAULTIP_ENCRYPT_MODE_CCM    0x5
#define VAULTIP_ENCRYPT_MODE_XTS    0x6
#define VAULTIP_ENCRYPT_MODE_GCM    0x7

#define VAULTIP_ENCRYPT_GCM_MODE_GHASH          0x0
#define VAULTIP_ENCRYPT_GCM_MODE_GCM_Y0_0       0x1
#define VAULTIP_ENCRYPT_GCM_MODE_GCM_Y0_CALC    0x2
#define VAULTIP_ENCRYPT_GCM_MODE_GCM_AUTO       0x3

#define VAULTIP_ENCRYPT_KEY_LENGTH_128          0x1
#define VAULTIP_ENCRYPT_KEY_LENGTH_192          0x2
#define VAULTIP_ENCRYPT_KEY_LENGTH_256          0x3

#define VAULTIP_HASH_ALGORITHM_SHA_1    0x1
#define VAULTIP_HASH_ALGORITHM_SHA_224  0x2
#define VAULTIP_HASH_ALGORITHM_SHA_256  0x3
#define VAULTIP_HASH_ALGORITHM_SHA_384  0x4
#define VAULTIP_HASH_ALGORITHM_SHA_512  0x5

#define VAULTIP_HASH_MODE_INITIAL_FINAL         0x0
#define VAULTIP_HASH_MODE_CONTINUED_FINAL       0x1
#define VAULTIP_HASH_MODE_INITIAL_NOT_FINAL     0x2
#define VAULTIP_HASH_MODE_CONTINUED_NOT_FINAL   0x3

#define VAULTIP_MAC_ALGORITHM_HMAC_SHA_1    0x1
#define VAULTIP_MAC_ALGORITHM_HMAC_SHA_224  0x2
#define VAULTIP_MAC_ALGORITHM_HMAC_SHA_256  0x3
#define VAULTIP_MAC_ALGORITHM_HMAC_SHA_384  0x4
#define VAULTIP_MAC_ALGORITHM_HMAC_SHA_512  0x5
#define VAULTIP_MAC_ALGORITHM_AES_CMAC      0x8
#define VAULTIP_MAC_ALGORITHM_AES_CBC_MAC   0x9

#define VAULTIP_MAC_MODE_INITIAL_FINAL         0x0
#define VAULTIP_MAC_MODE_CONTINUED_FINAL       0x1
#define VAULTIP_MAC_MODE_INITIAL_NOT_FINAL     0x2
#define VAULTIP_MAC_MODE_CONTINUED_NOT_FINAL   0x3

#define VAULTIP_PUBLIC_KEY_COMMAND_ECC_KEY_CHECK                                    0x01
#define VAULTIP_PUBLIC_KEY_COMMAND_DH_KEY_CHECK                                     0x02
#define VAULTIP_PUBLIC_KEY_COMMAND_ECDSA_SIGN                                       0x06
#define VAULTIP_PUBLIC_KEY_COMMAND_ECDSA_VERIFY                                     0x07
#define VAULTIP_PUBLIC_KEY_COMMAND_RSA_PKCS15_SIGN                                  0x08
#define VAULTIP_PUBLIC_KEY_COMMAND_RSA_PKCS15_VERIFY                                0x09
#define VAULTIP_PUBLIC_KEY_COMMAND_RSA_PSS_SIGN                                     0x0C
#define VAULTIP_PUBLIC_KEY_COMMAND_RSA_PSS_VERIFY                                   0x0D
#define VAULTIP_PUBLIC_KEY_COMMAND_DH_GENERATE_PUBLIC_KEY                           0x10
#define VAULTIP_PUBLIC_KEY_COMMAND_DH_GENERATE_PUBLIC_AND_PRIVATE_KEY               0x11
#define VAULTIP_PUBLIC_KEY_COMMAND_DH_GENERATE_SECRETS_SINGLE_KEY_PAIR              0x12
#define VAULTIP_PUBLIC_KEY_COMMAND_DH_GENERATE_SECRETS_DUAL_KEY_PAIR                0x13
#define VAULTIP_PUBLIC_KEY_COMMAND_RSA_OAEP_WRAP                                    0x18
#define VAULTIP_PUBLIC_KEY_COMMAND_RSA_OAEP_WRAP_INPUT_HASHED                       0x19
#define VAULTIP_PUBLIC_KEY_COMMAND_RSA_OAEP_UNWRAP                                  0x1A
#define VAULTIP_PUBLIC_KEY_COMMAND_RSA_OAEP_UNWRAP_INPUT_HASHED                     0x1B
#define VAULTIP_PUBLIC_KEY_COMMAND_RSA_PKCS15_UNWRAP                                0x21
#define VAULTIP_PUBLIC_KEY_COMMAND_RSA_PKCS15_WRAP                                  0x22
#define VAULTIP_PUBLIC_KEY_COMMAND_ELGAMAL_ECC_ENCRYPTION                           0x24
#define VAULTIP_PUBLIC_KEY_COMMAND_ELGAMAL_ECC_DECRYPTION                           0x25
#define VAULTIP_PUBLIC_KEY_COMMAND_ECDH_CURVE25519_GENERATE_PUBLIC_KEY              0x28
#define VAULTIP_PUBLIC_KEY_COMMAND_ECDH_CURVE25519_GENERATE_PRIVATE_AND_PUBLIC_KEY  0x29
#define VAULTIP_PUBLIC_KEY_COMMAND_ECDH_CURVE25519_GENERATE_SHARED_SECRETS          0x2A
#define VAULTIP_PUBLIC_KEY_COMMAND_ECDSA_GENERATE_PUBLIC_KEY                        0x2B
#define VAULTIP_PUBLIC_KEY_COMMAND_ECDSA_GENERATE_PRIVATE_AND_PUBLIC_KEY            0x2C
#define VAULTIP_PUBLIC_KEY_COMMAND_ECDSA_SIGN_INITIAL                               0x2D
#define VAULTIP_PUBLIC_KEY_COMMAND_ECDSA_SIGN_UPDATE                                0x2E
#define VAULTIP_PUBLIC_KEY_COMMAND_ECDSA_SIGN_FINAL                                 0x2F
#define VAULTIP_PUBLIC_KEY_COMMAND_ECDSA_VERIFY_INITIAL                             0x30
#define VAULTIP_PUBLIC_KEY_COMMAND_ECDSA_VERIFY_FINAL                               0x31

#define VAULTIP_TRNG_NORMAL 0x0
#define VAULTIP_TRNG_RAW 0x5244

typedef struct VAULTIP_SUBVECTOR_HEADER_s {
    uint32_t SubVectorLength : 16;
    uint32_t SubVectorIndex : 8;
    uint32_t NrOfSubVectors : 8;
} VAULTIP_SUBVECTOR_HEADER_t;
static_assert(4 == sizeof(VAULTIP_SUBVECTOR_HEADER_t), "sizeof(VAULTIP_SUBVECTOR_HEADER_t) is not 4!");

typedef struct VAULTIP_SUBVECTOR_32_s {
    VAULTIP_SUBVECTOR_HEADER_t header;
    union {
        uint32_t u32[1];
        uint8_t u8[4];
    } data;
} VAULTIP_SUBVECTOR_32_t;
static_assert((4 + 4) == sizeof(VAULTIP_SUBVECTOR_32_t), "sizeof(VAULTIP_SUBVECTOR_32_t) is not 8!");

#define VAULTIP_SUBVECTOR_64_ENTRIES ((64 + 31) / 32)
typedef struct VAULTIP_SUBVECTOR_64_s {
    VAULTIP_SUBVECTOR_HEADER_t header;
    union {
        uint32_t u32[VAULTIP_SUBVECTOR_64_ENTRIES];
        uint8_t u8[4 * VAULTIP_SUBVECTOR_64_ENTRIES];
    } data;
} VAULTIP_SUBVECTOR_64_t;
static_assert((4 + 8) == sizeof(VAULTIP_SUBVECTOR_64_t), "sizeof(VAULTIP_SUBVECTOR_64_t) is not 12!");

#define VAULTIP_SUBVECTOR_256_ENTRIES ((256 + 31) / 32)
typedef struct VAULTIP_SUBVECTOR_256_s {
    VAULTIP_SUBVECTOR_HEADER_t header;
    union {
        uint32_t u32[VAULTIP_SUBVECTOR_256_ENTRIES];
        uint8_t u8[4 * VAULTIP_SUBVECTOR_256_ENTRIES];
    } data;
} VAULTIP_SUBVECTOR_256_t;
static_assert((4 + 32) == sizeof(VAULTIP_SUBVECTOR_256_t), "sizeof(VAULTIP_SUBVECTOR_256_t) is not 36!");

#define VAULTIP_SUBVECTOR_384_ENTRIES ((384 + 31) / 32)
typedef struct VAULTIP_SUBVECTOR_384_s {
    VAULTIP_SUBVECTOR_HEADER_t header;
    union {
        uint32_t u32[VAULTIP_SUBVECTOR_384_ENTRIES];
        uint8_t u8[4 * VAULTIP_SUBVECTOR_384_ENTRIES];
    } data;
} VAULTIP_SUBVECTOR_384_t;
static_assert((4 + 48) == sizeof(VAULTIP_SUBVECTOR_384_t), "sizeof(VAULTIP_SUBVECTOR_384_t) is not 52!");

#define VAULTIP_SUBVECTOR_521_ENTRIES ((521 + 31) / 32)
typedef struct VAULTIP_SUBVECTOR_521_s {
    VAULTIP_SUBVECTOR_HEADER_t header;
    union {
        uint32_t u32[VAULTIP_SUBVECTOR_521_ENTRIES];
        uint8_t u8[4 * VAULTIP_SUBVECTOR_521_ENTRIES];
    } data;
} VAULTIP_SUBVECTOR_521_t;
static_assert((4 + 68) == sizeof(VAULTIP_SUBVECTOR_521_t), "sizeof(VAULTIP_SUBVECTOR_521_t) is not 72!");

typedef struct VAULTIP_EC_256_DOMAIN_PARAMETERS_s {
    VAULTIP_SUBVECTOR_256_t curve_modulus_p;
    VAULTIP_SUBVECTOR_256_t curve_constant_a;
    VAULTIP_SUBVECTOR_256_t curve_constant_b;
    VAULTIP_SUBVECTOR_256_t curve_order_n;
    VAULTIP_SUBVECTOR_256_t curve_base_point_x;
    VAULTIP_SUBVECTOR_256_t curve_base_point_y;
    VAULTIP_SUBVECTOR_32_t curve_cofactor;
} VAULTIP_EC_256_DOMAIN_PARAMETERS_t;

typedef struct VAULTIP_EC_384_DOMAIN_PARAMETERS_s {
    VAULTIP_SUBVECTOR_384_t curve_modulus_p;
    VAULTIP_SUBVECTOR_384_t curve_constant_a;
    VAULTIP_SUBVECTOR_384_t curve_constant_b;
    VAULTIP_SUBVECTOR_384_t curve_order_n;
    VAULTIP_SUBVECTOR_384_t curve_base_point_x;
    VAULTIP_SUBVECTOR_384_t curve_base_point_y;
    VAULTIP_SUBVECTOR_32_t curve_cofactor;
} VAULTIP_EC_384_DOMAIN_PARAMETERS_t;

typedef struct VAULTIP_EC_521_DOMAIN_PARAMETERS_s {
    VAULTIP_SUBVECTOR_521_t curve_modulus_p;
    VAULTIP_SUBVECTOR_521_t curve_constant_a;
    VAULTIP_SUBVECTOR_521_t curve_constant_b;
    VAULTIP_SUBVECTOR_521_t curve_order_n;
    VAULTIP_SUBVECTOR_521_t curve_base_point_x;
    VAULTIP_SUBVECTOR_521_t curve_base_point_y;
    VAULTIP_SUBVECTOR_32_t curve_cofactor;
} VAULTIP_EC_521_DOMAIN_PARAMETERS_t;

typedef struct VAULTIP_PUBLIC_KEY_ECDSA_P256_s {
    VAULTIP_SUBVECTOR_256_t point_x;
    VAULTIP_SUBVECTOR_256_t point_y;
} VAULTIP_PUBLIC_KEY_ECDSA_P256_t;

typedef struct VAULTIP_PRIVATE_KEY_ECDSA_P256_s {
    VAULTIP_SUBVECTOR_256_t modulus;
} VAULTIP_PRIVATE_KEY_ECDSA_P256_t;

typedef struct VAULTIP_PUBLIC_KEY_ECDSA_P384_s {
    VAULTIP_SUBVECTOR_384_t point_x;
    VAULTIP_SUBVECTOR_384_t point_y;
} VAULTIP_PUBLIC_KEY_ECDSA_P384_t;

typedef struct VAULTIP_PRIVATE_KEY_ECDSA_P384_s {
    VAULTIP_SUBVECTOR_384_t modulus;
} VAULTIP_PRIVATE_KEY_ECDSA_P384_t;

typedef struct VAULTIP_PUBLIC_KEY_ECDSA_P521_s {
    VAULTIP_SUBVECTOR_521_t point_x;
    VAULTIP_SUBVECTOR_521_t point_y;
} VAULTIP_PUBLIC_KEY_ECDSA_P521_t;

typedef struct VAULTIP_PRIVATE_KEY_ECDSA_P521_s {
    VAULTIP_SUBVECTOR_521_t modulus;
} VAULTIP_PRIVATE_KEY_ECDSA_P521_t;

typedef struct VAULTIP_PUBLIC_KEY_ECDSA_25519_s {
    VAULTIP_SUBVECTOR_256_t point_x;
} VAULTIP_PUBLIC_KEY_ECDSA_25519_t;

typedef struct VAULTIP_PRIVATE_KEY_ECDSA_25519_s {
    VAULTIP_SUBVECTOR_256_t modulus;
} VAULTIP_PRIVATE_KEY_ECDSA_25519_t;

#define VAULTIP_SUBVECTOR_2048_ENTRIES (2048 / 32)
typedef struct VAULTIP_SUBVECTOR_2048_s {
    VAULTIP_SUBVECTOR_HEADER_t header;
    union {
        uint32_t u32[VAULTIP_SUBVECTOR_2048_ENTRIES];
        uint8_t u8[4 * VAULTIP_SUBVECTOR_2048_ENTRIES];
    } data;
} VAULTIP_SUBVECTOR_2048_t;
static_assert((4 + 256) == sizeof(VAULTIP_SUBVECTOR_2048_t), "sizeof(VAULTIP_SUBVECTOR_2048_t) is not 260!");

#define VAULTIP_SUBVECTOR_3072_ENTRIES (3072 / 32)
typedef struct VAULTIP_SUBVECTOR_3072_s {
    VAULTIP_SUBVECTOR_HEADER_t header;
    union {
        uint32_t u32[VAULTIP_SUBVECTOR_3072_ENTRIES];
        uint8_t u8[4 * VAULTIP_SUBVECTOR_3072_ENTRIES];
    } data;
} VAULTIP_SUBVECTOR_3072_t;
static_assert((4 + 384) == sizeof(VAULTIP_SUBVECTOR_3072_t), "sizeof(VAULTIP_SUBVECTOR_3072_t) is not 384!");

#define VAULTIP_SUBVECTOR_4096_ENTRIES (4096 / 32)
typedef struct VAULTIP_SUBVECTOR_4096_s {
    VAULTIP_SUBVECTOR_HEADER_t header;
    union {
        uint32_t u32[VAULTIP_SUBVECTOR_4096_ENTRIES];
        uint8_t u8[4 * VAULTIP_SUBVECTOR_4096_ENTRIES];
    } data;
} VAULTIP_SUBVECTOR_4096_t;
static_assert((4 + 512) == sizeof(VAULTIP_SUBVECTOR_4096_t), "sizeof(VAULTIP_SUBVECTOR_4096_t) is not 516!");

typedef struct VAULTIP_PUBLIC_KEY_RSA_2048_s {
    VAULTIP_SUBVECTOR_2048_t modulus;
    VAULTIP_SUBVECTOR_64_t exponent;
} VAULTIP_PUBLIC_KEY_RSA_2048_t;

typedef struct VAULTIP_PRIVATE_KEY_RSA_2048_s {
    VAULTIP_SUBVECTOR_2048_t modulus;
    VAULTIP_SUBVECTOR_2048_t exponent;
} VAULTIP_PRIVATE_KEY_RSA_2048_t;

typedef struct VAULTIP_PUBLIC_KEY_RSA_3072_s {
    VAULTIP_SUBVECTOR_3072_t modulus;
    VAULTIP_SUBVECTOR_64_t exponent;
} VAULTIP_PUBLIC_KEY_RSA_3072_t;

typedef struct VAULTIP_PRIVATE_KEY_RSA_3072_s {
    VAULTIP_SUBVECTOR_3072_t modulus;
    VAULTIP_SUBVECTOR_3072_t exponent;
} VAULTIP_PRIVATE_KEY_RSA_3072_t;

typedef struct VAULTIP_PUBLIC_KEY_RSA_4096_s {
    VAULTIP_SUBVECTOR_4096_t modulus;
    VAULTIP_SUBVECTOR_64_t exponent;
} VAULTIP_PUBLIC_KEY_RSA_4096_t;

typedef struct VAULTIP_PRIVATE_KEY_RSA_4096_s {
    VAULTIP_SUBVECTOR_4096_t modulus;
    VAULTIP_SUBVECTOR_4096_t exponent;
} VAULTIP_PRIVATE_KEY_RSA_4096_t;

typedef struct VAULTIP_SIGNATURE_EC_P256_s {
    VAULTIP_SUBVECTOR_256_t r;
    VAULTIP_SUBVECTOR_256_t s;
} VAULTIP_SIGNATURE_EC_P256_t;

typedef struct VAULTIP_SIGNATURE_EC_P384_s {
    VAULTIP_SUBVECTOR_384_t r;
    VAULTIP_SUBVECTOR_384_t s;
} VAULTIP_SIGNATURE_EC_P384_t;

typedef struct VAULTIP_SIGNATURE_EC_P521_s {
    VAULTIP_SUBVECTOR_521_t r;
    VAULTIP_SUBVECTOR_521_t s;
} VAULTIP_SIGNATURE_EC_P521_t;

typedef struct VAULTIP_SIGNATURE_EC_25519_s {
    VAULTIP_SUBVECTOR_256_t r;
} VAULTIP_SIGNATURE_EC_25519_t;

typedef struct VAULTIP_SIGNATURE_RSA_2048_s {
    VAULTIP_SUBVECTOR_2048_t s;
} VAULTIP_SIGNATURE_RSA_2048_t;

typedef struct VAULTIP_SIGNATURE_RSA_3072_s {
    VAULTIP_SUBVECTOR_3072_t s;
} VAULTIP_SIGNATURE_RSA_3072_t;

typedef struct VAULTIP_SIGNATURE_RSA_4096_s {
    VAULTIP_SUBVECTOR_4096_t s;
} VAULTIP_SIGNATURE_RSA_4096_t;

#endif
