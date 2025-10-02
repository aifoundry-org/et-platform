/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies or
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

/**
 * @file $Id$
 * @version 2.0
 * @date 10/23/2019
 * @author HDL Design House
 *
 *  @Component      PLL
 *
 *  @Filename       pllMovellus_api.c
 *
 *  @Description    PLL API
 *
 *//*======================================================================== */


/* =============================================================================
 * STANDARD INCLUDE FILES
 * =============================================================================
 */

// Include files
#include "cpu.h"
#include "et.h"
#include "pllMovellus_api.h"
#include "pllMovellus_regs.h"
#include "movellus_hpdpll_modes_config.h"
#include "hal_device.h"

/* =============================================================================
 * GLOBAL VARIABLES DECLARATIONS
 * =============================================================================
 */

PLL_API_t  pllMovellusApi[NUMBER_OF_PLL_INSTANCES];

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

/*==================== Function Separator =============================*/
void pllMovellus_updateRegisters( et_handle_t handle )
{
    PLL_API_t * pll_handle;
    pll_handle = (PLL_API_t *)handle;

    /* Writing 1 to PLL register update strobe */
    setBitField( pll_handle -> reg -> REG38, REG38__REG_UPDATE_STROBE, 0x1 );

} /* pllMovellus_updateRegisters() */

/*==================== Function Separator =============================*/
et_status_t pllMovellus_waitForLock( uint64_t pllSelect, uint8_t no_printDbg )
{
    PLL_API_t * pll_handle;
    uint32_t timeout = MAX_LOCK_ATTEMPTS;

    /* Waiting for all PLLs to be locked */
    if( pllSelect == WAIT_FOR_LOCK_OF_ALL_PLLS ){
        for( int i = 0; i < NUMBER_OF_PLL_INSTANCES; i++ ){
            pll_handle = (PLL_API_t *)&pllMovellusApi[i];
            if( pll_handle -> reg == NULL || pll_handle -> cfg.pllMode == NO_MODE )
                continue;
            while( pllMovellus_get_lock_detect_status( pll_handle ) != 1 ){
                 timeout -= 0x1;
                 if ( timeout == 0 ){
                    if(no_printDbg == 0) { printDbg("PLL%d is not locked", (((uint64_t)&(pll_handle -> reg-> REG00 ) >> 12) & 0x0Ful) - 0x03); }
                    return ET_FAIL;
                  }
            }
            pll_handle -> pllLock = 0x1;
            timeout = MAX_LOCK_ATTEMPTS;
        }
    }
    /* Waiting for one selected PLL to be locked */
    else if( pllSelect >= NUMBER_OF_PLL_INSTANCES ){
        if(no_printDbg == 0) { printDbg( "Given number of PLL instance %d does not exist\n", pllSelect ); }
        return ET_FAIL;
    }
    else{
         pll_handle = (PLL_API_t *)&pllMovellusApi[pllSelect];
         if( pll_handle -> reg == NULL ){
              if(no_printDbg == 0) { printDbg( "PLL instance No %d is not configured \n", pllSelect ); }
             return ET_FAIL;
         }
         else if( pll_handle -> cfg.pllMode == NO_MODE ){
             if(no_printDbg == 0) { printDbg( "PLL%d no need to be locked\n", pllSelect ); }
             return ET_PASS;
         }
         else{
            while( pllMovellus_get_lock_detect_status(pll_handle) != 1 ){
              timeout -= 0x1;
              if ( timeout == 0 ){
                 if(no_printDbg == 0) { printDbg("PLL%d is not locked", pllSelect ); }
                 return ET_FAIL;
              }
            }
            pll_handle -> pllLock = 0x1;
         }
    }

    return ET_PASS;
}

/*==================== Function Separator =============================*/
void pllMovellus_configureDefault( et_handle_t handle )
{
    uint32_t reg32;

    PLL_API_t * pll_handle;
    pll_handle = (PLL_API_t *)handle;

    reg32 = pll_handle -> reg -> REG00;
    setBitField( reg32, REG00__OPEN_LOOP_BYPASS_ENABLE, 0x0);
    setBitField( reg32, REG00__DSM_DITHER_ENABLE , 0x1);
    setBitField( reg32, REG00__DSM_ENABLE, 0x1);
    setBitField( reg32, REG00__FREQ_ACQ_ENABLE, 0x1);
    setBitField( reg32, REG00__DCO_NORMALIZATION_ENABLE, 0x1);
    setBitField( reg32, REG00__DLF_ENABLE, 0x1);
    setBitField( reg32, REG00__CLKREF_SWITCH_ENABLE, 0x1);
    setBitField( reg32, REG00__LOCK_TIMEOUT_ENABLE, 0x1);
    setBitField( reg32, REG00__CLKREF_SELECT, 0x0);
    pll_handle -> reg -> REG00 = reg32;

    setBitField( pll_handle -> reg -> REG04, REG04__DLF_LOCKED_KP, 0x0003);
    setBitField( pll_handle -> reg -> REG05, REG05__DLF_LOCKED_KI, 0x002E);
    setBitField( pll_handle -> reg -> REG06, REG06__DLF_LOCKED_KB, 0x027E);
    setBitField( pll_handle -> reg -> REG07, REG07__DLF_TRACK_KP, 0x0003);
    setBitField( pll_handle -> reg -> REG08, REG08__DLF_TRACK_KI, 0x0032);
    setBitField( pll_handle -> reg -> REG09, REG09__DLF_TRACK_KB, 0x02BE);
    setBitField( pll_handle -> reg -> REG0A, REG0A__FCW_LOL, 0x1);
    setBitField( pll_handle -> reg -> REG0A, REG0A__LOCK_COUNT, 0x0A);
    setBitField( pll_handle -> reg -> REG0B, REG0B__LOCK_THRESHOLD, 0x00FF);
    setBitField( pll_handle -> reg -> REG0C, REG0C__DCO_GAIN_TO_REF_FREQ, 0x0033);
    setBitField( pll_handle -> reg -> REG0D, REG0D__DSM_DITHER_WIDTH, 0x1);
    setBitField( pll_handle -> reg -> REG0D, REG0D__DSM_DITHER_POSITION, 0xF);
    setBitField( pll_handle -> reg -> REG0E, REG0E__POSTDIV, 0x1);

    reg32 = pll_handle -> reg -> REG12;
    setBitField( reg32, REG12__POSTDIV0_BYPASS_CLKSEL, 0x0);
    setBitField( reg32, REG12__POSTDIV0_BYPASS_ENABLE, 0x1);
    setBitField( reg32, REG12__POSTDIV0_POWERDOWN, 0x0);
    pll_handle -> reg -> REG12 = reg32;

    setBitField( pll_handle -> reg -> REG14, REG14__OPEN_LOOP_CODE, 0x0);

} /* pllMovellus_configureDefault() */

/* =============================================================================
 * FUNCTIONS
 * =============================================================================
 */
/*==================== Function Separator =============================*/
void pllMovellus_init( void )
{
    for( int i = 0; i < NUMBER_OF_PLL_INSTANCES; i++ ) {
        pllMovellusApi[i].reg = NULL;
    }

} /* pllMovellus_init() */


/*==================== Function Separator =============================*/
et_handle_t pllMovellus_open( uint32_t Id, API_IP_PARAMS_t *pIpParams )
{
    /* Initialize Data Structures */
    pllMovellusApi[Id].reg = (volatile struct pllMovellus_regs*)((uint64_t)pIpParams->baseAddress);

    return( (et_handle_t)&pllMovellusApi[Id] );

} /* pllMovellus_open() */


/*==================== Function Separator =============================*/
et_status_t pllMovellus_close( et_handle_t handle )
{
    et_status_t ets = ET_OK;
    PLL_API_t *p_pllMovellusapi;

        /* Convert handle to PLL_API_t */
    p_pllMovellusapi = (PLL_API_t *)handle;

    /* Clear inUse Flag */
    p_pllMovellusapi->reg = NULL;

    return ets;

} /* pllMovellus_close() */


/*==================== Function Separator =============================*/
et_status_t pllMovellus_setMode( et_handle_t handle, PLL_MODE_e mode)
{
    uint32_t volatile *writeAddress;
    et_status_t ets = ET_OK;

    PLL_API_t * pll_handle;
    pll_handle = (PLL_API_t *)handle;

    pll_handle -> pllLock = 0;

    /* Start programming PLL to mode N */
    if( !mode ){
        return ET_FAIL;
    }
    else{
        for ( int i = 0; i < gs_hpdpll_settings[(uint32_t)(mode-1)].count; i++) {
            writeAddress = (uint32_t volatile *)(&(pll_handle->reg->REG00) + gs_hpdpll_settings[(mode-1)].offsets[i]);
            writeAddress[0] = gs_hpdpll_settings[(mode-1)].values[i];
        }
    }

    /* Update PLL registers */
    pllMovellus_updateRegisters(pll_handle);

    /* Waiting for PLL lock */
    if( pll_handle -> waitPllLock == 0x1 ){
                                        /* Caculating pllSelect from pll_handle addr */
        ets = pllMovellus_waitForLock( (((uint64_t)&(pll_handle -> reg-> REG00 ) >> 12) & 0x0Ful) - 0x03ul, 0 );
    }

    return ets;

} /* pllMovellus_setMode() */

/*==================== Function Separator =============================*/

/*==================== Function Separator =============================*/
/*uint64_t pllMovellus_getClockFreq( et_handle_t handle, uint32_t pllId  )
{

}*/ /* pllMovellus_getClockFreq() */


/*==================== Function Separator =============================*/
uint32_t pllMovellus_get_lock_detect_status( et_handle_t handle )
{
    PLL_API_t * pll_handle;

    pll_handle = (PLL_API_t *)handle;

    return getBitField( pll_handle -> reg -> REG39, REG39__LOCK_DETECT_STATUS );

} /* pllMovellus_get_lock_detect_status() */


/*==================== Function Separator =============================*/
void pllMovellus_disable( et_handle_t handle )
{
    PLL_API_t * pll_handle;
    pll_handle = (PLL_API_t *)handle;

    /* Disable enable bit */
    setBitField( pll_handle -> reg -> REG00 , REG00__PLL_ENABLE, 0);

    /* Clear all PLL registers */
    pll_handle -> reg -> REG00 = 0x0000;
    pll_handle -> reg -> REG01 = 0x0000;
    pll_handle -> reg -> REG02 = 0x0000;
    pll_handle -> reg -> REG03 = 0x0000;
    pll_handle -> reg -> REG04 = 0x0000;
    pll_handle -> reg -> REG05 = 0x0000;
    pll_handle -> reg -> REG06 = 0x0000;
    pll_handle -> reg -> REG07 = 0x0000;
    pll_handle -> reg -> REG08 = 0x0000;
    pll_handle -> reg -> REG09 = 0x0000;
    pll_handle -> reg -> REG0A = 0x0000;
    pll_handle -> reg -> REG0B = 0x0000;
    pll_handle -> reg -> REG0C = 0x0000;
    pll_handle -> reg -> REG0D = 0x0000;
    pll_handle -> reg -> REG0E = 0x0000;
    pll_handle -> reg -> REG12 = 0x0000;
    pll_handle -> reg -> REG14 = 0x0000;

    /* Update PLL registers */
    pllMovellus_updateRegisters(pll_handle);
}


/*==================== Function Separator =============================*/
void pllMovellus_enable( et_handle_t handle )
{
    PLL_API_t * pll_handle;
    pll_handle = (PLL_API_t *)handle;

    /* Enable enable bit */
    setBitField( pll_handle -> reg -> REG00 , REG00__PLL_ENABLE, 1);

    /* Update PLL registers */
    pllMovellus_updateRegisters(pll_handle);

}

 /*     <EOF>     */

