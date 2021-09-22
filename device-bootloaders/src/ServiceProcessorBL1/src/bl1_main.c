#include "etsoc/drivers/serial/serial.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "printx.h"

#include "system_registers.h"
#include "service_processor_ROM_data.h"
#include "service_processor_BL1_data.h"
#include "bl1_sp_firmware_loader.h"
#include "bl1_flash_fs.h"
#include "bl1_main.h"
#include "bl1_timer.h"
#include "bl1_crypto.h"
#include "bl1_build_configuration.h"
#include "sp_otp.h"
#include "etsoc/isa/mem-access/io.h"

#include "hwinc/sp_cru_reset.h"
#include "hwinc/hal_device.h"

#if TEST_FRAMEWORK
#include "tf.h"
#endif // TEST_FRAMEWORK

//#define MINIMAL_IMAGE

#ifndef MINIMAL_IMAGE
SERVICE_PROCESSOR_BL1_DATA_t g_service_processor_bl1_data;

SERVICE_PROCESSOR_BL1_DATA_t *get_service_processor_bl1_data(void)
{
    return &g_service_processor_bl1_data;
}

bool is_vaultip_disabled(void)
{
    uint32_t rm_status2;
    static bool initialized = false;
    static bool vaultip_disabled = false;

    if (!initialized) {
        if (0 != sp_otp_get_vaultip_chicken_bit(&vaultip_disabled)) {
            vaultip_disabled = false;
        }

        rm_status2 = ioread32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_STATUS2_ADDRESS);
        if (0 != RESET_MANAGER_RM_STATUS2_A0_UNLOCK_GET(rm_status2) &&
            0 != RESET_MANAGER_RM_STATUS2_SKIP_VAULT_GET(rm_status2)) {
            vaultip_disabled = true;
        }
    }

    return vaultip_disabled;
}

static inline void evict_dcache(void)
{
    // use_tmask=0, dst=1 (L2/SP_RAM), set=0, way=0, num_lines=15
    uint64_t value = (1ull << 58) + 15ull;

    __asm__ __volatile__(
        // Wait for previous memory accesses to finish
        "fence\n"
        // Evict L1 Dcache: EvictSW for the 4 ways
        "csrw evict_sw, %0\n"
        "addi %0, %0, 64\n"
        "csrw evict_sw, %0\n"
        "addi %0, %0, 64\n"
        "csrw evict_sw, %0\n"
        "addi %0, %0, 64\n"
        "csrw evict_sw, %0\n"
        // Wait for the evicts to complete
        "csrwi tensor_wait, 6\n"
        : "=r"(value)
        : "0"(value)
        : "memory");
}

static inline void invalidate_icache(void)
{
    __asm__ __volatile__(
        // Wait for previous memory accesses to finish
        "fence\n"
        // Invalidate L1 Icache
        "csrwi cache_invalidate, 1\n"
        :
        :
        : "memory");
}

typedef int (*BL2_MAIN_PFN)(const SERVICE_PROCESSOR_BL1_DATA_t *data);

static void invoke_sp_bl2(void)
{
    union {
        struct {
            uint32_t lo;
            uint32_t hi;
        };
        uint64_t u64;
        BL2_MAIN_PFN pFN;
    } bl2_address;
    bl2_address.lo = g_service_processor_bl1_data.sp_bl2_header.info.image_info_and_signaure.info
                         .secret_info.exec_address_lo;
    bl2_address.hi = g_service_processor_bl1_data.sp_bl2_header.info.image_info_and_signaure.info
                         .secret_info.exec_address_hi;
    printx("Invoking SP BL2 @ 0x%" PRIx64 "!\r\n", bl2_address.u64);

    // Evict Dcache and invalidate Icache before jumping to BL2
    evict_dcache();
    invalidate_icache();

    bl2_address.pFN(&g_service_processor_bl1_data);
}

static int copy_rom_data(const SERVICE_PROCESSOR_ROM_DATA_t *rom_data)
{
    printx("SP ROM data address: %x\n", rom_data);
    if (NULL == rom_data ||
        sizeof(SERVICE_PROCESSOR_ROM_DATA_t) != rom_data->service_processor_rom_data_size ||
        SERVICE_PROCESSOR_ROM_DATA_VERSION != rom_data->service_processor_rom_version) {
        printx("Invalid ROM DATA!\n");
        return -1;
    }

    g_service_processor_bl1_data.service_processor_rom_version =
        rom_data->service_processor_rom_version;
    g_service_processor_bl1_data.sp_gpio_pins = rom_data->sp_gpio_pins;
    g_service_processor_bl1_data.sp_pll0_frequency = rom_data->sp_pll0_frequency;
    g_service_processor_bl1_data.sp_pll1_frequency = rom_data->sp_pll1_frequency;
    g_service_processor_bl1_data.pcie_pll0_frequency = rom_data->pcie_pll0_frequency;
    g_service_processor_bl1_data.timer_raw_ticks_before_pll_turned_on =
        rom_data->timer_raw_ticks_before_pll_turned_on;
    g_service_processor_bl1_data.vaultip_coid_set = rom_data->vaultip_coid_set;
    g_service_processor_bl1_data.spi_controller_rx_baudrate_divider =
        rom_data->spi_controller_rx_baudrate_divider;
    g_service_processor_bl1_data.spi_controller_tx_baudrate_divider =
        rom_data->spi_controller_tx_baudrate_divider;

    // copy the SP ROOT/ISSUING CA certificates chain
    memcpy(&(g_service_processor_bl1_data.sp_certificates), &(rom_data->sp_certificates),
           sizeof(rom_data->sp_certificates));

    // copy the SP BL1 header
    memcpy(&(g_service_processor_bl1_data.sp_bl1_header), &(rom_data->sp_bl1_header),
           sizeof(rom_data->sp_bl1_header));

    return 0;
}
#endif

int bl1_main(const SERVICE_PROCESSOR_ROM_DATA_t *rom_data);

int bl1_main(const SERVICE_PROCESSOR_ROM_DATA_t *rom_data)
{
#if TEST_FRAMEWORK
    printx("\n** SP BL1 STARTED - TF **\r\n");
    // Control does not return from call below
    TF_Wait_And_Process_TF_Cmds(TF_DEFAULT_ENTRY);
    TF_Wait_And_Process_TF_Cmds(TF_BL1_ENTRY);
#endif  // TEST_FRAMEWORK

    bool disable_vault;

    const IMAGE_VERSION_INFO_t *image_version_info = get_image_version_info();
    //SERIAL_init(UART0);
    printx("\n*** SP BL1 STARTED ***\r\n");
    printx("File version %u.%u.%u\n", image_version_info->file_version_major,
           image_version_info->file_version_minor, image_version_info->file_version_revision);
    printx("GIT version: %s\n", GIT_VERSION_STRING);
    printx("GIT hash: %s\n", GIT_HASH_STRING);

    volatile uint32_t *rom = (uint32_t *)0x40000000;
    printx("ROM: %08x %08x %08x %08x\n", rom[0], rom[1], rom[2], rom[3]);

#ifdef MINIMAL_IMAGE
    (void)rom_data;
#else
    memset(&g_service_processor_bl1_data, 0, sizeof(g_service_processor_bl1_data));
    g_service_processor_bl1_data.service_processor_bl1_data_size =
        sizeof(g_service_processor_bl1_data);
    g_service_processor_bl1_data.service_processor_bl1_version = SERVICE_PROCESSOR_BL1_DATA_VERSION;

    g_service_processor_bl1_data.service_processor_bl1_image_file_version_minor = image_version_info->file_version_minor;
    g_service_processor_bl1_data.service_processor_bl1_image_file_version_major =  image_version_info->file_version_major;
    g_service_processor_bl1_data.service_processor_bl1_image_file_version_revision = image_version_info->file_version_revision;

    if (0 != copy_rom_data(rom_data)) {
        printx("copy_rom_data() failed!!\n");
        goto FATAL_ERROR;
    }

    timer_init();

    if (0 != sp_otp_init()) {
        printx("sp_otp_init() failed!!\n");
        goto FATAL_ERROR;
    }

    disable_vault = is_vaultip_disabled();

    if (false == disable_vault) {
        MESSAGE_INFO("CE DIS\n");
    } else {
        if (0 != crypto_init()) {
            printx("crypto_init() failed!!\n");
            goto FATAL_ERROR;
        }
    }

    if (0 != flash_fs_init(&(g_service_processor_bl1_data.flash_fs_bl1_info),
                           &(rom_data->flash_fs_rom_info))) {
        printx("flash_fs_init() failed!!\n");
        goto FATAL_ERROR;
    }

    if (0 != load_bl2_firmware()) {
        printx("load_bl2_firmware() failed!!\n");
        goto FATAL_ERROR;
    }

    printx("time: %lu\n", timer_get_ticks_count());

    invoke_sp_bl2();
    goto HALT;

FATAL_ERROR:
    printx("BOOT FAILED! Waiting for reset!\r\n");
    printx("*** SP BL1 FINISHED ***\r\n");
    goto HALT;
#endif

HALT:
    while (1) {
    }
}
