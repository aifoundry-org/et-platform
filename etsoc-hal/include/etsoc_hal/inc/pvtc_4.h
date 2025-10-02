// TS  4 PD  4 VM  0 CH  16
// #define PVT_START_ADDR   0x0000                   
// #define PVT_END_ADDR     0x0040                   
// #define PVT_SIZE         0x0040                   
// #define IRQ_START_ADDR   0x0040                   
// #define IRQ_END_ADDR     0x0080                   
// #define IRQ_SIZE         0x0040                   
// #define TS_START_ADDR   0x0080                  
// #define TS_END_ADDR     0x01c0                  
// #define TS_SIZE         0x0140                  
// #define PD_START_ADDR   0x0200                  
// #define PD_END_ADDR     0x0340                  
// #define PD_SIZE         0x0140                  
  #define PVT_COMP_ID_ADDR         0x0000                          // Access: RO Volatile: False     Description: PVT Controller ID and revision
  #define PVT_IP_CFG_ADDR          0x0004                          // Access: RO Volatile: False     Description: PVT Controller IP configuration
  #define PVT_ID_NUM_ADDR          0x0008                          // Access: RO Volatile: False     Description: PVT Customer defined controller information
  #define PVT_TM_SCRATCH_ADDR      0x000c                          // Access: RW Volatile: False     Description: PVT scratch test register
  #define PVT_REG_LOCK_ADDR        0x0010                          // Access: WO Volatile: False     Description: PVT Soft lock register. Any write to register locks certain controllers registers. Un-locked by writing 0x1ACCE551
  #define PVT_LOCK_STATUS_ADDR     0x0014                          // Access: RO Volatile: True      Description: PVT Lock Status. Reading returns the current lock status.
  #define PVT_TAM_STATUS_ADDR      0x0018                          // Access: RO Volatile: True      Description: PVT Test Access Status. Read of register indicates that the TAM has been active
  #define PVT_TAM_CLEAR_ADDR       0x001c                          // Access: WO Volatile: True      Description: PVT Test Access Clear. Write to this register clears the TAM status register.
  #define PVT_TMR_CTRL_ADDR        0x0020                          // Access: RW Volatile: False     Description: PVT Timer Control register. R/W to set timers behaviour.
  #define PVT_TMR_STATUS_ADDR      0x0024                          // Access: RO Volatile: True      Description: PVT Timer Status register. Read of register returns timer status. If timer is enabled
  #define PVT_TMR_IRQ_CLEAR_ADDR   0x0028                          // Access: WO Volatile: False     Description: PVT Timer IRQ clear register. Write to register clears timer IRQ
  #define PVT_TMR_IRQ_TEST_ADDR    0x002c                          // Access: RW Volatile: False     Description: PVT Timer IRQ test register. Write to register will trigger a timer IRQ. If timer IRQ enabled
  #define IRQ_EN_ADDR           0x0040                       // Access: RW Volatile: False  Description: PVT master IRQ register, enables IRQ from the IP blocks and timer.
  #define IRQ_RES0_ADDR         0x0044                       // Access: RO Volatile: False  Description: Reserved. Returns 0, function may change in future revisions.
  #define IRQ_RES1_ADDR         0x0048                       // Access: RO Volatile: False  Description: Reserved. Returns 0, function may change in future revisions.
  #define IRQ_RES2_ADDR         0x004c                       // Access: RO Volatile: False  Description: Reserved. Returns 0, function may change in future revisions.
  #define IRQ_TR_MASK_ADDR      0x0050                       // Access: RW Volatile: False  Description: PVT master IRQ mask register, allows masking of Timer IRQ. Write 1 to mask IRQ source.
  #define IRQ_TS_MASK_ADDR      0x0054                       // Access: RW Volatile: False  Description: PVT master IRQ mask register, allows masking of TS IRQ. Write 1 to mask IRQ source.
  #define IRQ_VM_MASK_ADDR      0x0058                       // Access: RW Volatile: False  Description: PVT master IRQ mask register, allows masking of VM IRQ. Write 1 to mask IRQ source.
  #define IRQ_PD_MASK_ADDR      0x005c                       // Access: RW Volatile: False  Description: PVT master IRQ mask register, allows masking of PD IRQ. Write 1 to mask IRQ source.
  #define IRQ_TR_STATUS_ADDR    0x0060                       // Access: RO Volatile: True   Description: PVT master IRQ status register. Reading gives Timer IRQ status (after mask, if applied).
  #define IRQ_TS_STATUS_ADDR    0x0064                       // Access: RO Volatile: True   Description: PVT master IRQ status register. Reading gives TS IRQ status (after masking, if applied).
  #define IRQ_VM_STATUS_ADDR    0x0068                       // Access: RO Volatile: True   Description: PVT master IRQ status register. Reading gives VM IRQ status (after masking, if applied).
  #define IRQ_PD_STATUS_ADDR    0x006c                       // Access: RO Volatile: True   Description: PVT master IRQ status register. Reading gives PD IRQ status (after masking, if applied).
  #define IRQ_TR_RAW_ADDR       0x0070                       // Access: RO Volatile: True   Description: PVT master IRQ raw status register. Reading gives Timer IRQ status no masking.
  #define IRQ_TS_RAW_ADDR       0x0074                       // Access: RO Volatile: True   Description: PVT master IRQ raw status register. Reading gives TS IRQ status no masking.
  #define IRQ_VM_RAW_ADDR       0x0078                       // Access: RO Volatile: True   Description: PVT master IRQ raw status register. Reading gives VM IRQ status no masking.
  #define IRQ_PD_RAW_ADDR       0x007c                       // Access: RO Volatile: True   Description: PVT master IRQ raw status register. Reading gives PD IRQ status no masking.
  #define TS_CMN_CLK_SYNTH_ADDR      0x0080                            // Access: RW Volatile: False       Description: TS Clock Synthesiser control register.
  #define TS_CMN_SDIF_DISABLE_ADDR   0x0084                            // Access: RW Volatile: False       Description: TS SDIF disable (Active High). When asserted completely disables the selected TS instance(s), by forcing the TS macro clock and reset low.
  #define TS_CMN_SDIF_STATUS_ADDR    0x0088                            // Access: RO Volatile: True        Description: TS SDIF Status Register.
  #define TS_CMN_SDIF_ADDR           0x008c                            // Access: RW Volatile: False       Description: TS SDIF write data register.
  #define TS_CMN_SDIF_HALT_ADDR      0x0090                            // Access: WO Volatile: False       Description: TS SDIF halt register. Halts all SDIF data transfer and resets SDIF slave.
  #define TS_CMN_SDIF_CTRL_ADDR      0x0094                            // Access: RW Volatile: False       Description: TS SDIF programming inhibit (Active High). When asserted inhibits serial programming of the selected TS instance(s).
  #define TS_CMN_SMPL_CTRL_ADDR      0x00a0                            // Access: RW Volatile: False       Description: TS SDIF sample counter control.
  #define TS_CMN_SMPL_CLR_ADDR       0x00a4                            // Access: WO Volatile: False       Description: TS SDIF sample counter clear.
  #define TS_CMN_SMPL_CNT_ADDR       0x00a8                            // Access: RO Volatile: True        Description: TS SDIF sample counter current value.
  #define TS_00_IRQ_ENABLE_ADDR      0x00c0                            // Access: RW Volatile: False       Description: TS-00 IRQ enable register, enables individual IRQ sources from the TS
  #define TS_00_IRQ_STATUS_ADDR      0x00c4                            // Access: RO Volatile: True        Description: TS-00 IRQ status register, reports status of individual IRQ sources from the TS
  #define TS_00_IRQ_CLEAR_ADDR       0x00c8                            // Access: WO Volatile: False       Description: TS-00 IRQ clear register, clears individual IRQ sources from the TS
  #define TS_00_IRQ_TEST_ADDR        0x00cc                            // Access: RW Volatile: False       Description: TS-00 IRQ test register. Write to register will trigger an TS IRQ. If the IRQ enabled
  #define TS_00_SDIF_RDATA_ADDR      0x00d0                            // Access: RO Volatile: True        Description: TS-00 read data register.
  #define TS_00_SDIF_DONE_ADDR       0x00d4                            // Access: RO Volatile: True        Description: TS-00 SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define TS_00_SDIF_DATA_ADDR       0x00d8                            // Access: RO Volatile: True        Description: TS-00 SDIF sample data register.
  #define TS_00_RES0_ADDR            0x00dc                            // Access: RO Volatile: False       Description: Reserved. Returns 0, function may change in future revisions.
  #define TS_00_ALARMA_CFG_ADDR      0x00e0                            // Access: RW Volatile: False       Description: TS-00 Alarm A configuration.
  #define TS_00_ALARMB_CFG_ADDR      0x00e4                            // Access: RW Volatile: False       Description: TS-00 Alarm B configuration.
  #define TS_00_SMPL_HILO_ADDR       0x00e8                            // Access: RO Volatile: True        Description: TS-00 Sample max/min high/low value.
  #define TS_00_HILO_RESET_ADDR      0x00ec                            // Access: WO Volatile: False       Description: TS-00 Reset sample high/low register.
  #define TS_01_IRQ_ENABLE_ADDR      0x0100                            // Access: RW Volatile: False       Description: TS-01 IRQ enable register, enables individual IRQ sources from the TS
  #define TS_01_IRQ_STATUS_ADDR      0x0104                            // Access: RO Volatile: True        Description: TS-01 IRQ status register, reports status of individual IRQ sources from the TS
  #define TS_01_IRQ_CLEAR_ADDR       0x0108                            // Access: WO Volatile: False       Description: TS-01 IRQ clear register, clears individual IRQ sources from the TS
  #define TS_01_IRQ_TEST_ADDR        0x010c                            // Access: RW Volatile: False       Description: TS-01 IRQ test register. Write to register will trigger an TS IRQ. If the IRQ enabled
  #define TS_01_SDIF_RDATA_ADDR      0x0110                            // Access: RO Volatile: True        Description: TS-01 read data register.
  #define TS_01_SDIF_DONE_ADDR       0x0114                            // Access: RO Volatile: True        Description: TS-01 SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define TS_01_SDIF_DATA_ADDR       0x0118                            // Access: RO Volatile: True        Description: TS-01 SDIF sample data register.
  #define TS_01_RES0_ADDR            0x011c                            // Access: RO Volatile: False       Description: Reserved. Returns 0, function may change in future revisions.
  #define TS_01_ALARMA_CFG_ADDR      0x0120                            // Access: RW Volatile: False       Description: TS-01 Alarm A configuration.
  #define TS_01_ALARMB_CFG_ADDR      0x0124                            // Access: RW Volatile: False       Description: TS-01 Alarm B configuration.
  #define TS_01_SMPL_HILO_ADDR       0x0128                            // Access: RO Volatile: True        Description: TS-01 Sample max/min high/low value.
  #define TS_01_HILO_RESET_ADDR      0x012c                            // Access: WO Volatile: False       Description: TS-01 Reset sample high/low register.
  #define TS_02_IRQ_ENABLE_ADDR      0x0140                            // Access: RW Volatile: False       Description: TS-02 IRQ enable register, enables individual IRQ sources from the TS
  #define TS_02_IRQ_STATUS_ADDR      0x0144                            // Access: RO Volatile: True        Description: TS-02 IRQ status register, reports status of individual IRQ sources from the TS
  #define TS_02_IRQ_CLEAR_ADDR       0x0148                            // Access: WO Volatile: False       Description: TS-02 IRQ clear register, clears individual IRQ sources from the TS
  #define TS_02_IRQ_TEST_ADDR        0x014c                            // Access: RW Volatile: False       Description: TS-02 IRQ test register. Write to register will trigger an TS IRQ. If the IRQ enabled
  #define TS_02_SDIF_RDATA_ADDR      0x0150                            // Access: RO Volatile: True        Description: TS-02 read data register.
  #define TS_02_SDIF_DONE_ADDR       0x0154                            // Access: RO Volatile: True        Description: TS-02 SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define TS_02_SDIF_DATA_ADDR       0x0158                            // Access: RO Volatile: True        Description: TS-02 SDIF sample data register.
  #define TS_02_RES0_ADDR            0x015c                            // Access: RO Volatile: False       Description: Reserved. Returns 0, function may change in future revisions.
  #define TS_02_ALARMA_CFG_ADDR      0x0160                            // Access: RW Volatile: False       Description: TS-02 Alarm A configuration.
  #define TS_02_ALARMB_CFG_ADDR      0x0164                            // Access: RW Volatile: False       Description: TS-02 Alarm B configuration.
  #define TS_02_SMPL_HILO_ADDR       0x0168                            // Access: RO Volatile: True        Description: TS-02 Sample max/min high/low value.
  #define TS_02_HILO_RESET_ADDR      0x016c                            // Access: WO Volatile: False       Description: TS-02 Reset sample high/low register.
  #define TS_03_IRQ_ENABLE_ADDR      0x0180                            // Access: RW Volatile: False       Description: TS-03 IRQ enable register, enables individual IRQ sources from the TS
  #define TS_03_IRQ_STATUS_ADDR      0x0184                            // Access: RO Volatile: True        Description: TS-03 IRQ status register, reports status of individual IRQ sources from the TS
  #define TS_03_IRQ_CLEAR_ADDR       0x0188                            // Access: WO Volatile: False       Description: TS-03 IRQ clear register, clears individual IRQ sources from the TS
  #define TS_03_IRQ_TEST_ADDR        0x018c                            // Access: RW Volatile: False       Description: TS-03 IRQ test register. Write to register will trigger an TS IRQ. If the IRQ enabled
  #define TS_03_SDIF_RDATA_ADDR      0x0190                            // Access: RO Volatile: True        Description: TS-03 read data register.
  #define TS_03_SDIF_DONE_ADDR       0x0194                            // Access: RO Volatile: True        Description: TS-03 SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define TS_03_SDIF_DATA_ADDR       0x0198                            // Access: RO Volatile: True        Description: TS-03 SDIF sample data register.
  #define TS_03_RES0_ADDR            0x019c                            // Access: RO Volatile: False       Description: Reserved. Returns 0, function may change in future revisions.
  #define TS_03_ALARMA_CFG_ADDR      0x01a0                            // Access: RW Volatile: False       Description: TS-03 Alarm A configuration.
  #define TS_03_ALARMB_CFG_ADDR      0x01a4                            // Access: RW Volatile: False       Description: TS-03 Alarm B configuration.
  #define TS_03_SMPL_HILO_ADDR       0x01a8                            // Access: RO Volatile: True        Description: TS-03 Sample max/min high/low value.
  #define TS_03_HILO_RESET_ADDR      0x01ac                            // Access: WO Volatile: False       Description: TS-03 Reset sample high/low register.
  #define PD_CMN_CLK_SYNTH_ADDR       0x0300                            // Access: RW Volatile: False       Description: PD Clock Synthesiser control register.
  #define PD_CMN_SDIF_DISABLE_ADDR    0x0304                            // Access: RW Volatile: False       Description: PD SDIF disable (Active High). When asserted completely disables the selected PD instance(s), by forcing the PD macro clock and reset low.
  #define PD_CMN_SDIF_STATUS_ADDR     0x0308                            // Access: RO Volatile: True        Description: PD SDIF Status Register.
  #define PD_CMN_SDIF_ADDR            0x030c                            // Access: RW Volatile: False       Description: PD SDIF write data register.
  #define PD_CMN_SDIF_HALT_ADDR       0x0310                            // Access: WO Volatile: False       Description: PD SDIF halt register. Halts all SDIF data transfer and resets SDIF slave.
  #define PD_CMN_SDIF_CTRL_ADDR       0x0314                            // Access: RW Volatile: False       Description: PD SDIF programming inhibit (Active High). When asserted inhibits serial programming of the selected PD instance(s).
  #define PD_CMN_SMPL_CTRL_ADDR       0x0320                            // Access: RW Volatile: False       Description: PD SDIF sample counter control.
  #define PD_CMN_SMPL_CLR_ADDR        0x0324                            // Access: WO Volatile: False       Description: PD SDIF sample counter clear.
  #define PD_CMN_SMPL_CNT_ADDR        0x0328                            // Access: RO Volatile: True        Description: PD SDIF sample counter current value.
  #define PD_00_IRQ_ENABLE_ADDR       0x0340                            // Access: RW Volatile: False       Description: PD-00 IRQ enable register, enables individual IRQ sources from the PD
  #define PD_00_IRQ_STATUS_ADDR       0x0344                            // Access: RO Volatile: True        Description: PD-00 IRQ status register, reports status of individual IRQ sources from the PD
  #define PD_00_IRQ_CLEAR_ADDR        0x0348                            // Access: WO Volatile: False       Description: PD-00 IRQ clear register, clears individual IRQ sources from the PD
  #define PD_00_IRQ_TEST_ADDR         0x034c                            // Access: RW Volatile: False       Description: PD-00 IRQ test register. Write to register will trigger an PD IRQ. If the IRQ enabled
  #define PD_00_SDIF_RDATA_ADDR       0x0350                            // Access: RO Volatile: True        Description: PD-00 read data register.
  #define PD_00_SDIF_DONE_ADDR        0x0354                            // Access: RO Volatile: True        Description: PD-00 SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define PD_00_SDIF_DATA_ADDR        0x0358                            // Access: RO Volatile: True        Description: PD-00 SDIF sample data register.
  #define PD_00_RES0_ADDR             0x035c                            // Access: RO Volatile: False       Description: Reserved. Returns 0, function may change in future revisions.
  #define PD_00_ALARMA_CFG_ADDR       0x0360                            // Access: RW Volatile: False       Description: PD-00 Alarm A configuration.
  #define PD_00_ALARMB_CFG_ADDR       0x0364                            // Access: RW Volatile: False       Description: PD-00 Alarm B configuration.
  #define PD_00_SMPL_HILO_ADDR        0x0368                            // Access: RO Volatile: True        Description: PD-00 Sample max/min high/low value.
  #define PD_00_HILO_RESET_ADDR       0x036c                            // Access: WO Volatile: False       Description: PD-00 Reset sample high/low register.
  #define PD_01_IRQ_ENABLE_ADDR       0x0380                            // Access: RW Volatile: False       Description: PD-01 IRQ enable register, enables individual IRQ sources from the PD
  #define PD_01_IRQ_STATUS_ADDR       0x0384                            // Access: RO Volatile: True        Description: PD-01 IRQ status register, reports status of individual IRQ sources from the PD
  #define PD_01_IRQ_CLEAR_ADDR        0x0388                            // Access: WO Volatile: False       Description: PD-01 IRQ clear register, clears individual IRQ sources from the PD
  #define PD_01_IRQ_TEST_ADDR         0x038c                            // Access: RW Volatile: False       Description: PD-01 IRQ test register. Write to register will trigger an PD IRQ. If the IRQ enabled
  #define PD_01_SDIF_RDATA_ADDR       0x0390                            // Access: RO Volatile: True        Description: PD-01 read data register.
  #define PD_01_SDIF_DONE_ADDR        0x0394                            // Access: RO Volatile: True        Description: PD-01 SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define PD_01_SDIF_DATA_ADDR        0x0398                            // Access: RO Volatile: True        Description: PD-01 SDIF sample data register.
  #define PD_01_RES0_ADDR             0x039c                            // Access: RO Volatile: False       Description: Reserved. Returns 0, function may change in future revisions.
  #define PD_01_ALARMA_CFG_ADDR       0x03a0                            // Access: RW Volatile: False       Description: PD-01 Alarm A configuration.
  #define PD_01_ALARMB_CFG_ADDR       0x03a4                            // Access: RW Volatile: False       Description: PD-01 Alarm B configuration.
  #define PD_01_SMPL_HILO_ADDR        0x03a8                            // Access: RO Volatile: True        Description: PD-01 Sample max/min high/low value.
  #define PD_01_HILO_RESET_ADDR       0x03ac                            // Access: WO Volatile: False       Description: PD-01 Reset sample high/low register.
  #define PD_02_IRQ_ENABLE_ADDR       0x03c0                            // Access: RW Volatile: False       Description: PD-02 IRQ enable register, enables individual IRQ sources from the PD
  #define PD_02_IRQ_STATUS_ADDR       0x03c4                            // Access: RO Volatile: True        Description: PD-02 IRQ status register, reports status of individual IRQ sources from the PD
  #define PD_02_IRQ_CLEAR_ADDR        0x03c8                            // Access: WO Volatile: False       Description: PD-02 IRQ clear register, clears individual IRQ sources from the PD
  #define PD_02_IRQ_TEST_ADDR         0x03cc                            // Access: RW Volatile: False       Description: PD-02 IRQ test register. Write to register will trigger an PD IRQ. If the IRQ enabled
  #define PD_02_SDIF_RDATA_ADDR       0x03d0                            // Access: RO Volatile: True        Description: PD-02 read data register.
  #define PD_02_SDIF_DONE_ADDR        0x03d4                            // Access: RO Volatile: True        Description: PD-02 SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define PD_02_SDIF_DATA_ADDR        0x03d8                            // Access: RO Volatile: True        Description: PD-02 SDIF sample data register.
  #define PD_02_RES0_ADDR             0x03dc                            // Access: RO Volatile: False       Description: Reserved. Returns 0, function may change in future revisions.
  #define PD_02_ALARMA_CFG_ADDR       0x03e0                            // Access: RW Volatile: False       Description: PD-02 Alarm A configuration.
  #define PD_02_ALARMB_CFG_ADDR       0x03e4                            // Access: RW Volatile: False       Description: PD-02 Alarm B configuration.
  #define PD_02_SMPL_HILO_ADDR        0x03e8                            // Access: RO Volatile: True        Description: PD-02 Sample max/min high/low value.
  #define PD_02_HILO_RESET_ADDR       0x03ec                            // Access: WO Volatile: False       Description: PD-02 Reset sample high/low register.
  #define PD_03_IRQ_ENABLE_ADDR       0x0400                            // Access: RW Volatile: False       Description: PD-03 IRQ enable register, enables individual IRQ sources from the PD
  #define PD_03_IRQ_STATUS_ADDR       0x0404                            // Access: RO Volatile: True        Description: PD-03 IRQ status register, reports status of individual IRQ sources from the PD
  #define PD_03_IRQ_CLEAR_ADDR        0x0408                            // Access: WO Volatile: False       Description: PD-03 IRQ clear register, clears individual IRQ sources from the PD
  #define PD_03_IRQ_TEST_ADDR         0x040c                            // Access: RW Volatile: False       Description: PD-03 IRQ test register. Write to register will trigger an PD IRQ. If the IRQ enabled
  #define PD_03_SDIF_RDATA_ADDR       0x0410                            // Access: RO Volatile: True        Description: PD-03 read data register.
  #define PD_03_SDIF_DONE_ADDR        0x0414                            // Access: RO Volatile: True        Description: PD-03 SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define PD_03_SDIF_DATA_ADDR        0x0418                            // Access: RO Volatile: True        Description: PD-03 SDIF sample data register.
  #define PD_03_RES0_ADDR             0x041c                            // Access: RO Volatile: False       Description: Reserved. Returns 0, function may change in future revisions.
  #define PD_03_ALARMA_CFG_ADDR       0x0420                            // Access: RW Volatile: False       Description: PD-03 Alarm A configuration.
  #define PD_03_ALARMB_CFG_ADDR       0x0424                            // Access: RW Volatile: False       Description: PD-03 Alarm B configuration.
  #define PD_03_SMPL_HILO_ADDR        0x0428                            // Access: RO Volatile: True        Description: PD-03 Sample max/min high/low value.
  #define PD_03_HILO_RESET_ADDR       0x042c                            // Access: WO Volatile: False       Description: PD-03 Reset sample high/low register.
