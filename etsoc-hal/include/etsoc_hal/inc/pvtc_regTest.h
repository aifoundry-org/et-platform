/*------------------------------------------------------------------------- 
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*------------------------------------------------------------------------- 
*/

/**
* @file pvtc_regTest.h 
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
 *  @Filename       pvtc_regTest.h
 *
 *  @Description    IPs register table 
 *
 *//*======================================================================== */

#include "pvtc.h"


REGTEST_t pvtcRegs[] =

{

/* regAddress                                        regSize                       resetValue                                        bitMask                                           regName                            */   

//{  COMMON_IP_CLK_SYNTH_OFFSET,                       REGTEST_SIZE_32_BIT,          COMMON_IP_CLK_SYNTH_READ_MASK,                    Register,                                         "clk_synth",                       },   

//{  COMMON_IP_SDIF_DISABLE_OFFSET,                    REGTEST_SIZE_32_BIT,          COMMON_IP_SDIF_DISABLE_READ_MASK,                 Register,                                         "sdif_disable",                    },   

//{  COMMON_IP_SDIF_STATUS_OFFSET,                     REGTEST_SIZE_32_BIT,          COMMON_IP_SDIF_STATUS_READ_MASK,                  Register,                                         "sdif_status",                     },   

//{  COMMON_IP_SDIF_OFFSET,                            REGTEST_SIZE_32_BIT,          COMMON_IP_SDIF_READ_MASK,                         Register,                                         "sdif",                            },   

//{  COMMON_IP_SDIF_HALT_OFFSET,                       REGTEST_SIZE_32_BIT,          COMMON_IP_SDIF_HALT_READ_MASK,                    Register,                                         "sdif_halt",                       },   

//{  COMMON_IP_SDIF_CTRL_OFFSET,                       REGTEST_SIZE_32_BIT,          COMMON_IP_SDIF_CTRL_READ_MASK,                    Register,                                         "sdif_ctrl",                       },   

//{  COMMON_IP_SMPL_CTRL_OFFSET,                       REGTEST_SIZE_32_BIT,          COMMON_IP_SMPL_CTRL_READ_MASK,                    Register,                                         "smpl_ctrl",                       },   

//{  COMMON_IP_SMPL_HALT_OFFSET,                       REGTEST_SIZE_32_BIT,          COMMON_IP_SMPL_HALT_READ_MASK,                    Register,                                         "smpl_halt",                       },   

//{  COMMON_IP_SMPL_CNT_OFFSET,                        REGTEST_SIZE_32_BIT,          COMMON_IP_SMPL_CNT_READ_MASK,                     Register,                                         "smpl_cnt",                        },   

{  COMMON_PVT_COMP_ID_OFFSET,                        REGTEST_SIZE_32_BIT,          COMMON_PVT_COMP_ID_RESET_VALUE,                   COMMON_PVT_COMP_ID_WRITE_MASK,                    "pvt_comp_id",                     },   

{  COMMON_PVT_IP_CFG_OFFSET,                         REGTEST_SIZE_32_BIT,          COMMON_PVT_IP_CFG_RESET_VALUE,                    COMMON_PVT_IP_CFG_WRITE_MASK,                     "pvt_ip_cfg",                      },   

//{  COMMON_PVT_IP_NUM_OFFSET,                         REGTEST_SIZE_32_BIT,          COMMON_PVT_IP_NUM_RESET_VALUE,                    COMMON_PVT_IP_NUM_WRITE_MASK,                     "pvt_ip_num",                      },   

{  COMMON_PVT_TM_SCRATCH_OFFSET,                     REGTEST_SIZE_32_BIT,          COMMON_PVT_TM_SCRATCH_RESET_VALUE,                COMMON_PVT_TM_SCRATCH_WRITE_MASK,                 "pvt_tm_scratch",                  },   

//{  COMMON_PVT_REG_LOCK_OFFSET,                       REGTEST_SIZE_32_BIT,          COMMON_PVT_REG_LOCK_RESET_VALUE,                  COMMON_PVT_REG_LOCK_WRITE_MASK,                   "pvt_reg_lock",                    },   

{  COMMON_PVT_REG_LOCK_STATUS_OFFSET,                REGTEST_SIZE_32_BIT,          COMMON_PVT_REG_LOCK_STATUS_RESET_VALUE,           COMMON_PVT_REG_LOCK_STATUS_WRITE_MASK,            "pvt_reg_lock_status",             },   

{  COMMON_PVT_TAM_STATUS_OFFSET,                     REGTEST_SIZE_32_BIT,          COMMON_PVT_TAM_STATUS_RESET_VALUE,                COMMON_PVT_TAM_STATUS_WRITE_MASK,                 "pvt_tam_status",                  },   

//{  COMMON_PVT_TAM_CLEAR_OFFSET,                      REGTEST_SIZE_32_BIT,          COMMON_PVT_TAM_CLEAR_RESET_VALUE,                 COMMON_PVT_TAM_CLEAR_WRITE_MASK,                  "pvt_tam_clear",                   },   

{  COMMON_PVT_TMR_CTRL_OFFSET,                       REGTEST_SIZE_32_BIT,          COMMON_PVT_TMR_CTRL_RESET_VALUE,                  COMMON_PVT_TMR_CTRL_WRITE_MASK,                   "pvt_tmr_ctrl",                    },   

//{  COMMON_PVT_TMR_STATUS_OFFSET,                     REGTEST_SIZE_32_BIT,          COMMON_PVT_TMR_STATUS_RESET_VALUE,                COMMON_PVT_TMR_STATUS_WRITE_MASK,                 "pvt_tmr_status",                  },   

//{  COMMON_PVT_TMR_IRQ_CLEAR_OFFSET,                  REGTEST_SIZE_32_BIT,          COMMON_PVT_TMR_IRQ_CLEAR_RESET_VALUE,             COMMON_PVT_TMR_IRQ_CLEAR_WRITE_MASK,              "pvt_tmr_irq_clear",               },   

{  COMMON_PVT_TMR_IRQ_TEST_OFFSET,                   REGTEST_SIZE_32_BIT,          COMMON_PVT_TMR_IRQ_TEST_RESET_VALUE,              COMMON_PVT_TMR_IRQ_TEST_WRITE_MASK,               "pvt_tmr_irq_test",                },   

//{  IRQ_IRQ_EN_OFFSET,                                REGTEST_SIZE_32_BIT,          IRQ_IRQ_EN_RESET_VALUE,                           IRQ_IRQ_EN_WRITE_MASK,                            "irq_en",                          },   

//{  IRQ_IRQ_TR_MASK_OFFSET,                           REGTEST_SIZE_32_BIT,          IRQ_IRQ_TR_MASK_RESET_VALUE,                      IRQ_IRQ_TR_MASK_WRITE_MASK,                       "irq_tr_mask",                     },   

//{  IRQ_IRQ_TS_MASK_OFFSET,                           REGTEST_SIZE_32_BIT,          IRQ_IRQ_TS_MASK_RESET_VALUE,                      IRQ_IRQ_TS_MASK_WRITE_MASK,                       "irq_ts_mask",                     },   

//{  IRQ_IRQ_VM_MASK_OFFSET,                           REGTEST_SIZE_32_BIT,          IRQ_IRQ_VM_MASK_RESET_VALUE,                      IRQ_IRQ_VM_MASK_WRITE_MASK,                       "irq_vm_mask",                     },   

//{  IRQ_IRQ_PD_MASK_OFFSET,                           REGTEST_SIZE_32_BIT,          IRQ_IRQ_PD_MASK_RESET_VALUE,                      IRQ_IRQ_PD_MASK_WRITE_MASK,                       "irq_pd_mask",                     },   

{  IRQ_IRQ_TR_STATUS_OFFSET,                         REGTEST_SIZE_32_BIT,          IRQ_IRQ_TR_STATUS_RESET_VALUE,                    IRQ_IRQ_TR_STATUS_WRITE_MASK,                     "irq_tr_status",                   },   

{  IRQ_IRQ_TS_STATUS_OFFSET,                         REGTEST_SIZE_32_BIT,          IRQ_IRQ_TS_STATUS_RESET_VALUE,                    IRQ_IRQ_TS_STATUS_WRITE_MASK,                     "irq_ts_status",                   },   

{  IRQ_IRQ_VM_STATUS_OFFSET,                         REGTEST_SIZE_32_BIT,          IRQ_IRQ_VM_STATUS_RESET_VALUE,                    IRQ_IRQ_VM_STATUS_WRITE_MASK,                     "irq_vm_status",                   },   

{  IRQ_IRQ_PD_STATUS_OFFSET,                         REGTEST_SIZE_32_BIT,          IRQ_IRQ_PD_STATUS_RESET_VALUE,                    IRQ_IRQ_PD_STATUS_WRITE_MASK,                     "irq_pd_status",                   },   

{  IRQ_IRQ_TR_RAW_STATUS_OFFSET,                     REGTEST_SIZE_32_BIT,          IRQ_IRQ_TR_RAW_STATUS_RESET_VALUE,                IRQ_IRQ_TR_RAW_STATUS_WRITE_MASK,                 "irq_tr_raw_status",               },   

{  IRQ_IRQ_TS_RAW_STATUS_OFFSET,                     REGTEST_SIZE_32_BIT,          IRQ_IRQ_TS_RAW_STATUS_RESET_VALUE,                IRQ_IRQ_TS_RAW_STATUS_WRITE_MASK,                 "irq_ts_raw_status",               },   

{  IRQ_IRQ_VM_RAW_STATUS_OFFSET,                     REGTEST_SIZE_32_BIT,          IRQ_IRQ_VM_RAW_STATUS_RESET_VALUE,                IRQ_IRQ_VM_RAW_STATUS_WRITE_MASK,                 "irq_vm_raw_status",               },   

{  IRQ_IRQ_PD_RAW_STATUS_OFFSET,                     REGTEST_SIZE_32_BIT,          IRQ_IRQ_PD_RAW_STATUS_RESET_VALUE,                IRQ_IRQ_PD_RAW_STATUS_WRITE_MASK,                 "irq_pd_raw_status",               },   

   /* End List Delimiter */

{  REGTEST_END_OF_LIST,                              0,                            0,                                                0,                                                0,                                 }    

};  

