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
* @file  
* @version $Release$ 
* @date $Date$
* @author 
*
* @brief 
*
* 
*/ 
/** 
 *  @Component      interrupts
 *
 *  @Filename       local_interupts.h
 *
 *  @Description    The component defines local interupts
 *
 *//*======================================================================== */

#ifndef __LOCAL_INTERRUPTS_H
#define __LOCAL_INTERRUPTS_H

#ifdef __cplusplus
extern "C"
{
#endif

#define SP_L1_IRQ_OFFSET                    4096
#define SP_USER_SW_IRQ                      SP_L1_IRQ_OFFSET
#define SP_SUPERVISOR_SW_IRQ                (SP_L1_IRQ_OFFSET + 1)
#define SP_MACHINE_SW_IRQ                   (SP_L1_IRQ_OFFSET + 3)
#define SP_USER_TIMER_IRQ                   (SP_L1_IRQ_OFFSET + 4)
#define SP_SUPERVISOR_TIMER_IRQ             (SP_L1_IRQ_OFFSET + 5)
#define SP_MACHINE_TIMER_IRQ                (SP_L1_IRQ_OFFSET + 7)
#define SP_USER_EXTERNAL_IRQ                (SP_L1_IRQ_OFFSET + 8)
#define SP_SUPERVISOR_EXTERNAL_IRQ          (SP_L1_IRQ_OFFSET + 9)
#define SP_MACHINE_EXTERNAL_IRQ             (SP_L1_IRQ_OFFSET + 11)
#define SP_BAD_IPI_REDIRECT_IRQ             (SP_L1_IRQ_OFFSET + 16)
#define SP_INST_CACHE_ECC_CNT_OVERFOW_IRQ   (SP_L1_IRQ_OFFSET + 19)
#define SP_BUS_ERROR_IRQ                    (SP_L1_IRQ_OFFSET + 23)

#define L1_IRQ_NUMBER                       24

#ifdef __cplusplus
}
#endif

#endif /* __LOCAL_INTERRUPTS_H */


/*****     <EOF>     *****/
