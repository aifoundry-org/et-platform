/*------------------------------------------------------------------------- 
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*------------------------------------------------------------------------- 
*/

/**
* @file $Id$ 
* @version $Release$ 
* @date $Date$ 
* @author 
*
* @brief plic_api.c   
*/ 
/** 
 *  @Component    PLIC 
 * 
 *  @Filename     plic_api.c 
 * 
 *  @Description  PLIC Driver
 * 
 *//*======================================================================== */ 



/* =============================================================================
 * STANDARD INCLUDE FILES
 * =============================================================================
 */ 
#include "cpu.h"
#include "soc.h"
#include "et.h"
#include "api.h"
#include "plic_api.h"
#include "plic_regs.h"
#include "spio_plic.h"
#include "pu_plic.h" 

/* =============================================================================
 * GLOBAL VARIABLES DECLARATIONS
 * =============================================================================
 */
PLIC_API_t plicApi[numPLICs];

/* =============================================================================
 * LOCAL TYPES AND DEFINITIONS
 * =============================================================================
 */
/* Tebles with enable addresses */ 
uint64_t spioTable[spioTargetCnt] = { 0x50002000ul , 0x50002080ul };
uint64_t puTable[puTargetCnt] = { 0x10004080ul, 0x10004000ul,  0x10002080ul, 0x10002000ul, 0x10002180ul,  0x10002100ul,  0x10002280ul,  0x10002200ul,  0x10002380ul,  0x10002300ul,  0x10003080ul, 0x10003000ul };

/* Teble with PU hart addresses */ 
uint64_t puHart[puTargetCnt]  = { 0x10241000ul, 0x10240000ul,  0x10201000ul, 0x10200000ul, 0x10203000ul,  0x10202000ul,  0x10205000ul,  0x10204000ul,  0x10207000ul,  0x10206000ul,  0x10221000ul, 0x10220000ul }; 
/* =============================================================================
 * LOCAL VARIABLES DECLARATIONS
 * =============================================================================
 */

 /* ============================================================================
 * LOCAL FUNCTIONS PROTOTYPES
 * =============================================================================
 */

/* =============================================================================
 * FUNCTIONS
 * =============================================================================
 */

/*==================== Function Separator =============================*/
void plic_init( void)
{
    uint32_t id;

    for ( id = 0; id < numPLICs; id++ ) {
        plicApi[id].reg = NULL;
        plicApi[id].pending = NULL;
    }
    
} /*    plic_init()     */

/*==================== Function Separator =============================*/
et_handle_t plic_open( uint32_t id, API_IP_PARAMS_t *pIpParams )
{
    
    if(plicApi[id].reg == NULL) {
        plicApi[id].pri = (volatile struct priority_regs*)((uint64_t)(pIpParams -> baseAddress + PRIORITY_BASE));
        plicApi[id].pending = (volatile struct pending_regs*)((uint64_t)(pIpParams -> baseAddress + PENDING_BASE));
        plicApi[id].hart = (volatile struct hart_regs*)((uint64_t)(pIpParams -> baseAddress + HART_BASE));
    }
    else{
        return NULL;
    }

    return  (et_handle_t)&plicApi[id];

    
} /*    plic_open()     */

/*==================== Function Separator =============================*/
et_status_t plic_close( et_handle_t handle )
{
    PLIC_API_t *p_plic;
    et_status_t est = ET_OK;

    p_plic = (PLIC_API_t *)handle;
    p_plic -> reg = NULL;

    return est;
} /*    plic_close()    */

/*==================== Function Separator =============================*/
et_status_t plic_setPriority( et_handle_t handle, uint64_t source, uint32_t priority_level)
{
    PLIC_API_t *p_plic;
    et_status_t est = ET_OK;
    
    p_plic = (PLIC_API_t *)handle;
    
    if( INTH_NUMBER_OF_INT == 0 || ( source > INTH_NUMBER_OF_INT + 1 ) )
       est = ET_FAIL;       
    else
       p_plic -> pri -> priority_source[source] = priority_level;
          
    return est;
    
} /*    plic_setPriority()*/

/*==================== Function Separator =============================*/
et_status_t plic_enableInterrupt( uint64_t source, uint32_t target, uint8_t id )
{
    et_status_t est = ET_OK;
    uint32_t *pEnableReg;
    
    
    if( id )
       pEnableReg = (uint32_t *) spioTable[target];
    else 
       pEnableReg = (uint32_t *) puTable[target];

    if( target > INTH_TARGETS ) 
         est = ET_FAIL;           
    else 
       pEnableReg[source>>5] |= 0x1u << ( source & 0x1Fu );        
       
    return est;
    
} /*    plic_enableInterrupt()*/

/*==================== Function Separator =============================*/
et_status_t plic_disableInterrupt( uint64_t source, uint32_t target, uint8_t id )
{
    et_status_t est = ET_OK;
    uint32_t *pEnableReg;
    
    
    if( id )
       pEnableReg = (uint32_t *) spioTable[target];
    else 
       pEnableReg = (uint32_t *) puTable[target];      
    
    if( target > INTH_TARGETS ) 
         est = ET_FAIL;          
    else
        pEnableReg[source>>5] &= (~(0x1u << ( source & 0x1Fu ))); 

    return est;
    
} /*    plic_disableInterrupt()*/

/*==================== Function Separator =============================*/
uint64_t plic_getPendingInterrupt( et_handle_t handle, uint64_t source )
{
    uint64_t pendingValue;
    PLIC_API_t *p_plic;
    
    p_plic = (PLIC_API_t *)handle;
    
    if( source > SPIO_PLIC_INTR_SRC_CNT ) 
         pendingValue = 0;           
    else
        pendingValue = p_plic -> pending -> pending[source >> 5]; 
        
    return pendingValue;
    
} /*    plic_getPendingInterrupt()*/

uint64_t plic_getPriority( et_handle_t handle, uint64_t source )
{
    uint64_t priorityValue;
    PLIC_API_t *p_plic;
     
    p_plic = (PLIC_API_t *)handle;
    if( source > SPIO_PLIC_INTR_SRC_CNT ) 
        return ET_FAIL; 
    else
        priorityValue = p_plic -> pri -> priority_source[source >> 5]; 
     
    return priorityValue;
    
} /*    plic_getPriority()*/
/*==================== Function Separator =============================*/
et_status_t plic_setThreshold( et_handle_t handle, uint32_t target, uint32_t threshold_level)
{
    uint32_t *pThrReg;
    PLIC_API_t *p_plic;
    et_status_t est = ET_OK;
    
    p_plic = (PLIC_API_t *)handle;
    
    if( target > INTH_TARGETS ) 
        est = ET_FAIL;
    /* Set SPIO PLIC threshold, >>28 because the highest nibble of SPIO PLIC addr is 5 */                
    else if( ((uint64_t)(&(p_plic -> hart -> reg[target].maxid_target)) & 0x00000000FFFFFFFF) >> 28 == 0x5ul )     
        p_plic -> hart -> reg[target].threshold_target = threshold_level; 
    /* Set PU PLIC threshold, using addresses from the table */
    else {
        pThrReg = (uint32_t *) puHart[target];
        pThrReg[0] = threshold_level;
    }
    
    return est;
} /*    plic_setThreshold()*/

/*==================== Function Separator =============================*/
uint64_t plic_getIrqNo( et_handle_t handle, uint32_t target )
{
    uint64_t irqNo;
    uint32_t *pMaxIdReg;
    PLIC_API_t *p_plic;
    
    p_plic = (PLIC_API_t *)handle;

    if( target > INTH_TARGETS ) 
       return ET_FAIL;   
    /* Get SPIO PLIC irqNo, >>28 because the highest nibble of SPIO PLIC addr is 5 */ 
    else if( ((uint64_t)(&(p_plic -> hart -> reg[target].maxid_target)) & 0x00000000FFFFFFFF)  >> 28 == 0x5ul )
       irqNo = p_plic -> hart -> reg[target].maxid_target; 
    /* Get PU PLIC irqNo, using addresses from the table */          
    else{
       pMaxIdReg = (uint32_t *) puHart[target];
       irqNo = (uint64_t)(pMaxIdReg[1]);
    }   
    
    return irqNo;
    
} /*    plic_getIrqNo()*/

et_status_t plic_setIrqNo( et_handle_t handle, uint32_t source, uint32_t target)
{
    uint32_t *pMaxIdReg;
    PLIC_API_t *p_plic;
    et_status_t est = ET_OK;
    
    p_plic = (PLIC_API_t *)handle;
    
    if( target > INTH_TARGETS ) 
         est = ET_FAIL;  
    /* Set SPIO PLIC irqNo, >>28 because the highest nibble of SPIO PLIC addr is 5 */               
    else if ( ((uint64_t)(&(p_plic -> hart -> reg[target].maxid_target)) & 0x00000000FFFFFFFF) >> 28 == 0x5ul )
        p_plic -> hart -> reg[target].maxid_target = source; 
    /* Set PU PLIC irqNo, using addresses from the table */
    else {
        pMaxIdReg = (uint32_t *) puHart[target];
        pMaxIdReg[1] = source; 
    }
   
    return est;
    
} /*    plic_getIrqNo()*/

/*****     < EOF >     *****/

