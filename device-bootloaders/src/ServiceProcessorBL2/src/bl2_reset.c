#include "serial.h"

#include <stdio.h>

#include "bl2_reset.h"

#include "etsoc_hal/cm_esr.h"
#include "etsoc_hal/rm_esr.h"
#include "hal_device.h"

int release_memshire_from_reset(void) {
    volatile Reset_Manager_t * rm_esr = (Reset_Manager_t *)R_SP_CRU_BASEADDR;
    //volatile Clock_Manager_t * cm_esr = (Clock_Manager_t *)R_SP_CRU_BASEADDR;

    rm_esr->rm_memshire_cold.R = ((Reset_Manager_rm_memshire_cold_t){ .B = { .rstn = 0x01 } }).R;
    rm_esr->rm_memshire_warm.R = ((Reset_Manager_rm_memshire_warm_t){ .B = { .rstn = 0xFF } }).R;
    return 0;
}

int release_minions_from_reset(void) {
    volatile Reset_Manager_t * rm_esr = (Reset_Manager_t *)R_SP_CRU_BASEADDR;
    //volatile Clock_Manager_t * cm_esr = (Clock_Manager_t *)R_SP_CRU_BASEADDR;

    rm_esr->rm_minion.R = ((Reset_Manager_rm_minion_t){ .B = { .cold_rstn = 1, .warm_rstn = 1 } }).R;
    rm_esr->rm_minion_warm_a.R = ((Reset_Manager_rm_minion_warm_a_t){ .B = { .rstn = 0xFFFFFFFF } }).R;
    rm_esr->rm_minion_warm_b.R = ((Reset_Manager_rm_minion_warm_b_t){ .B = { .rstn = 0x3 } }).R;
 
    return 0;
}
