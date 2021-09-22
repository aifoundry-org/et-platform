/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file pmic_controller.c
    \brief A C module that implements the secure sign in functionality. It 
    loads and compares the image certificates.

    Public interfaces:
        verify_esperanto_image_certificate
        load_sw_certificates_chain
*/
/***********************************************************************/

#include "etsoc/drivers/serial/serial.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "log.h"
#include "bl2_main.h"
#include "bl2_certificates.h"
#include "sp_otp.h"
#include "bl2_flashfs_driver.h"
#include "constant_memory_compare.h"
#include "bl2_crypto.h"

#pragma GCC diagnostic ignored "-Wswitch-enum"

static bool gs_vaultip_disabled;
static bool gs_ignore_signatures;

static void dump_sha512(const uint8_t hash[512 / 8])
{
    Log_Write(LOG_LEVEL_INFO, "%02x%02x%02x%02x%02x%02x..%02x%02x%02x%02x%02x%02x\n", hash[0], hash[1], hash[2],
           hash[3], hash[4], hash[5], hash[58], hash[59], hash[60], hash[61], hash[62], hash[63]);
}

static int verify_certificate_name_sequence(const SECURITY_CERTIFICATE_NAME_SEQUENCE_t *sequence)
{
    if (sequence->organization_length > sizeof(sequence->organization)) 
    {
        return -1;
    }
    if (sequence->org_unit_length > sizeof(sequence->org_unit)) 
    {
        return -1;
    }
    if (sequence->city_length > sizeof(sequence->city)) 
    {
        return -1;
    }
    if (sequence->state_length > sizeof(sequence->state)) 
    {
        return -1;
    }
    if (sequence->common_name_length > sizeof(sequence->common_name)) 
    {
        return -1;
    }
    if (sequence->serial_number_length > sizeof(sequence->serial_number)) 
    {
        return -1;
    }
    if (0 == sequence->common_name_length) 
    {
        return -1;
    }
    return 0;
}

static int basic_certificate_check(const ESPERANTO_CERTIFICATE_t *certificate)
{
    if (NULL == certificate) 
    {
        return -1;
    }

    if (CURRENT_CERTIFICATE_HEADER_TAG != certificate->certificate_info.header_tag) 
    {
        Log_Write(LOG_LEVEL_ERROR, "basic_certificate_check: invalid certificate header tag!\n");
        return -1;
    }

    if (CURRENT_CERTIFICATE_VERSION_TAG != certificate->certificate_info.version_tag) 
    {
        Log_Write(LOG_LEVEL_ERROR, "basic_certificate_check: invalid certificate version!\n");
        return -1;
    }

    if (0 != crypto_verify_signature_params(&(certificate->certificate_info_signature))) 
    {
        Log_Write(LOG_LEVEL_ERROR, "basic_certificate_check: crypto_verify_signature_params() failed!\n");
        return -1;
    }

    if (certificate->certificate_info.hash_algorithm !=
        certificate->certificate_info_signature.hashAlg) 
    {
        Log_Write(LOG_LEVEL_ERROR, "basic_certificate_check: certificate signature hash algorithm mismatch!\n");
        return -1;
    }

    if (certificate->certificate_info.signing_key_type !=
        certificate->certificate_info_signature.keyType) 
    {
        Log_Write(LOG_LEVEL_ERROR, "basic_certificate_check: certificate signature signing key type mismatch!\n");
        return -1;
    }

    switch (certificate->certificate_info.signing_key_type) 
    {
        case PUBLIC_KEY_TYPE_EC:
            if (certificate->certificate_info.curveID !=
                certificate->certificate_info_signature.ec.curveID) 
            {
                Log_Write(LOG_LEVEL_ERROR, "basic_certificate_check: certificate signature EC curve mismatch!\n");
                return -1;
            }
            break;
        case PUBLIC_KEY_TYPE_RSA:
            if (certificate->certificate_info.keySize !=
                certificate->certificate_info_signature.rsa.keySize) 
            {
                Log_Write(LOG_LEVEL_ERROR, "basic_certificate_check: certificate signature RSA key size mismatch!\n");
                return -1;
            }
            break;
        default:
            Log_Write(LOG_LEVEL_ERROR, "basic_certificate_check: invalid signing key type!\n");
            return -1;
    }

    if (0 != crypto_verify_public_key_params(&(certificate->certificate_info.subject_public_key))) 
    {
        Log_Write(LOG_LEVEL_ERROR, "basic_certificate_check: verify_public_key_params() failed!\n");
        return -1;
    }

    if (0 != verify_certificate_name_sequence(&(certificate->certificate_info.subject))) 
    {
        Log_Write(LOG_LEVEL_ERROR, "basic_certificate_check: verify_certificate_name_sequence(subject) failed!\n");
        return -1;
    }

    if (0 != verify_certificate_name_sequence(&(certificate->certificate_info.issuer))) 
    {
        Log_Write(LOG_LEVEL_ERROR, "basic_certificate_check: verify_certificate_name_sequence(issuer) failed!\n");
        return -1;
    }

    return 0;
}

/* static void dump_sha512(const uint8_t hash[512/8]) {
     Log_Write(LOG_LEVEL_DEBUG, "%02x%02x%02x%02x%02x%02x..%02x%02x%02x%02x%02x%02x\n",
         hash[0], hash[1], hash[2], hash[3], hash[4], hash[5],
         hash[58], hash[59], hash[60], hash[61], hash[62], hash[63]);
 } */

static int is_sw_root_ca_hash_provisioned(bool *provisioned)
{
    *provisioned = false;
    return 0;
}

static const uint8_t *get_sw_root_ca_sha512_hash(void)
{
    return NULL;
}

static int enhanced_certificate_check(const ESPERANTO_CERTIFICATE_t *certificate,
                                      const ESPERANTO_CERTIFICATE_t *parent_certificate,
                                      uint32_t required_designation_flags)
{
    static uint8_t hash_result[512 / 8];

    if (NULL == certificate) 
    {
        return -1;
    }

    if (NULL == parent_certificate) 
    {
        /* we are checking the SW ROOT CA certificate */
        if (0 != crypto_hash(HASH_ALG_SHA2_512, certificate, sizeof(ESPERANTO_CERTIFICATE_t),
                             hash_result)) 
        {
            Log_Write(LOG_LEVEL_ERROR, "enhanced_certificate_check: vaultip_hash_sha_512(sp_root_ca) failed!\n");
            return -1;
        }

        if (0 != constant_time_memory_compare(hash_result, get_sw_root_ca_sha512_hash(), 512 / 8)) 
        {
            Log_Write(LOG_LEVEL_ERROR, "enhanced_certificate_check: Invalid SW ROOT CA certificate hash!\n");
            Log_Write(LOG_LEVEL_ERROR, "Expected: ");
            dump_sha512(get_sw_root_ca_sha512_hash());
            Log_Write(LOG_LEVEL_ERROR, "Computed: ");
            dump_sha512(hash_result);
            return -1;
        }
    } 
    else 
    {
        /* we are checking a lower-level certificate */

        if (0 == parent_certificate->certificate_info.is_CA) 
        {
            Log_Write(LOG_LEVEL_ERROR, "enhanced_certificate_check: Parent certificate is not a CA!\n");
            return -1;
        }

        /* todo: verify the esperanto attributes
         if (0 == parent_certificate->certificate_info.esperanto_attributes) {
             Log_Write(LOG_LEVEL_ERROR, "enhanced_certificate_check: esperanto attributes mismatch!\n");
             return -1;
         } */

        if (required_designation_flags !=
            (parent_certificate->certificate_info.esperanto_designation &
             required_designation_flags)) 
        {
            Log_Write(LOG_LEVEL_ERROR, "enhanced_certificate_check: esperanto designation mismatch!\n");
            return -1;
        }

        if (0 != constant_time_memory_compare(
                     &(certificate->certificate_info.issuer_key_identifier),
                     &(parent_certificate->certificate_info.subject_key_identifier),
                     sizeof(certificate->certificate_info.subject_key_identifier))) 
        {
            Log_Write(LOG_LEVEL_ERROR, "enhanced_certificate_check: Certificate parent key identifier mismatch!\n");
            return -1;
        }

        if (0 != constant_time_memory_compare(&(certificate->certificate_info.issuer),
                                              &(parent_certificate->certificate_info.subject),
                                              sizeof(certificate->certificate_info.issuer))) 
        {
            Log_Write(LOG_LEVEL_ERROR, "enhanced_certificate_check: Certificate parent info mismatch!\n");
            return -1;
        }

        if (gs_ignore_signatures) 
        {
            MESSAGE_INFO("CRT SIG IGN\n");
        } 
        else 
        {
            if (0 != crypto_verify_pk_signature(
                         &(parent_certificate->certificate_info.subject_public_key),
                         &(certificate->certificate_info_signature),
                         &(certificate->certificate_info), sizeof(certificate->certificate_info))) 
            {
                Log_Write(LOG_LEVEL_ERROR, "enhanced_certificate_check: signature check failed!\n");
                return -1;
            }
        }
    }

    return 0;
}

static int verify_certificate(const ESPERANTO_CERTIFICATE_t *certificate,
                              const ESPERANTO_CERTIFICATE_t *parent_certificate,
                              uint32_t required_designation_flags)
{
    if (0 != basic_certificate_check(certificate)) 
    {
        return -1;
    }
    if (0 !=
        enhanced_certificate_check(certificate, parent_certificate, required_designation_flags)) 
    {
        return -1;
    }
    return 0;
}

int verify_esperanto_image_certificate(const ESPERANTO_IMAGE_TYPE_t image_type,
                                       const ESPERANTO_CERTIFICATE_t *certificate)
{
    SERVICE_PROCESSOR_BL2_DATA_t *bl2_data = get_service_processor_bl2_data();
    uint32_t designation_flags;
    const ESPERANTO_CERTIFICATE_t *parent_certificate;

    if (gs_vaultip_disabled) 
    {
        MESSAGE_ERROR("CE DIS!\n");
        return -1;
    }

    switch (image_type) 
    {
        case ESPERANTO_IMAGE_TYPE_SP_BL1:
            designation_flags = ESPERANTO_CERTIFICATE_DESIGNATION_BL1_CA;
            parent_certificate = &(bl2_data->sp_certificates[1]);
            break;
        case ESPERANTO_IMAGE_TYPE_SP_BL2:
            designation_flags = ESPERANTO_CERTIFICATE_DESIGNATION_BL2_CA;
            parent_certificate = &(bl2_data->sp_certificates[1]);
            break;
        case ESPERANTO_IMAGE_TYPE_MACHINE_MINION:
            designation_flags = ESPERANTO_CERTIFICATE_DESIGNATION_MACHINE_MINION_CA;
            parent_certificate = &(bl2_data->sw_certificates[1]);
            break;
        case ESPERANTO_IMAGE_TYPE_MASTER_MINION:
            designation_flags = ESPERANTO_CERTIFICATE_DESIGNATION_MASTER_MINION_CA;
            parent_certificate = &(bl2_data->sw_certificates[1]);
            break;
        case ESPERANTO_IMAGE_TYPE_WORKER_MINION:
            designation_flags = ESPERANTO_CERTIFICATE_DESIGNATION_WORKER_MINION_CA;
            parent_certificate = &(bl2_data->sw_certificates[1]);
            break;
        /* case ESPERANTO_IMAGE_TYPE_COMPUTE_KERNEL:
             designation_flags = ;
             parent_certificate = &(bl2_data->sw_certificates[1]);
             break; */
        case ESPERANTO_IMAGE_TYPE_MAXION_BL1:
            designation_flags = ESPERANTO_CERTIFICATE_DESIGNATION_MAXION_BL1_CA;
            parent_certificate = &(bl2_data->sw_certificates[1]);
            break;
        default:
            Log_Write(LOG_LEVEL_ERROR, "verify_esperanto_image_certificate: invalid image type!\n");
            return -1;
    }

    if (0 != verify_certificate(certificate, parent_certificate, designation_flags)) 
    {
        Log_Write(LOG_LEVEL_ERROR, "verify_esperanto_image_certificate: verify_certificate() failed!\n");
        return -1;
    }
    return 0;
}

static int verify_sw_certificates_chain(const ESPERANTO_CERTIFICATE_t sw_certificates[2])
{
    if (NULL == sw_certificates) 
    {
        return -1;
    }

    if (0 !=
        verify_certificate(&sw_certificates[0], NULL, ESPERANTO_CERTIFICATE_DESIGNATION_ROOT_CA)) 
    {
        Log_Write(LOG_LEVEL_ERROR, "verify_sw_certificates_chain: Invalid SW ROOT CA certificate!\n");
        return -1;
    }
    Log_Write(LOG_LEVEL_INFO, "SW ROOT CA Certificate OK!\n");
    if (0 != verify_certificate(&sw_certificates[1], &sw_certificates[0],
                                ESPERANTO_CERTIFICATE_DESIGNATION_ISSUING_CA)) 
    {
        Log_Write(LOG_LEVEL_ERROR, "verify_sw_certificates_chain: Invalid ISSUING CA certificate!\n");
        return -1;
    }
    Log_Write(LOG_LEVEL_INFO, "SW Issuing CA Certificate OK!\n");
    return 0;
}

int load_sw_certificates_chain(void)
{
    bool sw_root_ca_hash_available;
    uint32_t sw_certificates_size;
    SERVICE_PROCESSOR_BL2_DATA_t *bl2_data = get_service_processor_bl2_data();

    gs_vaultip_disabled = is_vaultip_disabled();
    gs_ignore_signatures = false;

    if (0 != sp_otp_get_signatures_check_chicken_bit(&gs_ignore_signatures)) 
    {
        gs_ignore_signatures = false;
    }

    if (gs_vaultip_disabled) 
    {
        /* TODO: Update the following to Log macro - set to INFO/DEBUG
        Log_Write(LOG_LEVEL_ERROR, "Skip VaultIP OTP SW ROOT CA HASH check!\n"); */
        sw_root_ca_hash_available = false;
    } 
    else 
    {
        if (0 != is_sw_root_ca_hash_provisioned(&sw_root_ca_hash_available)) 
        {
            Log_Write(LOG_LEVEL_ERROR, "is_sw_root_ca_hash_provisioned() failed!\n");
            return -1;
        }
    }

    if (!sw_root_ca_hash_available) 
    {
        /* TODO: Update the following to Log macro - set to INFO/DEBUG
           Log_Write(LOG_LEVEL_INFO, "SW ROOT CA not provisioned. Using SP ROOT CA.\n"); */
        memcpy(bl2_data->sw_certificates, bl2_data->sp_certificates,
               sizeof(bl2_data->sw_certificates));
        return 0;
    }

    /* load the SW CA certificates */
    if (0 != flashfs_drv_get_file_size(ESPERANTO_FLASH_REGION_ID_SW_CERTIFICATES,
                                       &sw_certificates_size)) 
    {
        Log_Write(LOG_LEVEL_ERROR, "flashfs_drv_get_file_size(SW_CRT) failed!\n");
        return -1;
    }
    if (sizeof(bl2_data->sw_certificates) != sw_certificates_size) 
    {
        Log_Write(LOG_LEVEL_ERROR, "Invalid SW certificates file size!\n");
        return -1;
    }
    if (0 != flashfs_drv_read_file(ESPERANTO_FLASH_REGION_ID_SW_CERTIFICATES, 0,
                                   bl2_data->sw_certificates, sizeof(bl2_data->sw_certificates))) 
    {
        Log_Write(LOG_LEVEL_ERROR, "flashfs_drv_read_file(SW_CRT) failed!\n");
        return -1;
    }
    Log_Write(LOG_LEVEL_INFO, "Loaded SW certificates chain.\n");

    if (0 != verify_sw_certificates_chain(bl2_data->sw_certificates)) 
    {
        Log_Write(LOG_LEVEL_INFO, "Invalid SW certificate chain!\n");
        return -1;
    }

    bl2_data->sw_certificates_loaded = 1;
    return 0;
}
