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
* @file pShire_pll_regTest.h 
* @version $Release$ 
* @date $Date$
* @author 
*
* @brief 
*
* Setup SoC to enable TC run 
*/ 
/** 
 *  @Component      HAL
 *
 *  @Filename       pShire_pll_regTest.h
 *
 *  @Description    IPs register table 
 *
 *//*======================================================================== */

#include "mvls_tn7_hpdpll.ipxact.h"


REGTEST_t pShire_pllRegs[] =

{

/* regAddress                                        regSize                       resetValue                                        bitMask                                           regName                            */   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER0_OFFSET,       REGTEST_SIZE_16_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER0_READ_MASK,    Register,                                         "Register0",                       },   

{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER1_OFFSET*4,       REGTEST_SIZE_32_BIT,          (MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER1_RESET_VALUE | 0x1),  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER1_WRITE_MASK,   "Register1",                       },   

{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER2_OFFSET*4,       REGTEST_SIZE_32_BIT,          (0x14),  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER2_WRITE_MASK,   "Register2",                       },   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER3_OFFSET,       REGTEST_SIZE_16_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER3_READ_MASK,    Register,                                         "Register3",                       },   

{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER4_OFFSET*4,       REGTEST_SIZE_32_BIT,          (0x2bb),  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER4_WRITE_MASK,   "Register4",                       },   

{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER5_OFFSET*4,       REGTEST_SIZE_32_BIT,          (0xaeb),  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER5_WRITE_MASK,   "Register5",                       },   

{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER6_OFFSET*4,       REGTEST_SIZE_32_BIT,          (0x1bf4),  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER6_WRITE_MASK,   "Register6",                       },   

{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER7_OFFSET*4,       REGTEST_SIZE_32_BIT,          (0x2bb),  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER7_WRITE_MASK,   "Register7",                       },   

{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER8_OFFSET*4,       REGTEST_SIZE_32_BIT,          (0xaeb),  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER8_WRITE_MASK,   "Register8",                       },   

{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER9_OFFSET*4,       REGTEST_SIZE_32_BIT,          (0x1bf4),  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER9_WRITE_MASK,   "Register9",                       },   

{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER10_OFFSET*4,      REGTEST_SIZE_32_BIT,          (0x1f0), MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER10_WRITE_MASK,  "Register10",                      },   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER11_OFFSET,      REGTEST_SIZE_16_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER11_READ_MASK,   Register,                                         "Register11",                      },   

{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER12_OFFSET*4,      REGTEST_SIZE_32_BIT,          (0xc), MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER12_WRITE_MASK,  "Register12",                      },   

{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER13_OFFSET*4,      REGTEST_SIZE_32_BIT,          (0x0), MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER13_WRITE_MASK,  "Register13",                      },   

{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER14_OFFSET*4,      REGTEST_SIZE_32_BIT,          (0x2), MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER14_WRITE_MASK,  "Register14",                      },   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER15_OFFSET*4,      REGTEST_SIZE_32_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER15_RESET_VALUE, MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER15_WRITE_MASK,  "Register15",                      },   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER16_OFFSET,      REGTEST_SIZE_16_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER16_READ_MASK,   Register,                                         "Register16",                      },   

{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER22_OFFSET*4,      REGTEST_SIZE_32_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER22_RESET_VALUE, MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER22_WRITE_MASK,  "Register22",                      },   

{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER23_OFFSET*4,      REGTEST_SIZE_32_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER23_RESET_VALUE, MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER23_WRITE_MASK,  "Register23",                      },   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER24_OFFSET,      REGTEST_SIZE_16_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER24_READ_MASK,   Register,                                         "Register24",                      },   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER25_OFFSET,      REGTEST_SIZE_16_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER25_READ_MASK,   Register,                                         "Register25",                      },   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER26_OFFSET,      REGTEST_SIZE_16_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER26_READ_MASK,   Register,                                         "Register26",                      },   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER27_OFFSET,      REGTEST_SIZE_16_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER27_READ_MASK,   Register,                                         "Register27",                      },   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER28_OFFSET,      REGTEST_SIZE_16_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER28_READ_MASK,   Register,                                         "Register28",                      },   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER29_OFFSET,      REGTEST_SIZE_16_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER29_READ_MASK,   Register,                                         "Register29",                      },   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER30_OFFSET,      REGTEST_SIZE_16_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER30_READ_MASK,   Register,                                         "Register30",                      },   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER31_OFFSET,      REGTEST_SIZE_16_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER31_READ_MASK,   Register,                                         "Register31",                      },   

{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER32_OFFSET*4,      REGTEST_SIZE_32_BIT,          (0x1), MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER32_WRITE_MASK,  "Register32",                      },   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER33_OFFSET*4,      REGTEST_SIZE_32_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER33_RESET_VALUE, MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER33_WRITE_MASK,  "Register33",                      },   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER34_OFFSET,      REGTEST_SIZE_16_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER34_READ_MASK,   Register,                                         "Register34",                      },   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER35_OFFSET,      REGTEST_SIZE_16_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER35_READ_MASK,   Register,                                         "Register35",                      },   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER36_OFFSET,      REGTEST_SIZE_16_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER36_READ_MASK,   Register,                                         "Register36",                      },   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER37_OFFSET,      REGTEST_SIZE_16_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER37_READ_MASK,   Register,                                         "Register37",                      },   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER38_OFFSET,      REGTEST_SIZE_16_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER38_READ_MASK,   Register,                                         "Register38",                      },   

{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER39_OFFSET*4,      REGTEST_SIZE_32_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER39_RESET_VALUE, MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER39_WRITE_MASK,  "Register39",                      },   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER40_OFFSET,      REGTEST_SIZE_16_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER40_READ_MASK,   Register,                                         "Register40",                      },   

//{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER56_OFFSET,      REGTEST_SIZE_16_BIT,          MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER56_READ_MASK,   Register,                                         "Register56",                      },   

{  MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER57_OFFSET*4,      REGTEST_SIZE_32_BIT,          (MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER57_RESET_VALUE | 0x1), MVLS_TN7_HPDPLL_MAIN_MAIN_REGISTER57_WRITE_MASK,  "Register57",                      },   

   /* End List Delimiter */

{  REGTEST_END_OF_LIST,                              0,                            0,                                                0,                                                0,                                 }    

};  

