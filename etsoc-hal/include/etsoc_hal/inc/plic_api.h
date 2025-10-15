/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

/**
 *  @Component      PLIC
 *
 *  @Filename       plic_api.h
 *
 *  @Description    The PLIC component contains API interface to PLIC
 *
 *//*======================================================================== */

#ifndef __PLIC_API_H
#define __PLIC_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "api.h"

/* =============================================================================
 * EXPORTED DEFINITIONS
 * =============================================================================
 */ 
 
 /* =============================================================================
 * EXPORTED TYPES
 * =============================================================================
 */
 

/*-------------------------------------------------------------------------*//**
 * @TYPE         PLIC_CFG_t
 *
 * @BRIEF        PLIC configuration structure
 *
 * @DESCRIPTION  struct which defines PLIC configuration parameters
 *
 *//*------------------------------------------------------------------------ */ 
typedef struct plic_cfg 
{
    uint64_t irqNo;
   
} PLIC_CFG_t;

/*-------------------------------------------------------------------------*//**
 * @TYPE         PLIC_API_t
 *
 * @BRIEF        PLIC API structure
 *
 * @DESCRIPTION  struct which defines PLIC API interface
 *
 *//*------------------------------------------------------------------------ */ 
typedef struct plic_api
{
    volatile struct plic_regs*        reg;  // Pointer to instance register set
    volatile struct priority_regs*    pri;
    volatile struct pending_regs*     pending;
    volatile struct enable_regs*      enable;
    volatile struct hart_regs*        hart;
    PLIC_CFG_t                        cfg; // IP Configuration structure

} PLIC_API_t;

/*-------------------------------------------------------------------------*//**
 * @TYPE         EnableAddr
 *
 * @BRIEF        EnableAddr structure
 *
 * @DESCRIPTION  struct with enable target addresses
 *
 *//*------------------------------------------------------------------------ */ 

/* =============================================================================
 * EXPORTED VARIABLES
 * =============================================================================
 */ 



 /* =============================================================================
 * EXPORTED FUNCTIONS
 * =============================================================================
 */ 

 /* ------------------------------------------------------------------------*//**
 * @FUNCTION      plic_init
 *
 * @BRIEF         Initialize PLIC API
 *
 * @RETURNS       void
 *
 * @DESCRIPTION   Initialize PLIC API
 *
 *//*------------------------------------------------------------------------ */
extern void plic_init( void );



/* ------------------------------------------------------------------------*//**
 * @FUNCTION      plic_open
 *
 * @BRIEF         Open PLIC API
 *
 * @param[in]     uint32_t id                ->  id from soc.h
 * @param[in]     API_IP_PARAMS_t *pIpParams -> Pointer to API_IP_PARAMS_t 
 *                                              structure holding IP instantiation 
 *                                              parameters
 *
 * @RETURNS       et_handle_t
 *
 * @DESCRIPTION   Open PLIC API and initialize its structures
 *
 *//*------------------------------------------------------------------------ */
extern et_handle_t plic_open( uint32_t id, API_IP_PARAMS_t *pIpParams );


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      plic_close
 *
 * @BRIEF         Close PLIC API
 *
 * @param[in]     et_handle_t handle -> Handle to opened PLIC IP
 * 
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Close PLIC API and initialize its structures
 *
 *//*------------------------------------------------------------------------ */ 
extern et_status_t plic_close( et_handle_t handle );


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      plic_configure
 *
 * @BRIEF         Configure PLIC API
 *
 * @param[in]     et_handle_t handle -> Handle to opened PLIC IP
 * 
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Configure PLIC API and initialize its structures
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t plics_configure( et_handle_t handle );


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      plic_setPriority
 *
 * @BRIEF         Set priority for selected sources
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Set priority for selected sources
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t plic_setPriority( et_handle_t handle, uint64_t source, uint32_t priority_level);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      plic_enableInterrupt
 *
 * @BRIEF         enable interrupt for selected sources
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   enable interrupt for selected sources
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t plic_enableInterrupt( uint64_t source, uint32_t target, uint8_t id );


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      plic_disableInterrupt
 *
 * @BRIEF         disable interrupt for selected sources
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   disable interrupt for selected sources
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t plic_disableInterrupt( uint64_t source, uint32_t target, uint8_t id );


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      plic_setThreshold
 *
 * @BRIEF         set threshold for selected target
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   set threshold for selected target
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t plic_setThreshold( et_handle_t handle, uint32_t target, uint32_t threshold_level);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      plic_getIrqNo
 *
 * @BRIEF         get IRQ Number for selected target
 *
 * @RETURNS       uint64_t
 *
 * @DESCRIPTION   get IRQ Number for selected target
 *
 *//*------------------------------------------------------------------------ */
extern uint64_t plic_getIrqNo( et_handle_t handle, uint32_t target);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      plic_getPendingInterrupt
 *
 * @BRIEF         get pending Number for selected source
 *
 * @RETURNS       uint64_t
 *
 * @DESCRIPTION   get pending Number for selected source
 *
 *//*------------------------------------------------------------------------ */

extern uint64_t plic_getPendingInterrupt( et_handle_t handle, uint64_t source );

/* ------------------------------------------------------------------------*//**
 * @FUNCTION      plic_getPriority
 *
 * @BRIEF         get pending Number for selected source
 *
 * @RETURNS       uint64_t
 *
 * @DESCRIPTION   get pending Number for selected source
 *
 *//*------------------------------------------------------------------------ */
extern uint64_t plic_getPriority( et_handle_t handle, uint64_t source );


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      plic_setIrqNo
 *
 * @BRIEF         set maxId register after handling interrupt
 *
 * @RETURNS       void
 *
 * @DESCRIPTION   set maxId register after handling interrupt
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t plic_setIrqNo( et_handle_t handle, uint32_t source, uint32_t target);

#ifdef __cplusplus
}
#endif

#endif	/* __PLIC_API_H */


/*     <EOF>     */

