#include "et_cru.h"
#include "serial.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "printx.h"

#include "system_registers.h"
#include "service_processor_ROM_data.h"
#include "service_processor_BL1_data.h"
#include "bl1_sp_firmware_loader.h"
#include "bl1_flash_fs.h"
#include "bl1_main.h"
#include "build_configuration.h"

SERVICE_PROCESSOR_BL1_DATA_t g_service_processor_bl1_data;
//static bool normal_boot_priority = true;

SERVICE_PROCESSOR_BL1_DATA_t * get_service_processor_bl1_data(void) {
    return &g_service_processor_bl1_data;
}

typedef int (*BL2_MAIN_PFN)(const SERVICE_PROCESSOR_BL1_DATA_t * data);

static void invoke_sp_bl2(void) {
    union {
        struct {
            uint32_t lo;
            uint32_t hi;
        };
        uint64_t u64;
        BL2_MAIN_PFN pFN;
    } bl2_address;
    bl2_address.lo = g_service_processor_bl1_data.sp_bl2_header.info.image_info_and_signaure.info.secret_info.exec_address_lo;
    bl2_address.hi = g_service_processor_bl1_data.sp_bl2_header.info.image_info_and_signaure.info.secret_info.exec_address_hi;
    printx("Invoking SP BL2 @ 0x%08x_%08x!\r\n", bl2_address.hi, bl2_address.lo);
    bl2_address.pFN(&g_service_processor_bl1_data);
}

static int copy_rom_data(const SERVICE_PROCESSOR_ROM_DATA_t * rom_data) {
    printx("SP ROM data address: %x\n", rom_data);
    if (NULL == rom_data || sizeof(SERVICE_PROCESSOR_ROM_DATA_t) != rom_data->service_processor_rom_data_size || SERVICE_PROCESSOR_ROM_DATA_VERSION != rom_data->service_processor_rom_version) {
        printx("Invalid ROM DATA!\n");
        return -1;
    }

    g_service_processor_bl1_data.service_processor_rom_version = rom_data->service_processor_rom_version;

    // copy the SP ROOT/ISSUING CA certificates chain
    memcpy(&(g_service_processor_bl1_data.sp_certificates), &(rom_data->sp_certificates), sizeof(rom_data->sp_certificates));

    // copy the SP BL1 header
    memcpy(&(g_service_processor_bl1_data.sp_bl1_header), &(rom_data->sp_bl1_header), sizeof(rom_data->sp_bl1_header));

    return 0;
}

int bl1_main(const SERVICE_PROCESSOR_ROM_DATA_t * rom_data);

int bl1_main(const SERVICE_PROCESSOR_ROM_DATA_t * rom_data)
{
    //SERIAL_init(UART0);
    printx("\n*** SP BL1 STARTED ***\r\n");
    printx("File version %u.%u.%u\n", FILE_VERSION_MAJOR, FILE_VERSION_MINOR, FILE_REVISION_NUMBER);
    printx("GIT version: %s\n", GIT_VERSION_STRING);
    printx("GIT hash: %s\n", GIT_HASH_STRING);

    memset(&g_service_processor_bl1_data, 0, sizeof(g_service_processor_bl1_data));
    g_service_processor_bl1_data.service_processor_bl1_data_size = sizeof(g_service_processor_bl1_data);
    g_service_processor_bl1_data.service_processor_bl1_version = SERVICE_PROCESSOR_BL1_DATA_VERSION;

    printx("SP ROM data address: %x\n", rom_data);
    if (0 != copy_rom_data(rom_data)) {
        printx("copy_rom_data() failed!!\n");
        goto FATAL_ERROR;
    }

    if (0 != flash_fs_init(&(g_service_processor_bl1_data.flash_fs_bl1_info), &(rom_data->flash_fs_rom_info))) {
        printx("flash_fs_init() failed!!\n");
        goto FATAL_ERROR;
    }

    if (0 != load_bl2_firmware()) {
        printx("load_bl2_firmware() failed!!\n");
        goto FATAL_ERROR;
    }

    invoke_sp_bl2();
    for (;;);

FATAL_ERROR:
    printx("BOOT FAILED! Waiting for reset!\r\n");
    for (;;);

    printx("*** SP BL1 FINISHED ***\r\n");

    while (1) {}
}
