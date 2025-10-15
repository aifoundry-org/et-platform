/*------------------------------------------------------------------------- 
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*------------------------------------------------------------------------- 
*/

/**
* @file puPlic_regTest.h 
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
 *  @Filename       puPlic_regTest.h
 *
 *  @Description    IPs register table 
 *
 *//*======================================================================== */

#include "pu_plic.h"


static REGTEST_t pu_plicRegs[] =

{

/* regAddress                                        regSize                       resetValue                                        bitMask                                           regName                            */   

{  PU_PLIC_PRIORITY_0_OFFSET,                        REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_0_RESET_VALUE,                   PU_PLIC_PRIORITY_0_WRITE_MASK,                    "priority_0",                      },   

{  PU_PLIC_PRIORITY_1_OFFSET,                        REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_1_RESET_VALUE,                   PU_PLIC_PRIORITY_1_WRITE_MASK,                    "priority_1",                      },   

{  PU_PLIC_PRIORITY_2_OFFSET,                        REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_2_RESET_VALUE,                   PU_PLIC_PRIORITY_2_WRITE_MASK,                    "priority_2",                      },   

{  PU_PLIC_PRIORITY_3_OFFSET,                        REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_3_RESET_VALUE,                   PU_PLIC_PRIORITY_3_WRITE_MASK,                    "priority_3",                      },   

{  PU_PLIC_PRIORITY_4_OFFSET,                        REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_4_RESET_VALUE,                   PU_PLIC_PRIORITY_4_WRITE_MASK,                    "priority_4",                      },   

{  PU_PLIC_PRIORITY_5_OFFSET,                        REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_5_RESET_VALUE,                   PU_PLIC_PRIORITY_5_WRITE_MASK,                    "priority_5",                      },   

{  PU_PLIC_PRIORITY_6_OFFSET,                        REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_6_RESET_VALUE,                   PU_PLIC_PRIORITY_6_WRITE_MASK,                    "priority_6",                      },   

{  PU_PLIC_PRIORITY_7_OFFSET,                        REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_7_RESET_VALUE,                   PU_PLIC_PRIORITY_7_WRITE_MASK,                    "priority_7",                      },   

{  PU_PLIC_PRIORITY_8_OFFSET,                        REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_8_RESET_VALUE,                   PU_PLIC_PRIORITY_8_WRITE_MASK,                    "priority_8",                      },   

{  PU_PLIC_PRIORITY_9_OFFSET,                        REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_9_RESET_VALUE,                   PU_PLIC_PRIORITY_9_WRITE_MASK,                    "priority_9",                      },   

{  PU_PLIC_PRIORITY_10_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_10_RESET_VALUE,                  PU_PLIC_PRIORITY_10_WRITE_MASK,                   "priority_10",                     },   

{  PU_PLIC_PRIORITY_11_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_11_RESET_VALUE,                  PU_PLIC_PRIORITY_11_WRITE_MASK,                   "priority_11",                     },   

{  PU_PLIC_PRIORITY_12_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_12_RESET_VALUE,                  PU_PLIC_PRIORITY_12_WRITE_MASK,                   "priority_12",                     },   

{  PU_PLIC_PRIORITY_13_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_13_RESET_VALUE,                  PU_PLIC_PRIORITY_13_WRITE_MASK,                   "priority_13",                     },   

{  PU_PLIC_PRIORITY_14_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_14_RESET_VALUE,                  PU_PLIC_PRIORITY_14_WRITE_MASK,                   "priority_14",                     },   

{  PU_PLIC_PRIORITY_15_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_15_RESET_VALUE,                  PU_PLIC_PRIORITY_15_WRITE_MASK,                   "priority_15",                     },   

{  PU_PLIC_PRIORITY_16_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_16_RESET_VALUE,                  PU_PLIC_PRIORITY_16_WRITE_MASK,                   "priority_16",                     },   

{  PU_PLIC_PRIORITY_17_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_17_RESET_VALUE,                  PU_PLIC_PRIORITY_17_WRITE_MASK,                   "priority_17",                     },   

{  PU_PLIC_PRIORITY_18_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_18_RESET_VALUE,                  PU_PLIC_PRIORITY_18_WRITE_MASK,                   "priority_18",                     },   

{  PU_PLIC_PRIORITY_19_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_19_RESET_VALUE,                  PU_PLIC_PRIORITY_19_WRITE_MASK,                   "priority_19",                     },   

{  PU_PLIC_PRIORITY_20_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_20_RESET_VALUE,                  PU_PLIC_PRIORITY_20_WRITE_MASK,                   "priority_20",                     },   

{  PU_PLIC_PRIORITY_21_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_21_RESET_VALUE,                  PU_PLIC_PRIORITY_21_WRITE_MASK,                   "priority_21",                     },   

{  PU_PLIC_PRIORITY_22_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_22_RESET_VALUE,                  PU_PLIC_PRIORITY_22_WRITE_MASK,                   "priority_22",                     },   

{  PU_PLIC_PRIORITY_23_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_23_RESET_VALUE,                  PU_PLIC_PRIORITY_23_WRITE_MASK,                   "priority_23",                     },   

{  PU_PLIC_PRIORITY_24_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_24_RESET_VALUE,                  PU_PLIC_PRIORITY_24_WRITE_MASK,                   "priority_24",                     },   

{  PU_PLIC_PRIORITY_25_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_25_RESET_VALUE,                  PU_PLIC_PRIORITY_25_WRITE_MASK,                   "priority_25",                     },   

{  PU_PLIC_PRIORITY_26_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_26_RESET_VALUE,                  PU_PLIC_PRIORITY_26_WRITE_MASK,                   "priority_26",                     },   

{  PU_PLIC_PRIORITY_27_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_27_RESET_VALUE,                  PU_PLIC_PRIORITY_27_WRITE_MASK,                   "priority_27",                     },   

{  PU_PLIC_PRIORITY_28_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_28_RESET_VALUE,                  PU_PLIC_PRIORITY_28_WRITE_MASK,                   "priority_28",                     },   

{  PU_PLIC_PRIORITY_29_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_29_RESET_VALUE,                  PU_PLIC_PRIORITY_29_WRITE_MASK,                   "priority_29",                     },   

{  PU_PLIC_PRIORITY_30_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_30_RESET_VALUE,                  PU_PLIC_PRIORITY_30_WRITE_MASK,                   "priority_30",                     },   

{  PU_PLIC_PRIORITY_31_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_31_RESET_VALUE,                  PU_PLIC_PRIORITY_31_WRITE_MASK,                   "priority_31",                     },   

{  PU_PLIC_PRIORITY_32_OFFSET,                       REGTEST_SIZE_32_BIT,          PU_PLIC_PRIORITY_32_RESET_VALUE,                  PU_PLIC_PRIORITY_32_WRITE_MASK,                   "priority_32",                     },   

{  PU_PLIC_PENDING_R0_OFFSET,                        REGTEST_SIZE_32_BIT,          PU_PLIC_PENDING_R0_RESET_VALUE,                   PU_PLIC_PENDING_R0_WRITE_MASK,                    "pending_r0",                      },   

{  PU_PLIC_PENDING_R1_OFFSET,                        REGTEST_SIZE_32_BIT,          PU_PLIC_PENDING_R1_RESET_VALUE,                   PU_PLIC_PENDING_R1_WRITE_MASK,                    "pending_r1",                      },   

{  PU_PLIC_ENABLE_T0_R0_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T0_R0_RESET_VALUE,                 PU_PLIC_ENABLE_T0_R0_WRITE_MASK,                  "enable_t0_r0",                    },   

{  PU_PLIC_ENABLE_T0_R1_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T0_R1_RESET_VALUE,                 PU_PLIC_ENABLE_T0_R1_WRITE_MASK,                  "enable_t0_r1",                    },   

{  PU_PLIC_ENABLE_T1_R0_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T1_R0_RESET_VALUE,                 PU_PLIC_ENABLE_T1_R0_WRITE_MASK,                  "enable_t1_r0",                    },   

{  PU_PLIC_ENABLE_T1_R1_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T1_R1_RESET_VALUE,                 PU_PLIC_ENABLE_T1_R1_WRITE_MASK,                  "enable_t1_r1",                    },   

{  PU_PLIC_ENABLE_T2_R0_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T2_R0_RESET_VALUE,                 PU_PLIC_ENABLE_T2_R0_WRITE_MASK,                  "enable_t2_r0",                    },   

{  PU_PLIC_ENABLE_T2_R1_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T2_R1_RESET_VALUE,                 PU_PLIC_ENABLE_T2_R1_WRITE_MASK,                  "enable_t2_r1",                    },   

{  PU_PLIC_ENABLE_T3_R0_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T3_R0_RESET_VALUE,                 PU_PLIC_ENABLE_T3_R0_WRITE_MASK,                  "enable_t3_r0",                    },   

{  PU_PLIC_ENABLE_T3_R1_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T3_R1_RESET_VALUE,                 PU_PLIC_ENABLE_T3_R1_WRITE_MASK,                  "enable_t3_r1",                    },   

{  PU_PLIC_ENABLE_T4_R0_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T4_R0_RESET_VALUE,                 PU_PLIC_ENABLE_T4_R0_WRITE_MASK,                  "enable_t4_r0",                    },   

{  PU_PLIC_ENABLE_T4_R1_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T4_R1_RESET_VALUE,                 PU_PLIC_ENABLE_T4_R1_WRITE_MASK,                  "enable_t4_r1",                    },   

{  PU_PLIC_ENABLE_T5_R0_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T5_R0_RESET_VALUE,                 PU_PLIC_ENABLE_T5_R0_WRITE_MASK,                  "enable_t5_r0",                    },   

{  PU_PLIC_ENABLE_T5_R1_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T5_R1_RESET_VALUE,                 PU_PLIC_ENABLE_T5_R1_WRITE_MASK,                  "enable_t5_r1",                    },   

{  PU_PLIC_ENABLE_T6_R0_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T6_R0_RESET_VALUE,                 PU_PLIC_ENABLE_T6_R0_WRITE_MASK,                  "enable_t6_r0",                    },   

{  PU_PLIC_ENABLE_T6_R1_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T6_R1_RESET_VALUE,                 PU_PLIC_ENABLE_T6_R1_WRITE_MASK,                  "enable_t6_r1",                    },   

{  PU_PLIC_ENABLE_T7_R0_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T7_R0_RESET_VALUE,                 PU_PLIC_ENABLE_T7_R0_WRITE_MASK,                  "enable_t7_r0",                    },   

{  PU_PLIC_ENABLE_T7_R1_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T7_R1_RESET_VALUE,                 PU_PLIC_ENABLE_T7_R1_WRITE_MASK,                  "enable_t7_r1",                    },   

{  PU_PLIC_ENABLE_T8_R0_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T8_R0_RESET_VALUE,                 PU_PLIC_ENABLE_T8_R0_WRITE_MASK,                  "enable_t8_r0",                    },   

{  PU_PLIC_ENABLE_T8_R1_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T8_R1_RESET_VALUE,                 PU_PLIC_ENABLE_T8_R1_WRITE_MASK,                  "enable_t8_r1",                    },   

{  PU_PLIC_ENABLE_T9_R0_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T9_R0_RESET_VALUE,                 PU_PLIC_ENABLE_T9_R0_WRITE_MASK,                  "enable_t9_r0",                    },   

{  PU_PLIC_ENABLE_T9_R1_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T9_R1_RESET_VALUE,                 PU_PLIC_ENABLE_T9_R1_WRITE_MASK,                  "enable_t9_r1",                    },   

{  PU_PLIC_ENABLE_T10_R0_OFFSET,                     REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T10_R0_RESET_VALUE,                PU_PLIC_ENABLE_T10_R0_WRITE_MASK,                 "enable_t10_r0",                   },   

{  PU_PLIC_ENABLE_T10_R1_OFFSET,                     REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T10_R1_RESET_VALUE,                PU_PLIC_ENABLE_T10_R1_WRITE_MASK,                 "enable_t10_r1",                   },   

{  PU_PLIC_ENABLE_T11_R0_OFFSET,                     REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T11_R0_RESET_VALUE,                PU_PLIC_ENABLE_T11_R0_WRITE_MASK,                 "enable_t11_r0",                   },   

{  PU_PLIC_ENABLE_T11_R1_OFFSET,                     REGTEST_SIZE_32_BIT,          PU_PLIC_ENABLE_T11_R1_RESET_VALUE,                PU_PLIC_ENABLE_T11_R1_WRITE_MASK,                 "enable_t11_r1",                   },   

{  PU_PLIC_THRESHOLD_T0_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_THRESHOLD_T0_RESET_VALUE,                 PU_PLIC_THRESHOLD_T0_WRITE_MASK,                  "threshold_t0",                    },   

{  PU_PLIC_MAXID_T0_OFFSET,                          REGTEST_SIZE_32_BIT,          PU_PLIC_MAXID_T0_RESET_VALUE,                     0x0, /* maxId is writable only after read */                       "maxID_t0",                        },   

{  PU_PLIC_THRESHOLD_T1_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_THRESHOLD_T1_RESET_VALUE,                 PU_PLIC_THRESHOLD_T1_WRITE_MASK,                  "threshold_t1",                    },   

{  PU_PLIC_MAXID_T1_OFFSET,                          REGTEST_SIZE_32_BIT,          PU_PLIC_MAXID_T1_RESET_VALUE,                     0x0, /* maxId is writable only after read */                       "maxID_t1",                        },   

{  PU_PLIC_THRESHOLD_T2_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_THRESHOLD_T2_RESET_VALUE,                 PU_PLIC_THRESHOLD_T2_WRITE_MASK,                  "threshold_t2",                    },   

{  PU_PLIC_MAXID_T2_OFFSET,                          REGTEST_SIZE_32_BIT,          PU_PLIC_MAXID_T2_RESET_VALUE,                     0x0, /* maxId is writable only after read */                       "maxID_t2",                        },   

{  PU_PLIC_THRESHOLD_T3_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_THRESHOLD_T3_RESET_VALUE,                 PU_PLIC_THRESHOLD_T3_WRITE_MASK,                  "threshold_t3",                    },   

{  PU_PLIC_MAXID_T3_OFFSET,                          REGTEST_SIZE_32_BIT,          PU_PLIC_MAXID_T3_RESET_VALUE,                     0x0, /* maxId is writable only after read */                       "maxID_t3",                        },   

{  PU_PLIC_THRESHOLD_T4_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_THRESHOLD_T4_RESET_VALUE,                 PU_PLIC_THRESHOLD_T4_WRITE_MASK,                  "threshold_t4",                    },   

{  PU_PLIC_MAXID_T4_OFFSET,                          REGTEST_SIZE_32_BIT,          PU_PLIC_MAXID_T4_RESET_VALUE,                     0x0, /* maxId is writable only after read */                       "maxID_t4",                        },   

{  PU_PLIC_THRESHOLD_T5_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_THRESHOLD_T5_RESET_VALUE,                 PU_PLIC_THRESHOLD_T5_WRITE_MASK,                  "threshold_t5",                    },   

{  PU_PLIC_MAXID_T5_OFFSET,                          REGTEST_SIZE_32_BIT,          PU_PLIC_MAXID_T5_RESET_VALUE,                     0x0, /* maxId is writable only after read */                       "maxID_t5",                        },   

{  PU_PLIC_THRESHOLD_T6_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_THRESHOLD_T6_RESET_VALUE,                 PU_PLIC_THRESHOLD_T6_WRITE_MASK,                  "threshold_t6",                    },   

{  PU_PLIC_MAXID_T6_OFFSET,                          REGTEST_SIZE_32_BIT,          PU_PLIC_MAXID_T6_RESET_VALUE,                     0x0, /* maxId is writable only after read */                       "maxID_t6",                        },   

{  PU_PLIC_THRESHOLD_T7_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_THRESHOLD_T7_RESET_VALUE,                 PU_PLIC_THRESHOLD_T7_WRITE_MASK,                  "threshold_t7",                    },   

{  PU_PLIC_MAXID_T7_OFFSET,                          REGTEST_SIZE_32_BIT,          PU_PLIC_MAXID_T7_RESET_VALUE,                     0x0, /* maxId is writable only after read */                       "maxID_t7",                        },   

{  PU_PLIC_THRESHOLD_T8_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_THRESHOLD_T8_RESET_VALUE,                 PU_PLIC_THRESHOLD_T8_WRITE_MASK,                  "threshold_t8",                    },   

{  PU_PLIC_MAXID_T8_OFFSET,                          REGTEST_SIZE_32_BIT,          PU_PLIC_MAXID_T8_RESET_VALUE,                     0x0, /* maxId is writable only after read */                       "maxID_t8",                        },   

{  PU_PLIC_THRESHOLD_T9_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_PLIC_THRESHOLD_T9_RESET_VALUE,                 PU_PLIC_THRESHOLD_T9_WRITE_MASK,                  "threshold_t9",                    },   

{  PU_PLIC_MAXID_T9_OFFSET,                          REGTEST_SIZE_32_BIT,          PU_PLIC_MAXID_T9_RESET_VALUE,                     0x0, /* maxId is writable only after read */                       "maxID_t9",                        },   

{  PU_PLIC_THRESHOLD_T10_OFFSET,                     REGTEST_SIZE_32_BIT,          PU_PLIC_THRESHOLD_T10_RESET_VALUE,                PU_PLIC_THRESHOLD_T10_WRITE_MASK,                 "threshold_t10",                   },   

{  PU_PLIC_MAXID_T10_OFFSET,                         REGTEST_SIZE_32_BIT,          PU_PLIC_MAXID_T10_RESET_VALUE,                    0x0, /* maxId is writable only after read */                      "maxID_t10",                       },   

{  PU_PLIC_THRESHOLD_T11_OFFSET,                     REGTEST_SIZE_32_BIT,          PU_PLIC_THRESHOLD_T11_RESET_VALUE,                PU_PLIC_THRESHOLD_T11_WRITE_MASK,                 "threshold_t11",                   },   

{  PU_PLIC_MAXID_T11_OFFSET,                         REGTEST_SIZE_32_BIT,          PU_PLIC_MAXID_T11_RESET_VALUE,                    0x0, /* maxId is writable only after read */                      "maxID_t11",                       },   

   /* End List Delimiter */

{  REGTEST_END_OF_LIST,                              0,                            0,                                                0,                                                0,                                 }    

};  

