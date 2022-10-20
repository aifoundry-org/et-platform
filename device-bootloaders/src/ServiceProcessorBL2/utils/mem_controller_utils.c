/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/

#include <stdint.h>
#include "delays.h"
#include "log.h"

#include "bl2_scratch_buffer.h"
#include "bl2_flashfs_driver.h"

#include "hwinc/hal_device.h"
#include "hwinc/ddrc_reg_def.h"
#include "hwinc/etsoc_mem_shire_esr.h"
#include "hwinc/ms_reg_def.h"
#include "hal_ddr_init.h"

#include "mem_controller.h"

#define MR_DENSITY         8
#define MR_VENDOR_ID       5
#define SET_MR_ADDRESS(mr) (mr << 8)
#define MR_SET_READ        ((0 << 31) | 1)
#define MR_READ            (uint32_t)((1 << 31) | 1)
/*
** Private functions/macros/variables used only in this file
*/

static ms_dram_status_t gs_dram_status = { UNINITIALIZED,
                                           { UNINITIALIZED, UNINITIALIZED, UNINITIALIZED,
                                             UNINITIALIZED, UNINITIALIZED, UNINITIALIZED,
                                             UNINITIALIZED, UNINITIALIZED } };

#define GET_MEMSHIRE_ESR(memshire, reg) \
    ((((uint64_t)memshire & 0x7) | 0xe8) << 22 | reg) // 0xe8=232

const ms_dram_status_t *ms_get_dram_status(void)
{
    return &gs_dram_status;
}

static void ms_setup_dram_status(void)
{
    for (int index = 0; index < HW_NUMBER_OF_MEMSHIRE; ++index)
    {
        if (index < MEMSHIRE_BASE || index >= (MEMSHIRE_BASE + NUMBER_OF_MEMSHIRE))
            gs_dram_status.physical_memshire_status[index] = DISABLED;
    }
}

static void ms_set_dram_status_physical_memshire(uint32_t memshire, ms_status_t status)
{
    // memshire is a logical memshire index
    if (memshire > HW_NUMBER_OF_MEMSHIRE)
        return;
    gs_dram_status.physical_memshire_status[memshire] = status;
}

// This function updates the system status based on each memshire's status
static void ms_update_dram_status_system_memshire(void)
{
    gs_dram_status.system_status = 0x0;
    for (int index = MEMSHIRE_BASE; index < MEMSHIRE_BASE + NUMBER_OF_MEMSHIRE; ++index)
    {
        if (gs_dram_status.physical_memshire_status[index] == FAILED ||
            gs_dram_status.physical_memshire_status[index] == UNINITIALIZED)
        {
            gs_dram_status.system_status |= (0x1u << index);
        }
    }
}

static inline uint32_t ms_peek_ddrc_reg(uint32_t memshire, uint32_t blk, uint64_t reg,
                                        uint32_t wait_value, uint32_t wait_mask)
{
    uint32_t value = ms_read_ddrc_reg(memshire, blk, reg);
    return (value ^ wait_value) & wait_mask;
}

static inline volatile void *get_ddrc_address(uint32_t memshire, uint32_t block, uint64_t reg)
{
    if (block == 1)
        reg |= 0x1000;
    return (volatile void *)(R_SHIRE_LPDDR_BASEADDR | ((uint64_t)memshire & 0x7) << 26 | reg);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
static uint32_t load_raw_image(ESPERANTO_FLASH_REGION_ID_t region_id, void *buffer,
                               uint32_t buffer_size)
{
    ESPERANTO_RAW_IMAGE_FILE_HEADER_t image_file_header;
    uint32_t image_file_size;
    const char *image_name;

    switch (region_id)
    {
        case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING:
            image_name = "DDR_TRAINING_1D_I";
            break;
        case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_PAYLOAD_800MHZ:
            image_name = "DDR_TRAINING_1D_800_D";
            break;
        case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_PAYLOAD_933MHZ:
            image_name = "DDR_TRAINING_1D_933_D";
            break;
        case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_PAYLOAD_1067MHZ:
            image_name = "DDR_TRAINING_1D_1067_D";
            break;
        case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D:
            image_name = "DDR_TRAINING_2D_I";
            break;
        case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D_PAYLOAD_800MHZ:
            image_name = "DDR_TRAINING_2D_800_D";
            break;
        case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D_PAYLOAD_933MHZ:
            image_name = "DDR_TRAINING_2D_933_D";
            break;
        case ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D_PAYLOAD_1067MHZ:
            image_name = "DDR_TRAINING_2D_1067_D";
            break;
        default:
            Log_Write(LOG_LEVEL_ERROR, "load_raw_image: Invalid region id %d\n", region_id);
            return 0;
    }
    Log_Write(LOG_LEVEL_INFO, "load_raw_image: Loading %s firmware\n", image_name);

    if (0 != flashfs_drv_get_file_size(region_id, &image_file_size))
    {
        Log_Write(LOG_LEVEL_ERROR, "load_raw_image: flashfs_drv_get_file_size(%s) failed!\n",
                  image_name);
        return 0;
    }
    if (image_file_size < sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t))
    {
        Log_Write(LOG_LEVEL_ERROR, "load_raw_image: %s image file too small!\n", image_name);
        return 0;
    }
    if (0 != flashfs_drv_read_file(region_id, 0, &image_file_header,
                                   sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "load_raw_image: flashfs_drv_read_file(%s header) failed!\n",
                  image_name);
        return 0;
    }
    Log_Write(LOG_LEVEL_DEBUG, "Loaded %s header...\n", image_name);

    image_file_size = image_file_header.info.image_info_and_signaure.info.raw_image_size;
    if (image_file_size > buffer_size)
    {
        Log_Write(LOG_LEVEL_ERROR,
                  "load_raw_image: buffer too small to fit %s image file at %d bytes!\n",
                  image_name, image_file_size);
        return 0;
    }

    if (0 != flashfs_drv_read_file(region_id, sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t), buffer,
                                   image_file_size))
    {
        Log_Write(LOG_LEVEL_ERROR, "load_raw_image: flashfs_drv_read_file(code) failed!\n");
        return 0;
    }

    Log_Write(LOG_LEVEL_DEBUG, "load_raw_image: Loaded %s firmware with %d bytes\n", image_name,
              image_file_size);
    return image_file_size;
}
#pragma GCC diagnostic pop

static uint64_t program_all_ddrc(uint64_t target_address, const void *buffer, uint32_t size)
{
    uint32_t memshire;
    const uint32_t *ptr = buffer;

    Log_Write(LOG_LEVEL_INFO, "program_all_ddrc: %d bytes to 0x%016lx\n", size, target_address);

    while (((const uint8_t *)ptr - (const uint8_t *)buffer) < size)
    {
        FOR_EACH_MEMSHIRE(ms_write_ddrc_addr(memshire, target_address, *ptr);)
        target_address += 4;
        ++ptr;
    }

#if (DDR_DIAG & DDR_DIAG_VERIFY_SRAM_WRITE)
    uint32_t read_back;
    ptr = buffer;
    target_address -= (size + 3) / 4 * 4; // round up to multiple of 4
    while (((const uint8_t *)ptr - (const uint8_t *)buffer) < size)
    {
        FOR_EACH_MEMSHIRE(read_back = ms_read_ddrc_reg(memshire, 0, target_address); if (read_back !=
                                                                                         *ptr) {
            Log_Write(
                LOG_LEVEL_CRITICAL,
                "DDR:[%d][txt]program_all_ddrc() write verification fails at address 0x%016lx\n",
                memshire, target_address);
            Log_Write(
                LOG_LEVEL_CRITICAL,
                "DDR:[%d][txt]program_all_ddrc() write verification (written 0x%08x, read back 0x%08x)\n",
                memshire, *ptr, read_back);
        })
        target_address += 4;
        ++ptr;
    }
#endif // (DDR_DIAG & DDR_DIAG_VERIFY_SRAM_WRITE)

    Log_Write(LOG_LEVEL_DEBUG, "program_all_ddrc: done till address 0x%016lx\n", target_address);
    return target_address;
}

static void load_training_fw(ESPERANTO_FLASH_REGION_ID_t region_instruction,
                             ESPERANTO_FLASH_REGION_ID_t region_data)
{
    void *buffer;
    uint32_t buffer_size;
    uint32_t real_size;
    uint64_t target_address = DDRC_PMU_SRAM;
    uint32_t memshire; // for FOR_EACH_MEMSHIRE

    Log_Write(LOG_LEVEL_INFO, "DDR:[-1][txt]load_training_fw: start\n");

    // get scratch buffer to use
    buffer = get_scratch_buffer(&buffer_size);

    // load Synopsys firmware code
    real_size = load_raw_image(region_instruction, buffer, buffer_size);
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[-1][txt]load_training_fw: load_raw_image returns %d bytes\n",
              real_size);

    if (real_size == 0)
    {
        Log_Write(LOG_LEVEL_ERROR, "load_training_fw: load_raw_image failed with region_id %d!\n",
                  region_instruction);
        return;
    }

    // program Synopsys firmware code
    target_address = program_all_ddrc(target_address, buffer, real_size);
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[-1][txt]load_training_fw: program_all_ddrc reaches 0x%016lx\n",
              target_address);

    FOR_EACH_MEMSHIRE(ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x00000001);
                      ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x00000000);)

    // load Synopsys firmware data for a specific frequency
    real_size = load_raw_image(region_data, buffer, buffer_size);
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[-1][txt]load_training_fw: load_raw_image returns %d bytes\n",
              real_size);

    if (real_size == 0)
    {
        Log_Write(LOG_LEVEL_ERROR, "load_training_fw: load_raw_image failed with region_id %d!\n",
                  region_data);
        return;
    }

    // program Synopsys firmware data for a specific frequency
    program_all_ddrc(target_address, buffer, real_size);
}

typedef enum
{
    TRAINING_1D,
    TRAINING_2D
} training_stage;

static const char *get_training_status_text(uint8_t major_msg)
{
    switch (major_msg)
    {
        case 0x07:
            return "training has run successfully ... firmware complete";
        case 0xff:
            return "training has failed ... firmware complete";
        case 0x00:
            return "end of initialization";
        case 0x01:
            return "end of fine write leveling";
        case 0x02:
            return "end of read enable training";
        case 0x03:
            return "end of read delay center optimization";
        case 0x04:
            return "end of write delay center optimization";
        case 0x05:
            return "end of 2D read delay/voltage center optimization";
        case 0x06:
            return "end of 2D write delay /voltage center optimization";
        case 0x09:
            return "end of max read latency training";
        case 0x0a:
            return "end of read dq deskew training";
        case 0x0b:
            return "reserved";
        case 0x0c:
            return "end of all DB training ... MREP/DWL/MRD/MWD complete";
        case 0x0d:
            return "end of CA training";
        case 0xfd:
            return "end of MPR read delay center optimization";
        case 0xfe:
            return "end of write leveling coarse delay";
        default:
            return "unknown";
    }
}

static uint32_t get_mail(uint32_t memshire)
{
    uint32_t mail;

    ms_poll_ddrc_reg(memshire, 2, APBONLY0_UctShadowRegs, 0x00000000, 0x1, 100,
                     1); // wait for handshake

    mail = ms_read_ddrc_reg(memshire, 2, APBONLY0_UctWriteOnlyShadow) |
           (ms_read_ddrc_reg(memshire, 2, APBONLY0_UctDatWriteOnlyShadow) << 16); // read message

    ms_write_ddrc_reg(memshire, 2, APBONLY0_DctWriteProt, 0x00000000); // ack message
    ms_poll_ddrc_reg(memshire, 2, APBONLY0_UctShadowRegs, 0x00000001, 0x1, 100,
                     1);                                               // wait for handshake
    ms_write_ddrc_reg(memshire, 2, APBONLY0_DctWriteProt, 0x00000001); // ack handshake

    return mail;
}

static void get_streaming_messages(uint32_t memshire)
{
    uint32_t mail;
    uint16_t count;

    mail = get_mail(memshire);
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][sms]0x%08x\n", memshire, mail);

    count = mail & 0xffff;
    while (count-- > 0)
    {
        mail = get_mail(memshire);
        Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][smd]0x%08x\n", memshire, mail);
    }
}

static void wait_for_training_internal(training_stage stage, uint32_t memshire,
                                       uint32_t train_poll_max_iterations,
                                       uint32_t train_poll_iteration_delay)
{
    uint8_t major_msg;
    uint8_t number_of_shire_completed;
    uint8_t shire_completed[HW_NUMBER_OF_MEMSHIRE] = { 0 };

    Log_Write(LOG_LEVEL_DEBUG, "DDR:[0][txt]Wait for training\n");
    ms_setup_dram_status();

    // memshire is not define outside, use it as a local variable
    memshire = MEMSHIRE_BASE;
    number_of_shire_completed = 0;
    while (number_of_shire_completed < NUMBER_OF_MEMSHIRE)
    {
        if (memshire == MEMSHIRE_BASE + NUMBER_OF_MEMSHIRE)
        {
            usdelay(train_poll_iteration_delay); // delay between each cycle
            memshire = MEMSHIRE_BASE;
        }
        (void)train_poll_iteration_delay;

        // Check if a specific shire is completed or a message is ready to be processed
        if (shire_completed[memshire] ||
            ms_peek_ddrc_reg(memshire, 2, APBONLY0_UctShadowRegs, 0x0, 0x1) != 0)
        {
            // none of above happen, try next memshire
            ++memshire;
            continue;
        }

        major_msg = (uint8_t)(ms_read_ddrc_reg(memshire, 2, APBONLY0_UctWriteOnlyShadow) &
                              0xff); // read message

        ms_write_ddrc_reg(memshire, 2, APBONLY0_DctWriteProt, 0x00000000); // ack message
        ms_poll_ddrc_reg(memshire, 2, APBONLY0_UctShadowRegs, 0x00000001, 0x1, 100,
                         1);                                               // wait for handshake
        ms_write_ddrc_reg(memshire, 2, APBONLY0_DctWriteProt, 0x00000001); // ack handshake

        Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][mms]0x%08x\n", memshire, major_msg);

        if (major_msg == 0x08)
        {
            // process streaming messages on 0x08
            get_streaming_messages(memshire);
        }
        else if (major_msg == 0x7)
        {
            // mark the memshire as completed on success(0x7).
            Log_Write(LOG_LEVEL_INFO, "DDR:[%d][txt]Training success\n", memshire);
            Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]?? ? ? ??? ??? ??? ?? ?? ? ?\n", memshire);
            Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]?? ??? ??? ??? ??? ?? ?? ? ?\n", memshire);

            shire_completed[memshire] = 1;
            ++number_of_shire_completed;
            ms_set_dram_status_physical_memshire(memshire, WORKING);
        }
        else if (major_msg == 0xff || major_msg == 0x0b)
        {
            // mark the memshire as completed on fail(0xff/0x0b).
            Log_Write(LOG_LEVEL_INFO, "DDR:[%d][txt]Training failure!\n", memshire);
            Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]??? ??? ? ?   ??? ??? ?\n", memshire);
            Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]??  ??? ? ??? ??? ??? ?\n", memshire);
            shire_completed[memshire] = 1;
            ++number_of_shire_completed;
            ms_set_dram_status_physical_memshire(memshire, FAILED);
        }
        else
        {
            Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]%s\n", memshire,
                      get_training_status_text(major_msg));
        }

        ++memshire;
    }

    ms_update_dram_status_system_memshire();

    // System software doesn't use these 2 variables.  But we want to keep the same interface by hardware team
    (void)stage;
    (void)train_poll_max_iterations; // suppress compilation warning
    return;
}

void ms_printout_status(uint32_t memshire, bool ddrc_enabled)
{
    (void)memshire;

    // print out ms_mem_status
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]ms_mem_status = 0x%02x\n", 0,
              (uint8_t)ms_read_esr(0, ms_mem_status));
    // usdelay(PRINT_DELAY);
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]ms_mem_status = 0x%02x\n", 4,
              (uint8_t)ms_read_esr(4, ms_mem_status));
    // usdelay(PRINT_DELAY);

    if (!ddrc_enabled)
        return;

    // print out PHY PLL lock status
    uint32_t ddrc_pll_lock;
    uint32_t value;
    uint32_t muxsel;
    size_t counter = 0;

    muxsel = ms_read_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel);
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]APBONLY0_MicroContMuxSel = 0x%08x\n", memshire,
              muxsel);
    // usdelay(PRINT_DELAY);

    // get control
    if (muxsel)
    {
        ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x0);
        Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]APBONLY0_MicroContMuxSel = 0x%08x\n", memshire,
                  ms_read_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel));
        // usdelay(PRINT_DELAY);
    }

    counter = 0;
    ddrc_pll_lock = 0;
    for (int i = 0; i < 20; ++i)
    {
        value = ms_read_ddrc_reg(memshire, 2, MASTER0_PllLockStatus);
        if (value)
        {
            ddrc_pll_lock = value;
            ++counter;
        }
    }
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]PHY PLL Lock Status = 0x%08x (%ld/20)\n", memshire,
              ddrc_pll_lock, counter);
    // usdelay(PRINT_DELAY);
    if (!ddrc_pll_lock)
    {
        Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]PHY PLL Lock LOST!!!\n", memshire);
        // usdelay(PRINT_DELAY);
    }

    // return control
    if (muxsel)
    {
        ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x1);
        Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]APBONLY0_MicroContMuxSel = 0x%08x\n", memshire,
                  ms_read_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel));
        // usdelay(PRINT_DELAY);
    }
}

/*
** Support functions for ddr
*/
#if (DDR_DIAG & DDR_DIAG_MEMSHIRE_ID)
void check_memshire_revision_id(uint32_t memshire)
{
    uint64_t rev_id;

    uint32_t memshire_id;

    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]MemshireID check\n", memshire);
    rev_id = ms_read_esr(memshire, ms_mem_revision_id);
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]MemshireID = 0x%02x\n", memshire,
              (uint8_t)ETSOC_MEM_SHIRE_ESR_MS_MEM_REVISION_ID_MEMSHIRE_ID_GET(rev_id));
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]RevisionB3 = 0x%02x\n", memshire,
              (uint8_t)ETSOC_MEM_SHIRE_ESR_MS_MEM_REVISION_ID_MEMSHIRE_REVISION_B3_GET(rev_id));
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]RevisionB2 = 0x%02x\n", memshire,
              (uint8_t)ETSOC_MEM_SHIRE_ESR_MS_MEM_REVISION_ID_MEMSHIRE_REVISION_B2_GET(rev_id));
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]RevisionB1 = 0x%02x\n", memshire,
              (uint8_t)ETSOC_MEM_SHIRE_ESR_MS_MEM_REVISION_ID_MEMSHIRE_REVISION_B1_GET(rev_id));
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]RevisionB0 = 0x%02x\n", memshire,
              (uint8_t)ETSOC_MEM_SHIRE_ESR_MS_MEM_REVISION_ID_MEMSHIRE_REVISION_B0_GET(rev_id));
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]Raw = 0x%016lx\n", memshire, rev_id);
    memshire_id = ETSOC_MEM_SHIRE_ESR_MS_MEM_REVISION_ID_MEMSHIRE_ID_GET(rev_id);
    if (memshire_id != memshire)
        Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]MemshireID check failed!  Reading 0x%02x\n",
                  memshire, memshire_id);
}
#endif // (DDR_DIAG & DDR_DIAG_MEMSHIRE_ID)

/*
** Low-level support functions
*/
uint64_t ms_read_esr(uint32_t memshire, uint64_t reg)
{
    uint64_t v = *(volatile uint64_t *)GET_MEMSHIRE_ESR(memshire, reg);
    return v;
}

void ms_write_esr(uint32_t memshire, uint64_t reg, uint64_t value)
{
    *(volatile uint64_t *)GET_MEMSHIRE_ESR(memshire, reg) = value;

#if (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
    uint64_t read_back = ms_read_esr(memshire, reg);
    if (read_back != value)
    {
        Log_Write(LOG_LEVEL_CRITICAL,
                  "DDR:[%d][txt]ms_write_esr() write verification fails at register 0x%016lx\n",
                  memshire, reg);
        Log_Write(
            LOG_LEVEL_CRITICAL,
            "DDR:[%d][txt]ms_write_esr() write verification (written 0x%016lx, read back 0x%016lx)\n",
            memshire, value, read_back);
    }
#endif // (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
}

uint32_t ms_read_reg(uint32_t memshire, uint64_t reg)
{
    uint32_t v = *(volatile uint32_t *)GET_MEMSHIRE_ESR(memshire, reg);
    return v;
}

void ms_write_reg(uint32_t memshire, uint64_t reg, uint32_t value)
{
    *(volatile uint32_t *)GET_MEMSHIRE_ESR(memshire, reg) = value;

#if (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
    uint32_t read_back = ms_read_reg(memshire, reg);
    if (read_back != value)
    {
        Log_Write(LOG_LEVEL_CRITICAL,
                  "DDR:[%d][txt]ms_write_reg() write verification fails at register 0x%016lx\n",
                  memshire, reg);
        Log_Write(
            LOG_LEVEL_CRITICAL,
            "DDR:[%d][txt]ms_write_reg() write verification (written 0x%08x, read back 0x%08x)\n",
            memshire, value, read_back);
    }

#endif // (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
}

uint32_t ms_read_ddrc_reg(uint32_t memshire, uint32_t blk, uint64_t reg)
{
    uint32_t v = *(volatile uint32_t *)get_ddrc_address(memshire, blk, reg);
    return v;
}

void ms_write_ddrc_reg(uint32_t memshire, uint32_t blk, uint64_t reg, uint32_t value)
{
    *(volatile uint32_t *)get_ddrc_address(memshire, blk, reg) = value;

#if (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
    uint32_t read_back = ms_read_ddrc_reg(memshire, blk, reg);
    if (read_back != value)
    {
        Log_Write(
            LOG_LEVEL_CRITICAL,
            "DDR:[%d][txt]ms_write_ddrc_reg() write verification fails at register 0x%016lx\n",
            memshire, reg);
        Log_Write(
            LOG_LEVEL_CRITICAL,
            "DDR:[%d][txt]ms_write_ddrc_reg() write verification (written 0x%08x, read back 0x%08x)\n",
            memshire, value, read_back);
    }
#endif // (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
}

void ms_write_both_ddrc_reg(uint32_t memshire, uint64_t reg, uint32_t value)
{
    *(volatile uint32_t *)get_ddrc_address(memshire, 0, reg) = value;
    *(volatile uint32_t *)get_ddrc_address(memshire, 1, reg) = value;

#if (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
    uint32_t read_back0 = ms_read_ddrc_reg(memshire, 0, reg);
    uint32_t read_back1 = ms_read_ddrc_reg(memshire, 1, reg);
    if (read_back0 != value || read_back1 != value)
    {
        Log_Write(
            LOG_LEVEL_CRITICAL,
            "DDR:[%d][txt]ms_write_both_ddrc_reg() write verification fails at register 0x%016lx (written 0x%08x)\n",
            memshire, reg, value);
        Log_Write(
            LOG_LEVEL_CRITICAL,
            "DDR:[%d][txt]ms_write_both_ddrc_reg() write verification physical address %p, %p\n",
            memshire, get_ddrc_address(memshire, 0, reg), get_ddrc_address(memshire, 1, reg));
        Log_Write(
            LOG_LEVEL_CRITICAL,
            "DDR:[%d][txt]ms_write_both_ddrc_reg() write verification read back[0]=0x%08x, read back[1]=0x%08x)\n",
            memshire, read_back0, read_back1);
    }
#endif // (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
}

void ms_write_ddrc_addr(uint32_t memshire, uint64_t addr_in, uint32_t value)
{
    *(volatile uint32_t *)get_ddrc_address(memshire, 0, addr_in) = value;

#if (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
    uint32_t read_back = ms_read_ddrc_reg(memshire, 0, addr_in);
    if (read_back != value)
    {
        Log_Write(
            LOG_LEVEL_CRITICAL,
            "DDR:[%d][txt]ms_write_ddrc_addr() write verification fails at register 0x%016lx\n",
            memshire, addr_in);
        Log_Write(
            LOG_LEVEL_CRITICAL,
            "DDR:[%d][txt]ms_write_ddrc_addr() write verification (written 0x%08x, read back 0x%08x)\n",
            memshire, value, read_back);
    }
#endif // (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
}

uint32_t ms_poll_pll_reg(uint32_t memshire, uint64_t reg, uint32_t wait_value, uint32_t wait_mask,
                         uint32_t timeout_tries)
{
    while (1)
    {
        uint32_t value = ms_read_reg(memshire, reg);
        if (((value ^ wait_value) & wait_mask) == 0)
            break;

        timeout_tries--;
        if (timeout_tries == 0)
        {
            break;
        }
        usdelay(1);
    }

    return timeout_tries;
}

uint32_t ms_poll_ddrc_reg(uint32_t memshire, uint32_t blk, uint64_t reg, uint32_t wait_value,
                          uint32_t wait_mask, uint32_t timeout_tries, uint32_t wait_count)
{
    while (1)
    {
        uint32_t value = ms_read_ddrc_reg(memshire, blk, reg);
        if (((value ^ wait_value) & wait_mask) == 0)
            break;

        timeout_tries--;
        if (timeout_tries == 0)
        {
            break;
        }
        (void)wait_count;
    }

    return timeout_tries;
}

/*
** Training related functions
** - Reading training info from external storage, either file or flash.
** - Writing into DDR memory controllers
*/
void ms_ddr_phy_1d_train_from_file(uint32_t mem_config, uint32_t memshire)
{
    ESPERANTO_FLASH_REGION_ID_t region_id;

    // decide which frequency to run
    switch (mem_config)
    {
        case DDR_800MHZ:
            region_id = ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_PAYLOAD_800MHZ;
            break;
        case DDR_933MHZ:
            region_id = ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_PAYLOAD_933MHZ;
            break;
        case DDR_1067MHZ:
            region_id = ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_PAYLOAD_1067MHZ;
            break;
        default:
            Log_Write(LOG_LEVEL_ERROR, "DDR:[0][txt]Unrecongnized mem_config = %d\n", mem_config);
            return;
    }

    load_training_fw(ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING, region_id);

    // suppress compilation warning
    (void)memshire;
}

void ms_wait_for_training(uint32_t memshire, uint32_t train_poll_max_iterations,
                          uint32_t train_poll_iteration_delay)
{
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[-1][txt]ms_wait_for_training\n");
    wait_for_training_internal(TRAINING_1D, memshire, train_poll_max_iterations,
                               train_poll_iteration_delay);
}

void ms_ddr_phy_2d_train_from_file(uint32_t mem_config, uint32_t memshire)
{
    ESPERANTO_FLASH_REGION_ID_t region_id;

    // decide which frequency to run
    switch (mem_config)
    {
        case DDR_800MHZ:
            region_id = ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D_PAYLOAD_800MHZ;
            break;
        case DDR_933MHZ:
            region_id = ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D_PAYLOAD_933MHZ;
            break;
        case DDR_1067MHZ:
            region_id = ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D_PAYLOAD_1067MHZ;
            break;
        default:
            Log_Write(LOG_LEVEL_ERROR, "DDR:[0][txt]Unrecongnized mem_config = %d\n", mem_config);
            return;
    }

    load_training_fw(ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D, region_id);

    // suppress compilation warning
    (void)memshire;
}

void ms_wait_for_training_2d(uint32_t memshire, uint32_t train_poll_max_iterations,
                             uint32_t train_poll_iteration_delay)
{
    wait_for_training_internal(TRAINING_2D, memshire, train_poll_max_iterations,
                               train_poll_iteration_delay);
}

//
// Read location in PHY RAM.
// It is assumed that the address has the leading "5"
//
// reference $REPOROOT/dv/tests/memshire/tcl_tests/tcl_scripts/lib/ms_initlib.tcl
static uint32_t ms_read_phy_ram(uint32_t memshire, const uint64_t addr)
{
    const uint64_t LPDDR_PHY_BASE = R_SHIRE_LPDDR_BASEADDR + 0x0002000000;
    volatile const uint32_t *fulladdr =
        (uint32_t *)(LPDDR_PHY_BASE | (((memshire & 0x7) << 26) | (addr << 2)));
    uint32_t value = *fulladdr;
    return value;
}

void ms_write_phy_ram(uint32_t memshire, const uint64_t addr, const uint32_t value)
{
    const uint64_t LPDDR_PHY_BASE = R_SHIRE_LPDDR_BASEADDR + 0x0002000000;
    volatile uint32_t *fulladdr =
        (uint32_t *)(LPDDR_PHY_BASE | (((memshire & 0x7) << 26) | (addr << 2)));
    *fulladdr = value;
#if (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
    uint32_t read_back = ms_read_phy_ram(memshire, addr);
    if (read_back != value)
    {
        Log_Write(LOG_LEVEL_DEBUG,
                  "DDR:[%d][txt]ms_write_phy_ram() write verification fails at register 0x%016lx\n",
                  memshire, addr);
        Log_Write(
            LOG_LEVEL_DEBUG,
            "DDR:[%d][txt]ms_write_phy_ram() write verification (written 0x%016lx, read back 0x%016lx)\n",
            memshire, value, read_back);
    }
#endif // (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
}

// reference $REPOROOT/dv/tests/memshire/tcl_tests/tcl_scripts/lib/ms_initlib.tcl
static uint32_t read_and_set_max_txdqdly(uint32_t curr_max, uint32_t memshire, uint32_t controller,
                                         uint64_t regname)
{
    uint32_t value;

    value = ms_read_ddrc_reg(memshire, controller, regname);
    return (value > curr_max ? value : curr_max);
}

// chA/chB: channel0/1
// reference $REPOROOT/dv/tests/memshire/tcl_tests/tcl_scripts/lib/ms_initlib.tcl
static void update_DFITMG2_rd2wr(uint32_t memshire, uint8_t channel)
{
    // get these addresses from mnPmuSramMsgBlock_lpddr4.h for both 1D/2D chA & chB
    const uint32_t CDD_ChX_RW_0_0[2] = { 0x54015, 0x5402f };

    uint32_t cdd_chX_rw_0_0_raw = ms_read_phy_ram(memshire, CDD_ChX_RW_0_0[channel]);

    // important to convert both into 8 bits signed integer to represent a correct negtive value
    // variables need to be int to do arithmetic later
    int cdd_chX_rw_0_0_lo = (int8_t)(cdd_chX_rw_0_0_raw & 0xff);
    int cdd_chX_rw_0_0_hi = (int8_t)((cdd_chX_rw_0_0_raw >> 8) & 0xff);

    // taking abs() on both values
    if (cdd_chX_rw_0_0_lo < 0)
        cdd_chX_rw_0_0_lo *= -1;
    if (cdd_chX_rw_0_0_hi < 0)
        cdd_chX_rw_0_0_hi *= -1;

    uint32_t cdd_chX_rw_0_0 =
        (uint32_t)((cdd_chX_rw_0_0_lo > cdd_chX_rw_0_0_hi) ? cdd_chX_rw_0_0_lo : cdd_chX_rw_0_0_hi);

    Log_Write(LOG_LEVEL_DEBUG,
              "DDR:[%d][txt]Channel %d : cdd_chX_rw_0_0_raw is 0x%08x cdd_chX_rw_0_0 is 0x%02x\n",
              memshire, channel, cdd_chX_rw_0_0_raw, cdd_chX_rw_0_0);

    // rd2wr is bits 13:8 of the DRAMTMG2 register
    uint32_t curr_dramtmg2 = ms_read_ddrc_reg(memshire, 0, DRAMTMG2);
    uint32_t curr_rd2wr = (curr_dramtmg2 >> 8) & 0x3f;
    uint32_t new_rd2wr = curr_rd2wr + (((cdd_chX_rw_0_0 + 1) & 0xff) >> 1);
    uint32_t new_dramtmg2 = (curr_dramtmg2 & 0xffffc0ff) | ((new_rd2wr & 0x3f) << 8);

    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]Channel %d : current DRAMTMG2 is 0x%08x\n", memshire,
              channel, curr_dramtmg2);
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]Channel %d : current rd2wr is    0x%08x\n", memshire,
              channel, curr_rd2wr);
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]Channel %d : new     rd2wr is    0x%08x\n", memshire,
              channel, new_rd2wr);
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]Channel %d : new     DRAMTMG2 is 0x%08x\n", memshire,
              channel, new_dramtmg2);

    ms_write_ddrc_reg(memshire, channel, DRAMTMG2, new_dramtmg2);
}

//
// Update DDR controller registers based on traning data from the PHY
//
// reference $REPOROOT/dv/tests/memshire/tcl_tests/tcl_scripts/lib/ms_initlib.tcl
void post_train_update_regs(uint32_t memshire)
{
    // make PHY CSRs accessable from the APB and turn on the clock
    ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x00000000);
    ms_write_ddrc_reg(memshire, 2, DRTUB0_UcclkHclkEnables, 0x00000003);
    ms_read_ddrc_reg(memshire, 2, DRTUB0_UcclkHclkEnables); // make sure writes are done

    // ######################################################################################
    // # update DFITMG1.dfi_t_wrdata_delay based on Trained_TxDqsDly
    // ######################################################################################
    uint32_t max_txdqdly = 0;
    uint32_t max_txdqdly_ui;

    max_txdqdly = read_and_set_max_txdqdly(max_txdqdly, memshire, 2, DBYTE0_TxDqsDlyTg0_u0_p0);
    max_txdqdly = read_and_set_max_txdqdly(max_txdqdly, memshire, 2, DBYTE0_TxDqsDlyTg0_u1_p0);
    max_txdqdly = read_and_set_max_txdqdly(max_txdqdly, memshire, 2, DBYTE1_TxDqsDlyTg0_u0_p0);
    max_txdqdly = read_and_set_max_txdqdly(max_txdqdly, memshire, 2, DBYTE1_TxDqsDlyTg0_u1_p0);
    max_txdqdly = read_and_set_max_txdqdly(max_txdqdly, memshire, 2, DBYTE2_TxDqsDlyTg0_u0_p0);
    max_txdqdly = read_and_set_max_txdqdly(max_txdqdly, memshire, 2, DBYTE2_TxDqsDlyTg0_u1_p0);
    max_txdqdly = read_and_set_max_txdqdly(max_txdqdly, memshire, 2, DBYTE3_TxDqsDlyTg0_u0_p0);
    max_txdqdly = read_and_set_max_txdqdly(max_txdqdly, memshire, 2, DBYTE3_TxDqsDlyTg0_u1_p0);

    uint32_t course = (max_txdqdly >> 6) & 0xf; // bits [9:6] is coarse (1 UI)
    uint32_t fine = max_txdqdly & 0x1f;         // bits [4:0] is fine   (1/32 of a UI)

    if (fine > 0)
        max_txdqdly_ui = course + 1;
    else
        max_txdqdly_ui = course;

    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]max_txdqdly is 0x%08x  max_txdqdly_ui is 0x%08x\n",
              memshire, max_txdqdly, max_txdqdly_ui);

    // dfi_t_wrdata_delay is bits 20:16 of the DFITMG1 register
    // read current value of DFITMG1 (from ctrl 0)
    uint32_t curr_dfitmg1 = ms_read_ddrc_reg(memshire, 0, DFITMG1);

    uint32_t curr_txdqdly_dficlk = (curr_dfitmg1 >> 16) & 0x1f;
    uint32_t new_txdqdly_dficlk =
        curr_txdqdly_dficlk + ((max_txdqdly_ui + 1) >> 1); // +1 to implement ceil

    // merge in new value of tphy_wrdata_delay
    uint32_t new_dfitmg1 = (curr_dfitmg1 & 0xffe0ffff) | ((new_txdqdly_dficlk & 0x1f) << 16);

    Log_Write(LOG_LEVEL_DEBUG,
              "DDR:[%d][txt]curr_txdqdly_dficlk is 0x%08x new_txdqdly_dficlk is 0x%08x\n", memshire,
              curr_txdqdly_dficlk, new_txdqdly_dficlk);

    // write new value back to both controllers
    ms_write_both_ddrc_reg(memshire, DFITMG1, new_dfitmg1);

    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]current DFITMG1 is 0x%08x new DFITMG1 is 0x%08x\n",
              memshire, curr_dfitmg1, new_dfitmg1);

    // ######################################################################################
    // # update DFITMG2.rd2wr based on CDD_ChA/B_RW_0_0
    // ######################################################################################
    update_DFITMG2_rd2wr(memshire, 0);
    update_DFITMG2_rd2wr(memshire, 1);

    // turn off the clocks now
    ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x00000001);
    ms_write_ddrc_reg(memshire, 2, DRTUB0_UcclkHclkEnables, 0x00000002);
    // Enable Read FIFO ptr mismatch interrupt within the phy
    ms_write_ddrc_reg(memshire, 3, MASTER0_PhyInterruptEnable, 0x00000400);
}

void mem_disable_unused_clocks(void)
{
    uint32_t memshire;

    FOR_EACH_MEMSHIRE(ms_write_esr(memshire, ms_clk_gate_ctl, 0x1); // turn off memshire debug clock
                      ms_write_ddrc_reg(memshire, 0, SS_clk_rst_ctrl,
                                        0xffffe3f5); // turn off pclks and scrub clocks
    )
}

//-----------------------------------------------------
// Check DRAM Density (MR8)
//  - initiate MRR to U0/MR8
//-----------------------------------------------------
//
// MR8 has the DRAM density and type
// for the micron part it is 0x10 for 32gb and 0x18 for 64gb
//
uint32_t ms_verify_ddr_density(uint32_t memshire)
{
    uint32_t value;
    uint32_t status = 0;

    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][ms_verify_ddr_density] Reading MR %d\n", memshire,
              MR_DENSITY);

    // Poll MSTAT to ensure the bus isn't busy.
    uint32_t busy = 1;
    while (busy == 1)
    {
        busy = (uint32_t)ms_read_ddrc_reg(memshire, 0, MRSTAT) & 0x1;
        usdelay(10);
    }
    // Set MR Address in MRCTRL1[15:8]
    value = ms_read_ddrc_reg(memshire, 0, MRCTRL1);
    value |= SET_MR_ADDRESS(MR_DENSITY);
    ms_write_ddrc_reg(memshire, 0, MRCTRL1, value);

    // set type = read in MRCTRL0
    ms_write_ddrc_reg(memshire, 0, MRCTRL0, MR_SET_READ);

    // Clear status before initiating read.
    ms_write_esr(memshire, ddrc_mrr_status, 0x0);

    // setting bit 31 causes mrr to occur
    ms_write_ddrc_reg(memshire, 0, MRCTRL0, MR_READ);
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][ms_verify_ddr_density] Waiting for status.\n", memshire);
    //wait for mrr u0 status to be set
    //ms_poll_ddrc_reg(memshire,0,ddrc_mrr_status,0x1,0x1,100,1); // RHS: This don't work

    while (status == 0)
    {
        status = (uint32_t)ms_read_esr(memshire, ddrc_mrr_status);
        usdelay(10);
    }

    value = (uint32_t)ms_read_esr(memshire, ddrc_mrr_status);
    //evl_log [format "TCL: Read %x from mrr_status,"  $data];
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][ms_verify_ddr_density] ddrc_mrr_status=%x\n", memshire,
              value);
    // FUTURE : should also check bit 2 = 0
    //check_esr ddrc_mrr_status 0x1 0x1
    value = (uint32_t)ms_read_esr(memshire, ddrc_mrr_status);
    if (value != 0x1)
    {
        Log_Write(LOG_LEVEL_DEBUG,
                  "DDR:[%d][ms_verify_ddr_density] ERROR: ddrc_mrr_status=%x expecting 0x1\n",
                  memshire, value);
    }
    //check_esr ddrc_u0_mrr_data 0x10|0x18 (32gb|64gb)
    value = (uint32_t)ms_read_esr(memshire, ddrc_u0_mrr_data);
    uint32_t density = 0;
    density = (uint32_t)((value >> 2) & 0xf);
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][ms_verify_ddr_density] ddrc_mrr_data=%x, density=%x \n",
              memshire, value, density);
    if (density == 0x6)
    {
        Log_Write(LOG_LEVEL_DEBUG,
                  "DDR:[%d][ms_verify_ddr_density] Density=64gb/32GB, density=%x \n", memshire,
                  density);
    }
    else if (density == 0x4)
    {
        Log_Write(LOG_LEVEL_DEBUG,
                  "DDR:[%d][ms_verify_ddr_density] Density=32gb/16GB, density=%x \n", memshire,
                  density);
    }
    else
    {
        Log_Write(LOG_LEVEL_DEBUG,
                  "DDR:[%d][ms_verify_ddr_density] ERROR: Density=??, density=%x \n", memshire,
                  density);
    }

    //write_esr ddrc_mrr_status 0x0
    ms_write_esr(memshire, ddrc_mrr_status, 0x0);

    //check_esr ddrc_mrr_status 0x0
    value = (uint32_t)ms_read_esr(memshire, ddrc_mrr_status);
    if (value != 0x0)
    {
        Log_Write(LOG_LEVEL_DEBUG,
                  "DDR:[%d][ms_verify_ddr_density] ERROR: ddrc_mrr_status=%x expecting 0x0\n",
                  memshire, value);
    }
    return density;
}

//-----------------------------------------------------
// Check DRAM Vendor ID (MR5)
//  - initiate MRR to U0/MR5
//-----------------------------------------------------
//
// MR5 has the DRAM Vendor ID
// Micron = 0xFF
// SK-Hynix = 0x06
//
uint32_t ms_verify_ddr_vendor(uint32_t memshire)
{
    uint32_t value;
    uint32_t status = 0;
    uint32_t busy = 1;
    uint32_t vendor = 0;

    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][ms_verify_ddr_vendor] Reading MR %d\n", memshire,
              MR_VENDOR_ID);

    // Poll MSTAT to ensure the bus isn't busy.

    while (busy == 1)
    {
        busy = (uint32_t)ms_read_ddrc_reg(memshire, 0, MRSTAT) & 0x1;
        usdelay(10);
    }
    // Set MR Address in MRCTRL1[15:8]
    value = ms_read_ddrc_reg(memshire, 0, MRCTRL1);
    value |= SET_MR_ADDRESS(MR_VENDOR_ID);
    ms_write_ddrc_reg(memshire, 0, MRCTRL1, value);

    // set type = read in MRCTRL0
    ms_write_ddrc_reg(memshire, 0, MRCTRL0, MR_SET_READ);

    // Clear status before initiating read.
    ms_write_esr(memshire, ddrc_mrr_status, 0x0);

    // setting bit 31 causes mrr to occur
    ms_write_ddrc_reg(memshire, 0, MRCTRL0, MR_READ);
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][ms_verify_ddr_vendor] Waiting for status.\n", memshire);
    //wait for mrr u0 status to be set
    //ms_poll_ddrc_reg(memshire,0,ddrc_mrr_status,0x1,0x1,100,1); // RHS: This don't work

    while (status == 0)
    {
        status = (uint32_t)ms_read_esr(memshire, ddrc_mrr_status);
        usdelay(10);
    }
    //poll_esr ddrc_mrr_status 0x1 0x1 100
    value = (uint32_t)ms_read_esr(memshire, ddrc_mrr_status);
    //evl_log [format "TCL: Read %x from mrr_status,"  $value];
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][ms_verify_ddr_vendor] ddrc_mrr_status=%x\n", memshire,
              value);
    // FUTURE : should also check bit 2 = 0
    //check_esr ddrc_mrr_status 0x1 0x1
    value = (uint32_t)ms_read_esr(memshire, ddrc_mrr_status);
    if (value != 0x1)
    {
        Log_Write(LOG_LEVEL_DEBUG,
                  "DDR:[%d][ms_verify_ddr_vendor] ERROR: ddrc_mrr_status=%x expecting 0x1\n",
                  memshire, value);
    }
    //check_esr ddrc_u0_mrr_data 0x10|0x18 (32gb|64gb)
    value = (uint32_t)ms_read_esr(memshire, ddrc_u0_mrr_data);
    vendor = (uint32_t)(value & 0xff);

    //write_esr ddrc_mrr_status 0x0
    ms_write_esr(memshire, ddrc_mrr_status, 0x0);

    //check_esr ddrc_mrr_status 0x0
    value = (uint32_t)ms_read_esr(memshire, ddrc_mrr_status);
    if (value != 0x0)
    {
        Log_Write(LOG_LEVEL_DEBUG,
                  "DDR:[%d][ms_verify_ddr_vendor] ERROR: ddrc_mrr_status=%x expecting 0x0\n",
                  memshire, value);
    }
    return vendor;
}