#include "et_cru.h"
#include "serial.h"

#include "printx.h"
#include "spi_controller.h"
#include "spi_flash.h"
#include "flash_fs.h"
#include "jedec_sfdp.h"

int flash_fs_init(SPI_FLASH_ID_t flash_id) {
    return spi_flash_init(flash_id);
}

int flash_fs_get_flash_size(SPI_FLASH_ID_t flash_id, uint32_t * size) {
    uint8_t manufacturer_id;
    uint16_t device_id;
    static SFDP_HEADER_t sfdp_header;
    //static SFDP_PARAMETER_HEADER_t sfdp_parameter_header;
    //static SFDP_BASIC_FLASH_PARAMETER_TABLE_t sfdp_basic_parameter_table;

    if (0 != spi_flash_rdid(flash_id, &manufacturer_id, &device_id)) {
        return -1;
    }

    printx("RDID: man_id: 0x%02x, dev_id: 0x%04x\n", manufacturer_id, device_id);

    if (0 != spi_flash_rdsfdp(flash_id, 0, (uint8_t*)&sfdp_header, sizeof(sfdp_header))) {
        return -1;
    }

    printx("SFDP: signature: %c%c%c%c [%02x %02x %02x %02x]\n", sfdp_header.signature[3], sfdp_header.signature[2], sfdp_header.signature[1], sfdp_header.signature[0],
                                                                sfdp_header.signature[3], sfdp_header.signature[2], sfdp_header.signature[1], sfdp_header.signature[0]);
    printx("SFDP: minor=0x%x, major=0x%x, noph=0x%x, ap=0x%x\n", sfdp_header.minor_revision, sfdp_header.major_revision, sfdp_header.number_of_parameter_headers, sfdp_header.access_protocol);

    *size = 0;
    return 0;
}
