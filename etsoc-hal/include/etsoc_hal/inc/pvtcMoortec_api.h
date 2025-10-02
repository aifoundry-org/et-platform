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
 *  @Component      PVTC
 *
 *  @Filename       pvtcMoortec_api.h
 *
 *  @Description    The PVTC component contains API interface to PVTC
 *
 *//*======================================================================== */

#ifndef __PVTCMOORTEC_API_H
#define __PVTCMOORTEC_API_H

#ifdef __cplusplus
extern
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
 * @TYPE         PVTC_CONFIG_t
 *
 * @BRIEF        PVTC register structure
 *
 * @DESCRIPTION  struct which defines PVTC registers
 *
 *//*------------------------------------------------------------------------ */
typedef struct pvtcMoortec_config
{
    // list of all registers here:

    // Note that member names must not match registers or register bit field names,
    // But should be easily readable

} PVTC_CONFIG_t;

/*-------------------------------------------------------------------------*//**
 * @TYPE         PVTC_CFG_t
 *
 * @BRIEF        PVTC configuration structure
 *
 * @DESCRIPTION  struct which defines PVTC configuration parameters
 *
 *//*------------------------------------------------------------------------ */
typedef struct pvtcMoortec_cfg
{
    uint8_t         ts_num;
    uint8_t         pd_num;
    uint8_t         vm_num;
    uint8_t         ch_num;

} PVTC_CFG_t;

/*-------------------------------------------------------------------------*//**
 * @TYPE         PVTC_API_t
 *
 * @BRIEF        PVTC API structure
 *
 * @DESCRIPTION  struct which defines PVTC API interface
 *
 *//*------------------------------------------------------------------------ */
typedef struct pvtcMoortec_api
{
    volatile struct pvtcMoortec_regs*   reg;   // Pointer to instance register set
    PVTC_CFG_t                       cfg;

} PVTC_API_t; /* pvtcMoortec_api */

/* =============================================================================
 * EXPORTED VARIABLES
 * =============================================================================
 */



 /* =============================================================================
 * EXPORTED FUNCTIONS
 * =============================================================================
 */

 /* ------------------------------------------------------------------------*//**
 * @FUNCTION      pvtcMoortec_init
 *
 * @BRIEF         Initialize PVTC API
 *
 * @RETURNS       void
 *
 * @DESCRIPTION   Initialize PVTC API
 *
 *//*------------------------------------------------------------------------ */
extern void pvtcMoortec_init( void );



/* ------------------------------------------------------------------------*//**
 * @FUNCTION      pvtcMoortec_open
 *
 * @BRIEF         Open PVTC API
 *
 * @param[in]     uint32_t pvtcId             -> pll Id from soc.h
 * @param[in]     API_IP_PARAMS_t *pIpParams  -> Pointer to API_IP_PARAMS_t
 *                                               structure holding IP instantiation
 *                                               parameters
 *
 * @RETURNS       et_handle_t
 *
 * @DESCRIPTION   Open PVTC API and initialize its structures
 *
 *//*------------------------------------------------------------------------ */
extern et_handle_t pvtcMoortec_open( uint32_t pvtcId, API_IP_PARAMS_t *pIpParams );


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      pvtcMoortec_close
 *
 * @BRIEF         Close PVTC API
 *
 * @param[in]     et_handle_t handle -> Handle to opened PVTC IP
 *
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Close PVTC API and initialize its structures
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t pvtcMoortec_close( et_handle_t handle );


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      pvtcMoortec_configure
 *
 * @BRIEF         Configure PVTC API
 *
 * @param[in]     et_handle_t handle -> Handle to opened PVTC IP
 *
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Configure PVTC API and initialize its structures
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t pvtcMoortec_configure( et_handle_t handle );


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      pvtcMovellus_write
 *
 * @BRIEF         Write PVTC API
 *
 * @param[in]     et_handle_t handle -> Handle to opened PVTC IP
 * @param[in]     void *txBuff          -> Transmit Buffer
 * @param[in]     uint32_t txCount      -> Number of elements to transfer
 * @param[in]     uint32_t blocking     -> Block return until transmit completes ( == true )
 *                                         or return immediately ( == false )
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Write PVTC API and initialize its structures
 *
 *//*------------------------------------------------------------------------ */
//extern uint32_t pvtcMoortec_write( et_handle_t handle, void *txBuff, uint32_t txCount, uint32_t blocking );


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      pvtcMoortec_read
 *
 * @BRIEF         Read PVTC API
 *
 * @param[in]     et_handle_t handle -> Handle to opened PVTC IP
 * @param[in]     void *rxBuff          -> Receive Buffer
 * @param[in]     uint32_t rxCount      -> Number of elements to receive
 * @param[in]     uint32_t blocking     -> Block return until receive completes ( == true )
 *                                         or return immediately ( == false )
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Read PVTC API and initialize its structures
 *
 *//*------------------------------------------------------------------------ */
//extern uint32_t pvtcMoortec_read( et_handle_t handle, void *rxBuff, uint32_t rxCount, uint32_t blocking );

//extern et_status_t pvtcMoortec_alive( et_handle_t handle );

#ifdef __cplusplus
}
#endif

#endif  /* __PVTCMOVELLUS_API_H */


/*     <EOF>     */

