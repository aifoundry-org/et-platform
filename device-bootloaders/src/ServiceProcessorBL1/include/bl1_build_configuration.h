#ifndef __BL1_BUILD_CONFIGURATION_H__
#define __BL1_BUILD_CONFIGURATION_H__

#include <stdint.h>

#include "esperanto_signed_image_format/executable_image.h"

#include "../../ServiceProcessorBL1/include/build_configuration.h"

// the esperanto_sign_elf tool will search the elf binary for a
// symbol named 'g_image_version_info'.
// The symbol is expected to be of type IMAGE_VERSION_INFO_t as defined in esperanto_executable_image.h
//
// if the symbol size is equal to sizeof(IMAGE_VERSION_INFO_t)
// and the IMAGE_VERSION_INFO_t.prolog value is equal to IMAGE_VERSION_INFO_PROLOG_TAG
// and the IMAGE_VERSION_INFO_t.epilog value is equal to IMAGE_VERSION_INFO_EPILOG_TAG
// then the esperanto_sign_elf tool will use the file_version, git_hash and git_version
// to automatically initialize the signed header structure

static IMAGE_VERSION_INFO_t g_image_version_info __attribute__((used)) = {
    .prolog_tag = IMAGE_VERSION_INFO_PROLOG_TAG,
    .file_version_revision = FILE_REVISION_NUMBER,
    .file_version_minor = FILE_VERSION_MINOR,
    .file_version_major = FILE_VERSION_MAJOR,
    .git_hash = GIT_HASH_ARRAY,
    .git_version = GIT_VERSION_ARRAY,
    .epilog_tag = IMAGE_VERSION_INFO_EPILOG_TAG
} ;

const IMAGE_VERSION_INFO_t *get_image_version_info(void);

const IMAGE_VERSION_INFO_t *get_image_version_info(void)
{
    return &g_image_version_info;
}

#endif
