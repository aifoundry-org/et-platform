/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

/**
 *  @Component      CRU
 *
 *  @Filename       et_cru_api.c
 *
 *  @Description    The CRU component contains API interface to SPIO CRU
 *
 *//*======================================================================== */


/* =============================================================================
 * STANDARD INCLUDE FILES
 * =============================================================================
 */

/* Common include files */
#include "cpu.h"
#include "et.h"
#include "api.h"
#include "soc.h"
#include "tb.h"

/* SPIO CRU specific include files */
#include "et_cru.h"
#include "et_cru_api.h"
#include "cm_esr.h"
#include "rm_esr.h"
#include "macros.h"
/* PLL specific include files */
#include "pllMovellus_api.h"
#include "pllMovellus_regs.h"


/* =============================================================================
 * GLOBAL VARIABLES DECLARATIONS
 * =============================================================================
 */

ET_CRU_API_t  et_cru_api;


/* =============================================================================
 * LOCAL TYPES AND DEFINITIONS
 * =============================================================================
 */



/* =============================================================================
 * LOCAL VARIABLES DECLARATIONS
 * =============================================================================
 */



/* =============================================================================
 * LOCAL FUNCTIONS PROTOTYPES
 * =============================================================================
 */


 /* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_getMode
 *
 * @BRIEF         Initialize CRU API
 *
 * @RETURNS       uint64_t mode
 *
 * @DESCRIPTION   Checks if we have a PLL configuration mode which can provide
 *                PLL output outputFreq given PLL reference clock inputFreq
 *                Returns modeId if there is a match, 0 otherwise
 *
 *//*------------------------------------------------------------------------ */
extern uint64_t et_cru_getMode( uint64_t inputFreq, uint64_t outputFreq );


/* =============================================================================
 * FUNCTIONS
 * =============================================================================
 */

/*==================== Function Separator =============================*/
void et_cru_init( void )
{
    et_cru_api.cm_esr = NULL;
    et_cru_api.rm_esr = NULL;

    et_cru_api.spio_pll0_handle = NULL;
    et_cru_api.spio_pll1_handle = NULL;
    et_cru_api.spio_pll2_handle = NULL;
    et_cru_api.spio_pll4_handle = NULL;

} /* et_cru_init() */

/*==================== Function Separator =============================*/
et_handle_t et_cru_open( API_IP_PARAMS_t *pIpParams )
{
    /* Initialize Data Structures */
    et_cru_api.cm_esr = ( Clock_Manager*)((uint64_t)pIpParams->baseAddress);
    et_cru_api.rm_esr = ( Reset_Manager*)((uint64_t)pIpParams->baseAddress);

    /* PLL handles will be assigned elsewhere if needed */

    return( (et_handle_t)&et_cru_api );

} /* et_cru_open() */

/*==================== Function Separator =============================*/
et_status_t et_cru_close( et_handle_t handle )
{
    et_status_t ets = ET_OK;
    ET_CRU_API_t *p_et_cru_api;

        /* Convert handle to ET_CRU_API_t */
    p_et_cru_api = (ET_CRU_API_t *)handle;

    /* Clear inUse Flag */
    p_et_cru_api->cm_esr = NULL;
    p_et_cru_api->rm_esr = NULL;

    p_et_cru_api->spio_pll0_handle = NULL;
    p_et_cru_api->spio_pll1_handle = NULL;
    p_et_cru_api->spio_pll2_handle = NULL;
    p_et_cru_api->spio_pll4_handle = NULL;

    return ets;

} /* pllMovellus_close() */


/*==================== Function Separator =============================*/
extern et_status_t et_cru_enablePLLoutput(et_handle_t handle, uint32_t pllId)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    switch (pllId) {
        case 0 :
            break;
        case 1 :
            set_bitField( et_cru_handle->cm_esr->cm_pll1_ctrl, CLOCK_MANAGER_CM_PLL1_CTRL_ENABLE, 0b1);
            break;
        case 2 :
            set_bitField( et_cru_handle->cm_esr->cm_pll2_ctrl, CLOCK_MANAGER_CM_PLL2_CTRL_ENABLE, 0b1);
            break;
        case 4 :
            set_bitField( et_cru_handle->cm_esr->cm_pll4_ctrl, CLOCK_MANAGER_CM_PLL4_CTRL_ENABLE, 0b1);
            break;
        default :
            return ET_FAIL;
        }

    return ET_OK;
} /* et_cru_enablePLLoutput */


/*==================== Function Separator =============================*/
et_status_t et_cru_disablePLLoutput(et_handle_t handle, uint32_t pllId)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    switch (pllId) {

        case 0 :
            break;
        case 1 :
             set_bitField( et_cru_handle->cm_esr->cm_pll1_ctrl, CLOCK_MANAGER_CM_PLL1_CTRL_ENABLE, 0b0);
            break;
        case 2 :
            set_bitField( et_cru_handle->cm_esr->cm_pll2_ctrl, CLOCK_MANAGER_CM_PLL2_CTRL_ENABLE, 0b0);
            break;
        case 4 :
            set_bitField( et_cru_handle->cm_esr->cm_pll4_ctrl, CLOCK_MANAGER_CM_PLL4_CTRL_ENABLE, 0b0);
            break;
        default :
            return ET_FAIL;
        }

    return ET_PASS;
} /* et_cru_disablePLLoutput */


/*==================== Function Separator =============================*/
et_status_t et_cru_enableAllInterrupts(et_handle_t handle)
{
    uint32_t reg32;
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    reg32 = et_cru_handle->cm_esr->cm_pll0_ctrl;
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL0_CTRL_LOSSINTR_EN, 0x1);
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL0_CTRL_LOCKINTR_EN, 0x1);

    et_cru_handle->cm_esr->cm_pll0_ctrl = reg32;

    reg32 = et_cru_handle->cm_esr->cm_pll1_ctrl;
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL1_CTRL_LOSSINTR_EN, 0x1);
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL1_CTRL_LOCKINTR_EN, 0x1);

    et_cru_handle->cm_esr->cm_pll1_ctrl = reg32;

    reg32 = et_cru_handle->cm_esr->cm_pll2_ctrl;
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL2_CTRL_LOSSINTR_EN, 0x1);
    set_bitField(reg32, CLOCK_MANAGER_CM_PLL2_CTRL_LOCKINTR_EN, 0x1);

    et_cru_handle->cm_esr->cm_pll2_ctrl = reg32;

    reg32 = et_cru_handle->cm_esr->cm_pll4_ctrl;
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL4_CTRL_LOSSINTR_EN, 0x1);
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL4_CTRL_LOCKINTR_EN, 0x1);

    et_cru_handle->cm_esr->cm_pll4_ctrl = reg32;

    return ET_OK;

}

/*==================== Function Separator =============================*/
et_status_t et_cru_enableAllLockInterrupts(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    set_bitField( et_cru_handle->cm_esr->cm_pll0_ctrl, CLOCK_MANAGER_CM_PLL0_CTRL_LOCKINTR_EN, 0x1);

    set_bitField( et_cru_handle->cm_esr->cm_pll1_ctrl, CLOCK_MANAGER_CM_PLL1_CTRL_LOCKINTR_EN, 0x1);

    set_bitField( et_cru_handle->cm_esr->cm_pll2_ctrl, CLOCK_MANAGER_CM_PLL2_CTRL_LOCKINTR_EN, 0x1);

    set_bitField( et_cru_handle->cm_esr->cm_pll4_ctrl, CLOCK_MANAGER_CM_PLL4_CTRL_LOCKINTR_EN, 0x1);

    return ET_OK;

}

/*==================== Function Separator =============================*/
et_status_t et_cru_enableAllLossInterrupts(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    set_bitField( et_cru_handle->cm_esr->cm_pll0_ctrl, CLOCK_MANAGER_CM_PLL0_CTRL_LOSSINTR_EN, 0x1);

    set_bitField( et_cru_handle->cm_esr->cm_pll1_ctrl, CLOCK_MANAGER_CM_PLL1_CTRL_LOSSINTR_EN, 0x1);

    set_bitField( et_cru_handle->cm_esr->cm_pll2_ctrl, CLOCK_MANAGER_CM_PLL2_CTRL_LOSSINTR_EN, 0x1);

    set_bitField( et_cru_handle->cm_esr->cm_pll4_ctrl, CLOCK_MANAGER_CM_PLL4_CTRL_LOSSINTR_EN, 0x1);

    return ET_OK;

}

/*==================== Function Separator =============================*/
et_status_t et_cru_disableAllInterrupts(et_handle_t handle)
{
    uint32_t reg32;
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    reg32 = et_cru_handle->cm_esr->cm_pll0_ctrl;
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL0_CTRL_LOSSINTR_EN, 0b0);
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL0_CTRL_LOCKINTR_EN, 0b0);
    et_cru_handle->cm_esr->cm_pll0_ctrl = reg32;

    reg32 = et_cru_handle->cm_esr->cm_pll1_ctrl;
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL1_CTRL_LOSSINTR_EN, 0b0);
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL1_CTRL_LOCKINTR_EN, 0b0);
    et_cru_handle->cm_esr->cm_pll1_ctrl = reg32;

    reg32 = et_cru_handle->cm_esr->cm_pll2_ctrl;
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL2_CTRL_LOSSINTR_EN, 0b0);
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL2_CTRL_LOCKINTR_EN, 0b0);
    et_cru_handle->cm_esr->cm_pll2_ctrl = reg32;

    reg32 = et_cru_handle->cm_esr->cm_pll4_ctrl;
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL4_CTRL_LOSSINTR_EN, 0b0);
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL4_CTRL_LOCKINTR_EN, 0b0);
    et_cru_handle->cm_esr->cm_pll4_ctrl = reg32;

    return ET_OK;

}


/*==================== Function Separator =============================*/
et_status_t et_cru_enablePllLockInterrupt(et_handle_t handle, uint32_t pllId)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;
    et_status_t est = ET_OK;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    /* First, disable all interrupts */
    est |= et_cru_disableAllInterrupts( et_cru_handle );

    /* Next, enable LOCK interrupt with pllId */
    switch(pllId)
    {
        case 0:
            set_bitField( et_cru_handle->cm_esr->cm_pll0_ctrl, CLOCK_MANAGER_CM_PLL0_CTRL_LOCKINTR_EN, 0b1);
            break;
        case 1:
            set_bitField( et_cru_handle->cm_esr->cm_pll1_ctrl, CLOCK_MANAGER_CM_PLL1_CTRL_LOCKINTR_EN, 0b1);
            break;
        case 2:
            set_bitField( et_cru_handle->cm_esr->cm_pll2_ctrl, CLOCK_MANAGER_CM_PLL2_CTRL_LOCKINTR_EN, 0b1);
            break;
        case 4:
            set_bitField( et_cru_handle->cm_esr->cm_pll4_ctrl, CLOCK_MANAGER_CM_PLL4_CTRL_LOCKINTR_EN, 0b1);
            break;
        default:
            est |= ET_FAIL;
            break;
    }

    return est;

} /* et_cru_enablePllLockInterrupt */


/*==================== Function Separator =============================*/
et_status_t et_cru_enablePllLossInterrupt(et_handle_t handle, uint32_t pllId)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;
    et_status_t est = ET_OK;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    /* First, disable all interrupts */
    est |= et_cru_disableAllInterrupts( et_cru_handle );

    /* Next, enable LOSS interrupt with pllId */
    switch(pllId)
    {
        case 0:
            set_bitField( et_cru_handle->cm_esr->cm_pll0_ctrl, CLOCK_MANAGER_CM_PLL0_CTRL_LOSSINTR_EN, 0b1);
            break;
        case 1:
            set_bitField( et_cru_handle->cm_esr->cm_pll1_ctrl, CLOCK_MANAGER_CM_PLL1_CTRL_LOSSINTR_EN, 0b1);
            break;
        case 2:
            set_bitField( et_cru_handle->cm_esr->cm_pll2_ctrl, CLOCK_MANAGER_CM_PLL2_CTRL_LOSSINTR_EN, 0b1);
            break;
        case 4:
            set_bitField( et_cru_handle->cm_esr->cm_pll4_ctrl, CLOCK_MANAGER_CM_PLL4_CTRL_LOSSINTR_EN, 0b1);
            break;
        default:
            est |= ET_FAIL;
            break;
    }

    return est;

} /* et_cru_enablePllLossInterrupt */


/*==================== Function Separator =============================*/
et_status_t et_cru_disablePllInterrupt(et_handle_t handle, uint32_t pllId)
{
    uint32_t reg32;
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;
    et_status_t est = ET_OK;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    switch(pllId)
    {
        case 0:
            reg32 = et_cru_handle->cm_esr->cm_pll0_ctrl;
            set_bitField( reg32, CLOCK_MANAGER_CM_PLL0_CTRL_LOSSINTR_EN, 0b0);
            set_bitField( reg32, CLOCK_MANAGER_CM_PLL0_CTRL_LOCKINTR_EN, 0b0);
            et_cru_handle->cm_esr->cm_pll0_ctrl = reg32;
            break;
        case 1:
            reg32 = et_cru_handle->cm_esr->cm_pll1_ctrl;
            set_bitField( reg32, CLOCK_MANAGER_CM_PLL1_CTRL_LOSSINTR_EN, 0b0);
            set_bitField( reg32, CLOCK_MANAGER_CM_PLL1_CTRL_LOCKINTR_EN, 0b0);
            et_cru_handle->cm_esr->cm_pll1_ctrl = reg32;
            break;
        case 2:
            reg32 = et_cru_handle->cm_esr->cm_pll2_ctrl;
            set_bitField( reg32, CLOCK_MANAGER_CM_PLL2_CTRL_LOSSINTR_EN, 0b0);
            set_bitField( reg32, CLOCK_MANAGER_CM_PLL2_CTRL_LOCKINTR_EN, 0b0);
            et_cru_handle->cm_esr->cm_pll2_ctrl = reg32;
            break;
        case 4:
            reg32 = et_cru_handle->cm_esr->cm_pll4_ctrl;
            set_bitField( reg32, CLOCK_MANAGER_CM_PLL4_CTRL_LOSSINTR_EN, 0b0);
            set_bitField( reg32, CLOCK_MANAGER_CM_PLL4_CTRL_LOCKINTR_EN, 0b0);
            et_cru_handle->cm_esr->cm_pll4_ctrl = reg32;
            break;
        default:
            est = ET_FAIL;
            break;
    }

    return est;

} /* et_cru_disablePllInterrupt */


/*==================== Function Separator =============================*/
et_status_t et_cru_disablePllLockInterrupt(et_handle_t handle, uint32_t pllId)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;
    et_status_t est = ET_OK;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    switch(pllId)
    {
        case 0:
            set_bitField( et_cru_handle->cm_esr->cm_pll0_ctrl, CLOCK_MANAGER_CM_PLL0_CTRL_LOCKINTR_EN, 0b0);
            break;
        case 1:
            set_bitField( et_cru_handle->cm_esr->cm_pll1_ctrl, CLOCK_MANAGER_CM_PLL1_CTRL_LOCKINTR_EN, 0b0);
            break;
        case 2:
            set_bitField( et_cru_handle->cm_esr->cm_pll2_ctrl, CLOCK_MANAGER_CM_PLL2_CTRL_LOCKINTR_EN, 0b0);
            break;
        case 4:
            set_bitField( et_cru_handle->cm_esr->cm_pll4_ctrl, CLOCK_MANAGER_CM_PLL4_CTRL_LOCKINTR_EN, 0b0);
            break;
        default:
            est = ET_FAIL;
            break;
    }

    return est;

} /* et_cru_disablePllLockInterrupt */


/*==================== Function Separator =============================*/
et_status_t et_cru_disablePllLossInterrupt(et_handle_t handle, uint32_t pllId)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;
    et_status_t est = ET_OK;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    switch(pllId)
    {
        case 0:
            set_bitField( et_cru_handle->cm_esr->cm_pll0_ctrl, CLOCK_MANAGER_CM_PLL0_CTRL_LOSSINTR_EN, 0b0);
            break;
        case 1:
            set_bitField( et_cru_handle->cm_esr->cm_pll1_ctrl, CLOCK_MANAGER_CM_PLL1_CTRL_LOSSINTR_EN, 0b0);
            break;
        case 2:
            set_bitField( et_cru_handle->cm_esr->cm_pll2_ctrl, CLOCK_MANAGER_CM_PLL2_CTRL_LOSSINTR_EN, 0b0);
            break;
        case 4:
            set_bitField( et_cru_handle->cm_esr->cm_pll4_ctrl, CLOCK_MANAGER_CM_PLL4_CTRL_LOSSINTR_EN, 0b0);
            break;
        default:
            est |= ET_FAIL;
            break;
    }

    return est;

} /* et_cru_disablePllLossInterrupt */


/*==================== Function Separator =============================*/
et_status_t et_cru_clearPllLockInterrupt(et_handle_t handle, uint32_t pllId)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;
    et_status_t est = ET_OK;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    switch(pllId)
    {
        case 0:
            set_bitField( et_cru_handle->cm_esr->cm_pll0_status, CLOCK_MANAGER_CM_PLL0_STATUS_LOCK, 0b0);
            break;
        case 1:
            set_bitField( et_cru_handle->cm_esr->cm_pll1_status, CLOCK_MANAGER_CM_PLL1_STATUS_LOCK, 0b0);
            break;
        case 2:
            set_bitField( et_cru_handle->cm_esr->cm_pll2_status, CLOCK_MANAGER_CM_PLL2_STATUS_LOCK, 0b0);
            break;
        case 4:
            set_bitField( et_cru_handle->cm_esr->cm_pll4_status, CLOCK_MANAGER_CM_PLL4_STATUS_LOCK, 0b0);
            break;
        default:
            est |= ET_FAIL;
            break;
    }

    return est;

} /* et_cru_clearPllLockInterrupt */


/*==================== Function Separator =============================*/
et_status_t et_cru_clearAllPllInterrupts(et_handle_t handle)
{
    uint32_t reg32;
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    reg32 = et_cru_handle->cm_esr->cm_pll0_status;
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL0_STATUS_LOSS, 0b0);
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL0_STATUS_LOCK, 0b0);
    et_cru_handle->cm_esr->cm_pll0_status = reg32;

    reg32 = et_cru_handle->cm_esr->cm_pll1_status;
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL1_STATUS_LOSS, 0b0);
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL1_STATUS_LOCK, 0b0);
    et_cru_handle->cm_esr->cm_pll1_status = reg32;

    reg32 = et_cru_handle->cm_esr->cm_pll2_status;
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL2_STATUS_LOSS, 0b0);
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL2_STATUS_LOCK, 0b0);
    et_cru_handle->cm_esr->cm_pll2_status = reg32;

    reg32 = et_cru_handle->cm_esr->cm_pll4_status;
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL4_STATUS_LOSS, 0b0);
    set_bitField( reg32, CLOCK_MANAGER_CM_PLL4_STATUS_LOCK, 0b0);
    et_cru_handle->cm_esr->cm_pll4_status = reg32;

    return ET_OK;

} /* et_cru_clearAllPllInterrupts */


/*==================== Function Separator =============================*/
et_status_t et_cru_clearAllPllLossInterrupts(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    set_bitField( et_cru_handle->cm_esr->cm_pll0_status, CLOCK_MANAGER_CM_PLL0_STATUS_LOSS, 0b0);
    set_bitField( et_cru_handle->cm_esr->cm_pll1_status, CLOCK_MANAGER_CM_PLL1_STATUS_LOSS, 0b0);
    set_bitField( et_cru_handle->cm_esr->cm_pll2_status, CLOCK_MANAGER_CM_PLL2_STATUS_LOSS, 0b0);
    set_bitField( et_cru_handle->cm_esr->cm_pll4_status, CLOCK_MANAGER_CM_PLL4_STATUS_LOSS, 0b0);

    return ET_OK;

} /* et_cru_clearAllPllLossInterrupts */


/*==================== Function Separator =============================*/
et_status_t et_cru_clearAllPllLockInterrupts(et_handle_t handle)
{

    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    set_bitField( et_cru_handle->cm_esr->cm_pll0_status, CLOCK_MANAGER_CM_PLL0_STATUS_LOCK, 0b0);
    set_bitField( et_cru_handle->cm_esr->cm_pll1_status, CLOCK_MANAGER_CM_PLL1_STATUS_LOCK, 0b0);
    set_bitField( et_cru_handle->cm_esr->cm_pll2_status, CLOCK_MANAGER_CM_PLL2_STATUS_LOCK, 0b0);
    set_bitField( et_cru_handle->cm_esr->cm_pll4_status, CLOCK_MANAGER_CM_PLL4_STATUS_LOCK, 0b0);

    return ET_OK;

} /* et_cru_clearAllPllLockInterrupts */


/*==================== Function Separator =============================*/
et_status_t et_cru_clearPllLossInterrupt(et_handle_t handle, uint32_t pllId)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;
    et_status_t est = ET_OK;

   if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    switch(pllId)
    {
        case 0:
            set_bitField( et_cru_handle->cm_esr->cm_pll0_status, CLOCK_MANAGER_CM_PLL0_STATUS_LOSS, 0b0);
            break;
        case 1:
            set_bitField( et_cru_handle->cm_esr->cm_pll1_status, CLOCK_MANAGER_CM_PLL1_STATUS_LOSS, 0b0);
            break;
        case 2:
            set_bitField( et_cru_handle->cm_esr->cm_pll2_status, CLOCK_MANAGER_CM_PLL2_STATUS_LOSS, 0b0);
            break;
        case 4:
            set_bitField( et_cru_handle->cm_esr->cm_pll4_status, CLOCK_MANAGER_CM_PLL4_STATUS_LOSS, 0b0);
            break;
        default:
            est = ET_FAIL;
            break;
    }

    return est;
} /* et_cru_clearPllLossInterrupt */


/*==================== Function Separator =============================*/
uint32_t et_cru_getLossInterruptStatus(et_handle_t handle, uint32_t pllId)
{
    uint32_t status;
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    switch (pllId) {
        case 0:
            status = get_bitField(et_cru_handle->cm_esr->cm_pll0_status, CLOCK_MANAGER_CM_PLL0_STATUS_LOSS);
            break;
        case 1:
            status = get_bitField(et_cru_handle->cm_esr->cm_pll1_status, CLOCK_MANAGER_CM_PLL1_STATUS_LOSS);
            break;
        case 2:
            status = get_bitField(et_cru_handle->cm_esr->cm_pll2_status, CLOCK_MANAGER_CM_PLL2_STATUS_LOSS);
            break;
        case 4:
            status = get_bitField(et_cru_handle->cm_esr->cm_pll4_status, CLOCK_MANAGER_CM_PLL4_STATUS_LOSS);
            break;
    }
    return status;

} /* et_cru_getLossInterruptStatus */


/*==================== Function Separator =============================*/
uint32_t et_cru_getLockInterruptStatus(et_handle_t handle, uint32_t pllId)
{
    uint32_t status;
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    switch (pllId) {
        case 0:
            status =  get_bitField(et_cru_handle->cm_esr->cm_pll0_status, CLOCK_MANAGER_CM_PLL0_STATUS_LOCK);
            break;
        case 1:
            status =  get_bitField(et_cru_handle->cm_esr->cm_pll1_status, CLOCK_MANAGER_CM_PLL1_STATUS_LOCK);
            break;
        case 2:
            status =  get_bitField(et_cru_handle->cm_esr->cm_pll2_status, CLOCK_MANAGER_CM_PLL2_STATUS_LOCK);
            break;
        case 4:
            status =  get_bitField(et_cru_handle->cm_esr->cm_pll4_status, CLOCK_MANAGER_CM_PLL4_STATUS_LOCK);
            break;
    }
    return status;

} /* et_cru_getLockInterruptStatus */


/*==================== Function Separator =============================*/
et_status_t et_cru_enablePLL0Mission(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    set_bitField( et_cru_handle->cm_esr->cm_ios_ctrl, CLOCK_MANAGER_CM_IOS_CTRL_MISSION, 0b1);

    return ET_OK;
}


/*==================== Function Separator =============================*/
et_status_t et_cru_disablePLL0Mission(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    set_bitField( et_cru_handle->cm_esr->cm_ios_ctrl, CLOCK_MANAGER_CM_IOS_CTRL_MISSION, 0b0);

    return ET_OK;
}


/*==================== Function Separator =============================*/
/* RTLMIN-5448
extern et_status_t et_cru_clk_noc_dvfs(et_handle_t handle, uint64_t id)
{
    uint32_t reg32;
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;
    et_status_t est = ET_OK;

    reg32 = et_cru_handle->cm_esr->cm_dvfs_ctrl;
    switch(id)
    {
        case 0:
            set_bitField( reg32, CLOCK_MANAGER_CM_DVFS_CTRL_SELECT, 0b0);
            break;
        case 1:
            set_bitField( reg32, CLOCK_MANAGER_CM_DVFS_CTRL_SELECT, 0b1);
            break;
        default:
            est = ET_FAIL;
            break;
    }
    et_cru_handle->cm_esr->cm_dvfs_ctrl = reg32;

    return est;

} */ /* et_cru_clk_noc_dvfs */


/*==================== Function Separator =============================*/
uint64_t et_cru_getMode( uint64_t inputFreq, uint64_t outputFreq ) 
{

    for ( uint64_t i = 0; i < sizeof(gs_et_cru_modes)/sizeof(ET_CRU_MODES_t); i++) {

        if( ( inputFreq  > (gs_et_cru_modes[i].inputFreq  - gs_et_cru_modes[i].inputFreq/100)   && inputFreq  < (gs_et_cru_modes[i].inputFreq  + gs_et_cru_modes[i].inputFreq/100) ) && \
            ( outputFreq > (gs_et_cru_modes[i].outputFreq - gs_et_cru_modes[i].outputFreq/100)  && outputFreq < (gs_et_cru_modes[i].outputFreq + gs_et_cru_modes[i].outputFreq/100) ) ) {

            return gs_et_cru_modes[i].mode;
            break;
        }
    }

    return 0; /* No match in table found */

} /* et_cru_getMode() */


/*==================== Function Separator =============================*/
extern et_status_t et_cru_setClockFreq( et_handle_t handle, uint64_t id, uint64_t target_freq ) 
{

    PLL_API_t *pll_handle;
    ET_CRU_MODES_t targetConfiguration;
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;
    et_status_t est = ET_OK;

    SOC_CLOCK_e socClock;
    socClock = (SOC_CLOCK_e) id;

    /* Assigning values to targetConfiguration and pll_handle of the PLL that needs to be programmed */
    targetConfiguration.inputFreq  = pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_CLK_REF_HPDPLL)/8];
    targetConfiguration.mode = 0;
    switch (socClock) {

        case SOC_SPIO_PLL0_CLK :
            pll_handle = et_cru_handle->spio_pll0_handle;
            targetConfiguration.outputFreq = target_freq;
            break;
        case SOC_SPIO_PLL1_CLK :
            pll_handle = et_cru_handle->spio_pll1_handle;
            targetConfiguration.outputFreq = target_freq;
            break;
        case SOC_SPIO_PLL2_CLK :
            pll_handle = et_cru_handle->spio_pll2_handle;
            targetConfiguration.outputFreq = target_freq;
            break;
        case SOC_SPIO_PLL4_CLK :
            pll_handle = et_cru_handle->spio_pll4_handle;
            targetConfiguration.outputFreq = target_freq;
            break;
        case SOC_CLK_200MHZ :
            pll_handle = et_cru_handle->spio_pll1_handle;
            targetConfiguration.outputFreq = target_freq*10;
            break;
        case SOC_CLK_500MHZ :
            pll_handle = et_cru_handle->spio_pll1_handle;
            targetConfiguration.outputFreq = target_freq*4;
            break;
        default:
            // printDbg("et_cru_setClockFreq: No enum found");
            return ET_FAIL;
    }

    /* Checking for available mode that will satisfy given parameters */
    targetConfiguration.mode = et_cru_getMode(targetConfiguration.inputFreq, targetConfiguration.outputFreq);

    if ( targetConfiguration.mode == 0) {
        // printDbg("et_cru_setClockFreq: No mode found");
        return ET_FAIL; /* No mathing mode available */
    }
    else
        est |= pllMovellus_setMode( pll_handle, targetConfiguration.mode );

    return est;

}


/*==================== Function Separator =============================*/
extern uint64_t et_cru_getClockFreq( uint64_t id ) 
{
    SOC_CLOCK_e socClock;
    socClock = (SOC_CLOCK_e) id;

    switch (socClock) {

        /* External */
/*
        case SOC_CLK_100_IN:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_CLK_100_IN)/8];
        case SOC_OSC_24_IN:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_OSC_24_IN)/8];
        case SOC_CLK_EXT_IN:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_CLK_EXT_IN)/8];
*/
        /* SPIO PLL */
        case SOC_SPIO_PLL0_CLK:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_SPIO_PLL0_CLK)/8];
        case SOC_SPIO_PLL1_CLK:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_SPIO_PLL1_CLK)/8];
        case SOC_SPIO_PLL2_CLK:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_SPIO_PLL2_CLK)/8];
        case SOC_SPIO_PLL4_CLK:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_SPIO_PLL4_CLK)/8];

        /* Fixed */
        case SOC_CLK_3MHZ:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_CLK_3MHZ)/8];
        case SOC_CLK_10MHZ:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_CLK_10MHZ)/8];
        case SOC_CLK_25MHZ:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_CLK_25MHZ)/8];
        case SOC_CLK_200MHZ:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_CLK_200MHZ)/8];
        case SOC_CLK_500MHZ:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_CLK_500MHZ)/8];

        /* Other */
        case SOC_CLK_REF_LVDPLL:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_CLK_REF_LVDPLL)/8];
        case SOC_CLK_REF_HPDPLL:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_CLK_REF_HPDPLL)/8];
        case SOC_CLK_STEP_MIN:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_CLK_STEP_MIN)/8];
        case SOC_CLK___NOC_ORIGIN:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_CLK___NOC_ORIGIN)/8];
        case SOC_CLK_IOS_ORIGIN:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_CLK_IOS_ORIGIN)/8];
        case SOC_CLK_IOS_APB_ORIGIN:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_CLK_IOS_APB_ORIGIN)/8];
        case SOC_CLK_IOS_AHB_ORIGIN:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_CLK_IOS_AHB_ORIGIN)/8];
        case SOC_CLK_IOS_EFUSE_SP_JTAG:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_CLK_IOS_EFUSE_SP_JTAG)/8];
        case SOC_CLK_MAIN_WRCK:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_CLK_MAIN_WRCK)/8];
        case SOC_CLK_VAULT_WRCK:
            return pSoC->cruClkFreq[offsetof(SOC_CLOCK_t, SOC_CLK_VAULT_WRCK)/8];

        default:
            return 0;
    }

} /* et_cru_getClockFreq() */


/* =============================================================================
 * PLL RESET RELATED API FUNCTIONS
 * =============================================================================
 */


// FUTURE: Find better way for maxion core and memshire warm resets!
extern et_status_t et_cru_reset( et_handle_t handle, uint64_t soc_reset, uint64_t action)
{
    uint32_t reg32;
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;
    et_status_t est = ET_OK;

    SOC_RESET_e soc_reset_e;
    soc_reset_e = (SOC_RESET_e) soc_reset;
    ET_CRU_RESET_ACTION_e reset_action_e;
    reset_action_e = (ET_CRU_RESET_ACTION_e) action;

    switch(soc_reset_e)
    {

        /* Maxion, 10 resets */
        case RESET_COLD_MAX:
            set_bitField( et_cru_handle->rm_esr->rm_max_cold, RESET_MANAGER_RM_MAX_COLD_RSTN, reset_action_e );
            break;
        case RESET_WARM_MAX_CORE_ALL:
            if(reset_action_e == RELEASE)
                set_bitField( et_cru_handle->rm_esr->rm_max_warm, RESET_MANAGER_RM_MAX_WARM_CORE_RSTN, 0b1111 );
            else
                set_bitField( et_cru_handle->rm_esr->rm_max_warm, RESET_MANAGER_RM_MAX_WARM_CORE_RSTN, 0b0000 );
            break;
        case RESET_WARM_MAX_CORE_0:
            reg32 = et_cru_handle->rm_esr->rm_max_warm;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MAX_WARM_CORE_RSTN, 0b0001 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MAX_WARM_CORE_RSTN, 0b0001 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_max_warm = reg32;
            break;
        case RESET_WARM_MAX_CORE_1:
            reg32 = et_cru_handle->rm_esr->rm_max_warm;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MAX_WARM_CORE_RSTN, 0b0010 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MAX_WARM_CORE_RSTN, 0b0010 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_max_warm = reg32;
            break;
        case RESET_WARM_MAX_CORE_2:
            reg32 = et_cru_handle->rm_esr->rm_max_warm;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MAX_WARM_CORE_RSTN, 0b0100 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MAX_WARM_CORE_RSTN, 0b0100 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_max_warm = reg32;
            break;
        case RESET_WARM_MAX_CORE_3:
            reg32 = et_cru_handle->rm_esr->rm_max_warm;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MAX_WARM_CORE_RSTN, 0b1000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MAX_WARM_CORE_RSTN, 0b1000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_max_warm = reg32;
            break;
        case RESET_WARM_MAX_DEBUG:
            set_bitField( et_cru_handle->rm_esr->rm_max_warm, RESET_MANAGER_RM_MAX_WARM_DEBUG_RSTN, reset_action_e );
            break;
        case RESET_WARM_MAX_UNCORE:
            set_bitField( et_cru_handle->rm_esr->rm_max_warm, RESET_MANAGER_RM_MAX_WARM_UNCORE_RSTN, reset_action_e );
            break;
        case RESET_N_MAX_PLL_CORE:
            set_bitField( et_cru_handle->rm_esr->rm_max, RESET_MANAGER_RM_MAX_PLL_CORE_RSTN, reset_action_e );
                break;
        case RESET_N_MAX_PLL_UNCORE:
            set_bitField( et_cru_handle->rm_esr->rm_max, RESET_MANAGER_RM_MAX_PLL_UNCORE_RSTN, reset_action_e );
            break;

        /* MemShire, 10 resets */
        case RESET_COLD_MEMSHIRE_EAST: // We have common COLD reset for both memshires
        case RESET_COLD_MEMSHIRE_WEST: // FUTURE This is confusing!
            set_bitField( et_cru_handle->rm_esr->rm_memshire_cold, RESET_MANAGER_RM_MEMSHIRE_COLD_RSTN, reset_action_e );
            break;
        case RESET_WARM_MEMSHIRE_EAST_0:
            reg32 = et_cru_handle->rm_esr->rm_memshire_warm;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MEMSHIRE_WARM_RSTN, 0x01 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MEMSHIRE_WARM_RSTN, 0x01);
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_memshire_warm = reg32;
            break;
        case RESET_WARM_MEMSHIRE_EAST_1:
            reg32 = et_cru_handle->rm_esr->rm_memshire_warm;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MEMSHIRE_WARM_RSTN, 0x02 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MEMSHIRE_WARM_RSTN, 0x02);
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_memshire_warm = reg32;
            break;
        case RESET_WARM_MEMSHIRE_EAST_2:
            reg32 = et_cru_handle->rm_esr->rm_memshire_warm;

            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MEMSHIRE_WARM_RSTN, 0x04 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MEMSHIRE_WARM_RSTN, 0x04);
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_memshire_warm = reg32;
            break;
        case RESET_WARM_MEMSHIRE_EAST_3:
            reg32 = et_cru_handle->rm_esr->rm_memshire_warm;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MEMSHIRE_WARM_RSTN, 0x08 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MEMSHIRE_WARM_RSTN, 0x08);
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_memshire_warm = reg32;
            break;
        case RESET_WARM_MEMSHIRE_WEST_0:
            reg32 = et_cru_handle->rm_esr->rm_memshire_warm;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MEMSHIRE_WARM_RSTN, 0x10 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MEMSHIRE_WARM_RSTN, 0x10);
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_memshire_warm = reg32;
            break;
        case RESET_WARM_MEMSHIRE_WEST_1:
            reg32 = et_cru_handle->rm_esr->rm_memshire_warm;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MEMSHIRE_WARM_RSTN, 0x20 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MEMSHIRE_WARM_RSTN, 0x20);
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_memshire_warm = reg32;
            break;
        case RESET_WARM_MEMSHIRE_WEST_2:
            reg32 = et_cru_handle->rm_esr->rm_memshire_warm;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MEMSHIRE_WARM_RSTN, 0x40 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MEMSHIRE_WARM_RSTN, 0x40);
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_memshire_warm = reg32;
            break;
        case RESET_WARM_MEMSHIRE_WEST_3:
            reg32 = et_cru_handle->rm_esr->rm_memshire_warm;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MEMSHIRE_WARM_RSTN, 0x80 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MEMSHIRE_WARM_RSTN, 0x80);
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_memshire_warm = reg32;
            break;

/* Minion, 34 resets */
/*        case RESET_BOOT_ALL_MINION:
            set_bitField( et_cru_handle->rm_esr->rm_minion, , reset_action_e );
            break;
*/        case RESET_COLD_MINION:
            set_bitField( et_cru_handle->rm_esr->rm_minion, RESET_MANAGER_RM_MINION_COLD_RSTN, reset_action_e );
            break;
        case RESET_WARM_MINION:
            set_bitField( et_cru_handle->rm_esr->rm_minion, RESET_MANAGER_RM_MINION_WARM_RSTN, reset_action_e );
            break;
        case RESET_WARM_MINION_0:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000001);
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000001 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_1:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000002 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000002 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_2:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000004 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000004 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_3:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000008 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000008 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_4:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000010 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000010 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_5:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000020 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000020 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_6:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000040 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000040 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_7:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000080 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000080 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_8:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000100 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000100 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_9:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000200 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000200 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_10:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000400 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000400 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_11:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000800 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00000800 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_12:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00001000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00001000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_13:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00002000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00002000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_14:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00004000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00004000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_15:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00008000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00008000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_16:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00010000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00010000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_17:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00020000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00020000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_18:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00040000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00040000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_19:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00080000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00080000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_20:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00100000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00100000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_21:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00200000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00200000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_22:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00400000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00400000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_23:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00800000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x00800000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_24:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x01000000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x01000000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_25:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x02000000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x02000000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_26:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x04000000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x04000000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_27:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x08000000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x08000000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_28:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x10000000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x10000000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_29:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x20000000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x20000000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_30:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x40000000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x40000000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_31:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_a;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x80000000 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_A_RSTN, 0x80000000 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_a = reg32;
            break;
        case RESET_WARM_MINION_32:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_b;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_B_RSTN, 0b01 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_B_RSTN, 0b01 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_b = reg32;
            break;
        case RESET_WARM_MINION_33:
            reg32 = et_cru_handle->rm_esr->rm_minion_warm_b;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_B_RSTN, 0b10 );
            else {
                set_bitField( reg32, RESET_MANAGER_RM_MINION_WARM_B_RSTN, 0b10 );
                reg32 &= ~reg32;
            }
            et_cru_handle->rm_esr->rm_minion_warm_b = reg32;
            break;

        /* pShire, 3 resets */
        case RESET_COLD_PSHIRE:
            set_bitField( et_cru_handle->rm_esr->rm_pshire_cold, RESET_MANAGER_RM_PSHIRE_COLD_RSTN, reset_action_e );
            break;
        case RESET_WARM_PSHIRE:
            set_bitField( et_cru_handle->rm_esr->rm_pshire_warm, RESET_MANAGER_RM_PSHIRE_WARM_RSTN, reset_action_e );
            break;
        case RESET_DEBUG_PSHIRE:
            set_bitField( et_cru_handle->rm_esr->rm_pshire_warm, RESET_MANAGER_RM_PSHIRE_WARM_RSTN_DEBUG, reset_action_e );
            break;

        /* Fixed clocks, 6 resets */
        /* These use boot_rstn_clkinit, maybe remove them? */
        case RESET_N_COLD_CLK:
                // FUTURE
            break;
        case RESET_N_COLD_CLK_10MHZ:
                // FUTURE
            break;
        case RESET_N_COLD_CLK_24MHZ:
                // FUTURE
            break;
        case RESET_N_COLD_CLK_25MHZ:
                // FUTURE
            break;
        case RESET_N_COLD_CLK_200MHZ:
                // FUTURE
            break;
        case RESET_N_COLD_CLK_500MHZ:
                // FUTURE
            break;


   //volatile uint32_t rm_pu_gpio; /**< Offset 0x500 (R/W) */
  // volatile uint32_t rm_pu_wdt; /**< Offset 0x504 (R/W) */
  // volatile uint32_t rm_pu_timers; /**< Offset 0x508 (R/W) */
   //volatile uint32_t rm_pu_uart0; /**< Offset 0x50c (R/W) */
   //volatile uint32_t rm_pu_uart1; /**< Offset 0x510 (R/W) */
  // volatile uint32_t rm_pu_i2c; /**< Offset 0x514 (R/W) */
  // volatile uint32_t rm_pu_i3c; /**< Offset 0x518 (R/W) */
  // volatile uint32_t rm_pu_spi; /**< Offset 0x51c (R/W) */


        /* IOS, 9 resets */
        /* These are part of spio_boot module */
        //.boot_rstn_ios_slave(boot_rstn_ios_slave),
        //.boot_rstn_ios_fabric(boot_rstn_ios_fabric),
        //.boot_rstn_main_noc (boot_rstn_main_noc),
        case RESET_N_COLD_CLK_IOS:
                // FUTURE
            break;
        case RESET_N_COLD_CLK_IOS_AHB:
                // FUTURE
            break;
        case RESET_N_COLD_CLK_IOS_APB:
                // FUTURE
            break;
        case RESET_N_COLD_CLK_IOS_AXI_BRONZE:
                // FUTURE
            break;
        case RESET_N_IOS_I2C:
                // FUTURE
            break;
        case RESET_N_IOS_I3C:
                // FUTURE
            break;
        case RESET_N_IOS_NOC:
                // FUTURE
            break;
        case RESET_N_IOS_SPI:
                // FUTURE
            break;
        case RESET_N_IOS_UART:
                // FUTURE
            break;


        /* IOS USB, 15 resets */

        /* USB 0 */
        case RESET_N_IOS_USB2_0_ALL:
            reg32 = et_cru_handle->rm_esr->rm_usb2_0;
            set_bitField( reg32, RESET_MANAGER_RM_USB2_0_AHB2AXI_RSTN, reset_action_e );
            set_bitField( reg32, RESET_MANAGER_RM_USB2_0_CTRL_AHB_RSTN, reset_action_e );
            set_bitField( reg32, RESET_MANAGER_RM_USB2_0_CTRL_PHY_RSTN, reset_action_e );
            set_bitField( reg32, RESET_MANAGER_RM_USB2_0_PHY_PO_RSTN, reset_action_e );
            set_bitField( reg32, RESET_MANAGER_RM_USB2_0_PHY_PORT0_RSTN, reset_action_e );
            set_bitField( reg32, RESET_MANAGER_RM_USB2_0_RELOC_RSTN, reset_action_e );
            set_bitField( reg32, RESET_MANAGER_RM_USB2_0_ESR_RSTN, reset_action_e );
            et_cru_handle->rm_esr->rm_usb2_0 = reg32;
            break;
        case RESET_N_IOS_USB2_0_AHB2AXI_AHB_CLK:
            set_bitField( et_cru_handle->rm_esr->rm_usb2_0, RESET_MANAGER_RM_USB2_0_AHB2AXI_RSTN, reset_action_e );
            break;
        case RESET_N_IOS_USB2_0_CTRL_AHB_CLK:
            set_bitField( et_cru_handle->rm_esr->rm_usb2_0, RESET_MANAGER_RM_USB2_0_CTRL_AHB_RSTN, reset_action_e );
            break;
        case RESET_N_IOS_USB2_0_CTRL_PHY_CLK:
            set_bitField( et_cru_handle->rm_esr->rm_usb2_0, RESET_MANAGER_RM_USB2_0_CTRL_PHY_RSTN, reset_action_e );
            break;
        case RESET_N_IOS_USB2_0_ESR_APB_CLK:
            set_bitField( et_cru_handle->rm_esr->rm_usb2_0, RESET_MANAGER_RM_USB2_0_ESR_RSTN, reset_action_e );
            break;
        case RESET_N_IOS_USB2_0_PHY_POR:
            set_bitField( et_cru_handle->rm_esr->rm_usb2_0, RESET_MANAGER_RM_USB2_0_PHY_PO_RSTN, reset_action_e );
            break;
        case RESET_N_IOS_USB2_0_PHY_PORT0:
            set_bitField( et_cru_handle->rm_esr->rm_usb2_0, RESET_MANAGER_RM_USB2_0_PHY_PORT0_RSTN, reset_action_e );
            break;
        case RESET_N_IOS_USB2_0_RELOC_APB_CLK:
            set_bitField( et_cru_handle->rm_esr->rm_usb2_0, RESET_MANAGER_RM_USB2_0_RELOC_RSTN, reset_action_e );
            break;
        case RESET_N_IOS_USB2_0_DBG_UTMI_CLK:
            set_bitField( et_cru_handle->rm_esr->rm_usb2_0, RESET_MANAGER_RM_USB2_0_DBG_RSTN, reset_action_e );
            break;

        /* USB 1 */
        case RESET_N_IOS_USB2_1_ALL:
            reg32 = et_cru_handle->rm_esr->rm_usb2_1;
            set_bitField( reg32, RESET_MANAGER_RM_USB2_1_AHB2AXI_RSTN, reset_action_e );
            set_bitField( reg32, RESET_MANAGER_RM_USB2_1_CTRL_AHB_RSTN, reset_action_e );
            set_bitField( reg32, RESET_MANAGER_RM_USB2_1_CTRL_PHY_RSTN, reset_action_e );
            set_bitField( reg32, RESET_MANAGER_RM_USB2_1_PHY_PO_RSTN, reset_action_e );
            set_bitField( reg32, RESET_MANAGER_RM_USB2_1_PHY_PORT0_RSTN, reset_action_e );
            set_bitField( reg32, RESET_MANAGER_RM_USB2_1_RELOC_RSTN, reset_action_e );
            set_bitField( reg32, RESET_MANAGER_RM_USB2_1_ESR_RSTN, reset_action_e );
            et_cru_handle->rm_esr->rm_usb2_1 = reg32;
            break;
        case RESET_N_IOS_USB2_1_AHB2AXI_AHB_CLK:
            set_bitField( et_cru_handle->rm_esr->rm_usb2_1, RESET_MANAGER_RM_USB2_1_AHB2AXI_RSTN, reset_action_e );
            break;
        case RESET_N_IOS_USB2_1_CTRL_AHB_CLK:
            set_bitField( et_cru_handle->rm_esr->rm_usb2_1, RESET_MANAGER_RM_USB2_1_CTRL_AHB_RSTN, reset_action_e );
            break;
        case RESET_N_IOS_USB2_1_CTRL_PHY_CLK:
            set_bitField( et_cru_handle->rm_esr->rm_usb2_1, RESET_MANAGER_RM_USB2_1_CTRL_PHY_RSTN, reset_action_e );
            break;
        case RESET_N_IOS_USB2_1_ESR_APB_CLK:
            set_bitField( et_cru_handle->rm_esr->rm_usb2_1, RESET_MANAGER_RM_USB2_1_ESR_RSTN, reset_action_e );
            break;
        case RESET_N_IOS_USB2_1_PHY_POR:
            set_bitField( et_cru_handle->rm_esr->rm_usb2_1, RESET_MANAGER_RM_USB2_1_PHY_PO_RSTN, reset_action_e );
            break;
        case RESET_N_IOS_USB2_1_PHY_PORT0:
            set_bitField( et_cru_handle->rm_esr->rm_usb2_1, RESET_MANAGER_RM_USB2_1_PHY_PORT0_RSTN, reset_action_e );
            break;
        case RESET_N_IOS_USB2_1_RELOC_APB_CLK:
            set_bitField( et_cru_handle->rm_esr->rm_usb2_1, RESET_MANAGER_RM_USB2_1_RELOC_RSTN, reset_action_e );
            break;

        /* SPIO, 23 resets */
        case RESET_N_SPIO_DMA:
            set_bitField( et_cru_handle->rm_esr->rm_spio_dma, RESET_MANAGER_RM_SPIO_DMA_RSTN, reset_action_e );
            break;
        case RESET_N_SPIO_GPIO:
            set_bitField( et_cru_handle->rm_esr->rm_spio_gpio, RESET_MANAGER_RM_SPIO_GPIO_RSTN, reset_action_e );
            break;
        case RESET_N_SPIO_I2C0:
            set_bitField( et_cru_handle->rm_esr->rm_spio_i2c0, RESET_MANAGER_RM_SPIO_I2C0_RSTN, reset_action_e );
            break;
        case RESET_N_SPIO_I2C1:
            set_bitField( et_cru_handle->rm_esr->rm_spio_i2c1, RESET_MANAGER_RM_SPIO_I2C1_RSTN, reset_action_e );
            break;
        case RESET_N_SPIO_PLL_ALL:
            set_bitField( et_cru_handle->rm_esr->rm_ios_pll0, RESET_MANAGER_RM_IOS_PLL0_RSTN, reset_action_e );
            set_bitField( et_cru_handle->rm_esr->rm_ios_pll1, RESET_MANAGER_RM_IOS_PLL1_RSTN, reset_action_e );
            set_bitField( et_cru_handle->rm_esr->rm_ios_pll2, RESET_MANAGER_RM_IOS_PLL2_RSTN, reset_action_e );
//            set_bitField( et_cru_handle->rm_esr->rm_ios_pll3, RESET_MANAGER_RM_IOS_PLL3_RSTN, reset_action_e );
            set_bitField( et_cru_handle->rm_esr->rm_ios_pll4, RESET_MANAGER_RM_IOS_PLL4_RSTN, reset_action_e );
            break;
        case RESET_N_SPIO_PLL0:
            set_bitField( et_cru_handle->rm_esr->rm_ios_pll0, RESET_MANAGER_RM_IOS_PLL0_RSTN, reset_action_e );
            break;
        case RESET_N_SPIO_PLL1:
            set_bitField( et_cru_handle->rm_esr->rm_ios_pll1, RESET_MANAGER_RM_IOS_PLL1_RSTN, reset_action_e );
            break;
        case RESET_N_SPIO_PLL2:
            set_bitField( et_cru_handle->rm_esr->rm_ios_pll2, RESET_MANAGER_RM_IOS_PLL2_RSTN, reset_action_e );
            break;
        case RESET_N_SPIO_PLL3:
            set_bitField( et_cru_handle->rm_esr->rm_ios_pll3, RESET_MANAGER_RM_IOS_PLL3_RSTN, reset_action_e );
            break;
        case RESET_N_SPIO_PLL4:
            set_bitField( et_cru_handle->rm_esr->rm_ios_pll4, RESET_MANAGER_RM_IOS_PLL4_RSTN, reset_action_e );
            break;
//        case RESET_N_SPIO_SP:

//            break;
        case RESET_N_SPIO_SPI0:
            set_bitField( et_cru_handle->rm_esr->rm_spio_spi0, RESET_MANAGER_RM_SPIO_SPI0_RSTN, reset_action_e );
            break;
        case RESET_N_SPIO_SPI1:
            set_bitField( et_cru_handle->rm_esr->rm_spio_spi1, RESET_MANAGER_RM_SPIO_SPI1_RSTN, reset_action_e );
            break;
        case RESET_N_SPIO_TIMER0:
            reg32 = et_cru_handle->rm_esr->rm_spio_timers;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_SPIO_TIMERS_RSTN, 0xFF );
            else
                set_bitField( reg32, RESET_MANAGER_RM_SPIO_TIMERS_RSTN, 0xFE );
            et_cru_handle->rm_esr->rm_spio_timers = reg32;
            break;
        case RESET_N_SPIO_TIMER1:
            reg32 = et_cru_handle->rm_esr->rm_spio_timers;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_SPIO_TIMERS_RSTN, 0xFF );
            else
                set_bitField( reg32, RESET_MANAGER_RM_SPIO_TIMERS_RSTN, 0xFD );
            et_cru_handle->rm_esr->rm_spio_timers = reg32;
            break;
        case RESET_N_SPIO_TIMER2:
            reg32 = et_cru_handle->rm_esr->rm_spio_timers;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_SPIO_TIMERS_RSTN, 0xFF );
            else
                set_bitField( reg32, RESET_MANAGER_RM_SPIO_TIMERS_RSTN, 0xFB );
            et_cru_handle->rm_esr->rm_spio_timers = reg32;
            break;
        case RESET_N_SPIO_TIMER3:
            reg32 = et_cru_handle->rm_esr->rm_spio_timers;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_SPIO_TIMERS_RSTN, 0xFF );
            else
                set_bitField( reg32, RESET_MANAGER_RM_SPIO_TIMERS_RSTN, 0xF7 );
            et_cru_handle->rm_esr->rm_spio_timers = reg32;
            break;
        case RESET_N_SPIO_TIMER4:
            reg32 = et_cru_handle->rm_esr->rm_spio_timers;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_SPIO_TIMERS_RSTN, 0xFF );
            else
                set_bitField( reg32, RESET_MANAGER_RM_SPIO_TIMERS_RSTN, 0xEF );
            et_cru_handle->rm_esr->rm_spio_timers = reg32;
            break;
        case RESET_N_SPIO_TIMER5:
            reg32 = et_cru_handle->rm_esr->rm_spio_timers;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_SPIO_TIMERS_RSTN, 0xFF );
            else
                set_bitField( reg32, RESET_MANAGER_RM_SPIO_TIMERS_RSTN, 0xDF );
            et_cru_handle->rm_esr->rm_spio_timers = reg32;
            break;
        case RESET_N_SPIO_TIMER6:
            reg32 = et_cru_handle->rm_esr->rm_spio_timers;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_SPIO_TIMERS_RSTN, 0xFF );
            else
                set_bitField( reg32, RESET_MANAGER_RM_SPIO_TIMERS_RSTN, 0xBF );
            et_cru_handle->rm_esr->rm_spio_timers = reg32;
            break;
        case RESET_N_SPIO_TIMER7:
            reg32 = et_cru_handle->rm_esr->rm_spio_timers;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_SPIO_TIMERS_RSTN, 0xFF );
            else
                set_bitField( reg32, RESET_MANAGER_RM_SPIO_TIMERS_RSTN, 0x7F );
            et_cru_handle->rm_esr->rm_spio_timers = reg32;
            break;
        case RESET_N_SPIO_UART0:
            set_bitField( et_cru_handle->rm_esr->rm_spio_uart0, RESET_MANAGER_RM_SPIO_UART0_RSTN, reset_action_e );
            break;
        case RESET_N_SPIO_UART1:
            set_bitField( et_cru_handle->rm_esr->rm_spio_uart1, RESET_MANAGER_RM_SPIO_UART1_RSTN, reset_action_e );
            break;
        case RESET_N_SPIO_VAULT_POR:
                // FUTURE
            break;
        case RESET_N_SPIO_VAULT_SOFT:
                // set_bitField( et_cru_handle->rm_esr->rm_ios_vault, RESET_MANAGER_RM_IOS_VAULT_SOFT_RSTN, reset_action_e );
            break;
        case RESET_N_SPIO_WDT:
            set_bitField( et_cru_handle->rm_esr->rm_spio_wdt, RESET_MANAGER_RM_SPIO_WDT_RSTN, reset_action_e );
            break;

        /* PU, 15 resets */
        case RESET_N_PU_GPIO:
            set_bitField( et_cru_handle->rm_esr->rm_pu_gpio, RESET_MANAGER_RM_PU_GPIO_RSTN, reset_action_e );
            break;
        case RESET_N_PU_I2C:
            set_bitField( et_cru_handle->rm_esr->rm_pu_i2c, RESET_MANAGER_RM_PU_I2C_RSTN, reset_action_e );
            break;
        case RESET_N_PU_I3C:
            set_bitField( et_cru_handle->rm_esr->rm_pu_i3c, RESET_MANAGER_RM_PU_I3C_RSTN, reset_action_e );
            break;
        case RESET_N_PU_SPI:
            set_bitField( et_cru_handle->rm_esr->rm_pu_spi, RESET_MANAGER_RM_PU_SPI_RSTN, reset_action_e );
            break;
        case RESET_N_PU_TIMER0:
            reg32 = et_cru_handle->rm_esr->rm_pu_timers;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_PU_TIMERS_RSTN, 0xFF );
            else
                set_bitField( reg32, RESET_MANAGER_RM_PU_TIMERS_RSTN, 0xFE );
            et_cru_handle->rm_esr->rm_pu_timers = reg32;
            break;
        case RESET_N_PU_TIMER1:
            reg32 = et_cru_handle->rm_esr->rm_pu_timers;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_PU_TIMERS_RSTN, 0xFF );
            else
                set_bitField( reg32, RESET_MANAGER_RM_PU_TIMERS_RSTN, 0xFD );
            et_cru_handle->rm_esr->rm_pu_timers = reg32;
            break;
        case RESET_N_PU_TIMER2:
            reg32 = et_cru_handle->rm_esr->rm_pu_timers;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_PU_TIMERS_RSTN, 0xFF );
            else
                set_bitField( reg32, RESET_MANAGER_RM_PU_TIMERS_RSTN, 0xFB );
            et_cru_handle->rm_esr->rm_pu_timers = reg32;
            break;
        case RESET_N_PU_TIMER3:
            reg32 = et_cru_handle->rm_esr->rm_pu_timers;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_PU_TIMERS_RSTN, 0xFF );
            else
                set_bitField( reg32, RESET_MANAGER_RM_PU_TIMERS_RSTN, 0xF7 );
            et_cru_handle->rm_esr->rm_pu_timers = reg32;
            break;
        case RESET_N_PU_TIMER4:
            reg32 = et_cru_handle->rm_esr->rm_pu_timers;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_PU_TIMERS_RSTN, 0xFF );
            else
                set_bitField( reg32, RESET_MANAGER_RM_PU_TIMERS_RSTN, 0xEF );
            et_cru_handle->rm_esr->rm_pu_timers = reg32;
            break;
        case RESET_N_PU_TIMER5:
            reg32 = et_cru_handle->rm_esr->rm_pu_timers;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_PU_TIMERS_RSTN, 0xFF );
            else
                set_bitField( reg32, RESET_MANAGER_RM_PU_TIMERS_RSTN, 0xDF );
            et_cru_handle->rm_esr->rm_pu_timers = reg32;
            break;
        case RESET_N_PU_TIMER6:
            reg32 = et_cru_handle->rm_esr->rm_pu_timers;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_PU_TIMERS_RSTN, 0xFF );
            else
                set_bitField( reg32, RESET_MANAGER_RM_PU_TIMERS_RSTN, 0xBF );
            et_cru_handle->rm_esr->rm_pu_timers = reg32;
            break;
        case RESET_N_PU_TIMER7:
            reg32 = et_cru_handle->rm_esr->rm_pu_timers;
            if(reset_action_e == RELEASE)
                set_bitField( reg32, RESET_MANAGER_RM_PU_TIMERS_RSTN, 0xFF );
            else
                set_bitField( reg32, RESET_MANAGER_RM_PU_TIMERS_RSTN, 0x7F );
            et_cru_handle->rm_esr->rm_pu_timers = reg32;
            break;
        case RESET_N_PU_UART0:
            set_bitField( et_cru_handle->rm_esr->rm_pu_uart0, RESET_MANAGER_RM_PU_UART0_RSTN, reset_action_e );
            break;
        case RESET_N_PU_UART1:
            set_bitField( et_cru_handle->rm_esr->rm_pu_uart1, RESET_MANAGER_RM_PU_UART1_RSTN, reset_action_e );
            break;
        case RESET_N_PU_WDT:
            set_bitField( et_cru_handle->rm_esr->rm_pu_wdt, RESET_MANAGER_RM_PU_WDT_RSTN, reset_action_e );
            break;


// rm_ios_periph

/*
1. Start supplying hclk (AHB). This is automatically done by the boot FSM.
2. Reset hclk domain (AHB) by applying active-low hresetn. This is automatically done by the boot FSM.
3. Complete the programming sequence of Section 4.3 \u201cHost Controller Setup Sequence for eMMC Interface\u201d in eMMC User Guide.
4. Apply active-low reset bresetn, aresetn, tresetn to reset internal clock domains (bclk, aclk, tmclk). This can be achieved by writing the corresponding fields in rm_emmc ESR.
5. Apply active-low reset cresetn_tx to reset card clock domain.

*/

        /* eMMC, 4 resets */
        case RESET_N_EMMC_ACLK:
            set_bitField( et_cru_handle->rm_esr->rm_emmc, RESET_MANAGER_RM_EMMC_ACLK_RSTN, reset_action_e );
            break;
        case RESET_N_EMMC_BCLK:
            set_bitField( et_cru_handle->rm_esr->rm_emmc, RESET_MANAGER_RM_EMMC_BCLK_RSTN, reset_action_e );
            break;
        case RESET_N_EMMC_CCLK:
            set_bitField( et_cru_handle->rm_esr->rm_emmc, RESET_MANAGER_RM_EMMC_CCLK_RSTN, reset_action_e );
            break;
        case RESET_N_EMMC_TCLK:
            set_bitField( et_cru_handle->rm_esr->rm_emmc, RESET_MANAGER_RM_EMMC_TCLK_RSTN, reset_action_e );
            break;
        case RESET_N_EMMC_SEQUENCE:
            reg32 = et_cru_handle->rm_esr->rm_emmc;
            set_bitField( reg32, RESET_MANAGER_RM_EMMC_ACLK_RSTN, reset_action_e );
            set_bitField( reg32, RESET_MANAGER_RM_EMMC_BCLK_RSTN, reset_action_e );
            set_bitField( reg32, RESET_MANAGER_RM_EMMC_TCLK_RSTN, reset_action_e );
            et_cru_handle->rm_esr->rm_emmc = reg32;
            set_bitField( reg32, RESET_MANAGER_RM_EMMC_CCLK_RSTN, reset_action_e );
            et_cru_handle->rm_esr->rm_emmc = reg32;
            break;

        /* IOS Peripherals, resets */
        case RESET_N_UST:
                // FUTURE
            break;
/*        case RESET_N_IOS_SP:
              set_bitField( et_cru_handle->rm_esr->rm_ios_sp, RESET_MANAGER_RM_IOS_SP_RSTN, reset_action_e );
            break;
        case RESET_N_IOS_VAULT:
              set_bitField( et_cru_handle->rm_esr->rm_ios_vault, RESET_MANAGER_RM_IOS_VAULT_SOFT_RSTN, reset_action_e );
            break;
*/        case RESET_N_SYSTEM:
//            set_bitField( et_cru_handle->rm_esr->rm_sys_reset_ctrl, RESET_MANAGER_RM_SYS_RESET_CTRL_ENABLE, reset_action_e );
              set_bitField( et_cru_handle->rm_esr->rm_main_noc, RESET_MANAGER_RM_MAIN_NOC_RSTN, reset_action_e );
            break;
        case RESET_N_SYSTEM_DEBUG:
              set_bitField( et_cru_handle->rm_esr->rm_debug_noc, RESET_MANAGER_RM_DEBUG_NOC_RSTN, reset_action_e );
            break;
/*        case RESET_N_IOS_PERIPH:
              set_bitField( et_cru_handle->rm_esr->rm_ios_periph, RESET_MANAGER_RM_IOS_PERIPH_RSTN, reset_action_e );
            break;
*/        case RESET_N_BOOT_CLK_IOS_APB:
        /* FUTURE
            reg [0:0] rm_status_bringup;
            reg [0:0] rm_status_maxion_enabled;
            reg [0:0] rm_status_vault_abort_ack;
            reg [5:0] rm_status_boot;
        */
            break;

    /* Cru system reset */
        case RESET_CRU_SYSTEM:
              set_bitField( et_cru_handle->rm_esr->rm_sys_reset_config, RESET_MANAGER_RM_SYS_RESET_CONFIG_WIDTH, 0b0000000 );
              set_bitField( et_cru_handle->rm_esr->rm_sys_reset_ctrl, RESET_MANAGER_RM_SYS_RESET_CTRL_ENABLE, reset_action_e );
            break;

// LINE

        default:
            break;
    }
    return est;

} /* et_cru_reset() */




/*==================== Function Separator =============================*/
et_status_t et_cru_releaseCruSysReset( et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    set_bitField( et_cru_handle->rm_esr->rm_sys_reset_ctrl, RESET_MANAGER_RM_SYS_RESET_CTRL_ENABLE, 0b1 );

    return ET_OK;
}


/*==================== Function Separator =============================*/
et_status_t et_cru_maxionPll( et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    set_bitField( et_cru_handle->cm_esr->cm_max, CLOCK_MANAGER_CM_MAX_PLL_SEL, 0b1 );

    return ET_OK;
}


/*==================== Function Separator =============================*/
et_status_t et_cru_clearLossPending(et_handle_t handle, uint32_t pllId)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    switch (pllId) {
        case 0:
            set_bitField( et_cru_handle->cm_esr->cm_pll0_status, CLOCK_MANAGER_CM_PLL0_STATUS_LOSS, 0b0 );
            break;
        case 1:
            set_bitField( et_cru_handle->cm_esr->cm_pll1_status, CLOCK_MANAGER_CM_PLL1_STATUS_LOSS, 0b0 );
            break;
        case 2:
            set_bitField( et_cru_handle->cm_esr->cm_pll2_status, CLOCK_MANAGER_CM_PLL2_STATUS_LOSS, 0b0 );
            break;
        case 4:
            set_bitField( et_cru_handle->cm_esr->cm_pll4_status, CLOCK_MANAGER_CM_PLL4_STATUS_LOSS, 0b0 );
            break;
    }

    return ET_OK;
} /* et_cru_clearLossPending */


/*==================== Function Separator =============================*/
et_status_t et_cru_clearLockPending(et_handle_t handle, uint32_t pllId)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    if( et_cru_handle->cm_esr == NULL )
        return ET_FAIL;

    switch (pllId) {
        case 0:
            set_bitField( et_cru_handle->cm_esr->cm_pll0_status, CLOCK_MANAGER_CM_PLL0_STATUS_LOCK, 0b0 );
            break;
        case 1:
            set_bitField( et_cru_handle->cm_esr->cm_pll1_status, CLOCK_MANAGER_CM_PLL1_STATUS_LOCK, 0b0 );
            break;
        case 2:
            set_bitField( et_cru_handle->cm_esr->cm_pll2_status, CLOCK_MANAGER_CM_PLL2_STATUS_LOCK, 0b0 );
            break;
        case 4:
            set_bitField( et_cru_handle->cm_esr->cm_pll4_status, CLOCK_MANAGER_CM_PLL4_STATUS_LOCK, 0b0 );
            break;
    }

    return ET_OK;
} /* et_cru_clearLockPending */


 /* =============================================================================
 * RM GET STATUS API FUNCTIONS
 * =============================================================================
 */

/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_rm1(et_handle_t handle)
{
    uint32_t reg32;
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    reg32 = et_cru_handle->rm_esr->rm_status;
    //return get_bitField(reg32, RESET_MANAGER_RM_STATUS_BOOT_FINISH);
    return reg32;

} /* et_cru_rm_get_status_rm1 */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_rm2(et_handle_t handle)
{
    uint32_t reg32;
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    reg32 = et_cru_handle->rm_esr->rm_status2;
    //return get_bitField(reg32, RESET_MANAGER_RM_STATUS_BOOT_FINISH);
    return reg32;

} /* et_cru_rm_get_status_rm2 */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_boot_finish(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status, RESET_MANAGER_RM_STATUS_BOOT_FINISH);

} /* et_cru_rm_get_status_boot_finish */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_boot_error(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status, RESET_MANAGER_RM_STATUS_BOOT_ERROR);

} /* et_cru_rm_get_status_boot_error */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_boot_sp_hold_reset(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status, RESET_MANAGER_RM_STATUS_BOOT_SP_HOLD_RESET);

} /* et_cru_rm_get_status_boot_sp_hold_reset */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_vault_abort_ack(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status, RESET_MANAGER_RM_STATUS_VAULT_ABORT_ACK);

} /* et_cru_rm_get_status_vault_abort_ack */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_lockbox_bringup(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status, RESET_MANAGER_RM_STATUS_LOCKBOX_BRINGUP);

} /* et_cru_rm_get_status_lockbox_bringup */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_ots_lockbox_jtag_rot_enable(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status, RESET_MANAGER_RM_STATUS_LOCKBOX_JTAG_ROT_ENABLE);

} /* et_cru_rm_get_status_ots_lockbox_jtag_rot_enable */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_ots_lockbox_jtag_dft_enable(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status, RESET_MANAGER_RM_STATUS_LOCKBOX_JTAG_DFT_ENABLE);

} /* et_cru_rm_get_status_ots_lockbox_jtag_dft_enable */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_ots_lockbox_debug_enable(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status, RESET_MANAGER_RM_STATUS_LOCKBOX_DEBUG_ENABLE);

} /* et_cru_rm_get_status_ots_lockbox_debug_enable */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_ots_lockbox_maxion_nonsecure_access_enable(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status, RESET_MANAGER_RM_STATUS_LOCKBOX_MAXION_NONSECURE_ACCESS_ENABLE);

} /* et_cru_rm_get_status_ots_lockbox_maxion_nonsecure_access_enable */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_ots_lockbox_minion_nonsecure_access_enable(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status, RESET_MANAGER_RM_STATUS_LOCKBOX_MINION_NONSECURE_ACCESS_ENABLE);

} /* et_cru_rm_get_status_ots_lockbox_minion_nonsecure_access_enable */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_debug_enable(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status2, RESET_MANAGER_RM_STATUS2_DEBUG_ENABLE);

} /* et_cru_rm_get_status_debug_enable */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_emergency_dbg(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status2, RESET_MANAGER_RM_STATUS2_EMERGENCY_DBG);

} /* et_cru_rm_get_status_emergency_dbg */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_error_sms_bihr(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status2, RESET_MANAGER_RM_STATUS2_ERROR_SMS_BIHR);

} /* et_cru_rm_get_status_error_sms_bihr */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_error_sms_bist(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status2, RESET_MANAGER_RM_STATUS2_ERROR_SMS_BIST);

} /* et_cru_rm_get_status_error_sms_bist */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_error_sms_udr(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status2, RESET_MANAGER_RM_STATUS2_ERROR_SMS_UDR);

} /* et_cru_rm_get_status_error_sms_udr */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_error_vaultsms_bihr(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status2, RESET_MANAGER_RM_STATUS2_ERROR_VAULTSMS_BIHR);

} /* et_cru_rm_get_status_error_vaultsms_bihr */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_error_vaultsms_bist(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status2, RESET_MANAGER_RM_STATUS2_ERROR_VAULTSMS_BIST);

} /* et_cru_rm_get_status_error_vaultsms_bist */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_error_vaultsms_udr(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status2, RESET_MANAGER_RM_STATUS2_ERROR_VAULTSMS_UDR);

} /* et_cru_rm_get_status_error_vaultsms_udr */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_skip_sms_bihr(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status2, RESET_MANAGER_RM_STATUS2_SKIP_SMS_BIHR);

} /* et_cru_rm_get_status_skip_sms_bihr */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_skip_sms_bist(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status2, RESET_MANAGER_RM_STATUS2_SKIP_BIST);

} /* et_cru_rm_get_status_skip_sms_bist */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_skip_sms_udr(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status2, RESET_MANAGER_RM_STATUS2_SKIP_SMS_UDR);

} /* et_cru_rm_get_status_skip_sms_udr */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_skip_vault(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status2, RESET_MANAGER_RM_STATUS2_SKIP_VAULT);

} /* et_cru_rm_get_status_skip_vault */


/*==================== Function Separator =============================*/
extern uint32_t et_cru_rm_get_status_strap_in(et_handle_t handle)
{
    ET_CRU_API_t * et_cru_handle;
    et_cru_handle = (ET_CRU_API_t *)handle;

    return get_bitField(et_cru_handle->rm_esr->rm_status2, RESET_MANAGER_RM_STATUS2_STRAP_IN);

} /* et_cru_rm_get_status_strap_in */


 /* =============================================================================
 * END OF RM GET STATUS API FUNCTIONS
 * =============================================================================
 */



   /*     <EOF>     */

