/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

#ifndef __PVTC_MOORTEC_REGS_H__
#define __PVTC_MOORTEC_REGS_H__

//#include "pvtcMoortec_regs_macro.h
//if NUMBER_OF_CH<16, space needs to be filled after VM_n_CH_m_SDIF_DATA

#define NUMBER_OF_TS 8
#define NUMBER_OF_PD 8
#define NUMBER_OF_VM 8
#define NUMBER_OF_CH 16

#if NUMBER_OF_TS>0
  #define TS_BASE 0x80
  #define TS_SIZE (0x40 + (0x40*NUMBER_OF_TS))
  #if NUMBER_OF_PD>0
    #define PD_BASE ((TS_BASE + TS_SIZE + 0x40) & 0xFF80)
    #define PD_SIZE (0x40 + (0x40*NUMBER_OF_PD))
    #if NUMBER_OF_VM>0
      #define VM_BASE ((TS_BASE + PD_BASE + TS_SIZE + PD_SIZE + (0x200*NUMBER_OF_CH/8)) & (0xFF00 << (NUMBER_OF_CH/8)))
      #define VM_SIZE 0x200*NUMBER_OF_CH/8 + 0x100*(NUMBER_OF_CH/8)*NUMBER_OF_VM
    #endif
  #endif
#endif

#define TS_AFTER           ((PD_BASE)-TS_SIZE-TS_BASE)/4
#define PD_AFTER           (VM_BASE-PD_SIZE-PD_BASE)/4
#define VM_AFTER_SDIF_DATA (0x40-NUMBER_OF_CH*4)/4

#define CAT(a, b) PRIMITIVE_CAT(a, b)
#define PRIMITIVE_CAT(a, b) a ## b

#define TS_REPEAT(N,M)   CAT(TS_REPEAT,N)(M)
#define PD_REPEAT(N,M)   CAT(PD_REPEAT,N)(M)
#define VM_REPEAT(N,C,M,X,Y) CAT(VM_REPEAT,N)(M,C,X,Y)

#define TS_REPEAT0(M)
#define TS_REPEAT1(M) M(0)
#define TS_REPEAT2(M) TS_REPEAT1(M) M(1)
#define TS_REPEAT3(M) TS_REPEAT2(M) M(2)
#define TS_REPEAT4(M) TS_REPEAT3(M) M(3)
#define TS_REPEAT5(M) TS_REPEAT4(M) M(4)
#define TS_REPEAT6(M) TS_REPEAT5(M) M(5)
#define TS_REPEAT7(M) TS_REPEAT6(M) M(6)
#define TS_REPEAT8(M) TS_REPEAT7(M) M(7)

#define PD_REPEAT0(M)
#define PD_REPEAT1(M) M(0)
#define PD_REPEAT2(M) PD_REPEAT1(M) M(1)
#define PD_REPEAT3(M) PD_REPEAT2(M) M(2)
#define PD_REPEAT4(M) PD_REPEAT3(M) M(3)
#define PD_REPEAT5(M) PD_REPEAT4(M) M(4)
#define PD_REPEAT6(M) PD_REPEAT5(M) M(5)
#define PD_REPEAT7(M) PD_REPEAT6(M) M(6)
#define PD_REPEAT8(M) PD_REPEAT7(M) M(7)

#define VM_REPEAT0(M,C,X,Y)
#define VM_REPEAT1(M,C,X,Y)  M(0) CAT(VM_CH_REPEAT,C)(X,0) CAT(VM_CH_REPEAT,C)(Y,0)
#define VM_REPEAT2(M,C,X,Y)  VM_REPEAT1(M,C,X,Y)  M(1)  CAT(VM_CH_REPEAT,C)(X,1)  CAT(VM_CH_REPEAT,C)(Y,1) 
#define VM_REPEAT3(M,C,X,Y)  VM_REPEAT2(M,C,X,Y)  M(2)  CAT(VM_CH_REPEAT,C)(X,2)  CAT(VM_CH_REPEAT,C)(Y,2) 
#define VM_REPEAT4(M,C,X,Y)  VM_REPEAT3(M,C,X,Y)  M(3)  CAT(VM_CH_REPEAT,C)(X,3)  CAT(VM_CH_REPEAT,C)(Y,3) 
#define VM_REPEAT5(M,C,X,Y)  VM_REPEAT4(M,C,X,Y)  M(4)  CAT(VM_CH_REPEAT,C)(X,4)  CAT(VM_CH_REPEAT,C)(Y,4) 
#define VM_REPEAT6(M,C,X,Y)  VM_REPEAT5(M,C,X,Y)  M(5)  CAT(VM_CH_REPEAT,C)(X,5)  CAT(VM_CH_REPEAT,C)(Y,5) 
#define VM_REPEAT7(M,C,X,Y)  VM_REPEAT6(M,C,X,Y)  M(6)  CAT(VM_CH_REPEAT,C)(X,6)  CAT(VM_CH_REPEAT,C)(Y,6) 
#define VM_REPEAT8(M,C,X,Y)  VM_REPEAT7(M,C,X,Y)  M(7)  CAT(VM_CH_REPEAT,C)(X,7)  CAT(VM_CH_REPEAT,C)(Y,7) 
#define VM_REPEAT9(M,C,X,Y)  VM_REPEAT8(M,C,X,Y)  M(8)  CAT(VM_CH_REPEAT,C)(X,8)  CAT(VM_CH_REPEAT,C)(Y,8) 
#define VM_REPEAT10(M,C,X,Y) VM_REPEAT9(M,C,X,Y)  M(9)  CAT(VM_CH_REPEAT,C)(X,9)  CAT(VM_CH_REPEAT,C)(Y,9) 
#define VM_REPEAT11(M,C,X,Y) VM_REPEAT10(M,C,X,Y) M(10) CAT(VM_CH_REPEAT,C)(X,10) CAT(VM_CH_REPEAT,C)(Y,10) 
#define VM_REPEAT12(M,C,X,Y) VM_REPEAT11(M,C,X,Y) M(11) CAT(VM_CH_REPEAT,C)(X,11) CAT(VM_CH_REPEAT,C)(Y,11) 
#define VM_REPEAT13(M,C,X,Y) VM_REPEAT12(M,C,X,Y) M(12) CAT(VM_CH_REPEAT,C)(X,12) CAT(VM_CH_REPEAT,C)(Y,12) 
#define VM_REPEAT14(M,C,X,Y) VM_REPEAT13(M,C,X,Y) M(13) CAT(VM_CH_REPEAT,C)(X,13) CAT(VM_CH_REPEAT,C)(Y,13) 
#define VM_REPEAT15(M,C,X,Y) VM_REPEAT14(M,C,X,Y) M(14) CAT(VM_CH_REPEAT,C)(X,14) CAT(VM_CH_REPEAT,C)(Y,14) 
#define VM_REPEAT16(M,C,X,Y) VM_REPEAT15(M,C,X,Y) M(15) CAT(VM_CH_REPEAT,C)(X,15) CAT(VM_CH_REPEAT,C)(Y,15) 
#define VM_REPEAT17(M,C,X,Y) VM_REPEAT16(M,C,X,Y) M(16) CAT(VM_CH_REPEAT,C)(X,16) CAT(VM_CH_REPEAT,C)(Y,16) 
#define VM_REPEAT18(M,C,X,Y) VM_REPEAT17(M,C,X,Y) M(17) CAT(VM_CH_REPEAT,C)(X,17) CAT(VM_CH_REPEAT,C)(Y,17) 
#define VM_REPEAT19(M,C,X,Y) VM_REPEAT18(M,C,X,Y) M(18) CAT(VM_CH_REPEAT,C)(X,18) CAT(VM_CH_REPEAT,C)(Y,18) 
#define VM_REPEAT20(M,C,X,Y) VM_REPEAT19(M,C,X,Y) M(19) CAT(VM_CH_REPEAT,C)(X,19) CAT(VM_CH_REPEAT,C)(Y,19) 
#define VM_REPEAT21(M,C,X,Y) VM_REPEAT20(M,C,X,Y) M(20) CAT(VM_CH_REPEAT,C)(X,20) CAT(VM_CH_REPEAT,C)(Y,20) 
#define VM_REPEAT22(M,C,X,Y) VM_REPEAT21(M,C,X,Y) M(21) CAT(VM_CH_REPEAT,C)(X,21) CAT(VM_CH_REPEAT,C)(Y,21) 
#define VM_REPEAT23(M,C,X,Y) VM_REPEAT22(M,C,X,Y) M(22) CAT(VM_CH_REPEAT,C)(X,22) CAT(VM_CH_REPEAT,C)(Y,22) 
#define VM_REPEAT24(M,C,X,Y) VM_REPEAT23(M,C,X,Y) M(23) CAT(VM_CH_REPEAT,C)(X,23) CAT(VM_CH_REPEAT,C)(Y,23) 

#define VM_CH_REPEAT0(X,N)
#define VM_CH_REPEAT1(X,N)  X(N,0)
#define VM_CH_REPEAT2(X,N)  VM_CH_REPEAT1(X,N)  X(N,1)
#define VM_CH_REPEAT3(X,N)  VM_CH_REPEAT2(X,N)  X(N,2)
#define VM_CH_REPEAT4(X,N)  VM_CH_REPEAT3(X,N)  X(N,3)
#define VM_CH_REPEAT5(X,N)  VM_CH_REPEAT4(X,N)  X(N,4)
#define VM_CH_REPEAT6(X,N)  VM_CH_REPEAT5(X,N)  X(N,5)
#define VM_CH_REPEAT7(X,N)  VM_CH_REPEAT6(X,N)  X(N,6)
#define VM_CH_REPEAT8(X,N)  VM_CH_REPEAT7(X,N)  X(N,7)
#define VM_CH_REPEAT9(X,N)  VM_CH_REPEAT8(X,N)  X(N,8)
#define VM_CH_REPEAT10(X,N) VM_CH_REPEAT9(X,N)  X(N,9)
#define VM_CH_REPEAT11(X,N) VM_CH_REPEAT10(X,N) X(N,10)
#define VM_CH_REPEAT12(X,N) VM_CH_REPEAT11(X,N) X(N,11)
#define VM_CH_REPEAT13(X,N) VM_CH_REPEAT12(X,N) X(N,12)
#define VM_CH_REPEAT14(X,N) VM_CH_REPEAT13(X,N) X(N,13)
#define VM_CH_REPEAT15(X,N) VM_CH_REPEAT14(X,N) X(N,14)
#define VM_CH_REPEAT16(X,N) VM_CH_REPEAT15(X,N) X(N,15)

#define TS_INDIVIDUAL_REGS(NUM)                           \
        volatile uint32_t TS_##NUM##_IRQ_ENABLE;          \
        volatile uint32_t TS_##NUM##_IRQ_STATUS;          \
        volatile uint32_t TS_##NUM##_IRQ_CLR;             \
        volatile uint32_t TS_##NUM##_IRQ_TEST;            \
        volatile uint32_t TS_##NUM##_SDIF_RDATA;          \
        volatile uint32_t TS_##NUM##_SDIF_DONE;           \
        volatile uint32_t TS_##NUM##_SDIF_DATA;           \
        volatile uint32_t unimplemented_ts_##NUM##_0;     \
        volatile uint32_t TS_##NUM##_ALARMA_CFG;          \
        volatile uint32_t TS_##NUM##_ALARMB_CFG;          \
        volatile uint32_t TS_##NUM##_SMPL_HILO;           \
        volatile uint32_t TS_##NUM##_HILO_RESET;          \
        volatile uint32_t unimplemented_ts_##NUM##_1 [4];

#define PD_INDIVIDUAL_REGS(NUM)                           \
        volatile uint32_t PD_##NUM##_IRQ_ENABLE;          \
        volatile uint32_t PD_##NUM##_IRQ_STATUS;          \
        volatile uint32_t PD_##NUM##_IRQ_CLR;             \
        volatile uint32_t PD_##NUM##_IRQ_TEST;            \
        volatile uint32_t PD_##NUM##_SDIF_RDATA;          \
        volatile uint32_t PD_##NUM##_SDIF_DONE;           \
        volatile uint32_t PD_##NUM##_SDIF_DATA;           \
        volatile uint32_t unimplemented_pd_##NUM##_0;     \
        volatile uint32_t PD_##NUM##_ALARMA_CFG;          \
        volatile uint32_t PD_##NUM##_ALARMB_CFG;          \
        volatile uint32_t PD_##NUM##_SMPL_HILO;           \
        volatile uint32_t PD_##NUM##_HILO_RESET;          \
        volatile uint32_t unimplemented_pd_##NUM##_1 [4];
	
#define VM_INDIVIDUAL_REGS(NUM)                         \
        volatile uint32_t VM_##NUM##_IRQ_ENABLE;        \
        volatile uint32_t VM_##NUM##_IRQ_STATUS;        \
        volatile uint32_t VM_##NUM##_IRQ_CLR;           \
        volatile uint32_t VM_##NUM##_IRQ_TEST;          \
        volatile uint32_t VM_##NUM##_IRQ_ALARMA_ENABLE; \
        volatile uint32_t VM_##NUM##_IRQ_ALARMA_STATUS; \
        volatile uint32_t VM_##NUM##_IRQ_ALARMA_CLR;    \
        volatile uint32_t VM_##NUM##_IRQ_ALARMA_TEST;   \
        volatile uint32_t VM_##NUM##_IRQ_ALARMB_ENABLE; \
        volatile uint32_t VM_##NUM##_IRQ_ALARMB_STATUS; \
        volatile uint32_t VM_##NUM##_IRQ_ALARMB_CLR;    \
        volatile uint32_t VM_##NUM##_IRQ_ALARMB_TEST;   \
        volatile uint32_t VM_##NUM##_SDIF_RDATA;        \
        volatile uint32_t VM_##NUM##_SDIF_DONE;      
	
#define VM_INDIVIDUAL_REGS_PER_CHAN_0(NUM,CHAN)               \
        volatile uint32_t VM_##NUM##_CH_##CHAN##_SDIF_DATA; 
	
#define VM_INDIVIDUAL_REGS_PER_CHAN_1(NUM,CHAN)               \
        volatile uint32_t VM_##NUM##_CH_##CHAN##_ALARMA_CFG;  \
        volatile uint32_t VM_##NUM##_CH_##CHAN##_ALARMB_CFG;  \
        volatile uint32_t VM_##NUM##_CH_##CHAN##_SMPL_HILO;   \
        volatile uint32_t VM_##NUM##_CH_##CHAN##_HILO_RESET;
	
		
struct pvtcMoortec_regs {
    volatile uint32_t PVTC_COMP_ID;
    volatile uint32_t PVTC_IP_CONFIG;
    volatile uint32_t PVTC_ID_NUM;
    volatile uint32_t PVTC_TM_SCRATCH;
    volatile uint32_t PVTC_REG_LOCK;
    volatile uint32_t PVTC_REG_LOCK_STATUS;
    volatile uint32_t PVTC_TAM_STATUS;
    volatile uint32_t PVTC_TAM_CLEAR;
    volatile uint32_t PVTC_TMR_CTRL;
    volatile uint32_t PVTC_TMR_STATUS;
    volatile uint32_t PVTC_TMR_IRQ_CLEAR;
    volatile uint32_t PVTC_TMR_IRQ_TEST;
    volatile uint32_t unimplemented_00 [4];
    volatile uint32_t IRQ_EN;
    volatile uint32_t unimplemented_01 [3];
    volatile uint32_t IRQ_TR_MASK;
    volatile uint32_t IRQ_TS_MASK;
    volatile uint32_t IRQ_VM_MASK;
    volatile uint32_t IRQ_PD_MASK;
    volatile uint32_t IRQ_TR_STATUS;
    volatile uint32_t IRQ_TS_STATUS;
    volatile uint32_t IRQ_VM_STATUS;
    volatile uint32_t IRQ_PD_STATUS;
    volatile uint32_t IRQ_TR_RAW_STATUS;
    volatile uint32_t IRQ_TS_RAW_STATUS;
    volatile uint32_t IRQ_VM_RAW_STATUS;
    volatile uint32_t IRQ_PD_RAW_STATUS;
#if NUMBER_OF_TS>0
    volatile uint32_t TS_CLK_SYNTH;
    volatile uint32_t TS_SDIF_DISABLE;
    volatile uint32_t TS_SDIF_STATUS;
    volatile uint32_t TS_SDIF;
    volatile uint32_t TS_SDIF_HALT;
    volatile uint32_t TS_SDIF_CTRL;
    volatile uint32_t unimplemented_02 [2];
    volatile uint32_t TS_SMPL_CTRL;
    volatile uint32_t TS_SMPL_CLR;
    volatile uint32_t TS_SMPL_CNT;
    volatile uint32_t unimplemented_03 [5];
    TS_REPEAT(NUMBER_OF_TS,TS_INDIVIDUAL_REGS)
    volatile uint32_t unimplemented_after_ts[TS_AFTER];
#endif
#if NUMBER_OF_PD>0
    volatile uint32_t PD_CLK_SYNTH;
    volatile uint32_t PD_SDIF_DISABLE;
    volatile uint32_t PD_SDIF_STATUS;
    volatile uint32_t PD_SDIF;
    volatile uint32_t PD_SDIF_HALT;
    volatile uint32_t PD_SDIF_CTRL;
    volatile uint32_t unimplemented_04 [2];
    volatile uint32_t PD_SMPL_CTRL;
    volatile uint32_t PD_SMPL_CLR;
    volatile uint32_t PD_SMPL_CNT;
    volatile uint32_t unimplemented_05 [5];
    PD_REPEAT(NUMBER_OF_PD,PD_INDIVIDUAL_REGS)
    volatile uint32_t unimplemented_after_pd[PD_AFTER];
#endif
#if NUMBER_OF_VM>0
    volatile uint32_t VM_CLK_SYNTH;
    volatile uint32_t VM_SDIF_DISABLE;
    volatile uint32_t VM_SDIF_STATUS;
    volatile uint32_t VM_SDIF;
    volatile uint32_t VM_SDIF_HALT;
    volatile uint32_t VM_SDIF_CTRL;
    volatile uint32_t unimplemented_06 [2];
    volatile uint32_t VM_SMPL_CTRL;
    volatile uint32_t VM_SMPL_CLR;
    volatile uint32_t VM_SMPL_CNT;
    volatile uint32_t unimplemented_07 [5];
    VM_REPEAT(NUMBER_OF_VM,NUMBER_OF_CH,VM_INDIVIDUAL_REGS,VM_INDIVIDUAL_REGS_PER_CHAN_0,VM_INDIVIDUAL_REGS_PER_CHAN_1);
#endif

};

#endif /* __PVTC_MOORTEC_REGS_H__ */
