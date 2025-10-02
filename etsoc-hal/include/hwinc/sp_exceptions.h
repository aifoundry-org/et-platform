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
 *  @Component      exceptions
 *
 *  @Filename       sp_exceptions.h
 *
 *  @Description    The component defines SP exceptions
 *
 *//*======================================================================== */

#ifndef __SP_EXCEPTIONS_H 
#define __SP_EXCEPTIONS_H 

#ifdef __cplusplus
extern "C"
{
#endif

#define INSTRUCTION_ADDRESS_MISALIGNED  0
#define INSTRUCTION_ACCESS_FAULT        1 
#define ILLEGAL_INSTRUCTION             2
#define BREAKPOINT                      3
#define LOAD_ADDRESS_MISALIGNED         4
#define LOAD_ACCESS_FAULT               5
#define STORE_ADDRESS_MISALIGNED        6
#define STORE_ACCESS_FAULT              7
#define USER_MODE_ECALL                 8
#define S_MODE_ECALL_EXCEPTION          9
/*#define VS_ECALL_EXCEPTION           10 it's not in Spec anymore */
#define M_MODE_ECALL                   11
#define INSTRUCTION_PAGE_FAULT         12
#define LOAD_PAGE_FAULT                13
#define STORE_PAGE_FAULT               15
#define INSTRUCTION_BUS_ERROR          25
#define INSTRUCTION_ECC_ERROR          26
#define LOAD_SPLIT_PAGE_FAULT          27
#define STORE_SPLIT_PAGE_FAULT         28 
/*#define BUS_ERROR                    29 Moved to inerrupt */
#define MCODE_EMULATION                30 
/*#define TXFMA_OFF                    31 it's not in Spec anymore */

#define EXCP_NUMBER                    32

#ifdef __cplusplus
}
#endif

#endif /* __SP_EXCEPTIONS_H */

/*****     <EOF>     *****/
