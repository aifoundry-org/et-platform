/* Copyright (c) 2025 Ainekko, Co. */
/* SPDX-License-Identifier: Apache-2.0 */
/*                                                                         */
// From commit c7d8be73042b8b85bfb9ab77352864920bcfc00c in esperanto-soc repository
// START OF AUTO-GENERATED CODE
//#################################################################################


//
// DDR Phy initialization for 1067 Mhz clock (before training).
//
uint32_t ms_init_ddr_phy_1067_pre (uint32_t memshire) {
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB0_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB1_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB2_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB3_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB4_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB5_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllCtrl2_p0, 0x19);                    // PllFreqSel=0x19
   ms_write_ddrc_reg(memshire, 2, MASTER0_ARdPtrInitVal_p0, 0x2);                // ARdPtrInitVal_p0=2
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BGPR4_p0, 0x0);                   // Seq0BGPR4_p0=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DqsPreambleControl_p0, 0x1a3);         // WDQSEXTENSION=1, LP4SttcPreBridgeRxEn=1,
                                                                                 // LP4PostambleExt=0, LP4TglTwoTckTxDqsPre=1,
                                                                                 // PositionDfeInit=0, TwoTckTxDqsPre=1,
                                                                                 // TwoTckRxDqsPre=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DbyteDllModeCntrl, 0x2);               // DllRxPreambleMode=1, Reserved=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DllLockParam_p0, 0x212);               // RFU_DllLockParam_p0=0x212
   ms_write_ddrc_reg(memshire, 2, MASTER0_DllGainCtl_p0, 0x61);                  // DllSeedSel=0, DllGainTV=6, DllGainIV=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_ProcOdtTimeCtl_p0, 0x3);               // POdtTailWidthExt=0, POdtStartDelay=0,
                                                                                 // POdtTailWidth=3
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, ANIB0_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB1_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB2_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB3_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB4_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB5_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiMode, 0x3);                         // Dfi1Override=0, Dfi1Enable=1, Dfi0Enable=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiCAMode, 0x4);                       // DfiD4AltCAMode=0, DfiLp4CAMode=1, DfiD4CAMode=0,
                                                                                 // DfiLp3CAMode=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalDrvStr0, 0x0);                      // CalDrvStrPu50=0, CalDrvStrPd50=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalVRefs, 0x2);                        // CalVRefs=2
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalUclkInfo_p0, 0x42b);                // CalUClkTicksPer1uS=0x42b
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalRate, 0x9);                         // DisableBackgroundZQUpdates=0, CalOnce=0,
                                                                                 // CalRun=0, CalInterval=9
   ms_write_ddrc_reg(memshire, 2, MASTER0_VrefInGlobal_p0, 0x32c);               // GlobalVrefInMode=0, GlobalVrefInTrim=0,
                                                                                 // GlobalVrefInDAC=0x65, GlobalVrefInSel=4
   ms_write_ddrc_reg(memshire, 2, DBYTE0_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqRatio_p0, 0x1);                 // DfiFreqRatio_p0=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_TristateModeCA_p0, 0x1);               // CkDisVal=0, DDR2TMode=0, DisDynAdrTri=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat0, 0x0);                    // DfiFreqXlatVal3=0, DfiFreqXlatVal2=0,
                                                                                 // DfiFreqXlatVal1=0, DfiFreqXlatVal0=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat1, 0x0);                    // DfiFreqXlatVal7=0, DfiFreqXlatVal6=0,
                                                                                 // DfiFreqXlatVal5=0, DfiFreqXlatVal4=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat2, 0x4444);                 // DfiFreqXlatVal11=4, DfiFreqXlatVal10=4,
                                                                                 // DfiFreqXlatVal9=4, DfiFreqXlatVal8=4
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat3, 0x8888);                 // DfiFreqXlatVal15=8, DfiFreqXlatVal14=8,
                                                                                 // DfiFreqXlatVal13=8, DfiFreqXlatVal12=8
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat4, 0x5555);                 // DfiFreqXlatVal19=5, DfiFreqXlatVal18=5,
                                                                                 // DfiFreqXlatVal17=5, DfiFreqXlatVal16=5
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat5, 0x0);                    // DfiFreqXlatVal23=0, DfiFreqXlatVal22=0,
                                                                                 // DfiFreqXlatVal21=0, DfiFreqXlatVal20=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat6, 0x0);                    // DfiFreqXlatVal27=0, DfiFreqXlatVal26=0,
                                                                                 // DfiFreqXlatVal25=0, DfiFreqXlatVal24=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat7, 0xf000);                 // DfiFreqXlatVal31=0xf, DfiFreqXlatVal30=0,
                                                                                 // DfiFreqXlatVal29=0, DfiFreqXlatVal28=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_MasterX4Config, 0x0);                  // X4TG=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DMIPinPresent_p0, 0x1);                // RdDbiEnabled=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_Acx4AnibDis, 0x0);                     // Acx4AnibDis=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllCtrl1_p0, 0x21);                    // PllCpPropCtrl=1, PllCpIntCtrl=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllCtrl4_p0, 0xd8);                    // PllCpPropGsCtrl=6, PllCpIntGsCtrl=0x18
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllCtrl2_p0, 0x18);                    // PllFreqSel=0x18
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllTestMode_p0, 0x524);                // PllTestMode_p0=0x524
   ms_write_ddrc_reg(memshire, 2, MASTER0_DFIPHYUPD, 0x0);                       // DFIPHYUPDINTTHRESHOLD=0, DFIPHYUPDTHRESHOLD=0,
                                                                                 // DFIPHYUPDMODE=0, DFIPHYUPDRESP=0, DFIPHYUPDCNT=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_MemResetL, 0x2);                       // ProtectMemReset=1, MemResetLValue=0
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x0);                // MicroContMuxSel=0

   return 0;
}

//
// DDR Phy initialization for 1067 Mhz clock (after training).
//
uint32_t ms_init_ddr_phy_1067_post (uint32_t memshire) {
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x0);                // MicroContMuxSel=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s0, 0x10);           // PreSequenceReg0b0s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s1, 0x400);          // PreSequenceReg0b0s1=0x400
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s2, 0x10e);          // PreSequenceReg0b0s2=0x10e
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s0, 0x0);            // PreSequenceReg0b1s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s1, 0x0);            // PreSequenceReg0b1s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s2, 0x8);            // PreSequenceReg0b1s2=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s0, 0xb);               // SequenceReg0b0s0=0xb
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s1, 0x480);             // SequenceReg0b0s1=0x480
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s2, 0x109);             // SequenceReg0b0s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s0, 0x8);               // SequenceReg0b1s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s1, 0x448);             // SequenceReg0b1s1=0x448
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s2, 0x139);             // SequenceReg0b1s2=0x139
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s0, 0x8);               // SequenceReg0b2s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s1, 0x478);             // SequenceReg0b2s1=0x478
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s2, 0x109);             // SequenceReg0b2s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s0, 0x0);               // SequenceReg0b3s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s1, 0xe8);              // SequenceReg0b3s1=0xe8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s2, 0x109);             // SequenceReg0b3s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s0, 0x2);               // SequenceReg0b4s0=2
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s1, 0x10);              // SequenceReg0b4s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s2, 0x139);             // SequenceReg0b4s2=0x139
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s0, 0xb);               // SequenceReg0b5s0=0xb
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s1, 0x7c0);             // SequenceReg0b5s1=0x7c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s2, 0x139);             // SequenceReg0b5s2=0x139
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s0, 0x44);              // SequenceReg0b6s0=0x44
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s1, 0x633);             // SequenceReg0b6s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s2, 0x159);             // SequenceReg0b6s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s0, 0x14f);             // SequenceReg0b7s0=0x14f
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s1, 0x630);             // SequenceReg0b7s1=0x630
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s2, 0x159);             // SequenceReg0b7s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s0, 0x47);              // SequenceReg0b8s0=0x47
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s1, 0x633);             // SequenceReg0b8s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s2, 0x149);             // SequenceReg0b8s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s0, 0x4f);              // SequenceReg0b9s0=0x4f
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s1, 0x633);             // SequenceReg0b9s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s2, 0x179);             // SequenceReg0b9s2=0x179
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s0, 0x8);              // SequenceReg0b10s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s1, 0xe0);             // SequenceReg0b10s1=0xe0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s2, 0x109);            // SequenceReg0b10s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s0, 0x0);              // SequenceReg0b11s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s1, 0x7c8);            // SequenceReg0b11s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s2, 0x109);            // SequenceReg0b11s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s0, 0x0);              // SequenceReg0b12s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s1, 0x1);              // SequenceReg0b12s1=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s2, 0x8);              // SequenceReg0b12s2=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s0, 0x0);              // SequenceReg0b13s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s1, 0x45a);            // SequenceReg0b13s1=0x45a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s2, 0x9);              // SequenceReg0b13s2=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s0, 0x0);              // SequenceReg0b14s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s1, 0x448);            // SequenceReg0b14s1=0x448
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s2, 0x109);            // SequenceReg0b14s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s0, 0x40);             // SequenceReg0b15s0=0x40
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s1, 0x633);            // SequenceReg0b15s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s2, 0x179);            // SequenceReg0b15s2=0x179
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s0, 0x1);              // SequenceReg0b16s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s1, 0x618);            // SequenceReg0b16s1=0x618
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s2, 0x109);            // SequenceReg0b16s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s0, 0x40c0);           // SequenceReg0b17s0=0x40c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s1, 0x633);            // SequenceReg0b17s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s2, 0x149);            // SequenceReg0b17s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s0, 0x8);              // SequenceReg0b18s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s1, 0x4);              // SequenceReg0b18s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s2, 0x48);             // SequenceReg0b18s2=0x48
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s0, 0x4040);           // SequenceReg0b19s0=0x4040
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s1, 0x633);            // SequenceReg0b19s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s2, 0x149);            // SequenceReg0b19s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s0, 0x0);              // SequenceReg0b20s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s1, 0x4);              // SequenceReg0b20s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s2, 0x48);             // SequenceReg0b20s2=0x48
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s0, 0x40);             // SequenceReg0b21s0=0x40
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s1, 0x633);            // SequenceReg0b21s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s2, 0x149);            // SequenceReg0b21s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s0, 0x10);             // SequenceReg0b22s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s1, 0x4);              // SequenceReg0b22s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s2, 0x18);             // SequenceReg0b22s2=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s0, 0x0);              // SequenceReg0b23s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s1, 0x4);              // SequenceReg0b23s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s2, 0x78);             // SequenceReg0b23s2=0x78
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s0, 0x549);            // SequenceReg0b24s0=0x549
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s1, 0x633);            // SequenceReg0b24s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s2, 0x159);            // SequenceReg0b24s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s0, 0xd49);            // SequenceReg0b25s0=0xd49
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s1, 0x633);            // SequenceReg0b25s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s2, 0x159);            // SequenceReg0b25s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s0, 0x94a);            // SequenceReg0b26s0=0x94a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s1, 0x633);            // SequenceReg0b26s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s2, 0x159);            // SequenceReg0b26s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s0, 0x441);            // SequenceReg0b27s0=0x441
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s1, 0x633);            // SequenceReg0b27s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s2, 0x149);            // SequenceReg0b27s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s0, 0x42);             // SequenceReg0b28s0=0x42
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s1, 0x633);            // SequenceReg0b28s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s2, 0x149);            // SequenceReg0b28s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s0, 0x1);              // SequenceReg0b29s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s1, 0x633);            // SequenceReg0b29s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s2, 0x149);            // SequenceReg0b29s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s0, 0x0);              // SequenceReg0b30s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s1, 0xe0);             // SequenceReg0b30s1=0xe0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s2, 0x109);            // SequenceReg0b30s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s0, 0xa);              // SequenceReg0b31s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s1, 0x10);             // SequenceReg0b31s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s2, 0x109);            // SequenceReg0b31s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s0, 0x9);              // SequenceReg0b32s0=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s1, 0x3c0);            // SequenceReg0b32s1=0x3c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s2, 0x149);            // SequenceReg0b32s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s0, 0x9);              // SequenceReg0b33s0=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s1, 0x3c0);            // SequenceReg0b33s1=0x3c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s2, 0x159);            // SequenceReg0b33s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s0, 0x18);             // SequenceReg0b34s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s1, 0x10);             // SequenceReg0b34s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s2, 0x109);            // SequenceReg0b34s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s0, 0x0);              // SequenceReg0b35s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s1, 0x3c0);            // SequenceReg0b35s1=0x3c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s2, 0x109);            // SequenceReg0b35s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s0, 0x18);             // SequenceReg0b36s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s1, 0x4);              // SequenceReg0b36s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s2, 0x48);             // SequenceReg0b36s2=0x48
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s0, 0x18);             // SequenceReg0b37s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s1, 0x4);              // SequenceReg0b37s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s2, 0x58);             // SequenceReg0b37s2=0x58
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s0, 0xb);              // SequenceReg0b38s0=0xb
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s1, 0x10);             // SequenceReg0b38s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s2, 0x109);            // SequenceReg0b38s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s0, 0x1);              // SequenceReg0b39s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s1, 0x10);             // SequenceReg0b39s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s2, 0x109);            // SequenceReg0b39s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s0, 0x5);              // SequenceReg0b40s0=5
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s1, 0x7c0);            // SequenceReg0b40s1=0x7c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s2, 0x109);            // SequenceReg0b40s2=0x109
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x0, 0x811);                      // RFU_AcsmSeq0x0=0x811
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x0, 0x880);                      // RFU_AcsmSeq1x0=0x880
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x0, 0x0);                        // RFU_AcsmSeq2x0=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x0, 0x0);                        // RFU_AcsmSeq3x0=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x1, 0x4008);                     // RFU_AcsmSeq0x1=0x4008
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x1, 0x83);                       // RFU_AcsmSeq1x1=0x83
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x1, 0x4f);                       // RFU_AcsmSeq2x1=0x4f
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x1, 0x0);                        // RFU_AcsmSeq3x1=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x2, 0x4040);                     // RFU_AcsmSeq0x2=0x4040
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x2, 0x83);                       // RFU_AcsmSeq1x2=0x83
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x2, 0x51);                       // RFU_AcsmSeq2x2=0x51
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x2, 0x0);                        // RFU_AcsmSeq3x2=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x3, 0x811);                      // RFU_AcsmSeq0x3=0x811
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x3, 0x880);                      // RFU_AcsmSeq1x3=0x880
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x3, 0x0);                        // RFU_AcsmSeq2x3=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x3, 0x0);                        // RFU_AcsmSeq3x3=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x4, 0x720);                      // RFU_AcsmSeq0x4=0x720
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x4, 0xf);                        // RFU_AcsmSeq1x4=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x4, 0x1740);                     // RFU_AcsmSeq2x4=0x1740
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x4, 0x0);                        // RFU_AcsmSeq3x4=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x5, 0x16);                       // RFU_AcsmSeq0x5=0x16
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x5, 0x83);                       // RFU_AcsmSeq1x5=0x83
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x5, 0x4b);                       // RFU_AcsmSeq2x5=0x4b
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x5, 0x0);                        // RFU_AcsmSeq3x5=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x6, 0x716);                      // RFU_AcsmSeq0x6=0x716
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x6, 0xf);                        // RFU_AcsmSeq1x6=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x6, 0x2001);                     // RFU_AcsmSeq2x6=0x2001
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x6, 0x0);                        // RFU_AcsmSeq3x6=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x7, 0x716);                      // RFU_AcsmSeq0x7=0x716
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x7, 0xf);                        // RFU_AcsmSeq1x7=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x7, 0x2800);                     // RFU_AcsmSeq2x7=0x2800
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x7, 0x0);                        // RFU_AcsmSeq3x7=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x8, 0x716);                      // RFU_AcsmSeq0x8=0x716
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x8, 0xf);                        // RFU_AcsmSeq1x8=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x8, 0xf00);                      // RFU_AcsmSeq2x8=0xf00
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x8, 0x0);                        // RFU_AcsmSeq3x8=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x9, 0x720);                      // RFU_AcsmSeq0x9=0x720
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x9, 0xf);                        // RFU_AcsmSeq1x9=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x9, 0x1400);                     // RFU_AcsmSeq2x9=0x1400
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x9, 0x0);                        // RFU_AcsmSeq3x9=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x10, 0xe08);                     // RFU_AcsmSeq0x10=0xe08
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x10, 0xc15);                     // RFU_AcsmSeq1x10=0xc15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x10, 0x0);                       // RFU_AcsmSeq2x10=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x10, 0x0);                       // RFU_AcsmSeq3x10=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x11, 0x625);                     // RFU_AcsmSeq0x11=0x625
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x11, 0x15);                      // RFU_AcsmSeq1x11=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x11, 0x0);                       // RFU_AcsmSeq2x11=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x11, 0x0);                       // RFU_AcsmSeq3x11=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x12, 0x4028);                    // RFU_AcsmSeq0x12=0x4028
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x12, 0x80);                      // RFU_AcsmSeq1x12=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x12, 0x0);                       // RFU_AcsmSeq2x12=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x12, 0x0);                       // RFU_AcsmSeq3x12=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x13, 0xe08);                     // RFU_AcsmSeq0x13=0xe08
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x13, 0xc1a);                     // RFU_AcsmSeq1x13=0xc1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x13, 0x0);                       // RFU_AcsmSeq2x13=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x13, 0x0);                       // RFU_AcsmSeq3x13=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x14, 0x625);                     // RFU_AcsmSeq0x14=0x625
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x14, 0x1a);                      // RFU_AcsmSeq1x14=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x14, 0x0);                       // RFU_AcsmSeq2x14=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x14, 0x0);                       // RFU_AcsmSeq3x14=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x15, 0x4040);                    // RFU_AcsmSeq0x15=0x4040
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x15, 0x80);                      // RFU_AcsmSeq1x15=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x15, 0x0);                       // RFU_AcsmSeq2x15=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x15, 0x0);                       // RFU_AcsmSeq3x15=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x16, 0x2604);                    // RFU_AcsmSeq0x16=0x2604
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x16, 0x15);                      // RFU_AcsmSeq1x16=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x16, 0x0);                       // RFU_AcsmSeq2x16=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x16, 0x0);                       // RFU_AcsmSeq3x16=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x17, 0x708);                     // RFU_AcsmSeq0x17=0x708
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x17, 0x5);                       // RFU_AcsmSeq1x17=5
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x17, 0x0);                       // RFU_AcsmSeq2x17=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x17, 0x2002);                    // RFU_AcsmSeq3x17=0x2002
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x18, 0x8);                       // RFU_AcsmSeq0x18=8
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x18, 0x80);                      // RFU_AcsmSeq1x18=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x18, 0x0);                       // RFU_AcsmSeq2x18=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x18, 0x0);                       // RFU_AcsmSeq3x18=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x19, 0x2604);                    // RFU_AcsmSeq0x19=0x2604
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x19, 0x1a);                      // RFU_AcsmSeq1x19=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x19, 0x0);                       // RFU_AcsmSeq2x19=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x19, 0x0);                       // RFU_AcsmSeq3x19=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x20, 0x708);                     // RFU_AcsmSeq0x20=0x708
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x20, 0xa);                       // RFU_AcsmSeq1x20=0xa
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x20, 0x0);                       // RFU_AcsmSeq2x20=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x20, 0x2002);                    // RFU_AcsmSeq3x20=0x2002
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x21, 0x4040);                    // RFU_AcsmSeq0x21=0x4040
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x21, 0x80);                      // RFU_AcsmSeq1x21=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x21, 0x0);                       // RFU_AcsmSeq2x21=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x21, 0x0);                       // RFU_AcsmSeq3x21=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x22, 0x60a);                     // RFU_AcsmSeq0x22=0x60a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x22, 0x15);                      // RFU_AcsmSeq1x22=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x22, 0x1200);                    // RFU_AcsmSeq2x22=0x1200
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x22, 0x0);                       // RFU_AcsmSeq3x22=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x23, 0x61a);                     // RFU_AcsmSeq0x23=0x61a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x23, 0x15);                      // RFU_AcsmSeq1x23=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x23, 0x1300);                    // RFU_AcsmSeq2x23=0x1300
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x23, 0x0);                       // RFU_AcsmSeq3x23=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x24, 0x60a);                     // RFU_AcsmSeq0x24=0x60a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x24, 0x1a);                      // RFU_AcsmSeq1x24=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x24, 0x1200);                    // RFU_AcsmSeq2x24=0x1200
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x24, 0x0);                       // RFU_AcsmSeq3x24=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x25, 0x642);                     // RFU_AcsmSeq0x25=0x642
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x25, 0x1a);                      // RFU_AcsmSeq1x25=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x25, 0x1300);                    // RFU_AcsmSeq2x25=0x1300
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x25, 0x0);                       // RFU_AcsmSeq3x25=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x26, 0x4808);                    // RFU_AcsmSeq0x26=0x4808
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x26, 0x880);                     // RFU_AcsmSeq1x26=0x880
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x26, 0x0);                       // RFU_AcsmSeq2x26=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x26, 0x0);                       // RFU_AcsmSeq3x26=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s0, 0x0);              // SequenceReg0b41s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s1, 0x790);            // SequenceReg0b41s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s2, 0x11a);            // SequenceReg0b41s2=0x11a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s0, 0x8);              // SequenceReg0b42s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s1, 0x7aa);            // SequenceReg0b42s1=0x7aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s2, 0x2a);             // SequenceReg0b42s2=0x2a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s0, 0x10);             // SequenceReg0b43s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s1, 0x7b2);            // SequenceReg0b43s1=0x7b2
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s2, 0x2a);             // SequenceReg0b43s2=0x2a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s0, 0x0);              // SequenceReg0b44s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s1, 0x7c8);            // SequenceReg0b44s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s2, 0x109);            // SequenceReg0b44s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s0, 0x10);             // SequenceReg0b45s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s1, 0x10);             // SequenceReg0b45s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s2, 0x109);            // SequenceReg0b45s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s0, 0x10);             // SequenceReg0b46s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s1, 0x2a8);            // SequenceReg0b46s1=0x2a8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s2, 0x129);            // SequenceReg0b46s2=0x129
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s0, 0x8);              // SequenceReg0b47s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s1, 0x370);            // SequenceReg0b47s1=0x370
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s2, 0x129);            // SequenceReg0b47s2=0x129
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s0, 0xa);              // SequenceReg0b48s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s1, 0x3c8);            // SequenceReg0b48s1=0x3c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s2, 0x1a9);            // SequenceReg0b48s2=0x1a9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s0, 0xc);              // SequenceReg0b49s0=0xc
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s1, 0x408);            // SequenceReg0b49s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s2, 0x199);            // SequenceReg0b49s2=0x199
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s0, 0x14);             // SequenceReg0b50s0=0x14
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s1, 0x790);            // SequenceReg0b50s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s2, 0x11a);            // SequenceReg0b50s2=0x11a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s0, 0x8);              // SequenceReg0b51s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s1, 0x4);              // SequenceReg0b51s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s2, 0x18);             // SequenceReg0b51s2=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s0, 0xe);              // SequenceReg0b52s0=0xe
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s1, 0x408);            // SequenceReg0b52s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s2, 0x199);            // SequenceReg0b52s2=0x199
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s0, 0x8);              // SequenceReg0b53s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s1, 0x8568);           // SequenceReg0b53s1=0x8568
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s2, 0x108);            // SequenceReg0b53s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s0, 0x18);             // SequenceReg0b54s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s1, 0x790);            // SequenceReg0b54s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s2, 0x16a);            // SequenceReg0b54s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s0, 0x8);              // SequenceReg0b55s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s1, 0x1d8);            // SequenceReg0b55s1=0x1d8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s2, 0x169);            // SequenceReg0b55s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s0, 0x10);             // SequenceReg0b56s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s1, 0x8558);           // SequenceReg0b56s1=0x8558
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s2, 0x168);            // SequenceReg0b56s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s0, 0x70);             // SequenceReg0b57s0=0x70
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s1, 0x788);            // SequenceReg0b57s1=0x788
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s2, 0x16a);            // SequenceReg0b57s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s0, 0x1ff8);           // SequenceReg0b58s0=0x1ff8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s1, 0x85a8);           // SequenceReg0b58s1=0x85a8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s2, 0x1e8);            // SequenceReg0b58s2=0x1e8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s0, 0x50);             // SequenceReg0b59s0=0x50
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s1, 0x798);            // SequenceReg0b59s1=0x798
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s2, 0x16a);            // SequenceReg0b59s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s0, 0x60);             // SequenceReg0b60s0=0x60
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s1, 0x7a0);            // SequenceReg0b60s1=0x7a0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s2, 0x16a);            // SequenceReg0b60s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s0, 0x8);              // SequenceReg0b61s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s1, 0x8310);           // SequenceReg0b61s1=0x8310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s2, 0x168);            // SequenceReg0b61s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s0, 0x8);              // SequenceReg0b62s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s1, 0xa310);           // SequenceReg0b62s1=0xa310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s2, 0x168);            // SequenceReg0b62s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s0, 0xa);              // SequenceReg0b63s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s1, 0x408);            // SequenceReg0b63s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s2, 0x169);            // SequenceReg0b63s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s0, 0x6e);             // SequenceReg0b64s0=0x6e
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s1, 0x0);              // SequenceReg0b64s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s2, 0x68);             // SequenceReg0b64s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s0, 0x0);              // SequenceReg0b65s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s1, 0x408);            // SequenceReg0b65s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s2, 0x169);            // SequenceReg0b65s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s0, 0x0);              // SequenceReg0b66s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s1, 0x8310);           // SequenceReg0b66s1=0x8310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s2, 0x168);            // SequenceReg0b66s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s0, 0x0);              // SequenceReg0b67s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s1, 0xa310);           // SequenceReg0b67s1=0xa310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s2, 0x168);            // SequenceReg0b67s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s0, 0x1ff8);           // SequenceReg0b68s0=0x1ff8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s1, 0x85a8);           // SequenceReg0b68s1=0x85a8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s2, 0x1e8);            // SequenceReg0b68s2=0x1e8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s0, 0x68);             // SequenceReg0b69s0=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s1, 0x798);            // SequenceReg0b69s1=0x798
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s2, 0x16a);            // SequenceReg0b69s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s0, 0x78);             // SequenceReg0b70s0=0x78
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s1, 0x7a0);            // SequenceReg0b70s1=0x7a0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s2, 0x16a);            // SequenceReg0b70s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s0, 0x68);             // SequenceReg0b71s0=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s1, 0x790);            // SequenceReg0b71s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s2, 0x16a);            // SequenceReg0b71s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s0, 0x8);              // SequenceReg0b72s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s1, 0x8b10);           // SequenceReg0b72s1=0x8b10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s2, 0x168);            // SequenceReg0b72s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s0, 0x8);              // SequenceReg0b73s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s1, 0xab10);           // SequenceReg0b73s1=0xab10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s2, 0x168);            // SequenceReg0b73s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s0, 0xa);              // SequenceReg0b74s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s1, 0x408);            // SequenceReg0b74s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s2, 0x169);            // SequenceReg0b74s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s0, 0x58);             // SequenceReg0b75s0=0x58
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s1, 0x0);              // SequenceReg0b75s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s2, 0x68);             // SequenceReg0b75s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s0, 0x0);              // SequenceReg0b76s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s1, 0x408);            // SequenceReg0b76s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s2, 0x169);            // SequenceReg0b76s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s0, 0x0);              // SequenceReg0b77s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s1, 0x8b10);           // SequenceReg0b77s1=0x8b10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s2, 0x168);            // SequenceReg0b77s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s0, 0x1);              // SequenceReg0b78s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s1, 0xab10);           // SequenceReg0b78s1=0xab10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s2, 0x168);            // SequenceReg0b78s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s0, 0x0);              // SequenceReg0b79s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s1, 0x1d8);            // SequenceReg0b79s1=0x1d8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s2, 0x169);            // SequenceReg0b79s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s0, 0x80);             // SequenceReg0b80s0=0x80
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s1, 0x790);            // SequenceReg0b80s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s2, 0x16a);            // SequenceReg0b80s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s0, 0x18);             // SequenceReg0b81s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s1, 0x7aa);            // SequenceReg0b81s1=0x7aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s2, 0x6a);             // SequenceReg0b81s2=0x6a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s0, 0xa);              // SequenceReg0b82s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s1, 0x0);              // SequenceReg0b82s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s2, 0x1e9);            // SequenceReg0b82s2=0x1e9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s0, 0x8);              // SequenceReg0b83s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s1, 0x8080);           // SequenceReg0b83s1=0x8080
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s2, 0x108);            // SequenceReg0b83s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s0, 0xf);              // SequenceReg0b84s0=0xf
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s1, 0x408);            // SequenceReg0b84s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s2, 0x169);            // SequenceReg0b84s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s0, 0xc);              // SequenceReg0b85s0=0xc
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s1, 0x0);              // SequenceReg0b85s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s2, 0x68);             // SequenceReg0b85s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s0, 0x9);              // SequenceReg0b86s0=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s1, 0x0);              // SequenceReg0b86s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s2, 0x1a9);            // SequenceReg0b86s2=0x1a9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s0, 0x0);              // SequenceReg0b87s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s1, 0x408);            // SequenceReg0b87s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s2, 0x169);            // SequenceReg0b87s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s0, 0x0);              // SequenceReg0b88s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s1, 0x8080);           // SequenceReg0b88s1=0x8080
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s2, 0x108);            // SequenceReg0b88s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s0, 0x8);              // SequenceReg0b89s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s1, 0x7aa);            // SequenceReg0b89s1=0x7aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s2, 0x6a);             // SequenceReg0b89s2=0x6a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s0, 0x0);              // SequenceReg0b90s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s1, 0x8568);           // SequenceReg0b90s1=0x8568
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s2, 0x108);            // SequenceReg0b90s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s0, 0xb7);             // SequenceReg0b91s0=0xb7
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s1, 0x790);            // SequenceReg0b91s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s2, 0x16a);            // SequenceReg0b91s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s0, 0x1f);             // SequenceReg0b92s0=0x1f
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s1, 0x0);              // SequenceReg0b92s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s2, 0x68);             // SequenceReg0b92s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s0, 0x8);              // SequenceReg0b93s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s1, 0x8558);           // SequenceReg0b93s1=0x8558
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s2, 0x168);            // SequenceReg0b93s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s0, 0xf);              // SequenceReg0b94s0=0xf
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s1, 0x408);            // SequenceReg0b94s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s2, 0x169);            // SequenceReg0b94s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s0, 0xd);              // SequenceReg0b95s0=0xd
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s1, 0x0);              // SequenceReg0b95s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s2, 0x68);             // SequenceReg0b95s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s0, 0x0);              // SequenceReg0b96s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s1, 0x408);            // SequenceReg0b96s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s2, 0x169);            // SequenceReg0b96s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s0, 0x0);              // SequenceReg0b97s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s1, 0x8558);           // SequenceReg0b97s1=0x8558
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s2, 0x168);            // SequenceReg0b97s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s0, 0x8);              // SequenceReg0b98s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s1, 0x3c8);            // SequenceReg0b98s1=0x3c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s2, 0x1a9);            // SequenceReg0b98s2=0x1a9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s0, 0x3);              // SequenceReg0b99s0=3
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s1, 0x370);            // SequenceReg0b99s1=0x370
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s2, 0x129);            // SequenceReg0b99s2=0x129
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s0, 0x20);            // SequenceReg0b100s0=0x20
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s1, 0x2aa);           // SequenceReg0b100s1=0x2aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s2, 0x9);             // SequenceReg0b100s2=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s0, 0x8);             // SequenceReg0b101s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s1, 0xe8);            // SequenceReg0b101s1=0xe8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s2, 0x109);           // SequenceReg0b101s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s0, 0x0);             // SequenceReg0b102s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s1, 0x8140);          // SequenceReg0b102s1=0x8140
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s2, 0x10c);           // SequenceReg0b102s2=0x10c
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s0, 0x10);            // SequenceReg0b103s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s1, 0x8138);          // SequenceReg0b103s1=0x8138
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s2, 0x104);           // SequenceReg0b103s2=0x104
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s0, 0x8);             // SequenceReg0b104s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s1, 0x448);           // SequenceReg0b104s1=0x448
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s2, 0x109);           // SequenceReg0b104s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s0, 0xf);             // SequenceReg0b105s0=0xf
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s1, 0x7c0);           // SequenceReg0b105s1=0x7c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s2, 0x109);           // SequenceReg0b105s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s0, 0x0);             // SequenceReg0b106s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s1, 0xe8);            // SequenceReg0b106s1=0xe8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s2, 0x109);           // SequenceReg0b106s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s0, 0x47);            // SequenceReg0b107s0=0x47
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s1, 0x630);           // SequenceReg0b107s1=0x630
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s2, 0x109);           // SequenceReg0b107s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s0, 0x8);             // SequenceReg0b108s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s1, 0x618);           // SequenceReg0b108s1=0x618
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s2, 0x109);           // SequenceReg0b108s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s0, 0x8);             // SequenceReg0b109s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s1, 0xe0);            // SequenceReg0b109s1=0xe0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s2, 0x109);           // SequenceReg0b109s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s0, 0x0);             // SequenceReg0b110s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s1, 0x7c8);           // SequenceReg0b110s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s2, 0x109);           // SequenceReg0b110s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s0, 0x8);             // SequenceReg0b111s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s1, 0x8140);          // SequenceReg0b111s1=0x8140
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s2, 0x10c);           // SequenceReg0b111s2=0x10c
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s0, 0x0);             // SequenceReg0b112s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s1, 0x478);           // SequenceReg0b112s1=0x478
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s2, 0x109);           // SequenceReg0b112s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s0, 0x0);             // SequenceReg0b113s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s1, 0x1);             // SequenceReg0b113s1=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s2, 0x8);             // SequenceReg0b113s2=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s0, 0x8);             // SequenceReg0b114s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s1, 0x4);             // SequenceReg0b114s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s2, 0x0);             // SequenceReg0b114s2=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s0, 0x8);           // PostSequenceReg0b0s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s1, 0x7c8);         // PostSequenceReg0b0s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s2, 0x109);         // PostSequenceReg0b0s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s0, 0x0);           // PostSequenceReg0b1s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s1, 0x400);         // PostSequenceReg0b1s1=0x400
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s2, 0x106);         // PostSequenceReg0b1s2=0x106
   ms_write_ddrc_reg(memshire, 2, APBONLY0_SequencerOverride, 0x400);            // RFU_SequencerOverride=0x400
   ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b0, 0x0);                 // RFU_StartVector0b0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b8, 0x29);                // RFU_StartVector0b8=0x29
   ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b15, 0x68);               // RFU_StartVector0b15=0x68
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl0, 0x0);                    // RFU_AcsmCsMapCtrl0=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl1, 0x101);                  // RFU_AcsmCsMapCtrl1=0x101
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl2, 0x105);                  // RFU_AcsmCsMapCtrl2=0x105
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl3, 0x107);                  // RFU_AcsmCsMapCtrl3=0x107
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl4, 0x10f);                  // RFU_AcsmCsMapCtrl4=0x10f
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl5, 0x202);                  // RFU_AcsmCsMapCtrl5=0x202
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl6, 0x20a);                  // RFU_AcsmCsMapCtrl6=0x20a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl7, 0x20b);                  // RFU_AcsmCsMapCtrl7=0x20b
   ms_write_ddrc_reg(memshire, 2, MASTER0_DbyteDllModeCntrl, 0x2);               // DllRxPreambleMode=1, Reserved=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiWrRdDataCsConfig, 0x3);
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY0_p0, 0x85);                   // Seq0BDLY0_p0=0x85
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY1_p0, 0x10a);                  // Seq0BDLY1_p0=0x10a
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY2_p0, 0xa6b);                  // Seq0BDLY2_p0=0xa6b
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY3_p0, 0x2c);                   // Seq0BDLY3_p0=0x2c
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag0, 0x0);              // Seq0BDisableFlag0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag1, 0x173);            // Seq0BDisableFlag1=0x173
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag2, 0x60);             // Seq0BDisableFlag2=0x60
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag3, 0x6110);           // Seq0BDisableFlag3=0x6110
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag4, 0x2152);           // Seq0BDisableFlag4=0x2152
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag5, 0xdfbd);           // Seq0BDisableFlag5=0xdfbd
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag6, 0x2060);           // Seq0BDisableFlag6=0x2060
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag7, 0x6152);           // Seq0BDisableFlag7=0x6152
   ms_write_ddrc_reg(memshire, 2, MASTER0_PPTTrainSetup_p0, 0x0);                // PhyMstrMaxReqToAck=0, PhyMstrTrainInterval=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_PPTTrainSetup2_p0, 0x3);               // RFU_PPTTrainSetup2_p0=3
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x0_p0, 0xe0);               // AcsmPlayback0x0_p0=0xe0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x0_p0, 0x12);               // AcsmPlayback1x0_p0=0x12
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x1_p0, 0xe0);               // AcsmPlayback0x1_p0=0xe0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x1_p0, 0x12);               // AcsmPlayback1x1_p0=0x12
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x2_p0, 0xe0);               // AcsmPlayback0x2_p0=0xe0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x2_p0, 0x12);               // AcsmPlayback1x2_p0=0x12
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCtrl13, 0xf);                        // RFU_AcsmCtrl13=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalZap, 0x1);                          // CalZap=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalRate, 0x19);                        // DisableBackgroundZQUpdates=0, CalOnce=0,
                                                                                 // CalRun=1, CalInterval=9
   ms_write_ddrc_reg(memshire, 2, DRTUB0_UcclkHclkEnables, 0x2);                 // HclkEn=1, UcclkEn=0
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x1);                // MicroContMuxSel=1

   return 0;
}

//
// DDR Phy initialization for 933 Mhz clock (before training).
//
uint32_t ms_init_ddr_phy_933_pre (uint32_t memshire) {
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB0_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB1_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB2_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB3_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB4_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB5_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllCtrl2_p0, 0x19);                    // PllFreqSel=0x19
   ms_write_ddrc_reg(memshire, 2, MASTER0_ARdPtrInitVal_p0, 0x2);                // ARdPtrInitVal_p0=2
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BGPR4_p0, 0x0);                   // Seq0BGPR4_p0=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DqsPreambleControl_p0, 0x1a3);         // WDQSEXTENSION=1, LP4SttcPreBridgeRxEn=1,
                                                                                 // LP4PostambleExt=0, LP4TglTwoTckTxDqsPre=1,
                                                                                 // PositionDfeInit=0, TwoTckTxDqsPre=1,
                                                                                 // TwoTckRxDqsPre=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DbyteDllModeCntrl, 0x2);               // DllRxPreambleMode=1, Reserved=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DllLockParam_p0, 0x212);               // RFU_DllLockParam_p0=0x212
   ms_write_ddrc_reg(memshire, 2, MASTER0_DllGainCtl_p0, 0x61);                  // DllSeedSel=0, DllGainTV=6, DllGainIV=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_ProcOdtTimeCtl_p0, 0x3);               // POdtTailWidthExt=0, POdtStartDelay=0,
                                                                                 // POdtTailWidth=3
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, ANIB0_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB1_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB2_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB3_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB4_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB5_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiMode, 0x3);                         // Dfi1Override=0, Dfi1Enable=1, Dfi0Enable=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiCAMode, 0x4);                       // DfiD4AltCAMode=0, DfiLp4CAMode=1, DfiD4CAMode=0,
                                                                                 // DfiLp3CAMode=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalDrvStr0, 0x0);                      // CalDrvStrPu50=0, CalDrvStrPd50=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalVRefs, 0x2);                        // CalVRefs=2
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalUclkInfo_p0, 0x3a5);                // CalUClkTicksPer1uS=0x3a5
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalRate, 0x9);                         // DisableBackgroundZQUpdates=0, CalOnce=0,
                                                                                 // CalRun=0, CalInterval=9
   ms_write_ddrc_reg(memshire, 2, MASTER0_VrefInGlobal_p0, 0x32c);               // GlobalVrefInMode=0, GlobalVrefInTrim=0,
                                                                                 // GlobalVrefInDAC=0x65, GlobalVrefInSel=4
   ms_write_ddrc_reg(memshire, 2, DBYTE0_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqRatio_p0, 0x1);                 // DfiFreqRatio_p0=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_TristateModeCA_p0, 0x1);               // CkDisVal=0, DDR2TMode=0, DisDynAdrTri=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat0, 0x0);                    // DfiFreqXlatVal3=0, DfiFreqXlatVal2=0,
                                                                                 // DfiFreqXlatVal1=0, DfiFreqXlatVal0=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat1, 0x0);                    // DfiFreqXlatVal7=0, DfiFreqXlatVal6=0,
                                                                                 // DfiFreqXlatVal5=0, DfiFreqXlatVal4=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat2, 0x4444);                 // DfiFreqXlatVal11=4, DfiFreqXlatVal10=4,
                                                                                 // DfiFreqXlatVal9=4, DfiFreqXlatVal8=4
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat3, 0x8888);                 // DfiFreqXlatVal15=8, DfiFreqXlatVal14=8,
                                                                                 // DfiFreqXlatVal13=8, DfiFreqXlatVal12=8
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat4, 0x5555);                 // DfiFreqXlatVal19=5, DfiFreqXlatVal18=5,
                                                                                 // DfiFreqXlatVal17=5, DfiFreqXlatVal16=5
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat5, 0x0);                    // DfiFreqXlatVal23=0, DfiFreqXlatVal22=0,
                                                                                 // DfiFreqXlatVal21=0, DfiFreqXlatVal20=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat6, 0x0);                    // DfiFreqXlatVal27=0, DfiFreqXlatVal26=0,
                                                                                 // DfiFreqXlatVal25=0, DfiFreqXlatVal24=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat7, 0xf000);                 // DfiFreqXlatVal31=0xf, DfiFreqXlatVal30=0,
                                                                                 // DfiFreqXlatVal29=0, DfiFreqXlatVal28=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_MasterX4Config, 0x0);                  // X4TG=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DMIPinPresent_p0, 0x1);                // RdDbiEnabled=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_Acx4AnibDis, 0x0);                     // Acx4AnibDis=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllCtrl1_p0, 0x43);                    // PllCpPropCtrl=2, PllCpIntCtrl=3
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllCtrl4_p0, 0xd8);                    // PllCpPropGsCtrl=6, PllCpIntGsCtrl=0x18
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllCtrl2_p0, 0x19);                    // PllFreqSel=0x19
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllTestMode_p0, 0x424);                // PllTestMode_p0=0x424
   ms_write_ddrc_reg(memshire, 2, MASTER0_DFIPHYUPD, 0x0);                       // DFIPHYUPDINTTHRESHOLD=0, DFIPHYUPDTHRESHOLD=0,
                                                                                 // DFIPHYUPDMODE=0, DFIPHYUPDRESP=0, DFIPHYUPDCNT=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_MemResetL, 0x2);                       // ProtectMemReset=1, MemResetLValue=0
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x0);                // MicroContMuxSel=0

   return 0;
}

//
// DDR Phy initialization for 933 Mhz clock (after training).
//
uint32_t ms_init_ddr_phy_933_post (uint32_t memshire) {
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x0);                // MicroContMuxSel=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s0, 0x10);           // PreSequenceReg0b0s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s1, 0x400);          // PreSequenceReg0b0s1=0x400
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s2, 0x10e);          // PreSequenceReg0b0s2=0x10e
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s0, 0x0);            // PreSequenceReg0b1s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s1, 0x0);            // PreSequenceReg0b1s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s2, 0x8);            // PreSequenceReg0b1s2=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s0, 0xb);               // SequenceReg0b0s0=0xb
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s1, 0x480);             // SequenceReg0b0s1=0x480
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s2, 0x109);             // SequenceReg0b0s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s0, 0x8);               // SequenceReg0b1s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s1, 0x448);             // SequenceReg0b1s1=0x448
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s2, 0x139);             // SequenceReg0b1s2=0x139
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s0, 0x8);               // SequenceReg0b2s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s1, 0x478);             // SequenceReg0b2s1=0x478
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s2, 0x109);             // SequenceReg0b2s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s0, 0x0);               // SequenceReg0b3s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s1, 0xe8);              // SequenceReg0b3s1=0xe8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s2, 0x109);             // SequenceReg0b3s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s0, 0x2);               // SequenceReg0b4s0=2
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s1, 0x10);              // SequenceReg0b4s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s2, 0x139);             // SequenceReg0b4s2=0x139
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s0, 0xb);               // SequenceReg0b5s0=0xb
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s1, 0x7c0);             // SequenceReg0b5s1=0x7c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s2, 0x139);             // SequenceReg0b5s2=0x139
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s0, 0x44);              // SequenceReg0b6s0=0x44
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s1, 0x633);             // SequenceReg0b6s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s2, 0x159);             // SequenceReg0b6s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s0, 0x14f);             // SequenceReg0b7s0=0x14f
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s1, 0x630);             // SequenceReg0b7s1=0x630
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s2, 0x159);             // SequenceReg0b7s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s0, 0x47);              // SequenceReg0b8s0=0x47
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s1, 0x633);             // SequenceReg0b8s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s2, 0x149);             // SequenceReg0b8s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s0, 0x4f);              // SequenceReg0b9s0=0x4f
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s1, 0x633);             // SequenceReg0b9s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s2, 0x179);             // SequenceReg0b9s2=0x179
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s0, 0x8);              // SequenceReg0b10s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s1, 0xe0);             // SequenceReg0b10s1=0xe0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s2, 0x109);            // SequenceReg0b10s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s0, 0x0);              // SequenceReg0b11s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s1, 0x7c8);            // SequenceReg0b11s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s2, 0x109);            // SequenceReg0b11s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s0, 0x0);              // SequenceReg0b12s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s1, 0x1);              // SequenceReg0b12s1=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s2, 0x8);              // SequenceReg0b12s2=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s0, 0x0);              // SequenceReg0b13s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s1, 0x45a);            // SequenceReg0b13s1=0x45a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s2, 0x9);              // SequenceReg0b13s2=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s0, 0x0);              // SequenceReg0b14s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s1, 0x448);            // SequenceReg0b14s1=0x448
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s2, 0x109);            // SequenceReg0b14s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s0, 0x40);             // SequenceReg0b15s0=0x40
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s1, 0x633);            // SequenceReg0b15s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s2, 0x179);            // SequenceReg0b15s2=0x179
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s0, 0x1);              // SequenceReg0b16s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s1, 0x618);            // SequenceReg0b16s1=0x618
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s2, 0x109);            // SequenceReg0b16s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s0, 0x40c0);           // SequenceReg0b17s0=0x40c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s1, 0x633);            // SequenceReg0b17s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s2, 0x149);            // SequenceReg0b17s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s0, 0x8);              // SequenceReg0b18s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s1, 0x4);              // SequenceReg0b18s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s2, 0x48);             // SequenceReg0b18s2=0x48
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s0, 0x4040);           // SequenceReg0b19s0=0x4040
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s1, 0x633);            // SequenceReg0b19s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s2, 0x149);            // SequenceReg0b19s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s0, 0x0);              // SequenceReg0b20s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s1, 0x4);              // SequenceReg0b20s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s2, 0x48);             // SequenceReg0b20s2=0x48
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s0, 0x40);             // SequenceReg0b21s0=0x40
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s1, 0x633);            // SequenceReg0b21s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s2, 0x149);            // SequenceReg0b21s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s0, 0x10);             // SequenceReg0b22s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s1, 0x4);              // SequenceReg0b22s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s2, 0x18);             // SequenceReg0b22s2=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s0, 0x0);              // SequenceReg0b23s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s1, 0x4);              // SequenceReg0b23s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s2, 0x78);             // SequenceReg0b23s2=0x78
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s0, 0x549);            // SequenceReg0b24s0=0x549
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s1, 0x633);            // SequenceReg0b24s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s2, 0x159);            // SequenceReg0b24s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s0, 0xd49);            // SequenceReg0b25s0=0xd49
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s1, 0x633);            // SequenceReg0b25s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s2, 0x159);            // SequenceReg0b25s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s0, 0x94a);            // SequenceReg0b26s0=0x94a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s1, 0x633);            // SequenceReg0b26s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s2, 0x159);            // SequenceReg0b26s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s0, 0x441);            // SequenceReg0b27s0=0x441
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s1, 0x633);            // SequenceReg0b27s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s2, 0x149);            // SequenceReg0b27s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s0, 0x42);             // SequenceReg0b28s0=0x42
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s1, 0x633);            // SequenceReg0b28s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s2, 0x149);            // SequenceReg0b28s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s0, 0x1);              // SequenceReg0b29s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s1, 0x633);            // SequenceReg0b29s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s2, 0x149);            // SequenceReg0b29s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s0, 0x0);              // SequenceReg0b30s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s1, 0xe0);             // SequenceReg0b30s1=0xe0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s2, 0x109);            // SequenceReg0b30s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s0, 0xa);              // SequenceReg0b31s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s1, 0x10);             // SequenceReg0b31s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s2, 0x109);            // SequenceReg0b31s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s0, 0x9);              // SequenceReg0b32s0=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s1, 0x3c0);            // SequenceReg0b32s1=0x3c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s2, 0x149);            // SequenceReg0b32s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s0, 0x9);              // SequenceReg0b33s0=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s1, 0x3c0);            // SequenceReg0b33s1=0x3c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s2, 0x159);            // SequenceReg0b33s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s0, 0x18);             // SequenceReg0b34s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s1, 0x10);             // SequenceReg0b34s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s2, 0x109);            // SequenceReg0b34s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s0, 0x0);              // SequenceReg0b35s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s1, 0x3c0);            // SequenceReg0b35s1=0x3c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s2, 0x109);            // SequenceReg0b35s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s0, 0x18);             // SequenceReg0b36s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s1, 0x4);              // SequenceReg0b36s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s2, 0x48);             // SequenceReg0b36s2=0x48
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s0, 0x18);             // SequenceReg0b37s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s1, 0x4);              // SequenceReg0b37s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s2, 0x58);             // SequenceReg0b37s2=0x58
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s0, 0xb);              // SequenceReg0b38s0=0xb
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s1, 0x10);             // SequenceReg0b38s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s2, 0x109);            // SequenceReg0b38s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s0, 0x1);              // SequenceReg0b39s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s1, 0x10);             // SequenceReg0b39s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s2, 0x109);            // SequenceReg0b39s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s0, 0x5);              // SequenceReg0b40s0=5
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s1, 0x7c0);            // SequenceReg0b40s1=0x7c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s2, 0x109);            // SequenceReg0b40s2=0x109
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x0, 0x811);                      // RFU_AcsmSeq0x0=0x811
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x0, 0x880);                      // RFU_AcsmSeq1x0=0x880
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x0, 0x0);                        // RFU_AcsmSeq2x0=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x0, 0x0);                        // RFU_AcsmSeq3x0=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x1, 0x4008);                     // RFU_AcsmSeq0x1=0x4008
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x1, 0x83);                       // RFU_AcsmSeq1x1=0x83
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x1, 0x4f);                       // RFU_AcsmSeq2x1=0x4f
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x1, 0x0);                        // RFU_AcsmSeq3x1=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x2, 0x4040);                     // RFU_AcsmSeq0x2=0x4040
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x2, 0x83);                       // RFU_AcsmSeq1x2=0x83
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x2, 0x51);                       // RFU_AcsmSeq2x2=0x51
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x2, 0x0);                        // RFU_AcsmSeq3x2=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x3, 0x811);                      // RFU_AcsmSeq0x3=0x811
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x3, 0x880);                      // RFU_AcsmSeq1x3=0x880
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x3, 0x0);                        // RFU_AcsmSeq2x3=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x3, 0x0);                        // RFU_AcsmSeq3x3=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x4, 0x720);                      // RFU_AcsmSeq0x4=0x720
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x4, 0xf);                        // RFU_AcsmSeq1x4=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x4, 0x1740);                     // RFU_AcsmSeq2x4=0x1740
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x4, 0x0);                        // RFU_AcsmSeq3x4=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x5, 0x16);                       // RFU_AcsmSeq0x5=0x16
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x5, 0x83);                       // RFU_AcsmSeq1x5=0x83
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x5, 0x4b);                       // RFU_AcsmSeq2x5=0x4b
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x5, 0x0);                        // RFU_AcsmSeq3x5=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x6, 0x716);                      // RFU_AcsmSeq0x6=0x716
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x6, 0xf);                        // RFU_AcsmSeq1x6=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x6, 0x2001);                     // RFU_AcsmSeq2x6=0x2001
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x6, 0x0);                        // RFU_AcsmSeq3x6=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x7, 0x716);                      // RFU_AcsmSeq0x7=0x716
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x7, 0xf);                        // RFU_AcsmSeq1x7=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x7, 0x2800);                     // RFU_AcsmSeq2x7=0x2800
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x7, 0x0);                        // RFU_AcsmSeq3x7=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x8, 0x716);                      // RFU_AcsmSeq0x8=0x716
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x8, 0xf);                        // RFU_AcsmSeq1x8=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x8, 0xf00);                      // RFU_AcsmSeq2x8=0xf00
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x8, 0x0);                        // RFU_AcsmSeq3x8=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x9, 0x720);                      // RFU_AcsmSeq0x9=0x720
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x9, 0xf);                        // RFU_AcsmSeq1x9=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x9, 0x1400);                     // RFU_AcsmSeq2x9=0x1400
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x9, 0x0);                        // RFU_AcsmSeq3x9=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x10, 0xe08);                     // RFU_AcsmSeq0x10=0xe08
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x10, 0xc15);                     // RFU_AcsmSeq1x10=0xc15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x10, 0x0);                       // RFU_AcsmSeq2x10=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x10, 0x0);                       // RFU_AcsmSeq3x10=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x11, 0x625);                     // RFU_AcsmSeq0x11=0x625
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x11, 0x15);                      // RFU_AcsmSeq1x11=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x11, 0x0);                       // RFU_AcsmSeq2x11=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x11, 0x0);                       // RFU_AcsmSeq3x11=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x12, 0x4028);                    // RFU_AcsmSeq0x12=0x4028
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x12, 0x80);                      // RFU_AcsmSeq1x12=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x12, 0x0);                       // RFU_AcsmSeq2x12=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x12, 0x0);                       // RFU_AcsmSeq3x12=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x13, 0xe08);                     // RFU_AcsmSeq0x13=0xe08
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x13, 0xc1a);                     // RFU_AcsmSeq1x13=0xc1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x13, 0x0);                       // RFU_AcsmSeq2x13=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x13, 0x0);                       // RFU_AcsmSeq3x13=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x14, 0x625);                     // RFU_AcsmSeq0x14=0x625
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x14, 0x1a);                      // RFU_AcsmSeq1x14=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x14, 0x0);                       // RFU_AcsmSeq2x14=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x14, 0x0);                       // RFU_AcsmSeq3x14=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x15, 0x4040);                    // RFU_AcsmSeq0x15=0x4040
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x15, 0x80);                      // RFU_AcsmSeq1x15=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x15, 0x0);                       // RFU_AcsmSeq2x15=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x15, 0x0);                       // RFU_AcsmSeq3x15=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x16, 0x2604);                    // RFU_AcsmSeq0x16=0x2604
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x16, 0x15);                      // RFU_AcsmSeq1x16=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x16, 0x0);                       // RFU_AcsmSeq2x16=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x16, 0x0);                       // RFU_AcsmSeq3x16=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x17, 0x708);                     // RFU_AcsmSeq0x17=0x708
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x17, 0x5);                       // RFU_AcsmSeq1x17=5
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x17, 0x0);                       // RFU_AcsmSeq2x17=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x17, 0x2002);                    // RFU_AcsmSeq3x17=0x2002
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x18, 0x8);                       // RFU_AcsmSeq0x18=8
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x18, 0x80);                      // RFU_AcsmSeq1x18=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x18, 0x0);                       // RFU_AcsmSeq2x18=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x18, 0x0);                       // RFU_AcsmSeq3x18=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x19, 0x2604);                    // RFU_AcsmSeq0x19=0x2604
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x19, 0x1a);                      // RFU_AcsmSeq1x19=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x19, 0x0);                       // RFU_AcsmSeq2x19=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x19, 0x0);                       // RFU_AcsmSeq3x19=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x20, 0x708);                     // RFU_AcsmSeq0x20=0x708
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x20, 0xa);                       // RFU_AcsmSeq1x20=0xa
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x20, 0x0);                       // RFU_AcsmSeq2x20=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x20, 0x2002);                    // RFU_AcsmSeq3x20=0x2002
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x21, 0x4040);                    // RFU_AcsmSeq0x21=0x4040
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x21, 0x80);                      // RFU_AcsmSeq1x21=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x21, 0x0);                       // RFU_AcsmSeq2x21=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x21, 0x0);                       // RFU_AcsmSeq3x21=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x22, 0x60a);                     // RFU_AcsmSeq0x22=0x60a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x22, 0x15);                      // RFU_AcsmSeq1x22=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x22, 0x1200);                    // RFU_AcsmSeq2x22=0x1200
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x22, 0x0);                       // RFU_AcsmSeq3x22=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x23, 0x61a);                     // RFU_AcsmSeq0x23=0x61a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x23, 0x15);                      // RFU_AcsmSeq1x23=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x23, 0x1300);                    // RFU_AcsmSeq2x23=0x1300
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x23, 0x0);                       // RFU_AcsmSeq3x23=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x24, 0x60a);                     // RFU_AcsmSeq0x24=0x60a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x24, 0x1a);                      // RFU_AcsmSeq1x24=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x24, 0x1200);                    // RFU_AcsmSeq2x24=0x1200
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x24, 0x0);                       // RFU_AcsmSeq3x24=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x25, 0x642);                     // RFU_AcsmSeq0x25=0x642
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x25, 0x1a);                      // RFU_AcsmSeq1x25=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x25, 0x1300);                    // RFU_AcsmSeq2x25=0x1300
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x25, 0x0);                       // RFU_AcsmSeq3x25=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x26, 0x4808);                    // RFU_AcsmSeq0x26=0x4808
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x26, 0x880);                     // RFU_AcsmSeq1x26=0x880
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x26, 0x0);                       // RFU_AcsmSeq2x26=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x26, 0x0);                       // RFU_AcsmSeq3x26=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s0, 0x0);              // SequenceReg0b41s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s1, 0x790);            // SequenceReg0b41s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s2, 0x11a);            // SequenceReg0b41s2=0x11a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s0, 0x8);              // SequenceReg0b42s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s1, 0x7aa);            // SequenceReg0b42s1=0x7aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s2, 0x2a);             // SequenceReg0b42s2=0x2a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s0, 0x10);             // SequenceReg0b43s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s1, 0x7b2);            // SequenceReg0b43s1=0x7b2
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s2, 0x2a);             // SequenceReg0b43s2=0x2a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s0, 0x0);              // SequenceReg0b44s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s1, 0x7c8);            // SequenceReg0b44s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s2, 0x109);            // SequenceReg0b44s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s0, 0x10);             // SequenceReg0b45s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s1, 0x10);             // SequenceReg0b45s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s2, 0x109);            // SequenceReg0b45s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s0, 0x10);             // SequenceReg0b46s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s1, 0x2a8);            // SequenceReg0b46s1=0x2a8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s2, 0x129);            // SequenceReg0b46s2=0x129
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s0, 0x8);              // SequenceReg0b47s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s1, 0x370);            // SequenceReg0b47s1=0x370
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s2, 0x129);            // SequenceReg0b47s2=0x129
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s0, 0xa);              // SequenceReg0b48s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s1, 0x3c8);            // SequenceReg0b48s1=0x3c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s2, 0x1a9);            // SequenceReg0b48s2=0x1a9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s0, 0xc);              // SequenceReg0b49s0=0xc
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s1, 0x408);            // SequenceReg0b49s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s2, 0x199);            // SequenceReg0b49s2=0x199
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s0, 0x14);             // SequenceReg0b50s0=0x14
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s1, 0x790);            // SequenceReg0b50s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s2, 0x11a);            // SequenceReg0b50s2=0x11a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s0, 0x8);              // SequenceReg0b51s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s1, 0x4);              // SequenceReg0b51s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s2, 0x18);             // SequenceReg0b51s2=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s0, 0xe);              // SequenceReg0b52s0=0xe
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s1, 0x408);            // SequenceReg0b52s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s2, 0x199);            // SequenceReg0b52s2=0x199
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s0, 0x8);              // SequenceReg0b53s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s1, 0x8568);           // SequenceReg0b53s1=0x8568
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s2, 0x108);            // SequenceReg0b53s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s0, 0x18);             // SequenceReg0b54s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s1, 0x790);            // SequenceReg0b54s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s2, 0x16a);            // SequenceReg0b54s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s0, 0x8);              // SequenceReg0b55s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s1, 0x1d8);            // SequenceReg0b55s1=0x1d8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s2, 0x169);            // SequenceReg0b55s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s0, 0x10);             // SequenceReg0b56s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s1, 0x8558);           // SequenceReg0b56s1=0x8558
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s2, 0x168);            // SequenceReg0b56s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s0, 0x70);             // SequenceReg0b57s0=0x70
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s1, 0x788);            // SequenceReg0b57s1=0x788
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s2, 0x16a);            // SequenceReg0b57s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s0, 0x1ff8);           // SequenceReg0b58s0=0x1ff8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s1, 0x85a8);           // SequenceReg0b58s1=0x85a8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s2, 0x1e8);            // SequenceReg0b58s2=0x1e8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s0, 0x50);             // SequenceReg0b59s0=0x50
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s1, 0x798);            // SequenceReg0b59s1=0x798
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s2, 0x16a);            // SequenceReg0b59s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s0, 0x60);             // SequenceReg0b60s0=0x60
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s1, 0x7a0);            // SequenceReg0b60s1=0x7a0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s2, 0x16a);            // SequenceReg0b60s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s0, 0x8);              // SequenceReg0b61s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s1, 0x8310);           // SequenceReg0b61s1=0x8310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s2, 0x168);            // SequenceReg0b61s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s0, 0x8);              // SequenceReg0b62s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s1, 0xa310);           // SequenceReg0b62s1=0xa310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s2, 0x168);            // SequenceReg0b62s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s0, 0xa);              // SequenceReg0b63s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s1, 0x408);            // SequenceReg0b63s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s2, 0x169);            // SequenceReg0b63s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s0, 0x6e);             // SequenceReg0b64s0=0x6e
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s1, 0x0);              // SequenceReg0b64s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s2, 0x68);             // SequenceReg0b64s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s0, 0x0);              // SequenceReg0b65s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s1, 0x408);            // SequenceReg0b65s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s2, 0x169);            // SequenceReg0b65s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s0, 0x0);              // SequenceReg0b66s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s1, 0x8310);           // SequenceReg0b66s1=0x8310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s2, 0x168);            // SequenceReg0b66s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s0, 0x0);              // SequenceReg0b67s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s1, 0xa310);           // SequenceReg0b67s1=0xa310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s2, 0x168);            // SequenceReg0b67s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s0, 0x1ff8);           // SequenceReg0b68s0=0x1ff8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s1, 0x85a8);           // SequenceReg0b68s1=0x85a8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s2, 0x1e8);            // SequenceReg0b68s2=0x1e8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s0, 0x68);             // SequenceReg0b69s0=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s1, 0x798);            // SequenceReg0b69s1=0x798
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s2, 0x16a);            // SequenceReg0b69s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s0, 0x78);             // SequenceReg0b70s0=0x78
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s1, 0x7a0);            // SequenceReg0b70s1=0x7a0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s2, 0x16a);            // SequenceReg0b70s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s0, 0x68);             // SequenceReg0b71s0=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s1, 0x790);            // SequenceReg0b71s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s2, 0x16a);            // SequenceReg0b71s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s0, 0x8);              // SequenceReg0b72s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s1, 0x8b10);           // SequenceReg0b72s1=0x8b10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s2, 0x168);            // SequenceReg0b72s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s0, 0x8);              // SequenceReg0b73s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s1, 0xab10);           // SequenceReg0b73s1=0xab10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s2, 0x168);            // SequenceReg0b73s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s0, 0xa);              // SequenceReg0b74s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s1, 0x408);            // SequenceReg0b74s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s2, 0x169);            // SequenceReg0b74s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s0, 0x58);             // SequenceReg0b75s0=0x58
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s1, 0x0);              // SequenceReg0b75s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s2, 0x68);             // SequenceReg0b75s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s0, 0x0);              // SequenceReg0b76s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s1, 0x408);            // SequenceReg0b76s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s2, 0x169);            // SequenceReg0b76s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s0, 0x0);              // SequenceReg0b77s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s1, 0x8b10);           // SequenceReg0b77s1=0x8b10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s2, 0x168);            // SequenceReg0b77s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s0, 0x1);              // SequenceReg0b78s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s1, 0xab10);           // SequenceReg0b78s1=0xab10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s2, 0x168);            // SequenceReg0b78s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s0, 0x0);              // SequenceReg0b79s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s1, 0x1d8);            // SequenceReg0b79s1=0x1d8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s2, 0x169);            // SequenceReg0b79s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s0, 0x80);             // SequenceReg0b80s0=0x80
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s1, 0x790);            // SequenceReg0b80s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s2, 0x16a);            // SequenceReg0b80s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s0, 0x18);             // SequenceReg0b81s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s1, 0x7aa);            // SequenceReg0b81s1=0x7aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s2, 0x6a);             // SequenceReg0b81s2=0x6a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s0, 0xa);              // SequenceReg0b82s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s1, 0x0);              // SequenceReg0b82s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s2, 0x1e9);            // SequenceReg0b82s2=0x1e9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s0, 0x8);              // SequenceReg0b83s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s1, 0x8080);           // SequenceReg0b83s1=0x8080
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s2, 0x108);            // SequenceReg0b83s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s0, 0xf);              // SequenceReg0b84s0=0xf
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s1, 0x408);            // SequenceReg0b84s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s2, 0x169);            // SequenceReg0b84s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s0, 0xc);              // SequenceReg0b85s0=0xc
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s1, 0x0);              // SequenceReg0b85s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s2, 0x68);             // SequenceReg0b85s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s0, 0x9);              // SequenceReg0b86s0=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s1, 0x0);              // SequenceReg0b86s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s2, 0x1a9);            // SequenceReg0b86s2=0x1a9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s0, 0x0);              // SequenceReg0b87s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s1, 0x408);            // SequenceReg0b87s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s2, 0x169);            // SequenceReg0b87s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s0, 0x0);              // SequenceReg0b88s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s1, 0x8080);           // SequenceReg0b88s1=0x8080
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s2, 0x108);            // SequenceReg0b88s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s0, 0x8);              // SequenceReg0b89s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s1, 0x7aa);            // SequenceReg0b89s1=0x7aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s2, 0x6a);             // SequenceReg0b89s2=0x6a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s0, 0x0);              // SequenceReg0b90s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s1, 0x8568);           // SequenceReg0b90s1=0x8568
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s2, 0x108);            // SequenceReg0b90s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s0, 0xb7);             // SequenceReg0b91s0=0xb7
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s1, 0x790);            // SequenceReg0b91s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s2, 0x16a);            // SequenceReg0b91s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s0, 0x1f);             // SequenceReg0b92s0=0x1f
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s1, 0x0);              // SequenceReg0b92s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s2, 0x68);             // SequenceReg0b92s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s0, 0x8);              // SequenceReg0b93s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s1, 0x8558);           // SequenceReg0b93s1=0x8558
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s2, 0x168);            // SequenceReg0b93s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s0, 0xf);              // SequenceReg0b94s0=0xf
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s1, 0x408);            // SequenceReg0b94s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s2, 0x169);            // SequenceReg0b94s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s0, 0xd);              // SequenceReg0b95s0=0xd
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s1, 0x0);              // SequenceReg0b95s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s2, 0x68);             // SequenceReg0b95s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s0, 0x0);              // SequenceReg0b96s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s1, 0x408);            // SequenceReg0b96s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s2, 0x169);            // SequenceReg0b96s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s0, 0x0);              // SequenceReg0b97s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s1, 0x8558);           // SequenceReg0b97s1=0x8558
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s2, 0x168);            // SequenceReg0b97s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s0, 0x8);              // SequenceReg0b98s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s1, 0x3c8);            // SequenceReg0b98s1=0x3c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s2, 0x1a9);            // SequenceReg0b98s2=0x1a9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s0, 0x3);              // SequenceReg0b99s0=3
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s1, 0x370);            // SequenceReg0b99s1=0x370
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s2, 0x129);            // SequenceReg0b99s2=0x129
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s0, 0x20);            // SequenceReg0b100s0=0x20
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s1, 0x2aa);           // SequenceReg0b100s1=0x2aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s2, 0x9);             // SequenceReg0b100s2=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s0, 0x8);             // SequenceReg0b101s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s1, 0xe8);            // SequenceReg0b101s1=0xe8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s2, 0x109);           // SequenceReg0b101s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s0, 0x0);             // SequenceReg0b102s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s1, 0x8140);          // SequenceReg0b102s1=0x8140
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s2, 0x10c);           // SequenceReg0b102s2=0x10c
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s0, 0x10);            // SequenceReg0b103s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s1, 0x8138);          // SequenceReg0b103s1=0x8138
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s2, 0x104);           // SequenceReg0b103s2=0x104
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s0, 0x8);             // SequenceReg0b104s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s1, 0x448);           // SequenceReg0b104s1=0x448
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s2, 0x109);           // SequenceReg0b104s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s0, 0xf);             // SequenceReg0b105s0=0xf
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s1, 0x7c0);           // SequenceReg0b105s1=0x7c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s2, 0x109);           // SequenceReg0b105s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s0, 0x0);             // SequenceReg0b106s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s1, 0xe8);            // SequenceReg0b106s1=0xe8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s2, 0x109);           // SequenceReg0b106s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s0, 0x47);            // SequenceReg0b107s0=0x47
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s1, 0x630);           // SequenceReg0b107s1=0x630
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s2, 0x109);           // SequenceReg0b107s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s0, 0x8);             // SequenceReg0b108s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s1, 0x618);           // SequenceReg0b108s1=0x618
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s2, 0x109);           // SequenceReg0b108s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s0, 0x8);             // SequenceReg0b109s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s1, 0xe0);            // SequenceReg0b109s1=0xe0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s2, 0x109);           // SequenceReg0b109s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s0, 0x0);             // SequenceReg0b110s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s1, 0x7c8);           // SequenceReg0b110s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s2, 0x109);           // SequenceReg0b110s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s0, 0x8);             // SequenceReg0b111s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s1, 0x8140);          // SequenceReg0b111s1=0x8140
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s2, 0x10c);           // SequenceReg0b111s2=0x10c
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s0, 0x0);             // SequenceReg0b112s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s1, 0x478);           // SequenceReg0b112s1=0x478
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s2, 0x109);           // SequenceReg0b112s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s0, 0x0);             // SequenceReg0b113s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s1, 0x1);             // SequenceReg0b113s1=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s2, 0x8);             // SequenceReg0b113s2=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s0, 0x8);             // SequenceReg0b114s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s1, 0x4);             // SequenceReg0b114s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s2, 0x0);             // SequenceReg0b114s2=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s0, 0x8);           // PostSequenceReg0b0s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s1, 0x7c8);         // PostSequenceReg0b0s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s2, 0x109);         // PostSequenceReg0b0s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s0, 0x0);           // PostSequenceReg0b1s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s1, 0x400);         // PostSequenceReg0b1s1=0x400
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s2, 0x106);         // PostSequenceReg0b1s2=0x106
   ms_write_ddrc_reg(memshire, 2, APBONLY0_SequencerOverride, 0x400);            // RFU_SequencerOverride=0x400
   ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b0, 0x0);                 // RFU_StartVector0b0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b8, 0x29);                // RFU_StartVector0b8=0x29
   ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b15, 0x68);               // RFU_StartVector0b15=0x68
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl0, 0x0);                    // RFU_AcsmCsMapCtrl0=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl1, 0x101);                  // RFU_AcsmCsMapCtrl1=0x101
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl2, 0x105);                  // RFU_AcsmCsMapCtrl2=0x105
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl3, 0x107);                  // RFU_AcsmCsMapCtrl3=0x107
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl4, 0x10f);                  // RFU_AcsmCsMapCtrl4=0x10f
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl5, 0x202);                  // RFU_AcsmCsMapCtrl5=0x202
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl6, 0x20a);                  // RFU_AcsmCsMapCtrl6=0x20a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl7, 0x20b);                  // RFU_AcsmCsMapCtrl7=0x20b
   ms_write_ddrc_reg(memshire, 2, MASTER0_DbyteDllModeCntrl, 0x2);               // DllRxPreambleMode=1, Reserved=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiWrRdDataCsConfig, 0x3);
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY0_p0, 0x74);                   // Seq0BDLY0_p0=0x74
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY1_p0, 0xe9);                   // Seq0BDLY1_p0=0xe9
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY2_p0, 0x91c);                  // Seq0BDLY2_p0=0x91c
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY3_p0, 0x2c);                   // Seq0BDLY3_p0=0x2c
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag0, 0x0);              // Seq0BDisableFlag0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag1, 0x173);            // Seq0BDisableFlag1=0x173
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag2, 0x60);             // Seq0BDisableFlag2=0x60
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag3, 0x6110);           // Seq0BDisableFlag3=0x6110
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag4, 0x2152);           // Seq0BDisableFlag4=0x2152
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag5, 0xdfbd);           // Seq0BDisableFlag5=0xdfbd
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag6, 0x2060);           // Seq0BDisableFlag6=0x2060
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag7, 0x6152);           // Seq0BDisableFlag7=0x6152
   ms_write_ddrc_reg(memshire, 2, MASTER0_PPTTrainSetup_p0, 0x0);                // PhyMstrMaxReqToAck=0, PhyMstrTrainInterval=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_PPTTrainSetup2_p0, 0x3);               // RFU_PPTTrainSetup2_p0=3
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x0_p0, 0xe0);               // AcsmPlayback0x0_p0=0xe0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x0_p0, 0x12);               // AcsmPlayback1x0_p0=0x12
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x1_p0, 0xe0);               // AcsmPlayback0x1_p0=0xe0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x1_p0, 0x12);               // AcsmPlayback1x1_p0=0x12
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x2_p0, 0xe0);               // AcsmPlayback0x2_p0=0xe0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x2_p0, 0x12);               // AcsmPlayback1x2_p0=0x12
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCtrl13, 0xf);                        // RFU_AcsmCtrl13=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalZap, 0x1);                          // CalZap=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalRate, 0x19);                        // DisableBackgroundZQUpdates=0, CalOnce=0,
                                                                                 // CalRun=1, CalInterval=9
   ms_write_ddrc_reg(memshire, 2, DRTUB0_UcclkHclkEnables, 0x2);                 // HclkEn=1, UcclkEn=0
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x1);                // MicroContMuxSel=1

   return 0;
}

//
// DDR Phy initialization for 800 Mhz clock (before training).
//
uint32_t ms_init_ddr_phy_800_pre (uint32_t memshire) {
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB0_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB1_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB2_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB3_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB4_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB5_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllCtrl2_p0, 0x19);                    // PllFreqSel=0x19
   ms_write_ddrc_reg(memshire, 2, MASTER0_ARdPtrInitVal_p0, 0x2);                // ARdPtrInitVal_p0=2
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BGPR4_p0, 0x0);                   // Seq0BGPR4_p0=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DqsPreambleControl_p0, 0x1a3);         // WDQSEXTENSION=1, LP4SttcPreBridgeRxEn=1,
                                                                                 // LP4PostambleExt=0, LP4TglTwoTckTxDqsPre=1,
                                                                                 // PositionDfeInit=0, TwoTckTxDqsPre=1,
                                                                                 // TwoTckRxDqsPre=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DbyteDllModeCntrl, 0x2);               // DllRxPreambleMode=1, Reserved=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DllLockParam_p0, 0x212);               // RFU_DllLockParam_p0=0x212
   ms_write_ddrc_reg(memshire, 2, MASTER0_DllGainCtl_p0, 0x61);                  // DllSeedSel=0, DllGainTV=6, DllGainIV=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_ProcOdtTimeCtl_p0, 0x3);               // POdtTailWidthExt=0, POdtStartDelay=0,
                                                                                 // POdtTailWidth=3
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, ANIB0_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB1_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB2_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB3_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB4_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB5_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiMode, 0x3);                         // Dfi1Override=0, Dfi1Enable=1, Dfi0Enable=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiCAMode, 0x4);                       // DfiD4AltCAMode=0, DfiLp4CAMode=1, DfiD4CAMode=0,
                                                                                 // DfiLp3CAMode=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalDrvStr0, 0x0);                      // CalDrvStrPu50=0, CalDrvStrPd50=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalVRefs, 0x2);                        // CalVRefs=2
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalUclkInfo_p0, 0x320);                // CalUClkTicksPer1uS=0x320
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalRate, 0x9);                         // DisableBackgroundZQUpdates=0, CalOnce=0,
                                                                                 // CalRun=0, CalInterval=9
   ms_write_ddrc_reg(memshire, 2, MASTER0_VrefInGlobal_p0, 0x32c);               // GlobalVrefInMode=0, GlobalVrefInTrim=0,
                                                                                 // GlobalVrefInDAC=0x65, GlobalVrefInSel=4
   ms_write_ddrc_reg(memshire, 2, DBYTE0_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqRatio_p0, 0x1);                 // DfiFreqRatio_p0=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_TristateModeCA_p0, 0x1);               // CkDisVal=0, DDR2TMode=0, DisDynAdrTri=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat0, 0x0);                    // DfiFreqXlatVal3=0, DfiFreqXlatVal2=0,
                                                                                 // DfiFreqXlatVal1=0, DfiFreqXlatVal0=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat1, 0x0);                    // DfiFreqXlatVal7=0, DfiFreqXlatVal6=0,
                                                                                 // DfiFreqXlatVal5=0, DfiFreqXlatVal4=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat2, 0x4444);                 // DfiFreqXlatVal11=4, DfiFreqXlatVal10=4,
                                                                                 // DfiFreqXlatVal9=4, DfiFreqXlatVal8=4
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat3, 0x8888);                 // DfiFreqXlatVal15=8, DfiFreqXlatVal14=8,
                                                                                 // DfiFreqXlatVal13=8, DfiFreqXlatVal12=8
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat4, 0x5555);                 // DfiFreqXlatVal19=5, DfiFreqXlatVal18=5,
                                                                                 // DfiFreqXlatVal17=5, DfiFreqXlatVal16=5
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat5, 0x0);                    // DfiFreqXlatVal23=0, DfiFreqXlatVal22=0,
                                                                                 // DfiFreqXlatVal21=0, DfiFreqXlatVal20=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat6, 0x0);                    // DfiFreqXlatVal27=0, DfiFreqXlatVal26=0,
                                                                                 // DfiFreqXlatVal25=0, DfiFreqXlatVal24=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat7, 0xf000);                 // DfiFreqXlatVal31=0xf, DfiFreqXlatVal30=0,
                                                                                 // DfiFreqXlatVal29=0, DfiFreqXlatVal28=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_MasterX4Config, 0x0);                  // X4TG=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DMIPinPresent_p0, 0x1);                // RdDbiEnabled=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_Acx4AnibDis, 0x0);                     // Acx4AnibDis=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllCtrl1_p0, 0x43);                    // PllCpPropCtrl=2, PllCpIntCtrl=3
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllCtrl4_p0, 0xd8);                    // PllCpPropGsCtrl=6, PllCpIntGsCtrl=0x18
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllCtrl2_p0, 0x19);                    // PllFreqSel=0x19
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllTestMode_p0, 0x424);                // PllTestMode_p0=0x424
   ms_write_ddrc_reg(memshire, 2, MASTER0_DFIPHYUPD, 0x0);                       // DFIPHYUPDINTTHRESHOLD=0, DFIPHYUPDTHRESHOLD=0,
                                                                                 // DFIPHYUPDMODE=0, DFIPHYUPDRESP=0, DFIPHYUPDCNT=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_MemResetL, 0x2);                       // ProtectMemReset=1, MemResetLValue=0
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x0);                // MicroContMuxSel=0

   return 0;
}

//
// DDR Phy initialization for 800 Mhz clock (after training).
//
uint32_t ms_init_ddr_phy_800_post (uint32_t memshire) {
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x0);                // MicroContMuxSel=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s0, 0x10);           // PreSequenceReg0b0s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s1, 0x400);          // PreSequenceReg0b0s1=0x400
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s2, 0x10e);          // PreSequenceReg0b0s2=0x10e
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s0, 0x0);            // PreSequenceReg0b1s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s1, 0x0);            // PreSequenceReg0b1s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s2, 0x8);            // PreSequenceReg0b1s2=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s0, 0xb);               // SequenceReg0b0s0=0xb
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s1, 0x480);             // SequenceReg0b0s1=0x480
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s2, 0x109);             // SequenceReg0b0s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s0, 0x8);               // SequenceReg0b1s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s1, 0x448);             // SequenceReg0b1s1=0x448
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s2, 0x139);             // SequenceReg0b1s2=0x139
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s0, 0x8);               // SequenceReg0b2s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s1, 0x478);             // SequenceReg0b2s1=0x478
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s2, 0x109);             // SequenceReg0b2s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s0, 0x0);               // SequenceReg0b3s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s1, 0xe8);              // SequenceReg0b3s1=0xe8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s2, 0x109);             // SequenceReg0b3s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s0, 0x2);               // SequenceReg0b4s0=2
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s1, 0x10);              // SequenceReg0b4s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s2, 0x139);             // SequenceReg0b4s2=0x139
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s0, 0xb);               // SequenceReg0b5s0=0xb
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s1, 0x7c0);             // SequenceReg0b5s1=0x7c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s2, 0x139);             // SequenceReg0b5s2=0x139
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s0, 0x44);              // SequenceReg0b6s0=0x44
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s1, 0x633);             // SequenceReg0b6s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s2, 0x159);             // SequenceReg0b6s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s0, 0x14f);             // SequenceReg0b7s0=0x14f
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s1, 0x630);             // SequenceReg0b7s1=0x630
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s2, 0x159);             // SequenceReg0b7s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s0, 0x47);              // SequenceReg0b8s0=0x47
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s1, 0x633);             // SequenceReg0b8s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s2, 0x149);             // SequenceReg0b8s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s0, 0x4f);              // SequenceReg0b9s0=0x4f
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s1, 0x633);             // SequenceReg0b9s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s2, 0x179);             // SequenceReg0b9s2=0x179
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s0, 0x8);              // SequenceReg0b10s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s1, 0xe0);             // SequenceReg0b10s1=0xe0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s2, 0x109);            // SequenceReg0b10s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s0, 0x0);              // SequenceReg0b11s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s1, 0x7c8);            // SequenceReg0b11s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s2, 0x109);            // SequenceReg0b11s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s0, 0x0);              // SequenceReg0b12s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s1, 0x1);              // SequenceReg0b12s1=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s2, 0x8);              // SequenceReg0b12s2=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s0, 0x0);              // SequenceReg0b13s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s1, 0x45a);            // SequenceReg0b13s1=0x45a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s2, 0x9);              // SequenceReg0b13s2=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s0, 0x0);              // SequenceReg0b14s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s1, 0x448);            // SequenceReg0b14s1=0x448
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s2, 0x109);            // SequenceReg0b14s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s0, 0x40);             // SequenceReg0b15s0=0x40
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s1, 0x633);            // SequenceReg0b15s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s2, 0x179);            // SequenceReg0b15s2=0x179
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s0, 0x1);              // SequenceReg0b16s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s1, 0x618);            // SequenceReg0b16s1=0x618
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s2, 0x109);            // SequenceReg0b16s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s0, 0x40c0);           // SequenceReg0b17s0=0x40c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s1, 0x633);            // SequenceReg0b17s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s2, 0x149);            // SequenceReg0b17s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s0, 0x8);              // SequenceReg0b18s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s1, 0x4);              // SequenceReg0b18s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s2, 0x48);             // SequenceReg0b18s2=0x48
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s0, 0x4040);           // SequenceReg0b19s0=0x4040
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s1, 0x633);            // SequenceReg0b19s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s2, 0x149);            // SequenceReg0b19s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s0, 0x0);              // SequenceReg0b20s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s1, 0x4);              // SequenceReg0b20s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s2, 0x48);             // SequenceReg0b20s2=0x48
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s0, 0x40);             // SequenceReg0b21s0=0x40
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s1, 0x633);            // SequenceReg0b21s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s2, 0x149);            // SequenceReg0b21s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s0, 0x10);             // SequenceReg0b22s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s1, 0x4);              // SequenceReg0b22s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s2, 0x18);             // SequenceReg0b22s2=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s0, 0x0);              // SequenceReg0b23s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s1, 0x4);              // SequenceReg0b23s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s2, 0x78);             // SequenceReg0b23s2=0x78
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s0, 0x549);            // SequenceReg0b24s0=0x549
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s1, 0x633);            // SequenceReg0b24s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s2, 0x159);            // SequenceReg0b24s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s0, 0xd49);            // SequenceReg0b25s0=0xd49
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s1, 0x633);            // SequenceReg0b25s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s2, 0x159);            // SequenceReg0b25s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s0, 0x94a);            // SequenceReg0b26s0=0x94a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s1, 0x633);            // SequenceReg0b26s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s2, 0x159);            // SequenceReg0b26s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s0, 0x441);            // SequenceReg0b27s0=0x441
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s1, 0x633);            // SequenceReg0b27s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s2, 0x149);            // SequenceReg0b27s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s0, 0x42);             // SequenceReg0b28s0=0x42
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s1, 0x633);            // SequenceReg0b28s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s2, 0x149);            // SequenceReg0b28s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s0, 0x1);              // SequenceReg0b29s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s1, 0x633);            // SequenceReg0b29s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s2, 0x149);            // SequenceReg0b29s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s0, 0x0);              // SequenceReg0b30s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s1, 0xe0);             // SequenceReg0b30s1=0xe0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s2, 0x109);            // SequenceReg0b30s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s0, 0xa);              // SequenceReg0b31s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s1, 0x10);             // SequenceReg0b31s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s2, 0x109);            // SequenceReg0b31s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s0, 0x9);              // SequenceReg0b32s0=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s1, 0x3c0);            // SequenceReg0b32s1=0x3c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s2, 0x149);            // SequenceReg0b32s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s0, 0x9);              // SequenceReg0b33s0=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s1, 0x3c0);            // SequenceReg0b33s1=0x3c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s2, 0x159);            // SequenceReg0b33s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s0, 0x18);             // SequenceReg0b34s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s1, 0x10);             // SequenceReg0b34s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s2, 0x109);            // SequenceReg0b34s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s0, 0x0);              // SequenceReg0b35s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s1, 0x3c0);            // SequenceReg0b35s1=0x3c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s2, 0x109);            // SequenceReg0b35s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s0, 0x18);             // SequenceReg0b36s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s1, 0x4);              // SequenceReg0b36s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s2, 0x48);             // SequenceReg0b36s2=0x48
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s0, 0x18);             // SequenceReg0b37s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s1, 0x4);              // SequenceReg0b37s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s2, 0x58);             // SequenceReg0b37s2=0x58
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s0, 0xb);              // SequenceReg0b38s0=0xb
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s1, 0x10);             // SequenceReg0b38s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s2, 0x109);            // SequenceReg0b38s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s0, 0x1);              // SequenceReg0b39s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s1, 0x10);             // SequenceReg0b39s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s2, 0x109);            // SequenceReg0b39s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s0, 0x5);              // SequenceReg0b40s0=5
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s1, 0x7c0);            // SequenceReg0b40s1=0x7c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s2, 0x109);            // SequenceReg0b40s2=0x109
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x0, 0x811);                      // RFU_AcsmSeq0x0=0x811
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x0, 0x880);                      // RFU_AcsmSeq1x0=0x880
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x0, 0x0);                        // RFU_AcsmSeq2x0=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x0, 0x0);                        // RFU_AcsmSeq3x0=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x1, 0x4008);                     // RFU_AcsmSeq0x1=0x4008
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x1, 0x83);                       // RFU_AcsmSeq1x1=0x83
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x1, 0x4f);                       // RFU_AcsmSeq2x1=0x4f
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x1, 0x0);                        // RFU_AcsmSeq3x1=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x2, 0x4040);                     // RFU_AcsmSeq0x2=0x4040
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x2, 0x83);                       // RFU_AcsmSeq1x2=0x83
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x2, 0x51);                       // RFU_AcsmSeq2x2=0x51
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x2, 0x0);                        // RFU_AcsmSeq3x2=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x3, 0x811);                      // RFU_AcsmSeq0x3=0x811
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x3, 0x880);                      // RFU_AcsmSeq1x3=0x880
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x3, 0x0);                        // RFU_AcsmSeq2x3=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x3, 0x0);                        // RFU_AcsmSeq3x3=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x4, 0x720);                      // RFU_AcsmSeq0x4=0x720
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x4, 0xf);                        // RFU_AcsmSeq1x4=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x4, 0x1740);                     // RFU_AcsmSeq2x4=0x1740
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x4, 0x0);                        // RFU_AcsmSeq3x4=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x5, 0x16);                       // RFU_AcsmSeq0x5=0x16
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x5, 0x83);                       // RFU_AcsmSeq1x5=0x83
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x5, 0x4b);                       // RFU_AcsmSeq2x5=0x4b
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x5, 0x0);                        // RFU_AcsmSeq3x5=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x6, 0x716);                      // RFU_AcsmSeq0x6=0x716
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x6, 0xf);                        // RFU_AcsmSeq1x6=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x6, 0x2001);                     // RFU_AcsmSeq2x6=0x2001
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x6, 0x0);                        // RFU_AcsmSeq3x6=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x7, 0x716);                      // RFU_AcsmSeq0x7=0x716
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x7, 0xf);                        // RFU_AcsmSeq1x7=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x7, 0x2800);                     // RFU_AcsmSeq2x7=0x2800
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x7, 0x0);                        // RFU_AcsmSeq3x7=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x8, 0x716);                      // RFU_AcsmSeq0x8=0x716
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x8, 0xf);                        // RFU_AcsmSeq1x8=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x8, 0xf00);                      // RFU_AcsmSeq2x8=0xf00
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x8, 0x0);                        // RFU_AcsmSeq3x8=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x9, 0x720);                      // RFU_AcsmSeq0x9=0x720
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x9, 0xf);                        // RFU_AcsmSeq1x9=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x9, 0x1400);                     // RFU_AcsmSeq2x9=0x1400
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x9, 0x0);                        // RFU_AcsmSeq3x9=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x10, 0xe08);                     // RFU_AcsmSeq0x10=0xe08
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x10, 0xc15);                     // RFU_AcsmSeq1x10=0xc15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x10, 0x0);                       // RFU_AcsmSeq2x10=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x10, 0x0);                       // RFU_AcsmSeq3x10=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x11, 0x625);                     // RFU_AcsmSeq0x11=0x625
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x11, 0x15);                      // RFU_AcsmSeq1x11=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x11, 0x0);                       // RFU_AcsmSeq2x11=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x11, 0x0);                       // RFU_AcsmSeq3x11=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x12, 0x4028);                    // RFU_AcsmSeq0x12=0x4028
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x12, 0x80);                      // RFU_AcsmSeq1x12=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x12, 0x0);                       // RFU_AcsmSeq2x12=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x12, 0x0);                       // RFU_AcsmSeq3x12=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x13, 0xe08);                     // RFU_AcsmSeq0x13=0xe08
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x13, 0xc1a);                     // RFU_AcsmSeq1x13=0xc1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x13, 0x0);                       // RFU_AcsmSeq2x13=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x13, 0x0);                       // RFU_AcsmSeq3x13=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x14, 0x625);                     // RFU_AcsmSeq0x14=0x625
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x14, 0x1a);                      // RFU_AcsmSeq1x14=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x14, 0x0);                       // RFU_AcsmSeq2x14=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x14, 0x0);                       // RFU_AcsmSeq3x14=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x15, 0x4040);                    // RFU_AcsmSeq0x15=0x4040
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x15, 0x80);                      // RFU_AcsmSeq1x15=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x15, 0x0);                       // RFU_AcsmSeq2x15=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x15, 0x0);                       // RFU_AcsmSeq3x15=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x16, 0x2604);                    // RFU_AcsmSeq0x16=0x2604
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x16, 0x15);                      // RFU_AcsmSeq1x16=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x16, 0x0);                       // RFU_AcsmSeq2x16=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x16, 0x0);                       // RFU_AcsmSeq3x16=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x17, 0x708);                     // RFU_AcsmSeq0x17=0x708
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x17, 0x5);                       // RFU_AcsmSeq1x17=5
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x17, 0x0);                       // RFU_AcsmSeq2x17=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x17, 0x2002);                    // RFU_AcsmSeq3x17=0x2002
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x18, 0x8);                       // RFU_AcsmSeq0x18=8
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x18, 0x80);                      // RFU_AcsmSeq1x18=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x18, 0x0);                       // RFU_AcsmSeq2x18=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x18, 0x0);                       // RFU_AcsmSeq3x18=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x19, 0x2604);                    // RFU_AcsmSeq0x19=0x2604
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x19, 0x1a);                      // RFU_AcsmSeq1x19=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x19, 0x0);                       // RFU_AcsmSeq2x19=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x19, 0x0);                       // RFU_AcsmSeq3x19=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x20, 0x708);                     // RFU_AcsmSeq0x20=0x708
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x20, 0xa);                       // RFU_AcsmSeq1x20=0xa
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x20, 0x0);                       // RFU_AcsmSeq2x20=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x20, 0x2002);                    // RFU_AcsmSeq3x20=0x2002
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x21, 0x4040);                    // RFU_AcsmSeq0x21=0x4040
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x21, 0x80);                      // RFU_AcsmSeq1x21=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x21, 0x0);                       // RFU_AcsmSeq2x21=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x21, 0x0);                       // RFU_AcsmSeq3x21=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x22, 0x60a);                     // RFU_AcsmSeq0x22=0x60a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x22, 0x15);                      // RFU_AcsmSeq1x22=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x22, 0x1200);                    // RFU_AcsmSeq2x22=0x1200
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x22, 0x0);                       // RFU_AcsmSeq3x22=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x23, 0x61a);                     // RFU_AcsmSeq0x23=0x61a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x23, 0x15);                      // RFU_AcsmSeq1x23=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x23, 0x1300);                    // RFU_AcsmSeq2x23=0x1300
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x23, 0x0);                       // RFU_AcsmSeq3x23=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x24, 0x60a);                     // RFU_AcsmSeq0x24=0x60a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x24, 0x1a);                      // RFU_AcsmSeq1x24=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x24, 0x1200);                    // RFU_AcsmSeq2x24=0x1200
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x24, 0x0);                       // RFU_AcsmSeq3x24=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x25, 0x642);                     // RFU_AcsmSeq0x25=0x642
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x25, 0x1a);                      // RFU_AcsmSeq1x25=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x25, 0x1300);                    // RFU_AcsmSeq2x25=0x1300
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x25, 0x0);                       // RFU_AcsmSeq3x25=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x26, 0x4808);                    // RFU_AcsmSeq0x26=0x4808
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x26, 0x880);                     // RFU_AcsmSeq1x26=0x880
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x26, 0x0);                       // RFU_AcsmSeq2x26=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x26, 0x0);                       // RFU_AcsmSeq3x26=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s0, 0x0);              // SequenceReg0b41s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s1, 0x790);            // SequenceReg0b41s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s2, 0x11a);            // SequenceReg0b41s2=0x11a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s0, 0x8);              // SequenceReg0b42s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s1, 0x7aa);            // SequenceReg0b42s1=0x7aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s2, 0x2a);             // SequenceReg0b42s2=0x2a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s0, 0x10);             // SequenceReg0b43s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s1, 0x7b2);            // SequenceReg0b43s1=0x7b2
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s2, 0x2a);             // SequenceReg0b43s2=0x2a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s0, 0x0);              // SequenceReg0b44s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s1, 0x7c8);            // SequenceReg0b44s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s2, 0x109);            // SequenceReg0b44s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s0, 0x10);             // SequenceReg0b45s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s1, 0x10);             // SequenceReg0b45s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s2, 0x109);            // SequenceReg0b45s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s0, 0x10);             // SequenceReg0b46s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s1, 0x2a8);            // SequenceReg0b46s1=0x2a8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s2, 0x129);            // SequenceReg0b46s2=0x129
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s0, 0x8);              // SequenceReg0b47s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s1, 0x370);            // SequenceReg0b47s1=0x370
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s2, 0x129);            // SequenceReg0b47s2=0x129
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s0, 0xa);              // SequenceReg0b48s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s1, 0x3c8);            // SequenceReg0b48s1=0x3c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s2, 0x1a9);            // SequenceReg0b48s2=0x1a9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s0, 0xc);              // SequenceReg0b49s0=0xc
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s1, 0x408);            // SequenceReg0b49s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s2, 0x199);            // SequenceReg0b49s2=0x199
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s0, 0x14);             // SequenceReg0b50s0=0x14
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s1, 0x790);            // SequenceReg0b50s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s2, 0x11a);            // SequenceReg0b50s2=0x11a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s0, 0x8);              // SequenceReg0b51s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s1, 0x4);              // SequenceReg0b51s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s2, 0x18);             // SequenceReg0b51s2=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s0, 0xe);              // SequenceReg0b52s0=0xe
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s1, 0x408);            // SequenceReg0b52s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s2, 0x199);            // SequenceReg0b52s2=0x199
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s0, 0x8);              // SequenceReg0b53s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s1, 0x8568);           // SequenceReg0b53s1=0x8568
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s2, 0x108);            // SequenceReg0b53s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s0, 0x18);             // SequenceReg0b54s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s1, 0x790);            // SequenceReg0b54s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s2, 0x16a);            // SequenceReg0b54s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s0, 0x8);              // SequenceReg0b55s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s1, 0x1d8);            // SequenceReg0b55s1=0x1d8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s2, 0x169);            // SequenceReg0b55s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s0, 0x10);             // SequenceReg0b56s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s1, 0x8558);           // SequenceReg0b56s1=0x8558
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s2, 0x168);            // SequenceReg0b56s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s0, 0x70);             // SequenceReg0b57s0=0x70
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s1, 0x788);            // SequenceReg0b57s1=0x788
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s2, 0x16a);            // SequenceReg0b57s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s0, 0x1ff8);           // SequenceReg0b58s0=0x1ff8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s1, 0x85a8);           // SequenceReg0b58s1=0x85a8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s2, 0x1e8);            // SequenceReg0b58s2=0x1e8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s0, 0x50);             // SequenceReg0b59s0=0x50
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s1, 0x798);            // SequenceReg0b59s1=0x798
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s2, 0x16a);            // SequenceReg0b59s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s0, 0x60);             // SequenceReg0b60s0=0x60
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s1, 0x7a0);            // SequenceReg0b60s1=0x7a0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s2, 0x16a);            // SequenceReg0b60s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s0, 0x8);              // SequenceReg0b61s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s1, 0x8310);           // SequenceReg0b61s1=0x8310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s2, 0x168);            // SequenceReg0b61s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s0, 0x8);              // SequenceReg0b62s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s1, 0xa310);           // SequenceReg0b62s1=0xa310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s2, 0x168);            // SequenceReg0b62s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s0, 0xa);              // SequenceReg0b63s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s1, 0x408);            // SequenceReg0b63s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s2, 0x169);            // SequenceReg0b63s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s0, 0x6e);             // SequenceReg0b64s0=0x6e
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s1, 0x0);              // SequenceReg0b64s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s2, 0x68);             // SequenceReg0b64s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s0, 0x0);              // SequenceReg0b65s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s1, 0x408);            // SequenceReg0b65s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s2, 0x169);            // SequenceReg0b65s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s0, 0x0);              // SequenceReg0b66s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s1, 0x8310);           // SequenceReg0b66s1=0x8310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s2, 0x168);            // SequenceReg0b66s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s0, 0x0);              // SequenceReg0b67s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s1, 0xa310);           // SequenceReg0b67s1=0xa310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s2, 0x168);            // SequenceReg0b67s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s0, 0x1ff8);           // SequenceReg0b68s0=0x1ff8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s1, 0x85a8);           // SequenceReg0b68s1=0x85a8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s2, 0x1e8);            // SequenceReg0b68s2=0x1e8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s0, 0x68);             // SequenceReg0b69s0=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s1, 0x798);            // SequenceReg0b69s1=0x798
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s2, 0x16a);            // SequenceReg0b69s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s0, 0x78);             // SequenceReg0b70s0=0x78
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s1, 0x7a0);            // SequenceReg0b70s1=0x7a0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s2, 0x16a);            // SequenceReg0b70s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s0, 0x68);             // SequenceReg0b71s0=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s1, 0x790);            // SequenceReg0b71s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s2, 0x16a);            // SequenceReg0b71s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s0, 0x8);              // SequenceReg0b72s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s1, 0x8b10);           // SequenceReg0b72s1=0x8b10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s2, 0x168);            // SequenceReg0b72s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s0, 0x8);              // SequenceReg0b73s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s1, 0xab10);           // SequenceReg0b73s1=0xab10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s2, 0x168);            // SequenceReg0b73s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s0, 0xa);              // SequenceReg0b74s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s1, 0x408);            // SequenceReg0b74s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s2, 0x169);            // SequenceReg0b74s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s0, 0x58);             // SequenceReg0b75s0=0x58
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s1, 0x0);              // SequenceReg0b75s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s2, 0x68);             // SequenceReg0b75s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s0, 0x0);              // SequenceReg0b76s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s1, 0x408);            // SequenceReg0b76s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s2, 0x169);            // SequenceReg0b76s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s0, 0x0);              // SequenceReg0b77s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s1, 0x8b10);           // SequenceReg0b77s1=0x8b10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s2, 0x168);            // SequenceReg0b77s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s0, 0x1);              // SequenceReg0b78s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s1, 0xab10);           // SequenceReg0b78s1=0xab10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s2, 0x168);            // SequenceReg0b78s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s0, 0x0);              // SequenceReg0b79s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s1, 0x1d8);            // SequenceReg0b79s1=0x1d8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s2, 0x169);            // SequenceReg0b79s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s0, 0x80);             // SequenceReg0b80s0=0x80
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s1, 0x790);            // SequenceReg0b80s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s2, 0x16a);            // SequenceReg0b80s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s0, 0x18);             // SequenceReg0b81s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s1, 0x7aa);            // SequenceReg0b81s1=0x7aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s2, 0x6a);             // SequenceReg0b81s2=0x6a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s0, 0xa);              // SequenceReg0b82s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s1, 0x0);              // SequenceReg0b82s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s2, 0x1e9);            // SequenceReg0b82s2=0x1e9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s0, 0x8);              // SequenceReg0b83s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s1, 0x8080);           // SequenceReg0b83s1=0x8080
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s2, 0x108);            // SequenceReg0b83s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s0, 0xf);              // SequenceReg0b84s0=0xf
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s1, 0x408);            // SequenceReg0b84s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s2, 0x169);            // SequenceReg0b84s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s0, 0xc);              // SequenceReg0b85s0=0xc
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s1, 0x0);              // SequenceReg0b85s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s2, 0x68);             // SequenceReg0b85s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s0, 0x9);              // SequenceReg0b86s0=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s1, 0x0);              // SequenceReg0b86s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s2, 0x1a9);            // SequenceReg0b86s2=0x1a9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s0, 0x0);              // SequenceReg0b87s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s1, 0x408);            // SequenceReg0b87s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s2, 0x169);            // SequenceReg0b87s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s0, 0x0);              // SequenceReg0b88s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s1, 0x8080);           // SequenceReg0b88s1=0x8080
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s2, 0x108);            // SequenceReg0b88s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s0, 0x8);              // SequenceReg0b89s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s1, 0x7aa);            // SequenceReg0b89s1=0x7aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s2, 0x6a);             // SequenceReg0b89s2=0x6a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s0, 0x0);              // SequenceReg0b90s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s1, 0x8568);           // SequenceReg0b90s1=0x8568
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s2, 0x108);            // SequenceReg0b90s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s0, 0xb7);             // SequenceReg0b91s0=0xb7
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s1, 0x790);            // SequenceReg0b91s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s2, 0x16a);            // SequenceReg0b91s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s0, 0x1f);             // SequenceReg0b92s0=0x1f
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s1, 0x0);              // SequenceReg0b92s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s2, 0x68);             // SequenceReg0b92s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s0, 0x8);              // SequenceReg0b93s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s1, 0x8558);           // SequenceReg0b93s1=0x8558
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s2, 0x168);            // SequenceReg0b93s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s0, 0xf);              // SequenceReg0b94s0=0xf
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s1, 0x408);            // SequenceReg0b94s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s2, 0x169);            // SequenceReg0b94s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s0, 0xd);              // SequenceReg0b95s0=0xd
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s1, 0x0);              // SequenceReg0b95s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s2, 0x68);             // SequenceReg0b95s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s0, 0x0);              // SequenceReg0b96s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s1, 0x408);            // SequenceReg0b96s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s2, 0x169);            // SequenceReg0b96s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s0, 0x0);              // SequenceReg0b97s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s1, 0x8558);           // SequenceReg0b97s1=0x8558
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s2, 0x168);            // SequenceReg0b97s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s0, 0x8);              // SequenceReg0b98s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s1, 0x3c8);            // SequenceReg0b98s1=0x3c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s2, 0x1a9);            // SequenceReg0b98s2=0x1a9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s0, 0x3);              // SequenceReg0b99s0=3
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s1, 0x370);            // SequenceReg0b99s1=0x370
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s2, 0x129);            // SequenceReg0b99s2=0x129
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s0, 0x20);            // SequenceReg0b100s0=0x20
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s1, 0x2aa);           // SequenceReg0b100s1=0x2aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s2, 0x9);             // SequenceReg0b100s2=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s0, 0x8);             // SequenceReg0b101s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s1, 0xe8);            // SequenceReg0b101s1=0xe8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s2, 0x109);           // SequenceReg0b101s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s0, 0x0);             // SequenceReg0b102s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s1, 0x8140);          // SequenceReg0b102s1=0x8140
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s2, 0x10c);           // SequenceReg0b102s2=0x10c
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s0, 0x10);            // SequenceReg0b103s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s1, 0x8138);          // SequenceReg0b103s1=0x8138
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s2, 0x104);           // SequenceReg0b103s2=0x104
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s0, 0x8);             // SequenceReg0b104s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s1, 0x448);           // SequenceReg0b104s1=0x448
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s2, 0x109);           // SequenceReg0b104s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s0, 0xf);             // SequenceReg0b105s0=0xf
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s1, 0x7c0);           // SequenceReg0b105s1=0x7c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s2, 0x109);           // SequenceReg0b105s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s0, 0x0);             // SequenceReg0b106s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s1, 0xe8);            // SequenceReg0b106s1=0xe8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s2, 0x109);           // SequenceReg0b106s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s0, 0x47);            // SequenceReg0b107s0=0x47
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s1, 0x630);           // SequenceReg0b107s1=0x630
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s2, 0x109);           // SequenceReg0b107s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s0, 0x8);             // SequenceReg0b108s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s1, 0x618);           // SequenceReg0b108s1=0x618
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s2, 0x109);           // SequenceReg0b108s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s0, 0x8);             // SequenceReg0b109s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s1, 0xe0);            // SequenceReg0b109s1=0xe0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s2, 0x109);           // SequenceReg0b109s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s0, 0x0);             // SequenceReg0b110s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s1, 0x7c8);           // SequenceReg0b110s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s2, 0x109);           // SequenceReg0b110s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s0, 0x8);             // SequenceReg0b111s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s1, 0x8140);          // SequenceReg0b111s1=0x8140
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s2, 0x10c);           // SequenceReg0b111s2=0x10c
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s0, 0x0);             // SequenceReg0b112s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s1, 0x478);           // SequenceReg0b112s1=0x478
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s2, 0x109);           // SequenceReg0b112s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s0, 0x0);             // SequenceReg0b113s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s1, 0x1);             // SequenceReg0b113s1=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s2, 0x8);             // SequenceReg0b113s2=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s0, 0x8);             // SequenceReg0b114s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s1, 0x4);             // SequenceReg0b114s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s2, 0x0);             // SequenceReg0b114s2=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s0, 0x8);           // PostSequenceReg0b0s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s1, 0x7c8);         // PostSequenceReg0b0s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s2, 0x109);         // PostSequenceReg0b0s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s0, 0x0);           // PostSequenceReg0b1s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s1, 0x400);         // PostSequenceReg0b1s1=0x400
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s2, 0x106);         // PostSequenceReg0b1s2=0x106
   ms_write_ddrc_reg(memshire, 2, APBONLY0_SequencerOverride, 0x400);            // RFU_SequencerOverride=0x400
   ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b0, 0x0);                 // RFU_StartVector0b0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b8, 0x29);                // RFU_StartVector0b8=0x29
   ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b15, 0x68);               // RFU_StartVector0b15=0x68
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl0, 0x0);                    // RFU_AcsmCsMapCtrl0=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl1, 0x101);                  // RFU_AcsmCsMapCtrl1=0x101
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl2, 0x105);                  // RFU_AcsmCsMapCtrl2=0x105
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl3, 0x107);                  // RFU_AcsmCsMapCtrl3=0x107
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl4, 0x10f);                  // RFU_AcsmCsMapCtrl4=0x10f
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl5, 0x202);                  // RFU_AcsmCsMapCtrl5=0x202
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl6, 0x20a);                  // RFU_AcsmCsMapCtrl6=0x20a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl7, 0x20b);                  // RFU_AcsmCsMapCtrl7=0x20b
   ms_write_ddrc_reg(memshire, 2, MASTER0_DbyteDllModeCntrl, 0x2);               // DllRxPreambleMode=1, Reserved=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiWrRdDataCsConfig, 0x3);
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY0_p0, 0x64);                   // Seq0BDLY0_p0=0x64
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY1_p0, 0xc8);                   // Seq0BDLY1_p0=0xc8
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY2_p0, 0x7d0);                  // Seq0BDLY2_p0=0x7d0
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY3_p0, 0x2c);                   // Seq0BDLY3_p0=0x2c
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag0, 0x0);              // Seq0BDisableFlag0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag1, 0x173);            // Seq0BDisableFlag1=0x173
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag2, 0x60);             // Seq0BDisableFlag2=0x60
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag3, 0x6110);           // Seq0BDisableFlag3=0x6110
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag4, 0x2152);           // Seq0BDisableFlag4=0x2152
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag5, 0xdfbd);           // Seq0BDisableFlag5=0xdfbd
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag6, 0x2060);           // Seq0BDisableFlag6=0x2060
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag7, 0x6152);           // Seq0BDisableFlag7=0x6152
   ms_write_ddrc_reg(memshire, 2, MASTER0_PPTTrainSetup_p0, 0x0);                // PhyMstrMaxReqToAck=0, PhyMstrTrainInterval=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_PPTTrainSetup2_p0, 0x3);               // RFU_PPTTrainSetup2_p0=3
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x0_p0, 0xe0);               // AcsmPlayback0x0_p0=0xe0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x0_p0, 0x12);               // AcsmPlayback1x0_p0=0x12
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x1_p0, 0xe0);               // AcsmPlayback0x1_p0=0xe0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x1_p0, 0x12);               // AcsmPlayback1x1_p0=0x12
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x2_p0, 0xe0);               // AcsmPlayback0x2_p0=0xe0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x2_p0, 0x12);               // AcsmPlayback1x2_p0=0x12
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCtrl13, 0xf);                        // RFU_AcsmCtrl13=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalZap, 0x1);                          // CalZap=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalRate, 0x19);                        // DisableBackgroundZQUpdates=0, CalOnce=0,
                                                                                 // CalRun=1, CalInterval=9
   ms_write_ddrc_reg(memshire, 2, DRTUB0_UcclkHclkEnables, 0x2);                 // HclkEn=1, UcclkEn=0
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x1);                // MicroContMuxSel=1

   return 0;
}

//
// DDR Phy initialization for 1067 Mhz clock (before no training).
//
uint32_t ms_init_ddr_phy_1067_pre_skiptrain (uint32_t memshire) {
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB0_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB1_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB2_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB3_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB4_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB5_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllCtrl2_p0, 0x19);                    // PllFreqSel=0x19
   ms_write_ddrc_reg(memshire, 2, MASTER0_ARdPtrInitVal_p0, 0x2);                // ARdPtrInitVal_p0=2
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BGPR4_p0, 0x0);                   // Seq0BGPR4_p0=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DqsPreambleControl_p0, 0x1a3);         // WDQSEXTENSION=1, LP4SttcPreBridgeRxEn=1,
                                                                                 // LP4PostambleExt=0, LP4TglTwoTckTxDqsPre=1,
                                                                                 // PositionDfeInit=0, TwoTckTxDqsPre=1,
                                                                                 // TwoTckRxDqsPre=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DbyteDllModeCntrl, 0x2);               // DllRxPreambleMode=1, Reserved=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DllLockParam_p0, 0x212);               // RFU_DllLockParam_p0=0x212
   ms_write_ddrc_reg(memshire, 2, MASTER0_DllGainCtl_p0, 0x61);                  // DllSeedSel=0, DllGainTV=6, DllGainIV=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_ProcOdtTimeCtl_p0, 0x3);               // POdtTailWidthExt=0, POdtStartDelay=0,
                                                                                 // POdtTailWidth=3
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, ANIB0_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB1_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB2_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB3_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB4_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB5_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiMode, 0x3);                         // Dfi1Override=0, Dfi1Enable=1, Dfi0Enable=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiCAMode, 0x4);                       // DfiD4AltCAMode=0, DfiLp4CAMode=1, DfiD4CAMode=0,
                                                                                 // DfiLp3CAMode=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalDrvStr0, 0x0);                      // CalDrvStrPu50=0, CalDrvStrPd50=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalVRefs, 0x2);                        // CalVRefs=2
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalUclkInfo_p0, 0x42b);                // CalUClkTicksPer1uS=0x42b
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalRate, 0x9);                         // DisableBackgroundZQUpdates=0, CalOnce=0,
                                                                                 // CalRun=0, CalInterval=9
   ms_write_ddrc_reg(memshire, 2, MASTER0_VrefInGlobal_p0, 0x32c);               // GlobalVrefInMode=0, GlobalVrefInTrim=0,
                                                                                 // GlobalVrefInDAC=0x65, GlobalVrefInSel=4
   ms_write_ddrc_reg(memshire, 2, DBYTE0_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqRatio_p0, 0x1);                 // DfiFreqRatio_p0=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_TristateModeCA_p0, 0x1);               // CkDisVal=0, DDR2TMode=0, DisDynAdrTri=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat0, 0x0);                    // DfiFreqXlatVal3=0, DfiFreqXlatVal2=0,
                                                                                 // DfiFreqXlatVal1=0, DfiFreqXlatVal0=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat1, 0x0);                    // DfiFreqXlatVal7=0, DfiFreqXlatVal6=0,
                                                                                 // DfiFreqXlatVal5=0, DfiFreqXlatVal4=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat2, 0x4444);                 // DfiFreqXlatVal11=4, DfiFreqXlatVal10=4,
                                                                                 // DfiFreqXlatVal9=4, DfiFreqXlatVal8=4
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat3, 0x8888);                 // DfiFreqXlatVal15=8, DfiFreqXlatVal14=8,
                                                                                 // DfiFreqXlatVal13=8, DfiFreqXlatVal12=8
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat4, 0x5555);                 // DfiFreqXlatVal19=5, DfiFreqXlatVal18=5,
                                                                                 // DfiFreqXlatVal17=5, DfiFreqXlatVal16=5
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat5, 0x0);                    // DfiFreqXlatVal23=0, DfiFreqXlatVal22=0,
                                                                                 // DfiFreqXlatVal21=0, DfiFreqXlatVal20=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat6, 0x0);                    // DfiFreqXlatVal27=0, DfiFreqXlatVal26=0,
                                                                                 // DfiFreqXlatVal25=0, DfiFreqXlatVal24=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat7, 0xf000);                 // DfiFreqXlatVal31=0xf, DfiFreqXlatVal30=0,
                                                                                 // DfiFreqXlatVal29=0, DfiFreqXlatVal28=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_MasterX4Config, 0x0);                  // X4TG=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DMIPinPresent_p0, 0x1);                // RdDbiEnabled=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_Acx4AnibDis, 0x0);                     // Acx4AnibDis=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_DFIMRL_p0, 0x8);                        // DFIMRL_p0=8
   ms_write_ddrc_reg(memshire, 2, DBYTE1_DFIMRL_p0, 0x8);                        // DFIMRL_p0=8
   ms_write_ddrc_reg(memshire, 2, DBYTE2_DFIMRL_p0, 0x8);                        // DFIMRL_p0=8
   ms_write_ddrc_reg(memshire, 2, DBYTE3_DFIMRL_p0, 0x8);                        // DFIMRL_p0=8
   ms_write_ddrc_reg(memshire, 2, MASTER0_HwtMRL_p0, 0x8);                       // HwtMRL_p0=8
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqsDlyTg0_u0_p0, 0x100);              // TxDqsDlyTg0_u0_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqsDlyTg0_u1_p0, 0x100);              // TxDqsDlyTg0_u1_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqsDlyTg0_u0_p0, 0x100);              // TxDqsDlyTg0_u0_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqsDlyTg0_u1_p0, 0x100);              // TxDqsDlyTg0_u1_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqsDlyTg0_u0_p0, 0x100);              // TxDqsDlyTg0_u0_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqsDlyTg0_u1_p0, 0x100);              // TxDqsDlyTg0_u1_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqsDlyTg0_u0_p0, 0x100);              // TxDqsDlyTg0_u0_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqsDlyTg0_u1_p0, 0x100);              // TxDqsDlyTg0_u1_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r0_p0, 0x87);                // TxDqDlyTg0_r0_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r1_p0, 0x87);                // TxDqDlyTg0_r1_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r2_p0, 0x87);                // TxDqDlyTg0_r2_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r3_p0, 0x87);                // TxDqDlyTg0_r3_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r4_p0, 0x87);                // TxDqDlyTg0_r4_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r5_p0, 0x87);                // TxDqDlyTg0_r5_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r6_p0, 0x87);                // TxDqDlyTg0_r6_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r7_p0, 0x87);                // TxDqDlyTg0_r7_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r8_p0, 0x87);                // TxDqDlyTg0_r8_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r0_p0, 0x87);                // TxDqDlyTg0_r0_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r1_p0, 0x87);                // TxDqDlyTg0_r1_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r2_p0, 0x87);                // TxDqDlyTg0_r2_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r3_p0, 0x87);                // TxDqDlyTg0_r3_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r4_p0, 0x87);                // TxDqDlyTg0_r4_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r5_p0, 0x87);                // TxDqDlyTg0_r5_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r6_p0, 0x87);                // TxDqDlyTg0_r6_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r7_p0, 0x87);                // TxDqDlyTg0_r7_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r8_p0, 0x87);                // TxDqDlyTg0_r8_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r0_p0, 0x87);                // TxDqDlyTg0_r0_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r1_p0, 0x87);                // TxDqDlyTg0_r1_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r2_p0, 0x87);                // TxDqDlyTg0_r2_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r3_p0, 0x87);                // TxDqDlyTg0_r3_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r4_p0, 0x87);                // TxDqDlyTg0_r4_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r5_p0, 0x87);                // TxDqDlyTg0_r5_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r6_p0, 0x87);                // TxDqDlyTg0_r6_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r7_p0, 0x87);                // TxDqDlyTg0_r7_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r8_p0, 0x87);                // TxDqDlyTg0_r8_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r0_p0, 0x87);                // TxDqDlyTg0_r0_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r1_p0, 0x87);                // TxDqDlyTg0_r1_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r2_p0, 0x87);                // TxDqDlyTg0_r2_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r3_p0, 0x87);                // TxDqDlyTg0_r3_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r4_p0, 0x87);                // TxDqDlyTg0_r4_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r5_p0, 0x87);                // TxDqDlyTg0_r5_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r6_p0, 0x87);                // TxDqDlyTg0_r6_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r7_p0, 0x87);                // TxDqDlyTg0_r7_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r8_p0, 0x87);                // TxDqDlyTg0_r8_p0=0x87
   ms_write_ddrc_reg(memshire, 2, DBYTE0_RxEnDlyTg0_u0_p0, 0x4d6);               // RxEnDlyTg0_u0_p0=0x4d6
   ms_write_ddrc_reg(memshire, 2, DBYTE0_RxEnDlyTg0_u1_p0, 0x4d6);               // RxEnDlyTg0_u1_p0=0x4d6
   ms_write_ddrc_reg(memshire, 2, DBYTE1_RxEnDlyTg0_u0_p0, 0x4d6);               // RxEnDlyTg0_u0_p0=0x4d6
   ms_write_ddrc_reg(memshire, 2, DBYTE1_RxEnDlyTg0_u1_p0, 0x4d6);               // RxEnDlyTg0_u1_p0=0x4d6
   ms_write_ddrc_reg(memshire, 2, DBYTE2_RxEnDlyTg0_u0_p0, 0x4d6);               // RxEnDlyTg0_u0_p0=0x4d6
   ms_write_ddrc_reg(memshire, 2, DBYTE2_RxEnDlyTg0_u1_p0, 0x4d6);               // RxEnDlyTg0_u1_p0=0x4d6
   ms_write_ddrc_reg(memshire, 2, DBYTE3_RxEnDlyTg0_u0_p0, 0x4d6);               // RxEnDlyTg0_u0_p0=0x4d6
   ms_write_ddrc_reg(memshire, 2, DBYTE3_RxEnDlyTg0_u1_p0, 0x4d6);               // RxEnDlyTg0_u1_p0=0x4d6
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BGPR1_p0, 0x2600);                // Seq0BGPR1_p0=0x2600
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BGPR2_p0, 0x10);                  // Seq0BGPR2_p0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BGPR3_p0, 0x3200);                // Seq0BGPR3_p0=0x3200
   ms_write_ddrc_reg(memshire, 2, MASTER0_HwtLpCsEnA, 0x1);                      // HwtLpCsEnA=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_HwtLpCsEnB, 0x1);                      // HwtLpCsEnB=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_PptDqsCntInvTrnTg0_p0, 0x37);           // PptDqsCntInvTrnTg0_p0=0x37
   ms_write_ddrc_reg(memshire, 2, DBYTE1_PptDqsCntInvTrnTg0_p0, 0x37);           // PptDqsCntInvTrnTg0_p0=0x37
   ms_write_ddrc_reg(memshire, 2, DBYTE2_PptDqsCntInvTrnTg0_p0, 0x37);           // PptDqsCntInvTrnTg0_p0=0x37
   ms_write_ddrc_reg(memshire, 2, DBYTE3_PptDqsCntInvTrnTg0_p0, 0x37);           // PptDqsCntInvTrnTg0_p0=0x37
   ms_write_ddrc_reg(memshire, 2, DBYTE0_PptDqsCntInvTrnTg1_p0, 0x37);           // PptDqsCntInvTrnTg1_p0=0x37
   ms_write_ddrc_reg(memshire, 2, DBYTE1_PptDqsCntInvTrnTg1_p0, 0x37);           // PptDqsCntInvTrnTg1_p0=0x37
   ms_write_ddrc_reg(memshire, 2, DBYTE2_PptDqsCntInvTrnTg1_p0, 0x37);           // PptDqsCntInvTrnTg1_p0=0x37
   ms_write_ddrc_reg(memshire, 2, DBYTE3_PptDqsCntInvTrnTg1_p0, 0x37);           // PptDqsCntInvTrnTg1_p0=0x37
   ms_write_ddrc_reg(memshire, 2, DBYTE0_PptCtlStatic, 0x501);                   // RFU_PptCtlStatic=0x501
   ms_write_ddrc_reg(memshire, 2, DBYTE1_PptCtlStatic, 0x50d);                   // RFU_PptCtlStatic=0x50d
   ms_write_ddrc_reg(memshire, 2, DBYTE2_PptCtlStatic, 0x501);                   // RFU_PptCtlStatic=0x501
   ms_write_ddrc_reg(memshire, 2, DBYTE3_PptCtlStatic, 0x50d);                   // RFU_PptCtlStatic=0x50d
   ms_write_ddrc_reg(memshire, 2, MASTER0_HwtCAMode, 0x34);                      // RFU_HwtCAMode=0x34
   ms_write_ddrc_reg(memshire, 2, MASTER0_DllGainCtl_p0, 0x54);                  // DllSeedSel=0, DllGainTV=5, DllGainIV=4
   ms_write_ddrc_reg(memshire, 2, MASTER0_DllLockParam_p0, 0x2f2);               // RFU_DllLockParam_p0=0x2f2
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCtrl23, 0x10f);                      // RFU_AcsmCtrl23=0x10f
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllCtrl3, 0x61f0);                     // PllEnCal=0, PllForceCal=1, PllDacValIn=0x10,
                                                                                 // PllMaxRange=0x1f, PllSpare=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PhyInLP3, 0x0);                       // PhyInLP3=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DFIPHYUPD, 0x0);                       // DFIPHYUPDINTTHRESHOLD=0, DFIPHYUPDTHRESHOLD=0,
                                                                                 // DFIPHYUPDMODE=0, DFIPHYUPDRESP=0, DFIPHYUPDCNT=0

   return 0;
}

//
// DDR Phy initialization for 1067 Mhz clock (after no training).
//
uint32_t ms_init_ddr_phy_1067_post_skiptrain (uint32_t memshire) {
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x0);                // MicroContMuxSel=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s0, 0x10);           // PreSequenceReg0b0s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s1, 0x400);          // PreSequenceReg0b0s1=0x400
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s2, 0x10e);          // PreSequenceReg0b0s2=0x10e
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s0, 0x0);            // PreSequenceReg0b1s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s1, 0x0);            // PreSequenceReg0b1s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s2, 0x8);            // PreSequenceReg0b1s2=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s0, 0xb);               // SequenceReg0b0s0=0xb
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s1, 0x480);             // SequenceReg0b0s1=0x480
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s2, 0x109);             // SequenceReg0b0s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s0, 0x8);               // SequenceReg0b1s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s1, 0x448);             // SequenceReg0b1s1=0x448
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s2, 0x139);             // SequenceReg0b1s2=0x139
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s0, 0x8);               // SequenceReg0b2s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s1, 0x478);             // SequenceReg0b2s1=0x478
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s2, 0x109);             // SequenceReg0b2s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s0, 0x0);               // SequenceReg0b3s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s1, 0xe8);              // SequenceReg0b3s1=0xe8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s2, 0x109);             // SequenceReg0b3s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s0, 0x2);               // SequenceReg0b4s0=2
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s1, 0x10);              // SequenceReg0b4s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s2, 0x139);             // SequenceReg0b4s2=0x139
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s0, 0xb);               // SequenceReg0b5s0=0xb
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s1, 0x7c0);             // SequenceReg0b5s1=0x7c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s2, 0x139);             // SequenceReg0b5s2=0x139
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s0, 0x44);              // SequenceReg0b6s0=0x44
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s1, 0x633);             // SequenceReg0b6s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s2, 0x159);             // SequenceReg0b6s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s0, 0x14f);             // SequenceReg0b7s0=0x14f
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s1, 0x630);             // SequenceReg0b7s1=0x630
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s2, 0x159);             // SequenceReg0b7s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s0, 0x47);              // SequenceReg0b8s0=0x47
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s1, 0x633);             // SequenceReg0b8s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s2, 0x149);             // SequenceReg0b8s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s0, 0x4f);              // SequenceReg0b9s0=0x4f
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s1, 0x633);             // SequenceReg0b9s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s2, 0x179);             // SequenceReg0b9s2=0x179
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s0, 0x8);              // SequenceReg0b10s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s1, 0xe0);             // SequenceReg0b10s1=0xe0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s2, 0x109);            // SequenceReg0b10s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s0, 0x0);              // SequenceReg0b11s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s1, 0x7c8);            // SequenceReg0b11s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s2, 0x109);            // SequenceReg0b11s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s0, 0x0);              // SequenceReg0b12s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s1, 0x1);              // SequenceReg0b12s1=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s2, 0x8);              // SequenceReg0b12s2=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s0, 0x0);              // SequenceReg0b13s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s1, 0x45a);            // SequenceReg0b13s1=0x45a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s2, 0x9);              // SequenceReg0b13s2=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s0, 0x0);              // SequenceReg0b14s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s1, 0x448);            // SequenceReg0b14s1=0x448
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s2, 0x109);            // SequenceReg0b14s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s0, 0x40);             // SequenceReg0b15s0=0x40
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s1, 0x633);            // SequenceReg0b15s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s2, 0x179);            // SequenceReg0b15s2=0x179
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s0, 0x1);              // SequenceReg0b16s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s1, 0x618);            // SequenceReg0b16s1=0x618
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s2, 0x109);            // SequenceReg0b16s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s0, 0x40c0);           // SequenceReg0b17s0=0x40c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s1, 0x633);            // SequenceReg0b17s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s2, 0x149);            // SequenceReg0b17s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s0, 0x8);              // SequenceReg0b18s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s1, 0x4);              // SequenceReg0b18s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s2, 0x48);             // SequenceReg0b18s2=0x48
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s0, 0x4040);           // SequenceReg0b19s0=0x4040
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s1, 0x633);            // SequenceReg0b19s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s2, 0x149);            // SequenceReg0b19s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s0, 0x0);              // SequenceReg0b20s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s1, 0x4);              // SequenceReg0b20s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s2, 0x48);             // SequenceReg0b20s2=0x48
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s0, 0x40);             // SequenceReg0b21s0=0x40
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s1, 0x633);            // SequenceReg0b21s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s2, 0x149);            // SequenceReg0b21s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s0, 0x10);             // SequenceReg0b22s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s1, 0x4);              // SequenceReg0b22s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s2, 0x18);             // SequenceReg0b22s2=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s0, 0x0);              // SequenceReg0b23s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s1, 0x4);              // SequenceReg0b23s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s2, 0x78);             // SequenceReg0b23s2=0x78
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s0, 0x549);            // SequenceReg0b24s0=0x549
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s1, 0x633);            // SequenceReg0b24s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s2, 0x159);            // SequenceReg0b24s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s0, 0xd49);            // SequenceReg0b25s0=0xd49
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s1, 0x633);            // SequenceReg0b25s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s2, 0x159);            // SequenceReg0b25s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s0, 0x94a);            // SequenceReg0b26s0=0x94a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s1, 0x633);            // SequenceReg0b26s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s2, 0x159);            // SequenceReg0b26s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s0, 0x441);            // SequenceReg0b27s0=0x441
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s1, 0x633);            // SequenceReg0b27s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s2, 0x149);            // SequenceReg0b27s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s0, 0x42);             // SequenceReg0b28s0=0x42
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s1, 0x633);            // SequenceReg0b28s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s2, 0x149);            // SequenceReg0b28s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s0, 0x1);              // SequenceReg0b29s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s1, 0x633);            // SequenceReg0b29s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s2, 0x149);            // SequenceReg0b29s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s0, 0x0);              // SequenceReg0b30s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s1, 0xe0);             // SequenceReg0b30s1=0xe0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s2, 0x109);            // SequenceReg0b30s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s0, 0xa);              // SequenceReg0b31s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s1, 0x10);             // SequenceReg0b31s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s2, 0x109);            // SequenceReg0b31s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s0, 0x9);              // SequenceReg0b32s0=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s1, 0x3c0);            // SequenceReg0b32s1=0x3c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s2, 0x149);            // SequenceReg0b32s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s0, 0x9);              // SequenceReg0b33s0=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s1, 0x3c0);            // SequenceReg0b33s1=0x3c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s2, 0x159);            // SequenceReg0b33s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s0, 0x18);             // SequenceReg0b34s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s1, 0x10);             // SequenceReg0b34s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s2, 0x109);            // SequenceReg0b34s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s0, 0x0);              // SequenceReg0b35s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s1, 0x3c0);            // SequenceReg0b35s1=0x3c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s2, 0x109);            // SequenceReg0b35s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s0, 0x18);             // SequenceReg0b36s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s1, 0x4);              // SequenceReg0b36s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s2, 0x48);             // SequenceReg0b36s2=0x48
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s0, 0x18);             // SequenceReg0b37s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s1, 0x4);              // SequenceReg0b37s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s2, 0x58);             // SequenceReg0b37s2=0x58
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s0, 0xb);              // SequenceReg0b38s0=0xb
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s1, 0x10);             // SequenceReg0b38s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s2, 0x109);            // SequenceReg0b38s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s0, 0x1);              // SequenceReg0b39s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s1, 0x10);             // SequenceReg0b39s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s2, 0x109);            // SequenceReg0b39s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s0, 0x5);              // SequenceReg0b40s0=5
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s1, 0x7c0);            // SequenceReg0b40s1=0x7c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s2, 0x109);            // SequenceReg0b40s2=0x109
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x0, 0x811);                      // RFU_AcsmSeq0x0=0x811
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x0, 0x880);                      // RFU_AcsmSeq1x0=0x880
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x0, 0x0);                        // RFU_AcsmSeq2x0=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x0, 0x0);                        // RFU_AcsmSeq3x0=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x1, 0x4008);                     // RFU_AcsmSeq0x1=0x4008
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x1, 0x83);                       // RFU_AcsmSeq1x1=0x83
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x1, 0x4f);                       // RFU_AcsmSeq2x1=0x4f
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x1, 0x0);                        // RFU_AcsmSeq3x1=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x2, 0x4040);                     // RFU_AcsmSeq0x2=0x4040
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x2, 0x83);                       // RFU_AcsmSeq1x2=0x83
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x2, 0x51);                       // RFU_AcsmSeq2x2=0x51
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x2, 0x0);                        // RFU_AcsmSeq3x2=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x3, 0x811);                      // RFU_AcsmSeq0x3=0x811
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x3, 0x880);                      // RFU_AcsmSeq1x3=0x880
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x3, 0x0);                        // RFU_AcsmSeq2x3=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x3, 0x0);                        // RFU_AcsmSeq3x3=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x4, 0x720);                      // RFU_AcsmSeq0x4=0x720
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x4, 0xf);                        // RFU_AcsmSeq1x4=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x4, 0x1740);                     // RFU_AcsmSeq2x4=0x1740
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x4, 0x0);                        // RFU_AcsmSeq3x4=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x5, 0x16);                       // RFU_AcsmSeq0x5=0x16
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x5, 0x83);                       // RFU_AcsmSeq1x5=0x83
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x5, 0x4b);                       // RFU_AcsmSeq2x5=0x4b
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x5, 0x0);                        // RFU_AcsmSeq3x5=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x6, 0x716);                      // RFU_AcsmSeq0x6=0x716
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x6, 0xf);                        // RFU_AcsmSeq1x6=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x6, 0x2001);                     // RFU_AcsmSeq2x6=0x2001
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x6, 0x0);                        // RFU_AcsmSeq3x6=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x7, 0x716);                      // RFU_AcsmSeq0x7=0x716
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x7, 0xf);                        // RFU_AcsmSeq1x7=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x7, 0x2800);                     // RFU_AcsmSeq2x7=0x2800
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x7, 0x0);                        // RFU_AcsmSeq3x7=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x8, 0x716);                      // RFU_AcsmSeq0x8=0x716
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x8, 0xf);                        // RFU_AcsmSeq1x8=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x8, 0xf00);                      // RFU_AcsmSeq2x8=0xf00
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x8, 0x0);                        // RFU_AcsmSeq3x8=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x9, 0x720);                      // RFU_AcsmSeq0x9=0x720
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x9, 0xf);                        // RFU_AcsmSeq1x9=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x9, 0x1400);                     // RFU_AcsmSeq2x9=0x1400
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x9, 0x0);                        // RFU_AcsmSeq3x9=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x10, 0xe08);                     // RFU_AcsmSeq0x10=0xe08
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x10, 0xc15);                     // RFU_AcsmSeq1x10=0xc15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x10, 0x0);                       // RFU_AcsmSeq2x10=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x10, 0x0);                       // RFU_AcsmSeq3x10=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x11, 0x625);                     // RFU_AcsmSeq0x11=0x625
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x11, 0x15);                      // RFU_AcsmSeq1x11=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x11, 0x0);                       // RFU_AcsmSeq2x11=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x11, 0x0);                       // RFU_AcsmSeq3x11=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x12, 0x4028);                    // RFU_AcsmSeq0x12=0x4028
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x12, 0x80);                      // RFU_AcsmSeq1x12=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x12, 0x0);                       // RFU_AcsmSeq2x12=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x12, 0x0);                       // RFU_AcsmSeq3x12=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x13, 0xe08);                     // RFU_AcsmSeq0x13=0xe08
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x13, 0xc1a);                     // RFU_AcsmSeq1x13=0xc1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x13, 0x0);                       // RFU_AcsmSeq2x13=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x13, 0x0);                       // RFU_AcsmSeq3x13=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x14, 0x625);                     // RFU_AcsmSeq0x14=0x625
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x14, 0x1a);                      // RFU_AcsmSeq1x14=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x14, 0x0);                       // RFU_AcsmSeq2x14=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x14, 0x0);                       // RFU_AcsmSeq3x14=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x15, 0x4040);                    // RFU_AcsmSeq0x15=0x4040
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x15, 0x80);                      // RFU_AcsmSeq1x15=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x15, 0x0);                       // RFU_AcsmSeq2x15=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x15, 0x0);                       // RFU_AcsmSeq3x15=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x16, 0x2604);                    // RFU_AcsmSeq0x16=0x2604
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x16, 0x15);                      // RFU_AcsmSeq1x16=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x16, 0x0);                       // RFU_AcsmSeq2x16=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x16, 0x0);                       // RFU_AcsmSeq3x16=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x17, 0x708);                     // RFU_AcsmSeq0x17=0x708
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x17, 0x5);                       // RFU_AcsmSeq1x17=5
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x17, 0x0);                       // RFU_AcsmSeq2x17=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x17, 0x2002);                    // RFU_AcsmSeq3x17=0x2002
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x18, 0x8);                       // RFU_AcsmSeq0x18=8
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x18, 0x80);                      // RFU_AcsmSeq1x18=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x18, 0x0);                       // RFU_AcsmSeq2x18=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x18, 0x0);                       // RFU_AcsmSeq3x18=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x19, 0x2604);                    // RFU_AcsmSeq0x19=0x2604
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x19, 0x1a);                      // RFU_AcsmSeq1x19=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x19, 0x0);                       // RFU_AcsmSeq2x19=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x19, 0x0);                       // RFU_AcsmSeq3x19=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x20, 0x708);                     // RFU_AcsmSeq0x20=0x708
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x20, 0xa);                       // RFU_AcsmSeq1x20=0xa
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x20, 0x0);                       // RFU_AcsmSeq2x20=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x20, 0x2002);                    // RFU_AcsmSeq3x20=0x2002
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x21, 0x4040);                    // RFU_AcsmSeq0x21=0x4040
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x21, 0x80);                      // RFU_AcsmSeq1x21=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x21, 0x0);                       // RFU_AcsmSeq2x21=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x21, 0x0);                       // RFU_AcsmSeq3x21=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x22, 0x60a);                     // RFU_AcsmSeq0x22=0x60a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x22, 0x15);                      // RFU_AcsmSeq1x22=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x22, 0x1200);                    // RFU_AcsmSeq2x22=0x1200
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x22, 0x0);                       // RFU_AcsmSeq3x22=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x23, 0x61a);                     // RFU_AcsmSeq0x23=0x61a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x23, 0x15);                      // RFU_AcsmSeq1x23=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x23, 0x1300);                    // RFU_AcsmSeq2x23=0x1300
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x23, 0x0);                       // RFU_AcsmSeq3x23=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x24, 0x60a);                     // RFU_AcsmSeq0x24=0x60a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x24, 0x1a);                      // RFU_AcsmSeq1x24=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x24, 0x1200);                    // RFU_AcsmSeq2x24=0x1200
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x24, 0x0);                       // RFU_AcsmSeq3x24=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x25, 0x642);                     // RFU_AcsmSeq0x25=0x642
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x25, 0x1a);                      // RFU_AcsmSeq1x25=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x25, 0x1300);                    // RFU_AcsmSeq2x25=0x1300
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x25, 0x0);                       // RFU_AcsmSeq3x25=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x26, 0x4808);                    // RFU_AcsmSeq0x26=0x4808
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x26, 0x880);                     // RFU_AcsmSeq1x26=0x880
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x26, 0x0);                       // RFU_AcsmSeq2x26=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x26, 0x0);                       // RFU_AcsmSeq3x26=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s0, 0x0);              // SequenceReg0b41s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s1, 0x790);            // SequenceReg0b41s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s2, 0x11a);            // SequenceReg0b41s2=0x11a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s0, 0x8);              // SequenceReg0b42s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s1, 0x7aa);            // SequenceReg0b42s1=0x7aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s2, 0x2a);             // SequenceReg0b42s2=0x2a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s0, 0x10);             // SequenceReg0b43s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s1, 0x7b2);            // SequenceReg0b43s1=0x7b2
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s2, 0x2a);             // SequenceReg0b43s2=0x2a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s0, 0x0);              // SequenceReg0b44s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s1, 0x7c8);            // SequenceReg0b44s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s2, 0x109);            // SequenceReg0b44s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s0, 0x10);             // SequenceReg0b45s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s1, 0x10);             // SequenceReg0b45s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s2, 0x109);            // SequenceReg0b45s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s0, 0x10);             // SequenceReg0b46s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s1, 0x2a8);            // SequenceReg0b46s1=0x2a8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s2, 0x129);            // SequenceReg0b46s2=0x129
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s0, 0x8);              // SequenceReg0b47s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s1, 0x370);            // SequenceReg0b47s1=0x370
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s2, 0x129);            // SequenceReg0b47s2=0x129
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s0, 0xa);              // SequenceReg0b48s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s1, 0x3c8);            // SequenceReg0b48s1=0x3c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s2, 0x1a9);            // SequenceReg0b48s2=0x1a9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s0, 0xc);              // SequenceReg0b49s0=0xc
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s1, 0x408);            // SequenceReg0b49s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s2, 0x199);            // SequenceReg0b49s2=0x199
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s0, 0x14);             // SequenceReg0b50s0=0x14
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s1, 0x790);            // SequenceReg0b50s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s2, 0x11a);            // SequenceReg0b50s2=0x11a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s0, 0x8);              // SequenceReg0b51s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s1, 0x4);              // SequenceReg0b51s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s2, 0x18);             // SequenceReg0b51s2=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s0, 0xe);              // SequenceReg0b52s0=0xe
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s1, 0x408);            // SequenceReg0b52s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s2, 0x199);            // SequenceReg0b52s2=0x199
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s0, 0x8);              // SequenceReg0b53s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s1, 0x8568);           // SequenceReg0b53s1=0x8568
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s2, 0x108);            // SequenceReg0b53s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s0, 0x18);             // SequenceReg0b54s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s1, 0x790);            // SequenceReg0b54s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s2, 0x16a);            // SequenceReg0b54s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s0, 0x8);              // SequenceReg0b55s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s1, 0x1d8);            // SequenceReg0b55s1=0x1d8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s2, 0x169);            // SequenceReg0b55s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s0, 0x10);             // SequenceReg0b56s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s1, 0x8558);           // SequenceReg0b56s1=0x8558
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s2, 0x168);            // SequenceReg0b56s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s0, 0x70);             // SequenceReg0b57s0=0x70
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s1, 0x788);            // SequenceReg0b57s1=0x788
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s2, 0x16a);            // SequenceReg0b57s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s0, 0x1ff8);           // SequenceReg0b58s0=0x1ff8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s1, 0x85a8);           // SequenceReg0b58s1=0x85a8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s2, 0x1e8);            // SequenceReg0b58s2=0x1e8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s0, 0x50);             // SequenceReg0b59s0=0x50
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s1, 0x798);            // SequenceReg0b59s1=0x798
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s2, 0x16a);            // SequenceReg0b59s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s0, 0x60);             // SequenceReg0b60s0=0x60
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s1, 0x7a0);            // SequenceReg0b60s1=0x7a0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s2, 0x16a);            // SequenceReg0b60s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s0, 0x8);              // SequenceReg0b61s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s1, 0x8310);           // SequenceReg0b61s1=0x8310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s2, 0x168);            // SequenceReg0b61s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s0, 0x8);              // SequenceReg0b62s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s1, 0xa310);           // SequenceReg0b62s1=0xa310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s2, 0x168);            // SequenceReg0b62s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s0, 0xa);              // SequenceReg0b63s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s1, 0x408);            // SequenceReg0b63s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s2, 0x169);            // SequenceReg0b63s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s0, 0x6e);             // SequenceReg0b64s0=0x6e
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s1, 0x0);              // SequenceReg0b64s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s2, 0x68);             // SequenceReg0b64s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s0, 0x0);              // SequenceReg0b65s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s1, 0x408);            // SequenceReg0b65s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s2, 0x169);            // SequenceReg0b65s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s0, 0x0);              // SequenceReg0b66s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s1, 0x8310);           // SequenceReg0b66s1=0x8310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s2, 0x168);            // SequenceReg0b66s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s0, 0x0);              // SequenceReg0b67s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s1, 0xa310);           // SequenceReg0b67s1=0xa310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s2, 0x168);            // SequenceReg0b67s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s0, 0x1ff8);           // SequenceReg0b68s0=0x1ff8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s1, 0x85a8);           // SequenceReg0b68s1=0x85a8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s2, 0x1e8);            // SequenceReg0b68s2=0x1e8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s0, 0x68);             // SequenceReg0b69s0=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s1, 0x798);            // SequenceReg0b69s1=0x798
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s2, 0x16a);            // SequenceReg0b69s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s0, 0x78);             // SequenceReg0b70s0=0x78
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s1, 0x7a0);            // SequenceReg0b70s1=0x7a0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s2, 0x16a);            // SequenceReg0b70s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s0, 0x68);             // SequenceReg0b71s0=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s1, 0x790);            // SequenceReg0b71s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s2, 0x16a);            // SequenceReg0b71s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s0, 0x8);              // SequenceReg0b72s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s1, 0x8b10);           // SequenceReg0b72s1=0x8b10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s2, 0x168);            // SequenceReg0b72s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s0, 0x8);              // SequenceReg0b73s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s1, 0xab10);           // SequenceReg0b73s1=0xab10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s2, 0x168);            // SequenceReg0b73s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s0, 0xa);              // SequenceReg0b74s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s1, 0x408);            // SequenceReg0b74s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s2, 0x169);            // SequenceReg0b74s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s0, 0x58);             // SequenceReg0b75s0=0x58
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s1, 0x0);              // SequenceReg0b75s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s2, 0x68);             // SequenceReg0b75s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s0, 0x0);              // SequenceReg0b76s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s1, 0x408);            // SequenceReg0b76s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s2, 0x169);            // SequenceReg0b76s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s0, 0x0);              // SequenceReg0b77s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s1, 0x8b10);           // SequenceReg0b77s1=0x8b10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s2, 0x168);            // SequenceReg0b77s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s0, 0x1);              // SequenceReg0b78s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s1, 0xab10);           // SequenceReg0b78s1=0xab10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s2, 0x168);            // SequenceReg0b78s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s0, 0x0);              // SequenceReg0b79s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s1, 0x1d8);            // SequenceReg0b79s1=0x1d8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s2, 0x169);            // SequenceReg0b79s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s0, 0x80);             // SequenceReg0b80s0=0x80
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s1, 0x790);            // SequenceReg0b80s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s2, 0x16a);            // SequenceReg0b80s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s0, 0x18);             // SequenceReg0b81s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s1, 0x7aa);            // SequenceReg0b81s1=0x7aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s2, 0x6a);             // SequenceReg0b81s2=0x6a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s0, 0xa);              // SequenceReg0b82s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s1, 0x0);              // SequenceReg0b82s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s2, 0x1e9);            // SequenceReg0b82s2=0x1e9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s0, 0x8);              // SequenceReg0b83s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s1, 0x8080);           // SequenceReg0b83s1=0x8080
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s2, 0x108);            // SequenceReg0b83s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s0, 0xf);              // SequenceReg0b84s0=0xf
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s1, 0x408);            // SequenceReg0b84s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s2, 0x169);            // SequenceReg0b84s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s0, 0xc);              // SequenceReg0b85s0=0xc
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s1, 0x0);              // SequenceReg0b85s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s2, 0x68);             // SequenceReg0b85s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s0, 0x9);              // SequenceReg0b86s0=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s1, 0x0);              // SequenceReg0b86s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s2, 0x1a9);            // SequenceReg0b86s2=0x1a9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s0, 0x0);              // SequenceReg0b87s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s1, 0x408);            // SequenceReg0b87s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s2, 0x169);            // SequenceReg0b87s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s0, 0x0);              // SequenceReg0b88s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s1, 0x8080);           // SequenceReg0b88s1=0x8080
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s2, 0x108);            // SequenceReg0b88s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s0, 0x8);              // SequenceReg0b89s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s1, 0x7aa);            // SequenceReg0b89s1=0x7aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s2, 0x6a);             // SequenceReg0b89s2=0x6a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s0, 0x0);              // SequenceReg0b90s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s1, 0x8568);           // SequenceReg0b90s1=0x8568
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s2, 0x108);            // SequenceReg0b90s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s0, 0xb7);             // SequenceReg0b91s0=0xb7
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s1, 0x790);            // SequenceReg0b91s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s2, 0x16a);            // SequenceReg0b91s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s0, 0x1f);             // SequenceReg0b92s0=0x1f
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s1, 0x0);              // SequenceReg0b92s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s2, 0x68);             // SequenceReg0b92s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s0, 0x8);              // SequenceReg0b93s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s1, 0x8558);           // SequenceReg0b93s1=0x8558
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s2, 0x168);            // SequenceReg0b93s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s0, 0xf);              // SequenceReg0b94s0=0xf
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s1, 0x408);            // SequenceReg0b94s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s2, 0x169);            // SequenceReg0b94s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s0, 0xd);              // SequenceReg0b95s0=0xd
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s1, 0x0);              // SequenceReg0b95s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s2, 0x68);             // SequenceReg0b95s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s0, 0x0);              // SequenceReg0b96s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s1, 0x408);            // SequenceReg0b96s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s2, 0x169);            // SequenceReg0b96s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s0, 0x0);              // SequenceReg0b97s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s1, 0x8558);           // SequenceReg0b97s1=0x8558
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s2, 0x168);            // SequenceReg0b97s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s0, 0x8);              // SequenceReg0b98s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s1, 0x3c8);            // SequenceReg0b98s1=0x3c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s2, 0x1a9);            // SequenceReg0b98s2=0x1a9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s0, 0x3);              // SequenceReg0b99s0=3
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s1, 0x370);            // SequenceReg0b99s1=0x370
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s2, 0x129);            // SequenceReg0b99s2=0x129
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s0, 0x20);            // SequenceReg0b100s0=0x20
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s1, 0x2aa);           // SequenceReg0b100s1=0x2aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s2, 0x9);             // SequenceReg0b100s2=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s0, 0x8);             // SequenceReg0b101s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s1, 0xe8);            // SequenceReg0b101s1=0xe8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s2, 0x109);           // SequenceReg0b101s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s0, 0x0);             // SequenceReg0b102s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s1, 0x8140);          // SequenceReg0b102s1=0x8140
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s2, 0x10c);           // SequenceReg0b102s2=0x10c
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s0, 0x10);            // SequenceReg0b103s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s1, 0x8138);          // SequenceReg0b103s1=0x8138
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s2, 0x104);           // SequenceReg0b103s2=0x104
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s0, 0x8);             // SequenceReg0b104s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s1, 0x448);           // SequenceReg0b104s1=0x448
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s2, 0x109);           // SequenceReg0b104s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s0, 0xf);             // SequenceReg0b105s0=0xf
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s1, 0x7c0);           // SequenceReg0b105s1=0x7c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s2, 0x109);           // SequenceReg0b105s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s0, 0x0);             // SequenceReg0b106s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s1, 0xe8);            // SequenceReg0b106s1=0xe8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s2, 0x109);           // SequenceReg0b106s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s0, 0x47);            // SequenceReg0b107s0=0x47
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s1, 0x630);           // SequenceReg0b107s1=0x630
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s2, 0x109);           // SequenceReg0b107s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s0, 0x8);             // SequenceReg0b108s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s1, 0x618);           // SequenceReg0b108s1=0x618
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s2, 0x109);           // SequenceReg0b108s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s0, 0x8);             // SequenceReg0b109s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s1, 0xe0);            // SequenceReg0b109s1=0xe0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s2, 0x109);           // SequenceReg0b109s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s0, 0x0);             // SequenceReg0b110s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s1, 0x7c8);           // SequenceReg0b110s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s2, 0x109);           // SequenceReg0b110s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s0, 0x8);             // SequenceReg0b111s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s1, 0x8140);          // SequenceReg0b111s1=0x8140
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s2, 0x10c);           // SequenceReg0b111s2=0x10c
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s0, 0x0);             // SequenceReg0b112s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s1, 0x478);           // SequenceReg0b112s1=0x478
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s2, 0x109);           // SequenceReg0b112s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s0, 0x0);             // SequenceReg0b113s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s1, 0x1);             // SequenceReg0b113s1=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s2, 0x8);             // SequenceReg0b113s2=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s0, 0x8);             // SequenceReg0b114s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s1, 0x4);             // SequenceReg0b114s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s2, 0x0);             // SequenceReg0b114s2=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s0, 0x8);           // PostSequenceReg0b0s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s1, 0x7c8);         // PostSequenceReg0b0s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s2, 0x109);         // PostSequenceReg0b0s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s0, 0x0);           // PostSequenceReg0b1s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s1, 0x400);         // PostSequenceReg0b1s1=0x400
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s2, 0x106);         // PostSequenceReg0b1s2=0x106
   ms_write_ddrc_reg(memshire, 2, APBONLY0_SequencerOverride, 0x400);            // RFU_SequencerOverride=0x400
   ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b0, 0x0);                 // RFU_StartVector0b0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b8, 0x29);                // RFU_StartVector0b8=0x29
   ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b15, 0x68);               // RFU_StartVector0b15=0x68
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl0, 0x0);                    // RFU_AcsmCsMapCtrl0=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl1, 0x101);                  // RFU_AcsmCsMapCtrl1=0x101
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl2, 0x105);                  // RFU_AcsmCsMapCtrl2=0x105
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl3, 0x107);                  // RFU_AcsmCsMapCtrl3=0x107
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl4, 0x10f);                  // RFU_AcsmCsMapCtrl4=0x10f
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl5, 0x202);                  // RFU_AcsmCsMapCtrl5=0x202
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl6, 0x20a);                  // RFU_AcsmCsMapCtrl6=0x20a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl7, 0x20b);                  // RFU_AcsmCsMapCtrl7=0x20b
   ms_write_ddrc_reg(memshire, 2, MASTER0_DbyteDllModeCntrl, 0x2);               // DllRxPreambleMode=1, Reserved=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiWrRdDataCsConfig, 0x3);
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY0_p0, 0x85);                   // Seq0BDLY0_p0=0x85
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY1_p0, 0x10a);                  // Seq0BDLY1_p0=0x10a
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY2_p0, 0xa6b);                  // Seq0BDLY2_p0=0xa6b
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY3_p0, 0x2c);                   // Seq0BDLY3_p0=0x2c
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag0, 0x0);              // Seq0BDisableFlag0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag1, 0x173);            // Seq0BDisableFlag1=0x173
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag2, 0x60);             // Seq0BDisableFlag2=0x60
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag3, 0x6110);           // Seq0BDisableFlag3=0x6110
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag4, 0x2152);           // Seq0BDisableFlag4=0x2152
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag5, 0xdfbd);           // Seq0BDisableFlag5=0xdfbd
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag6, 0xffff);           // Seq0BDisableFlag6=0xffff
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag7, 0x6152);           // Seq0BDisableFlag7=0x6152
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x0_p0, 0xe0);               // AcsmPlayback0x0_p0=0xe0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x0_p0, 0x12);               // AcsmPlayback1x0_p0=0x12
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x1_p0, 0xe0);               // AcsmPlayback0x1_p0=0xe0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x1_p0, 0x12);               // AcsmPlayback1x1_p0=0x12
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x2_p0, 0xe0);               // AcsmPlayback0x2_p0=0xe0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x2_p0, 0x12);               // AcsmPlayback1x2_p0=0x12
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCtrl13, 0xf);                        // RFU_AcsmCtrl13=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalZap, 0x1);                          // CalZap=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalRate, 0x19);                        // DisableBackgroundZQUpdates=0, CalOnce=0,
                                                                                 // CalRun=1, CalInterval=9
   ms_write_ddrc_reg(memshire, 2, DRTUB0_UcclkHclkEnables, 0x2);                 // HclkEn=1, UcclkEn=0
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x1);                // MicroContMuxSel=1

   return 0;
}

//
// DDR Phy initialization for 933 Mhz clock (before no training).
//
uint32_t ms_init_ddr_phy_933_pre_skiptrain (uint32_t memshire) {
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB0_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB1_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB2_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB3_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB4_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB5_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllCtrl2_p0, 0x19);                    // PllFreqSel=0x19
   ms_write_ddrc_reg(memshire, 2, MASTER0_ARdPtrInitVal_p0, 0x2);                // ARdPtrInitVal_p0=2
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BGPR4_p0, 0x0);                   // Seq0BGPR4_p0=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DqsPreambleControl_p0, 0x1a3);         // WDQSEXTENSION=1, LP4SttcPreBridgeRxEn=1,
                                                                                 // LP4PostambleExt=0, LP4TglTwoTckTxDqsPre=1,
                                                                                 // PositionDfeInit=0, TwoTckTxDqsPre=1,
                                                                                 // TwoTckRxDqsPre=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DbyteDllModeCntrl, 0x2);               // DllRxPreambleMode=1, Reserved=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DllLockParam_p0, 0x212);               // RFU_DllLockParam_p0=0x212
   ms_write_ddrc_reg(memshire, 2, MASTER0_DllGainCtl_p0, 0x61);                  // DllSeedSel=0, DllGainTV=6, DllGainIV=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_ProcOdtTimeCtl_p0, 0x3);               // POdtTailWidthExt=0, POdtStartDelay=0,
                                                                                 // POdtTailWidth=3
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, ANIB0_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB1_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB2_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB3_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB4_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB5_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiMode, 0x3);                         // Dfi1Override=0, Dfi1Enable=1, Dfi0Enable=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiCAMode, 0x4);                       // DfiD4AltCAMode=0, DfiLp4CAMode=1, DfiD4CAMode=0,
                                                                                 // DfiLp3CAMode=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalDrvStr0, 0x0);                      // CalDrvStrPu50=0, CalDrvStrPd50=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalVRefs, 0x2);                        // CalVRefs=2
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalUclkInfo_p0, 0x3a5);                // CalUClkTicksPer1uS=0x3a5
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalRate, 0x9);                         // DisableBackgroundZQUpdates=0, CalOnce=0,
                                                                                 // CalRun=0, CalInterval=9
   ms_write_ddrc_reg(memshire, 2, MASTER0_VrefInGlobal_p0, 0x32c);               // GlobalVrefInMode=0, GlobalVrefInTrim=0,
                                                                                 // GlobalVrefInDAC=0x65, GlobalVrefInSel=4
   ms_write_ddrc_reg(memshire, 2, DBYTE0_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqRatio_p0, 0x1);                 // DfiFreqRatio_p0=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_TristateModeCA_p0, 0x1);               // CkDisVal=0, DDR2TMode=0, DisDynAdrTri=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat0, 0x0);                    // DfiFreqXlatVal3=0, DfiFreqXlatVal2=0,
                                                                                 // DfiFreqXlatVal1=0, DfiFreqXlatVal0=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat1, 0x0);                    // DfiFreqXlatVal7=0, DfiFreqXlatVal6=0,
                                                                                 // DfiFreqXlatVal5=0, DfiFreqXlatVal4=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat2, 0x4444);                 // DfiFreqXlatVal11=4, DfiFreqXlatVal10=4,
                                                                                 // DfiFreqXlatVal9=4, DfiFreqXlatVal8=4
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat3, 0x8888);                 // DfiFreqXlatVal15=8, DfiFreqXlatVal14=8,
                                                                                 // DfiFreqXlatVal13=8, DfiFreqXlatVal12=8
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat4, 0x5555);                 // DfiFreqXlatVal19=5, DfiFreqXlatVal18=5,
                                                                                 // DfiFreqXlatVal17=5, DfiFreqXlatVal16=5
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat5, 0x0);                    // DfiFreqXlatVal23=0, DfiFreqXlatVal22=0,
                                                                                 // DfiFreqXlatVal21=0, DfiFreqXlatVal20=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat6, 0x0);                    // DfiFreqXlatVal27=0, DfiFreqXlatVal26=0,
                                                                                 // DfiFreqXlatVal25=0, DfiFreqXlatVal24=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat7, 0xf000);                 // DfiFreqXlatVal31=0xf, DfiFreqXlatVal30=0,
                                                                                 // DfiFreqXlatVal29=0, DfiFreqXlatVal28=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_MasterX4Config, 0x0);                  // X4TG=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DMIPinPresent_p0, 0x1);                // RdDbiEnabled=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_Acx4AnibDis, 0x0);                     // Acx4AnibDis=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_DFIMRL_p0, 0x8);                        // DFIMRL_p0=8
   ms_write_ddrc_reg(memshire, 2, DBYTE1_DFIMRL_p0, 0x8);                        // DFIMRL_p0=8
   ms_write_ddrc_reg(memshire, 2, DBYTE2_DFIMRL_p0, 0x8);                        // DFIMRL_p0=8
   ms_write_ddrc_reg(memshire, 2, DBYTE3_DFIMRL_p0, 0x8);                        // DFIMRL_p0=8
   ms_write_ddrc_reg(memshire, 2, MASTER0_HwtMRL_p0, 0x8);                       // HwtMRL_p0=8
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqsDlyTg0_u0_p0, 0x100);              // TxDqsDlyTg0_u0_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqsDlyTg0_u1_p0, 0x100);              // TxDqsDlyTg0_u1_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqsDlyTg0_u0_p0, 0x100);              // TxDqsDlyTg0_u0_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqsDlyTg0_u1_p0, 0x100);              // TxDqsDlyTg0_u1_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqsDlyTg0_u0_p0, 0x100);              // TxDqsDlyTg0_u0_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqsDlyTg0_u1_p0, 0x100);              // TxDqsDlyTg0_u1_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqsDlyTg0_u0_p0, 0x100);              // TxDqsDlyTg0_u0_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqsDlyTg0_u1_p0, 0x100);              // TxDqsDlyTg0_u1_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r0_p0, 0x80);                // TxDqDlyTg0_r0_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r1_p0, 0x80);                // TxDqDlyTg0_r1_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r2_p0, 0x80);                // TxDqDlyTg0_r2_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r3_p0, 0x80);                // TxDqDlyTg0_r3_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r4_p0, 0x80);                // TxDqDlyTg0_r4_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r5_p0, 0x80);                // TxDqDlyTg0_r5_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r6_p0, 0x80);                // TxDqDlyTg0_r6_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r7_p0, 0x80);                // TxDqDlyTg0_r7_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r8_p0, 0x80);                // TxDqDlyTg0_r8_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r0_p0, 0x80);                // TxDqDlyTg0_r0_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r1_p0, 0x80);                // TxDqDlyTg0_r1_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r2_p0, 0x80);                // TxDqDlyTg0_r2_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r3_p0, 0x80);                // TxDqDlyTg0_r3_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r4_p0, 0x80);                // TxDqDlyTg0_r4_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r5_p0, 0x80);                // TxDqDlyTg0_r5_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r6_p0, 0x80);                // TxDqDlyTg0_r6_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r7_p0, 0x80);                // TxDqDlyTg0_r7_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r8_p0, 0x80);                // TxDqDlyTg0_r8_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r0_p0, 0x80);                // TxDqDlyTg0_r0_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r1_p0, 0x80);                // TxDqDlyTg0_r1_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r2_p0, 0x80);                // TxDqDlyTg0_r2_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r3_p0, 0x80);                // TxDqDlyTg0_r3_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r4_p0, 0x80);                // TxDqDlyTg0_r4_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r5_p0, 0x80);                // TxDqDlyTg0_r5_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r6_p0, 0x80);                // TxDqDlyTg0_r6_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r7_p0, 0x80);                // TxDqDlyTg0_r7_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r8_p0, 0x80);                // TxDqDlyTg0_r8_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r0_p0, 0x80);                // TxDqDlyTg0_r0_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r1_p0, 0x80);                // TxDqDlyTg0_r1_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r2_p0, 0x80);                // TxDqDlyTg0_r2_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r3_p0, 0x80);                // TxDqDlyTg0_r3_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r4_p0, 0x80);                // TxDqDlyTg0_r4_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r5_p0, 0x80);                // TxDqDlyTg0_r5_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r6_p0, 0x80);                // TxDqDlyTg0_r6_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r7_p0, 0x80);                // TxDqDlyTg0_r7_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r8_p0, 0x80);                // TxDqDlyTg0_r8_p0=0x80
   ms_write_ddrc_reg(memshire, 2, DBYTE0_RxEnDlyTg0_u0_p0, 0x459);               // RxEnDlyTg0_u0_p0=0x459
   ms_write_ddrc_reg(memshire, 2, DBYTE0_RxEnDlyTg0_u1_p0, 0x459);               // RxEnDlyTg0_u1_p0=0x459
   ms_write_ddrc_reg(memshire, 2, DBYTE1_RxEnDlyTg0_u0_p0, 0x459);               // RxEnDlyTg0_u0_p0=0x459
   ms_write_ddrc_reg(memshire, 2, DBYTE1_RxEnDlyTg0_u1_p0, 0x459);               // RxEnDlyTg0_u1_p0=0x459
   ms_write_ddrc_reg(memshire, 2, DBYTE2_RxEnDlyTg0_u0_p0, 0x459);               // RxEnDlyTg0_u0_p0=0x459
   ms_write_ddrc_reg(memshire, 2, DBYTE2_RxEnDlyTg0_u1_p0, 0x459);               // RxEnDlyTg0_u1_p0=0x459
   ms_write_ddrc_reg(memshire, 2, DBYTE3_RxEnDlyTg0_u0_p0, 0x459);               // RxEnDlyTg0_u0_p0=0x459
   ms_write_ddrc_reg(memshire, 2, DBYTE3_RxEnDlyTg0_u1_p0, 0x459);               // RxEnDlyTg0_u1_p0=0x459
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BGPR1_p0, 0x2200);                // Seq0BGPR1_p0=0x2200
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BGPR2_p0, 0xe);                   // Seq0BGPR2_p0=0xe
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BGPR3_p0, 0x2e00);                // Seq0BGPR3_p0=0x2e00
   ms_write_ddrc_reg(memshire, 2, MASTER0_HwtLpCsEnA, 0x1);                      // HwtLpCsEnA=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_HwtLpCsEnB, 0x1);                      // HwtLpCsEnB=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_PptDqsCntInvTrnTg0_p0, 0x30);           // PptDqsCntInvTrnTg0_p0=0x30
   ms_write_ddrc_reg(memshire, 2, DBYTE1_PptDqsCntInvTrnTg0_p0, 0x30);           // PptDqsCntInvTrnTg0_p0=0x30
   ms_write_ddrc_reg(memshire, 2, DBYTE2_PptDqsCntInvTrnTg0_p0, 0x30);           // PptDqsCntInvTrnTg0_p0=0x30
   ms_write_ddrc_reg(memshire, 2, DBYTE3_PptDqsCntInvTrnTg0_p0, 0x30);           // PptDqsCntInvTrnTg0_p0=0x30
   ms_write_ddrc_reg(memshire, 2, DBYTE0_PptDqsCntInvTrnTg1_p0, 0x30);           // PptDqsCntInvTrnTg1_p0=0x30
   ms_write_ddrc_reg(memshire, 2, DBYTE1_PptDqsCntInvTrnTg1_p0, 0x30);           // PptDqsCntInvTrnTg1_p0=0x30
   ms_write_ddrc_reg(memshire, 2, DBYTE2_PptDqsCntInvTrnTg1_p0, 0x30);           // PptDqsCntInvTrnTg1_p0=0x30
   ms_write_ddrc_reg(memshire, 2, DBYTE3_PptDqsCntInvTrnTg1_p0, 0x30);           // PptDqsCntInvTrnTg1_p0=0x30
   ms_write_ddrc_reg(memshire, 2, DBYTE0_PptCtlStatic, 0x501);                   // RFU_PptCtlStatic=0x501
   ms_write_ddrc_reg(memshire, 2, DBYTE1_PptCtlStatic, 0x50d);                   // RFU_PptCtlStatic=0x50d
   ms_write_ddrc_reg(memshire, 2, DBYTE2_PptCtlStatic, 0x501);                   // RFU_PptCtlStatic=0x501
   ms_write_ddrc_reg(memshire, 2, DBYTE3_PptCtlStatic, 0x50d);                   // RFU_PptCtlStatic=0x50d
   ms_write_ddrc_reg(memshire, 2, MASTER0_HwtCAMode, 0x34);                      // RFU_HwtCAMode=0x34
   ms_write_ddrc_reg(memshire, 2, MASTER0_DllGainCtl_p0, 0x54);                  // DllSeedSel=0, DllGainTV=5, DllGainIV=4
   ms_write_ddrc_reg(memshire, 2, MASTER0_DllLockParam_p0, 0x362);               // RFU_DllLockParam_p0=0x362
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCtrl23, 0x10f);                      // RFU_AcsmCtrl23=0x10f
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllCtrl3, 0x61f0);                     // PllEnCal=0, PllForceCal=1, PllDacValIn=0x10,
                                                                                 // PllMaxRange=0x1f, PllSpare=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PhyInLP3, 0x0);                       // PhyInLP3=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DFIPHYUPD, 0x0);                       // DFIPHYUPDINTTHRESHOLD=0, DFIPHYUPDTHRESHOLD=0,
                                                                                 // DFIPHYUPDMODE=0, DFIPHYUPDRESP=0, DFIPHYUPDCNT=0

   return 0;
}

//
// DDR Phy initialization for 933 Mhz clock (after no training).
//
uint32_t ms_init_ddr_phy_933_post_skiptrain (uint32_t memshire) {
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x0);                // MicroContMuxSel=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s0, 0x10);           // PreSequenceReg0b0s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s1, 0x400);          // PreSequenceReg0b0s1=0x400
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s2, 0x10e);          // PreSequenceReg0b0s2=0x10e
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s0, 0x0);            // PreSequenceReg0b1s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s1, 0x0);            // PreSequenceReg0b1s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s2, 0x8);            // PreSequenceReg0b1s2=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s0, 0xb);               // SequenceReg0b0s0=0xb
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s1, 0x480);             // SequenceReg0b0s1=0x480
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s2, 0x109);             // SequenceReg0b0s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s0, 0x8);               // SequenceReg0b1s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s1, 0x448);             // SequenceReg0b1s1=0x448
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s2, 0x139);             // SequenceReg0b1s2=0x139
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s0, 0x8);               // SequenceReg0b2s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s1, 0x478);             // SequenceReg0b2s1=0x478
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s2, 0x109);             // SequenceReg0b2s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s0, 0x0);               // SequenceReg0b3s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s1, 0xe8);              // SequenceReg0b3s1=0xe8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s2, 0x109);             // SequenceReg0b3s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s0, 0x2);               // SequenceReg0b4s0=2
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s1, 0x10);              // SequenceReg0b4s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s2, 0x139);             // SequenceReg0b4s2=0x139
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s0, 0xb);               // SequenceReg0b5s0=0xb
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s1, 0x7c0);             // SequenceReg0b5s1=0x7c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s2, 0x139);             // SequenceReg0b5s2=0x139
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s0, 0x44);              // SequenceReg0b6s0=0x44
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s1, 0x633);             // SequenceReg0b6s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s2, 0x159);             // SequenceReg0b6s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s0, 0x14f);             // SequenceReg0b7s0=0x14f
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s1, 0x630);             // SequenceReg0b7s1=0x630
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s2, 0x159);             // SequenceReg0b7s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s0, 0x47);              // SequenceReg0b8s0=0x47
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s1, 0x633);             // SequenceReg0b8s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s2, 0x149);             // SequenceReg0b8s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s0, 0x4f);              // SequenceReg0b9s0=0x4f
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s1, 0x633);             // SequenceReg0b9s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s2, 0x179);             // SequenceReg0b9s2=0x179
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s0, 0x8);              // SequenceReg0b10s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s1, 0xe0);             // SequenceReg0b10s1=0xe0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s2, 0x109);            // SequenceReg0b10s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s0, 0x0);              // SequenceReg0b11s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s1, 0x7c8);            // SequenceReg0b11s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s2, 0x109);            // SequenceReg0b11s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s0, 0x0);              // SequenceReg0b12s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s1, 0x1);              // SequenceReg0b12s1=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s2, 0x8);              // SequenceReg0b12s2=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s0, 0x0);              // SequenceReg0b13s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s1, 0x45a);            // SequenceReg0b13s1=0x45a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s2, 0x9);              // SequenceReg0b13s2=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s0, 0x0);              // SequenceReg0b14s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s1, 0x448);            // SequenceReg0b14s1=0x448
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s2, 0x109);            // SequenceReg0b14s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s0, 0x40);             // SequenceReg0b15s0=0x40
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s1, 0x633);            // SequenceReg0b15s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s2, 0x179);            // SequenceReg0b15s2=0x179
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s0, 0x1);              // SequenceReg0b16s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s1, 0x618);            // SequenceReg0b16s1=0x618
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s2, 0x109);            // SequenceReg0b16s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s0, 0x40c0);           // SequenceReg0b17s0=0x40c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s1, 0x633);            // SequenceReg0b17s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s2, 0x149);            // SequenceReg0b17s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s0, 0x8);              // SequenceReg0b18s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s1, 0x4);              // SequenceReg0b18s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s2, 0x48);             // SequenceReg0b18s2=0x48
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s0, 0x4040);           // SequenceReg0b19s0=0x4040
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s1, 0x633);            // SequenceReg0b19s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s2, 0x149);            // SequenceReg0b19s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s0, 0x0);              // SequenceReg0b20s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s1, 0x4);              // SequenceReg0b20s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s2, 0x48);             // SequenceReg0b20s2=0x48
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s0, 0x40);             // SequenceReg0b21s0=0x40
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s1, 0x633);            // SequenceReg0b21s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s2, 0x149);            // SequenceReg0b21s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s0, 0x10);             // SequenceReg0b22s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s1, 0x4);              // SequenceReg0b22s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s2, 0x18);             // SequenceReg0b22s2=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s0, 0x0);              // SequenceReg0b23s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s1, 0x4);              // SequenceReg0b23s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s2, 0x78);             // SequenceReg0b23s2=0x78
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s0, 0x549);            // SequenceReg0b24s0=0x549
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s1, 0x633);            // SequenceReg0b24s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s2, 0x159);            // SequenceReg0b24s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s0, 0xd49);            // SequenceReg0b25s0=0xd49
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s1, 0x633);            // SequenceReg0b25s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s2, 0x159);            // SequenceReg0b25s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s0, 0x94a);            // SequenceReg0b26s0=0x94a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s1, 0x633);            // SequenceReg0b26s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s2, 0x159);            // SequenceReg0b26s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s0, 0x441);            // SequenceReg0b27s0=0x441
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s1, 0x633);            // SequenceReg0b27s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s2, 0x149);            // SequenceReg0b27s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s0, 0x42);             // SequenceReg0b28s0=0x42
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s1, 0x633);            // SequenceReg0b28s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s2, 0x149);            // SequenceReg0b28s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s0, 0x1);              // SequenceReg0b29s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s1, 0x633);            // SequenceReg0b29s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s2, 0x149);            // SequenceReg0b29s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s0, 0x0);              // SequenceReg0b30s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s1, 0xe0);             // SequenceReg0b30s1=0xe0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s2, 0x109);            // SequenceReg0b30s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s0, 0xa);              // SequenceReg0b31s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s1, 0x10);             // SequenceReg0b31s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s2, 0x109);            // SequenceReg0b31s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s0, 0x9);              // SequenceReg0b32s0=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s1, 0x3c0);            // SequenceReg0b32s1=0x3c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s2, 0x149);            // SequenceReg0b32s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s0, 0x9);              // SequenceReg0b33s0=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s1, 0x3c0);            // SequenceReg0b33s1=0x3c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s2, 0x159);            // SequenceReg0b33s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s0, 0x18);             // SequenceReg0b34s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s1, 0x10);             // SequenceReg0b34s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s2, 0x109);            // SequenceReg0b34s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s0, 0x0);              // SequenceReg0b35s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s1, 0x3c0);            // SequenceReg0b35s1=0x3c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s2, 0x109);            // SequenceReg0b35s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s0, 0x18);             // SequenceReg0b36s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s1, 0x4);              // SequenceReg0b36s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s2, 0x48);             // SequenceReg0b36s2=0x48
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s0, 0x18);             // SequenceReg0b37s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s1, 0x4);              // SequenceReg0b37s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s2, 0x58);             // SequenceReg0b37s2=0x58
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s0, 0xb);              // SequenceReg0b38s0=0xb
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s1, 0x10);             // SequenceReg0b38s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s2, 0x109);            // SequenceReg0b38s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s0, 0x1);              // SequenceReg0b39s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s1, 0x10);             // SequenceReg0b39s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s2, 0x109);            // SequenceReg0b39s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s0, 0x5);              // SequenceReg0b40s0=5
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s1, 0x7c0);            // SequenceReg0b40s1=0x7c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s2, 0x109);            // SequenceReg0b40s2=0x109
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x0, 0x811);                      // RFU_AcsmSeq0x0=0x811
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x0, 0x880);                      // RFU_AcsmSeq1x0=0x880
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x0, 0x0);                        // RFU_AcsmSeq2x0=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x0, 0x0);                        // RFU_AcsmSeq3x0=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x1, 0x4008);                     // RFU_AcsmSeq0x1=0x4008
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x1, 0x83);                       // RFU_AcsmSeq1x1=0x83
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x1, 0x4f);                       // RFU_AcsmSeq2x1=0x4f
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x1, 0x0);                        // RFU_AcsmSeq3x1=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x2, 0x4040);                     // RFU_AcsmSeq0x2=0x4040
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x2, 0x83);                       // RFU_AcsmSeq1x2=0x83
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x2, 0x51);                       // RFU_AcsmSeq2x2=0x51
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x2, 0x0);                        // RFU_AcsmSeq3x2=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x3, 0x811);                      // RFU_AcsmSeq0x3=0x811
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x3, 0x880);                      // RFU_AcsmSeq1x3=0x880
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x3, 0x0);                        // RFU_AcsmSeq2x3=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x3, 0x0);                        // RFU_AcsmSeq3x3=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x4, 0x720);                      // RFU_AcsmSeq0x4=0x720
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x4, 0xf);                        // RFU_AcsmSeq1x4=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x4, 0x1740);                     // RFU_AcsmSeq2x4=0x1740
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x4, 0x0);                        // RFU_AcsmSeq3x4=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x5, 0x16);                       // RFU_AcsmSeq0x5=0x16
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x5, 0x83);                       // RFU_AcsmSeq1x5=0x83
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x5, 0x4b);                       // RFU_AcsmSeq2x5=0x4b
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x5, 0x0);                        // RFU_AcsmSeq3x5=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x6, 0x716);                      // RFU_AcsmSeq0x6=0x716
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x6, 0xf);                        // RFU_AcsmSeq1x6=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x6, 0x2001);                     // RFU_AcsmSeq2x6=0x2001
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x6, 0x0);                        // RFU_AcsmSeq3x6=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x7, 0x716);                      // RFU_AcsmSeq0x7=0x716
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x7, 0xf);                        // RFU_AcsmSeq1x7=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x7, 0x2800);                     // RFU_AcsmSeq2x7=0x2800
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x7, 0x0);                        // RFU_AcsmSeq3x7=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x8, 0x716);                      // RFU_AcsmSeq0x8=0x716
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x8, 0xf);                        // RFU_AcsmSeq1x8=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x8, 0xf00);                      // RFU_AcsmSeq2x8=0xf00
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x8, 0x0);                        // RFU_AcsmSeq3x8=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x9, 0x720);                      // RFU_AcsmSeq0x9=0x720
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x9, 0xf);                        // RFU_AcsmSeq1x9=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x9, 0x1400);                     // RFU_AcsmSeq2x9=0x1400
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x9, 0x0);                        // RFU_AcsmSeq3x9=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x10, 0xe08);                     // RFU_AcsmSeq0x10=0xe08
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x10, 0xc15);                     // RFU_AcsmSeq1x10=0xc15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x10, 0x0);                       // RFU_AcsmSeq2x10=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x10, 0x0);                       // RFU_AcsmSeq3x10=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x11, 0x625);                     // RFU_AcsmSeq0x11=0x625
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x11, 0x15);                      // RFU_AcsmSeq1x11=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x11, 0x0);                       // RFU_AcsmSeq2x11=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x11, 0x0);                       // RFU_AcsmSeq3x11=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x12, 0x4028);                    // RFU_AcsmSeq0x12=0x4028
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x12, 0x80);                      // RFU_AcsmSeq1x12=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x12, 0x0);                       // RFU_AcsmSeq2x12=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x12, 0x0);                       // RFU_AcsmSeq3x12=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x13, 0xe08);                     // RFU_AcsmSeq0x13=0xe08
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x13, 0xc1a);                     // RFU_AcsmSeq1x13=0xc1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x13, 0x0);                       // RFU_AcsmSeq2x13=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x13, 0x0);                       // RFU_AcsmSeq3x13=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x14, 0x625);                     // RFU_AcsmSeq0x14=0x625
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x14, 0x1a);                      // RFU_AcsmSeq1x14=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x14, 0x0);                       // RFU_AcsmSeq2x14=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x14, 0x0);                       // RFU_AcsmSeq3x14=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x15, 0x4040);                    // RFU_AcsmSeq0x15=0x4040
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x15, 0x80);                      // RFU_AcsmSeq1x15=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x15, 0x0);                       // RFU_AcsmSeq2x15=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x15, 0x0);                       // RFU_AcsmSeq3x15=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x16, 0x2604);                    // RFU_AcsmSeq0x16=0x2604
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x16, 0x15);                      // RFU_AcsmSeq1x16=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x16, 0x0);                       // RFU_AcsmSeq2x16=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x16, 0x0);                       // RFU_AcsmSeq3x16=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x17, 0x708);                     // RFU_AcsmSeq0x17=0x708
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x17, 0x5);                       // RFU_AcsmSeq1x17=5
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x17, 0x0);                       // RFU_AcsmSeq2x17=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x17, 0x2002);                    // RFU_AcsmSeq3x17=0x2002
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x18, 0x8);                       // RFU_AcsmSeq0x18=8
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x18, 0x80);                      // RFU_AcsmSeq1x18=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x18, 0x0);                       // RFU_AcsmSeq2x18=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x18, 0x0);                       // RFU_AcsmSeq3x18=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x19, 0x2604);                    // RFU_AcsmSeq0x19=0x2604
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x19, 0x1a);                      // RFU_AcsmSeq1x19=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x19, 0x0);                       // RFU_AcsmSeq2x19=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x19, 0x0);                       // RFU_AcsmSeq3x19=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x20, 0x708);                     // RFU_AcsmSeq0x20=0x708
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x20, 0xa);                       // RFU_AcsmSeq1x20=0xa
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x20, 0x0);                       // RFU_AcsmSeq2x20=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x20, 0x2002);                    // RFU_AcsmSeq3x20=0x2002
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x21, 0x4040);                    // RFU_AcsmSeq0x21=0x4040
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x21, 0x80);                      // RFU_AcsmSeq1x21=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x21, 0x0);                       // RFU_AcsmSeq2x21=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x21, 0x0);                       // RFU_AcsmSeq3x21=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x22, 0x60a);                     // RFU_AcsmSeq0x22=0x60a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x22, 0x15);                      // RFU_AcsmSeq1x22=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x22, 0x1200);                    // RFU_AcsmSeq2x22=0x1200
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x22, 0x0);                       // RFU_AcsmSeq3x22=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x23, 0x61a);                     // RFU_AcsmSeq0x23=0x61a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x23, 0x15);                      // RFU_AcsmSeq1x23=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x23, 0x1300);                    // RFU_AcsmSeq2x23=0x1300
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x23, 0x0);                       // RFU_AcsmSeq3x23=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x24, 0x60a);                     // RFU_AcsmSeq0x24=0x60a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x24, 0x1a);                      // RFU_AcsmSeq1x24=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x24, 0x1200);                    // RFU_AcsmSeq2x24=0x1200
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x24, 0x0);                       // RFU_AcsmSeq3x24=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x25, 0x642);                     // RFU_AcsmSeq0x25=0x642
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x25, 0x1a);                      // RFU_AcsmSeq1x25=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x25, 0x1300);                    // RFU_AcsmSeq2x25=0x1300
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x25, 0x0);                       // RFU_AcsmSeq3x25=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x26, 0x4808);                    // RFU_AcsmSeq0x26=0x4808
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x26, 0x880);                     // RFU_AcsmSeq1x26=0x880
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x26, 0x0);                       // RFU_AcsmSeq2x26=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x26, 0x0);                       // RFU_AcsmSeq3x26=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s0, 0x0);              // SequenceReg0b41s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s1, 0x790);            // SequenceReg0b41s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s2, 0x11a);            // SequenceReg0b41s2=0x11a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s0, 0x8);              // SequenceReg0b42s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s1, 0x7aa);            // SequenceReg0b42s1=0x7aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s2, 0x2a);             // SequenceReg0b42s2=0x2a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s0, 0x10);             // SequenceReg0b43s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s1, 0x7b2);            // SequenceReg0b43s1=0x7b2
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s2, 0x2a);             // SequenceReg0b43s2=0x2a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s0, 0x0);              // SequenceReg0b44s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s1, 0x7c8);            // SequenceReg0b44s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s2, 0x109);            // SequenceReg0b44s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s0, 0x10);             // SequenceReg0b45s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s1, 0x10);             // SequenceReg0b45s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s2, 0x109);            // SequenceReg0b45s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s0, 0x10);             // SequenceReg0b46s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s1, 0x2a8);            // SequenceReg0b46s1=0x2a8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s2, 0x129);            // SequenceReg0b46s2=0x129
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s0, 0x8);              // SequenceReg0b47s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s1, 0x370);            // SequenceReg0b47s1=0x370
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s2, 0x129);            // SequenceReg0b47s2=0x129
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s0, 0xa);              // SequenceReg0b48s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s1, 0x3c8);            // SequenceReg0b48s1=0x3c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s2, 0x1a9);            // SequenceReg0b48s2=0x1a9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s0, 0xc);              // SequenceReg0b49s0=0xc
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s1, 0x408);            // SequenceReg0b49s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s2, 0x199);            // SequenceReg0b49s2=0x199
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s0, 0x14);             // SequenceReg0b50s0=0x14
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s1, 0x790);            // SequenceReg0b50s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s2, 0x11a);            // SequenceReg0b50s2=0x11a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s0, 0x8);              // SequenceReg0b51s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s1, 0x4);              // SequenceReg0b51s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s2, 0x18);             // SequenceReg0b51s2=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s0, 0xe);              // SequenceReg0b52s0=0xe
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s1, 0x408);            // SequenceReg0b52s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s2, 0x199);            // SequenceReg0b52s2=0x199
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s0, 0x8);              // SequenceReg0b53s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s1, 0x8568);           // SequenceReg0b53s1=0x8568
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s2, 0x108);            // SequenceReg0b53s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s0, 0x18);             // SequenceReg0b54s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s1, 0x790);            // SequenceReg0b54s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s2, 0x16a);            // SequenceReg0b54s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s0, 0x8);              // SequenceReg0b55s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s1, 0x1d8);            // SequenceReg0b55s1=0x1d8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s2, 0x169);            // SequenceReg0b55s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s0, 0x10);             // SequenceReg0b56s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s1, 0x8558);           // SequenceReg0b56s1=0x8558
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s2, 0x168);            // SequenceReg0b56s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s0, 0x70);             // SequenceReg0b57s0=0x70
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s1, 0x788);            // SequenceReg0b57s1=0x788
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s2, 0x16a);            // SequenceReg0b57s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s0, 0x1ff8);           // SequenceReg0b58s0=0x1ff8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s1, 0x85a8);           // SequenceReg0b58s1=0x85a8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s2, 0x1e8);            // SequenceReg0b58s2=0x1e8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s0, 0x50);             // SequenceReg0b59s0=0x50
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s1, 0x798);            // SequenceReg0b59s1=0x798
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s2, 0x16a);            // SequenceReg0b59s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s0, 0x60);             // SequenceReg0b60s0=0x60
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s1, 0x7a0);            // SequenceReg0b60s1=0x7a0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s2, 0x16a);            // SequenceReg0b60s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s0, 0x8);              // SequenceReg0b61s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s1, 0x8310);           // SequenceReg0b61s1=0x8310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s2, 0x168);            // SequenceReg0b61s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s0, 0x8);              // SequenceReg0b62s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s1, 0xa310);           // SequenceReg0b62s1=0xa310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s2, 0x168);            // SequenceReg0b62s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s0, 0xa);              // SequenceReg0b63s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s1, 0x408);            // SequenceReg0b63s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s2, 0x169);            // SequenceReg0b63s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s0, 0x6e);             // SequenceReg0b64s0=0x6e
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s1, 0x0);              // SequenceReg0b64s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s2, 0x68);             // SequenceReg0b64s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s0, 0x0);              // SequenceReg0b65s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s1, 0x408);            // SequenceReg0b65s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s2, 0x169);            // SequenceReg0b65s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s0, 0x0);              // SequenceReg0b66s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s1, 0x8310);           // SequenceReg0b66s1=0x8310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s2, 0x168);            // SequenceReg0b66s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s0, 0x0);              // SequenceReg0b67s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s1, 0xa310);           // SequenceReg0b67s1=0xa310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s2, 0x168);            // SequenceReg0b67s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s0, 0x1ff8);           // SequenceReg0b68s0=0x1ff8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s1, 0x85a8);           // SequenceReg0b68s1=0x85a8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s2, 0x1e8);            // SequenceReg0b68s2=0x1e8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s0, 0x68);             // SequenceReg0b69s0=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s1, 0x798);            // SequenceReg0b69s1=0x798
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s2, 0x16a);            // SequenceReg0b69s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s0, 0x78);             // SequenceReg0b70s0=0x78
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s1, 0x7a0);            // SequenceReg0b70s1=0x7a0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s2, 0x16a);            // SequenceReg0b70s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s0, 0x68);             // SequenceReg0b71s0=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s1, 0x790);            // SequenceReg0b71s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s2, 0x16a);            // SequenceReg0b71s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s0, 0x8);              // SequenceReg0b72s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s1, 0x8b10);           // SequenceReg0b72s1=0x8b10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s2, 0x168);            // SequenceReg0b72s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s0, 0x8);              // SequenceReg0b73s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s1, 0xab10);           // SequenceReg0b73s1=0xab10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s2, 0x168);            // SequenceReg0b73s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s0, 0xa);              // SequenceReg0b74s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s1, 0x408);            // SequenceReg0b74s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s2, 0x169);            // SequenceReg0b74s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s0, 0x58);             // SequenceReg0b75s0=0x58
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s1, 0x0);              // SequenceReg0b75s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s2, 0x68);             // SequenceReg0b75s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s0, 0x0);              // SequenceReg0b76s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s1, 0x408);            // SequenceReg0b76s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s2, 0x169);            // SequenceReg0b76s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s0, 0x0);              // SequenceReg0b77s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s1, 0x8b10);           // SequenceReg0b77s1=0x8b10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s2, 0x168);            // SequenceReg0b77s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s0, 0x1);              // SequenceReg0b78s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s1, 0xab10);           // SequenceReg0b78s1=0xab10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s2, 0x168);            // SequenceReg0b78s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s0, 0x0);              // SequenceReg0b79s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s1, 0x1d8);            // SequenceReg0b79s1=0x1d8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s2, 0x169);            // SequenceReg0b79s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s0, 0x80);             // SequenceReg0b80s0=0x80
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s1, 0x790);            // SequenceReg0b80s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s2, 0x16a);            // SequenceReg0b80s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s0, 0x18);             // SequenceReg0b81s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s1, 0x7aa);            // SequenceReg0b81s1=0x7aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s2, 0x6a);             // SequenceReg0b81s2=0x6a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s0, 0xa);              // SequenceReg0b82s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s1, 0x0);              // SequenceReg0b82s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s2, 0x1e9);            // SequenceReg0b82s2=0x1e9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s0, 0x8);              // SequenceReg0b83s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s1, 0x8080);           // SequenceReg0b83s1=0x8080
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s2, 0x108);            // SequenceReg0b83s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s0, 0xf);              // SequenceReg0b84s0=0xf
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s1, 0x408);            // SequenceReg0b84s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s2, 0x169);            // SequenceReg0b84s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s0, 0xc);              // SequenceReg0b85s0=0xc
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s1, 0x0);              // SequenceReg0b85s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s2, 0x68);             // SequenceReg0b85s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s0, 0x9);              // SequenceReg0b86s0=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s1, 0x0);              // SequenceReg0b86s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s2, 0x1a9);            // SequenceReg0b86s2=0x1a9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s0, 0x0);              // SequenceReg0b87s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s1, 0x408);            // SequenceReg0b87s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s2, 0x169);            // SequenceReg0b87s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s0, 0x0);              // SequenceReg0b88s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s1, 0x8080);           // SequenceReg0b88s1=0x8080
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s2, 0x108);            // SequenceReg0b88s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s0, 0x8);              // SequenceReg0b89s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s1, 0x7aa);            // SequenceReg0b89s1=0x7aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s2, 0x6a);             // SequenceReg0b89s2=0x6a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s0, 0x0);              // SequenceReg0b90s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s1, 0x8568);           // SequenceReg0b90s1=0x8568
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s2, 0x108);            // SequenceReg0b90s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s0, 0xb7);             // SequenceReg0b91s0=0xb7
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s1, 0x790);            // SequenceReg0b91s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s2, 0x16a);            // SequenceReg0b91s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s0, 0x1f);             // SequenceReg0b92s0=0x1f
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s1, 0x0);              // SequenceReg0b92s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s2, 0x68);             // SequenceReg0b92s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s0, 0x8);              // SequenceReg0b93s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s1, 0x8558);           // SequenceReg0b93s1=0x8558
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s2, 0x168);            // SequenceReg0b93s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s0, 0xf);              // SequenceReg0b94s0=0xf
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s1, 0x408);            // SequenceReg0b94s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s2, 0x169);            // SequenceReg0b94s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s0, 0xd);              // SequenceReg0b95s0=0xd
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s1, 0x0);              // SequenceReg0b95s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s2, 0x68);             // SequenceReg0b95s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s0, 0x0);              // SequenceReg0b96s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s1, 0x408);            // SequenceReg0b96s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s2, 0x169);            // SequenceReg0b96s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s0, 0x0);              // SequenceReg0b97s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s1, 0x8558);           // SequenceReg0b97s1=0x8558
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s2, 0x168);            // SequenceReg0b97s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s0, 0x8);              // SequenceReg0b98s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s1, 0x3c8);            // SequenceReg0b98s1=0x3c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s2, 0x1a9);            // SequenceReg0b98s2=0x1a9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s0, 0x3);              // SequenceReg0b99s0=3
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s1, 0x370);            // SequenceReg0b99s1=0x370
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s2, 0x129);            // SequenceReg0b99s2=0x129
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s0, 0x20);            // SequenceReg0b100s0=0x20
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s1, 0x2aa);           // SequenceReg0b100s1=0x2aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s2, 0x9);             // SequenceReg0b100s2=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s0, 0x8);             // SequenceReg0b101s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s1, 0xe8);            // SequenceReg0b101s1=0xe8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s2, 0x109);           // SequenceReg0b101s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s0, 0x0);             // SequenceReg0b102s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s1, 0x8140);          // SequenceReg0b102s1=0x8140
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s2, 0x10c);           // SequenceReg0b102s2=0x10c
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s0, 0x10);            // SequenceReg0b103s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s1, 0x8138);          // SequenceReg0b103s1=0x8138
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s2, 0x104);           // SequenceReg0b103s2=0x104
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s0, 0x8);             // SequenceReg0b104s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s1, 0x448);           // SequenceReg0b104s1=0x448
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s2, 0x109);           // SequenceReg0b104s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s0, 0xf);             // SequenceReg0b105s0=0xf
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s1, 0x7c0);           // SequenceReg0b105s1=0x7c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s2, 0x109);           // SequenceReg0b105s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s0, 0x0);             // SequenceReg0b106s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s1, 0xe8);            // SequenceReg0b106s1=0xe8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s2, 0x109);           // SequenceReg0b106s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s0, 0x47);            // SequenceReg0b107s0=0x47
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s1, 0x630);           // SequenceReg0b107s1=0x630
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s2, 0x109);           // SequenceReg0b107s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s0, 0x8);             // SequenceReg0b108s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s1, 0x618);           // SequenceReg0b108s1=0x618
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s2, 0x109);           // SequenceReg0b108s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s0, 0x8);             // SequenceReg0b109s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s1, 0xe0);            // SequenceReg0b109s1=0xe0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s2, 0x109);           // SequenceReg0b109s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s0, 0x0);             // SequenceReg0b110s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s1, 0x7c8);           // SequenceReg0b110s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s2, 0x109);           // SequenceReg0b110s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s0, 0x8);             // SequenceReg0b111s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s1, 0x8140);          // SequenceReg0b111s1=0x8140
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s2, 0x10c);           // SequenceReg0b111s2=0x10c
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s0, 0x0);             // SequenceReg0b112s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s1, 0x478);           // SequenceReg0b112s1=0x478
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s2, 0x109);           // SequenceReg0b112s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s0, 0x0);             // SequenceReg0b113s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s1, 0x1);             // SequenceReg0b113s1=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s2, 0x8);             // SequenceReg0b113s2=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s0, 0x8);             // SequenceReg0b114s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s1, 0x4);             // SequenceReg0b114s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s2, 0x0);             // SequenceReg0b114s2=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s0, 0x8);           // PostSequenceReg0b0s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s1, 0x7c8);         // PostSequenceReg0b0s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s2, 0x109);         // PostSequenceReg0b0s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s0, 0x0);           // PostSequenceReg0b1s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s1, 0x400);         // PostSequenceReg0b1s1=0x400
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s2, 0x106);         // PostSequenceReg0b1s2=0x106
   ms_write_ddrc_reg(memshire, 2, APBONLY0_SequencerOverride, 0x400);            // RFU_SequencerOverride=0x400
   ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b0, 0x0);                 // RFU_StartVector0b0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b8, 0x29);                // RFU_StartVector0b8=0x29
   ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b15, 0x68);               // RFU_StartVector0b15=0x68
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl0, 0x0);                    // RFU_AcsmCsMapCtrl0=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl1, 0x101);                  // RFU_AcsmCsMapCtrl1=0x101
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl2, 0x105);                  // RFU_AcsmCsMapCtrl2=0x105
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl3, 0x107);                  // RFU_AcsmCsMapCtrl3=0x107
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl4, 0x10f);                  // RFU_AcsmCsMapCtrl4=0x10f
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl5, 0x202);                  // RFU_AcsmCsMapCtrl5=0x202
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl6, 0x20a);                  // RFU_AcsmCsMapCtrl6=0x20a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl7, 0x20b);                  // RFU_AcsmCsMapCtrl7=0x20b
   ms_write_ddrc_reg(memshire, 2, MASTER0_DbyteDllModeCntrl, 0x2);               // DllRxPreambleMode=1, Reserved=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiWrRdDataCsConfig, 0x3);
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY0_p0, 0x74);                   // Seq0BDLY0_p0=0x74
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY1_p0, 0xe9);                   // Seq0BDLY1_p0=0xe9
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY2_p0, 0x91c);                  // Seq0BDLY2_p0=0x91c
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY3_p0, 0x2c);                   // Seq0BDLY3_p0=0x2c
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag0, 0x0);              // Seq0BDisableFlag0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag1, 0x173);            // Seq0BDisableFlag1=0x173
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag2, 0x60);             // Seq0BDisableFlag2=0x60
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag3, 0x6110);           // Seq0BDisableFlag3=0x6110
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag4, 0x2152);           // Seq0BDisableFlag4=0x2152
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag5, 0xdfbd);           // Seq0BDisableFlag5=0xdfbd
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag6, 0xffff);           // Seq0BDisableFlag6=0xffff
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag7, 0x6152);           // Seq0BDisableFlag7=0x6152
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x0_p0, 0xe0);               // AcsmPlayback0x0_p0=0xe0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x0_p0, 0x12);               // AcsmPlayback1x0_p0=0x12
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x1_p0, 0xe0);               // AcsmPlayback0x1_p0=0xe0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x1_p0, 0x12);               // AcsmPlayback1x1_p0=0x12
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x2_p0, 0xe0);               // AcsmPlayback0x2_p0=0xe0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x2_p0, 0x12);               // AcsmPlayback1x2_p0=0x12
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCtrl13, 0xf);                        // RFU_AcsmCtrl13=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalZap, 0x1);                          // CalZap=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalRate, 0x19);                        // DisableBackgroundZQUpdates=0, CalOnce=0,
                                                                                 // CalRun=1, CalInterval=9
   ms_write_ddrc_reg(memshire, 2, DRTUB0_UcclkHclkEnables, 0x2);                 // HclkEn=1, UcclkEn=0
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x1);                // MicroContMuxSel=1

   return 0;
}

//
// DDR Phy initialization for 800 Mhz clock (before no training).
//
uint32_t ms_init_ddr_phy_800_pre_skiptrain (uint32_t memshire) {
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxSlewRate_b0_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxSlewRate_b1_p0, 0x5af);               // TxPreDrvMode=5, TxPreN=0xa, TxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB0_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB1_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB2_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB3_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB4_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, ANIB5_ATxSlewRate, 0x1af);                     // ATxPreDrvMode=1, ATxPreN=0xa, ATxPreP=0xf
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllCtrl2_p0, 0x19);                    // PllFreqSel=0x19
   ms_write_ddrc_reg(memshire, 2, MASTER0_ARdPtrInitVal_p0, 0x2);                // ARdPtrInitVal_p0=2
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BGPR4_p0, 0x0);                   // Seq0BGPR4_p0=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DqsPreambleControl_p0, 0x1a3);         // WDQSEXTENSION=1, LP4SttcPreBridgeRxEn=1,
                                                                                 // LP4PostambleExt=0, LP4TglTwoTckTxDqsPre=1,
                                                                                 // PositionDfeInit=0, TwoTckTxDqsPre=1,
                                                                                 // TwoTckRxDqsPre=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DbyteDllModeCntrl, 0x2);               // DllRxPreambleMode=1, Reserved=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DllLockParam_p0, 0x212);               // RFU_DllLockParam_p0=0x212
   ms_write_ddrc_reg(memshire, 2, MASTER0_DllGainCtl_p0, 0x61);                  // DllSeedSel=0, DllGainTV=6, DllGainIV=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_ProcOdtTimeCtl_p0, 0x3);               // POdtTailWidthExt=0, POdtStartDelay=0,
                                                                                 // POdtTailWidth=3
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxOdtDrvStren_b0_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxOdtDrvStren_b1_p0, 0xe00);            // ODTStrenN=0x38, ODTStrenP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxImpedanceCtrl1_b0_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxImpedanceCtrl1_b1_p0, 0xe00);         // DrvStrenFSDqN=0x38, DrvStrenFSDqP=0
   ms_write_ddrc_reg(memshire, 2, ANIB0_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB1_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB2_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB3_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB4_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, ANIB5_ATxImpedance, 0x60);                     // ADrvStrenN=3, ADrvStrenP=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiMode, 0x3);                         // Dfi1Override=0, Dfi1Enable=1, Dfi0Enable=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiCAMode, 0x4);                       // DfiD4AltCAMode=0, DfiLp4CAMode=1, DfiD4CAMode=0,
                                                                                 // DfiLp3CAMode=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalDrvStr0, 0x0);                      // CalDrvStrPu50=0, CalDrvStrPd50=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalVRefs, 0x2);                        // CalVRefs=2
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalUclkInfo_p0, 0x320);                // CalUClkTicksPer1uS=0x320
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalRate, 0x9);                         // DisableBackgroundZQUpdates=0, CalOnce=0,
                                                                                 // CalRun=0, CalInterval=9
   ms_write_ddrc_reg(memshire, 2, MASTER0_VrefInGlobal_p0, 0x32c);               // GlobalVrefInMode=0, GlobalVrefInTrim=0,
                                                                                 // GlobalVrefInDAC=0x65, GlobalVrefInSel=4
   ms_write_ddrc_reg(memshire, 2, DBYTE0_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_DqDqsRcvCntrl_b0_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_DqDqsRcvCntrl_b1_p0, 0x5a1);            // Reserved=0, GainCurrAdj=0xb, MajorModeDbyte=2,
                                                                                 // DfeCtrl=0, ExtVrefRange=0, SelAnalogVref=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqRatio_p0, 0x1);                 // DfiFreqRatio_p0=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_TristateModeCA_p0, 0x1);               // CkDisVal=0, DDR2TMode=0, DisDynAdrTri=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat0, 0x0);                    // DfiFreqXlatVal3=0, DfiFreqXlatVal2=0,
                                                                                 // DfiFreqXlatVal1=0, DfiFreqXlatVal0=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat1, 0x0);                    // DfiFreqXlatVal7=0, DfiFreqXlatVal6=0,
                                                                                 // DfiFreqXlatVal5=0, DfiFreqXlatVal4=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat2, 0x4444);                 // DfiFreqXlatVal11=4, DfiFreqXlatVal10=4,
                                                                                 // DfiFreqXlatVal9=4, DfiFreqXlatVal8=4
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat3, 0x8888);                 // DfiFreqXlatVal15=8, DfiFreqXlatVal14=8,
                                                                                 // DfiFreqXlatVal13=8, DfiFreqXlatVal12=8
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat4, 0x5555);                 // DfiFreqXlatVal19=5, DfiFreqXlatVal18=5,
                                                                                 // DfiFreqXlatVal17=5, DfiFreqXlatVal16=5
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat5, 0x0);                    // DfiFreqXlatVal23=0, DfiFreqXlatVal22=0,
                                                                                 // DfiFreqXlatVal21=0, DfiFreqXlatVal20=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat6, 0x0);                    // DfiFreqXlatVal27=0, DfiFreqXlatVal26=0,
                                                                                 // DfiFreqXlatVal25=0, DfiFreqXlatVal24=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiFreqXlat7, 0xf000);                 // DfiFreqXlatVal31=0xf, DfiFreqXlatVal30=0,
                                                                                 // DfiFreqXlatVal29=0, DfiFreqXlatVal28=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_MasterX4Config, 0x0);                  // X4TG=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DMIPinPresent_p0, 0x1);                // RdDbiEnabled=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_Acx4AnibDis, 0x0);                     // Acx4AnibDis=0
   ms_write_ddrc_reg(memshire, 2, DBYTE0_DFIMRL_p0, 0x7);                        // DFIMRL_p0=7
   ms_write_ddrc_reg(memshire, 2, DBYTE1_DFIMRL_p0, 0x7);                        // DFIMRL_p0=7
   ms_write_ddrc_reg(memshire, 2, DBYTE2_DFIMRL_p0, 0x7);                        // DFIMRL_p0=7
   ms_write_ddrc_reg(memshire, 2, DBYTE3_DFIMRL_p0, 0x7);                        // DFIMRL_p0=7
   ms_write_ddrc_reg(memshire, 2, MASTER0_HwtMRL_p0, 0x7);                       // HwtMRL_p0=7
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqsDlyTg0_u0_p0, 0x100);              // TxDqsDlyTg0_u0_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqsDlyTg0_u1_p0, 0x100);              // TxDqsDlyTg0_u1_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqsDlyTg0_u0_p0, 0x100);              // TxDqsDlyTg0_u0_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqsDlyTg0_u1_p0, 0x100);              // TxDqsDlyTg0_u1_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqsDlyTg0_u0_p0, 0x100);              // TxDqsDlyTg0_u0_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqsDlyTg0_u1_p0, 0x100);              // TxDqsDlyTg0_u1_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqsDlyTg0_u0_p0, 0x100);              // TxDqsDlyTg0_u0_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqsDlyTg0_u1_p0, 0x100);              // TxDqsDlyTg0_u1_p0=0x100
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r0_p0, 0x59);                // TxDqDlyTg0_r0_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r1_p0, 0x59);                // TxDqDlyTg0_r1_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r2_p0, 0x59);                // TxDqDlyTg0_r2_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r3_p0, 0x59);                // TxDqDlyTg0_r3_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r4_p0, 0x59);                // TxDqDlyTg0_r4_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r5_p0, 0x59);                // TxDqDlyTg0_r5_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r6_p0, 0x59);                // TxDqDlyTg0_r6_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r7_p0, 0x59);                // TxDqDlyTg0_r7_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TxDqDlyTg0_r8_p0, 0x59);                // TxDqDlyTg0_r8_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r0_p0, 0x59);                // TxDqDlyTg0_r0_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r1_p0, 0x59);                // TxDqDlyTg0_r1_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r2_p0, 0x59);                // TxDqDlyTg0_r2_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r3_p0, 0x59);                // TxDqDlyTg0_r3_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r4_p0, 0x59);                // TxDqDlyTg0_r4_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r5_p0, 0x59);                // TxDqDlyTg0_r5_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r6_p0, 0x59);                // TxDqDlyTg0_r6_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r7_p0, 0x59);                // TxDqDlyTg0_r7_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TxDqDlyTg0_r8_p0, 0x59);                // TxDqDlyTg0_r8_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r0_p0, 0x59);                // TxDqDlyTg0_r0_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r1_p0, 0x59);                // TxDqDlyTg0_r1_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r2_p0, 0x59);                // TxDqDlyTg0_r2_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r3_p0, 0x59);                // TxDqDlyTg0_r3_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r4_p0, 0x59);                // TxDqDlyTg0_r4_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r5_p0, 0x59);                // TxDqDlyTg0_r5_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r6_p0, 0x59);                // TxDqDlyTg0_r6_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r7_p0, 0x59);                // TxDqDlyTg0_r7_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TxDqDlyTg0_r8_p0, 0x59);                // TxDqDlyTg0_r8_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r0_p0, 0x59);                // TxDqDlyTg0_r0_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r1_p0, 0x59);                // TxDqDlyTg0_r1_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r2_p0, 0x59);                // TxDqDlyTg0_r2_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r3_p0, 0x59);                // TxDqDlyTg0_r3_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r4_p0, 0x59);                // TxDqDlyTg0_r4_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r5_p0, 0x59);                // TxDqDlyTg0_r5_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r6_p0, 0x59);                // TxDqDlyTg0_r6_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r7_p0, 0x59);                // TxDqDlyTg0_r7_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TxDqDlyTg0_r8_p0, 0x59);                // TxDqDlyTg0_r8_p0=0x59
   ms_write_ddrc_reg(memshire, 2, DBYTE0_RxEnDlyTg0_u0_p0, 0x3de);               // RxEnDlyTg0_u0_p0=0x3de
   ms_write_ddrc_reg(memshire, 2, DBYTE0_RxEnDlyTg0_u1_p0, 0x3de);               // RxEnDlyTg0_u1_p0=0x3de
   ms_write_ddrc_reg(memshire, 2, DBYTE1_RxEnDlyTg0_u0_p0, 0x3de);               // RxEnDlyTg0_u0_p0=0x3de
   ms_write_ddrc_reg(memshire, 2, DBYTE1_RxEnDlyTg0_u1_p0, 0x3de);               // RxEnDlyTg0_u1_p0=0x3de
   ms_write_ddrc_reg(memshire, 2, DBYTE2_RxEnDlyTg0_u0_p0, 0x3de);               // RxEnDlyTg0_u0_p0=0x3de
   ms_write_ddrc_reg(memshire, 2, DBYTE2_RxEnDlyTg0_u1_p0, 0x3de);               // RxEnDlyTg0_u1_p0=0x3de
   ms_write_ddrc_reg(memshire, 2, DBYTE3_RxEnDlyTg0_u0_p0, 0x3de);               // RxEnDlyTg0_u0_p0=0x3de
   ms_write_ddrc_reg(memshire, 2, DBYTE3_RxEnDlyTg0_u1_p0, 0x3de);               // RxEnDlyTg0_u1_p0=0x3de
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BGPR1_p0, 0x1e00);                // Seq0BGPR1_p0=0x1e00
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BGPR2_p0, 0xc);                   // Seq0BGPR2_p0=0xc
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BGPR3_p0, 0x2a00);                // Seq0BGPR3_p0=0x2a00
   ms_write_ddrc_reg(memshire, 2, MASTER0_HwtLpCsEnA, 0x1);                      // HwtLpCsEnA=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_HwtLpCsEnB, 0x1);                      // HwtLpCsEnB=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_PptDqsCntInvTrnTg0_p0, 0x29);           // PptDqsCntInvTrnTg0_p0=0x29
   ms_write_ddrc_reg(memshire, 2, DBYTE1_PptDqsCntInvTrnTg0_p0, 0x29);           // PptDqsCntInvTrnTg0_p0=0x29
   ms_write_ddrc_reg(memshire, 2, DBYTE2_PptDqsCntInvTrnTg0_p0, 0x29);           // PptDqsCntInvTrnTg0_p0=0x29
   ms_write_ddrc_reg(memshire, 2, DBYTE3_PptDqsCntInvTrnTg0_p0, 0x29);           // PptDqsCntInvTrnTg0_p0=0x29
   ms_write_ddrc_reg(memshire, 2, DBYTE0_PptDqsCntInvTrnTg1_p0, 0x29);           // PptDqsCntInvTrnTg1_p0=0x29
   ms_write_ddrc_reg(memshire, 2, DBYTE1_PptDqsCntInvTrnTg1_p0, 0x29);           // PptDqsCntInvTrnTg1_p0=0x29
   ms_write_ddrc_reg(memshire, 2, DBYTE2_PptDqsCntInvTrnTg1_p0, 0x29);           // PptDqsCntInvTrnTg1_p0=0x29
   ms_write_ddrc_reg(memshire, 2, DBYTE3_PptDqsCntInvTrnTg1_p0, 0x29);           // PptDqsCntInvTrnTg1_p0=0x29
   ms_write_ddrc_reg(memshire, 2, DBYTE0_PptCtlStatic, 0x501);                   // RFU_PptCtlStatic=0x501
   ms_write_ddrc_reg(memshire, 2, DBYTE1_PptCtlStatic, 0x50d);                   // RFU_PptCtlStatic=0x50d
   ms_write_ddrc_reg(memshire, 2, DBYTE2_PptCtlStatic, 0x501);                   // RFU_PptCtlStatic=0x501
   ms_write_ddrc_reg(memshire, 2, DBYTE3_PptCtlStatic, 0x50d);                   // RFU_PptCtlStatic=0x50d
   ms_write_ddrc_reg(memshire, 2, MASTER0_HwtCAMode, 0x34);                      // RFU_HwtCAMode=0x34
   ms_write_ddrc_reg(memshire, 2, MASTER0_DllGainCtl_p0, 0x54);                  // DllSeedSel=0, DllGainTV=5, DllGainIV=4
   ms_write_ddrc_reg(memshire, 2, MASTER0_DllLockParam_p0, 0x3f2);               // RFU_DllLockParam_p0=0x3f2
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCtrl23, 0x10f);                      // RFU_AcsmCtrl23=0x10f
   ms_write_ddrc_reg(memshire, 2, MASTER0_PllCtrl3, 0x61f0);                     // PllEnCal=0, PllForceCal=1, PllDacValIn=0x10,
                                                                                 // PllMaxRange=0x1f, PllSpare=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PhyInLP3, 0x0);                       // PhyInLP3=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DFIPHYUPD, 0x0);                       // DFIPHYUPDINTTHRESHOLD=0, DFIPHYUPDTHRESHOLD=0,
                                                                                 // DFIPHYUPDMODE=0, DFIPHYUPDRESP=0, DFIPHYUPDCNT=0

   return 0;
}

//
// DDR Phy initialization for 800 Mhz clock (after no training).
//
uint32_t ms_init_ddr_phy_800_post_skiptrain (uint32_t memshire) {
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x0);                // MicroContMuxSel=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s0, 0x10);           // PreSequenceReg0b0s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s1, 0x400);          // PreSequenceReg0b0s1=0x400
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b0s2, 0x10e);          // PreSequenceReg0b0s2=0x10e
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s0, 0x0);            // PreSequenceReg0b1s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s1, 0x0);            // PreSequenceReg0b1s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PreSequenceReg0b1s2, 0x8);            // PreSequenceReg0b1s2=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s0, 0xb);               // SequenceReg0b0s0=0xb
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s1, 0x480);             // SequenceReg0b0s1=0x480
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b0s2, 0x109);             // SequenceReg0b0s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s0, 0x8);               // SequenceReg0b1s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s1, 0x448);             // SequenceReg0b1s1=0x448
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b1s2, 0x139);             // SequenceReg0b1s2=0x139
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s0, 0x8);               // SequenceReg0b2s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s1, 0x478);             // SequenceReg0b2s1=0x478
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b2s2, 0x109);             // SequenceReg0b2s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s0, 0x0);               // SequenceReg0b3s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s1, 0xe8);              // SequenceReg0b3s1=0xe8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b3s2, 0x109);             // SequenceReg0b3s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s0, 0x2);               // SequenceReg0b4s0=2
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s1, 0x10);              // SequenceReg0b4s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b4s2, 0x139);             // SequenceReg0b4s2=0x139
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s0, 0xb);               // SequenceReg0b5s0=0xb
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s1, 0x7c0);             // SequenceReg0b5s1=0x7c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b5s2, 0x139);             // SequenceReg0b5s2=0x139
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s0, 0x44);              // SequenceReg0b6s0=0x44
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s1, 0x633);             // SequenceReg0b6s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b6s2, 0x159);             // SequenceReg0b6s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s0, 0x14f);             // SequenceReg0b7s0=0x14f
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s1, 0x630);             // SequenceReg0b7s1=0x630
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b7s2, 0x159);             // SequenceReg0b7s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s0, 0x47);              // SequenceReg0b8s0=0x47
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s1, 0x633);             // SequenceReg0b8s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b8s2, 0x149);             // SequenceReg0b8s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s0, 0x4f);              // SequenceReg0b9s0=0x4f
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s1, 0x633);             // SequenceReg0b9s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b9s2, 0x179);             // SequenceReg0b9s2=0x179
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s0, 0x8);              // SequenceReg0b10s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s1, 0xe0);             // SequenceReg0b10s1=0xe0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b10s2, 0x109);            // SequenceReg0b10s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s0, 0x0);              // SequenceReg0b11s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s1, 0x7c8);            // SequenceReg0b11s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b11s2, 0x109);            // SequenceReg0b11s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s0, 0x0);              // SequenceReg0b12s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s1, 0x1);              // SequenceReg0b12s1=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b12s2, 0x8);              // SequenceReg0b12s2=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s0, 0x0);              // SequenceReg0b13s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s1, 0x45a);            // SequenceReg0b13s1=0x45a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b13s2, 0x9);              // SequenceReg0b13s2=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s0, 0x0);              // SequenceReg0b14s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s1, 0x448);            // SequenceReg0b14s1=0x448
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b14s2, 0x109);            // SequenceReg0b14s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s0, 0x40);             // SequenceReg0b15s0=0x40
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s1, 0x633);            // SequenceReg0b15s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b15s2, 0x179);            // SequenceReg0b15s2=0x179
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s0, 0x1);              // SequenceReg0b16s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s1, 0x618);            // SequenceReg0b16s1=0x618
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b16s2, 0x109);            // SequenceReg0b16s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s0, 0x40c0);           // SequenceReg0b17s0=0x40c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s1, 0x633);            // SequenceReg0b17s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b17s2, 0x149);            // SequenceReg0b17s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s0, 0x8);              // SequenceReg0b18s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s1, 0x4);              // SequenceReg0b18s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b18s2, 0x48);             // SequenceReg0b18s2=0x48
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s0, 0x4040);           // SequenceReg0b19s0=0x4040
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s1, 0x633);            // SequenceReg0b19s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b19s2, 0x149);            // SequenceReg0b19s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s0, 0x0);              // SequenceReg0b20s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s1, 0x4);              // SequenceReg0b20s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b20s2, 0x48);             // SequenceReg0b20s2=0x48
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s0, 0x40);             // SequenceReg0b21s0=0x40
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s1, 0x633);            // SequenceReg0b21s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b21s2, 0x149);            // SequenceReg0b21s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s0, 0x10);             // SequenceReg0b22s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s1, 0x4);              // SequenceReg0b22s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b22s2, 0x18);             // SequenceReg0b22s2=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s0, 0x0);              // SequenceReg0b23s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s1, 0x4);              // SequenceReg0b23s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b23s2, 0x78);             // SequenceReg0b23s2=0x78
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s0, 0x549);            // SequenceReg0b24s0=0x549
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s1, 0x633);            // SequenceReg0b24s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b24s2, 0x159);            // SequenceReg0b24s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s0, 0xd49);            // SequenceReg0b25s0=0xd49
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s1, 0x633);            // SequenceReg0b25s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b25s2, 0x159);            // SequenceReg0b25s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s0, 0x94a);            // SequenceReg0b26s0=0x94a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s1, 0x633);            // SequenceReg0b26s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b26s2, 0x159);            // SequenceReg0b26s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s0, 0x441);            // SequenceReg0b27s0=0x441
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s1, 0x633);            // SequenceReg0b27s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b27s2, 0x149);            // SequenceReg0b27s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s0, 0x42);             // SequenceReg0b28s0=0x42
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s1, 0x633);            // SequenceReg0b28s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b28s2, 0x149);            // SequenceReg0b28s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s0, 0x1);              // SequenceReg0b29s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s1, 0x633);            // SequenceReg0b29s1=0x633
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b29s2, 0x149);            // SequenceReg0b29s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s0, 0x0);              // SequenceReg0b30s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s1, 0xe0);             // SequenceReg0b30s1=0xe0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b30s2, 0x109);            // SequenceReg0b30s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s0, 0xa);              // SequenceReg0b31s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s1, 0x10);             // SequenceReg0b31s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b31s2, 0x109);            // SequenceReg0b31s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s0, 0x9);              // SequenceReg0b32s0=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s1, 0x3c0);            // SequenceReg0b32s1=0x3c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b32s2, 0x149);            // SequenceReg0b32s2=0x149
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s0, 0x9);              // SequenceReg0b33s0=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s1, 0x3c0);            // SequenceReg0b33s1=0x3c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b33s2, 0x159);            // SequenceReg0b33s2=0x159
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s0, 0x18);             // SequenceReg0b34s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s1, 0x10);             // SequenceReg0b34s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b34s2, 0x109);            // SequenceReg0b34s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s0, 0x0);              // SequenceReg0b35s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s1, 0x3c0);            // SequenceReg0b35s1=0x3c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b35s2, 0x109);            // SequenceReg0b35s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s0, 0x18);             // SequenceReg0b36s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s1, 0x4);              // SequenceReg0b36s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b36s2, 0x48);             // SequenceReg0b36s2=0x48
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s0, 0x18);             // SequenceReg0b37s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s1, 0x4);              // SequenceReg0b37s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b37s2, 0x58);             // SequenceReg0b37s2=0x58
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s0, 0xb);              // SequenceReg0b38s0=0xb
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s1, 0x10);             // SequenceReg0b38s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b38s2, 0x109);            // SequenceReg0b38s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s0, 0x1);              // SequenceReg0b39s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s1, 0x10);             // SequenceReg0b39s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b39s2, 0x109);            // SequenceReg0b39s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s0, 0x5);              // SequenceReg0b40s0=5
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s1, 0x7c0);            // SequenceReg0b40s1=0x7c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b40s2, 0x109);            // SequenceReg0b40s2=0x109
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x0, 0x811);                      // RFU_AcsmSeq0x0=0x811
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x0, 0x880);                      // RFU_AcsmSeq1x0=0x880
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x0, 0x0);                        // RFU_AcsmSeq2x0=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x0, 0x0);                        // RFU_AcsmSeq3x0=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x1, 0x4008);                     // RFU_AcsmSeq0x1=0x4008
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x1, 0x83);                       // RFU_AcsmSeq1x1=0x83
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x1, 0x4f);                       // RFU_AcsmSeq2x1=0x4f
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x1, 0x0);                        // RFU_AcsmSeq3x1=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x2, 0x4040);                     // RFU_AcsmSeq0x2=0x4040
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x2, 0x83);                       // RFU_AcsmSeq1x2=0x83
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x2, 0x51);                       // RFU_AcsmSeq2x2=0x51
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x2, 0x0);                        // RFU_AcsmSeq3x2=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x3, 0x811);                      // RFU_AcsmSeq0x3=0x811
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x3, 0x880);                      // RFU_AcsmSeq1x3=0x880
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x3, 0x0);                        // RFU_AcsmSeq2x3=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x3, 0x0);                        // RFU_AcsmSeq3x3=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x4, 0x720);                      // RFU_AcsmSeq0x4=0x720
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x4, 0xf);                        // RFU_AcsmSeq1x4=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x4, 0x1740);                     // RFU_AcsmSeq2x4=0x1740
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x4, 0x0);                        // RFU_AcsmSeq3x4=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x5, 0x16);                       // RFU_AcsmSeq0x5=0x16
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x5, 0x83);                       // RFU_AcsmSeq1x5=0x83
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x5, 0x4b);                       // RFU_AcsmSeq2x5=0x4b
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x5, 0x0);                        // RFU_AcsmSeq3x5=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x6, 0x716);                      // RFU_AcsmSeq0x6=0x716
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x6, 0xf);                        // RFU_AcsmSeq1x6=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x6, 0x2001);                     // RFU_AcsmSeq2x6=0x2001
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x6, 0x0);                        // RFU_AcsmSeq3x6=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x7, 0x716);                      // RFU_AcsmSeq0x7=0x716
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x7, 0xf);                        // RFU_AcsmSeq1x7=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x7, 0x2800);                     // RFU_AcsmSeq2x7=0x2800
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x7, 0x0);                        // RFU_AcsmSeq3x7=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x8, 0x716);                      // RFU_AcsmSeq0x8=0x716
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x8, 0xf);                        // RFU_AcsmSeq1x8=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x8, 0xf00);                      // RFU_AcsmSeq2x8=0xf00
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x8, 0x0);                        // RFU_AcsmSeq3x8=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x9, 0x720);                      // RFU_AcsmSeq0x9=0x720
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x9, 0xf);                        // RFU_AcsmSeq1x9=0xf
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x9, 0x1400);                     // RFU_AcsmSeq2x9=0x1400
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x9, 0x0);                        // RFU_AcsmSeq3x9=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x10, 0xe08);                     // RFU_AcsmSeq0x10=0xe08
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x10, 0xc15);                     // RFU_AcsmSeq1x10=0xc15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x10, 0x0);                       // RFU_AcsmSeq2x10=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x10, 0x0);                       // RFU_AcsmSeq3x10=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x11, 0x625);                     // RFU_AcsmSeq0x11=0x625
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x11, 0x15);                      // RFU_AcsmSeq1x11=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x11, 0x0);                       // RFU_AcsmSeq2x11=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x11, 0x0);                       // RFU_AcsmSeq3x11=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x12, 0x4028);                    // RFU_AcsmSeq0x12=0x4028
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x12, 0x80);                      // RFU_AcsmSeq1x12=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x12, 0x0);                       // RFU_AcsmSeq2x12=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x12, 0x0);                       // RFU_AcsmSeq3x12=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x13, 0xe08);                     // RFU_AcsmSeq0x13=0xe08
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x13, 0xc1a);                     // RFU_AcsmSeq1x13=0xc1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x13, 0x0);                       // RFU_AcsmSeq2x13=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x13, 0x0);                       // RFU_AcsmSeq3x13=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x14, 0x625);                     // RFU_AcsmSeq0x14=0x625
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x14, 0x1a);                      // RFU_AcsmSeq1x14=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x14, 0x0);                       // RFU_AcsmSeq2x14=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x14, 0x0);                       // RFU_AcsmSeq3x14=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x15, 0x4040);                    // RFU_AcsmSeq0x15=0x4040
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x15, 0x80);                      // RFU_AcsmSeq1x15=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x15, 0x0);                       // RFU_AcsmSeq2x15=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x15, 0x0);                       // RFU_AcsmSeq3x15=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x16, 0x2604);                    // RFU_AcsmSeq0x16=0x2604
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x16, 0x15);                      // RFU_AcsmSeq1x16=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x16, 0x0);                       // RFU_AcsmSeq2x16=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x16, 0x0);                       // RFU_AcsmSeq3x16=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x17, 0x708);                     // RFU_AcsmSeq0x17=0x708
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x17, 0x5);                       // RFU_AcsmSeq1x17=5
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x17, 0x0);                       // RFU_AcsmSeq2x17=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x17, 0x2002);                    // RFU_AcsmSeq3x17=0x2002
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x18, 0x8);                       // RFU_AcsmSeq0x18=8
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x18, 0x80);                      // RFU_AcsmSeq1x18=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x18, 0x0);                       // RFU_AcsmSeq2x18=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x18, 0x0);                       // RFU_AcsmSeq3x18=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x19, 0x2604);                    // RFU_AcsmSeq0x19=0x2604
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x19, 0x1a);                      // RFU_AcsmSeq1x19=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x19, 0x0);                       // RFU_AcsmSeq2x19=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x19, 0x0);                       // RFU_AcsmSeq3x19=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x20, 0x708);                     // RFU_AcsmSeq0x20=0x708
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x20, 0xa);                       // RFU_AcsmSeq1x20=0xa
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x20, 0x0);                       // RFU_AcsmSeq2x20=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x20, 0x2002);                    // RFU_AcsmSeq3x20=0x2002
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x21, 0x4040);                    // RFU_AcsmSeq0x21=0x4040
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x21, 0x80);                      // RFU_AcsmSeq1x21=0x80
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x21, 0x0);                       // RFU_AcsmSeq2x21=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x21, 0x0);                       // RFU_AcsmSeq3x21=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x22, 0x60a);                     // RFU_AcsmSeq0x22=0x60a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x22, 0x15);                      // RFU_AcsmSeq1x22=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x22, 0x1200);                    // RFU_AcsmSeq2x22=0x1200
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x22, 0x0);                       // RFU_AcsmSeq3x22=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x23, 0x61a);                     // RFU_AcsmSeq0x23=0x61a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x23, 0x15);                      // RFU_AcsmSeq1x23=0x15
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x23, 0x1300);                    // RFU_AcsmSeq2x23=0x1300
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x23, 0x0);                       // RFU_AcsmSeq3x23=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x24, 0x60a);                     // RFU_AcsmSeq0x24=0x60a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x24, 0x1a);                      // RFU_AcsmSeq1x24=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x24, 0x1200);                    // RFU_AcsmSeq2x24=0x1200
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x24, 0x0);                       // RFU_AcsmSeq3x24=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x25, 0x642);                     // RFU_AcsmSeq0x25=0x642
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x25, 0x1a);                      // RFU_AcsmSeq1x25=0x1a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x25, 0x1300);                    // RFU_AcsmSeq2x25=0x1300
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x25, 0x0);                       // RFU_AcsmSeq3x25=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq0x26, 0x4808);                    // RFU_AcsmSeq0x26=0x4808
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq1x26, 0x880);                     // RFU_AcsmSeq1x26=0x880
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq2x26, 0x0);                       // RFU_AcsmSeq2x26=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmSeq3x26, 0x0);                       // RFU_AcsmSeq3x26=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s0, 0x0);              // SequenceReg0b41s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s1, 0x790);            // SequenceReg0b41s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b41s2, 0x11a);            // SequenceReg0b41s2=0x11a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s0, 0x8);              // SequenceReg0b42s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s1, 0x7aa);            // SequenceReg0b42s1=0x7aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b42s2, 0x2a);             // SequenceReg0b42s2=0x2a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s0, 0x10);             // SequenceReg0b43s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s1, 0x7b2);            // SequenceReg0b43s1=0x7b2
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b43s2, 0x2a);             // SequenceReg0b43s2=0x2a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s0, 0x0);              // SequenceReg0b44s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s1, 0x7c8);            // SequenceReg0b44s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b44s2, 0x109);            // SequenceReg0b44s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s0, 0x10);             // SequenceReg0b45s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s1, 0x10);             // SequenceReg0b45s1=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b45s2, 0x109);            // SequenceReg0b45s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s0, 0x10);             // SequenceReg0b46s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s1, 0x2a8);            // SequenceReg0b46s1=0x2a8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b46s2, 0x129);            // SequenceReg0b46s2=0x129
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s0, 0x8);              // SequenceReg0b47s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s1, 0x370);            // SequenceReg0b47s1=0x370
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b47s2, 0x129);            // SequenceReg0b47s2=0x129
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s0, 0xa);              // SequenceReg0b48s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s1, 0x3c8);            // SequenceReg0b48s1=0x3c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b48s2, 0x1a9);            // SequenceReg0b48s2=0x1a9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s0, 0xc);              // SequenceReg0b49s0=0xc
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s1, 0x408);            // SequenceReg0b49s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b49s2, 0x199);            // SequenceReg0b49s2=0x199
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s0, 0x14);             // SequenceReg0b50s0=0x14
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s1, 0x790);            // SequenceReg0b50s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b50s2, 0x11a);            // SequenceReg0b50s2=0x11a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s0, 0x8);              // SequenceReg0b51s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s1, 0x4);              // SequenceReg0b51s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b51s2, 0x18);             // SequenceReg0b51s2=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s0, 0xe);              // SequenceReg0b52s0=0xe
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s1, 0x408);            // SequenceReg0b52s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b52s2, 0x199);            // SequenceReg0b52s2=0x199
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s0, 0x8);              // SequenceReg0b53s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s1, 0x8568);           // SequenceReg0b53s1=0x8568
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b53s2, 0x108);            // SequenceReg0b53s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s0, 0x18);             // SequenceReg0b54s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s1, 0x790);            // SequenceReg0b54s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b54s2, 0x16a);            // SequenceReg0b54s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s0, 0x8);              // SequenceReg0b55s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s1, 0x1d8);            // SequenceReg0b55s1=0x1d8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b55s2, 0x169);            // SequenceReg0b55s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s0, 0x10);             // SequenceReg0b56s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s1, 0x8558);           // SequenceReg0b56s1=0x8558
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b56s2, 0x168);            // SequenceReg0b56s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s0, 0x70);             // SequenceReg0b57s0=0x70
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s1, 0x788);            // SequenceReg0b57s1=0x788
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b57s2, 0x16a);            // SequenceReg0b57s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s0, 0x1ff8);           // SequenceReg0b58s0=0x1ff8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s1, 0x85a8);           // SequenceReg0b58s1=0x85a8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b58s2, 0x1e8);            // SequenceReg0b58s2=0x1e8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s0, 0x50);             // SequenceReg0b59s0=0x50
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s1, 0x798);            // SequenceReg0b59s1=0x798
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b59s2, 0x16a);            // SequenceReg0b59s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s0, 0x60);             // SequenceReg0b60s0=0x60
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s1, 0x7a0);            // SequenceReg0b60s1=0x7a0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b60s2, 0x16a);            // SequenceReg0b60s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s0, 0x8);              // SequenceReg0b61s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s1, 0x8310);           // SequenceReg0b61s1=0x8310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b61s2, 0x168);            // SequenceReg0b61s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s0, 0x8);              // SequenceReg0b62s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s1, 0xa310);           // SequenceReg0b62s1=0xa310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b62s2, 0x168);            // SequenceReg0b62s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s0, 0xa);              // SequenceReg0b63s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s1, 0x408);            // SequenceReg0b63s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b63s2, 0x169);            // SequenceReg0b63s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s0, 0x6e);             // SequenceReg0b64s0=0x6e
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s1, 0x0);              // SequenceReg0b64s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b64s2, 0x68);             // SequenceReg0b64s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s0, 0x0);              // SequenceReg0b65s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s1, 0x408);            // SequenceReg0b65s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b65s2, 0x169);            // SequenceReg0b65s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s0, 0x0);              // SequenceReg0b66s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s1, 0x8310);           // SequenceReg0b66s1=0x8310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b66s2, 0x168);            // SequenceReg0b66s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s0, 0x0);              // SequenceReg0b67s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s1, 0xa310);           // SequenceReg0b67s1=0xa310
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b67s2, 0x168);            // SequenceReg0b67s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s0, 0x1ff8);           // SequenceReg0b68s0=0x1ff8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s1, 0x85a8);           // SequenceReg0b68s1=0x85a8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b68s2, 0x1e8);            // SequenceReg0b68s2=0x1e8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s0, 0x68);             // SequenceReg0b69s0=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s1, 0x798);            // SequenceReg0b69s1=0x798
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b69s2, 0x16a);            // SequenceReg0b69s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s0, 0x78);             // SequenceReg0b70s0=0x78
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s1, 0x7a0);            // SequenceReg0b70s1=0x7a0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b70s2, 0x16a);            // SequenceReg0b70s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s0, 0x68);             // SequenceReg0b71s0=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s1, 0x790);            // SequenceReg0b71s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b71s2, 0x16a);            // SequenceReg0b71s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s0, 0x8);              // SequenceReg0b72s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s1, 0x8b10);           // SequenceReg0b72s1=0x8b10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b72s2, 0x168);            // SequenceReg0b72s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s0, 0x8);              // SequenceReg0b73s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s1, 0xab10);           // SequenceReg0b73s1=0xab10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b73s2, 0x168);            // SequenceReg0b73s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s0, 0xa);              // SequenceReg0b74s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s1, 0x408);            // SequenceReg0b74s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b74s2, 0x169);            // SequenceReg0b74s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s0, 0x58);             // SequenceReg0b75s0=0x58
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s1, 0x0);              // SequenceReg0b75s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b75s2, 0x68);             // SequenceReg0b75s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s0, 0x0);              // SequenceReg0b76s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s1, 0x408);            // SequenceReg0b76s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b76s2, 0x169);            // SequenceReg0b76s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s0, 0x0);              // SequenceReg0b77s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s1, 0x8b10);           // SequenceReg0b77s1=0x8b10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b77s2, 0x168);            // SequenceReg0b77s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s0, 0x1);              // SequenceReg0b78s0=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s1, 0xab10);           // SequenceReg0b78s1=0xab10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b78s2, 0x168);            // SequenceReg0b78s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s0, 0x0);              // SequenceReg0b79s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s1, 0x1d8);            // SequenceReg0b79s1=0x1d8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b79s2, 0x169);            // SequenceReg0b79s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s0, 0x80);             // SequenceReg0b80s0=0x80
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s1, 0x790);            // SequenceReg0b80s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b80s2, 0x16a);            // SequenceReg0b80s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s0, 0x18);             // SequenceReg0b81s0=0x18
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s1, 0x7aa);            // SequenceReg0b81s1=0x7aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b81s2, 0x6a);             // SequenceReg0b81s2=0x6a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s0, 0xa);              // SequenceReg0b82s0=0xa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s1, 0x0);              // SequenceReg0b82s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b82s2, 0x1e9);            // SequenceReg0b82s2=0x1e9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s0, 0x8);              // SequenceReg0b83s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s1, 0x8080);           // SequenceReg0b83s1=0x8080
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b83s2, 0x108);            // SequenceReg0b83s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s0, 0xf);              // SequenceReg0b84s0=0xf
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s1, 0x408);            // SequenceReg0b84s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b84s2, 0x169);            // SequenceReg0b84s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s0, 0xc);              // SequenceReg0b85s0=0xc
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s1, 0x0);              // SequenceReg0b85s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b85s2, 0x68);             // SequenceReg0b85s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s0, 0x9);              // SequenceReg0b86s0=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s1, 0x0);              // SequenceReg0b86s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b86s2, 0x1a9);            // SequenceReg0b86s2=0x1a9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s0, 0x0);              // SequenceReg0b87s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s1, 0x408);            // SequenceReg0b87s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b87s2, 0x169);            // SequenceReg0b87s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s0, 0x0);              // SequenceReg0b88s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s1, 0x8080);           // SequenceReg0b88s1=0x8080
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b88s2, 0x108);            // SequenceReg0b88s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s0, 0x8);              // SequenceReg0b89s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s1, 0x7aa);            // SequenceReg0b89s1=0x7aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b89s2, 0x6a);             // SequenceReg0b89s2=0x6a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s0, 0x0);              // SequenceReg0b90s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s1, 0x8568);           // SequenceReg0b90s1=0x8568
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b90s2, 0x108);            // SequenceReg0b90s2=0x108
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s0, 0xb7);             // SequenceReg0b91s0=0xb7
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s1, 0x790);            // SequenceReg0b91s1=0x790
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b91s2, 0x16a);            // SequenceReg0b91s2=0x16a
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s0, 0x1f);             // SequenceReg0b92s0=0x1f
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s1, 0x0);              // SequenceReg0b92s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b92s2, 0x68);             // SequenceReg0b92s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s0, 0x8);              // SequenceReg0b93s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s1, 0x8558);           // SequenceReg0b93s1=0x8558
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b93s2, 0x168);            // SequenceReg0b93s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s0, 0xf);              // SequenceReg0b94s0=0xf
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s1, 0x408);            // SequenceReg0b94s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b94s2, 0x169);            // SequenceReg0b94s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s0, 0xd);              // SequenceReg0b95s0=0xd
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s1, 0x0);              // SequenceReg0b95s1=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b95s2, 0x68);             // SequenceReg0b95s2=0x68
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s0, 0x0);              // SequenceReg0b96s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s1, 0x408);            // SequenceReg0b96s1=0x408
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b96s2, 0x169);            // SequenceReg0b96s2=0x169
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s0, 0x0);              // SequenceReg0b97s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s1, 0x8558);           // SequenceReg0b97s1=0x8558
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b97s2, 0x168);            // SequenceReg0b97s2=0x168
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s0, 0x8);              // SequenceReg0b98s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s1, 0x3c8);            // SequenceReg0b98s1=0x3c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b98s2, 0x1a9);            // SequenceReg0b98s2=0x1a9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s0, 0x3);              // SequenceReg0b99s0=3
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s1, 0x370);            // SequenceReg0b99s1=0x370
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b99s2, 0x129);            // SequenceReg0b99s2=0x129
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s0, 0x20);            // SequenceReg0b100s0=0x20
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s1, 0x2aa);           // SequenceReg0b100s1=0x2aa
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b100s2, 0x9);             // SequenceReg0b100s2=9
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s0, 0x8);             // SequenceReg0b101s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s1, 0xe8);            // SequenceReg0b101s1=0xe8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b101s2, 0x109);           // SequenceReg0b101s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s0, 0x0);             // SequenceReg0b102s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s1, 0x8140);          // SequenceReg0b102s1=0x8140
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b102s2, 0x10c);           // SequenceReg0b102s2=0x10c
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s0, 0x10);            // SequenceReg0b103s0=0x10
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s1, 0x8138);          // SequenceReg0b103s1=0x8138
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b103s2, 0x104);           // SequenceReg0b103s2=0x104
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s0, 0x8);             // SequenceReg0b104s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s1, 0x448);           // SequenceReg0b104s1=0x448
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b104s2, 0x109);           // SequenceReg0b104s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s0, 0xf);             // SequenceReg0b105s0=0xf
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s1, 0x7c0);           // SequenceReg0b105s1=0x7c0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b105s2, 0x109);           // SequenceReg0b105s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s0, 0x0);             // SequenceReg0b106s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s1, 0xe8);            // SequenceReg0b106s1=0xe8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b106s2, 0x109);           // SequenceReg0b106s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s0, 0x47);            // SequenceReg0b107s0=0x47
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s1, 0x630);           // SequenceReg0b107s1=0x630
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b107s2, 0x109);           // SequenceReg0b107s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s0, 0x8);             // SequenceReg0b108s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s1, 0x618);           // SequenceReg0b108s1=0x618
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b108s2, 0x109);           // SequenceReg0b108s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s0, 0x8);             // SequenceReg0b109s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s1, 0xe0);            // SequenceReg0b109s1=0xe0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b109s2, 0x109);           // SequenceReg0b109s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s0, 0x0);             // SequenceReg0b110s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s1, 0x7c8);           // SequenceReg0b110s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b110s2, 0x109);           // SequenceReg0b110s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s0, 0x8);             // SequenceReg0b111s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s1, 0x8140);          // SequenceReg0b111s1=0x8140
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b111s2, 0x10c);           // SequenceReg0b111s2=0x10c
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s0, 0x0);             // SequenceReg0b112s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s1, 0x478);           // SequenceReg0b112s1=0x478
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b112s2, 0x109);           // SequenceReg0b112s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s0, 0x0);             // SequenceReg0b113s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s1, 0x1);             // SequenceReg0b113s1=1
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b113s2, 0x8);             // SequenceReg0b113s2=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s0, 0x8);             // SequenceReg0b114s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s1, 0x4);             // SequenceReg0b114s1=4
   ms_write_ddrc_reg(memshire, 2, INITENG0_SequenceReg0b114s2, 0x0);             // SequenceReg0b114s2=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s0, 0x8);           // PostSequenceReg0b0s0=8
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s1, 0x7c8);         // PostSequenceReg0b0s1=0x7c8
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b0s2, 0x109);         // PostSequenceReg0b0s2=0x109
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s0, 0x0);           // PostSequenceReg0b1s0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s1, 0x400);         // PostSequenceReg0b1s1=0x400
   ms_write_ddrc_reg(memshire, 2, INITENG0_PostSequenceReg0b1s2, 0x106);         // PostSequenceReg0b1s2=0x106
   ms_write_ddrc_reg(memshire, 2, APBONLY0_SequencerOverride, 0x400);            // RFU_SequencerOverride=0x400
   ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b0, 0x0);                 // RFU_StartVector0b0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b8, 0x29);                // RFU_StartVector0b8=0x29
   ms_write_ddrc_reg(memshire, 2, INITENG0_StartVector0b15, 0x68);               // RFU_StartVector0b15=0x68
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl0, 0x0);                    // RFU_AcsmCsMapCtrl0=0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl1, 0x101);                  // RFU_AcsmCsMapCtrl1=0x101
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl2, 0x105);                  // RFU_AcsmCsMapCtrl2=0x105
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl3, 0x107);                  // RFU_AcsmCsMapCtrl3=0x107
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl4, 0x10f);                  // RFU_AcsmCsMapCtrl4=0x10f
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl5, 0x202);                  // RFU_AcsmCsMapCtrl5=0x202
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl6, 0x20a);                  // RFU_AcsmCsMapCtrl6=0x20a
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCsMapCtrl7, 0x20b);                  // RFU_AcsmCsMapCtrl7=0x20b
   ms_write_ddrc_reg(memshire, 2, MASTER0_DbyteDllModeCntrl, 0x2);               // DllRxPreambleMode=1, Reserved=0
   ms_write_ddrc_reg(memshire, 2, MASTER0_DfiWrRdDataCsConfig, 0x3);
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY0_p0, 0x64);                   // Seq0BDLY0_p0=0x64
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY1_p0, 0xc8);                   // Seq0BDLY1_p0=0xc8
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY2_p0, 0x7d0);                  // Seq0BDLY2_p0=0x7d0
   ms_write_ddrc_reg(memshire, 2, MASTER0_Seq0BDLY3_p0, 0x2c);                   // Seq0BDLY3_p0=0x2c
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag0, 0x0);              // Seq0BDisableFlag0=0
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag1, 0x173);            // Seq0BDisableFlag1=0x173
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag2, 0x60);             // Seq0BDisableFlag2=0x60
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag3, 0x6110);           // Seq0BDisableFlag3=0x6110
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag4, 0x2152);           // Seq0BDisableFlag4=0x2152
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag5, 0xdfbd);           // Seq0BDisableFlag5=0xdfbd
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag6, 0xffff);           // Seq0BDisableFlag6=0xffff
   ms_write_ddrc_reg(memshire, 2, INITENG0_Seq0BDisableFlag7, 0x6152);           // Seq0BDisableFlag7=0x6152
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x0_p0, 0xe0);               // AcsmPlayback0x0_p0=0xe0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x0_p0, 0x12);               // AcsmPlayback1x0_p0=0x12
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x1_p0, 0xe0);               // AcsmPlayback0x1_p0=0xe0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x1_p0, 0x12);               // AcsmPlayback1x1_p0=0x12
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback0x2_p0, 0xe0);               // AcsmPlayback0x2_p0=0xe0
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmPlayback1x2_p0, 0x12);               // AcsmPlayback1x2_p0=0x12
   ms_write_ddrc_reg(memshire, 2, ACSM0_AcsmCtrl13, 0xf);                        // RFU_AcsmCtrl13=0xf
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE0_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE1_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE2_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte1, 0x1);                         // RFU_TsmByte1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte2, 0x1);                         // RFU_TsmByte2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte3, 0x180);                       // RFU_TsmByte3=0x180
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TsmByte5, 0x1);                         // RFU_TsmByte5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_TrainingParam, 0x6209);                 // RFU_TrainingParam=0x6209
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm0_i0, 0x1);                          // RFU_Tsm0_i0=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i1, 0x1);                          // RFU_Tsm2_i1=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i2, 0x1);                          // RFU_Tsm2_i2=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i3, 0x1);                          // RFU_Tsm2_i3=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i4, 0x1);                          // RFU_Tsm2_i4=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i5, 0x1);                          // RFU_Tsm2_i5=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i6, 0x1);                          // RFU_Tsm2_i6=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i7, 0x1);                          // RFU_Tsm2_i7=1
   ms_write_ddrc_reg(memshire, 2, DBYTE3_Tsm2_i8, 0x1);                          // RFU_Tsm2_i8=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalZap, 0x1);                          // CalZap=1
   ms_write_ddrc_reg(memshire, 2, MASTER0_CalRate, 0x19);                        // DisableBackgroundZQUpdates=0, CalOnce=0,
                                                                                 // CalRun=1, CalInterval=9
   ms_write_ddrc_reg(memshire, 2, DRTUB0_UcclkHclkEnables, 0x2);                 // HclkEn=1, UcclkEn=0
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x1);                // MicroContMuxSel=1

   return 0;
}


uint32_t ms_init_config_ddrc (uint32_t memshire, uint32_t config_ecc, uint32_t config_real_pll, uint32_t config_800mhz, uint32_t config_933mhz, uint32_t config_training, uint32_t config_4gb, uint32_t config_8gb, uint32_t config_32gb, uint32_t config_sim_only) {

   if (config_800mhz) {
      if (config_training) {
         if (config_sim_only) {
            ms_write_both_ddrc_reg(memshire, INIT0, 0xc0020002);                 // skip_dram_init=0x3, post_cke_x1024=0x2, pre_cke_x1024=0x2
                                                                                 // skip_dram_init=3, post_cke_x1024=2,
                                                                                 // pre_cke_x1024=2
            ms_write_both_ddrc_reg(memshire, INIT1, 0x00020000);                 // dram_rstn_x1024=0x2, pre_ocd_x32=0x0
                                                                                 // dram_rstn_x1024=2
         } else {
            ms_write_both_ddrc_reg(memshire, INIT0, 0xc002061b);                 // skip_dram_init=0x3, post_cke_x1024=0x2, pre_cke_x1024=0x61b
                                                                                 // skip_dram_init=3, post_cke_x1024=2
            ms_write_both_ddrc_reg(memshire, INIT1, 0x01000000);                 // dram_rstn_x1024=0x100, pre_ocd_x32=0x0
         }
         ms_write_both_ddrc_reg(memshire, INIT4, 0x00f10008);                    // emr2=0xf1, emr3=0x8
                                                                                 // emr3=8
      } else {
         ms_write_both_ddrc_reg(memshire, INIT0, 0x00020002);                    // skip_dram_init=0x0, post_cke_x1024=0x2, pre_cke_x1024=0x2
                                                                                 // post_cke_x1024=2, pre_cke_x1024=2
         ms_write_both_ddrc_reg(memshire, INIT1, 0x00020000);                    // dram_rstn_x1024=0x2, pre_ocd_x32=0x0
                                                                                 // dram_rstn_x1024=2
         ms_write_both_ddrc_reg(memshire, INIT4, 0x00f10000);                    // emr2=0xf1, emr3=0x0
      }
      ms_write_both_ddrc_reg(memshire, DFITMG0, 0x029b820a);                     // dfi_t_ctrl_delay=0x2, dfi_rddata_use_dfi_phy_clk=0x1, dfi_t_rddata_en=0x1b, dfi_wrdata_use_dfi_phy_clk=0x1, dfi_tphy_wrdata=0x2, dfi_tphy_wrlat=0xa
                                                                                 // dfi_t_ctrl_delay=2, dfi_rddata_use_dfi_phy_clk=1,
                                                                                 // dfi_t_rddata_en=27, dfi_wrdata_use_dfi_phy_clk=1,
                                                                                 // dfi_tphy_wrdata=2, dfi_tphy_wrlat=10
      ms_write_both_ddrc_reg(memshire, DFITMG1, 0x000a0101);                     // dfi_t_wrdata_delay=0xa, dfi_t_dram_clk_disable=0x1, dfi_t_dram_clk_enable=0x1
                                                                                 // dfi_t_wrdata_delay=10, dfi_t_dram_clk_disable=1,
                                                                                 // dfi_t_dram_clk_enable=1
      ms_write_both_ddrc_reg(memshire, DFITMG2, 0x00001b09);                     // dfi_tphy_rdcslat=0x1b, dfi_tphy_wrcslat=0x9
                                                                                 // dfi_tphy_wrcslat=9
      ms_write_both_ddrc_reg(memshire, DRAMTMG0, 0x1b201a22);                    // wr2pre=0x1b, t_faw=0x20, t_ras_max=0x1a, t_ras_min=0x22
                                                                                 // wr2pre=27, t_faw=32, t_ras_max=26, t_ras_min=34
      ms_write_both_ddrc_reg(memshire, DRAMTMG1, 0x00060833);                    // t_xp=0x6, rd2pre=0x8, t_rc=0x33
                                                                                 // t_xp=6, rd2pre=8, t_rc=51
      ms_write_both_ddrc_reg(memshire, DRAMTMG12, 0x00030000);                   // t_cmdcke=0x3
                                                                                 // t_cmdcke=3
      ms_write_both_ddrc_reg(memshire, DRAMTMG13, 0x0c100002);                   // odtloff=0xc, t_ccd_mw=0x10, t_ppd=0x2
                                                                                 // odtloff=12, t_ccd_mw=16, t_ppd=2
      ms_write_both_ddrc_reg(memshire, DRAMTMG14, 0x000000e6);                   // t_xsr=0xe6
                                                                                 // t_xsr=230
      ms_write_both_ddrc_reg(memshire, DRAMTMG2, 0x07101114);                    // write_latency=0x7, read_latency=0x10, rd2wr=0x11, wr2rd=0x14
                                                                                 // write_latency=7, read_latency=16, rd2wr=17,
                                                                                 // wr2rd=20
      ms_write_both_ddrc_reg(memshire, DRAMTMG3, 0x00c0c000);                    // t_mrw=0xc, t_mrd=0xc
                                                                                 // t_mrw=12, t_mrd=12
      ms_write_both_ddrc_reg(memshire, DRAMTMG4, 0x0f04080f);                    // t_rcd=0xf, t_ccd=0x4, t_rrd=0x8, t_rp=0xf
                                                                                 // t_rcd=15, t_ccd=4, t_rrd=8, t_rp=15
      ms_write_both_ddrc_reg(memshire, DRAMTMG5, 0x0305000c);                    // t_cksrx=0x3, t_cksre=0x5, t_ckesr=0x0, t_cke=0xc
                                                                                 // t_cksrx=3, t_cksre=5, t_cke=12
      ms_write_both_ddrc_reg(memshire, DRAMTMG6, 0x00000007);                    // t_ckdpde=0x0, t_ckdpdx=0x0, t_ckcsx=0x7
                                                                                 // t_ckcsx=7
      ms_write_both_ddrc_reg(memshire, DRAMTMG7, 0x00000503);                    // t_ckpde=0x5, t_ckpdx=0x3
                                                                                 // t_ckpde=5, t_ckpdx=3
      ms_write_both_ddrc_reg(memshire, INIT2, 0x00000803);                       // idle_after_reset_x32=0x8, min_stable_clock_x1=0x3
                                                                                 // idle_after_reset_x32=8, min_stable_clock_x1=3
      ms_write_both_ddrc_reg(memshire, INIT3, 0x0054002d);                       // mr=0x54, emr=0x2d
      ms_write_both_ddrc_reg(memshire, RFSHCTL0, 0x00210000);                    // refresh_margin=0x2, refresh_to_x1_x32=0x10, refresh_burst=0x0, per_bank_refresh=0x0
                                                                                 // refresh_margin=2
      ms_write_both_ddrc_reg(memshire, RFSHTMG, 0x806100e0);                     // t_rfc_nom_x1_sel=0x1, t_rfc_nom_x1_x32=0x61, lpddr3_trefbw_en=0x0, t_rfc_min=0xe0
                                                                                 // t_rfc_nom_x1_sel=1, t_rfc_nom_x1_x32=97,
                                                                                 // t_rfc_min=224
      ms_write_both_ddrc_reg(memshire, ZQCTL0, 0xc3200000);                      // dis_auto_zq=0x1, dis_srx_zqcl=0x1, zq_resistor_shared=0x0, t_zq_long_nop=0x320, t_zq_short_nop=0x0
                                                                                 // dis_auto_zq=1, dis_srx_zqcl=1, t_zq_long_nop=800
      ms_write_both_ddrc_reg(memshire, ZQCTL1, 0x02800100);                      // t_zq_reset_nop=0x28, t_zq_short_interval_x1024=0x100
                                                                                 // t_zq_reset_nop=40, t_zq_short_interval_x1024=256

   } else {
      if (config_933mhz) {
         if (config_training) {
            if (config_sim_only) {
               ms_write_both_ddrc_reg(memshire, INIT0, 0xc0020002);              // skip_dram_init=0x3, post_cke_x1024=0x2, pre_cke_x1024=0x2
                                                                                 // skip_dram_init=3, post_cke_x1024=2,
                                                                                 // pre_cke_x1024=2
               ms_write_both_ddrc_reg(memshire, INIT1, 0x00020000);              // dram_rstn_x1024=0x2, pre_ocd_x32=0x0
                                                                                 // dram_rstn_x1024=2
            } else {
               ms_write_both_ddrc_reg(memshire, INIT0, 0xc002071e);              // skip_dram_init=0x3, post_cke_x1024=0x2, pre_cke_x1024=0x71e
                                                                                 // skip_dram_init=3, post_cke_x1024=2
               ms_write_both_ddrc_reg(memshire, INIT1, 0x00c80000);              // dram_rstn_x1024=0xc8, pre_ocd_x32=0x0
            }
            ms_write_both_ddrc_reg(memshire, INIT4, 0x00f10008);                 // emr2=0xf1, emr3=0x8
                                                                                 // emr3=8
         } else {
            ms_write_both_ddrc_reg(memshire, INIT0, 0x00020002);                 // skip_dram_init=0x0, post_cke_x1024=0x2, pre_cke_x1024=0x2
                                                                                 // post_cke_x1024=2, pre_cke_x1024=2
            ms_write_both_ddrc_reg(memshire, INIT1, 0x00020000);                 // dram_rstn_x1024=0x2, pre_ocd_x32=0x0
                                                                                 // dram_rstn_x1024=2
            ms_write_both_ddrc_reg(memshire, INIT4, 0x00f10000);                 // emr2=0xf1, emr3=0x0
         }
         ms_write_both_ddrc_reg(memshire, DFITMG0, 0x029f820c);                  // dfi_t_ctrl_delay=0x2, dfi_rddata_use_dfi_phy_clk=0x1, dfi_t_rddata_en=0x1f, dfi_wrdata_use_dfi_phy_clk=0x1, dfi_tphy_wrdata=0x2, dfi_tphy_wrlat=0xc
                                                                                 // dfi_t_ctrl_delay=2, dfi_rddata_use_dfi_phy_clk=1,
                                                                                 // dfi_t_rddata_en=31, dfi_wrdata_use_dfi_phy_clk=1,
                                                                                 // dfi_tphy_wrdata=2, dfi_tphy_wrlat=12
         ms_write_both_ddrc_reg(memshire, DFITMG1, 0x000a0101);                  // dfi_t_wrdata_delay=0xa, dfi_t_dram_clk_disable=0x1, dfi_t_dram_clk_enable=0x1
                                                                                 // dfi_t_wrdata_delay=10, dfi_t_dram_clk_disable=1,
                                                                                 // dfi_t_dram_clk_enable=1
         ms_write_both_ddrc_reg(memshire, DFITMG2, 0x00001f0b);                  // dfi_tphy_rdcslat=0x1f, dfi_tphy_wrcslat=0xb
         ms_write_both_ddrc_reg(memshire, DRAMTMG0, 0x1e261f28);                 // wr2pre=0x1e, t_faw=0x26, t_ras_max=0x1f, t_ras_min=0x28
                                                                                 // wr2pre=30, t_faw=38, t_ras_max=31, t_ras_min=40
         ms_write_both_ddrc_reg(memshire, DRAMTMG1, 0x0007083b);                 // t_xp=0x7, rd2pre=0x8, t_rc=0x3b
                                                                                 // t_xp=7, rd2pre=8, t_rc=59
         ms_write_both_ddrc_reg(memshire, DRAMTMG12, 0x00030000);                // t_cmdcke=0x3
                                                                                 // t_cmdcke=3
         ms_write_both_ddrc_reg(memshire, DRAMTMG13, 0x0d100002);                // odtloff=0xd, t_ccd_mw=0x10, t_ppd=0x2
                                                                                 // odtloff=13, t_ccd_mw=16, t_ppd=2
         ms_write_both_ddrc_reg(memshire, DRAMTMG14, 0x0000010d);                // t_xsr=0x10d
                                                                                 // t_xsr=269
         ms_write_both_ddrc_reg(memshire, DRAMTMG2, 0x08121316);                 // write_latency=0x8, read_latency=0x12, rd2wr=0x13, wr2rd=0x16
                                                                                 // write_latency=8, read_latency=18, rd2wr=19,
                                                                                 // wr2rd=22
         ms_write_both_ddrc_reg(memshire, DRAMTMG3, 0x00e0e000);                 // t_mrw=0xe, t_mrd=0xe
                                                                                 // t_mrw=14, t_mrd=14
         ms_write_both_ddrc_reg(memshire, DRAMTMG4, 0x11040a11);                 // t_rcd=0x11, t_ccd=0x4, t_rrd=0xa, t_rp=0x11
                                                                                 // t_rcd=17, t_ccd=4, t_rrd=10, t_rp=17
         ms_write_both_ddrc_reg(memshire, DRAMTMG5, 0x0305000e);                 // t_cksrx=0x3, t_cksre=0x5, t_ckesr=0x0, t_cke=0xe
                                                                                 // t_cksrx=3, t_cksre=5, t_cke=14
         ms_write_both_ddrc_reg(memshire, DRAMTMG6, 0x00000008);                 // t_ckdpde=0x0, t_ckdpdx=0x0, t_ckcsx=0x8
                                                                                 // t_ckcsx=8
         ms_write_both_ddrc_reg(memshire, DRAMTMG7, 0x00000503);                 // t_ckpde=0x5, t_ckpdx=0x3
                                                                                 // t_ckpde=5, t_ckpdx=3
         ms_write_both_ddrc_reg(memshire, INIT2, 0x00000a03);                    // idle_after_reset_x32=0xa, min_stable_clock_x1=0x3
                                                                                 // min_stable_clock_x1=3
         ms_write_both_ddrc_reg(memshire, INIT3, 0x00640036);                    // mr=0x64, emr=0x36
         ms_write_both_ddrc_reg(memshire, RFSHCTL0, 0x00210000);                 // refresh_margin=0x2, refresh_to_x1_x32=0x10, refresh_burst=0x0, per_bank_refresh=0x0
                                                                                 // refresh_margin=2
         ms_write_both_ddrc_reg(memshire, RFSHTMG, 0x80710106);                  // t_rfc_nom_x1_sel=0x1, t_rfc_nom_x1_x32=0x71, lpddr3_trefbw_en=0x0, t_rfc_min=0x106
                                                                                 // t_rfc_nom_x1_sel=1, t_rfc_nom_x1_x32=113,
                                                                                 // t_rfc_min=262
         ms_write_both_ddrc_reg(memshire, ZQCTL0, 0xc3a50000);                   // dis_auto_zq=0x1, dis_srx_zqcl=0x1, zq_resistor_shared=0x0, t_zq_long_nop=0x3a5, t_zq_short_nop=0x0
                                                                                 // dis_auto_zq=1, dis_srx_zqcl=1, t_zq_long_nop=933
         ms_write_both_ddrc_reg(memshire, ZQCTL1, 0x02f00100);                   // t_zq_reset_nop=0x2f, t_zq_short_interval_x1024=0x100
                                                                                 // t_zq_reset_nop=47, t_zq_short_interval_x1024=256

      } else { // 1067 mhz
         if (config_training) {
            if (config_sim_only) {
               ms_write_both_ddrc_reg(memshire, INIT0, 0xc0020002);              // skip_dram_init=0x3, post_cke_x1024=0x2, pre_cke_x1024=0x2
                                                                                 // skip_dram_init=3, post_cke_x1024=2,
                                                                                 // pre_cke_x1024=2
               ms_write_both_ddrc_reg(memshire, INIT1, 0x00020000);              // dram_rstn_x1024=0x2, pre_ocd_x32=0x0
                                                                                 // dram_rstn_x1024=2
            } else {
               ms_write_both_ddrc_reg(memshire, INIT0, 0xc003082c);              // skip_dram_init=0x3, post_cke_x1024=0x3, pre_cke_x1024=0x82c
                                                                                 // skip_dram_init=3, post_cke_x1024=3
               ms_write_both_ddrc_reg(memshire, INIT1, 0x00750000);              // dram_rstn_x1024=0x75, pre_ocd_x32=0x0
            }
            ms_write_both_ddrc_reg(memshire, INIT4, 0x00f10008);                 // emr2=0xf1, emr3=0x8
                                                                                 // emr3=8
         } else {
            ms_write_both_ddrc_reg(memshire, INIT0, 0x00020002);                 // skip_dram_init=0x0, post_cke_x1024=0x2, pre_cke_x1024=0x2
                                                                                 // post_cke_x1024=2, pre_cke_x1024=2
            ms_write_both_ddrc_reg(memshire, INIT1, 0x00020000);                 // dram_rstn_x1024=0x2, pre_ocd_x32=0x0
                                                                                 // dram_rstn_x1024=2
            ms_write_both_ddrc_reg(memshire, INIT4, 0x00f10000);                 // emr2=0xf1, emr3=0x0
         }
         ms_write_both_ddrc_reg(memshire, DFITMG0, 0x02a3820e);                  // dfi_t_ctrl_delay=0x2, dfi_rddata_use_dfi_phy_clk=0x1, dfi_t_rddata_en=0x23, dfi_wrdata_use_dfi_phy_clk=0x1, dfi_tphy_wrdata=0x2, dfi_tphy_wrlat=0xe
                                                                                 // dfi_t_ctrl_delay=2, dfi_rddata_use_dfi_phy_clk=1,
                                                                                 // dfi_t_rddata_en=35, dfi_wrdata_use_dfi_phy_clk=1,
                                                                                 // dfi_tphy_wrdata=2, dfi_tphy_wrlat=14
         ms_write_both_ddrc_reg(memshire, DFITMG1, 0x000a0101);                  // dfi_t_wrdata_delay=0xa, dfi_t_dram_clk_disable=0x1, dfi_t_dram_clk_enable=0x1
                                                                                 // dfi_t_wrdata_delay=10, dfi_t_dram_clk_disable=1,
                                                                                 // dfi_t_dram_clk_enable=1
         ms_write_both_ddrc_reg(memshire, DFITMG2, 0x0000230d);                  // dfi_tphy_rdcslat=0x23, dfi_tphy_wrcslat=0xd
         ms_write_both_ddrc_reg(memshire, DRAMTMG0, 0x2221242d);                 // wr2pre=0x22, t_faw=0x21, t_ras_max=0x24, t_ras_min=0x2d
                                                                                 // wr2pre=34, t_faw=33, t_ras_max=36, t_ras_min=45
         ms_write_both_ddrc_reg(memshire, DRAMTMG1, 0x00090944);                 // t_xp=0x9, rd2pre=0x9, t_rc=0x44
                                                                                 // t_xp=9, rd2pre=9, t_rc=68
         ms_write_both_ddrc_reg(memshire, DRAMTMG12, 0x00030000);                // t_cmdcke=0x3
                                                                                 // t_cmdcke=3
         ms_write_both_ddrc_reg(memshire, DRAMTMG13, 0x0e100002);                // odtloff=0xe, t_ccd_mw=0x10, t_ppd=0x2
                                                                                 // odtloff=14, t_ccd_mw=16, t_ppd=2
         ms_write_both_ddrc_reg(memshire, DRAMTMG14, 0x00000134);                // t_xsr=0x134
                                                                                 // t_xsr=308
         ms_write_both_ddrc_reg(memshire, DRAMTMG2, 0x09141419);                 // write_latency=0x9, read_latency=0x14, rd2wr=0x14, wr2rd=0x19
                                                                                 // write_latency=9, read_latency=20, rd2wr=20,
                                                                                 // wr2rd=25
         ms_write_both_ddrc_reg(memshire, DRAMTMG3, 0x00f0f000);                 // t_mrw=0xf, t_mrd=0xf
                                                                                 // t_mrw=15, t_mrd=15
         ms_write_both_ddrc_reg(memshire, DRAMTMG4, 0x14040914);                 // t_rcd=0x14, t_ccd=0x4, t_rrd=0x9, t_rp=0x14
                                                                                 // t_rcd=20, t_ccd=4, t_rrd=9, t_rp=20
         ms_write_both_ddrc_reg(memshire, DRAMTMG5, 0x03060011);                 // t_cksrx=0x3, t_cksre=0x6, t_ckesr=0x0, t_cke=0x11
                                                                                 // t_cksrx=3, t_cksre=6, t_cke=17
         ms_write_both_ddrc_reg(memshire, DRAMTMG6, 0x0000000a);                 // t_ckdpde=0x0, t_ckdpdx=0x0, t_ckcsx=0xa
                                                                                 // t_ckcsx=10
         ms_write_both_ddrc_reg(memshire, DRAMTMG7, 0x00000603);                 // t_ckpde=0x6, t_ckpdx=0x3
                                                                                 // t_ckpde=6, t_ckpdx=3
         ms_write_both_ddrc_reg(memshire, INIT2, 0x00000b03);                    // idle_after_reset_x32=0xb, min_stable_clock_x1=0x3
                                                                                 // min_stable_clock_x1=3
         ms_write_both_ddrc_reg(memshire, INIT3, 0x0074003f);                    // mr=0x74, emr=0x3f
         ms_write_both_ddrc_reg(memshire, RFSHCTL0, 0x00210000);                 // refresh_margin=0x2, refresh_to_x1_x32=0x10, refresh_burst=0x0, per_bank_refresh=0x0
                                                                                 // refresh_margin=2
         ms_write_both_ddrc_reg(memshire, RFSHTMG, 0x8082012c);                  // t_rfc_nom_x1_sel=0x1, t_rfc_nom_x1_x32=0x82, lpddr3_trefbw_en=0x0, t_rfc_min=0x12c
                                                                                 // t_rfc_nom_x1_sel=1, t_rfc_nom_x1_x32=130,
                                                                                 // t_rfc_min=300
         ms_write_both_ddrc_reg(memshire, ZQCTL0, 0xc42f0000);                   // dis_auto_zq=0x1, dis_srx_zqcl=0x1, zq_resistor_shared=0x0, t_zq_long_nop=0x42f, t_zq_short_nop=0x0
                                                                                 // dis_auto_zq=1, dis_srx_zqcl=1,
                                                                                 // t_zq_long_nop=1071
         ms_write_both_ddrc_reg(memshire, ZQCTL1, 0x03600100);                   // t_zq_reset_nop=0x36, t_zq_short_interval_x1024=0x100
                                                                                 // t_zq_reset_nop=54, t_zq_short_interval_x1024=256

      }
   }

   // settings common to all frequencies
   ms_write_both_ddrc_reg(memshire, DBICTL, 0x00000007);                         // rd_dbi_en=0x1, wr_dbi_en=0x1, dm_en=0x1
                                                                                 // rd_dbi_en=1, wr_dbi_en=1, dm_en=1
   ms_write_both_ddrc_reg(memshire, DFILPCFG0, 0x01202020);                      // dfi_tlp_resp=0x1, dfi_lp_wakeup_dpd=0x2, dfi_lp_en_dpd=0x0, dfi_lp_wakeup_sr=0x2, dfi_lp_en_sr=0x0, dfi_lp_wakeup_pd=0x2, dfi_lp_en_pd=0x0
                                                                                 // dfi_tlp_resp=1, dfi_lp_wakeup_dpd=2,
                                                                                 // dfi_lp_wakeup_sr=2, dfi_lp_wakeup_pd=2
   ms_write_both_ddrc_reg(memshire, DFIPHYMSTR, 0x00000000);                     // dfi_phymstr_en=0x0
   ms_write_both_ddrc_reg(memshire, DFIUPD0, 0x00400018);                        // dis_auto_ctrlupd=0x0, dis_auto_ctrlupd_srx=0x0, ctrlupd_pre_srx=0x0, dfi_t_ctrlup_max=0x40, dfi_t_ctrlup_min=0x18
                                                                                 // dfi_t_ctrlup_max=64, dfi_t_ctrlup_min=24
   ms_write_both_ddrc_reg(memshire, DFIUPD1, 0x004000ff);                        // dfi_t_ctrlupd_interval_min_x1024=0x40, dfi_t_ctrlupd_interval_max_x1024=0xff
                                                                                 // dfi_t_ctrlupd_interval_min_x1024=64,
                                                                                 // dfi_t_ctrlupd_interval_max_x1024=255
   ms_write_both_ddrc_reg(memshire, DFIUPD2, 0x00000000);                        // dfi_phyupd_en=0x0
   ms_write_both_ddrc_reg(memshire, INIT6, 0x0046004d);                          // mr4=0x46, mr5=0x4d
   ms_write_both_ddrc_reg(memshire, INIT7, 0x0000004f);                          // mr22=0x0, mr6=0x4f
   ms_write_both_ddrc_reg(memshire, MSTR, 0x00080020);                           // burst_rdwr=0x8, data_bus_width=0x0, en_2t_timing_mode=0x0, lpddr4=0x1, lpddr3=0x0, lpddr2=0x0
                                                                                 // burst_rdwr=8, lpddr4=1
   ms_write_both_ddrc_reg(memshire, ODTCFG, 0x00000000);                         // wr_odt_hold=0x0, wr_odt_delay=0x0, rd_odt_hold=0x0, rd_odt_delay=0x0
   ms_write_both_ddrc_reg(memshire, ODTMAP, 0x00000000);                         // rank0_rd_odt=0x0, rank0_wr_odt=0x0


   //#################################################################################
   // END OF AUTO-GENERATED CODE

   ms_write_both_ddrc_reg(memshire, RFSHCTL3, 0x00000001);                       // dis_auto_refresh = 1. put this here to inhibit bogus firing of perfop assertion
                                                                                 // refresh_update_level=0, dis_auto_refresh=1

   //######################################################################################
   // configure ECC
   //######################################################################################
   if (config_ecc) {
      ms_write_both_ddrc_reg(memshire, ECCCFG0, 0x003f7f14);                     // enable ECC
                                                                                 // ecc_region_map_granu=0,
                                                                                 // blk_channel_idle_time_x32=63,
                                                                                 // ecc_region_map=0x7f, dis_scrub=1, ecc_mode=4
   } else {
      ms_write_both_ddrc_reg(memshire, ECCCFG0, 0x003f7f10);                     // disable ECC
                                                                                 // ecc_region_map_granu=0,
                                                                                 // blk_channel_idle_time_x32=63,
                                                                                 // ecc_region_map=0x7f, dis_scrub=1, ecc_mode=0
   }
   ms_write_both_ddrc_reg(memshire, ECCCFG1, 0x000007b2);                        // active_blk_channel=7, blk_channel_active_term=1,
                                                                                 // ecc_region_waste_lock=1,
                                                                                 // ecc_region_parity_lock=1, data_poison_bit=1,
                                                                                 // data_poison_en=0
   ms_write_both_ddrc_reg(memshire, ECCCTL, 0x00000300);                         // enable int on both single and double bit errors. FUTURE do not int on SBE
                                                                                 // ecc_uncorrected_err_intr_force=0,
                                                                                 // ecc_corrected_err_intr_force=0,
                                                                                 // ecc_uncorrected_err_intr_en=1,
                                                                                 // ecc_corrected_err_intr_en=1,
                                                                                 // ecc_uncorr_err_cnt_clr=0, ecc_corr_err_cnt_clr=0,
                                                                                 // ecc_uncorrected_err_clr=0,
                                                                                 // ecc_corrected_err_clr=0

   //######################################################################################
   // set up address map
   //######################################################################################
   ms_write_both_ddrc_reg(memshire, ADDRMAP4, 0x00001f1f);                       // unused
                                                                                 // col_addr_shift=0, addrmap_col_b11=0x1f,
                                                                                 // addrmap_col_b10=0x1f
   ms_write_both_ddrc_reg(memshire, ADDRMAP9, 0x07070707);                       // unused
                                                                                 // addrmap_row_b5=7, addrmap_row_b4=7,
                                                                                 // addrmap_row_b3=7, addrmap_row_b2=7
   ms_write_both_ddrc_reg(memshire, ADDRMAP10, 0x07070707);                      // unused
                                                                                 // addrmap_row_b9=7, addrmap_row_b8=7,
                                                                                 // addrmap_row_b7=7, addrmap_row_b6=7
   ms_write_both_ddrc_reg(memshire, ADDRMAP11, 0x001f1f07);                      // unused
                                                                                 // addrmap_row_b10=7
   if (config_ecc) {
      ms_write_both_ddrc_reg(memshire, ADDRMAP1, 0x00050505);                    // 1/2 cache line same bank
                                                                                 // addrmap_bank_b2=5, addrmap_bank_b1=5,
                                                                                 // addrmap_bank_b0=5
      ms_write_both_ddrc_reg(memshire, ADDRMAP2, 0x00000000);                    // 1/2 cache line same bank
                                                                                 // addrmap_col_b5=0, addrmap_col_b4=0,
                                                                                 // addrmap_col_b3=0, addrmap_col_b2=0
      ms_write_both_ddrc_reg(memshire, ADDRMAP5, 0x04040404);                    // addrmap_row_b11=4, addrmap_row_b2_10=4,
                                                                                 // addrmap_row_b1=4, addrmap_row_b0=4
      if (config_4gb) {
         // 4 GB of total system memory
         ms_write_both_ddrc_reg(memshire, ADDRMAP3, 0x11111100);                 // For smallest LPDDR4x part
                                                                                 // addrmap_col_b9=0x11, addrmap_col_b8=0x11,
                                                                                 // addrmap_col_b7=0x11, addrmap_col_b6=0
         ms_write_both_ddrc_reg(memshire, ADDRMAP6, 0x0f0f0404);                 // For smallest LPDDR4x part
                                                                                 // lpddr3_6gb_12gb=0, lpddr4_3gb_6gb_12gb=0,
                                                                                 // addrmap_row_b15=0xf, addrmap_row_b14=0xf,
                                                                                 // addrmap_row_b13=4, addrmap_row_b12=4
         ms_write_both_ddrc_reg(memshire, ADDRMAP7, 0x00000f0f);                 // For smallest LPDDR4x part
                                                                                 // addrmap_row_b16=0xf
      } else {
         if (config_8gb) {
            // 8 GB of total system memory
            ms_write_both_ddrc_reg(memshire, ADDRMAP3, 0x12121200);              // addrmap_col_b9=0x12, addrmap_col_b8=0x12,
                                                                                 // addrmap_col_b7=0x12, addrmap_col_b6=0
            ms_write_both_ddrc_reg(memshire, ADDRMAP6, 0x0f040404);              // lpddr3_6gb_12gb=0, lpddr4_3gb_6gb_12gb=0,
                                                                                 // addrmap_row_b15=0xf, addrmap_row_b14=4,
                                                                                 // addrmap_row_b13=4, addrmap_row_b12=4
            ms_write_both_ddrc_reg(memshire, ADDRMAP7, 0x00000f0f);              // addrmap_row_b16=0xf
         } else {
            if (config_32gb) {
               // 32 GB of total system memory
               ms_write_both_ddrc_reg(memshire, ADDRMAP3, 0x14141400);           // addrmap_col_b9=0x14, addrmap_col_b8=0x14,
                                                                                 // addrmap_col_b7=0x14, addrmap_col_b6=0
               ms_write_both_ddrc_reg(memshire, ADDRMAP6, 0x04040404);           // lpddr3_6gb_12gb=0, lpddr4_3gb_6gb_12gb=0,
                                                                                 // addrmap_row_b15=4, addrmap_row_b14=4,
                                                                                 // addrmap_row_b13=4, addrmap_row_b12=4
               ms_write_both_ddrc_reg(memshire, ADDRMAP7, 0x00000f04);           // addrmap_row_b16=4
            } else {
               // 16 GB of total system memory (default)
               ms_write_both_ddrc_reg(memshire, ADDRMAP3, 0x13131300);           // addrmap_col_b9=0x13, addrmap_col_b8=0x13,
                                                                                 // addrmap_col_b7=0x13, addrmap_col_b6=0
               ms_write_both_ddrc_reg(memshire, ADDRMAP6, 0x04040404);           // lpddr3_6gb_12gb=0, lpddr4_3gb_6gb_12gb=0,
                                                                                 // addrmap_row_b15=4, addrmap_row_b14=4,
                                                                                 // addrmap_row_b13=4, addrmap_row_b12=4
               ms_write_both_ddrc_reg(memshire, ADDRMAP7, 0x00000f0f);           // addrmap_row_b16=0xf
            }
         }
      }
   } else {
      ms_write_both_ddrc_reg(memshire, ADDRMAP1, 0x00030303);                    // 1/2 cache line same bank
                                                                                 // addrmap_bank_b2=3, addrmap_bank_b1=3,
                                                                                 // addrmap_bank_b0=3
      ms_write_both_ddrc_reg(memshire, ADDRMAP2, 0x03000000);                    // 1/2 cache line same bank
                                                                                 // addrmap_col_b5=3, addrmap_col_b4=0,
                                                                                 // addrmap_col_b3=0, addrmap_col_b2=0
      ms_write_both_ddrc_reg(memshire, ADDRMAP3, 0x03030303);                    // 1/2 cache line same bank
                                                                                 // addrmap_col_b9=3, addrmap_col_b8=3,
                                                                                 // addrmap_col_b7=3, addrmap_col_b6=3
      ms_write_both_ddrc_reg(memshire, ADDRMAP5, 0x07070707);                    // addrmap_row_b11=7, addrmap_row_b2_10=7,
                                                                                 // addrmap_row_b1=7, addrmap_row_b0=7
      ms_write_both_ddrc_reg(memshire, ADDRMAP6, 0x07070707);                    // assume addressing for largest LPDDR4x part is okay always
                                                                                 // lpddr3_6gb_12gb=0, lpddr4_3gb_6gb_12gb=0,
                                                                                 // addrmap_row_b15=7, addrmap_row_b14=7,
                                                                                 // addrmap_row_b13=7, addrmap_row_b12=7
      ms_write_both_ddrc_reg(memshire, ADDRMAP7, 0x00000f07);                    // assume addressing for largest LPDDR4x part is okay always
                                                                                 // addrmap_row_b16=7
   }

   //######################################################################################
   // performance related. Need to refine
   //######################################################################################
   ms_write_both_ddrc_reg(memshire, SCHED, 0x00a01f01);                          // rdwr_idle_gap=0, go2critical_hysteresis=0xa0,
                                                                                 // lpr_num_entries=0x1f, autopre_rmw=0, pageclose=0,
                                                                                 // prefer_write=0, force_low_pri_n=1
   ms_write_both_ddrc_reg(memshire, SCHED1, 0x00000000);                         // pageclose_timer=0
   ms_write_both_ddrc_reg(memshire, PERFHPR1, 0x0f000001);                       // hpr_xact_run_length=0xf, hpr_max_starve=1
   ms_write_both_ddrc_reg(memshire, PERFLPR1, 0x0f00007f);                       // lpr_xact_run_length=0xf, lpr_max_starve=0x7f
   ms_write_both_ddrc_reg(memshire, PERFWR1, 0x0f00007f);                        // w_xact_run_length=0xf, w_max_starve=0x7f
   ms_write_both_ddrc_reg(memshire, PCCFG, 0x00000000);                          // bl_exp_mode=0, pagematch_limit=0,
                                                                                 // go2critical_en=0
   ms_write_both_ddrc_reg(memshire, PCFGR_0, 0x0000100f);                        // port 0 is lower priority
                                                                                 // rd_port_pagematch_en=0, rd_port_urgent_en=0,
                                                                                 // rd_port_aging_en=1, rd_port_priority=0xf
   ms_write_both_ddrc_reg(memshire, PCFGR_1, 0x00001008);                        // port 0 is higher priority
                                                                                 // rd_port_pagematch_en=0, rd_port_urgent_en=0,
                                                                                 // rd_port_aging_en=1, rd_port_priority=8
   ms_write_both_ddrc_reg(memshire, PCFGW_0, 0x0000100f);                        // port 1 is lower priority. should writes be lower priority?
                                                                                 // wr_port_pagematch_en=0, wr_port_urgent_en=0,
                                                                                 // wr_port_aging_en=1, wr_port_priority=0xf
   ms_write_both_ddrc_reg(memshire, PCFGW_1, 0x00001008);                        // port 1 is higher priority
                                                                                 // wr_port_pagematch_en=0, wr_port_urgent_en=0,
                                                                                 // wr_port_aging_en=1, wr_port_priority=8

   //######################################################################################
   // These are commented out currently, but may want to change them in the future
   //######################################################################################
   // ms_write_both_ddrc_reg $memshire DERATEEN      0x00001404  # FUTURE derate is not needed for bring-up
   // ms_write_both_ddrc_reg $memshire DERATEINT     0xc0c188dd
   // ms_write_both_ddrc_reg $memshire DERATECTL     0x00000000
   // ms_write_both_ddrc_reg $memshire PWRTMG        0x0040d104  # FUTURE auto power down timing not needed for bring-up
   // ms_write_both_ddrc_reg $memshire RFSHTMG1      0x00410000  # FUTURE only used in per-bank refresh mode.
   // ms_write_both_ddrc_reg $memshire CRCPARCTL0    0x00000000  # FUTURE may want to enable dfi alert errors
   // ms_write_both_ddrc_reg $memshire DBG0          0x00000000  # used for debug hopefully not needed
   // ms_write_both_ddrc_reg $memshire DBG1          0x00000000  # used for debug hopefully not needed
   // ms_write_both_ddrc_reg $memshire DBGCMD        0x00000000  # used for debug hopefully not needed

   return 0;
}


//
// Phase 1 of the DDR initialization sequence.
//
uint32_t ms_init_seq_phase1 (uint32_t memshire, uint32_t config_ecc, uint32_t config_real_pll, uint32_t config_800mhz, uint32_t config_933mhz, uint32_t config_training, uint32_t config_4gb, uint32_t config_8gb, uint32_t config_32gb, uint32_t config_sim_only) {
   //
   // Start to initialize the memory controllers
   //
   if (config_real_pll) {
      ms_write_esr(memshire, ddrc_reset_ctl, 0x10d); // deassert apb reset. note: ddrc and pub are still in reset
   } else {
      ms_write_esr(memshire, ddrc_reset_ctl, 0x00d); // deassert apb reset. note: ddrc and pub are still in reset
   }
   ms_read_esr(memshire, ddrc_reset_ctl); // make sure reset write has completed

   // FIXME: add test that ddr subsytem registers can be read/written


   // disable AXI ports
   ms_write_both_ddrc_reg(memshire, PCTRL_0, 0x00000000);                        // port_en=0
   ms_write_both_ddrc_reg(memshire, PCTRL_1, 0x00000000);                        // port_en=0

   //
   // Configure the memory controllers
   //
   // step 2
   ms_init_config_ddrc(memshire, config_ecc, config_real_pll, config_800mhz, config_933mhz, config_training, config_4gb, config_8gb, config_32gb, config_sim_only);

   //
   // Turn off core and axi resets
   //
   // step 3

   if (config_training) {
      ms_write_both_ddrc_reg(memshire, PWRCTL, 0x00000020);                      // lpddr4_sr_allowed=0, dis_cam_drain_selfref=0,
                                                                                 // stay_in_selfref=0, selfref_sw=1,
                                                                                 // en_dfi_dram_clk_disable=0, deeppowerdown_en=0,
                                                                                 // powerdown_en=0, selfref_en=0
   } else {
      ms_write_both_ddrc_reg(memshire, PWRCTL, 0x00000000);                      // powerdown_en = 0, selfref_en = 0
                                                                                 // lpddr4_sr_allowed=0, dis_cam_drain_selfref=0,
                                                                                 // stay_in_selfref=0, selfref_sw=0,
                                                                                 // en_dfi_dram_clk_disable=0, deeppowerdown_en=0,
                                                                                 // powerdown_en=0, selfref_en=0
   }

   if (config_real_pll) {
      ms_write_esr(memshire, ddrc_reset_ctl, 0x10c);
   } else {
      ms_write_esr(memshire, ddrc_reset_ctl, 0x00c);
   }
   ms_read_esr(memshire, ddrc_reset_ctl);

   // required during training
   // step 4
   ms_write_both_ddrc_reg(memshire, RFSHCTL3, 0x00000001);                       // dis_auto_refresh = 1
                                                                                 // refresh_update_level=0, dis_auto_refresh=1

   // step 5
   ms_write_both_ddrc_reg(memshire, SWCTL, 0x00000000);                          // sw_done = 0
                                                                                 // sw_done=0

   // step 6
   ms_write_both_ddrc_reg(memshire, DFIMISC, 0x00000000);                        // dfi_init_complete_en = 0. Go to P0
                                                                                 // dfi_frequency=0, lp_optimized_write=0,
                                                                                 // dfi_init_start=0, ctl_idle_en=0,
                                                                                 // dfi_data_cs_polarity=0, phy_dbi_mode=0,
                                                                                 // dfi_init_complete_en=0

   // step 7
   ms_write_both_ddrc_reg(memshire, SWCTL, 0x00000001);                          // sw_done = 1
                                                                                 // sw_done=1

   ms_poll_ddrc_reg(memshire, 0, SWSTAT, 0x1, 0x1, 100, 1);                      // make sure sw_done_ack=1
   ms_poll_ddrc_reg(memshire, 1, SWSTAT, 0x1, 0x1, 100, 1);                      // make sure sw_done_ack=1

   // ready to start training (if enabled)

   return 0;
}

//
// write phy registers to compenstate for bub board swizzling of the CA bits for memshire 0
//
uint32_t ms_init_swizle_ca_bub (uint32_t memshire0, uint32_t memshire1, uint32_t memshire2, uint32_t memshire3, uint32_t memshire4, uint32_t memshire5, uint32_t memshire6, uint32_t memshire7) {
   if (memshire0) {
      ms_write_ddrc_reg(0, 2, MASTER0_MapCAA0toDfi, 0x3);
      ms_write_ddrc_reg(0, 2, MASTER0_MapCAA1toDfi, 0x5);
      ms_write_ddrc_reg(0, 2, MASTER0_MapCAA2toDfi, 0x1);
      ms_write_ddrc_reg(0, 2, MASTER0_MapCAA3toDfi, 0x4);
      ms_write_ddrc_reg(0, 2, MASTER0_MapCAA4toDfi, 0x0);
      ms_write_ddrc_reg(0, 2, MASTER0_MapCAA5toDfi, 0x2);
      ms_write_ddrc_reg(0, 2, MASTER0_MapCAB0toDfi, 0x3);
      ms_write_ddrc_reg(0, 2, MASTER0_MapCAB1toDfi, 0x1);
      ms_write_ddrc_reg(0, 2, MASTER0_MapCAB2toDfi, 0x4);
      ms_write_ddrc_reg(0, 2, MASTER0_MapCAB3toDfi, 0x0);
      ms_write_ddrc_reg(0, 2, MASTER0_MapCAB4toDfi, 0x2);
      ms_write_ddrc_reg(0, 2, MASTER0_MapCAB5toDfi, 0x5);
   }
   if (memshire1) {
      ms_write_ddrc_reg(1, 2, MASTER0_MapCAA0toDfi, 0x3);
      ms_write_ddrc_reg(1, 2, MASTER0_MapCAA1toDfi, 0x4);
      ms_write_ddrc_reg(1, 2, MASTER0_MapCAA2toDfi, 0x1);
      ms_write_ddrc_reg(1, 2, MASTER0_MapCAA3toDfi, 0x5);
      ms_write_ddrc_reg(1, 2, MASTER0_MapCAA4toDfi, 0x0);
      ms_write_ddrc_reg(1, 2, MASTER0_MapCAA5toDfi, 0x2);
      ms_write_ddrc_reg(1, 2, MASTER0_MapCAB0toDfi, 0x4);
      ms_write_ddrc_reg(1, 2, MASTER0_MapCAB1toDfi, 0x0);
      ms_write_ddrc_reg(1, 2, MASTER0_MapCAB2toDfi, 0x5);
      ms_write_ddrc_reg(1, 2, MASTER0_MapCAB3toDfi, 0x2);
      ms_write_ddrc_reg(1, 2, MASTER0_MapCAB4toDfi, 0x1);
      ms_write_ddrc_reg(1, 2, MASTER0_MapCAB5toDfi, 0x3);
   }
   if (memshire2) {
      ms_write_ddrc_reg(2, 2, MASTER0_MapCAA0toDfi, 0x5);
      ms_write_ddrc_reg(2, 2, MASTER0_MapCAA1toDfi, 0x4);
      ms_write_ddrc_reg(2, 2, MASTER0_MapCAA2toDfi, 0x3);
      ms_write_ddrc_reg(2, 2, MASTER0_MapCAA3toDfi, 0x0);
      ms_write_ddrc_reg(2, 2, MASTER0_MapCAA4toDfi, 0x1);
      ms_write_ddrc_reg(2, 2, MASTER0_MapCAA5toDfi, 0x2);
      ms_write_ddrc_reg(2, 2, MASTER0_MapCAB0toDfi, 0x2);
      ms_write_ddrc_reg(2, 2, MASTER0_MapCAB1toDfi, 0x0);
      ms_write_ddrc_reg(2, 2, MASTER0_MapCAB2toDfi, 0x4);
      ms_write_ddrc_reg(2, 2, MASTER0_MapCAB3toDfi, 0x5);
      ms_write_ddrc_reg(2, 2, MASTER0_MapCAB4toDfi, 0x1);
      ms_write_ddrc_reg(2, 2, MASTER0_MapCAB5toDfi, 0x3);
   }
   if (memshire3) {
      ms_write_ddrc_reg(3, 2, MASTER0_MapCAA0toDfi, 0x1);
      ms_write_ddrc_reg(3, 2, MASTER0_MapCAA1toDfi, 0x4);
      ms_write_ddrc_reg(3, 2, MASTER0_MapCAA2toDfi, 0x2);
      ms_write_ddrc_reg(3, 2, MASTER0_MapCAA3toDfi, 0x5);
      ms_write_ddrc_reg(3, 2, MASTER0_MapCAA4toDfi, 0x3);
      ms_write_ddrc_reg(3, 2, MASTER0_MapCAA5toDfi, 0x0);
      ms_write_ddrc_reg(3, 2, MASTER0_MapCAB0toDfi, 0x3);
      ms_write_ddrc_reg(3, 2, MASTER0_MapCAB1toDfi, 0x0);
      ms_write_ddrc_reg(3, 2, MASTER0_MapCAB2toDfi, 0x5);
      ms_write_ddrc_reg(3, 2, MASTER0_MapCAB3toDfi, 0x1);
      ms_write_ddrc_reg(3, 2, MASTER0_MapCAB4toDfi, 0x2);
      ms_write_ddrc_reg(3, 2, MASTER0_MapCAB5toDfi, 0x4);
   }
   if (memshire4) {
      ms_write_ddrc_reg(4, 2, MASTER0_MapCAA0toDfi, 0x1);
      ms_write_ddrc_reg(4, 2, MASTER0_MapCAA1toDfi, 0x4);
      ms_write_ddrc_reg(4, 2, MASTER0_MapCAA2toDfi, 0x2);
      ms_write_ddrc_reg(4, 2, MASTER0_MapCAA3toDfi, 0x5);
      ms_write_ddrc_reg(4, 2, MASTER0_MapCAA4toDfi, 0x3);
      ms_write_ddrc_reg(4, 2, MASTER0_MapCAA5toDfi, 0x0);
      ms_write_ddrc_reg(4, 2, MASTER0_MapCAB0toDfi, 0x3);
      ms_write_ddrc_reg(4, 2, MASTER0_MapCAB1toDfi, 0x0);
      ms_write_ddrc_reg(4, 2, MASTER0_MapCAB2toDfi, 0x5);
      ms_write_ddrc_reg(4, 2, MASTER0_MapCAB3toDfi, 0x1);
      ms_write_ddrc_reg(4, 2, MASTER0_MapCAB4toDfi, 0x2);
      ms_write_ddrc_reg(4, 2, MASTER0_MapCAB5toDfi, 0x4);
   }
   if (memshire5) {
      ms_write_ddrc_reg(5, 2, MASTER0_MapCAA0toDfi, 0x5);
      ms_write_ddrc_reg(5, 2, MASTER0_MapCAA1toDfi, 0x4);
      ms_write_ddrc_reg(5, 2, MASTER0_MapCAA2toDfi, 0x3);
      ms_write_ddrc_reg(5, 2, MASTER0_MapCAA3toDfi, 0x0);
      ms_write_ddrc_reg(5, 2, MASTER0_MapCAA4toDfi, 0x1);
      ms_write_ddrc_reg(5, 2, MASTER0_MapCAA5toDfi, 0x2);
      ms_write_ddrc_reg(5, 2, MASTER0_MapCAB0toDfi, 0x2);
      ms_write_ddrc_reg(5, 2, MASTER0_MapCAB1toDfi, 0x0);
      ms_write_ddrc_reg(5, 2, MASTER0_MapCAB2toDfi, 0x4);
      ms_write_ddrc_reg(5, 2, MASTER0_MapCAB3toDfi, 0x5);
      ms_write_ddrc_reg(5, 2, MASTER0_MapCAB4toDfi, 0x1);
      ms_write_ddrc_reg(5, 2, MASTER0_MapCAB5toDfi, 0x3);
   }
   if (memshire6) {
      ms_write_ddrc_reg(6, 2, MASTER0_MapCAA0toDfi, 0x3);
      ms_write_ddrc_reg(6, 2, MASTER0_MapCAA1toDfi, 0x4);
      ms_write_ddrc_reg(6, 2, MASTER0_MapCAA2toDfi, 0x1);
      ms_write_ddrc_reg(6, 2, MASTER0_MapCAA3toDfi, 0x5);
      ms_write_ddrc_reg(6, 2, MASTER0_MapCAA4toDfi, 0x0);
      ms_write_ddrc_reg(6, 2, MASTER0_MapCAA5toDfi, 0x2);
      ms_write_ddrc_reg(6, 2, MASTER0_MapCAB0toDfi, 0x4);
      ms_write_ddrc_reg(6, 2, MASTER0_MapCAB1toDfi, 0x0);
      ms_write_ddrc_reg(6, 2, MASTER0_MapCAB2toDfi, 0x5);
      ms_write_ddrc_reg(6, 2, MASTER0_MapCAB3toDfi, 0x2);
      ms_write_ddrc_reg(6, 2, MASTER0_MapCAB4toDfi, 0x1);
      ms_write_ddrc_reg(6, 2, MASTER0_MapCAB5toDfi, 0x3);
   }
   if (memshire7) {
      ms_write_ddrc_reg(7, 2, MASTER0_MapCAA0toDfi, 0x3);
      ms_write_ddrc_reg(7, 2, MASTER0_MapCAA1toDfi, 0x5);
      ms_write_ddrc_reg(7, 2, MASTER0_MapCAA2toDfi, 0x1);
      ms_write_ddrc_reg(7, 2, MASTER0_MapCAA3toDfi, 0x4);
      ms_write_ddrc_reg(7, 2, MASTER0_MapCAA4toDfi, 0x0);
      ms_write_ddrc_reg(7, 2, MASTER0_MapCAA5toDfi, 0x2);
      ms_write_ddrc_reg(7, 2, MASTER0_MapCAB0toDfi, 0x3);
      ms_write_ddrc_reg(7, 2, MASTER0_MapCAB1toDfi, 0x1);
      ms_write_ddrc_reg(7, 2, MASTER0_MapCAB2toDfi, 0x4);
      ms_write_ddrc_reg(7, 2, MASTER0_MapCAB3toDfi, 0x0);
      ms_write_ddrc_reg(7, 2, MASTER0_MapCAB4toDfi, 0x2);
      ms_write_ddrc_reg(7, 2, MASTER0_MapCAB5toDfi, 0x5);
   }
   return 0;
}

//
// write phy registers to compenstate for bub board swizzling of the DQ bits for memshire 0
//
uint32_t ms_init_swizle_dq_bub (uint32_t memshire0, uint32_t memshire1, uint32_t memshire2, uint32_t memshire3, uint32_t memshire4, uint32_t memshire5, uint32_t memshire6, uint32_t memshire7) {
   if (memshire0) {
      ms_write_ddrc_reg(0, 2, DBYTE0_Dq0LnSel, 0x5);
      ms_write_ddrc_reg(0, 2, DBYTE0_Dq1LnSel, 0x2);
      ms_write_ddrc_reg(0, 2, DBYTE0_Dq2LnSel, 0x3);
      ms_write_ddrc_reg(0, 2, DBYTE0_Dq3LnSel, 0x0);
      ms_write_ddrc_reg(0, 2, DBYTE0_Dq4LnSel, 0x1);
      ms_write_ddrc_reg(0, 2, DBYTE0_Dq5LnSel, 0x7);
      ms_write_ddrc_reg(0, 2, DBYTE0_Dq6LnSel, 0x4);
      ms_write_ddrc_reg(0, 2, DBYTE0_Dq7LnSel, 0x6);
      ms_write_ddrc_reg(0, 2, DBYTE1_Dq0LnSel, 0x3);
      ms_write_ddrc_reg(0, 2, DBYTE1_Dq1LnSel, 0x2);
      ms_write_ddrc_reg(0, 2, DBYTE1_Dq2LnSel, 0x7);
      ms_write_ddrc_reg(0, 2, DBYTE1_Dq3LnSel, 0x0);
      ms_write_ddrc_reg(0, 2, DBYTE1_Dq4LnSel, 0x6);
      ms_write_ddrc_reg(0, 2, DBYTE1_Dq5LnSel, 0x1);
      ms_write_ddrc_reg(0, 2, DBYTE1_Dq6LnSel, 0x5);
      ms_write_ddrc_reg(0, 2, DBYTE1_Dq7LnSel, 0x4);
      ms_write_ddrc_reg(0, 2, DBYTE2_Dq0LnSel, 0x6);
      ms_write_ddrc_reg(0, 2, DBYTE2_Dq1LnSel, 0x3);
      ms_write_ddrc_reg(0, 2, DBYTE2_Dq2LnSel, 0x4);
      ms_write_ddrc_reg(0, 2, DBYTE2_Dq3LnSel, 0x2);
      ms_write_ddrc_reg(0, 2, DBYTE2_Dq4LnSel, 0x5);
      ms_write_ddrc_reg(0, 2, DBYTE2_Dq5LnSel, 0x7);
      ms_write_ddrc_reg(0, 2, DBYTE2_Dq6LnSel, 0x1);
      ms_write_ddrc_reg(0, 2, DBYTE2_Dq7LnSel, 0x0);
      ms_write_ddrc_reg(0, 2, DBYTE3_Dq0LnSel, 0x3);
      ms_write_ddrc_reg(0, 2, DBYTE3_Dq1LnSel, 0x1);
      ms_write_ddrc_reg(0, 2, DBYTE3_Dq2LnSel, 0x5);
      ms_write_ddrc_reg(0, 2, DBYTE3_Dq3LnSel, 0x0);
      ms_write_ddrc_reg(0, 2, DBYTE3_Dq4LnSel, 0x7);
      ms_write_ddrc_reg(0, 2, DBYTE3_Dq5LnSel, 0x2);
      ms_write_ddrc_reg(0, 2, DBYTE3_Dq6LnSel, 0x6);
      ms_write_ddrc_reg(0, 2, DBYTE3_Dq7LnSel, 0x4);
   }
   if (memshire1) {
      ms_write_ddrc_reg(1, 2, DBYTE0_Dq0LnSel, 0x3);
      ms_write_ddrc_reg(1, 2, DBYTE0_Dq1LnSel, 0x2);
      ms_write_ddrc_reg(1, 2, DBYTE0_Dq2LnSel, 0x7);
      ms_write_ddrc_reg(1, 2, DBYTE0_Dq3LnSel, 0x6);
      ms_write_ddrc_reg(1, 2, DBYTE0_Dq4LnSel, 0x1);
      ms_write_ddrc_reg(1, 2, DBYTE0_Dq5LnSel, 0x0);
      ms_write_ddrc_reg(1, 2, DBYTE0_Dq6LnSel, 0x4);
      ms_write_ddrc_reg(1, 2, DBYTE0_Dq7LnSel, 0x5);
      ms_write_ddrc_reg(1, 2, DBYTE1_Dq0LnSel, 0x5);
      ms_write_ddrc_reg(1, 2, DBYTE1_Dq1LnSel, 0x1);
      ms_write_ddrc_reg(1, 2, DBYTE1_Dq2LnSel, 0x7);
      ms_write_ddrc_reg(1, 2, DBYTE1_Dq3LnSel, 0x6);
      ms_write_ddrc_reg(1, 2, DBYTE1_Dq4LnSel, 0x0);
      ms_write_ddrc_reg(1, 2, DBYTE1_Dq5LnSel, 0x2);
      ms_write_ddrc_reg(1, 2, DBYTE1_Dq6LnSel, 0x4);
      ms_write_ddrc_reg(1, 2, DBYTE1_Dq7LnSel, 0x3);
      ms_write_ddrc_reg(1, 2, DBYTE2_Dq0LnSel, 0x3);
      ms_write_ddrc_reg(1, 2, DBYTE2_Dq1LnSel, 0x5);
      ms_write_ddrc_reg(1, 2, DBYTE2_Dq2LnSel, 0x4);
      ms_write_ddrc_reg(1, 2, DBYTE2_Dq3LnSel, 0x7);
      ms_write_ddrc_reg(1, 2, DBYTE2_Dq4LnSel, 0x2);
      ms_write_ddrc_reg(1, 2, DBYTE2_Dq5LnSel, 0x0);
      ms_write_ddrc_reg(1, 2, DBYTE2_Dq6LnSel, 0x1);
      ms_write_ddrc_reg(1, 2, DBYTE2_Dq7LnSel, 0x6);
      ms_write_ddrc_reg(1, 2, DBYTE3_Dq0LnSel, 0x1);
      ms_write_ddrc_reg(1, 2, DBYTE3_Dq1LnSel, 0x3);
      ms_write_ddrc_reg(1, 2, DBYTE3_Dq2LnSel, 0x4);
      ms_write_ddrc_reg(1, 2, DBYTE3_Dq3LnSel, 0x0);
      ms_write_ddrc_reg(1, 2, DBYTE3_Dq4LnSel, 0x6);
      ms_write_ddrc_reg(1, 2, DBYTE3_Dq5LnSel, 0x2);
      ms_write_ddrc_reg(1, 2, DBYTE3_Dq6LnSel, 0x5);
      ms_write_ddrc_reg(1, 2, DBYTE3_Dq7LnSel, 0x7);
   }
   if (memshire2) {
      ms_write_ddrc_reg(2, 2, DBYTE0_Dq0LnSel, 0x3);
      ms_write_ddrc_reg(2, 2, DBYTE0_Dq1LnSel, 0x4);
      ms_write_ddrc_reg(2, 2, DBYTE0_Dq2LnSel, 0x6);
      ms_write_ddrc_reg(2, 2, DBYTE0_Dq3LnSel, 0x7);
      ms_write_ddrc_reg(2, 2, DBYTE0_Dq4LnSel, 0x2);
      ms_write_ddrc_reg(2, 2, DBYTE0_Dq5LnSel, 0x5);
      ms_write_ddrc_reg(2, 2, DBYTE0_Dq6LnSel, 0x1);
      ms_write_ddrc_reg(2, 2, DBYTE0_Dq7LnSel, 0x0);
      ms_write_ddrc_reg(2, 2, DBYTE1_Dq0LnSel, 0x1);
      ms_write_ddrc_reg(2, 2, DBYTE1_Dq1LnSel, 0x2);
      ms_write_ddrc_reg(2, 2, DBYTE1_Dq2LnSel, 0x0);
      ms_write_ddrc_reg(2, 2, DBYTE1_Dq3LnSel, 0x4);
      ms_write_ddrc_reg(2, 2, DBYTE1_Dq4LnSel, 0x6);
      ms_write_ddrc_reg(2, 2, DBYTE1_Dq5LnSel, 0x3);
      ms_write_ddrc_reg(2, 2, DBYTE1_Dq6LnSel, 0x7);
      ms_write_ddrc_reg(2, 2, DBYTE1_Dq7LnSel, 0x5);
      ms_write_ddrc_reg(2, 2, DBYTE2_Dq0LnSel, 0x6);
      ms_write_ddrc_reg(2, 2, DBYTE2_Dq1LnSel, 0x4);
      ms_write_ddrc_reg(2, 2, DBYTE2_Dq2LnSel, 0x3);
      ms_write_ddrc_reg(2, 2, DBYTE2_Dq3LnSel, 0x5);
      ms_write_ddrc_reg(2, 2, DBYTE2_Dq4LnSel, 0x0);
      ms_write_ddrc_reg(2, 2, DBYTE2_Dq5LnSel, 0x2);
      ms_write_ddrc_reg(2, 2, DBYTE2_Dq6LnSel, 0x7);
      ms_write_ddrc_reg(2, 2, DBYTE2_Dq7LnSel, 0x1);
      ms_write_ddrc_reg(2, 2, DBYTE3_Dq0LnSel, 0x2);
      ms_write_ddrc_reg(2, 2, DBYTE3_Dq1LnSel, 0x0);
      ms_write_ddrc_reg(2, 2, DBYTE3_Dq2LnSel, 0x7);
      ms_write_ddrc_reg(2, 2, DBYTE3_Dq3LnSel, 0x5);
      ms_write_ddrc_reg(2, 2, DBYTE3_Dq4LnSel, 0x6);
      ms_write_ddrc_reg(2, 2, DBYTE3_Dq5LnSel, 0x4);
      ms_write_ddrc_reg(2, 2, DBYTE3_Dq6LnSel, 0x3);
      ms_write_ddrc_reg(2, 2, DBYTE3_Dq7LnSel, 0x1);
   }
   if (memshire3) {
      ms_write_ddrc_reg(3, 2, DBYTE0_Dq0LnSel, 0x7);
      ms_write_ddrc_reg(3, 2, DBYTE0_Dq1LnSel, 0x5);
      ms_write_ddrc_reg(3, 2, DBYTE0_Dq2LnSel, 0x6);
      ms_write_ddrc_reg(3, 2, DBYTE0_Dq3LnSel, 0x0);
      ms_write_ddrc_reg(3, 2, DBYTE0_Dq4LnSel, 0x4);
      ms_write_ddrc_reg(3, 2, DBYTE0_Dq5LnSel, 0x3);
      ms_write_ddrc_reg(3, 2, DBYTE0_Dq6LnSel, 0x2);
      ms_write_ddrc_reg(3, 2, DBYTE0_Dq7LnSel, 0x1);
      ms_write_ddrc_reg(3, 2, DBYTE1_Dq0LnSel, 0x0);
      ms_write_ddrc_reg(3, 2, DBYTE1_Dq1LnSel, 0x4);
      ms_write_ddrc_reg(3, 2, DBYTE1_Dq2LnSel, 0x2);
      ms_write_ddrc_reg(3, 2, DBYTE1_Dq3LnSel, 0x7);
      ms_write_ddrc_reg(3, 2, DBYTE1_Dq4LnSel, 0x1);
      ms_write_ddrc_reg(3, 2, DBYTE1_Dq5LnSel, 0x3);
      ms_write_ddrc_reg(3, 2, DBYTE1_Dq6LnSel, 0x5);
      ms_write_ddrc_reg(3, 2, DBYTE1_Dq7LnSel, 0x6);
      ms_write_ddrc_reg(3, 2, DBYTE2_Dq0LnSel, 0x2);
      ms_write_ddrc_reg(3, 2, DBYTE2_Dq1LnSel, 0x5);
      ms_write_ddrc_reg(3, 2, DBYTE2_Dq2LnSel, 0x4);
      ms_write_ddrc_reg(3, 2, DBYTE2_Dq3LnSel, 0x3);
      ms_write_ddrc_reg(3, 2, DBYTE2_Dq4LnSel, 0x0);
      ms_write_ddrc_reg(3, 2, DBYTE2_Dq5LnSel, 0x7);
      ms_write_ddrc_reg(3, 2, DBYTE2_Dq6LnSel, 0x6);
      ms_write_ddrc_reg(3, 2, DBYTE2_Dq7LnSel, 0x1);
      ms_write_ddrc_reg(3, 2, DBYTE3_Dq0LnSel, 0x7);
      ms_write_ddrc_reg(3, 2, DBYTE3_Dq1LnSel, 0x6);
      ms_write_ddrc_reg(3, 2, DBYTE3_Dq2LnSel, 0x4);
      ms_write_ddrc_reg(3, 2, DBYTE3_Dq3LnSel, 0x5);
      ms_write_ddrc_reg(3, 2, DBYTE3_Dq4LnSel, 0x0);
      ms_write_ddrc_reg(3, 2, DBYTE3_Dq5LnSel, 0x3);
      ms_write_ddrc_reg(3, 2, DBYTE3_Dq6LnSel, 0x2);
      ms_write_ddrc_reg(3, 2, DBYTE3_Dq7LnSel, 0x1);
   }
   if (memshire4) {
      ms_write_ddrc_reg(4, 2, DBYTE0_Dq0LnSel, 0x7);
      ms_write_ddrc_reg(4, 2, DBYTE0_Dq1LnSel, 0x5);
      ms_write_ddrc_reg(4, 2, DBYTE0_Dq2LnSel, 0x6);
      ms_write_ddrc_reg(4, 2, DBYTE0_Dq3LnSel, 0x0);
      ms_write_ddrc_reg(4, 2, DBYTE0_Dq4LnSel, 0x4);
      ms_write_ddrc_reg(4, 2, DBYTE0_Dq5LnSel, 0x3);
      ms_write_ddrc_reg(4, 2, DBYTE0_Dq6LnSel, 0x2);
      ms_write_ddrc_reg(4, 2, DBYTE0_Dq7LnSel, 0x1);
      ms_write_ddrc_reg(4, 2, DBYTE1_Dq0LnSel, 0x0);
      ms_write_ddrc_reg(4, 2, DBYTE1_Dq1LnSel, 0x4);
      ms_write_ddrc_reg(4, 2, DBYTE1_Dq2LnSel, 0x2);
      ms_write_ddrc_reg(4, 2, DBYTE1_Dq3LnSel, 0x7);
      ms_write_ddrc_reg(4, 2, DBYTE1_Dq4LnSel, 0x1);
      ms_write_ddrc_reg(4, 2, DBYTE1_Dq5LnSel, 0x3);
      ms_write_ddrc_reg(4, 2, DBYTE1_Dq6LnSel, 0x5);
      ms_write_ddrc_reg(4, 2, DBYTE1_Dq7LnSel, 0x6);
      ms_write_ddrc_reg(4, 2, DBYTE2_Dq0LnSel, 0x2);
      ms_write_ddrc_reg(4, 2, DBYTE2_Dq1LnSel, 0x5);
      ms_write_ddrc_reg(4, 2, DBYTE2_Dq2LnSel, 0x4);
      ms_write_ddrc_reg(4, 2, DBYTE2_Dq3LnSel, 0x3);
      ms_write_ddrc_reg(4, 2, DBYTE2_Dq4LnSel, 0x0);
      ms_write_ddrc_reg(4, 2, DBYTE2_Dq5LnSel, 0x7);
      ms_write_ddrc_reg(4, 2, DBYTE2_Dq6LnSel, 0x6);
      ms_write_ddrc_reg(4, 2, DBYTE2_Dq7LnSel, 0x1);
      ms_write_ddrc_reg(4, 2, DBYTE3_Dq0LnSel, 0x7);
      ms_write_ddrc_reg(4, 2, DBYTE3_Dq1LnSel, 0x6);
      ms_write_ddrc_reg(4, 2, DBYTE3_Dq2LnSel, 0x4);
      ms_write_ddrc_reg(4, 2, DBYTE3_Dq3LnSel, 0x5);
      ms_write_ddrc_reg(4, 2, DBYTE3_Dq4LnSel, 0x0);
      ms_write_ddrc_reg(4, 2, DBYTE3_Dq5LnSel, 0x3);
      ms_write_ddrc_reg(4, 2, DBYTE3_Dq6LnSel, 0x2);
      ms_write_ddrc_reg(4, 2, DBYTE3_Dq7LnSel, 0x1);
   }
   if (memshire5) {
      ms_write_ddrc_reg(5, 2, DBYTE0_Dq0LnSel, 0x3);
      ms_write_ddrc_reg(5, 2, DBYTE0_Dq1LnSel, 0x4);
      ms_write_ddrc_reg(5, 2, DBYTE0_Dq2LnSel, 0x6);
      ms_write_ddrc_reg(5, 2, DBYTE0_Dq3LnSel, 0x7);
      ms_write_ddrc_reg(5, 2, DBYTE0_Dq4LnSel, 0x2);
      ms_write_ddrc_reg(5, 2, DBYTE0_Dq5LnSel, 0x5);
      ms_write_ddrc_reg(5, 2, DBYTE0_Dq6LnSel, 0x1);
      ms_write_ddrc_reg(5, 2, DBYTE0_Dq7LnSel, 0x0);
      ms_write_ddrc_reg(5, 2, DBYTE1_Dq0LnSel, 0x1);
      ms_write_ddrc_reg(5, 2, DBYTE1_Dq1LnSel, 0x2);
      ms_write_ddrc_reg(5, 2, DBYTE1_Dq2LnSel, 0x0);
      ms_write_ddrc_reg(5, 2, DBYTE1_Dq3LnSel, 0x4);
      ms_write_ddrc_reg(5, 2, DBYTE1_Dq4LnSel, 0x6);
      ms_write_ddrc_reg(5, 2, DBYTE1_Dq5LnSel, 0x3);
      ms_write_ddrc_reg(5, 2, DBYTE1_Dq6LnSel, 0x7);
      ms_write_ddrc_reg(5, 2, DBYTE1_Dq7LnSel, 0x5);
      ms_write_ddrc_reg(5, 2, DBYTE2_Dq0LnSel, 0x6);
      ms_write_ddrc_reg(5, 2, DBYTE2_Dq1LnSel, 0x4);
      ms_write_ddrc_reg(5, 2, DBYTE2_Dq2LnSel, 0x3);
      ms_write_ddrc_reg(5, 2, DBYTE2_Dq3LnSel, 0x5);
      ms_write_ddrc_reg(5, 2, DBYTE2_Dq4LnSel, 0x0);
      ms_write_ddrc_reg(5, 2, DBYTE2_Dq5LnSel, 0x2);
      ms_write_ddrc_reg(5, 2, DBYTE2_Dq6LnSel, 0x7);
      ms_write_ddrc_reg(5, 2, DBYTE2_Dq7LnSel, 0x1);
      ms_write_ddrc_reg(5, 2, DBYTE3_Dq0LnSel, 0x2);
      ms_write_ddrc_reg(5, 2, DBYTE3_Dq1LnSel, 0x0);
      ms_write_ddrc_reg(5, 2, DBYTE3_Dq2LnSel, 0x7);
      ms_write_ddrc_reg(5, 2, DBYTE3_Dq3LnSel, 0x5);
      ms_write_ddrc_reg(5, 2, DBYTE3_Dq4LnSel, 0x6);
      ms_write_ddrc_reg(5, 2, DBYTE3_Dq5LnSel, 0x4);
      ms_write_ddrc_reg(5, 2, DBYTE3_Dq6LnSel, 0x3);
      ms_write_ddrc_reg(5, 2, DBYTE3_Dq7LnSel, 0x1);
   }
   if (memshire6) {
      ms_write_ddrc_reg(6, 2, DBYTE0_Dq0LnSel, 0x3);
      ms_write_ddrc_reg(6, 2, DBYTE0_Dq1LnSel, 0x2);
      ms_write_ddrc_reg(6, 2, DBYTE0_Dq2LnSel, 0x7);
      ms_write_ddrc_reg(6, 2, DBYTE0_Dq3LnSel, 0x6);
      ms_write_ddrc_reg(6, 2, DBYTE0_Dq4LnSel, 0x1);
      ms_write_ddrc_reg(6, 2, DBYTE0_Dq5LnSel, 0x0);
      ms_write_ddrc_reg(6, 2, DBYTE0_Dq6LnSel, 0x4);
      ms_write_ddrc_reg(6, 2, DBYTE0_Dq7LnSel, 0x5);
      ms_write_ddrc_reg(6, 2, DBYTE1_Dq0LnSel, 0x5);
      ms_write_ddrc_reg(6, 2, DBYTE1_Dq1LnSel, 0x1);
      ms_write_ddrc_reg(6, 2, DBYTE1_Dq2LnSel, 0x7);
      ms_write_ddrc_reg(6, 2, DBYTE1_Dq3LnSel, 0x6);
      ms_write_ddrc_reg(6, 2, DBYTE1_Dq4LnSel, 0x0);
      ms_write_ddrc_reg(6, 2, DBYTE1_Dq5LnSel, 0x2);
      ms_write_ddrc_reg(6, 2, DBYTE1_Dq6LnSel, 0x4);
      ms_write_ddrc_reg(6, 2, DBYTE1_Dq7LnSel, 0x3);
      ms_write_ddrc_reg(6, 2, DBYTE2_Dq0LnSel, 0x3);
      ms_write_ddrc_reg(6, 2, DBYTE2_Dq1LnSel, 0x5);
      ms_write_ddrc_reg(6, 2, DBYTE2_Dq2LnSel, 0x4);
      ms_write_ddrc_reg(6, 2, DBYTE2_Dq3LnSel, 0x7);
      ms_write_ddrc_reg(6, 2, DBYTE2_Dq4LnSel, 0x2);
      ms_write_ddrc_reg(6, 2, DBYTE2_Dq5LnSel, 0x0);
      ms_write_ddrc_reg(6, 2, DBYTE2_Dq6LnSel, 0x1);
      ms_write_ddrc_reg(6, 2, DBYTE2_Dq7LnSel, 0x6);
      ms_write_ddrc_reg(6, 2, DBYTE3_Dq0LnSel, 0x1);
      ms_write_ddrc_reg(6, 2, DBYTE3_Dq1LnSel, 0x3);
      ms_write_ddrc_reg(6, 2, DBYTE3_Dq2LnSel, 0x4);
      ms_write_ddrc_reg(6, 2, DBYTE3_Dq3LnSel, 0x0);
      ms_write_ddrc_reg(6, 2, DBYTE3_Dq4LnSel, 0x6);
      ms_write_ddrc_reg(6, 2, DBYTE3_Dq5LnSel, 0x2);
      ms_write_ddrc_reg(6, 2, DBYTE3_Dq6LnSel, 0x5);
      ms_write_ddrc_reg(6, 2, DBYTE3_Dq7LnSel, 0x7);
   }
   if (memshire7) {
      ms_write_ddrc_reg(7, 2, DBYTE0_Dq0LnSel, 0x5);
      ms_write_ddrc_reg(7, 2, DBYTE0_Dq1LnSel, 0x2);
      ms_write_ddrc_reg(7, 2, DBYTE0_Dq2LnSel, 0x3);
      ms_write_ddrc_reg(7, 2, DBYTE0_Dq3LnSel, 0x0);
      ms_write_ddrc_reg(7, 2, DBYTE0_Dq4LnSel, 0x1);
      ms_write_ddrc_reg(7, 2, DBYTE0_Dq5LnSel, 0x7);
      ms_write_ddrc_reg(7, 2, DBYTE0_Dq6LnSel, 0x4);
      ms_write_ddrc_reg(7, 2, DBYTE0_Dq7LnSel, 0x6);
      ms_write_ddrc_reg(7, 2, DBYTE1_Dq0LnSel, 0x3);
      ms_write_ddrc_reg(7, 2, DBYTE1_Dq1LnSel, 0x2);
      ms_write_ddrc_reg(7, 2, DBYTE1_Dq2LnSel, 0x7);
      ms_write_ddrc_reg(7, 2, DBYTE1_Dq3LnSel, 0x0);
      ms_write_ddrc_reg(7, 2, DBYTE1_Dq4LnSel, 0x6);
      ms_write_ddrc_reg(7, 2, DBYTE1_Dq5LnSel, 0x1);
      ms_write_ddrc_reg(7, 2, DBYTE1_Dq6LnSel, 0x5);
      ms_write_ddrc_reg(7, 2, DBYTE1_Dq7LnSel, 0x4);
      ms_write_ddrc_reg(7, 2, DBYTE2_Dq0LnSel, 0x6);
      ms_write_ddrc_reg(7, 2, DBYTE2_Dq1LnSel, 0x3);
      ms_write_ddrc_reg(7, 2, DBYTE2_Dq2LnSel, 0x4);
      ms_write_ddrc_reg(7, 2, DBYTE2_Dq3LnSel, 0x2);
      ms_write_ddrc_reg(7, 2, DBYTE2_Dq4LnSel, 0x5);
      ms_write_ddrc_reg(7, 2, DBYTE2_Dq5LnSel, 0x7);
      ms_write_ddrc_reg(7, 2, DBYTE2_Dq6LnSel, 0x1);
      ms_write_ddrc_reg(7, 2, DBYTE2_Dq7LnSel, 0x0);
      ms_write_ddrc_reg(7, 2, DBYTE3_Dq0LnSel, 0x3);
      ms_write_ddrc_reg(7, 2, DBYTE3_Dq1LnSel, 0x1);
      ms_write_ddrc_reg(7, 2, DBYTE3_Dq2LnSel, 0x5);
      ms_write_ddrc_reg(7, 2, DBYTE3_Dq3LnSel, 0x0);
      ms_write_ddrc_reg(7, 2, DBYTE3_Dq4LnSel, 0x7);
      ms_write_ddrc_reg(7, 2, DBYTE3_Dq5LnSel, 0x2);
      ms_write_ddrc_reg(7, 2, DBYTE3_Dq6LnSel, 0x6);
      ms_write_ddrc_reg(7, 2, DBYTE3_Dq7LnSel, 0x4);
   }
   return 0;
}

//
// Phase 2 of the DDR initialization sequence.
//
uint32_t ms_init_seq_phase2 (uint32_t memshire, uint32_t config_real_pll) {
   //
   // Turn off pub reset
   //

   // To avoid timing issues on the pub reset signal (Reset), deassertion should occur
   // via the DDR subsytem clock and reset control register
   ms_write_ddrc_reg(memshire, 0, SS_clk_rst_ctrl, 0xf7ffffff); // drive pub reset from DDR subsystem reg
   if (config_real_pll) {
      ms_write_esr(memshire, ddrc_reset_ctl, 0x108); // stop driving pub reset from memshire ESR
   } else {
      ms_write_esr(memshire, ddrc_reset_ctl, 0x008); // stop driving pub reset turn off pub reset from
   }

   ms_write_ddrc_reg(memshire, 0, SS_clk_rst_ctrl, 0xffffffff); // stop driving pub reset from DDR subsystem reg

   ms_read_ddrc_reg(memshire, 0, SS_clk_rst_ctrl); // make previous writes are complete
   ms_read_esr(memshire, ddrc_reset_ctl);
   return 0;
}


//
// Phase 3_01 of the DDR initialization sequence.
//
// This routine is used to initialize the PHY before training.
//
uint32_t ms_init_seq_phase3_01 (uint32_t memshire, uint32_t config_800mhz, uint32_t config_933mhz) {
   if (config_800mhz) {
      ms_init_ddr_phy_800_pre(memshire);
   } else {
      if (config_933mhz) {
         ms_init_ddr_phy_933_pre(memshire);
      } else {
         ms_init_ddr_phy_1067_pre(memshire);
      }
   }
   return 0;
}

//
// Phase 3_01_skiptrain of the DDR initialization sequence.
//
// This routine is used to initialize the PHY before training.
//
uint32_t ms_init_seq_phase3_01_skiptrain (uint32_t memshire, uint32_t config_800mhz, uint32_t config_933mhz) {
   if (config_800mhz) {
      ms_init_ddr_phy_800_pre_skiptrain(memshire);
   } else {
      if (config_933mhz) {
         ms_init_ddr_phy_933_pre_skiptrain(memshire);
      } else {
         ms_init_ddr_phy_1067_pre_skiptrain(memshire);
      }
   }
   return 0;
}

//
// Phase 3_02 of the DDR initialization sequence.
//
// This routine is used to load the PHY RAMs with training code.
//
uint32_t ms_init_seq_phase3_02_no_loop (uint32_t memshire, uint32_t config_800mhz, uint32_t config_933mhz) {
   if (config_800mhz) {
      ms_ddr_phy_1d_train_from_file(2, memshire);
   } else {
      if (config_933mhz) {
         ms_ddr_phy_1d_train_from_file(1, memshire);
      } else {
         ms_ddr_phy_1d_train_from_file(0, memshire);
      }
   }
   return 0;
}


//
// Phase 3_03 of the DDR initialization sequence.
//
// This routine is used to start the training.
//
uint32_t ms_init_seq_phase3_03 (uint32_t memshire, uint32_t config_debug_level, uint32_t config_sim_only) {
   if (config_sim_only) {
      ms_write_ddrc_addr(memshire, 0x62150000, 0x00000600); // configure for short init
   }
   ms_write_ddrc_addr(memshire, 0x62150024, config_debug_level);
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x00000001);         // MicroContMuxSel=1
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x00000001);         // MicroContMuxSel=1
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroReset, 0x00000009);              // ResetToMicro=1, RSVDMicro=0, TestWakeup=0,
                                                                                 // StallToMicro=1
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroReset, 0x00000001);              // ResetToMicro=0, RSVDMicro=0, TestWakeup=0,
                                                                                 // StallToMicro=1
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroReset, 0x00000000);              // ResetToMicro=0, RSVDMicro=0, TestWakeup=0,
                                                                                 // StallToMicro=0

   ms_read_ddrc_reg(memshire, 2, APBONLY0_MicroReset); // make previous writes are complete
   return 0;
}


//
// Phase 3_04 of the DDR initialization sequence.
//
// This routine is used to poll for the end of training.
//
uint32_t ms_init_seq_phase3_04_no_loop (uint32_t memshire, uint32_t train_poll_max_iterations, uint32_t train_poll_iteration_delay) {
   ms_wait_for_training(memshire, train_poll_max_iterations, train_poll_iteration_delay);
   return 0;
}

//
// Phase 3_05 of the DDR initialization sequence.
//
// This routine is used to load the PHY RAMs with training code.
//
uint32_t ms_init_seq_phase3_05_no_loop (uint32_t memshire, uint32_t config_800mhz, uint32_t config_933mhz) {
   if (config_800mhz) {
      ms_ddr_phy_2d_train_from_file(2, memshire);
   } else {
      if (config_933mhz) {
         ms_ddr_phy_2d_train_from_file(1, memshire);
      } else {
         ms_ddr_phy_2d_train_from_file(0, memshire);
      }
   }
   return 0;
}

//
// Phase 3_06 of the DDR initialization sequence.
//
// This routine is used to start the 2d training.
//
uint32_t ms_init_seq_phase3_06 (uint32_t memshire, uint32_t config_debug_level, uint32_t config_sim_only) {
   if (config_sim_only) {
      ms_write_ddrc_addr(memshire, 0x62150000, 0x00000600); // sim only timing
   }

   ms_write_ddrc_addr(memshire, 0x62150024, config_debug_level);
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x00000001);         // MicroContMuxSel=1
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x00000001);         // MicroContMuxSel=1
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroReset, 0x00000009);              // ResetToMicro=1, RSVDMicro=0, TestWakeup=0,
                                                                                 // StallToMicro=1
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroReset, 0x00000001);              // ResetToMicro=0, RSVDMicro=0, TestWakeup=0,
                                                                                 // StallToMicro=1
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroReset, 0x00000000);              // ResetToMicro=0, RSVDMicro=0, TestWakeup=0,
                                                                                 // StallToMicro=0

   ms_read_ddrc_reg(memshire, 2, APBONLY0_MicroReset); // make previous writes are complete
   return 0;
}


//
// Phase 3_07 of the DDR initialization sequence.
//
// This routine is used to poll for the end of training.
//
uint32_t ms_init_seq_phase3_07_no_loop (uint32_t memshire, uint32_t train_poll_max_iterations, uint32_t train_poll_iteration_delay) {
   ms_wait_for_training_2d(memshire, train_poll_max_iterations, train_poll_iteration_delay);
   return 0;
}


//
// Phase 3_08 of the DDR initialization sequence.
//
// This routine is used to finish the training sequence.
//
uint32_t ms_init_seq_phase3_08 (uint32_t memshire, uint32_t config_ecc, uint32_t config_800mhz, uint32_t config_933mhz, uint32_t config_4gb, uint32_t config_8gb, uint32_t config_32gb) {

   ms_write_ddrc_reg(memshire, 2, APBONLY0_DctWriteProt, 0x00000001);            // DctWriteProt=1
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroReset, 0x00000001);              // ResetToMicro=0, RSVDMicro=0, TestWakeup=0,
                                                                                 // StallToMicro=1
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x00000000);         // MicroContMuxSel=0
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x00000001);         // MicroContMuxSel=1
   ms_write_ddrc_reg(memshire, 2, APBONLY0_MicroContMuxSel, 0x00000000);         // MicroContMuxSel=0
   return 0;
}


//
// Phase 4_01 of the DDR initialization sequence.
//
// This routine is used to initialize the PHY afer training.
//
uint32_t ms_init_seq_phase4_01 (uint32_t memshire, uint32_t config_800mhz, uint32_t config_933mhz) {
   if (config_800mhz) {
      ms_init_ddr_phy_800_post(memshire);
   } else {
      if (config_933mhz) {
         ms_init_ddr_phy_933_post(memshire);
      } else {
         ms_init_ddr_phy_1067_post(memshire);
      }
   }

   ms_write_both_ddrc_reg(memshire, DFIMISC, 0x000010a0);                        // dfi_frequency=0x10, lp_optimized_write=1,
                                                                                 // dfi_init_start=1, ctl_idle_en=0,
                                                                                 // dfi_data_cs_polarity=0, phy_dbi_mode=0,
                                                                                 // dfi_init_complete_en=0
   return 0;
}

//
// Phase 4_01_skiptrain of the DDR initialization sequence.
//
// This routine is used to initialize the PHY afer training.
//
uint32_t ms_init_seq_phase4_01_skiptrain (uint32_t memshire, uint32_t config_800mhz, uint32_t config_933mhz) {
   if (config_800mhz) {
      ms_init_ddr_phy_800_post_skiptrain(memshire);
   } else {
      if (config_933mhz) {
         ms_init_ddr_phy_933_post_skiptrain(memshire);
      } else {
         ms_init_ddr_phy_1067_post_skiptrain(memshire);
      }
   }

   ms_write_both_ddrc_reg(memshire, DFIMISC, 0x000010a0);                        // dfi_frequency=0x10, lp_optimized_write=1,
                                                                                 // dfi_init_start=1, ctl_idle_en=0,
                                                                                 // dfi_data_cs_polarity=0, phy_dbi_mode=0,
                                                                                 // dfi_init_complete_en=0
   return 0;
}

//
// Phase 4_02 of the DDR initialization sequence.
//
uint32_t ms_init_seq_phase4_02 (uint32_t memshire, uint32_t config_auto_precharge, uint32_t config_disable_unused_clks, uint32_t config_training) {

   // step 14
   ms_poll_ddrc_reg(memshire, 2, MASTER0_CalBusy, 0x0, 0x1, 1000, 1); // Wait for Calibration to be done

   // step 15
   ms_write_both_ddrc_reg(memshire, SWCTL, 0x00000000);                          // sw_done=0

   // step 16
   ms_write_both_ddrc_reg(memshire, DFIMISC, 0x000010a1);                        // set dfi_init_start=1
                                                                                 // dfi_frequency=0x10, lp_optimized_write=1,
                                                                                 // ctl_idle_en=0, dfi_data_cs_polarity=0,
                                                                                 // phy_dbi_mode=0, dfi_init_complete_en=1

   // step 17
   ms_write_both_ddrc_reg(memshire, SWCTL, 0x00000001);                          // sw_done=1
   ms_poll_ddrc_reg(memshire, 0, SWSTAT, 0x1, 0x1, 100, 1);                      // make sure sw_done_ack=1
   ms_poll_ddrc_reg(memshire, 1, SWSTAT, 0x1, 0x1, 100, 1);                      // make sure sw_done_ack=1

   // step 18
   ms_poll_ddrc_reg(memshire, 0, DFISTAT, 0x1, 0x1, 1000, 1);                    // dfi_init_complete
                                                                                 // dfi_init_complete=1
   ms_poll_ddrc_reg(memshire, 1, DFISTAT, 0x1, 0x1, 1000, 1);                    // dfi_init_complete=1

   // step 19
   ms_write_both_ddrc_reg(memshire, SWCTL, 0x00000000);                          // sw_done=0

   // step 20
   ms_write_both_ddrc_reg(memshire, DFIMISC, 0x00001080);                        // set dfi_init_start=0
                                                                                 // dfi_frequency=0x10, lp_optimized_write=1,
                                                                                 // ctl_idle_en=0, dfi_data_cs_polarity=0,
                                                                                 // phy_dbi_mode=0, dfi_init_complete_en=0

   // step 21
   // Update controller registgers based on training results
   if (config_training) {
      post_train_update_regs(memshire);
   }

   // step 22
   ms_write_both_ddrc_reg(memshire, DFIMISC, 0x00001081);                        // set dfi_init_complete_en to 1
                                                                                 // dfi_frequency=0x10, lp_optimized_write=1,
                                                                                 // dfi_init_start=0, ctl_idle_en=0,
                                                                                 // dfi_data_cs_polarity=0, phy_dbi_mode=0,
                                                                                 // dfi_init_complete_en=1

   // step 23
   ms_write_both_ddrc_reg(memshire, PWRCTL, 0x00000000);                         // disable self-refresh mode for performance reasons
                                                                                 // lpddr4_sr_allowed=0, dis_cam_drain_selfref=0,
                                                                                 // stay_in_selfref=0, selfref_sw=0,
                                                                                 // en_dfi_dram_clk_disable=0, deeppowerdown_en=0,
                                                                                 // powerdown_en=0, selfref_en=0

   // step 24
   ms_write_both_ddrc_reg(memshire, SWCTL, 0x00000001);                          // sw_done=1
   ms_poll_ddrc_reg(memshire, 0, SWSTAT, 0x1, 0x1, 100, 1);                      // make sure sw_done_ack=1
   ms_poll_ddrc_reg(memshire, 1, SWSTAT, 0x1, 0x1, 100, 1);                      // make sure sw_done_ack=1

   // step 25
   ms_poll_ddrc_reg(memshire, 0, STAT, 0x1, 0x1, 1000, 1);                       // make sure operating mode is not init
   ms_poll_ddrc_reg(memshire, 1, STAT, 0x1, 0x1, 1000, 1);                       // make sure operating mode is not init

   if (config_training) {
      ms_write_both_ddrc_reg(memshire, RFSHCTL3, 0x00000000);                    // refresh_update_level=0, dis_auto_refresh=0
   }

   if (config_auto_precharge) {
      ms_write_esr(memshire, ddrc_main_ctl, 0xf); // set memshire esr to enable auto_pre_charge
   }

   // enable AXI ports
   ms_write_both_ddrc_reg(memshire, PCTRL_0, 0x00000001);                        // port_en=1
   ms_write_both_ddrc_reg(memshire, PCTRL_1, 0x00000001);                        // port_en=1

   if (config_disable_unused_clks) {
      ms_write_esr(memshire, ms_clk_gate_ctl, 0x1); // turn off memshire debug clock
      ms_write_ddrc_reg(memshire, 0, SS_clk_rst_ctrl, 0xffffe3f5); // turn off pclks and scrub clocks
   }
   return 0;
}

//
// ms_init_clear_ddr : writes 0 to all protected DDR memory to have good ECC
//
// Follows sequence from pae 397 of uMCTL2 Databook v3.50a
//
uint32_t ms_init_clear_ddr (uint32_t memshire, uint32_t config_disable_unused_clks, uint32_t config_4gb, uint32_t config_8gb, uint32_t config_32gb, uint32_t init_pattern) {

   // turn on clocks in case they were diasbled before
   ms_write_ddrc_reg(memshire, 0, SS_clk_rst_ctrl, 0xffffffff);

   //setting scrub start end range masks to zero causes all of memory to be initialized
   ms_write_esr(memshire, ddrc_scrub1, 0x000000000);
   if (config_4gb) {
      ms_write_esr(memshire, ddrc_scrub2, 0x006ffffff);
   } else {
      if (config_8gb) {
         ms_write_esr(memshire, ddrc_scrub2, 0x00dffffff);
      } else {
         if (config_32gb) {
            ms_write_esr(memshire, ddrc_scrub2, 0x037ffffff);
         } else {
            ms_write_esr(memshire, ddrc_scrub2, 0x01bffffff);
         }
      }
   }

   // set ECCCFG1.ecc_parity_region_lock = 1
   ms_write_both_ddrc_reg(memshire, ECCCFG1, 0x7b0);                             // active_blk_channel=7, blk_channel_active_term=1,
                                                                                 // ecc_region_waste_lock=1,
                                                                                 // ecc_region_parity_lock=1, data_poison_bit=0,
                                                                                 // data_poison_en=0

   // disable AXI ports
   ms_write_both_ddrc_reg(memshire, PCTRL_0, 0x00000000);                        // port_en=0
   ms_write_both_ddrc_reg(memshire, PCTRL_1, 0x00000000);                        // port_en=0

   // set SBRCTL.scrub_mode=0 and SBRCTL.scrub_interval = 0
   ms_write_both_ddrc_reg(memshire, SBRCTL, 0x00000006);                         // scrub_interval=0, scrub_burst=0, scrub_mode=1,
                                                                                 // scrub_during_lowpower=1, scrub_en=0

   // write 0 data
   ms_write_both_ddrc_reg(memshire, SBRWDATA0, init_pattern);                    // scrub_pattern0=0

   // enable initialation by programming SBRCTL.scrub_en = 1
   ms_write_both_ddrc_reg(memshire, SBRCTL, 0x00000007);                         // scrub_interval=0, scrub_burst=0, scrub_mode=1,
                                                                                 // scrub_during_lowpower=1, scrub_en=1

   // wait for scrub_done (SBRSTAT bit 1) to be 1 on both controllers
   ms_poll_ddrc_reg(memshire, 0, SBRSTAT, 0x2, 0x2, 10000, 1000);                // scrub_done=1
   ms_poll_ddrc_reg(memshire, 1, SBRSTAT, 0x2, 0x2, 10000, 1000);                // scrub_done=1

   // wait for scrub_busy (SBRSTAT bit 0) to be 0 on both controllers
   ms_poll_ddrc_reg(memshire, 0, SBRSTAT, 0x0, 0x1, 10000, 1000);                // scrub_busy=0
   ms_poll_ddrc_reg(memshire, 1, SBRSTAT, 0x0, 0x1, 10000, 1000);                // scrub_busy=0

   // disable scrubber
   ms_write_both_ddrc_reg(memshire, SBRCTL, 0x00000000);                         // scrub_interval=0, scrub_burst=0, scrub_mode=0,
                                                                                 // scrub_during_lowpower=0, scrub_en=0

   // enable AXI ports
   ms_write_both_ddrc_reg(memshire, PCTRL_0, 0x00000001);                        // port_en=1
   ms_write_both_ddrc_reg(memshire, PCTRL_1, 0x00000001);                        // port_en=1

   // turn off pclks and scrub clock if desired
   if (config_disable_unused_clks) {
      // turn off pclks and scrub clocks
      ms_write_ddrc_reg(memshire, 0, SS_clk_rst_ctrl, 0xffffe3f5);
   }

   return 0;
}

