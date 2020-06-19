#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "service_processor_BL1_data.h"

static void usage(const char *name);

static const struct option long_options[] = {
    {"output", required_argument, NULL, 'o'},
    {"help",   no_argument,       NULL, 'h'},
    {NULL,     0,                 NULL,  0 }
};

int main(int argc, char *argv[])
{
    int opt;
    FILE *fp;
    SERVICE_PROCESSOR_BL1_DATA_t data;
    const char *output = NULL;

    while ((opt = getopt_long(argc, argv, "o:h", long_options, NULL)) != -1) {
        switch (opt) {
        case 'o':
            output = optarg;
            break;
        case 'h':
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (!output) {
        fprintf(stderr, "Error: must select an output file\n");
        return EXIT_FAILURE;
    }

    memset(&data, 0, sizeof(data));
    data.service_processor_bl1_data_size = sizeof(data);
    data.service_processor_bl1_version = SERVICE_PROCESSOR_BL1_DATA_VERSION;
    data.service_processor_rom_version = 0xDEADBEEF;
    data.sp_gpio_pins = 0;
    data.sp_pll0_frequency = 0;
    data.sp_pll1_frequency = 0;
    data.pcie_pll0_frequency = 0;
    data.timer_raw_ticks_before_pll_turned_on = 0;
    data.vaultip_coid_set = 0;
    data.spi_controller_rx_baudrate_divider = 0;
    data.spi_controller_tx_baudrate_divider = 0;
    // data.flash_fs_bl1_info bypassed
    // data.pcie_config_header bypassed
    // data.sp_certificates[2] bypassed
    // data.sp_bl1_header bypassed
    // data.sp_bl2_header bypassed

    fp = fopen(output, "wb");
    if (!fp) {
        fprintf(stderr, "Error creating '%s' for writing: %s\n", output, strerror(errno));
        return EXIT_FAILURE;
    }

    fwrite(&data, sizeof(data), 1, fp);
    fclose(fp);

    printf("SERVICE_PROCESSOR_BL1_DATA_t written to '%s'\n", output);

    return EXIT_SUCCESS;
}

void usage(const char *name)
{
    printf(
        "Usage:\n    %s [OPTIONS] -o <output file>\n\n"
        "  -o, --output        output file\n"
        "  -h, --help          print help and exit\n"
        , name);
}
