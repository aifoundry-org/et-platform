#include "serial.h"
#include "etsoc_hal/mvls_tn7_hpdpll.ipxact.h"
#include "etsoc_hal/rm_esr.h"
#include "etsoc_hal/cm_esr.h"
#include "etsoc_hal/pshire_esr.h"
#include "hal_device.h"

#include <stdio.h>

#include "sp_pll.h"

#include "movellus_pll_modes_config.h"

#define PLL_LOCK_TIMEOUT 10000

#define PLL_REG_INDEX_REG_0 0
#define PLL_REG_INDEX_REG_UPDATE_STROBE 0x38
#define PLL_REG_INDEX_REG_LOCK_DETECT_STATUS 0x39
#define DCO_NORMALIZATION_ENABLE__SHIFT 7u
#define DCO_NORMALIZATION_ENABLE__MASK (1u << DCO_NORMALIZATION_ENABLE__SHIFT)

static int release_pshire_from_reset(void) {
    volatile Reset_Manager_t * reset_manager = (Reset_Manager_t*)R_SP_CRU_BASEADDR;
    reset_manager->rm_pshire_cold.R = (Reset_Manager_rm_pshire_cold_t){ .B = { .rstn = 1 }}.R;
    reset_manager->rm_pshire_warm.R = (Reset_Manager_rm_pshire_warm_t){ .B = { .rstn = 1 }}.R;
    return 0;
}

static void update_pll_registers(volatile uint32_t * pll_registers) {
    uint32_t strobe;

    strobe = pll_registers[PLL_REG_INDEX_REG_UPDATE_STROBE];
    strobe = strobe & 0xFFFFFFFE;
    pll_registers[PLL_REG_INDEX_REG_UPDATE_STROBE] = strobe;

    strobe = pll_registers[PLL_REG_INDEX_REG_UPDATE_STROBE];
    strobe = strobe | 1;
    pll_registers[PLL_REG_INDEX_REG_UPDATE_STROBE] = strobe;
}

static int configure_and_lock_pll(volatile uint32_t * pll_registers, uint32_t mode) {
    uint32_t timeout = PLL_LOCK_TIMEOUT;
    uint8_t register_index;
    uint16_t register_value;
    uint8_t entry_index;
    uint32_t pll_settings_index = mode - 1;
    uint32_t reg0;

    // program the PLL registers using generated configuration data
    for (entry_index = 0; entry_index < gs_pll_settings[pll_settings_index].count; entry_index++) {
        register_index = gs_pll_settings[pll_settings_index].offsets[entry_index];
        register_value = gs_pll_settings[pll_settings_index].values[entry_index];
        pll_registers[register_index] = register_value;
    }

    // Update PLL registers
    update_pll_registers(pll_registers);


    ///////////////////////////////////////////////////////////////////////////////////////
    // BUG WORKAROUND BEGIN
    ///////////////////////////////////////////////////////////////////////////////////////

    // Toggle the DCO_NORMALIZATION_ENABLE 1 -> 0 -> 1 in order for PLL to acquire lock
    // this is required to work around a HW bug in Movellus PLL

    // if the DCO_NORMALIZATION_ENABLE bit is NOT 1, set it to 1
    reg0 = pll_registers[PLL_REG_INDEX_REG_0];
    if (!(reg0 & DCO_NORMALIZATION_ENABLE__MASK)) {
        reg0 = reg0 | DCO_NORMALIZATION_ENABLE__MASK;
        pll_registers[PLL_REG_INDEX_REG_0] = reg0;
        update_pll_registers(pll_registers);
    }

    // set the DCO_NORMALIZATION_ENABLE bit to 0
    reg0 = pll_registers[PLL_REG_INDEX_REG_0];
    reg0 = reg0 & ~DCO_NORMALIZATION_ENABLE__MASK;
    pll_registers[PLL_REG_INDEX_REG_0] = reg0;
    update_pll_registers(pll_registers);

    // set the DCO_NORMALIZATION_ENABLE bit to 1
    reg0 = pll_registers[PLL_REG_INDEX_REG_0];
    reg0 = reg0 | DCO_NORMALIZATION_ENABLE__MASK;
    pll_registers[PLL_REG_INDEX_REG_0] = reg0;
    update_pll_registers(pll_registers);

    ///////////////////////////////////////////////////////////////////////////////////////
    // BUG WORKAROUND END
    ///////////////////////////////////////////////////////////////////////////////////////

    // wait for the PLL to lock
    while (timeout > 0) {
        if (pll_registers[PLL_REG_INDEX_REG_LOCK_DETECT_STATUS] & 1) {
            return 0;
        }
    }
    return -1;
}

int sp_pll_stage_0_init(void) {
    volatile Clock_Manager_t * sp_clock_manager = (volatile Clock_Manager_t *)R_SP_CRU_BASEADDR;
    volatile Pshire_t * pshire_manager = (volatile Pshire_t *)R_PCIE_ESR_BASEADDR;

    // configure and lock the PLLs

    if (0 != configure_and_lock_pll((uint32_t*)R_SP_PLL0_BASEADDR, 3)) { // mode 3 - 1 GHz
        return -1;
    }
    if (0 != configure_and_lock_pll((uint32_t*)R_SP_PLL1_BASEADDR, 1)) { // mode 1 - 2 GHz
        return -1;
    }

    if (0 != release_pshire_from_reset()) {
        return -1;
    }

    if (0 != configure_and_lock_pll((uint32_t*)R_PCIE_PLLP0_BASEADDR, 6)) { // mode 6 - 1080 MHz
        return -1;
    }

    // disable PLLs bypass

    sp_clock_manager->cm_ios_ctrl.R = ((Clock_Manager_cm_ios_ctrl_t){.B = { .mission = 1 }}).R;
    sp_clock_manager->cm_pll1_ctrl.R = ((Clock_Manager_cm_pll1_ctrl_t){.B = { .enable = 1 }}).R;
    pshire_manager->pshire_ctrl.R = ((Pshire_pshire_ctrl_t){.B = { .pll0_byp = 0 }}).R;

    return 0;
}

int sp_pll_stage_1_init(void) {
    volatile Clock_Manager_t * sp_clock_manager = (volatile Clock_Manager_t *)R_SP_CRU_BASEADDR;

    // configure and lock the PLLs

    if (0 != configure_and_lock_pll((uint32_t*)R_SP_PLL2_BASEADDR, 5)) { // mode 5 - 500 MHz
        return -1;
    }
    if (0 != configure_and_lock_pll((uint32_t*)R_SP_PLL4_BASEADDR, 3)) { // mode 3 - 1 GHz
        return -1;
    }

    // disable PLLs bypass

    sp_clock_manager->cm_pll2_ctrl.R = ((Clock_Manager_cm_pll2_ctrl_t){.B = { .enable = 1 }}).R;
    sp_clock_manager->cm_pll4_ctrl.R = ((Clock_Manager_cm_pll1_ctrl_t){.B = { .enable = 1 }}).R;

    return 0;
}
