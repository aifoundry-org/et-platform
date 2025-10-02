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
 *  @Filename       pvtcMoortec_api.c
 *
 *  @Description    PVTC API
 *
 *//*======================================================================== */
 
 
/* =============================================================================
 * STANDARD INCLUDE FILES
 * =============================================================================
 */

// Include files
#include "cpu.h"
#include "et.h"
#include "api.h"
#include "pvtcMoortec_api.h"
#include "pvtcMoortec_regs.h"
#include "pvtc.h"

/* =============================================================================
 * GLOBAL VARIABLES DECLARATIONS
 * =============================================================================
 */

PVTC_API_t  pvtcMoortecApi[5];

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
 


/* =============================================================================
 * FUNCTIONS
 * =============================================================================
 */

/*==================== Function Separator =============================*/
void pvtcMoortec_init( void ) {

    for(uint32_t i = 0; i < 5; i++ ) {  // FUTURE: get number of instances from soc.h There is no such constant in soc.h
        pvtcMoortecApi[i].reg = NULL;
    }

} /* pvtcMoortec_init() */

/*==================== Function Separator =============================*/
et_handle_t pvtcMoortec_open( uint32_t Id, API_IP_PARAMS_t *pIpParams )
{
    /* Initialize Data Structures */
    pvtcMoortecApi[Id].reg = (volatile struct pvtcMoortec_regs*)((uint64_t)pIpParams->baseAddress);
    
    return( (et_handle_t)&pvtcMoortecApi[Id] );
 
} /* pvtcMoortec_open() */


/*==================== Function Separator =============================*/
et_status_t pvtcMoortec_close( et_handle_t handle )
{
    et_status_t ets = ET_OK;
    PVTC_API_t *p_pvtcMoortecapi;

	/* Convert handle to PVTC_API_t */
    p_pvtcMoortecapi = (PVTC_API_t *)handle;

	/* Stop IP */
	// p_pvtcMoortecapi->stop() ???
	
    /* Clear inUse Flag */
    p_pvtcMoortecapi->reg = NULL;

    return ets;

} /* pvtcMoortec_close() */


/*==================== Function Separator =============================*/
et_status_t pvtcMoortec_configure( et_handle_t handle )
{

//Function used to configure registers inside IP from data provided in cfg structure

//Returns status.
//Copy data from api->cfg to IP registers.

    et_status_t ets = ET_OK;
    PVTC_API_t *p_pvtcMoortecApi;

    p_pvtcMoortecApi = (PVTC_API_t *)handle;
    
    p_pvtcMoortecApi->cfg.ts_num = NUMBER_OF_TS;
    p_pvtcMoortecApi->cfg.pd_num = NUMBER_OF_PD;
    p_pvtcMoortecApi->cfg.vm_num = NUMBER_OF_VM;
    p_pvtcMoortecApi->cfg.ch_num = NUMBER_OF_CH;
    
    return ets;

} /* pvtcMoortec_configure() */


/*==================== Function Separator =============================*/
//uint32_t pvtcMoortec_write( et_handle_t handle, void *txBuff, uint32_t txCount, uint32_t blocking )
//{

 
//} /* pvtcMoortec_write() */


/*==================== Function Separator =============================*/
//uint32_t pvtcMoortec_read( et_handle_t handle, void *rxBuff, uint32_t rxCount, uint32_t blocking )
//{

//} /* pvtcMoortec_read() */

/*==================== Function Separator =============================*/
//et_status_t pvtcMoortec_alive( et_handle_t handle )
//{
//    et_status_t ets = ET_OK;
//    PVTC_API_t *p_pvtcMoortecApi;
//    uint32_t reg32;
// 
//    p_pvtcMoortecApi = (PVTC_API_t *)handle;
// 
//    reg32 = p_pvtcMoortecApi -> reg -> pvt_comp_id;


//} /* pvtcMoortec_alive() */


   /*     <EOF>     */

