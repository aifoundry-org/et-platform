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

#include <string.h>
#include "printx.h"

#include "bl1_main.h"
#include "bl1_sp_certificates.h"
#include "bl1_flash_fs.h"
#include "bl1_sp_certificates.h"
#include "bl1_sp_firmware_loader.h"
#include "bl1_crypto.h"

static int verify_bl2_image_file_header(const ESPERANTO_IMAGE_FILE_HEADER_t * sp_bl2_file_header, uint32_t sp_bl2_size) {
    if (NULL == sp_bl2_file_header) {
        return -1;
    }

    if (CURRENT_EXEC_FILE_HEADER_TAG != sp_bl2_file_header->info.file_header_tag) {
        printx("load_bl2_firmware: invalid file header tag (%08x)!\n", sp_bl2_file_header->info.file_header_tag);
        return -1;
    }

    if (CURRENT_EXEC_IMAGE_HEADER_TAG != sp_bl2_file_header->info.image_info_and_signaure.info.public_info.header_tag) {
        printx("load_bl2_firmware: invalid image header tag (%08x)!\n", sp_bl2_file_header->info.image_info_and_signaure.info.public_info.header_tag);
        return -1;
    }

    if (ESPERANTO_IMAGE_TYPE_SP_BL2 != sp_bl2_file_header->info.image_info_and_signaure.info.public_info.image_type) {
        printx("load_bl2_firmware: not a SP BL1 image type! (%08x)!\n", sp_bl2_file_header->info.image_info_and_signaure.info.public_info.image_type);
        return -1;
    }

    if ((sizeof(ESPERANTO_IMAGE_FILE_HEADER_t) + sp_bl2_file_header->info.image_info_and_signaure.info.public_info.code_and_data_size) != sp_bl2_size) {
        printx("load_bl2_firmware: invalid file size!\n");
        return -1;
    }

    if (0 != verify_bl2_certificate(&(sp_bl2_file_header->info.signing_certificate))) {
        printx("SP BL2 certificate is not valid!\n");
        return -1;
    } else {
        printx("SP BL2 certificate OK!\n");
    }

    if (0 != crypto_verify_pk_signature(&(sp_bl2_file_header->info.signing_certificate.certificate_info.subject_public_key), 
                                        &(sp_bl2_file_header->info.image_info_and_signaure.info_signature),
                                        &(sp_bl2_file_header->info.image_info_and_signaure.info), 
                                        sizeof(sp_bl2_file_header->info.image_info_and_signaure.info))) {
        printx("bl2_firmware signature is not valid!\n");
        return -1;
    } else {
        printx("bl2_firmware signature is OK!\n");
    }

    return 0;
}

static int load_bl2_code_and_data(const ESPERANTO_IMAGE_INFO_SECRET_t * load_info) {
    uint32_t load_offset;
    union {
        uint64_t u64;
        struct {
            uint32_t lo;
            uint32_t hi;
        };
    } load_address;
    uint32_t region_no;
    //SERVICE_PROCESSOR_bl1_data_t * bl1_data = get_service_processor_bl1_data();

    for (region_no = 0; region_no < load_info->load_regions_count; region_no++) {
        load_offset = (uint32_t)(sizeof(ESPERANTO_IMAGE_FILE_HEADER_t) + load_info->load_regions[region_no].region_offset);
        load_address.lo = load_info->load_regions[region_no].load_address_lo;
        load_address.hi = load_info->load_regions[region_no].load_address_hi;
        printx("Region %u: load=0x%x, addr=0x%x, fsize=0x%x, msize=0x%x\n", region_no, load_offset, load_address.lo, load_info->load_regions[region_no].load_size, load_info->load_regions[region_no].memory_size);

        if (load_info->load_regions[region_no].load_size > 0) {
            if (0 != flash_fs_read_file(ESPERANTO_FLASH_REGION_ID_SP_BL2, load_offset, (void*)load_address.u64, load_info->load_regions[region_no].load_size)) {
                printx("load_bl2_firmware: flash_fs_read_file(code) failed!\n");
                return -1;
            }
            printx("loaded 0x%x bytes at 0x%08x\n", load_info->load_regions[region_no].load_size, load_address.u64);
        }
        if (load_info->load_regions[region_no].memory_size > load_info->load_regions[region_no].load_size) {
            memset((void*)(load_address.u64 + load_info->load_regions[region_no].load_size), 0, load_info->load_regions[region_no].memory_size - load_info->load_regions[region_no].load_size);
            printx("erased 0x%x bytes\n", load_info->load_regions[region_no].memory_size - load_info->load_regions[region_no].load_size);
        }
    }

    return 0;
}
int load_bl2_firmware(void) {
    uint32_t sp_bl2_size;
    SERVICE_PROCESSOR_BL1_DATA_t * bl1_data = get_service_processor_bl1_data();

    // load the SP BL2 image
    if (0 != flash_fs_get_file_size(ESPERANTO_FLASH_REGION_ID_SP_BL2, &sp_bl2_size)) {
        printx("load_bl2_firmware: flash_fs_get_file_size(header) failed!\n");
        return -1;
    }
    if (sp_bl2_size < sizeof(ESPERANTO_IMAGE_FILE_HEADER_t)) {
        printx("load_bl2_firmware: SP_BL2 image file too small!");
        return -1;
    }
    if (0 != flash_fs_read_file(ESPERANTO_FLASH_REGION_ID_SP_BL2, 0, &(bl1_data->sp_bl2_header), sizeof(bl1_data->sp_bl2_header))) {
        printx("load_bl2_firmware: flash_fs_read_file(SP_BL2) failed!\n");
        return -1;
    }

    if (0 != verify_bl2_image_file_header(&bl1_data->sp_bl2_header, sp_bl2_size)) {
        printx("load_bl2_firmware: verify_bl2_image_file_header() failed!\n");
        return -1;
    }

    if (0 != load_bl2_code_and_data(&bl1_data->sp_bl2_header.info.image_info_and_signaure.info.secret_info)) {
        printx("load_bl2_firmware: load_bl2_code_and_data() failed!\n");
        return -1;
    }

    printx("load_bl2_firmware: Loaded SP_BL2 firmware.\n");

    return 0;
}
