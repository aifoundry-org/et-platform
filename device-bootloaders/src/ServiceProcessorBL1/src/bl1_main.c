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
#include "etsoc/isa/io.h"

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

    if (!initialized)
    {
        if (0 != sp_otp_get_vaultip_chicken_bit(&vaultip_disabled))
        {
            vaultip_disabled = false;
        }

        rm_status2 = ioread32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_STATUS2_ADDRESS);
        if (0 != RESET_MANAGER_RM_STATUS2_A0_UNLOCK_GET(rm_status2) &&
            0 != RESET_MANAGER_RM_STATUS2_SKIP_VAULT_GET(rm_status2))
        {
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
    union
    {
        struct
        {
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
    printx("Invoking SP BL2 !\r\n");
    //printx("Invoking SP BL2 @ 0x%" PRIx64 "!\r\n", bl2_address.u64);

    // Evict Dcache and invalidate Icache before jumping to BL2
    evict_dcache();
    invalidate_icache();

    bl2_address.pFN(&g_service_processor_bl1_data);
}

static int copy_partition_info_data(ESPERANTO_PARTITION_BL1_INFO_t *bl1_partition_info,
                                    const ESPERANTO_PARTITION_ROM_INFO_t *rom_partition_info)
{
    if (NULL == bl1_partition_info || NULL == rom_partition_info)
    {
        printx("copy_partition_info_data: invalid arguments!\n");
        return -1;
    }

    memcpy(&(bl1_partition_info->header), &(rom_partition_info->header),
           sizeof(rom_partition_info->header));
    memcpy(&(bl1_partition_info->regions_table), &(rom_partition_info->regions_table),
           sizeof(rom_partition_info->regions_table));

    bl1_partition_info->priority_designator_region_index =
        rom_partition_info->priority_designator_region_index;
    bl1_partition_info->boot_counters_region_index = rom_partition_info->boot_counters_region_index;
    bl1_partition_info->configuration_data_region_index =
        rom_partition_info->configuration_data_region_index;
    bl1_partition_info->vaultip_fw_region_index = rom_partition_info->vaultip_fw_region_index;
    bl1_partition_info->pcie_config_region_index = rom_partition_info->pcie_config_region_index;
    bl1_partition_info->sp_certificates_region_index =
        rom_partition_info->sp_certificates_region_index;
    bl1_partition_info->sp_bl1_region_index = rom_partition_info->sp_bl1_region_index;

    memcpy(&(bl1_partition_info->priority_designator_region_data),
           &(rom_partition_info->priority_designator_region_data),
           sizeof(rom_partition_info->priority_designator_region_data));
    memcpy(&(bl1_partition_info->boot_counters_region_data),
           &(rom_partition_info->boot_counters_region_data),
           sizeof(rom_partition_info->boot_counters_region_data));

    bl1_partition_info->priority_counter = rom_partition_info->priority_counter;
    bl1_partition_info->attempted_boot_counter = rom_partition_info->attempted_boot_counter;
    bl1_partition_info->completed_boot_counter = rom_partition_info->completed_boot_counter;
    bl1_partition_info->partition_valid = rom_partition_info->partition_valid;

    return 0;
}

static int copy_flash_fs_data(FLASH_FS_BL1_INFO_t *flash_fs_bl1_info,
                              const FLASH_FS_ROM_INFO_t *flash_fs_rom_info)
{
    if (NULL == flash_fs_bl1_info || NULL == flash_fs_rom_info)
    {
        printx("copy_flash_fs_data: invalid arguments!\n");
        return -1;
    }

    memset(flash_fs_bl1_info, 0, sizeof(FLASH_FS_BL1_INFO_t));

    for (uint32_t n = 0; n < 2; n++)
    {
        if (0 != copy_partition_info_data(&(flash_fs_bl1_info->partition_info[n]),
                                          &(flash_fs_rom_info->partition_info[n])))
        {
            printx("copy_partition_info_data(%u) failed!\n", n);
            return -1;
        }
    }

    flash_fs_bl1_info->flash_id_u32 = flash_fs_rom_info->flash_id_u32;
    flash_fs_bl1_info->flash_size = flash_fs_rom_info->flash_size;
    flash_fs_bl1_info->active_partition = flash_fs_rom_info->active_partition;
    flash_fs_bl1_info->other_partition_valid = flash_fs_rom_info->other_partition_valid;

    uint32_t region_index;
    uint32_t region_address;
    //uint32_t region_size;
    region_index = flash_fs_rom_info->partition_info[flash_fs_rom_info->active_partition]
                       .configuration_data_region_index;
    region_address = flash_fs_rom_info->partition_info[flash_fs_rom_info->active_partition]
                         .regions_table[region_index]
                         .region_offset *
                     FLASH_PAGE_SIZE;
    //region_size = flash_fs_rom_info->partition_info[flash_fs_rom_info->active_partition].regions_table[region_index].region_reserved_size * FLASH_PAGE_SIZE;
    //printx("region_index configuration data is 0x%08x \n", region_index);
    //printx("region_address configuration data is 0x%08x \n", region_address);
    //printx("region_size configuration data is 0x%08x \n", region_size);
    flash_fs_bl1_info->configuration_region_address = region_address;
    flash_fs_bl1_info->pcie_config_file_info = flash_fs_rom_info->pcie_config_file_info;
    flash_fs_bl1_info->vaultip_firmware_file_info = flash_fs_rom_info->vaultip_firmware_file_info;
    flash_fs_bl1_info->sp_certificates_file_info = flash_fs_rom_info->sp_certificates_file_info;
    flash_fs_bl1_info->sp_bl1_file_info = flash_fs_rom_info->sp_bl1_file_info;

    return 0;
}

static int copy_rom_data(const SERVICE_PROCESSOR_ROM_DATA_t *rom_data)
{
    //printx("SP ROM data address: %x\n", rom_data);
    if (NULL == rom_data ||
        sizeof(SERVICE_PROCESSOR_ROM_DATA_t) != rom_data->service_processor_rom_data_size ||
        SERVICE_PROCESSOR_ROM_DATA_VERSION != rom_data->service_processor_rom_version)
    {
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

    // copy the SP flash_fs info
    if (0 != copy_flash_fs_data(&g_service_processor_bl1_data.flash_fs_bl1_info,
                                &rom_data->flash_fs_rom_info))
    {
        printx("copy_flash_fs_data() failed!");
        return -1;
    }

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
#endif // TEST_FRAMEWORK

    bool disable_vault;

    const IMAGE_VERSION_INFO_t *image_version_info = get_image_version_info();
    //SERIAL_init(UART0);
    printx("\n*** SP BL1 STARTED ***\r\n");
    printx("File version %u.%u.%u\n", image_version_info->file_version_major,
           image_version_info->file_version_minor, image_version_info->file_version_revision);
    printx("GIT version: %s\n", GIT_VERSION_STRING);
    printx("GIT hash: %s\n", GIT_HASH_STRING);

#ifdef MINIMAL_IMAGE
    (void)rom_data;
#else
    memset(&g_service_processor_bl1_data, 0, sizeof(g_service_processor_bl1_data));
    g_service_processor_bl1_data.service_processor_bl1_data_size =
        sizeof(g_service_processor_bl1_data);
    g_service_processor_bl1_data.service_processor_bl1_version = SERVICE_PROCESSOR_BL1_DATA_VERSION;

    g_service_processor_bl1_data.service_processor_bl1_image_file_version_minor =
        image_version_info->file_version_minor;
    g_service_processor_bl1_data.service_processor_bl1_image_file_version_major =
        image_version_info->file_version_major;
    g_service_processor_bl1_data.service_processor_bl1_image_file_version_revision =
        image_version_info->file_version_revision;

    if (0 != copy_rom_data(rom_data))
    {
        printx("copy_rom_data() failed!!\n");
        goto FATAL_ERROR;
    }

    timer_init();

    if (0 != sp_otp_init())
    {
        printx("sp_otp_init() failed!!\n");
        goto FATAL_ERROR;
    }

    disable_vault = is_vaultip_disabled();

    if (false == disable_vault)
    {
        MESSAGE_INFO("CE DIS\n");
    }
    else
    {
        if (0 != crypto_init())
        {
            printx("crypto_init() failed!!\n");
            goto FATAL_ERROR;
        }
    }

    if (0 != spi_flash_init(g_service_processor_bl1_data.flash_fs_bl1_info.flash_id))
    {
        printx("spi_flash_init() failed!!\n");
        goto FATAL_ERROR;
    }

    if (0 != flash_fs_init(&g_service_processor_bl1_data.flash_fs_bl1_info))
    {
        printx("flash_fs_init() failed!!\n");
        goto FATAL_ERROR;
    }

    if (0 != load_bl2_firmware())
    {
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
    while (1)
    {
        /* Spin is loop if code jumps here
         Only error cases can jump here */
    }
}
