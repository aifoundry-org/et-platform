/*------------------------------------------------------------------------- 
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*------------------------------------------------------------------------- 
*/

/**
* @file spioPlic_regTest.h 
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
 *  @Filename       spioPlic_regTest.h
 *
 *  @Description    IPs register table 
 *
 *//*======================================================================== */

#include "spio_plic.h"


static REGTEST_t spio_plicRegs[] =

{

/* regAddress                                        regSize                       resetValue                                        bitMask                                           regName                            */   

{  SPIO_PLIC_PRIORITY_0_OFFSET,                      REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_0_RESET_VALUE,                 SPIO_PLIC_PRIORITY_0_WRITE_MASK,                  "priority_0",                      },   

{  SPIO_PLIC_PRIORITY_1_OFFSET,                      REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_1_RESET_VALUE,                 SPIO_PLIC_PRIORITY_1_WRITE_MASK,                  "priority_1",                      },   

{  SPIO_PLIC_PRIORITY_2_OFFSET,                      REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_2_RESET_VALUE,                 SPIO_PLIC_PRIORITY_2_WRITE_MASK,                  "priority_2",                      },   

{  SPIO_PLIC_PRIORITY_3_OFFSET,                      REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_3_RESET_VALUE,                 SPIO_PLIC_PRIORITY_3_WRITE_MASK,                  "priority_3",                      },   

{  SPIO_PLIC_PRIORITY_4_OFFSET,                      REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_4_RESET_VALUE,                 SPIO_PLIC_PRIORITY_4_WRITE_MASK,                  "priority_4",                      },   

{  SPIO_PLIC_PRIORITY_5_OFFSET,                      REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_5_RESET_VALUE,                 SPIO_PLIC_PRIORITY_5_WRITE_MASK,                  "priority_5",                      },   

{  SPIO_PLIC_PRIORITY_6_OFFSET,                      REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_6_RESET_VALUE,                 SPIO_PLIC_PRIORITY_6_WRITE_MASK,                  "priority_6",                      },   

{  SPIO_PLIC_PRIORITY_7_OFFSET,                      REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_7_RESET_VALUE,                 SPIO_PLIC_PRIORITY_7_WRITE_MASK,                  "priority_7",                      },   

{  SPIO_PLIC_PRIORITY_8_OFFSET,                      REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_8_RESET_VALUE,                 SPIO_PLIC_PRIORITY_8_WRITE_MASK,                  "priority_8",                      },   

{  SPIO_PLIC_PRIORITY_9_OFFSET,                      REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_9_RESET_VALUE,                 SPIO_PLIC_PRIORITY_9_WRITE_MASK,                  "priority_9",                      },   

{  SPIO_PLIC_PRIORITY_10_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_10_RESET_VALUE,                SPIO_PLIC_PRIORITY_10_WRITE_MASK,                 "priority_10",                     },   

{  SPIO_PLIC_PRIORITY_11_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_11_RESET_VALUE,                SPIO_PLIC_PRIORITY_11_WRITE_MASK,                 "priority_11",                     },   

{  SPIO_PLIC_PRIORITY_12_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_12_RESET_VALUE,                SPIO_PLIC_PRIORITY_12_WRITE_MASK,                 "priority_12",                     },   

{  SPIO_PLIC_PRIORITY_13_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_13_RESET_VALUE,                SPIO_PLIC_PRIORITY_13_WRITE_MASK,                 "priority_13",                     },   

{  SPIO_PLIC_PRIORITY_14_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_14_RESET_VALUE,                SPIO_PLIC_PRIORITY_14_WRITE_MASK,                 "priority_14",                     },   

{  SPIO_PLIC_PRIORITY_15_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_15_RESET_VALUE,                SPIO_PLIC_PRIORITY_15_WRITE_MASK,                 "priority_15",                     },   

{  SPIO_PLIC_PRIORITY_16_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_16_RESET_VALUE,                SPIO_PLIC_PRIORITY_16_WRITE_MASK,                 "priority_16",                     },   

{  SPIO_PLIC_PRIORITY_17_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_17_RESET_VALUE,                SPIO_PLIC_PRIORITY_17_WRITE_MASK,                 "priority_17",                     },   

{  SPIO_PLIC_PRIORITY_18_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_18_RESET_VALUE,                SPIO_PLIC_PRIORITY_18_WRITE_MASK,                 "priority_18",                     },   

{  SPIO_PLIC_PRIORITY_19_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_19_RESET_VALUE,                SPIO_PLIC_PRIORITY_19_WRITE_MASK,                 "priority_19",                     },   

{  SPIO_PLIC_PRIORITY_20_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_20_RESET_VALUE,                SPIO_PLIC_PRIORITY_20_WRITE_MASK,                 "priority_20",                     },   

{  SPIO_PLIC_PRIORITY_21_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_21_RESET_VALUE,                SPIO_PLIC_PRIORITY_21_WRITE_MASK,                 "priority_21",                     },   

{  SPIO_PLIC_PRIORITY_22_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_22_RESET_VALUE,                SPIO_PLIC_PRIORITY_22_WRITE_MASK,                 "priority_22",                     },   

{  SPIO_PLIC_PRIORITY_23_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_23_RESET_VALUE,                SPIO_PLIC_PRIORITY_23_WRITE_MASK,                 "priority_23",                     },   

{  SPIO_PLIC_PRIORITY_24_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_24_RESET_VALUE,                SPIO_PLIC_PRIORITY_24_WRITE_MASK,                 "priority_24",                     },   

{  SPIO_PLIC_PRIORITY_25_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_25_RESET_VALUE,                SPIO_PLIC_PRIORITY_25_WRITE_MASK,                 "priority_25",                     },   

{  SPIO_PLIC_PRIORITY_26_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_26_RESET_VALUE,                SPIO_PLIC_PRIORITY_26_WRITE_MASK,                 "priority_26",                     },   

{  SPIO_PLIC_PRIORITY_27_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_27_RESET_VALUE,                SPIO_PLIC_PRIORITY_27_WRITE_MASK,                 "priority_27",                     },   

{  SPIO_PLIC_PRIORITY_28_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_28_RESET_VALUE,                SPIO_PLIC_PRIORITY_28_WRITE_MASK,                 "priority_28",                     },   

{  SPIO_PLIC_PRIORITY_29_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_29_RESET_VALUE,                SPIO_PLIC_PRIORITY_29_WRITE_MASK,                 "priority_29",                     },   

{  SPIO_PLIC_PRIORITY_30_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_30_RESET_VALUE,                SPIO_PLIC_PRIORITY_30_WRITE_MASK,                 "priority_30",                     },   

{  SPIO_PLIC_PRIORITY_31_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_31_RESET_VALUE,                SPIO_PLIC_PRIORITY_31_WRITE_MASK,                 "priority_31",                     },   

{  SPIO_PLIC_PRIORITY_32_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_32_RESET_VALUE,                SPIO_PLIC_PRIORITY_32_WRITE_MASK,                 "priority_32",                     },   

{  SPIO_PLIC_PRIORITY_33_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_33_RESET_VALUE,                SPIO_PLIC_PRIORITY_33_WRITE_MASK,                 "priority_33",                     },   

{  SPIO_PLIC_PRIORITY_34_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_34_RESET_VALUE,                SPIO_PLIC_PRIORITY_34_WRITE_MASK,                 "priority_34",                     },   

{  SPIO_PLIC_PRIORITY_35_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_35_RESET_VALUE,                SPIO_PLIC_PRIORITY_35_WRITE_MASK,                 "priority_35",                     },   

{  SPIO_PLIC_PRIORITY_36_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_36_RESET_VALUE,                SPIO_PLIC_PRIORITY_36_WRITE_MASK,                 "priority_36",                     },   

{  SPIO_PLIC_PRIORITY_37_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_37_RESET_VALUE,                SPIO_PLIC_PRIORITY_37_WRITE_MASK,                 "priority_37",                     },   

{  SPIO_PLIC_PRIORITY_38_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_38_RESET_VALUE,                SPIO_PLIC_PRIORITY_38_WRITE_MASK,                 "priority_38",                     },   

{  SPIO_PLIC_PRIORITY_39_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_39_RESET_VALUE,                SPIO_PLIC_PRIORITY_39_WRITE_MASK,                 "priority_39",                     },   

{  SPIO_PLIC_PRIORITY_40_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_40_RESET_VALUE,                SPIO_PLIC_PRIORITY_40_WRITE_MASK,                 "priority_40",                     },   

{  SPIO_PLIC_PRIORITY_41_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_41_RESET_VALUE,                SPIO_PLIC_PRIORITY_41_WRITE_MASK,                 "priority_41",                     },   

{  SPIO_PLIC_PRIORITY_42_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_42_RESET_VALUE,                SPIO_PLIC_PRIORITY_42_WRITE_MASK,                 "priority_42",                     },   

{  SPIO_PLIC_PRIORITY_43_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_43_RESET_VALUE,                SPIO_PLIC_PRIORITY_43_WRITE_MASK,                 "priority_43",                     },   

{  SPIO_PLIC_PRIORITY_44_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_44_RESET_VALUE,                SPIO_PLIC_PRIORITY_44_WRITE_MASK,                 "priority_44",                     },   

{  SPIO_PLIC_PRIORITY_45_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_45_RESET_VALUE,                SPIO_PLIC_PRIORITY_45_WRITE_MASK,                 "priority_45",                     },   

{  SPIO_PLIC_PRIORITY_46_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_46_RESET_VALUE,                SPIO_PLIC_PRIORITY_46_WRITE_MASK,                 "priority_46",                     },   

{  SPIO_PLIC_PRIORITY_47_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_47_RESET_VALUE,                SPIO_PLIC_PRIORITY_47_WRITE_MASK,                 "priority_47",                     },   

{  SPIO_PLIC_PRIORITY_48_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_48_RESET_VALUE,                SPIO_PLIC_PRIORITY_48_WRITE_MASK,                 "priority_48",                     },   

{  SPIO_PLIC_PRIORITY_49_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_49_RESET_VALUE,                SPIO_PLIC_PRIORITY_49_WRITE_MASK,                 "priority_49",                     },   

{  SPIO_PLIC_PRIORITY_50_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_50_RESET_VALUE,                SPIO_PLIC_PRIORITY_50_WRITE_MASK,                 "priority_50",                     },   

{  SPIO_PLIC_PRIORITY_51_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_51_RESET_VALUE,                SPIO_PLIC_PRIORITY_51_WRITE_MASK,                 "priority_51",                     },   

{  SPIO_PLIC_PRIORITY_52_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_52_RESET_VALUE,                SPIO_PLIC_PRIORITY_52_WRITE_MASK,                 "priority_52",                     },   

{  SPIO_PLIC_PRIORITY_53_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_53_RESET_VALUE,                SPIO_PLIC_PRIORITY_53_WRITE_MASK,                 "priority_53",                     },   

{  SPIO_PLIC_PRIORITY_54_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_54_RESET_VALUE,                SPIO_PLIC_PRIORITY_54_WRITE_MASK,                 "priority_54",                     },   

{  SPIO_PLIC_PRIORITY_55_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_55_RESET_VALUE,                SPIO_PLIC_PRIORITY_55_WRITE_MASK,                 "priority_55",                     },   

{  SPIO_PLIC_PRIORITY_56_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_56_RESET_VALUE,                SPIO_PLIC_PRIORITY_56_WRITE_MASK,                 "priority_56",                     },   

{  SPIO_PLIC_PRIORITY_57_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_57_RESET_VALUE,                SPIO_PLIC_PRIORITY_57_WRITE_MASK,                 "priority_57",                     },   

{  SPIO_PLIC_PRIORITY_58_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_58_RESET_VALUE,                SPIO_PLIC_PRIORITY_58_WRITE_MASK,                 "priority_58",                     },   

{  SPIO_PLIC_PRIORITY_59_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_59_RESET_VALUE,                SPIO_PLIC_PRIORITY_59_WRITE_MASK,                 "priority_59",                     },   

{  SPIO_PLIC_PRIORITY_60_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_60_RESET_VALUE,                SPIO_PLIC_PRIORITY_60_WRITE_MASK,                 "priority_60",                     },   

{  SPIO_PLIC_PRIORITY_61_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_61_RESET_VALUE,                SPIO_PLIC_PRIORITY_61_WRITE_MASK,                 "priority_61",                     },   

{  SPIO_PLIC_PRIORITY_62_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_62_RESET_VALUE,                SPIO_PLIC_PRIORITY_62_WRITE_MASK,                 "priority_62",                     },   

{  SPIO_PLIC_PRIORITY_63_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_63_RESET_VALUE,                SPIO_PLIC_PRIORITY_63_WRITE_MASK,                 "priority_63",                     },   

{  SPIO_PLIC_PRIORITY_64_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_64_RESET_VALUE,                SPIO_PLIC_PRIORITY_64_WRITE_MASK,                 "priority_64",                     },   

{  SPIO_PLIC_PRIORITY_65_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_65_RESET_VALUE,                SPIO_PLIC_PRIORITY_65_WRITE_MASK,                 "priority_65",                     },   

{  SPIO_PLIC_PRIORITY_66_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_66_RESET_VALUE,                SPIO_PLIC_PRIORITY_66_WRITE_MASK,                 "priority_66",                     },   

{  SPIO_PLIC_PRIORITY_67_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_67_RESET_VALUE,                SPIO_PLIC_PRIORITY_67_WRITE_MASK,                 "priority_67",                     },   

{  SPIO_PLIC_PRIORITY_68_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_68_RESET_VALUE,                SPIO_PLIC_PRIORITY_68_WRITE_MASK,                 "priority_68",                     },   

{  SPIO_PLIC_PRIORITY_69_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_69_RESET_VALUE,                SPIO_PLIC_PRIORITY_69_WRITE_MASK,                 "priority_69",                     },   

{  SPIO_PLIC_PRIORITY_70_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_70_RESET_VALUE,                SPIO_PLIC_PRIORITY_70_WRITE_MASK,                 "priority_70",                     },   

{  SPIO_PLIC_PRIORITY_71_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_71_RESET_VALUE,                SPIO_PLIC_PRIORITY_71_WRITE_MASK,                 "priority_71",                     },   

{  SPIO_PLIC_PRIORITY_72_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_72_RESET_VALUE,                SPIO_PLIC_PRIORITY_72_WRITE_MASK,                 "priority_72",                     },   

{  SPIO_PLIC_PRIORITY_73_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_73_RESET_VALUE,                SPIO_PLIC_PRIORITY_73_WRITE_MASK,                 "priority_73",                     },   

{  SPIO_PLIC_PRIORITY_74_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_74_RESET_VALUE,                SPIO_PLIC_PRIORITY_74_WRITE_MASK,                 "priority_74",                     },   

{  SPIO_PLIC_PRIORITY_75_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_75_RESET_VALUE,                SPIO_PLIC_PRIORITY_75_WRITE_MASK,                 "priority_75",                     },   

{  SPIO_PLIC_PRIORITY_76_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_76_RESET_VALUE,                SPIO_PLIC_PRIORITY_76_WRITE_MASK,                 "priority_76",                     },   

{  SPIO_PLIC_PRIORITY_77_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_77_RESET_VALUE,                SPIO_PLIC_PRIORITY_77_WRITE_MASK,                 "priority_77",                     },   

{  SPIO_PLIC_PRIORITY_78_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_78_RESET_VALUE,                SPIO_PLIC_PRIORITY_78_WRITE_MASK,                 "priority_78",                     },   

{  SPIO_PLIC_PRIORITY_79_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_79_RESET_VALUE,                SPIO_PLIC_PRIORITY_79_WRITE_MASK,                 "priority_79",                     },   

{  SPIO_PLIC_PRIORITY_80_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_80_RESET_VALUE,                SPIO_PLIC_PRIORITY_80_WRITE_MASK,                 "priority_80",                     },   

{  SPIO_PLIC_PRIORITY_81_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_81_RESET_VALUE,                SPIO_PLIC_PRIORITY_81_WRITE_MASK,                 "priority_81",                     },   

{  SPIO_PLIC_PRIORITY_82_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_82_RESET_VALUE,                SPIO_PLIC_PRIORITY_82_WRITE_MASK,                 "priority_82",                     },   

{  SPIO_PLIC_PRIORITY_83_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_83_RESET_VALUE,                SPIO_PLIC_PRIORITY_83_WRITE_MASK,                 "priority_83",                     },   

{  SPIO_PLIC_PRIORITY_84_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_84_RESET_VALUE,                SPIO_PLIC_PRIORITY_84_WRITE_MASK,                 "priority_84",                     },   

{  SPIO_PLIC_PRIORITY_85_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_85_RESET_VALUE,                SPIO_PLIC_PRIORITY_85_WRITE_MASK,                 "priority_85",                     },   

{  SPIO_PLIC_PRIORITY_86_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_86_RESET_VALUE,                SPIO_PLIC_PRIORITY_86_WRITE_MASK,                 "priority_86",                     },   

{  SPIO_PLIC_PRIORITY_87_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_87_RESET_VALUE,                SPIO_PLIC_PRIORITY_87_WRITE_MASK,                 "priority_87",                     },   

{  SPIO_PLIC_PRIORITY_88_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_88_RESET_VALUE,                SPIO_PLIC_PRIORITY_88_WRITE_MASK,                 "priority_88",                     },   

{  SPIO_PLIC_PRIORITY_89_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_89_RESET_VALUE,                SPIO_PLIC_PRIORITY_89_WRITE_MASK,                 "priority_89",                     },   

{  SPIO_PLIC_PRIORITY_90_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_90_RESET_VALUE,                SPIO_PLIC_PRIORITY_90_WRITE_MASK,                 "priority_90",                     },   

{  SPIO_PLIC_PRIORITY_91_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_91_RESET_VALUE,                SPIO_PLIC_PRIORITY_91_WRITE_MASK,                 "priority_91",                     },   

{  SPIO_PLIC_PRIORITY_92_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_92_RESET_VALUE,                SPIO_PLIC_PRIORITY_92_WRITE_MASK,                 "priority_92",                     },   

{  SPIO_PLIC_PRIORITY_93_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_93_RESET_VALUE,                SPIO_PLIC_PRIORITY_93_WRITE_MASK,                 "priority_93",                     },   

{  SPIO_PLIC_PRIORITY_94_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_94_RESET_VALUE,                SPIO_PLIC_PRIORITY_94_WRITE_MASK,                 "priority_94",                     },   

{  SPIO_PLIC_PRIORITY_95_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_95_RESET_VALUE,                SPIO_PLIC_PRIORITY_95_WRITE_MASK,                 "priority_95",                     },   

{  SPIO_PLIC_PRIORITY_96_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_96_RESET_VALUE,                SPIO_PLIC_PRIORITY_96_WRITE_MASK,                 "priority_96",                     },   

{  SPIO_PLIC_PRIORITY_97_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_97_RESET_VALUE,                SPIO_PLIC_PRIORITY_97_WRITE_MASK,                 "priority_97",                     },   

{  SPIO_PLIC_PRIORITY_98_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_98_RESET_VALUE,                SPIO_PLIC_PRIORITY_98_WRITE_MASK,                 "priority_98",                     },   

{  SPIO_PLIC_PRIORITY_99_OFFSET,                     REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_99_RESET_VALUE,                SPIO_PLIC_PRIORITY_99_WRITE_MASK,                 "priority_99",                     },   

{  SPIO_PLIC_PRIORITY_100_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_100_RESET_VALUE,               SPIO_PLIC_PRIORITY_100_WRITE_MASK,                "priority_100",                    },   

{  SPIO_PLIC_PRIORITY_101_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_101_RESET_VALUE,               SPIO_PLIC_PRIORITY_101_WRITE_MASK,                "priority_101",                    },   

{  SPIO_PLIC_PRIORITY_102_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_102_RESET_VALUE,               SPIO_PLIC_PRIORITY_102_WRITE_MASK,                "priority_102",                    },   

{  SPIO_PLIC_PRIORITY_103_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_103_RESET_VALUE,               SPIO_PLIC_PRIORITY_103_WRITE_MASK,                "priority_103",                    },   

{  SPIO_PLIC_PRIORITY_104_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_104_RESET_VALUE,               SPIO_PLIC_PRIORITY_104_WRITE_MASK,                "priority_104",                    },   

{  SPIO_PLIC_PRIORITY_105_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_105_RESET_VALUE,               SPIO_PLIC_PRIORITY_105_WRITE_MASK,                "priority_105",                    },   

{  SPIO_PLIC_PRIORITY_106_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_106_RESET_VALUE,               SPIO_PLIC_PRIORITY_106_WRITE_MASK,                "priority_106",                    },   

{  SPIO_PLIC_PRIORITY_107_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_107_RESET_VALUE,               SPIO_PLIC_PRIORITY_107_WRITE_MASK,                "priority_107",                    },   

{  SPIO_PLIC_PRIORITY_108_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_108_RESET_VALUE,               SPIO_PLIC_PRIORITY_108_WRITE_MASK,                "priority_108",                    },   

{  SPIO_PLIC_PRIORITY_109_OFFSET,                    REGTEST_SIZE_32_BIT,          0x6, /*priority is set in init, before regTest*/  SPIO_PLIC_PRIORITY_109_WRITE_MASK,                "priority_109",                    },   

{  SPIO_PLIC_PRIORITY_110_OFFSET,                    REGTEST_SIZE_32_BIT,          0x6, /*priority is set in init, before regTest*/  SPIO_PLIC_PRIORITY_110_WRITE_MASK,                "priority_110",                    },   

{  SPIO_PLIC_PRIORITY_111_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_111_RESET_VALUE,               SPIO_PLIC_PRIORITY_111_WRITE_MASK,                "priority_111",                    },   

{  SPIO_PLIC_PRIORITY_112_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_112_RESET_VALUE,               SPIO_PLIC_PRIORITY_112_WRITE_MASK,                "priority_112",                    },   

{  SPIO_PLIC_PRIORITY_113_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_113_RESET_VALUE,               SPIO_PLIC_PRIORITY_113_WRITE_MASK,                "priority_113",                    },   

{  SPIO_PLIC_PRIORITY_114_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_114_RESET_VALUE,               SPIO_PLIC_PRIORITY_114_WRITE_MASK,                "priority_114",                    },   

{  SPIO_PLIC_PRIORITY_115_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_115_RESET_VALUE,               SPIO_PLIC_PRIORITY_115_WRITE_MASK,                "priority_115",                    },   

{  SPIO_PLIC_PRIORITY_116_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_116_RESET_VALUE,               SPIO_PLIC_PRIORITY_116_WRITE_MASK,                "priority_116",                    },   

{  SPIO_PLIC_PRIORITY_117_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_117_RESET_VALUE,               SPIO_PLIC_PRIORITY_117_WRITE_MASK,                "priority_117",                    },   

{  SPIO_PLIC_PRIORITY_118_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_118_RESET_VALUE,               SPIO_PLIC_PRIORITY_118_WRITE_MASK,                "priority_118",                    },   

{  SPIO_PLIC_PRIORITY_119_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_119_RESET_VALUE,               SPIO_PLIC_PRIORITY_119_WRITE_MASK,                "priority_119",                    },   

{  SPIO_PLIC_PRIORITY_120_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_120_RESET_VALUE,               SPIO_PLIC_PRIORITY_120_WRITE_MASK,                "priority_120",                    },   

{  SPIO_PLIC_PRIORITY_121_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_121_RESET_VALUE,               SPIO_PLIC_PRIORITY_121_WRITE_MASK,                "priority_121",                    },   

{  SPIO_PLIC_PRIORITY_122_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_122_RESET_VALUE,               SPIO_PLIC_PRIORITY_122_WRITE_MASK,                "priority_122",                    },   

{  SPIO_PLIC_PRIORITY_123_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_123_RESET_VALUE,               SPIO_PLIC_PRIORITY_123_WRITE_MASK,                "priority_123",                    },   

{  SPIO_PLIC_PRIORITY_124_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_124_RESET_VALUE,               SPIO_PLIC_PRIORITY_124_WRITE_MASK,                "priority_124",                    },   

{  SPIO_PLIC_PRIORITY_125_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_125_RESET_VALUE,               SPIO_PLIC_PRIORITY_125_WRITE_MASK,                "priority_125",                    },   

{  SPIO_PLIC_PRIORITY_126_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_126_RESET_VALUE,               SPIO_PLIC_PRIORITY_126_WRITE_MASK,                "priority_126",                    },   

{  SPIO_PLIC_PRIORITY_127_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_127_RESET_VALUE,               SPIO_PLIC_PRIORITY_127_WRITE_MASK,                "priority_127",                    },   

{  SPIO_PLIC_PRIORITY_128_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_128_RESET_VALUE,               SPIO_PLIC_PRIORITY_128_WRITE_MASK,                "priority_128",                    },   

{  SPIO_PLIC_PRIORITY_129_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_129_RESET_VALUE,               SPIO_PLIC_PRIORITY_129_WRITE_MASK,                "priority_129",                    },   

{  SPIO_PLIC_PRIORITY_130_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_130_RESET_VALUE,               SPIO_PLIC_PRIORITY_130_WRITE_MASK,                "priority_130",                    },   

{  SPIO_PLIC_PRIORITY_131_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_131_RESET_VALUE,               SPIO_PLIC_PRIORITY_131_WRITE_MASK,                "priority_131",                    },   

{  SPIO_PLIC_PRIORITY_132_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_132_RESET_VALUE,               SPIO_PLIC_PRIORITY_132_WRITE_MASK,                "priority_132",                    },   

#ifdef SI_BRINGUP
{  SPIO_PLIC_PRIORITY_133_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_133_RESET_VALUE,               SPIO_PLIC_PRIORITY_133_WRITE_MASK,                "priority_133",                    },   
#else
{  SPIO_PLIC_PRIORITY_133_OFFSET,                    REGTEST_SIZE_32_BIT,          0x7, /*priority is set in soc.c*/                 SPIO_PLIC_PRIORITY_133_WRITE_MASK,                "priority_133",                    },   
  #pragma GCC warning "SPIO_PLIC_PRIORITY_133_OFFSET reset value is altered by the soc.c"
#endif
{  SPIO_PLIC_PRIORITY_134_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_134_RESET_VALUE,               SPIO_PLIC_PRIORITY_134_WRITE_MASK,                "priority_134",                    },   

{  SPIO_PLIC_PRIORITY_135_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_135_RESET_VALUE,               SPIO_PLIC_PRIORITY_135_WRITE_MASK,                "priority_135",                    },   

{  SPIO_PLIC_PRIORITY_136_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_136_RESET_VALUE,               SPIO_PLIC_PRIORITY_136_WRITE_MASK,                "priority_136",                    },   

{  SPIO_PLIC_PRIORITY_137_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_137_RESET_VALUE,               SPIO_PLIC_PRIORITY_137_WRITE_MASK,                "priority_137",                    },   

{  SPIO_PLIC_PRIORITY_138_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_138_RESET_VALUE,               SPIO_PLIC_PRIORITY_138_WRITE_MASK,                "priority_138",                    },   

{  SPIO_PLIC_PRIORITY_139_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_139_RESET_VALUE,               SPIO_PLIC_PRIORITY_139_WRITE_MASK,                "priority_139",                    },   

{  SPIO_PLIC_PRIORITY_140_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_140_RESET_VALUE,               SPIO_PLIC_PRIORITY_140_WRITE_MASK,                "priority_140",                    },   

{  SPIO_PLIC_PRIORITY_141_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_PRIORITY_141_RESET_VALUE,               SPIO_PLIC_PRIORITY_141_WRITE_MASK,                "priority_141",                    },   

{  SPIO_PLIC_PENDING_R0_OFFSET,                      REGTEST_SIZE_32_BIT,          SPIO_PLIC_PENDING_R0_RESET_VALUE,                 SPIO_PLIC_PENDING_R0_WRITE_MASK,                  "pending_r0",                      },   

{  SPIO_PLIC_PENDING_R1_OFFSET,                      REGTEST_SIZE_32_BIT,          SPIO_PLIC_PENDING_R1_RESET_VALUE,                 SPIO_PLIC_PENDING_R1_WRITE_MASK,                  "pending_r1",                      },   

{  SPIO_PLIC_PENDING_R2_OFFSET,                      REGTEST_SIZE_32_BIT,          SPIO_PLIC_PENDING_R2_RESET_VALUE,                 SPIO_PLIC_PENDING_R2_WRITE_MASK,                  "pending_r2",                      },   

{  SPIO_PLIC_PENDING_R3_OFFSET,                      REGTEST_SIZE_32_BIT,          SPIO_PLIC_PENDING_R3_RESET_VALUE,                 SPIO_PLIC_PENDING_R3_WRITE_MASK,                  "pending_r3",                      },   

{  SPIO_PLIC_PENDING_R4_OFFSET,                      REGTEST_SIZE_32_BIT,          SPIO_PLIC_PENDING_R4_RESET_VALUE,                 SPIO_PLIC_PENDING_R4_WRITE_MASK,                  "pending_r4",                      },   

{  SPIO_PLIC_ENABLE_T0_R0_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_ENABLE_T0_R0_RESET_VALUE,               SPIO_PLIC_ENABLE_T0_R0_WRITE_MASK,                "enable_t0_r0",                    },   

{  SPIO_PLIC_ENABLE_T0_R1_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_ENABLE_T0_R1_RESET_VALUE,               SPIO_PLIC_ENABLE_T0_R1_WRITE_MASK,                "enable_t0_r1",                    },   

{  SPIO_PLIC_ENABLE_T0_R2_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_ENABLE_T0_R2_RESET_VALUE,               SPIO_PLIC_ENABLE_T0_R2_WRITE_MASK,                "enable_t0_r2",                    },   

{  SPIO_PLIC_ENABLE_T0_R3_OFFSET,                    REGTEST_SIZE_32_BIT,          0x2000,                                           SPIO_PLIC_ENABLE_T0_R3_WRITE_MASK,                "enable_t0_r3",                    },                                              /*IRQ 109 is enabled in inth,before regTest */
#ifdef SI_BRINGUP
{  SPIO_PLIC_ENABLE_T0_R4_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_ENABLE_T0_R4_RESET_VALUE,              SPIO_PLIC_ENABLE_T0_R4_WRITE_MASK,                "enable_t0_r4",                    },                                              /*IRQ 133 is enabled in soc.c,before regTest */
#else
{  SPIO_PLIC_ENABLE_T0_R4_OFFSET,                    REGTEST_SIZE_32_BIT,           0x20,                                            SPIO_PLIC_ENABLE_T0_R4_WRITE_MASK,                "enable_t0_r4",                    },                                              /*IRQ 133 is enabled in soc.c,before regTest */
   #pragma GCC warning "SPIO_PLIC_ENABLE_T0_R4_OFFSET reset value is altered by the soc.c"
#endif
{  SPIO_PLIC_ENABLE_T1_R0_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_ENABLE_T1_R0_RESET_VALUE,               SPIO_PLIC_ENABLE_T1_R0_WRITE_MASK,                "enable_t1_r0",                    },   

{  SPIO_PLIC_ENABLE_T1_R1_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_ENABLE_T1_R1_RESET_VALUE,               SPIO_PLIC_ENABLE_T1_R1_WRITE_MASK,                "enable_t1_r1",                    },   

{  SPIO_PLIC_ENABLE_T1_R2_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_ENABLE_T1_R2_RESET_VALUE,               SPIO_PLIC_ENABLE_T1_R2_WRITE_MASK,                "enable_t1_r2",                    },   

{  SPIO_PLIC_ENABLE_T1_R3_OFFSET,                    REGTEST_SIZE_32_BIT,          0x4000,                                           SPIO_PLIC_ENABLE_T1_R3_WRITE_MASK,                "enable_t1_r3",                    },                                              /*IRQ 110 is enabled in inth,before regTest */

{  SPIO_PLIC_ENABLE_T1_R4_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_ENABLE_T1_R4_RESET_VALUE,               SPIO_PLIC_ENABLE_T1_R4_WRITE_MASK,                "enable_t1_r4",                    },   

{  SPIO_PLIC_THRESHOLD_T0_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_THRESHOLD_T0_RESET_VALUE,               SPIO_PLIC_THRESHOLD_T0_WRITE_MASK,                "threshold_t0",                    },   

{  SPIO_PLIC_MAXID_T0_OFFSET,                        REGTEST_SIZE_32_BIT,          SPIO_PLIC_MAXID_T0_RESET_VALUE,                   0x0, /* maxId is writable only after read */                      "maxID_t0",                        },   

{  SPIO_PLIC_THRESHOLD_T1_OFFSET,                    REGTEST_SIZE_32_BIT,          SPIO_PLIC_THRESHOLD_T1_RESET_VALUE,               SPIO_PLIC_THRESHOLD_T1_WRITE_MASK,                "threshold_t1",                    },   

{  SPIO_PLIC_MAXID_T1_OFFSET,                        REGTEST_SIZE_32_BIT,          SPIO_PLIC_MAXID_T1_RESET_VALUE,                   0x0, /* maxId is writable only after read */                     "maxID_t1",                        },   

   /* End List Delimiter */

{  REGTEST_END_OF_LIST,                              0,                            0,                                                0,                                                0,                                 }    

}; 


