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

#include "printx.h"

#include "bl1_main.h"
#include "bl1_sp_certificates.h"
#include "sp_otp.h"
#include "bl1_flash_fs.h"
#include "constant_memory_compare.h"
#include "bl1_crypto.h"

#pragma GCC diagnostic ignored "-Wswitch-enum"

static bool gs_vaultip_disabled;
static bool gs_ignore_signatures;

static int verify_certificate_name_sequence(const SECURITY_CERTIFICATE_NAME_SEQUENCE_t *sequence)
{
    if (sequence->organization_length > sizeof(sequence->organization)) {
        return -1;
    }
    if (sequence->org_unit_length > sizeof(sequence->org_unit)) {
        return -1;
    }
    if (sequence->city_length > sizeof(sequence->city)) {
        return -1;
    }
    if (sequence->state_length > sizeof(sequence->state)) {
        return -1;
    }
    if (sequence->common_name_length > sizeof(sequence->common_name)) {
        return -1;
    }
    if (sequence->serial_number_length > sizeof(sequence->serial_number)) {
        return -1;
    }
    if (0 == sequence->common_name_length) {
        return -1;
    }
    return 0;
}

static int basic_certificate_check(const ESPERANTO_CERTIFICATE_t *certificate)
{
    if (NULL == certificate) {
        return -1;
    }

    if (CURRENT_CERTIFICATE_HEADER_TAG != certificate->certificate_info.header_tag) {
        printx("basic_certificate_check: invalid certificate header tag!\n");
        return -1;
    }

    if (CURRENT_CERTIFICATE_VERSION_TAG != certificate->certificate_info.version_tag) {
        printx("basic_certificate_check: invalid certificate version!\n");
        return -1;
    }

    if (0 != crypto_verify_signature_params(&(certificate->certificate_info_signature))) {
        printx("basic_certificate_check: crypto_verify_signature_params() failed!\n");
        return -1;
    }

    if (certificate->certificate_info.hash_algorithm !=
        certificate->certificate_info_signature.hashAlg) {
        printx("basic_certificate_check: certificate signature hash algorithm mismatch!\n");
        return -1;
    }

    if (certificate->certificate_info.signing_key_type !=
        certificate->certificate_info_signature.keyType) {
        printx("basic_certificate_check: certificate signature signing key type mismatch!\n");
        return -1;
    }

    switch (certificate->certificate_info.signing_key_type) {
    case PUBLIC_KEY_TYPE_EC:
        if (certificate->certificate_info.curveID !=
            certificate->certificate_info_signature.ec.curveID) {
            printx("basic_certificate_check: certificate signature EC curve mismatch!\n");
            return -1;
        }
        break;
    case PUBLIC_KEY_TYPE_RSA:
        if (certificate->certificate_info.keySize !=
            certificate->certificate_info_signature.rsa.keySize) {
            printx("basic_certificate_check: certificate signature RSA key size mismatch!\n");
            return -1;
        }
        break;
    default:
        printx("basic_certificate_check: invalid signing key type!\n");
        return -1;
    }

    if (0 != crypto_verify_public_key_params(&(certificate->certificate_info.subject_public_key))) {
        printx("basic_certificate_check: verify_public_key_params() failed!\n");
        return -1;
    }

    if (0 != verify_certificate_name_sequence(&(certificate->certificate_info.subject))) {
        printx("basic_certificate_check: verify_certificate_name_sequence(subject) failed!\n");
        return -1;
    }

    if (0 != verify_certificate_name_sequence(&(certificate->certificate_info.issuer))) {
        printx("basic_certificate_check: verify_certificate_name_sequence(issuer) failed!\n");
        return -1;
    }

    return 0;
}

// static void dump_sha512(const uint8_t hash[512/8]) {
//     printx("%02x%02x%02x%02x%02x%02x..%02x%02x%02x%02x%02x%02x\n",
//         hash[0], hash[1], hash[2], hash[3], hash[4], hash[5],
//         hash[58], hash[59], hash[60], hash[61], hash[62], hash[63]);
// }

static int enhanced_certificate_check(const ESPERANTO_CERTIFICATE_t *certificate,
                                      const ESPERANTO_CERTIFICATE_t *parent_certificate,
                                      uint32_t required_designation_flags)
{
    if (NULL == certificate) {
        return -1;
    }

    if (NULL == parent_certificate) {
        return -1;
    } else {
        // we are checking a lower-level certificate

        if (0 == parent_certificate->certificate_info.is_CA) {
            printx("enhanced_certificate_check: Parent certificate is not a CA!\n");
            return -1;
        }

        // todo: verify the esperanto attributes
        // if (0 == parent_certificate->certificate_info.esperanto_attributes) {
        //     printx("enhanced_certificate_check: esperanto attributes mismatch!\n");
        //     return -1;
        // }

        if (required_designation_flags !=
            (parent_certificate->certificate_info.esperanto_designation &
             required_designation_flags)) {
            printx("enhanced_certificate_check: esperanto designation mismatch!\n");
            return -1;
        }

        if (0 != constant_time_memory_compare(
                     &(certificate->certificate_info.issuer_key_identifier),
                     &(parent_certificate->certificate_info.subject_key_identifier),
                     sizeof(certificate->certificate_info.subject_key_identifier))) {
            printx("enhanced_certificate_check: Certificate parent key identifier mismatch!\n");
            return -1;
        }

        if (0 != constant_time_memory_compare(&(certificate->certificate_info.issuer),
                                              &(parent_certificate->certificate_info.subject),
                                              sizeof(certificate->certificate_info.issuer))) {
            printx("enhanced_certificate_check: Certificate parent info mismatch!\n");
            return -1;
        }

        if (gs_ignore_signatures) {
            MESSAGE_INFO("CRT SIG IGN\n");
        } else {
            if (0 != crypto_verify_pk_signature(
                         &(parent_certificate->certificate_info.subject_public_key),
                         &(certificate->certificate_info_signature),
                         &(certificate->certificate_info), sizeof(certificate->certificate_info))) {
                printx("enhanced_certificate_check: signature check failed!\n");
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
    if (0 != basic_certificate_check(certificate)) {
        return -1;
    }
    if (0 !=
        enhanced_certificate_check(certificate, parent_certificate, required_designation_flags)) {
        return -1;
    }
    return 0;
}

int verify_bl2_certificate(const ESPERANTO_CERTIFICATE_t *certificate)
{
    SERVICE_PROCESSOR_BL1_DATA_t *bl1_data = get_service_processor_bl1_data();

    gs_vaultip_disabled = is_vaultip_disabled();
    gs_ignore_signatures = false;

    if (0 != sp_otp_get_signatures_check_chicken_bit(&gs_ignore_signatures)) {
        gs_ignore_signatures = false;
    }

    if (gs_vaultip_disabled) {
        MESSAGE_ERROR("CE DIS!\n");
        return -1;
    }

    if (0 != verify_certificate(certificate, &(bl1_data->sp_certificates[1]),
                                ESPERANTO_CERTIFICATE_DESIGNATION_BL2_CA)) {
        printx("verify_bl2_certificate: verify_certificate(0 failed!\n");
        return -1;
    }
    return 0;
}
