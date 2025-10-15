/*------------------------------------------------------------------------- 
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*------------------------------------------------------------------------- 
*/

/**
* @file i2c_0_spio_regTest.h 
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
 *  @Filename       i2c_0_spio_regTest.h
 *
 *  @Description    IPs register table 
 *
 *//*======================================================================== */

#include "DW_apb_i2c.h"


static REGTEST_t i2cRegs[] =

{

/* regAddress                                        regSize                       resetValue                                        bitMask                                           regName                            */   

//{  I2C_IC_CON_OFFSET,                                REGTEST_SIZE_32_BIT,          I2C_IC_CON_RESET_VALUE,                           I2C_IC_CON_WRITE_MASK,                            "IC_CON",                          },   

{  I2C_IC_TAR_OFFSET,                                REGTEST_SIZE_32_BIT,          I2C_IC_TAR_RESET_VALUE,                           I2C_IC_TAR_WRITE_MASK,                            "IC_TAR",                          },   

{  I2C_IC_SAR_OFFSET,                                REGTEST_SIZE_32_BIT,          I2C_IC_SAR_RESET_VALUE,                           I2C_IC_SAR_WRITE_MASK,                            "IC_SAR",                          },   

//{  I2C_IC_DATA_CMD_OFFSET,                           REGTEST_SIZE_32_BIT,          I2C_IC_DATA_CMD_RESET_VALUE,                      I2C_IC_DATA_CMD_WRITE_MASK,                       "IC_DATA_CMD",                     },   

//{  I2C_IC_SS_SCL_HCNT_OFFSET,                        REGTEST_SIZE_32_BIT,          I2C_IC_SS_SCL_HCNT_RESET_VALUE,                   I2C_IC_SS_SCL_HCNT_WRITE_MASK,                    "IC_SS_SCL_HCNT",                  },   

//{  I2C_IC_SS_SCL_LCNT_OFFSET,                        REGTEST_SIZE_32_BIT,          I2C_IC_SS_SCL_LCNT_RESET_VALUE,                   I2C_IC_SS_SCL_LCNT_WRITE_MASK,                    "IC_SS_SCL_LCNT",                  },   

//{  I2C_IC_FS_SCL_HCNT_OFFSET,                        REGTEST_SIZE_32_BIT,          I2C_IC_FS_SCL_HCNT_RESET_VALUE,                   I2C_IC_FS_SCL_HCNT_WRITE_MASK,                    "IC_FS_SCL_HCNT",                  },   

//{  I2C_IC_FS_SCL_LCNT_OFFSET,                        REGTEST_SIZE_32_BIT,          I2C_IC_FS_SCL_LCNT_RESET_VALUE,                   I2C_IC_FS_SCL_LCNT_WRITE_MASK,                    "IC_FS_SCL_LCNT",                  },   

{  I2C_IC_INTR_STAT_OFFSET,                          REGTEST_SIZE_32_BIT,          I2C_IC_INTR_STAT_RESET_VALUE,                     I2C_IC_INTR_STAT_WRITE_MASK,                      "IC_INTR_STAT",                    },   

{  I2C_IC_INTR_MASK_OFFSET,                          REGTEST_SIZE_32_BIT,          I2C_IC_INTR_MASK_RESET_VALUE,                     I2C_IC_INTR_MASK_WRITE_MASK,                      "IC_INTR_MASK",                    },   

{  I2C_IC_RAW_INTR_STAT_OFFSET,                      REGTEST_SIZE_32_BIT,          I2C_IC_RAW_INTR_STAT_RESET_VALUE,                 I2C_IC_RAW_INTR_STAT_WRITE_MASK,                  "IC_RAW_INTR_STAT",                },   

{  I2C_IC_RX_TL_OFFSET,                              REGTEST_SIZE_32_BIT,          I2C_IC_RX_TL_RESET_VALUE,                         I2C_IC_RX_TL_WRITE_MASK,                          "IC_RX_TL",                        },   

{  I2C_IC_TX_TL_OFFSET,                              REGTEST_SIZE_32_BIT,          I2C_IC_TX_TL_RESET_VALUE,                         I2C_IC_TX_TL_WRITE_MASK,                          "IC_TX_TL",                        },   

{  I2C_IC_CLR_INTR_OFFSET,                           REGTEST_SIZE_32_BIT,          I2C_IC_CLR_INTR_RESET_VALUE,                      I2C_IC_CLR_INTR_WRITE_MASK,                       "IC_CLR_INTR",                     },   

{  I2C_IC_CLR_RX_UNDER_OFFSET,                       REGTEST_SIZE_32_BIT,          I2C_IC_CLR_RX_UNDER_RESET_VALUE,                  I2C_IC_CLR_RX_UNDER_WRITE_MASK,                   "IC_CLR_RX_UNDER",                 },   

{  I2C_IC_CLR_RX_OVER_OFFSET,                        REGTEST_SIZE_32_BIT,          I2C_IC_CLR_RX_OVER_RESET_VALUE,                   I2C_IC_CLR_RX_OVER_WRITE_MASK,                    "IC_CLR_RX_OVER",                  },   

{  I2C_IC_CLR_TX_OVER_OFFSET,                        REGTEST_SIZE_32_BIT,          I2C_IC_CLR_TX_OVER_RESET_VALUE,                   I2C_IC_CLR_TX_OVER_WRITE_MASK,                    "IC_CLR_TX_OVER",                  },   

{  I2C_IC_CLR_RD_REQ_OFFSET,                         REGTEST_SIZE_32_BIT,          I2C_IC_CLR_RD_REQ_RESET_VALUE,                    I2C_IC_CLR_RD_REQ_WRITE_MASK,                     "IC_CLR_RD_REQ",                   },   

{  I2C_IC_CLR_TX_ABRT_OFFSET,                        REGTEST_SIZE_32_BIT,          I2C_IC_CLR_TX_ABRT_RESET_VALUE,                   I2C_IC_CLR_TX_ABRT_WRITE_MASK,                    "IC_CLR_TX_ABRT",                  },   

{  I2C_IC_CLR_RX_DONE_OFFSET,                        REGTEST_SIZE_32_BIT,          I2C_IC_CLR_RX_DONE_RESET_VALUE,                   I2C_IC_CLR_RX_DONE_WRITE_MASK,                    "IC_CLR_RX_DONE",                  },   

{  I2C_IC_CLR_ACTIVITY_OFFSET,                       REGTEST_SIZE_32_BIT,          I2C_IC_CLR_ACTIVITY_RESET_VALUE,                  I2C_IC_CLR_ACTIVITY_WRITE_MASK,                   "IC_CLR_ACTIVITY",                 },   

{  I2C_IC_CLR_STOP_DET_OFFSET,                       REGTEST_SIZE_32_BIT,          I2C_IC_CLR_STOP_DET_RESET_VALUE,                  I2C_IC_CLR_STOP_DET_WRITE_MASK,                   "IC_CLR_STOP_DET",                 },   

{  I2C_IC_CLR_START_DET_OFFSET,                      REGTEST_SIZE_32_BIT,          I2C_IC_CLR_START_DET_RESET_VALUE,                 I2C_IC_CLR_START_DET_WRITE_MASK,                  "IC_CLR_START_DET",                },   

{  I2C_IC_CLR_GEN_CALL_OFFSET,                       REGTEST_SIZE_32_BIT,          I2C_IC_CLR_GEN_CALL_RESET_VALUE,                  I2C_IC_CLR_GEN_CALL_WRITE_MASK,                   "IC_CLR_GEN_CALL",                 },   

//{  I2C_IC_ENABLE_OFFSET,                             REGTEST_SIZE_32_BIT,          I2C_IC_ENABLE_RESET_VALUE,                        I2C_IC_ENABLE_WRITE_MASK,                         "IC_ENABLE",                       },   

{  I2C_IC_STATUS_OFFSET,                             REGTEST_SIZE_32_BIT,          I2C_IC_STATUS_RESET_VALUE,                        I2C_IC_STATUS_WRITE_MASK,                         "IC_STATUS",                       },   

{  I2C_IC_TXFLR_OFFSET,                              REGTEST_SIZE_32_BIT,          I2C_IC_TXFLR_RESET_VALUE,                         I2C_IC_TXFLR_WRITE_MASK,                          "IC_TXFLR",                        },   

{  I2C_IC_RXFLR_OFFSET,                              REGTEST_SIZE_32_BIT,          I2C_IC_RXFLR_RESET_VALUE,                         I2C_IC_RXFLR_WRITE_MASK,                          "IC_RXFLR",                        },   

{  I2C_IC_SDA_HOLD_OFFSET,                           REGTEST_SIZE_32_BIT,          I2C_IC_SDA_HOLD_RESET_VALUE,                      I2C_IC_SDA_HOLD_WRITE_MASK,                       "IC_SDA_HOLD",                     },   

{  I2C_IC_TX_ABRT_SOURCE_OFFSET,                     REGTEST_SIZE_32_BIT,          I2C_IC_TX_ABRT_SOURCE_RESET_VALUE,                I2C_IC_TX_ABRT_SOURCE_WRITE_MASK,                 "IC_TX_ABRT_SOURCE",               },   

{  I2C_IC_SDA_SETUP_OFFSET,                          REGTEST_SIZE_32_BIT,          I2C_IC_SDA_SETUP_RESET_VALUE,                     I2C_IC_SDA_SETUP_WRITE_MASK,                      "IC_SDA_SETUP",                    },   

{  I2C_IC_ACK_GENERAL_CALL_OFFSET,                   REGTEST_SIZE_32_BIT,          I2C_IC_ACK_GENERAL_CALL_RESET_VALUE,              I2C_IC_ACK_GENERAL_CALL_WRITE_MASK,               "IC_ACK_GENERAL_CALL",             },   

{  I2C_IC_ENABLE_STATUS_OFFSET,                      REGTEST_SIZE_32_BIT,          I2C_IC_ENABLE_STATUS_RESET_VALUE,                 I2C_IC_ENABLE_STATUS_WRITE_MASK,                  "IC_ENABLE_STATUS",                },   

//{  I2C_IC_FS_SPKLEN_OFFSET,                          REGTEST_SIZE_32_BIT,          I2C_IC_FS_SPKLEN_RESET_VALUE,                     I2C_IC_FS_SPKLEN_WRITE_MASK,                      "IC_FS_SPKLEN",                    },   

//{  I2C_IC_SCL_STUCK_AT_LOW_TIMEOUT_OFFSET,           REGTEST_SIZE_32_BIT,          I2C_IC_SCL_STUCK_AT_LOW_TIMEOUT_RESET_VALUE,      I2C_IC_SCL_STUCK_AT_LOW_TIMEOUT_WRITE_MASK,       "IC_SCL_STUCK_AT_LOW_TIMEOUT",     },   

//{  I2C_IC_SDA_STUCK_AT_LOW_TIMEOUT_OFFSET,           REGTEST_SIZE_32_BIT,          I2C_IC_SDA_STUCK_AT_LOW_TIMEOUT_RESET_VALUE,      I2C_IC_SDA_STUCK_AT_LOW_TIMEOUT_WRITE_MASK,       "IC_SDA_STUCK_AT_LOW_TIMEOUT",     },   

{  I2C_IC_CLR_SCL_STUCK_DET_OFFSET,                  REGTEST_SIZE_32_BIT,          I2C_IC_CLR_SCL_STUCK_DET_RESET_VALUE,             I2C_IC_CLR_SCL_STUCK_DET_WRITE_MASK,              "IC_CLR_SCL_STUCK_DET",            },   

//{  I2C_IC_SMBUS_CLK_LOW_SEXT_OFFSET,                 REGTEST_SIZE_32_BIT,          I2C_IC_SMBUS_CLK_LOW_SEXT_RESET_VALUE,            I2C_IC_SMBUS_CLK_LOW_SEXT_WRITE_MASK,             "IC_SMBUS_CLK_LOW_SEXT",           },   

//{  I2C_IC_SMBUS_CLK_LOW_MEXT_OFFSET,                 REGTEST_SIZE_32_BIT,          I2C_IC_SMBUS_CLK_LOW_MEXT_RESET_VALUE,            I2C_IC_SMBUS_CLK_LOW_MEXT_WRITE_MASK,             "IC_SMBUS_CLK_LOW_MEXT",           },   

{  I2C_IC_SMBUS_THIGH_MAX_IDLE_COUNT_OFFSET,         REGTEST_SIZE_32_BIT,          I2C_IC_SMBUS_THIGH_MAX_IDLE_COUNT_RESET_VALUE,    I2C_IC_SMBUS_THIGH_MAX_IDLE_COUNT_WRITE_MASK,     "IC_SMBUS_THIGH_MAX_IDLE_COUNT",   },   

{  I2C_IC_SMBUS_INTR_STAT_OFFSET,                    REGTEST_SIZE_32_BIT,          I2C_IC_SMBUS_INTR_STAT_RESET_VALUE,               I2C_IC_SMBUS_INTR_STAT_WRITE_MASK,                "IC_SMBUS_INTR_STAT",              },   

{  I2C_IC_SMBUS_INTR_MASK_OFFSET,                    REGTEST_SIZE_32_BIT,          I2C_IC_SMBUS_INTR_MASK_RESET_VALUE,               I2C_IC_SMBUS_INTR_MASK_WRITE_MASK,                "IC_SMBUS_INTR_MASK",              },   

{  I2C_IC_SMBUS_RAW_INTR_STAT_OFFSET,                REGTEST_SIZE_32_BIT,          I2C_IC_SMBUS_RAW_INTR_STAT_RESET_VALUE,           I2C_IC_SMBUS_RAW_INTR_STAT_WRITE_MASK,            "IC_SMBUS_RAW_INTR_STAT",          },   

//{  I2C_IC_CLR_SMBUS_INTR_OFFSET,                     REGTEST_SIZE_32_BIT,          I2C_IC_CLR_SMBUS_INTR_RESET_VALUE,                I2C_IC_CLR_SMBUS_INTR_WRITE_MASK,                 "IC_CLR_SMBUS_INTR",               },   

{  I2C_REG_TIMEOUT_RST_OFFSET,                       REGTEST_SIZE_32_BIT,          I2C_REG_TIMEOUT_RST_RESET_VALUE,                  I2C_REG_TIMEOUT_RST_WRITE_MASK,                   "REG_TIMEOUT_RST",                 },   

{  I2C_IC_COMP_PARAM_1_OFFSET,                       REGTEST_SIZE_32_BIT,          I2C_IC_COMP_PARAM_1_RESET_VALUE,                  I2C_IC_COMP_PARAM_1_WRITE_MASK,                   "IC_COMP_PARAM_1",                 },   

{  I2C_IC_COMP_VERSION_OFFSET,                       REGTEST_SIZE_32_BIT,          I2C_IC_COMP_VERSION_RESET_VALUE,                  I2C_IC_COMP_VERSION_WRITE_MASK,                   "IC_COMP_VERSION",                 },   

{  I2C_IC_COMP_TYPE_OFFSET,                          REGTEST_SIZE_32_BIT,          I2C_IC_COMP_TYPE_RESET_VALUE,                     I2C_IC_COMP_TYPE_WRITE_MASK,                      "IC_COMP_TYPE",                    },   

   /* End List Delimiter */

{  REGTEST_END_OF_LIST,                              0,                            0,                                                0,                                                0,                                 }    

};  

