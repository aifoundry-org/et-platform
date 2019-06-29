#include "mm_build_configuration.h"
#include "build_configuration.h"

// the esperanto_sign_elf tool will search the elf binary for a 
// symbol named 'g_image_version_info'.
// The symbol is expected to be of type IMAGE_VERSION_INFO_t as defined in esperanto_executable_image.h
//
// typedef struct IMAGE_VERSION_INFO {
//     unsigned int prolog_tag;
//     union {
//         uint32_t file_version;
//         struct {
//             uint8_t file_version_revision;
//             uint8_t file_version_minor;
//             uint8_t file_version_major;
//             uint8_t file_version_reserved;
//         };
//     };
//     unsigned char git_hash[32];
//     char git_version[MAX_GIT_VERSION_LENGTH];
//     unsigned int epilog_tag;
// } IMAGE_VERSION_INFO_t;
//
// if the symbol size is equal to sizeof(IMAGE_VERSION_INFO_t)
// and the IMAGE_VERSION_INFO_t.prolog value is equal to IMAGE_VERSION_INFO_PROLOG_TAG
// and the IMAGE_VERSION_INFO_t.epilog value is equal to IMAGE_VERSION_INFO_EPILOG_TAG
// then the esperanto_sign_elf tool will use the file_version, git_hash and git_version
// to automatically initialize the signed header structure

/*__attribute__((used))*/ static const IMAGE_VERSION_INFO_t IMAGE_VERSION_INFO_SYMBOL = {
    .prolog_tag = IMAGE_VERSION_INFO_PROLOG_TAG,
    .file_version_revision = FILE_REVISION_NUMBER,
    .file_version_minor = FILE_VERSION_MINOR,
    .file_version_major = FILE_VERSION_MAJOR,
    .git_hash = GIT_HASH_ARRAY,
    .git_version = GIT_VERSION_ARRAY,
    .epilog_tag = IMAGE_VERSION_INFO_EPILOG_TAG
};

const IMAGE_VERSION_INFO_t * get_image_version_info(void);

const IMAGE_VERSION_INFO_t * get_image_version_info(void) {
    return &IMAGE_VERSION_INFO_SYMBOL;
}
