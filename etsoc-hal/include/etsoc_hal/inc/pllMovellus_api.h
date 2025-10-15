/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
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
 *  @Filename       pllMovellus_api.h
 *
 *  @Description    The PLL component contains API interface to PLL
 *
 *//*======================================================================== */

#ifndef __PLLMOVELLUS_API_H
#define __PLLMOVELLUS_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "api.h"

/* Movellus specifications */
#define MIN_INPUT_CLOCK 24000000
#define MAX_INPUT_CLOCK 100000000

#define NUMBER_OF_PLL_INSTANCES  8
#define MAX_LOCK_ATTEMPTS 0x0200
#define WAIT_FOR_LOCK_OF_ALL_PLLS 0xFFFFFFFF
/* =============================================================================
 * EXPORTED DEFINITIONS
 * =============================================================================
 */ 
 
typedef enum pllEnable 
{
    PLL_ENABLE = 0x0,
    PLL_DISABLE    

}PLL_ENABLE_t;

typedef enum pllState 
{
    PLL_BYPASS_OFF = 0x0,
    PLL_BYPASS_ON   

}PLL_STATE_t;

/*-------------------------------------------------------------------------*//**
 * @TYPE         PLL_MODE_e
 *
 * @BRIEF        CPU Clock configuration
 *
 * @DESCRIPTION  List of available PLL modes
 *
 *//*------------------------------------------------------------------------ */
typedef enum pllMode 
{
    NO_MODE  = 0x0,
    MODE1,
    MODE2,
    MODE3,
    MODE4,
    MODE5,
    MODE6,
    MODE7,
    MODE8,
    MODE9,
    MODE10,
    MODE11,
    MODE12,
    MODE13,
    MODE14,
    MODE15,
    MODE16,
    MODE17,
    MODE18,
    MODE19,
    MODE20,
    MODE21,
    MODE22,
    MODE23,
    MODE24,
    MODE25,
    MODE26,
    MODE27,
    MODE28,
    MODE29,
    MODE30,
    MODE31,
    MODE32,
    MODE33,
    MODE34,
    MODE35,
    MODE36,
    MODE37,
    MODE38,
    MODE39,
    MODE40,
    MODE41,
    MODE42,
    MODE43,
    MODE44,
    MODE45,
    MODE46
} PLL_MODE_e;

 /* =============================================================================
 * EXPORTED TYPES
 * =============================================================================
 */
 


 /*-------------------------------------------------------------------------*//**
 * @TYPE         PLL_CONFIG_t
 *
 * @BRIEF        PLL configuration structure
 *
 * @DESCRIPTION  struct which defines PLL behavior 
 *
 *//*------------------------------------------------------------------------ */ 
typedef struct pllMovellus_config
{
    uint64_t         inputClk;
    uint64_t         outputClk;
    PLL_STATE_t      bypassPll;
    PLL_ENABLE_t     enablePll;
    PLL_MODE_e       pllMode;

} PLL_CONFIG_t;



/*-------------------------------------------------------------------------*//**
 * @TYPE         PLL_CFG_t
 *
 * @BRIEF        PLL configuration structure
 *
 * @DESCRIPTION  struct which defines PLL configuration parameters
 *
 *//*------------------------------------------------------------------------ */ 

/*-------------------------------------------------------------------------*//**
 * @TYPE         PLL_API_t
 *
 * @BRIEF        PLL API structure
 *
 * @DESCRIPTION  struct which defines PLL API interface
 *
 *//*------------------------------------------------------------------------ */ 
typedef struct pllMovellus_api
{
    volatile struct pllMovellus_regs    *reg; // Pointer to instance register set
    PLL_CONFIG_t                        cfg; // IP Configuration structure
    uint64_t      waitPllLock;
    uint64_t      pllLock;
    
} PLL_API_t;

/* =============================================================================
 * EXPORTED VARIABLES
 * =============================================================================
 */ 



 /* =============================================================================
 * EXPORTED FUNCTIONS
 * =============================================================================
 */ 

 /* ------------------------------------------------------------------------*//**
 * @FUNCTION      pllMovellus_updateRegisters
 *
 * @BRIEF         Initialize PLL API
 *
 * @RETURNS       void
 *
 * @DESCRIPTION   Update PLL registers (control registers are updated with the contents of [APB accessible] temporary registers)
 *
 *//*------------------------------------------------------------------------ */
extern void pllMovellus_updateRegisters( et_handle_t handle );


 /* ------------------------------------------------------------------------*//**
 * @FUNCTION      pllMovellus_init
 *
 * @BRIEF         Initialize PLL API
 *
 * @RETURNS       void
 *
 * @DESCRIPTION   Initialize PLL API
 *
 *//*------------------------------------------------------------------------ */
extern void pllMovellus_init( void );



/* ------------------------------------------------------------------------*//**
 * @FUNCTION      pllMovellus_open
 *
 * @BRIEF         Open PLL API
 *
 * @param[in]     uint32_t pllId             -> pll Id from soc.h
 * @param[in]     API_IP_PARAMS_t *pIpParams -> Pointer to API_IP_PARAMS_t 
 *                                              structure holding IP instantiation 
 *                                              parameters
 *
 * @RETURNS       et_handle_t
 *
 * @DESCRIPTION   Open PLL API and initialize its structures
 *
 *//*------------------------------------------------------------------------ */
extern et_handle_t pllMovellus_open( uint32_t pllId, API_IP_PARAMS_t *pIpParams );


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      pllMovellus_close
 *
 * @BRIEF         Close PLL API
 *
 * @param[in]     et_handle_t handle -> Handle to opened PLL IP
 * 
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Close PLL API and initialize its structures
 *
 *//*------------------------------------------------------------------------ */ 
extern et_status_t pllMovellus_close( et_handle_t handle );


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      pllMovellus_getClockFreq
 *
 * @BRIEF         Get PLL frequency API
 *
 * @param[in]     et_handle_t handle    -> Handle to opened PLL IP
 * @param[in]     uint32_t clkId -       > clk Id
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Get current freq output from PLL handle with pllId
 *
 *//*------------------------------------------------------------------------ */
//extern uint64_t pllMovellus_getClockFreq( et_handle_t handle, uint32_t pllId );



 /* ------------------------------------------------------------------------*//**
 * @FUNCTION      pllMovellus_get_lock_detect_status
 *
 * @BRIEF         Initialize PLL API
 *
 * @param[in]     et_handle_t handle    -> Handle to opened PLL IP
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Returns lock_detect_status field from PLL handle
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t pllMovellus_get_lock_detect_status( et_handle_t handle );


 /* ------------------------------------------------------------------------*//**
 * @FUNCTION      pllMovellus_disable
 *
 * @BRIEF         Initialize PLL API
 *
 * @param[in]     et_handle_t handle    -> Handle to opened PLL IP
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Disables PLLs
 *
 *//*------------------------------------------------------------------------ */
extern void pllMovellus_disable( et_handle_t handle );


 /* ------------------------------------------------------------------------*//**
 * @FUNCTION      pllMovellus_enable
 *
 * @BRIEF         Initialize PLL API
 *
 * @param[in]     et_handle_t handle    -> Handle to opened PLL IP
 *
 * @RETURNS       -
 *
 * @DESCRIPTION   Enable PLLs
 *
 *//*------------------------------------------------------------------------ */
extern void pllMovellus_enable( et_handle_t handle );


 /* ------------------------------------------------------------------------*//**
 * @FUNCTION      pllMovellus_setMode
 *
 * @BRIEF         Initialize PLL API
 *
 * @param[in]     et_handle_t handle    -> Handle to opened PLL IP
 * @param[in]     PLL_MODE_e mode       -> 
 *
 * @RETURNS       -
 *
 * @DESCRIPTION   Programs the PLL with a specific mode 
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t pllMovellus_setMode( et_handle_t handle, PLL_MODE_e mode);

 /* ------------------------------------------------------------------------*//**
 * @FUNCTION      pllMovellus_setMode_quick
 *
 * @BRIEF         Initialize PLL API
 *
 * @param[in]     et_handle_t handle    -> Handle to opened PLL IP
 * @param[in]     PLL_MODE_e mode       -> 
 *
 * @RETURNS       -
 *
 * @DESCRIPTION   Programs the PLL to new frequency (PLL has to be previously fully programmed)
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t pllMovellus_setMode_quick( et_handle_t handle, PLL_MODE_e mode);

 /* ------------------------------------------------------------------------*//**
 * @FUNCTION      pllMovellus_configureDefault
 *
 * @BRIEF         Initialize PLL API
 *
 * @param[in]     et_handle_t handle    -> Handle to opened PLL IP
 * 
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Programs the PLL with a specific mode 
 *
 *//*------------------------------------------------------------------------ */
extern void pllMovellus_configureDefault( et_handle_t handle );


 /* ------------------------------------------------------------------------*//**
 * @FUNCTION      pllMovellus_configureDefault
 *
 * @BRIEF         Initialize PLL API
 *
 * @param[in]     et_handle_t handle    -> Handle to opened PLL IP
 * 
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Programs the PLL with a specific mode 
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t pllMovellus_waitForLock( uint64_t pllSelect, uint8_t no_printDbg );


#ifdef __cplusplus
}
#endif

#endif	/* __PLLMOVELLUS_API_H */


/*     <EOF>     */

