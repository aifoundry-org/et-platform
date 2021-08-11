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
#include "usdelay.h"
#include "log.h"

#include "bl2_scratch_buffer.h"
#include "bl2_flashfs_driver.h"

#include "hwinc/hal_device.h"
#include "hwinc/ddrc_reg_def.h"
#include "hwinc/etsoc_mem_shire_esr.h"
#include "hal_ddr_init.h"

#include "mem_controller.h"

/*
** Private functions/macros used only in this file
*/
#define GET_MEMSHIRE_ESR(memshire, reg) \
        ((((uint64_t)memshire&0x7)|0xe8) << 22 | reg)      // 0xe8=232

static inline uint32_t ms_peek_ddrc_reg(uint32_t memshire, uint32_t blk, uint64_t reg, uint32_t wait_value, uint32_t wait_mask)
{
    uint32_t value = ms_read_ddrc_reg(memshire, blk, reg);
    return (value ^ wait_value) & wait_mask;
}

static inline volatile void *get_ddrc_address(uint32_t memshire, uint32_t block, uint64_t reg)
{
  if(block == 1)
    reg |= 0x1000;
  return (volatile void *) (R_SHIRE_LPDDR_BASEADDR | ((uint64_t)memshire & 0x7) << 26 | reg);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
static uint32_t load_raw_image(ESPERANTO_FLASH_REGION_ID_t region_id, void *buffer, uint32_t buffer_size)
{
  ESPERANTO_RAW_IMAGE_FILE_HEADER_t image_file_header;
  uint32_t image_file_size;
  const char *image_name;

  switch(region_id) {
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
      Log_Write(LOG_LEVEL_ERROR, "load_raw_image: unrecongnized region id %d\n", region_id);
      return 0;
  }
  Log_Write(LOG_LEVEL_INFO, "load_raw_image: Loading %s firmware\n", image_name);

  if (0 != flashfs_drv_get_file_size(region_id, &image_file_size))
  {
    Log_Write(LOG_LEVEL_ERROR, "load_raw_image: flashfs_drv_get_file_size(%s) failed!\n", image_name);
    return 0;
  }
  if (image_file_size < sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t))
  {
    Log_Write(LOG_LEVEL_ERROR, "load_raw_image: %s image file too small!\n", image_name);
    return 0;
  }
  if (0 != flashfs_drv_read_file(region_id, 0, &image_file_header, sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t)))
  {
    Log_Write(LOG_LEVEL_ERROR, "load_raw_image: flashfs_drv_read_file(%s header) failed!\n", image_name);
    return 0;
  }
  Log_Write(LOG_LEVEL_INFO, "Loaded %s header...\n", image_name);

  image_file_size = image_file_header.info.image_info_and_signaure.info.raw_image_size;
  if(image_file_size > buffer_size) {
    Log_Write(LOG_LEVEL_ERROR, "load_raw_image: buffer too small to fit %s image file at %d bytes!\n", image_name, image_file_size);
    return 0;
  }

  if (0 != flashfs_drv_read_file(region_id, sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t), buffer, image_file_size))
  {
    Log_Write(LOG_LEVEL_ERROR, "load_raw_image: flashfs_drv_read_file(code) failed!\n");
    return 0;
  }

  Log_Write(LOG_LEVEL_INFO, "load_raw_image: Loaded %s firmware with %d bytes\n", image_name, image_file_size);
  return image_file_size;
}
#pragma GCC diagnostic pop

static uint64_t program_all_ddrc(uint64_t target_address, const void *buffer, uint32_t size)
{
  uint32_t memshire;
  const uint32_t *ptr = buffer;

  Log_Write(LOG_LEVEL_INFO, "program_all_ddrc: %d bytes to 0x%016lx\n", size, target_address);

  while( ((const uint8_t*)ptr - (const uint8_t*)buffer) < size) {
    FOR_EACH_MEMSHIRE(
      ms_write_ddrc_addr(memshire, target_address, *ptr);
    )
    target_address += 4;
    ++ptr;
  }

#if (DDR_DIAG & DDR_DIAG_VERIFY_SRAM_WRITE)
  uint32_t read_back;
  ptr = buffer;
  target_address -= (size + 3)/4*4;  // round up to multiple of 4
  while( ((const uint8_t*)ptr - (const uint8_t*)buffer) < size) {
    FOR_EACH_MEMSHIRE(
      read_back = ms_read_ddrc_reg(memshire, 0, target_address);
      if(read_back != *ptr) {
        Log_Write(LOG_LEVEL_ERROR, "DDR:[%d][txt]program_all_ddrc() write verification fails at address 0x%016lx (written 0x%08x, read back 0x%08x)\n",
          memshire, target_address, *ptr, read_back);
      }
    )
    target_address += 4;
    ++ptr;
  }
#endif // (DDR_DIAG & DDR_DIAG_VERIFY_SRAM_WRITE)

  Log_Write(LOG_LEVEL_INFO, "program_all_ddrc: done till address 0x%016lx\n", target_address);
  return target_address;
}

static void load_training_fw(ESPERANTO_FLASH_REGION_ID_t region_instruction, ESPERANTO_FLASH_REGION_ID_t region_data)
{
  void *buffer;
  uint32_t buffer_size;
  uint32_t real_size;
  uint64_t target_address = DDRC_PMU_SRAM;
  uint32_t memshire;    // for FOR_EACH_MEMSHIRE

  // get scratch buffer to use
  buffer = get_scratch_buffer(&buffer_size);

  // load Synopsys firmware code
  real_size = load_raw_image(region_instruction, buffer, buffer_size);
  if(real_size == 0) {
    Log_Write(LOG_LEVEL_ERROR, "load_training_fw: load_raw_image failed with region_id %d!\n", region_instruction);
    return;
  }

  // program Synopsys firmware code
  target_address = program_all_ddrc(target_address, buffer, real_size);

  FOR_EACH_MEMSHIRE(
    ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x00000001);
    ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x00000000);
  )

  // load Synopsys firmware data for a specific frequency
  real_size = load_raw_image(region_data, buffer, buffer_size);
  if(real_size == 0) {
    Log_Write(LOG_LEVEL_ERROR, "load_training_fw: load_raw_image failed with region_id %d!\n", region_data);
    return;
  }

  // program Synopsys firmware data for a specific frequency
  program_all_ddrc(target_address, buffer, real_size);
}

typedef enum
{
  TRAINING_1D,
  TRAINING_2D
}training_stage;

#if (DDR_DIAG & DDR_DIAG_DEBUG_INFO)
static const char* get_training_status_text(uint8_t major_msg)
{
  switch(major_msg) {
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

static void log_training_error(training_stage stage, uint32_t memshire, uint8_t major_msg)
{
  if(stage == TRAINING_1D)
    Log_Write(LOG_LEVEL_DEBUG, "DDR TRAIN (S%d) received DDRC PHY message 0x%02x (%s)", memshire+232, major_msg, get_training_status_text(major_msg));
  else
    Log_Write(LOG_LEVEL_DEBUG, "DDR TRAIN 2d (S%d) received DDRC PHY message 0x%02x (%s)", memshire+232, major_msg, get_training_status_text(major_msg));
}

#define LOG_TRAINING(stage, memshire, major_msg)    log_training_error(stage, memshire, major_msg)

#else   // (DDR_DIAG & DDR_DIAG_DEBUG_INFO)

#define LOG_TRAINING(stage, memshire, major_msg)    ((void)stage)     // to suppress compilation warning

#endif  // (DDR_DIAG & DDR_DIAG_DEBUG_INFO)

static uint32_t get_mail(uint32_t memshire)
{
  uint32_t mail;

  ms_poll_ddrc_reg(memshire, 2, APBONLY0_UctShadowRegs, 0x00000000, 0x1, 100, 1);               // wait for handshake

  mail = ms_read_ddrc_reg(memshire, 2, APBONLY0_UctWriteOnlyShadow) |
         (ms_read_ddrc_reg(memshire, 2, APBONLY0_UctDatWriteOnlyShadow) << 16);                 // read message

  ms_write_ddrc_reg(memshire, 2, APBONLY0_DctWriteProt, 0x00000000);                            // ack message
  ms_poll_ddrc_reg(memshire, 2, APBONLY0_UctShadowRegs, 0x00000001, 0x1, 100, 1);               // wait for handshake
  ms_write_ddrc_reg(memshire, 2, APBONLY0_DctWriteProt, 0x00000001);                            // ack handshake

  return mail;
}

static void get_streaming_messages(uint32_t memshire)
{
  uint32_t mail;
  uint16_t count;

  mail = get_mail(memshire);
  Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][sms]0x%08x", memshire, mail);

  count = mail & 0xffff;
  while(count-- > 0) {
    mail = get_mail(memshire);
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][smd]0x%08x", memshire, mail);
  }
}

static void wait_for_training_internal(training_stage stage, uint32_t memshire, uint32_t train_poll_max_iterations, uint32_t train_poll_iteration_delay)
{
  uint8_t major_msg;
  uint8_t number_of_shire_completed;
  uint8_t shire_completed[NUMBER_OF_MEMSHIRE] = { 0 };

  Log_Write(LOG_LEVEL_DEBUG, "DDR:[0][txt]Wait for training");

  // memshire is not define outside, use it as a local variable
  memshire = 0;
  number_of_shire_completed = 0;
  while(number_of_shire_completed < NUMBER_OF_MEMSHIRE) {

    if(memshire == NUMBER_OF_MEMSHIRE) {
      usdelay(train_poll_iteration_delay);      // delay between each cycle
      memshire = 0;
    }

    // Check if a specific shire is completed or a message is ready to be processed
    if(shire_completed[memshire] || ms_peek_ddrc_reg(memshire, 2, APBONLY0_UctShadowRegs, 0x0, 0x1) != 0) {
      // none of above happen, try next memshire
      ++memshire;
      continue;
    }

    major_msg = (uint8_t)(ms_read_ddrc_reg(memshire, 2, APBONLY0_UctWriteOnlyShadow) & 0xff);     // read message

    ms_write_ddrc_reg(memshire, 2, APBONLY0_DctWriteProt, 0x00000000);                            // ack message
    ms_poll_ddrc_reg(memshire, 2, APBONLY0_UctShadowRegs, 0x00000001, 0x1, 100, 1);               // wait for handshake
    ms_write_ddrc_reg(memshire, 2, APBONLY0_DctWriteProt, 0x00000001);                            // ack handshake

    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][mms]0x%08x", memshire, major_msg);
    LOG_TRAINING(stage, memshire, major_msg);

    if(major_msg == 0x08) {
      // process streaming messages on 0x08
      get_streaming_messages(memshire);
    }
    else if(major_msg == 0x7) {
      // mark the memshire as completed on success(0x7).
      Log_Write(LOG_LEVEL_CRITICAL, "DDR:[%d][txt]Training success.", memshire);
      shire_completed[memshire] = 1;
      ++number_of_shire_completed;
    }
    else if(major_msg == 0xff || major_msg == 0x0b) {
      // mark the memshire as completed on fail(0xff/0x0b).
      Log_Write(LOG_LEVEL_ERROR, "DDR:[%d][txt]Training failure!", memshire);
      shire_completed[memshire] = 1;
      ++number_of_shire_completed;
    }

    ++memshire;

  }

  // System software doesn't use these 2 variables.  But we want to keep the same interface by hardware team
  (void)train_poll_max_iterations;    // suppress compilation warning
  return;
}

/*
** Support functions for ddr
*/
#if (DDR_DIAG & DDR_DIAG_MEMSHIRE_ID)
void check_memshire_revision_id(uint32_t memshire)
{
    uint64_t rev_id;

    rev_id = ms_read_esr(memshire, ms_mem_revision_id);
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]MemshireID = 0x%02x\n", memshire, (uint8_t) ETSOC_MEM_SHIRE_ESR_MS_MEM_REVISION_ID_MEMSHIRE_ID_GET(rev_id));
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]RevisionB3 = 0x%02x\n", memshire, (uint8_t) ETSOC_MEM_SHIRE_ESR_MS_MEM_REVISION_ID_MEMSHIRE_REVISION_B3_GET(rev_id));
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]RevisionB2 = 0x%02x\n", memshire, (uint8_t) ETSOC_MEM_SHIRE_ESR_MS_MEM_REVISION_ID_MEMSHIRE_REVISION_B2_GET(rev_id));
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]RevisionB1 = 0x%02x\n", memshire, (uint8_t) ETSOC_MEM_SHIRE_ESR_MS_MEM_REVISION_ID_MEMSHIRE_REVISION_B1_GET(rev_id));
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]RevisionB0 = 0x%02x\n", memshire, (uint8_t) ETSOC_MEM_SHIRE_ESR_MS_MEM_REVISION_ID_MEMSHIRE_REVISION_B0_GET(rev_id));
    Log_Write(LOG_LEVEL_DEBUG, "DDR:[%d][txt]Raw = 0x%016lx\n", memshire, rev_id);
}
#endif // (DDR_DIAG & DDR_DIAG_MEMSHIRE_ID)

/*
** Low-level support functions
*/
uint64_t ms_read_esr(uint32_t memshire, uint64_t reg)
{
  return *(volatile uint64_t*)GET_MEMSHIRE_ESR(memshire, reg);
}

void ms_write_esr(uint32_t memshire, uint64_t reg, uint64_t value)
{
  *(volatile uint64_t*)GET_MEMSHIRE_ESR(memshire, reg) = value;

#if (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
  uint64_t read_back = ms_read_esr(memshire, reg);
  if(read_back != value) {
    Log_Write(LOG_LEVEL_ERROR, "DDR:[%d][txt]ms_write_esr() write verification fails at register 0x%016lx (written 0x%016lx, read back 0x%016lx)\n",
      memshire, reg, value, read_back);
  }
#endif // (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
}

uint32_t ms_read_reg(uint32_t memshire, uint64_t reg)
{
  return *(volatile uint32_t*)GET_MEMSHIRE_ESR(memshire, reg);
}

void ms_write_reg(uint32_t memshire, uint64_t reg, uint32_t value)
{
  *(volatile uint32_t*)GET_MEMSHIRE_ESR(memshire, reg) = value;

#if (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
  uint32_t read_back = ms_read_reg(memshire, reg);
  if(read_back != value) {
    Log_Write(LOG_LEVEL_ERROR, "DDR:[%d][txt]ms_write_reg() write verification fails at register 0x%016lx (written 0x%08x, read back 0x%08x)\n",
      memshire, reg, value, read_back);
  }
#endif // (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
}

uint32_t ms_read_ddrc_reg(uint32_t memshire, uint32_t blk, uint64_t reg)
{
  return *(volatile uint32_t*) get_ddrc_address(memshire, blk, reg);
}

void ms_write_ddrc_reg(uint32_t memshire, uint32_t blk, uint64_t reg, uint32_t value)
{
  *(volatile uint32_t*) get_ddrc_address(memshire, blk, reg) = value;

#if (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
  uint32_t read_back = ms_read_ddrc_reg(memshire, blk, reg);
  if(read_back != value) {
    Log_Write(LOG_LEVEL_ERROR, "DDR:[%d][txt]ms_write_ddrc_reg() write verification fails at register 0x%016lx (written 0x%08x, read back 0x%08x)\n",
      memshire, reg, value, read_back);
  }
#endif // (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
}

void ms_write_both_ddrc_reg(uint32_t memshire, uint64_t reg, uint32_t value)
{
  *(volatile uint32_t*) get_ddrc_address(memshire, 0, reg) = value;
  *(volatile uint32_t*) get_ddrc_address(memshire, 1, reg) = value;

#if (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
  uint32_t read_back0 = ms_read_ddrc_reg(memshire, 0, reg);
  uint32_t read_back1 = ms_read_ddrc_reg(memshire, 1, reg);
  if(read_back0 != value || read_back1 != value) {
    Log_Write(LOG_LEVEL_ERROR, "DDR:[%d][txt]ms_write_both_ddrc_reg() write verification fails at register 0x%016lx (written 0x%08x, read back[0] 0x%08x, read back[1] 0x%08x)\n",
      memshire, reg, value, read_back0, read_back1);
  }
#endif // (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
}

void ms_write_ddrc_addr(uint32_t memshire, uint64_t addr_in, uint32_t value)
{
  *(volatile uint32_t*) get_ddrc_address(memshire, 0, addr_in) = value;

#if (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
  uint32_t read_back = ms_read_ddrc_reg(memshire, 0, addr_in);
  if(read_back != value) {
    Log_Write(LOG_LEVEL_ERROR, "DDR:[%d][txt]ms_write_ddrc_addr() write verification fails at register 0x%016lx (written 0x%08x, read back 0x%08x)\n",
      memshire, addr_in, value, read_back);
  }
#endif // (DDR_DIAG & DDR_DIAG_VERIFY_REGISTER_WRITE)
}

uint32_t ms_poll_pll_reg(uint32_t memshire, uint64_t reg, uint32_t wait_value, uint32_t wait_mask, uint32_t timeout_tries)
{
  while(1) {
    uint32_t value = ms_read_reg(memshire, reg);
    if( ((value ^ wait_value) & wait_mask) == 0)
      break;

    timeout_tries--;
    if(timeout_tries == 0) {
      break;
    }
    usdelay(1);
  }

  return timeout_tries;
}

uint32_t ms_poll_ddrc_reg(uint32_t memshire, uint32_t blk, uint64_t reg, uint32_t wait_value, uint32_t wait_mask, uint32_t timeout_tries, uint32_t wait_count)
{
  while(1) {
    uint32_t value = ms_read_ddrc_reg(memshire, blk, reg);
    if( ((value ^ wait_value) & wait_mask) == 0)
      break;

    timeout_tries--;
    if(timeout_tries == 0) {
      break;
    }
    usdelay(wait_count);
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
  switch(mem_config) {
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
      Log_Write(LOG_LEVEL_ERROR, "DDR:[0][txt]Unrecongnized mem_config = %d", mem_config);
      return;
  }

  load_training_fw(ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING, region_id);

  // suppress compilation warning
  (void) memshire;
}

void ms_wait_for_training(uint32_t memshire, uint32_t train_poll_max_iterations, uint32_t train_poll_iteration_delay)
{
  wait_for_training_internal(TRAINING_1D, memshire, train_poll_max_iterations, train_poll_iteration_delay);
}

void ms_ddr_phy_2d_train_from_file(uint32_t mem_config, uint32_t memshire)
{
  ESPERANTO_FLASH_REGION_ID_t region_id;

  // decide which frequency to run
  switch(mem_config) {
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
      Log_Write(LOG_LEVEL_ERROR, "DDR:[0][txt]Unrecongnized mem_config = %d", mem_config);
      return;
  }

  load_training_fw(ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING_2D, region_id);

  // suppress compilation warning
  (void) memshire;
}

void ms_wait_for_training_2d(uint32_t memshire, uint32_t train_poll_max_iterations, uint32_t train_poll_iteration_delay)
{
  wait_for_training_internal(TRAINING_2D, memshire, train_poll_max_iterations, train_poll_iteration_delay);
}

void post_train_update_regs(uint32_t memshire)
{
  // suppress compilation warning
  (void) memshire;
}
