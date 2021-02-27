#include "bl2_ddr_init.h"
#include "dm_event_control.h"

//
// get_ms_reg_addr: This procedure is used to generate a memshire register address based on the
// memshire ID and register name.
//
static inline uint64_t *get_ms_reg_addr(uint8_t memshire, uint64_t reg)
{
    uint64_t *reg_addr;

    if ((reg & 0x0100000000) == 0) {
        reg_addr = (uint64_t *)((uint64_t)((memshire & 4) << 26) | reg);
    } else {
        reg_addr = (uint64_t *)((uint64_t)((memshire + 232) << 22) | reg);
    }
    return reg_addr;
}

static inline uint32_t ms_write_esr(uint8_t memshire, uint64_t reg, uint64_t value)
{
    uint64_t *addr = get_ms_reg_addr(memshire, reg);
    *addr = value;
    return 0;
}
static inline uint64_t ms_read_esr(uint8_t memshire, uint64_t reg)
{
    uint64_t *addr = get_ms_reg_addr(memshire, reg);
    return *addr;
}
// get_ddr_reg_addr: This procedure is used to generate a DDRC register address based on the
// memshire ID, block, and register name.  The block can have one of the following values:
//
//    0: ddr 0
//    1: ddr 1
//    2: phy
//    3: subystem
//
static inline uint32_t *get_ddrc_reg_addr(uint8_t memshire, uint32_t blk, uint32_t reg)
{
    uint32_t *reg_addr;

    if (blk == 1) {
        reg_addr = (uint32_t *)((uint64_t)reg | 0x00001000ul);
    } else {
        reg_addr = (uint32_t *)(uint64_t)reg;
    }
    reg_addr = (uint32_t *)(0x0060000000ul | (uint64_t)((memshire & 7) << 26) | (uint64_t)reg_addr);
    return reg_addr;
}

//
// Write register in DDRC0, DDRC1, or PHY
//
static inline uint32_t ms_write_ddrc_reg(uint8_t memshire, uint32_t blk, uint32_t reg,
                                         uint32_t value)
{
    uint32_t *addr = get_ddrc_reg_addr(memshire, blk, reg);
    *addr = value;
    return 0;
}

//
// Read register in DDRC0, DDRC1, or PHY
//
static inline uint32_t ms_read_ddrc_reg(uint8_t memshire, uint32_t blk, uint32_t reg)
{
    uint32_t *addr = (uint32_t *)get_ddrc_reg_addr(memshire, blk, reg);
    return *addr;
}

static inline uint32_t ms_poll_ddrc_reg(uint8_t memshire, uint32_t blk, uint32_t reg,
                                        uint32_t wait_value, uint32_t wait_mask,
                                        uint32_t timeout_tries)
{
    uint8_t done = 0;
    uint32_t read_value = 0;
    uint32_t timer = timeout_tries;

    while (done == 0) {
        read_value = ms_read_ddrc_reg(memshire, blk, reg);
        if (((read_value ^ wait_value) & wait_mask) == 0) {
            done = 1;
        } else
            timer--;
        if (timer == 0)
            return 1;
    }
    return 0;
}
//
// Write a register in both DDRC channels
//
static inline uint32_t ms_write_both_ddrc_reg(uint8_t memshire, uint32_t reg, uint32_t value)
{
    ms_write_ddrc_reg(memshire, 0, reg, value);
    ms_write_ddrc_reg(memshire, 1, reg, value);
    return 0;
}

//
// DDR Phy initialization for 1067 Mhz clock (without training).
//
uint8_t ms_init_ddr_phy_1067(uint8_t memshire)
{
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TxSlewRate_b0_p0, 0x5ff);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TxSlewRate_b1_p0, 0x5ff);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TxSlewRate_b0_p0, 0x5ff);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TxSlewRate_b1_p0, 0x5ff);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TxSlewRate_b0_p0, 0x5ff);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TxSlewRate_b1_p0, 0x5ff);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TxSlewRate_b0_p0, 0x5ff);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TxSlewRate_b1_p0, 0x5ff);
    ms_write_ddrc_reg(memshire, 2, ANIB0_ATxSlewRate, 0x1ff);
    ms_write_ddrc_reg(memshire, 2, ANIB1_ATxSlewRate, 0x1ff);
    ms_write_ddrc_reg(memshire, 2, ANIB2_ATxSlewRate, 0x1ff);
    ms_write_ddrc_reg(memshire, 2, ANIB3_ATxSlewRate, 0x1ff);
    ms_write_ddrc_reg(memshire, 2, ANIB4_ATxSlewRate, 0x1ff);
    ms_write_ddrc_reg(memshire, 2, ANIB5_ATxSlewRate, 0x1ff);
    ms_write_ddrc_reg(memshire, 2, MASTER0_PllCtrl2_p0, 0x19);
    ms_write_ddrc_reg(memshire, 2, MASTER0_ARdPtrInitVal_p0, 0x2);
    ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BGPR4_p0, 0x0);
    ms_write_ddrc_reg(memshire, 2, MASTER0_DqsPreambleControl_p0, 0x1e3);
    ms_write_ddrc_reg(memshire, 2, MASTER0_DbyteDllModeCntrl, 0x2);
    ms_write_ddrc_reg(memshire, 2, MASTER0_DllLockParam_p0, 0x212);
    ms_write_ddrc_reg(memshire, 2, MASTER0_DllGainCtl_p0, 0x61);
    ms_write_ddrc_reg(memshire, 2, MASTER0_ProcOdtTimeCtl_p0, 0x3);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TxOdtDrvStren_b0_p0, 0x600);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TxOdtDrvStren_b1_p0, 0x600);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TxOdtDrvStren_b0_p0, 0x600);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TxOdtDrvStren_b1_p0, 0x600);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TxOdtDrvStren_b0_p0, 0x600);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TxOdtDrvStren_b1_p0, 0x600);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TxOdtDrvStren_b0_p0, 0x600);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TxOdtDrvStren_b1_p0, 0x600);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TxImpedanceCtrl1_b0_p0, 0x618);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TxImpedanceCtrl1_b1_p0, 0x618);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TxImpedanceCtrl1_b0_p0, 0x618);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TxImpedanceCtrl1_b1_p0, 0x618);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TxImpedanceCtrl1_b0_p0, 0x618);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TxImpedanceCtrl1_b1_p0, 0x618);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TxImpedanceCtrl1_b0_p0, 0x618);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TxImpedanceCtrl1_b1_p0, 0x618);
    ms_write_ddrc_reg(memshire, 2, ANIB0_ATxImpedance, 0x3ff);
    ms_write_ddrc_reg(memshire, 2, ANIB1_ATxImpedance, 0x3ff);
    ms_write_ddrc_reg(memshire, 2, ANIB2_ATxImpedance, 0x3ff);
    ms_write_ddrc_reg(memshire, 2, ANIB3_ATxImpedance, 0x3ff);
    ms_write_ddrc_reg(memshire, 2, ANIB4_ATxImpedance, 0x3ff);
    ms_write_ddrc_reg(memshire, 2, ANIB5_ATxImpedance, 0x3ff);
    ms_write_ddrc_reg(memshire, 2, MASTER0_DfiMode, 0x3);
    ms_write_ddrc_reg(memshire, 2, MASTER0_DfiCAMode, 0x4);
    ms_write_ddrc_reg(memshire, 2, MASTER0_CalDrvStr0, 0x0);
    ms_write_ddrc_reg(memshire, 2, MASTER0_CalUclkInfo_p0, 0x42b);
    ms_write_ddrc_reg(memshire, 2, MASTER0_CalRate, 0x9);
    ms_write_ddrc_reg(memshire, 2, MASTER0_VrefInGlobal_p0, 0x104);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_DqDqsRcvCntrl_b0_p0, 0x5a1);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_DqDqsRcvCntrl_b1_p0, 0x5a1);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_DqDqsRcvCntrl_b0_p0, 0x5a1);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_DqDqsRcvCntrl_b1_p0, 0x5a1);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_DqDqsRcvCntrl_b0_p0, 0x5a1);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_DqDqsRcvCntrl_b1_p0, 0x5a1);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_DqDqsRcvCntrl_b0_p0, 0x5a1);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_DqDqsRcvCntrl_b1_p0, 0x5a1);
    ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqRatio_p0, 0x1);
    ms_write_ddrc_reg(memshire, 2, MASTER0_TristateModeCA_p0, 0x1);
    ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat0, 0x0);
    ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat1, 0x0);
    ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat2, 0x4444);
    ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat3, 0x8888);
    ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat4, 0x5555);
    ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat5, 0x0);
    ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat6, 0x0);
    ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat7, 0xf000);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_DqDqsRcvCntrl1, 0x500);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_DqDqsRcvCntrl1, 0x500);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_DqDqsRcvCntrl1, 0x500);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_DqDqsRcvCntrl1, 0x500);
    ms_write_ddrc_reg(memshire, 2, MASTER0_MasterX4Config, 0x0);
    ms_write_ddrc_reg(memshire, 2, MASTER0_DMIPinPresent_p0, 0x0);
    ms_write_ddrc_reg(memshire, 2, MASTER0_Acx4AnibDis, 0x0);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_DFIMRL_p0, 0x8);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_DFIMRL_p0, 0x8);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_DFIMRL_p0, 0x8);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_DFIMRL_p0, 0x8);
    ms_write_ddrc_reg(memshire, 2, MASTER0_HwtMRL_p0, 0x8);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqsDlyTg0_u0_p0, 0x100);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqsDlyTg0_u1_p0, 0x100);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqsDlyTg0_u0_p0, 0x100);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqsDlyTg0_u1_p0, 0x100);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqsDlyTg0_u0_p0, 0x100);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqsDlyTg0_u1_p0, 0x100);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqsDlyTg0_u0_p0, 0x100);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqsDlyTg0_u1_p0, 0x100);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r0_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r1_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r2_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r3_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r4_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r5_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r6_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r7_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r8_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r0_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r1_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r2_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r3_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r4_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r5_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r6_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r7_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r8_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r0_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r1_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r2_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r3_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r4_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r5_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r6_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r7_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r8_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r0_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r1_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r2_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r3_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r4_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r5_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r6_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r7_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r8_p0, 0x87);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_RxEnDlyTg0_u0_p0, 0x4d6);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_RxEnDlyTg0_u1_p0, 0x4d6);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_RxEnDlyTg0_u0_p0, 0x4d6);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_RxEnDlyTg0_u1_p0, 0x4d6);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_RxEnDlyTg0_u0_p0, 0x4d6);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_RxEnDlyTg0_u1_p0, 0x4d6);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_RxEnDlyTg0_u0_p0, 0x4d6);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_RxEnDlyTg0_u1_p0, 0x4d6);
    ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BGPR1_p0, 0x2200);
    ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BGPR2_p0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BGPR3_p0, 0x2e00);
    ms_write_ddrc_reg(memshire, 2, MASTER0_HwtLpCsEnA, 0x1);
    ms_write_ddrc_reg(memshire, 2, MASTER0_HwtLpCsEnB, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_PptDqsCntInvTrnTg0_p0, 0x37);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_PptDqsCntInvTrnTg0_p0, 0x37);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_PptDqsCntInvTrnTg0_p0, 0x37);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_PptDqsCntInvTrnTg0_p0, 0x37);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_PptDqsCntInvTrnTg1_p0, 0x37);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_PptDqsCntInvTrnTg1_p0, 0x37);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_PptDqsCntInvTrnTg1_p0, 0x37);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_PptDqsCntInvTrnTg1_p0, 0x37);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_PptCtlStatic, 0x501);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_PptCtlStatic, 0x50d);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_PptCtlStatic, 0x501);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_PptCtlStatic, 0x50d);
    ms_write_ddrc_reg(memshire, 2, MASTER0_HwtCAMode, 0x34);
    ms_write_ddrc_reg(memshire, 2, MASTER0_DllGainCtl_p0, 0x54);
    ms_write_ddrc_reg(memshire, 2, MASTER0_DllLockParam_p0, 0x2f2);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCtrl23, 0x10f);
    ms_write_ddrc_reg(memshire, 2, MASTER0_PllCtrl3, 0x61f0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_PhyInLP3, 0x0);
    ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s0, 0x10);
    ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s1, 0x400);
    ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s2, 0x10e);
    ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s1, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s2, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s0, 0xb);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s1, 0x480);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s1, 0x448);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s2, 0x139);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s1, 0x478);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s1, 0xe8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s0, 0x2);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s1, 0x10);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s2, 0x139);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s0, 0xb);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s1, 0x7c0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s2, 0x139);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s0, 0x44);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s1, 0x633);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s2, 0x159);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s0, 0x14f);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s1, 0x630);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s2, 0x159);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s0, 0x47);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s1, 0x633);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s2, 0x149);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s0, 0x4f);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s1, 0x633);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s2, 0x179);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s1, 0xe0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s1, 0x7c8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s1, 0x1);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s2, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s1, 0x45a);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s2, 0x9);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s1, 0x448);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s0, 0x40);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s1, 0x633);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s2, 0x179);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s0, 0x1);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s1, 0x618);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s0, 0x40c0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s1, 0x633);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s2, 0x149);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s1, 0x4);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s2, 0x48);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s0, 0x4040);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s1, 0x633);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s2, 0x149);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s1, 0x4);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s2, 0x48);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s0, 0x40);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s1, 0x633);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s2, 0x149);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s0, 0x10);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s1, 0x4);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s2, 0x18);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s1, 0x4);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s2, 0x78);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s0, 0x549);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s1, 0x633);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s2, 0x159);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s0, 0xd49);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s1, 0x633);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s2, 0x159);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s0, 0x94a);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s1, 0x633);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s2, 0x159);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s0, 0x441);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s1, 0x633);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s2, 0x149);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s0, 0x42);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s1, 0x633);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s2, 0x149);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s0, 0x1);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s1, 0x633);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s2, 0x149);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s1, 0xe0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s0, 0xa);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s1, 0x10);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s0, 0x9);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s1, 0x3c0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s2, 0x149);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s0, 0x9);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s1, 0x3c0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s2, 0x159);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s0, 0x18);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s1, 0x10);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s1, 0x3c0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s0, 0x18);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s1, 0x4);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s2, 0x48);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s0, 0x18);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s1, 0x4);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s2, 0x58);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s0, 0xb);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s1, 0x10);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s0, 0x1);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s1, 0x10);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s0, 0x5);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s1, 0x7c0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x0, 0x811);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x0, 0x880);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x0, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x0, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x1, 0x4008);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x1, 0x83);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x1, 0x4f);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x1, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x2, 0x4040);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x2, 0x83);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x2, 0x51);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x2, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x3, 0x811);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x3, 0x880);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x3, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x3, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x4, 0x720);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x4, 0xf);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x4, 0x1740);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x4, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x5, 0x16);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x5, 0x83);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x5, 0x4b);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x5, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x6, 0x716);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x6, 0xf);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x6, 0x2001);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x6, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x7, 0x716);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x7, 0xf);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x7, 0x2800);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x7, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x8, 0x716);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x8, 0xf);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x8, 0xf00);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x8, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x9, 0x720);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x9, 0xf);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x9, 0x1400);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x9, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x10, 0xe08);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x10, 0xc15);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x10, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x10, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x11, 0x625);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x11, 0x15);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x11, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x11, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x12, 0x4028);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x12, 0x80);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x12, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x12, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x13, 0xe08);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x13, 0xc1a);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x13, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x13, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x14, 0x625);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x14, 0x1a);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x14, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x14, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x15, 0x4040);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x15, 0x80);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x15, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x15, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x16, 0x2604);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x16, 0x15);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x16, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x16, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x17, 0x708);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x17, 0x5);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x17, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x17, 0x2002);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x18, 0x8);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x18, 0x80);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x18, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x18, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x19, 0x2604);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x19, 0x1a);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x19, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x19, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x20, 0x708);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x20, 0xa);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x20, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x20, 0x2002);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x21, 0x4040);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x21, 0x80);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x21, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x21, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x22, 0x60a);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x22, 0x15);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x22, 0x1200);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x22, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x23, 0x61a);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x23, 0x15);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x23, 0x1300);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x23, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x24, 0x60a);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x24, 0x1a);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x24, 0x1200);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x24, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x25, 0x642);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x25, 0x1a);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x25, 0x1300);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x25, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x26, 0x4808);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x26, 0x880);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x26, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x26, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s1, 0x790);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s2, 0x11a);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s1, 0x7aa);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s2, 0x2a);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s0, 0x10);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s1, 0x7b2);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s2, 0x2a);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s1, 0x7c8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s0, 0x10);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s1, 0x10);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s0, 0x10);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s1, 0x2a8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s2, 0x129);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s1, 0x370);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s2, 0x129);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s0, 0xa);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s1, 0x3c8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s2, 0x1a9);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s0, 0xc);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s1, 0x408);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s2, 0x199);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s0, 0x14);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s1, 0x790);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s2, 0x11a);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s1, 0x4);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s2, 0x18);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s0, 0xe);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s1, 0x408);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s2, 0x199);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s1, 0x8568);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s2, 0x108);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s0, 0x18);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s1, 0x790);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s2, 0x16a);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s1, 0x1d8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s2, 0x169);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s0, 0x10);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s1, 0x8558);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s2, 0x168);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s0, 0x70);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s1, 0x788);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s2, 0x16a);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s0, 0x1ff8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s1, 0x85a8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s2, 0x1e8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s0, 0x50);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s1, 0x798);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s2, 0x16a);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s0, 0x60);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s1, 0x7a0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s2, 0x16a);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s1, 0x8310);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s2, 0x168);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s1, 0xa310);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s2, 0x168);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s0, 0xa);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s1, 0x408);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s2, 0x169);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s0, 0x6e);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s1, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s2, 0x68);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s1, 0x408);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s2, 0x169);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s1, 0x8310);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s2, 0x168);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s1, 0xa310);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s2, 0x168);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s0, 0x1ff8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s1, 0x85a8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s2, 0x1e8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s0, 0x68);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s1, 0x798);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s2, 0x16a);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s0, 0x78);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s1, 0x7a0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s2, 0x16a);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s0, 0x68);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s1, 0x790);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s2, 0x16a);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s1, 0x8b10);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s2, 0x168);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s1, 0xab10);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s2, 0x168);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s0, 0xa);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s1, 0x408);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s2, 0x169);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s0, 0x58);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s1, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s2, 0x68);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s1, 0x408);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s2, 0x169);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s1, 0x8b10);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s2, 0x168);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s0, 0x1);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s1, 0xab10);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s2, 0x168);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s1, 0x1d8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s2, 0x169);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s0, 0x80);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s1, 0x790);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s2, 0x16a);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s0, 0x18);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s1, 0x7aa);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s2, 0x6a);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s0, 0xa);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s1, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s2, 0x1e9);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s1, 0x8080);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s2, 0x108);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s0, 0xf);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s1, 0x408);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s2, 0x169);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s0, 0xc);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s1, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s2, 0x68);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s0, 0x9);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s1, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s2, 0x1a9);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s1, 0x408);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s2, 0x169);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s1, 0x8080);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s2, 0x108);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s1, 0x7aa);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s2, 0x6a);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s1, 0x8568);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s2, 0x108);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s0, 0xb7);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s1, 0x790);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s2, 0x16a);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s0, 0x1f);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s1, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s2, 0x68);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s1, 0x8558);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s2, 0x168);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s0, 0xf);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s1, 0x408);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s2, 0x169);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s0, 0xd);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s1, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s2, 0x68);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s1, 0x408);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s2, 0x169);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s1, 0x8558);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s2, 0x168);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s1, 0x3c8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s2, 0x1a9);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s0, 0x3);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s1, 0x370);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s2, 0x129);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s0, 0x20);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s1, 0x2aa);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s2, 0x9);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s1, 0x400);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s2, 0x10e);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s1, 0xe8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s1, 0x8140);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s2, 0x10c);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s0, 0x10);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s1, 0x8138);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s2, 0x10c);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s1, 0x7c8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s2, 0x101);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s1, 0x448);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s0, 0xf);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s1, 0x7c0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s1, 0xe8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s0, 0x47);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s1, 0x630);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s1, 0x618);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s1, 0xe0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s1, 0x7c8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s1, 0x8140);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s2, 0x10c);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s1, 0x478);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s2, 0x109);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b115s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b115s1, 0x1);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b115s2, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b116s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b116s1, 0x4);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b116s2, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b117s0, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b117s1, 0x7c8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b117s2, 0x101);
    ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s1, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s2, 0x8);
    ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s1, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s2, 0x0);
    ms_write_ddrc_reg(memshire, 2, APBONLY0_SequencerOverride, 0x400);
    ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b8, 0x29);
    ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b15, 0x6a);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl0, 0x0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl1, 0x101);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl2, 0x105);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl3, 0x107);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl4, 0x10f);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl5, 0x202);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl6, 0x20a);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl7, 0x20b);
    ms_write_ddrc_reg(memshire, 2, MASTER0_DbyteDllModeCntrl, 0x2);
    ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY0_p0, 0x85);
    ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY1_p0, 0x10a);
    ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY2_p0, 0xa6b);
    ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY3_p0, 0x2c);
    ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag0, 0x0);
    ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag1, 0x173);
    ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag2, 0x60);
    ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag3, 0x6110);
    ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag4, 0x2152);
    ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag5, 0xdfbd);
    ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag6, 0xffff);
    ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag7, 0x6152);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x0_p0, 0xe0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x0_p0, 0x12);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x1_p0, 0xe0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x1_p0, 0x12);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x2_p0, 0xe0);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x2_p0, 0x12);
    ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCtrl13, 0xf);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte1, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte2, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte3, 0x180);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte5, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_TrainingParam, 0x6209);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm0_i0, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i1, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i2, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i3, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i4, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i5, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i6, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i7, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i8, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte1, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte2, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte3, 0x180);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte5, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_TrainingParam, 0x6209);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm0_i0, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i1, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i2, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i3, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i4, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i5, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i6, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i7, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i8, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte1, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte2, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte3, 0x180);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte5, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_TrainingParam, 0x6209);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm0_i0, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i1, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i2, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i3, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i4, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i5, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i6, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i7, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i8, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte1, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte2, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte3, 0x180);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte5, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_TrainingParam, 0x6209);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm0_i0, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i1, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i2, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i3, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i4, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i5, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i6, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i7, 0x1);
    ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i8, 0x1);
    ms_write_ddrc_reg(memshire, 2, MASTER0_CalZap, 0x1);
    ms_write_ddrc_reg(memshire, 2, MASTER0_CalRate, 0x19);
    ms_write_ddrc_reg(memshire, 2, DRTUB0_UcclkHclkEnables, 0x2);
    ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x1);

    return 0;
}

//
// Phase 1 of the DDR initialization sequence.
//
uint8_t ms_init_seq_phase1(uint8_t memshire, uint8_t config_ecc, uint8_t config_real_pll)
{
    //
    // Start to initialize the memory controllers
    //
    ms_write_esr(memshire, ddrc_reset_ctl, 0x10d);
    ms_read_esr(memshire, ddrc_reset_ctl);

    ms_write_both_ddrc_reg(memshire, DBG1, 0x00000001);
    ms_write_both_ddrc_reg(memshire, PWRCTL, 0x00000001);

    //
    // Make sure operating mode is Init
    //
    ms_poll_ddrc_reg(memshire, 0, STAT, 0x0, 0x7, 10);
    ms_poll_ddrc_reg(memshire, 1, STAT, 0x0, 0x7, 10);

    //
    // Configure the memory controllers
    //
    ms_write_both_ddrc_reg(memshire, MSTR, 0x80080020); // set device config to x16 parts
    ms_write_both_ddrc_reg(memshire, MRCTRL0, 0x0000a010);
    ms_write_both_ddrc_reg(memshire, MRCTRL1, 0x0001008c);
    ms_write_both_ddrc_reg(memshire, DERATEEN, 0x00000404);
    ms_write_both_ddrc_reg(memshire, DERATEINT, 0xc0c188dd);
    ms_write_both_ddrc_reg(memshire, DERATECTL, 0x00000000);
    ms_write_both_ddrc_reg(memshire, PWRCTL, 0x00000000);
    ms_write_both_ddrc_reg(memshire, PWRTMG, 0x0040d104);
    ms_write_both_ddrc_reg(memshire, HWLPCTL, 0x00af0003);
    ms_write_both_ddrc_reg(memshire, RFSHCTL0, 0x00210000);
    ms_write_both_ddrc_reg(memshire, RFSHCTL3, 0x00000001); // dis_auto_refresh = 1
    ms_write_both_ddrc_reg(memshire, RFSHTMG, 0x0082008c);
    ms_write_both_ddrc_reg(memshire, RFSHTMG1, 0x00410000);
    if (config_ecc) {
        ms_write_both_ddrc_reg(memshire, ECCCFG0, 0x003f7f14); // enable ECC
    } else {
        ms_write_both_ddrc_reg(memshire, ECCCFG0, 0x003f7f10); // disable ECC
    }
    ms_write_both_ddrc_reg(memshire, ECCCFG1, 0x000007b2);
    ms_write_both_ddrc_reg(memshire, ECCCTL, 0x00000300);
    ms_write_both_ddrc_reg(memshire, ECCPOISONADDR0, 0x000002b4);
    ms_write_both_ddrc_reg(memshire, ECCPOISONADDR1, 0x0001df60);
    ms_write_both_ddrc_reg(memshire, CRCPARCTL0, 0x00000000);
    ms_write_both_ddrc_reg(memshire, INIT0, 0x00020002);
    ms_write_both_ddrc_reg(memshire, INIT1, 0x00020002);
    ms_write_both_ddrc_reg(memshire, INIT2, 0x0000f405);
    ms_write_both_ddrc_reg(memshire, INIT3, 0x0074007f);
    ms_write_both_ddrc_reg(memshire, INIT4, 0x00330000);
    ms_write_both_ddrc_reg(memshire, INIT5, 0x0005000c);
    ms_write_both_ddrc_reg(memshire, INIT6, 0x0000004d);
    ms_write_both_ddrc_reg(memshire, INIT7, 0x0000004d);
    ms_write_both_ddrc_reg(memshire, DIMMCTL, 0x00000000);
    ms_write_both_ddrc_reg(memshire, DRAMTMG0, 0x2921242d); // set TRasMax = 36 due to 2:1 clock
    ms_write_both_ddrc_reg(memshire, DRAMTMG1, 0x00090901);
    ms_write_both_ddrc_reg(memshire, DRAMTMG2, 0x11120a21);
    ms_write_both_ddrc_reg(memshire, DRAMTMG3, 0x00f0f000);
    ms_write_both_ddrc_reg(memshire, DRAMTMG4, 0x14040914);
    ms_write_both_ddrc_reg(memshire, DRAMTMG5, 0x02061111);
    ms_write_both_ddrc_reg(memshire, DRAMTMG6, 0x0101000a);
    ms_write_both_ddrc_reg(memshire, DRAMTMG7, 0x00000602);
    ms_write_both_ddrc_reg(memshire, DRAMTMG8, 0x00000101);
    ms_write_both_ddrc_reg(memshire, DRAMTMG12, 0x00020000);
    ms_write_both_ddrc_reg(memshire, DRAMTMG13, 0x16100002);
    ms_write_both_ddrc_reg(memshire, DRAMTMG14, 0x00000093);
    ms_write_both_ddrc_reg(memshire, ZQCTL0, 0xc42f0021);
    ms_write_both_ddrc_reg(memshire, ZQCTL1, 0x036a3055);
    ms_write_both_ddrc_reg(memshire, ZQCTL2, 0x00000000);
    ms_write_both_ddrc_reg(memshire, DFITMG0, 0x049f821e);
    ms_write_both_ddrc_reg(memshire, DFITMG1, 0x00090303);
    ms_write_both_ddrc_reg(memshire, DFILPCFG0, 0x0321b010);
    ms_write_both_ddrc_reg(memshire, DFIUPD0, 0x80400018);
    ms_write_both_ddrc_reg(memshire, DFIUPD1, 0x003c00b1);
    ms_write_both_ddrc_reg(memshire, DFIUPD2, 0x80000000);
    ms_write_both_ddrc_reg(memshire, DFIMISC, 0x00000081);
    ms_write_both_ddrc_reg(memshire, DFITMG2, 0x00001f1e);
    ms_write_both_ddrc_reg(memshire, DBICTL, 0x00000001);
    ms_write_both_ddrc_reg(memshire, DFIPHYMSTR, 0x00000000);
    ms_write_both_ddrc_reg(memshire, ADDRMAP1, 0x00030303); // 1/2 cache line same bank
    ms_write_both_ddrc_reg(memshire, ADDRMAP2, 0x03000000); // 1/2 cache line same bank
    ms_write_both_ddrc_reg(memshire, ADDRMAP4, 0x00001f1f);

    if (config_ecc) {
        ms_write_both_ddrc_reg(memshire, ADDRMAP3, 0x11111103); // For smallest LPDDR4x part
        ms_write_both_ddrc_reg(memshire, ADDRMAP5, 0x04040404);
        ms_write_both_ddrc_reg(memshire, ADDRMAP6, 0x0f0f0404); // For smallest LPDDR4x part
        ms_write_both_ddrc_reg(memshire, ADDRMAP7, 0x00000f0f); // For smallest LPDDR4x part
    } else {
        ms_write_both_ddrc_reg(memshire, ADDRMAP3, 0x03030303); // 1/2 cache line same bank
        ms_write_both_ddrc_reg(memshire, ADDRMAP5, 0x07070707);
        ms_write_both_ddrc_reg(
            memshire, ADDRMAP6,
            0x07070707); // assume addressing for largest LPDDR4x part is okay always
        ms_write_both_ddrc_reg(
            memshire, ADDRMAP7,
            0x00000f07); // assume addressing for largest LPDDR4x part is okay always
    }
    ms_write_both_ddrc_reg(memshire, ADDRMAP9, 0x07070707); // unused
    ms_write_both_ddrc_reg(memshire, ADDRMAP10, 0x07070707); // unused
    ms_write_both_ddrc_reg(memshire, ADDRMAP11, 0x001f1f07); // unused
    ms_write_both_ddrc_reg(memshire, ODTCFG, 0x061a0c1c);
    ms_write_both_ddrc_reg(memshire, ODTMAP, 0x00000000);
    ms_write_both_ddrc_reg(memshire, SCHED, 0x00a01f01);
    ms_write_both_ddrc_reg(memshire, SCHED1, 0x00000000);
    ms_write_both_ddrc_reg(memshire, PERFHPR1, 0x0f000001);
    ms_write_both_ddrc_reg(memshire, PERFLPR1, 0x0f00007f);
    ms_write_both_ddrc_reg(memshire, PERFWR1, 0x0f00007f);
    ms_write_both_ddrc_reg(memshire, DBG0, 0x00000000);
    ms_write_both_ddrc_reg(memshire, DBG1, 0x00000000);
    ms_write_both_ddrc_reg(memshire, DBGCMD, 0x00000000);
    ms_write_both_ddrc_reg(memshire, SWCTL, 0x00000001);
    ms_write_both_ddrc_reg(memshire, POISONCFG, 0x00000010);
    ms_write_both_ddrc_reg(memshire, ADVECCINDEX, 0x000001d2);
    ms_write_both_ddrc_reg(memshire, ECCPOISONPAT0, 0x00000000);
    ms_write_both_ddrc_reg(memshire, ECCPOISONPAT2, 0x00000000);
    ms_write_both_ddrc_reg(memshire, PCTRL_0, 0x00000001);
    ms_write_both_ddrc_reg(memshire, PCTRL_1, 0x00000001);

    //
    // Make sure STAT == 0
    //
    ms_poll_ddrc_reg(memshire, 0, STAT, 0x0, 0xffffffff, 10);
    ms_poll_ddrc_reg(memshire, 1, STAT, 0x0, 0xffffffff, 10);

    ms_write_both_ddrc_reg(memshire, PCCFG, 0x00000000);
    ms_write_both_ddrc_reg(memshire, PCFGR_0, 0x0000400f);
    ms_write_both_ddrc_reg(memshire, PCFGR_1, 0x0000400f);
    ms_write_both_ddrc_reg(memshire, PCFGR_0, 0x0000500f);
    ms_write_both_ddrc_reg(memshire, PCFGR_1, 0x0000500f);
    ms_write_both_ddrc_reg(memshire, PCFGR_0, 0x0000500f);
    ms_write_both_ddrc_reg(memshire, PCFGR_1, 0x0000500f);
    ms_write_both_ddrc_reg(memshire, PCFGR_0, 0x0000100f);
    ms_write_both_ddrc_reg(memshire, PCFGR_1, 0x0000100f);
    ms_write_both_ddrc_reg(memshire, PCFGW_0, 0x0000400f);
    ms_write_both_ddrc_reg(memshire, PCFGW_1, 0x0000400f);
    ms_write_both_ddrc_reg(memshire, PCFGW_0, 0x0000500f);
    ms_write_both_ddrc_reg(memshire, PCFGW_1, 0x0000500f);
    ms_write_both_ddrc_reg(memshire, PCFGW_0, 0x0000500f);
    ms_write_both_ddrc_reg(memshire, PCFGW_1, 0x0000500f);
    ms_write_both_ddrc_reg(memshire, PCFGW_0, 0x0000100f);
    ms_write_both_ddrc_reg(memshire, PCFGW_1, 0x0000100f);
    ms_write_both_ddrc_reg(memshire, DBG1, 0x00000000);

    //
    // make power control reg can be written
    // FIXME tberg: is this needed?
    //
    ms_poll_ddrc_reg(memshire, 0, PWRCTL, 0, 0x1ff, 1);
    ms_poll_ddrc_reg(memshire, 0, PWRCTL, 0, 0x1ff, 1);

    ms_write_both_ddrc_reg(memshire, PWRCTL, 0x00000000);
    ms_poll_ddrc_reg(memshire, 0, PWRCTL, 0, 0x1ff, 1);
    ms_poll_ddrc_reg(memshire, 0, PWRCTL, 0, 0x1ff, 1);

    ms_write_both_ddrc_reg(memshire, PWRCTL, 0x00000000); // powerdown_en = 0, selfref_en = 0
    ms_write_both_ddrc_reg(memshire, SWCTL, 0x00000000); // sw_done = 0
    ms_write_both_ddrc_reg(memshire, DFIMISC, 0x00000080); // dfi_init_complete_en = 0
    ms_write_both_ddrc_reg(memshire, DFIMISC, 0x00001080); // ???

    //
    // These are probably optional, but good to make sure
    // the memory controllers are in the expected state
    //
    ms_poll_ddrc_reg(memshire, 0, INIT0, 0x00020002, 0xffffffff, 1);
    ms_poll_ddrc_reg(memshire, 0, DBICTL, 0x00000001, 0xffffffff, 1);
    ms_poll_ddrc_reg(memshire, 0, MSTR, 0x00080020, 0xffffffff, 1);
    ms_poll_ddrc_reg(memshire, 0, INIT3, 0x0074007f, 0xffffffff, 1);
    ms_poll_ddrc_reg(memshire, 0, INIT4, 0x00330000, 0xffffffff, 1);
    ms_poll_ddrc_reg(memshire, 0, INIT6, 0x0000004d, 0xffffffff, 1);

    ms_poll_ddrc_reg(memshire, 1, INIT0, 0x00020002, 0xffffffff, 1);
    ms_poll_ddrc_reg(memshire, 1, DBICTL, 0x00000001, 0xffffffff, 1);
    ms_poll_ddrc_reg(memshire, 1, MSTR, 0x00080020, 0xffffffff, 1);
    ms_poll_ddrc_reg(memshire, 1, INIT3, 0x0074007f, 0xffffffff, 1);
    ms_poll_ddrc_reg(memshire, 1, INIT4, 0x00330000, 0xffffffff, 1);
    ms_poll_ddrc_reg(memshire, 1, INIT6, 0x0000004d, 0xffffffff, 1);
    //
    // Turn off core and axi resets
    //
    if (config_real_pll) {
        ms_write_esr(memshire, ddrc_reset_ctl, 0x10c);
    } else {
        ms_write_esr(memshire, ddrc_reset_ctl, 0x00c);
    }

    return 0;
}

//
// Phase 2 of the DDR initialization sequence.
//
uint8_t ms_init_seq_phase2(uint8_t memshire, uint8_t config_real_pll)
{
    //
    // Turn off pub reset
    //
    if (config_real_pll) {
        ms_write_esr(memshire, ddrc_reset_ctl, 0x108);
    } else {
        ms_write_esr(memshire, ddrc_reset_ctl, 0x008);
    }

    return 0;
}

//
// Phase 3 of the DDR initialization sequence.
//
uint8_t ms_init_seq_phase3(uint8_t memshire)
{
    ms_init_ddr_phy_1067(memshire);
    ms_write_both_ddrc_reg(memshire, DFIMISC, 0x000010a0);
    return 0;
}

//
// Phase 4 of the DDR initialization sequence.
//
uint8_t ms_init_seq_phase4(uint8_t memshire)
{
    //
    // Wait for init complete
    //
    ms_poll_ddrc_reg(memshire, 0, DFISTAT, 0x1, 0x1, 1000);
    ms_poll_ddrc_reg(memshire, 1, DFISTAT, 0x1, 0x1, 1000);

    ms_write_both_ddrc_reg(memshire, DFIMISC, 0x00001080);
    ms_write_both_ddrc_reg(memshire, DFIMISC, 0x00001081);
    ms_write_both_ddrc_reg(memshire, DFIMISC, 0x00001081);
    ms_write_both_ddrc_reg(memshire, SWCTL, 0x00000001);

    ms_poll_ddrc_reg(memshire, 0, SWSTAT, 0x1, 0x1, 10);
    ms_poll_ddrc_reg(memshire, 1, SWSTAT, 0x1, 0x1, 10);
    ms_poll_ddrc_reg(memshire, 0, STAT, 0x1, 0x1, 1000);
    ms_poll_ddrc_reg(memshire, 1, STAT, 0x1, 0x1, 1000);

    ms_write_both_ddrc_reg(memshire, PWRCTL, 0x00000000);
    ms_write_both_ddrc_reg(memshire, PCTRL_0, 0x00000001);
    ms_write_both_ddrc_reg(memshire, PCTRL_1, 0x00000001);
    return 0;
}

void ddr_init(uint8_t memshire_id)
{
    // Args: Mem ID, Enable ECC, Real PLL
    ms_init_seq_phase1(memshire_id, 0, 1);
    // Args: Mem ID, Real PLL
    ms_init_seq_phase2(memshire_id, 1);
    ms_init_seq_phase3(memshire_id);
    ms_init_seq_phase4(memshire_id);
}

int ddr_config(void)
{
    // configure the dram controllers and train the memory

    for (uint8_t memshire_id = 0; memshire_id < 8; memshire_id++)
        ddr_init(memshire_id);

    return 0;

}

static struct ddr_event_control_block  event_control_block __attribute__((section(".data")));

int32_t ddr_error_control_init(dm_event_isr_callback event_cb)
{
    event_control_block.event_cb = event_cb;
    return  0;
}
int32_t ddr_error_control_deinit(void)
{
    return  0;
}

int32_t ddr_enable_uce_interrupt(void)
{
    return  0;
}

int32_t ddr_disable_ce_interrupt(void)
{
    return  0;
}

int32_t ddr_disable_uce_interrupt(void)
{
    return  0;
}

int32_t ddr_set_ce_threshold(uint32_t ce_threshold)
{
    (void)ce_threshold;
    return  0;
}

int32_t ddr_get_ce_count(uint32_t *ce_count)
{
    *ce_count = 0;
    return  0;
}

int32_t ddr_get_uce_count(uint32_t *uce_count)
{
    *uce_count = 0;
    return  0;
}

void ddr_error_threshold_isr(void)
{

}


