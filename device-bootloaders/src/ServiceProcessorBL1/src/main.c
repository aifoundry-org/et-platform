#include "et_cru.h"
#include "serial.h"

#include "printx.h"

#define TEST_SPI
//#define LOAD_FIRMWARE
//#define USE_CRC32

#ifdef LOAD_FIRMWARE
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#endif

#include "vaultip_hw.h"
#include "system_registers.h"
#include "flash_fs.h"

// Select SPIO peripherals for initial SP use
#define SPIO_NOC_SPIO_REGBUS_BASE_ADDRESS    0x0040100000ULL
#define SPIO_NOC_PU_MAIN_REGBUS_BASE_ADDRESS 0x0040200000ULL
#define SPIO_NOC_PSHIRE_REGBUS_BASE_ADDRESS  0x0040300000ULL
#define SPIO_SRAM_BASE_ADDRESS               0x0040400000ULL
#define SPIO_SRAM_SIZE                       0x100000UL // 1MB
#define SPIO_MAIN_NOC_REGBUS_BASE_ADDRESS    0x0042000000ULL
#define SPIO_PLIC_BASE_ADDRESS               0x0050000000ULL
#define SPIO_UART0_BASE_ADDRESS              0x0052022000ULL

#ifdef LOAD_FIRMWARE
#define VIP_FW_RAM_ADDR                      0x40410000
#define VIP_FW_RAM_SIZE                      82240
#define VIP_FW_RAM_CRC32                     0xd5fa7e38
#define FIRMWARE_HEADER_SIZE 256

#define VIP_FW_RAM_DW_0                      0xcf000000 // 00 00 00 cf 
#define VIP_FW_RAM_DW_1                      0x02775746 // 46 57 77 02
#define VIP_FW_RAM_DW_n_minus_1              0x6778a107 // 07 a1 78 67
#define VIP_FW_RAM_DW_n                      0x65073e1c // 1c 3e 07 65

#define SPIO_SPI0_BASE_ADDR                 0x52021000
#define SPIO_SPI1_BASE_ADDR                 0x54051000

static inline void * volatile_cast(volatile void * vp) {
  union {
    volatile void * vp;
    void * p;
  } u;

  u.vp = vp;
  return u.p;
}

#ifdef USE_CRC32
// Simple public domain implementation of the standard CRC32 checksum.
static uint32_t crc32_for_byte(uint32_t r) {
  for(int j = 0; j < 8; ++j)
    r = (r & 1? 0: (uint32_t)0xEDB88320L) ^ r >> 1;
  return r ^ (uint32_t)0xFF000000L;
}
static void crc32(const void *data, size_t n_bytes, uint32_t* crc) {
  static uint32_t table[0x100];
  if(!*table)
    for(size_t i = 0; i < 0x100; ++i)
      table[i] = crc32_for_byte((uint32_t)i);
  for(size_t i = 0; i < n_bytes; ++i)
    *crc = table[(const uint8_t)*crc ^ ((const uint8_t*)data)[i]] ^ *crc >> 8;
}
#endif

static int test_ram_firmware(void) {
#ifdef USE_CRC32
    void * data = (void*)VIP_FW_RAM_ADDR;
    uint32_t crc = 0;
    crc32(data, VIP_FW_RAM_SIZE, &crc);
    printx("VIP FW RAM image CRC32: %08x\n", crc);
    if (VIP_FW_RAM_CRC32 == crc) {
      return 0;
    } else {
      return -1;
    }
#else
    uint32_t * data = (uint32_t*)VIP_FW_RAM_ADDR;
    uint32_t last_index = (VIP_FW_RAM_SIZE / 4) - 1;
    if (VIP_FW_RAM_DW_0 != data[0]) {
      return -1;
    }
    if (VIP_FW_RAM_DW_1 != data[1]) {
      return -1;
    }
    if (VIP_FW_RAM_DW_n_minus_1 != data[last_index-1]) {
      return -1;
    }
    if (VIP_FW_RAM_DW_n != data[last_index]) {
      return -1;
    }
    return 0;
#endif
}
#endif

#ifdef LOAD_FIRMWARE
static int load_firmware(void) {
    volatile VAULTIP_HW_REGS_t * vaultip_regs = VAULTIP_REGISTERS;
    const uint32_t * firmware = (const uint32_t*)VIP_FW_RAM_ADDR;
    uint32_t countdown;
    EIP_VERSION_t eip_version;
    MODULE_STATUS_t module_status;
    MAILBOX_STAT_t mailbox_stat;

      // check VaultIP firmware registers

    eip_version = vaultip_regs->EIP_VERSION;
    if (0x82 != eip_version.R.EIP_number || 0x7D != eip_version.R.EIP_number_complement) {
        printx("EIP_VERSION mismatch!\n");
        return -1;
    }

    module_status = vaultip_regs->MODULE_STATUS;    
    if (0 == module_status.R.CRC24_OK) {
        printx("MODULE_STATUS CRC24_OK is not 1!\n");
        return -1;
    }
    if (0 != module_status.R.FatalError) {
        printx("MODULE_STATUS FatalError is 1!\n");
        return -1;
    }
    if (0 != module_status.R.fw_image_written) {
        printx("MODULE_STATUS fw_image_written is 1!\n");
        return -1;
    }
    if (0 != module_status.R.fw_image_checks_done) {
        printx("MODULE_STATUS fw_image_checks_done is 1!\n");
        return -1;
    }
    if (0 != module_status.R.fw_image_accepted) {
        printx("MODULE_STATUS fw_image_accepted is 1!\n");
        return -1;
    }

    // link mailbox

    vaultip_regs->MAILBOX_CTRL = (MAILBOX_CTRL_t){ .R.mbx1_link = 1 };
    mailbox_stat = vaultip_regs->MAILBOX_STAT;
    if (1 != mailbox_stat.R.mbx1_linked) {
        printx("MAILBOX_STAT mbx1_linked is still 0!\n");
        return -1;
    }
    
    printx("Linked mailbox...\n");

    // write RAM-based Firmware Code Validation and Boot token (1st 256 bytes of the image header)

    memcpy(volatile_cast(vaultip_regs->MAILBOX1), firmware, FIRMWARE_HEADER_SIZE);
    vaultip_regs->MAILBOX_CTRL = (MAILBOX_CTRL_t){ .R.mbx1_in_full = 1 };
    mailbox_stat = vaultip_regs->MAILBOX_STAT;
    if (1 != mailbox_stat.R.mbx1_in_full) {
        printx("MAILBOX_STAT mbx1_in_full is not 1!\n");
        return -1;
    }

    // unlink mailbox

    vaultip_regs->MAILBOX_CTRL = (MAILBOX_CTRL_t){ .R.mbx1_unlink = 1 };
    mailbox_stat = vaultip_regs->MAILBOX_STAT;
    if (1 == mailbox_stat.R.mbx1_linked) {
        printx("MAILBOX_STAT mbx1_linked is still 1!\n");
        return -1;
    }

    printx("Wrote header... (MAILBOX_STAT = 0x%x)\n", mailbox_stat.u32);
    // write the rest of firmware to FWRAM

    memcpy(volatile_cast(vaultip_regs->FWRAM_IN), firmware + FIRMWARE_HEADER_SIZE, VIP_FW_RAM_SIZE - FIRMWARE_HEADER_SIZE);

    // report that the firmware is written

    module_status = vaultip_regs->MODULE_STATUS;
//    printx("MODULE_STATUS = 0x%x\n", module_status.u32);

    module_status.R.fw_image_written = 1;
    vaultip_regs->MODULE_STATUS = module_status;
    module_status = vaultip_regs->MODULE_STATUS;
//    printx("Set fw_image_written, MODULE_STATUS = 0x%x\n", module_status.u32);
    if (0 == module_status.R.CRC24_OK) {
        printx("Firmware write failed, MODULE_STATUS CRC24_OK is not 1!\n");
        return -1;
    }
    if (0 != module_status.R.FatalError) {
        printx("Firmware write failed, MODULE_STATUS FatalError!\n");
        return -1;
    }

//    printx("Wrote image... (MODULE_STATUS = 0x%x)\n", module_status.u32);

    // wait for firmware check to start
    for (countdown = 0x10000; countdown > 0; countdown--) {
        module_status = vaultip_regs->MODULE_STATUS;
        if (0 == module_status.R.fw_image_checks_done) {
            goto FW_IMAGE_CHECKS_STARTED;
        }
    }
    printx("Timeout waiting for firmware checks to start! (MODULE_STATUS = 0x%x)\n", module_status.u32);
    return -1;

FW_IMAGE_CHECKS_STARTED:

    // wait for firmware accepted
    for (countdown = 0x80000000; countdown > 0; countdown--) {
        module_status = vaultip_regs->MODULE_STATUS;
        if (1 == module_status.R.fw_image_checks_done) {
            goto FW_IMAGE_CHECKS_DONE;
        }
    }
    printx("Timeout waiting for firmware checks done! (MODULE_STATUS = 0x%x)\n", module_status.u32);
    return -1;

FW_IMAGE_CHECKS_DONE:
    printx("Firmware checks done.\n");

    if (0 == module_status.R.fw_image_accepted) {
        printx("Firmware NOT accepted! MODULE_STATUS = 0x%X\n", module_status.u32);
        return -1;
    } else {
        printx("Firmware accepted. MODULE_STATUS = 0x%X\n", module_status.u32);
        return 0;
    }

}

static void dump_vip_regs(volatile VAULTIP_HW_REGS_t* vaultip_regs) {
    uint32_t val;

    val = vaultip_regs->EIP_VERSION.u32;
    printx("EIP_VERSION: 0x%x\n", val);

    val = vaultip_regs->EIP_OPTIONS2.u32;
    printx("EIP_OPTIONS2: 0x%x\n", val);

    val = vaultip_regs->EIP_OPTIONS.u32;
    printx("EIP_OPTIONS: 0x%x\n", val);

    val = vaultip_regs->MODULE_STATUS.u32;
    printx("MODULE_STATUS: 0x%x\n", val);

    val = vaultip_regs->MAILBOX_STAT.u32;
    printx("MAILBOX_STAT: 0x%x\n", val);

    val = vaultip_regs->MAILBOX_RAWSTAT.u32;
    printx("MAILBOX_RAWSTAT: 0x%x\n", val);

    val = vaultip_regs->MAILBOX_LINKID.u32;
    printx("MAILBOX_LINKID: 0x%x\n", val);

    val = vaultip_regs->MAILBOX_OUTID.u32;
    printx("MAILBOX_OUTID: 0x%x\n", val);

    val = vaultip_regs->MAILBOX_LOCKOUT.u32;
    printx("MAILBOX_LOCKOUT: 0x%x\n", val);
}

typedef uint64_t (*MISALIGNED_PFN)(void);
#endif

int main(void)
{
#ifdef TEST_SPI
    uint32_t flash_size;
#endif
    //MISALIGNED_PFN misaligned_pfn;
    // uint32_t n;
#ifdef LOAD_FIRMWARE
    volatile VAULTIP_HW_REGS_t* vaultip_regs = VAULTIP_REGISTERS;
#endif
    SERIAL_init(UART0);
    printx("*** SP ROM STARTED ***\r\n");
#ifdef TEST_SPI
    flash_fs_init(SPI_FLASH_ON_PACKAGE);
    flash_fs_get_flash_size(SPI_FLASH_ON_PACKAGE, &flash_size);

    printx("\r\n");
#endif

#ifdef LOAD_FIRMWARE
    dump_vip_regs(vaultip_regs);

    if (0 != test_ram_firmware()) {
      printx("VaultIP RAM FW CRC32 mismatch!\n");
    } else {
      if (0 != load_firmware()) {
        printx("Failed to load the VaultIP RAM FW image (1st attempt)!\n");
        dump_vip_regs(vaultip_regs);
        if (0 != load_firmware()) {
          printx("Failed to load the VaultIP RAM FW image (2nd attempt)!\n");
        } else {
          printx("VaultIP RAM FW image loaded OK.\n");
        }
      } else {
        printx("VaultIP RAM FW image loaded OK.\n");
      }
    }

    dump_vip_regs(vaultip_regs);
#endif
    printx("*** SP ROM FINISHED ***\r\n");

    while (1) {}
}
