#include <stdio.h>

#include <etsoc_hal/inc/etsoc_shire_other_esr.h>

#include "bl2_minion_pll_and_dll.h"
#include "minion_esr_defines.h"

/*==================== Function Separator =============================*/
#define TIMEOUT_PLL_CONFIG 100000
#define TIMEOUT_PLL_LOCK 100000

/*==================== Function Separator =============================*/
static void pll_config(uint8_t shire_id)
{
    uint64_t reg_value;

    // Select 1 GHz from step_clock, Bits[2:0] = 3'b011. Bit 3 to '1' to go with DLL output
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_CTRL_CLOCKMUX, 0xb);

    // PLL and DLL initialization
    /////////////////////////////////////////////////////////////////////////////
    // Initialize auto-config register and reset PLL
    //reg_value =  SHIRE_OTHER_PLL_AUTO_CONFIG_PCLK_SEL
    //          | (0x1 << SHIRE_OTHER_PLL_AUTO_CONFIG_RESET_OFF);
    //write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, reg_value);

    // Initialize auto-config register and reset DLL
    //reg_value =  SHIRE_OTHER_DLL_AUTO_CONFIG_PCLK_SEL
    //          | (0x1 << SHIRE_OTHER_DLL_AUTO_CONFIG_RESET_OFF);
    //write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_DLL_AUTO_CONFIG, reg_value);

    // PLL configuration (register values taken from Movellus testbench for LVDPLL v1.2.1a)
    /////////////////////////////////////////////////////////////////////////////
/*  SW-3876 Moving Compute Minion PLL configuration to Master Minion since its will support dynamic scaling of Frequency as DVFS is implemented: https://esperantotech.atlassian.net/wiki/spaces/SW/pages/836337737/Power+Management+System+Software+Specification#Cold-Reset-Sequence
    // Write PLL registers 0x00 to 0x0f. PLL registers can be downloaded in chunks of 16 registers
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x0000000f000101b8); // 750 MHz
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_1, 0x02bb1bf40aeb02bb);
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_2, 0x010c01f01bf40aeb);
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_3, 0x000200020000000c);
    pll_config_multiple_write(shire_id,0x00,0xf);

    // Write PLL registers 0x10 to 0x1a.
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x0000000000020002); // 750 MHz
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_1, 0x0000000000000000);
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_2, 0x0000000000000001);
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_3, 0x0000000000000000);
    pll_config_multiple_write(shire_id,0x10,0xa);

    // Write PLL registers 0x20 to 0x24.
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x0000000000000003); // 750 MHz
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_1, 0x0000000000000000);
    pll_config_multiple_write(shire_id,0x20,0x4);

    // Write PLL registers 0x27 to 0x28.(DATA_1 and DATA_2 and DATA_3 not changed)
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x000000000000000c);
    pll_config_multiple_write(shire_id,0x27,0x1);

    // Write PLL strobe register 0x38 to load previous configuration
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x0000000000000001);
    pll_config_multiple_write(shire_id,0x38,0x0);

    // Wait until PLL is locked to change clock mux
    while(!(read_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_PLL_READ_DATA) & 0x20000));

    // Select PLL[0] output. Bits[2:0]=3'b100. Bit 3 to '1' to go with DLL output
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_CTRL_CLOCKMUX, 0xc);
*/
    // DLL configuration (register values taken from Movellus testbench v 1.1.0b)
    /////////////////////////////////////////////////////////////////////////////

    // Auto-config register set dll_enable and get reset deasserted of the DLL
    reg_value =  SHIRE_OTHER_DLL_AUTO_CONFIG_PCLK_SEL
             | (0x1 << SHIRE_OTHER_DLL_AUTO_CONFIG_DLL_EN_OFF);
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_DLL_AUTO_CONFIG, reg_value);

    // Wait until DLL is locked to change clock mux
    while(!(read_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_DLL_READ_DATA) & 0x20000));
}

static void minion_pll_config(uint64_t shire_mask) {
    for (uint8_t i = 0; i <= 32; i++) {
        if (shire_mask & 1)
            pll_config(i);
        shire_mask >>= 1;
    }
}

int configure_minion_plls_and_dlls(uint64_t shire_mask) {
    minion_pll_config(shire_mask);
    return 0;
}

int enable_minion_neighborhoods(uint64_t shire_mask) {
    for (uint8_t i = 0; i <= 32; i++) {
        if (shire_mask & 1) {
            // Set Shire ID, enable cache and all Neighborhoods
            const uint64_t config =
                ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_SHIRE_ID_SET(i) |
                ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_CACHE_EN_SET(1) |
                ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_NEIGH_EN_SET(0xF);
            write_esr(PP_MACHINE, i, REGION_OTHER, SHIRE_OTHER_CONFIG, config);
        }
        shire_mask >>= 1;
    }
    return 0;
}

int enable_master_shire_threads(uint8_t mm_id) {
    // Enable only Device Runtime Management thread on Master Shire
    write_esr(PP_MACHINE, mm_id, REGION_OTHER, SHIRE_OTHER_THREAD0_DISABLE, ~(MM_RT_THREADS));
    return 0;
}
