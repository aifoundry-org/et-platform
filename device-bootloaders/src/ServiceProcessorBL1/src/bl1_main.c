#include "et_cru.h"
#include "serial.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "printx.h"

#include "system_registers.h"
#include "service_processor_ROM_data.h"

int bl1_main(const SERVICE_PROCESSOR_ROM_DATA_t * rom_data);

int bl1_main(const SERVICE_PROCESSOR_ROM_DATA_t * rom_data)
{
    SERIAL_init(UART0);
    printx("*** SP BL1 STARTED ***\r\n");

    printx("SP ROM data address: %x\n", rom_data);

    printx("*** SP BL1 FINISHED ***\r\n");

    while (1) {}
}
