// TS   8 PD   8 VM   2 CH   16
// #define PVT_START_ADDR    0x0000                   
// #define PVT_END_ADDR      0x0040                   
// #define PVT_SIZE          0x0040                   
// #define IRQ_START_ADDR    0x0040                   
// #define IRQ_END_ADDR      0x0080                   
// #define IRQ_SIZE          0x0040                   
// #define TS_START_ADDR    0x0080                  
// #define TS_END_ADDR      0x02c0                  
// #define TS_SIZE          0x0240                  
// #define PD_START_ADDR    0x0300                  
// #define PD_END_ADDR      0x0540                  
// #define PD_SIZE          0x0240                  
// #define VM_START_ADDR    0x0800                  
// #define VM_END_ADDR      0x1000                  
// #define VM_SIZE          0x0800                  
  #define PVT_COMP_ID_ADDR          0x0000                          // Access: RO Volatile: False     Description: PVT Controller ID and revision
  #define PVT_IP_CFG_ADDR           0x0004                          // Access: RO Volatile: False     Description: PVT Controller IP configuration
  #define PVT_ID_NUM_ADDR           0x0008                          // Access: RO Volatile: False     Description: PVT Customer defined controller information
  #define PVT_TM_SCRATCH_ADDR       0x000c                          // Access: RW Volatile: False     Description: PVT scratch test register
  #define PVT_REG_LOCK_ADDR         0x0010                          // Access: WO Volatile: False     Description: PVT Soft lock register. Any write to register locks certain controllers registers. Un-locked by writing 0x1ACCE551
  #define PVT_LOCK_STATUS_ADDR      0x0014                          // Access: RO Volatile: True      Description: PVT Lock Status. Reading returns the current lock status.
  #define PVT_TAM_STATUS_ADDR       0x0018                          // Access: RO Volatile: True      Description: PVT Test Access Status. Read of register indicates that the TAM has been active
  #define PVT_TAM_CLEAR_ADDR        0x001c                          // Access: WO Volatile: True      Description: PVT Test Access Clear. Write to this register clears the TAM status register.
  #define PVT_TMR_CTRL_ADDR         0x0020                          // Access: RW Volatile: False     Description: PVT Timer Control register. R/W to set timers behaviour.
  #define PVT_TMR_STATUS_ADDR       0x0024                          // Access: RO Volatile: True      Description: PVT Timer Status register. Read of register returns timer status. If timer is enabled
  #define PVT_TMR_IRQ_CLEAR_ADDR    0x0028                          // Access: WO Volatile: False     Description: PVT Timer IRQ clear register. Write to register clears timer IRQ
  #define PVT_TMR_IRQ_TEST_ADDR     0x002c                          // Access: RW Volatile: False     Description: PVT Timer IRQ test register. Write to register will trigger a timer IRQ. If timer IRQ enabled
  #define IRQ_EN_ADDR            0x0040                       // Access: RW Volatile: False  Description: PVT master IRQ register, enables IRQ from the IP blocks and timer.
  #define IRQ_RES0_ADDR          0x0044                       // Access: RO Volatile: False  Description: Reserved. Returns 0, function may change in future revisions.
  #define IRQ_RES1_ADDR          0x0048                       // Access: RO Volatile: False  Description: Reserved. Returns 0, function may change in future revisions.
  #define IRQ_RES2_ADDR          0x004c                       // Access: RO Volatile: False  Description: Reserved. Returns 0, function may change in future revisions.
  #define IRQ_TR_MASK_ADDR       0x0050                       // Access: RW Volatile: False  Description: PVT master IRQ mask register, allows masking of Timer IRQ. Write 1 to mask IRQ source.
  #define IRQ_TS_MASK_ADDR       0x0054                       // Access: RW Volatile: False  Description: PVT master IRQ mask register, allows masking of TS IRQ. Write 1 to mask IRQ source.
  #define IRQ_VM_MASK_ADDR       0x0058                       // Access: RW Volatile: False  Description: PVT master IRQ mask register, allows masking of VM IRQ. Write 1 to mask IRQ source.
  #define IRQ_PD_MASK_ADDR       0x005c                       // Access: RW Volatile: False  Description: PVT master IRQ mask register, allows masking of PD IRQ. Write 1 to mask IRQ source.
  #define IRQ_TR_STATUS_ADDR     0x0060                       // Access: RO Volatile: True   Description: PVT master IRQ status register. Reading gives Timer IRQ status (after mask, if applied).
  #define IRQ_TS_STATUS_ADDR     0x0064                       // Access: RO Volatile: True   Description: PVT master IRQ status register. Reading gives TS IRQ status (after masking, if applied).
  #define IRQ_VM_STATUS_ADDR     0x0068                       // Access: RO Volatile: True   Description: PVT master IRQ status register. Reading gives VM IRQ status (after masking, if applied).
  #define IRQ_PD_STATUS_ADDR     0x006c                       // Access: RO Volatile: True   Description: PVT master IRQ status register. Reading gives PD IRQ status (after masking, if applied).
  #define IRQ_TR_RAW_ADDR        0x0070                       // Access: RO Volatile: True   Description: PVT master IRQ raw status register. Reading gives Timer IRQ status no masking.
  #define IRQ_TS_RAW_ADDR        0x0074                       // Access: RO Volatile: True   Description: PVT master IRQ raw status register. Reading gives TS IRQ status no masking.
  #define IRQ_VM_RAW_ADDR        0x0078                       // Access: RO Volatile: True   Description: PVT master IRQ raw status register. Reading gives VM IRQ status no masking.
  #define IRQ_PD_RAW_ADDR        0x007c                       // Access: RO Volatile: True   Description: PVT master IRQ raw status register. Reading gives PD IRQ status no masking.
  #define TS_CMN_CLK_SYNTH_ADDR       0x0080                            // Access: RW Volatile: False       Description: TS Clock Synthesiser control register.
  #define TS_CMN_SDIF_DISABLE_ADDR    0x0084                            // Access: RW Volatile: False       Description: TS SDIF disable (Active High). When asserted completely disables the selected TS instance(s), by forcing the TS macro clock and reset low.
  #define TS_CMN_SDIF_STATUS_ADDR     0x0088                            // Access: RO Volatile: True        Description: TS SDIF Status Register.
  #define TS_CMN_SDIF_ADDR            0x008c                            // Access: RW Volatile: False       Description: TS SDIF write data register.
  #define TS_CMN_SDIF_HALT_ADDR       0x0090                            // Access: WO Volatile: False       Description: TS SDIF halt register. Halts all SDIF data transfer and resets SDIF slave.
  #define TS_CMN_SDIF_CTRL_ADDR       0x0094                            // Access: RW Volatile: False       Description: TS SDIF programming inhibit (Active High). When asserted inhibits serial programming of the selected TS instance(s).
  #define TS_CMN_SMPL_CTRL_ADDR       0x00a0                            // Access: RW Volatile: False       Description: TS SDIF sample counter control.
  #define TS_CMN_SMPL_CLR_ADDR        0x00a4                            // Access: WO Volatile: False       Description: TS SDIF sample counter clear.
  #define TS_CMN_SMPL_CNT_ADDR        0x00a8                            // Access: RO Volatile: True        Description: TS SDIF sample counter current value.
  #define TS_00_IRQ_ENABLE_ADDR       0x00c0                            // Access: RW Volatile: False       Description: TS-00 IRQ enable register, enables individual IRQ sources from the TS
  #define TS_00_IRQ_STATUS_ADDR       0x00c4                            // Access: RO Volatile: True        Description: TS-00 IRQ status register, reports status of individual IRQ sources from the TS
  #define TS_00_IRQ_CLEAR_ADDR        0x00c8                            // Access: WO Volatile: False       Description: TS-00 IRQ clear register, clears individual IRQ sources from the TS
  #define TS_00_IRQ_TEST_ADDR         0x00cc                            // Access: RW Volatile: False       Description: TS-00 IRQ test register. Write to register will trigger an TS IRQ. If the IRQ enabled
  #define TS_00_SDIF_RDATA_ADDR       0x00d0                            // Access: RO Volatile: True        Description: TS-00 read data register.
  #define TS_00_SDIF_DONE_ADDR        0x00d4                            // Access: RO Volatile: True        Description: TS-00 SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define TS_00_SDIF_DATA_ADDR        0x00d8                            // Access: RO Volatile: True        Description: TS-00 SDIF sample data register.
  #define TS_00_RES0_ADDR             0x00dc                            // Access: RO Volatile: False       Description: Reserved. Returns 0, function may change in future revisions.
  #define TS_00_ALARMA_CFG_ADDR       0x00e0                            // Access: RW Volatile: False       Description: TS-00 Alarm A configuration.
  #define TS_00_ALARMB_CFG_ADDR       0x00e4                            // Access: RW Volatile: False       Description: TS-00 Alarm B configuration.
  #define TS_00_SMPL_HILO_ADDR        0x00e8                            // Access: RO Volatile: True        Description: TS-00 Sample max/min high/low value.
  #define TS_00_HILO_RESET_ADDR       0x00ec                            // Access: WO Volatile: False       Description: TS-00 Reset sample high/low register.
  #define TS_01_IRQ_ENABLE_ADDR       0x0100                            // Access: RW Volatile: False       Description: TS-01 IRQ enable register, enables individual IRQ sources from the TS
  #define TS_01_IRQ_STATUS_ADDR       0x0104                            // Access: RO Volatile: True        Description: TS-01 IRQ status register, reports status of individual IRQ sources from the TS
  #define TS_01_IRQ_CLEAR_ADDR        0x0108                            // Access: WO Volatile: False       Description: TS-01 IRQ clear register, clears individual IRQ sources from the TS
  #define TS_01_IRQ_TEST_ADDR         0x010c                            // Access: RW Volatile: False       Description: TS-01 IRQ test register. Write to register will trigger an TS IRQ. If the IRQ enabled
  #define TS_01_SDIF_RDATA_ADDR       0x0110                            // Access: RO Volatile: True        Description: TS-01 read data register.
  #define TS_01_SDIF_DONE_ADDR        0x0114                            // Access: RO Volatile: True        Description: TS-01 SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define TS_01_SDIF_DATA_ADDR        0x0118                            // Access: RO Volatile: True        Description: TS-01 SDIF sample data register.
  #define TS_01_RES0_ADDR             0x011c                            // Access: RO Volatile: False       Description: Reserved. Returns 0, function may change in future revisions.
  #define TS_01_ALARMA_CFG_ADDR       0x0120                            // Access: RW Volatile: False       Description: TS-01 Alarm A configuration.
  #define TS_01_ALARMB_CFG_ADDR       0x0124                            // Access: RW Volatile: False       Description: TS-01 Alarm B configuration.
  #define TS_01_SMPL_HILO_ADDR        0x0128                            // Access: RO Volatile: True        Description: TS-01 Sample max/min high/low value.
  #define TS_01_HILO_RESET_ADDR       0x012c                            // Access: WO Volatile: False       Description: TS-01 Reset sample high/low register.
  #define TS_02_IRQ_ENABLE_ADDR       0x0140                            // Access: RW Volatile: False       Description: TS-02 IRQ enable register, enables individual IRQ sources from the TS
  #define TS_02_IRQ_STATUS_ADDR       0x0144                            // Access: RO Volatile: True        Description: TS-02 IRQ status register, reports status of individual IRQ sources from the TS
  #define TS_02_IRQ_CLEAR_ADDR        0x0148                            // Access: WO Volatile: False       Description: TS-02 IRQ clear register, clears individual IRQ sources from the TS
  #define TS_02_IRQ_TEST_ADDR         0x014c                            // Access: RW Volatile: False       Description: TS-02 IRQ test register. Write to register will trigger an TS IRQ. If the IRQ enabled
  #define TS_02_SDIF_RDATA_ADDR       0x0150                            // Access: RO Volatile: True        Description: TS-02 read data register.
  #define TS_02_SDIF_DONE_ADDR        0x0154                            // Access: RO Volatile: True        Description: TS-02 SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define TS_02_SDIF_DATA_ADDR        0x0158                            // Access: RO Volatile: True        Description: TS-02 SDIF sample data register.
  #define TS_02_RES0_ADDR             0x015c                            // Access: RO Volatile: False       Description: Reserved. Returns 0, function may change in future revisions.
  #define TS_02_ALARMA_CFG_ADDR       0x0160                            // Access: RW Volatile: False       Description: TS-02 Alarm A configuration.
  #define TS_02_ALARMB_CFG_ADDR       0x0164                            // Access: RW Volatile: False       Description: TS-02 Alarm B configuration.
  #define TS_02_SMPL_HILO_ADDR        0x0168                            // Access: RO Volatile: True        Description: TS-02 Sample max/min high/low value.
  #define TS_02_HILO_RESET_ADDR       0x016c                            // Access: WO Volatile: False       Description: TS-02 Reset sample high/low register.
  #define TS_03_IRQ_ENABLE_ADDR       0x0180                            // Access: RW Volatile: False       Description: TS-03 IRQ enable register, enables individual IRQ sources from the TS
  #define TS_03_IRQ_STATUS_ADDR       0x0184                            // Access: RO Volatile: True        Description: TS-03 IRQ status register, reports status of individual IRQ sources from the TS
  #define TS_03_IRQ_CLEAR_ADDR        0x0188                            // Access: WO Volatile: False       Description: TS-03 IRQ clear register, clears individual IRQ sources from the TS
  #define TS_03_IRQ_TEST_ADDR         0x018c                            // Access: RW Volatile: False       Description: TS-03 IRQ test register. Write to register will trigger an TS IRQ. If the IRQ enabled
  #define TS_03_SDIF_RDATA_ADDR       0x0190                            // Access: RO Volatile: True        Description: TS-03 read data register.
  #define TS_03_SDIF_DONE_ADDR        0x0194                            // Access: RO Volatile: True        Description: TS-03 SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define TS_03_SDIF_DATA_ADDR        0x0198                            // Access: RO Volatile: True        Description: TS-03 SDIF sample data register.
  #define TS_03_RES0_ADDR             0x019c                            // Access: RO Volatile: False       Description: Reserved. Returns 0, function may change in future revisions.
  #define TS_03_ALARMA_CFG_ADDR       0x01a0                            // Access: RW Volatile: False       Description: TS-03 Alarm A configuration.
  #define TS_03_ALARMB_CFG_ADDR       0x01a4                            // Access: RW Volatile: False       Description: TS-03 Alarm B configuration.
  #define TS_03_SMPL_HILO_ADDR        0x01a8                            // Access: RO Volatile: True        Description: TS-03 Sample max/min high/low value.
  #define TS_03_HILO_RESET_ADDR       0x01ac                            // Access: WO Volatile: False       Description: TS-03 Reset sample high/low register.
  #define TS_04_IRQ_ENABLE_ADDR       0x01c0                            // Access: RW Volatile: False       Description: TS-04 IRQ enable register, enables individual IRQ sources from the TS
  #define TS_04_IRQ_STATUS_ADDR       0x01c4                            // Access: RO Volatile: True        Description: TS-04 IRQ status register, reports status of individual IRQ sources from the TS
  #define TS_04_IRQ_CLEAR_ADDR        0x01c8                            // Access: WO Volatile: False       Description: TS-04 IRQ clear register, clears individual IRQ sources from the TS
  #define TS_04_IRQ_TEST_ADDR         0x01cc                            // Access: RW Volatile: False       Description: TS-04 IRQ test register. Write to register will trigger an TS IRQ. If the IRQ enabled
  #define TS_04_SDIF_RDATA_ADDR       0x01d0                            // Access: RO Volatile: True        Description: TS-04 read data register.
  #define TS_04_SDIF_DONE_ADDR        0x01d4                            // Access: RO Volatile: True        Description: TS-04 SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define TS_04_SDIF_DATA_ADDR        0x01d8                            // Access: RO Volatile: True        Description: TS-04 SDIF sample data register.
  #define TS_04_RES0_ADDR             0x01dc                            // Access: RO Volatile: False       Description: Reserved. Returns 0, function may change in future revisions.
  #define TS_04_ALARMA_CFG_ADDR       0x01e0                            // Access: RW Volatile: False       Description: TS-04 Alarm A configuration.
  #define TS_04_ALARMB_CFG_ADDR       0x01e4                            // Access: RW Volatile: False       Description: TS-04 Alarm B configuration.
  #define TS_04_SMPL_HILO_ADDR        0x01e8                            // Access: RO Volatile: True        Description: TS-04 Sample max/min high/low value.
  #define TS_04_HILO_RESET_ADDR       0x01ec                            // Access: WO Volatile: False       Description: TS-04 Reset sample high/low register.
  #define TS_05_IRQ_ENABLE_ADDR       0x0200                            // Access: RW Volatile: False       Description: TS-05 IRQ enable register, enables individual IRQ sources from the TS
  #define TS_05_IRQ_STATUS_ADDR       0x0204                            // Access: RO Volatile: True        Description: TS-05 IRQ status register, reports status of individual IRQ sources from the TS
  #define TS_05_IRQ_CLEAR_ADDR        0x0208                            // Access: WO Volatile: False       Description: TS-05 IRQ clear register, clears individual IRQ sources from the TS
  #define TS_05_IRQ_TEST_ADDR         0x020c                            // Access: RW Volatile: False       Description: TS-05 IRQ test register. Write to register will trigger an TS IRQ. If the IRQ enabled
  #define TS_05_SDIF_RDATA_ADDR       0x0210                            // Access: RO Volatile: True        Description: TS-05 read data register.
  #define TS_05_SDIF_DONE_ADDR        0x0214                            // Access: RO Volatile: True        Description: TS-05 SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define TS_05_SDIF_DATA_ADDR        0x0218                            // Access: RO Volatile: True        Description: TS-05 SDIF sample data register.
  #define TS_05_RES0_ADDR             0x021c                            // Access: RO Volatile: False       Description: Reserved. Returns 0, function may change in future revisions.
  #define TS_05_ALARMA_CFG_ADDR       0x0220                            // Access: RW Volatile: False       Description: TS-05 Alarm A configuration.
  #define TS_05_ALARMB_CFG_ADDR       0x0224                            // Access: RW Volatile: False       Description: TS-05 Alarm B configuration.
  #define TS_05_SMPL_HILO_ADDR        0x0228                            // Access: RO Volatile: True        Description: TS-05 Sample max/min high/low value.
  #define TS_05_HILO_RESET_ADDR       0x022c                            // Access: WO Volatile: False       Description: TS-05 Reset sample high/low register.
  #define TS_06_IRQ_ENABLE_ADDR       0x0240                            // Access: RW Volatile: False       Description: TS-06 IRQ enable register, enables individual IRQ sources from the TS
  #define TS_06_IRQ_STATUS_ADDR       0x0244                            // Access: RO Volatile: True        Description: TS-06 IRQ status register, reports status of individual IRQ sources from the TS
  #define TS_06_IRQ_CLEAR_ADDR        0x0248                            // Access: WO Volatile: False       Description: TS-06 IRQ clear register, clears individual IRQ sources from the TS
  #define TS_06_IRQ_TEST_ADDR         0x024c                            // Access: RW Volatile: False       Description: TS-06 IRQ test register. Write to register will trigger an TS IRQ. If the IRQ enabled
  #define TS_06_SDIF_RDATA_ADDR       0x0250                            // Access: RO Volatile: True        Description: TS-06 read data register.
  #define TS_06_SDIF_DONE_ADDR        0x0254                            // Access: RO Volatile: True        Description: TS-06 SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define TS_06_SDIF_DATA_ADDR        0x0258                            // Access: RO Volatile: True        Description: TS-06 SDIF sample data register.
  #define TS_06_RES0_ADDR             0x025c                            // Access: RO Volatile: False       Description: Reserved. Returns 0, function may change in future revisions.
  #define TS_06_ALARMA_CFG_ADDR       0x0260                            // Access: RW Volatile: False       Description: TS-06 Alarm A configuration.
  #define TS_06_ALARMB_CFG_ADDR       0x0264                            // Access: RW Volatile: False       Description: TS-06 Alarm B configuration.
  #define TS_06_SMPL_HILO_ADDR        0x0268                            // Access: RO Volatile: True        Description: TS-06 Sample max/min high/low value.
  #define TS_06_HILO_RESET_ADDR       0x026c                            // Access: WO Volatile: False       Description: TS-06 Reset sample high/low register.
  #define TS_07_IRQ_ENABLE_ADDR       0x0280                            // Access: RW Volatile: False       Description: TS-07 IRQ enable register, enables individual IRQ sources from the TS
  #define TS_07_IRQ_STATUS_ADDR       0x0284                            // Access: RO Volatile: True        Description: TS-07 IRQ status register, reports status of individual IRQ sources from the TS
  #define TS_07_IRQ_CLEAR_ADDR        0x0288                            // Access: WO Volatile: False       Description: TS-07 IRQ clear register, clears individual IRQ sources from the TS
  #define TS_07_IRQ_TEST_ADDR         0x028c                            // Access: RW Volatile: False       Description: TS-07 IRQ test register. Write to register will trigger an TS IRQ. If the IRQ enabled
  #define TS_07_SDIF_RDATA_ADDR       0x0290                            // Access: RO Volatile: True        Description: TS-07 read data register.
  #define TS_07_SDIF_DONE_ADDR        0x0294                            // Access: RO Volatile: True        Description: TS-07 SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define TS_07_SDIF_DATA_ADDR        0x0298                            // Access: RO Volatile: True        Description: TS-07 SDIF sample data register.
  #define TS_07_RES0_ADDR             0x029c                            // Access: RO Volatile: False       Description: Reserved. Returns 0, function may change in future revisions.
  #define TS_07_ALARMA_CFG_ADDR       0x02a0                            // Access: RW Volatile: False       Description: TS-07 Alarm A configuration.
  #define TS_07_ALARMB_CFG_ADDR       0x02a4                            // Access: RW Volatile: False       Description: TS-07 Alarm B configuration.
  #define TS_07_SMPL_HILO_ADDR        0x02a8                            // Access: RO Volatile: True        Description: TS-07 Sample max/min high/low value.
  #define TS_07_HILO_RESET_ADDR       0x02ac                            // Access: WO Volatile: False       Description: TS-07 Reset sample high/low register.
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
  #define PD_04_IRQ_ENABLE_ADDR       0x0440                            // Access: RW Volatile: False       Description: PD-04 IRQ enable register, enables individual IRQ sources from the PD
  #define PD_04_IRQ_STATUS_ADDR       0x0444                            // Access: RO Volatile: True        Description: PD-04 IRQ status register, reports status of individual IRQ sources from the PD
  #define PD_04_IRQ_CLEAR_ADDR        0x0448                            // Access: WO Volatile: False       Description: PD-04 IRQ clear register, clears individual IRQ sources from the PD
  #define PD_04_IRQ_TEST_ADDR         0x044c                            // Access: RW Volatile: False       Description: PD-04 IRQ test register. Write to register will trigger an PD IRQ. If the IRQ enabled
  #define PD_04_SDIF_RDATA_ADDR       0x0450                            // Access: RO Volatile: True        Description: PD-04 read data register.
  #define PD_04_SDIF_DONE_ADDR        0x0454                            // Access: RO Volatile: True        Description: PD-04 SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define PD_04_SDIF_DATA_ADDR        0x0458                            // Access: RO Volatile: True        Description: PD-04 SDIF sample data register.
  #define PD_04_RES0_ADDR             0x045c                            // Access: RO Volatile: False       Description: Reserved. Returns 0, function may change in future revisions.
  #define PD_04_ALARMA_CFG_ADDR       0x0460                            // Access: RW Volatile: False       Description: PD-04 Alarm A configuration.
  #define PD_04_ALARMB_CFG_ADDR       0x0464                            // Access: RW Volatile: False       Description: PD-04 Alarm B configuration.
  #define PD_04_SMPL_HILO_ADDR        0x0468                            // Access: RO Volatile: True        Description: PD-04 Sample max/min high/low value.
  #define PD_04_HILO_RESET_ADDR       0x046c                            // Access: WO Volatile: False       Description: PD-04 Reset sample high/low register.
  #define PD_05_IRQ_ENABLE_ADDR       0x0480                            // Access: RW Volatile: False       Description: PD-05 IRQ enable register, enables individual IRQ sources from the PD
  #define PD_05_IRQ_STATUS_ADDR       0x0484                            // Access: RO Volatile: True        Description: PD-05 IRQ status register, reports status of individual IRQ sources from the PD
  #define PD_05_IRQ_CLEAR_ADDR        0x0488                            // Access: WO Volatile: False       Description: PD-05 IRQ clear register, clears individual IRQ sources from the PD
  #define PD_05_IRQ_TEST_ADDR         0x048c                            // Access: RW Volatile: False       Description: PD-05 IRQ test register. Write to register will trigger an PD IRQ. If the IRQ enabled
  #define PD_05_SDIF_RDATA_ADDR       0x0490                            // Access: RO Volatile: True        Description: PD-05 read data register.
  #define PD_05_SDIF_DONE_ADDR        0x0494                            // Access: RO Volatile: True        Description: PD-05 SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define PD_05_SDIF_DATA_ADDR        0x0498                            // Access: RO Volatile: True        Description: PD-05 SDIF sample data register.
  #define PD_05_RES0_ADDR             0x049c                            // Access: RO Volatile: False       Description: Reserved. Returns 0, function may change in future revisions.
  #define PD_05_ALARMA_CFG_ADDR       0x04a0                            // Access: RW Volatile: False       Description: PD-05 Alarm A configuration.
  #define PD_05_ALARMB_CFG_ADDR       0x04a4                            // Access: RW Volatile: False       Description: PD-05 Alarm B configuration.
  #define PD_05_SMPL_HILO_ADDR        0x04a8                            // Access: RO Volatile: True        Description: PD-05 Sample max/min high/low value.
  #define PD_05_HILO_RESET_ADDR       0x04ac                            // Access: WO Volatile: False       Description: PD-05 Reset sample high/low register.
  #define PD_06_IRQ_ENABLE_ADDR       0x04c0                            // Access: RW Volatile: False       Description: PD-06 IRQ enable register, enables individual IRQ sources from the PD
  #define PD_06_IRQ_STATUS_ADDR       0x04c4                            // Access: RO Volatile: True        Description: PD-06 IRQ status register, reports status of individual IRQ sources from the PD
  #define PD_06_IRQ_CLEAR_ADDR        0x04c8                            // Access: WO Volatile: False       Description: PD-06 IRQ clear register, clears individual IRQ sources from the PD
  #define PD_06_IRQ_TEST_ADDR         0x04cc                            // Access: RW Volatile: False       Description: PD-06 IRQ test register. Write to register will trigger an PD IRQ. If the IRQ enabled
  #define PD_06_SDIF_RDATA_ADDR       0x04d0                            // Access: RO Volatile: True        Description: PD-06 read data register.
  #define PD_06_SDIF_DONE_ADDR        0x04d4                            // Access: RO Volatile: True        Description: PD-06 SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define PD_06_SDIF_DATA_ADDR        0x04d8                            // Access: RO Volatile: True        Description: PD-06 SDIF sample data register.
  #define PD_06_RES0_ADDR             0x04dc                            // Access: RO Volatile: False       Description: Reserved. Returns 0, function may change in future revisions.
  #define PD_06_ALARMA_CFG_ADDR       0x04e0                            // Access: RW Volatile: False       Description: PD-06 Alarm A configuration.
  #define PD_06_ALARMB_CFG_ADDR       0x04e4                            // Access: RW Volatile: False       Description: PD-06 Alarm B configuration.
  #define PD_06_SMPL_HILO_ADDR        0x04e8                            // Access: RO Volatile: True        Description: PD-06 Sample max/min high/low value.
  #define PD_06_HILO_RESET_ADDR       0x04ec                            // Access: WO Volatile: False       Description: PD-06 Reset sample high/low register.
  #define PD_07_IRQ_ENABLE_ADDR       0x0500                            // Access: RW Volatile: False       Description: PD-07 IRQ enable register, enables individual IRQ sources from the PD
  #define PD_07_IRQ_STATUS_ADDR       0x0504                            // Access: RO Volatile: True        Description: PD-07 IRQ status register, reports status of individual IRQ sources from the PD
  #define PD_07_IRQ_CLEAR_ADDR        0x0508                            // Access: WO Volatile: False       Description: PD-07 IRQ clear register, clears individual IRQ sources from the PD
  #define PD_07_IRQ_TEST_ADDR         0x050c                            // Access: RW Volatile: False       Description: PD-07 IRQ test register. Write to register will trigger an PD IRQ. If the IRQ enabled
  #define PD_07_SDIF_RDATA_ADDR       0x0510                            // Access: RO Volatile: True        Description: PD-07 read data register.
  #define PD_07_SDIF_DONE_ADDR        0x0514                            // Access: RO Volatile: True        Description: PD-07 SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define PD_07_SDIF_DATA_ADDR        0x0518                            // Access: RO Volatile: True        Description: PD-07 SDIF sample data register.
  #define PD_07_RES0_ADDR             0x051c                            // Access: RO Volatile: False       Description: Reserved. Returns 0, function may change in future revisions.
  #define PD_07_ALARMA_CFG_ADDR       0x0520                            // Access: RW Volatile: False       Description: PD-07 Alarm A configuration.
  #define PD_07_ALARMB_CFG_ADDR       0x0524                            // Access: RW Volatile: False       Description: PD-07 Alarm B configuration.
  #define PD_07_SMPL_HILO_ADDR        0x0528                            // Access: RO Volatile: True        Description: PD-07 Sample max/min high/low value.
  #define PD_07_HILO_RESET_ADDR       0x052c                            // Access: WO Volatile: False       Description: PD-07 Reset sample high/low register.
  #define VM_CMN_CLK_SYNTH_ADDR           0x0800                                // Access: RW Volatile: False           Description: VM Clock Synthesiser control register.
  #define VM_CMN_SDIF_DISABLE_ADDR        0x0804                                // Access: RW Volatile: False           Description: VM SDIF disable (Active High). When asserted completely disables the selected VM instance(s), by forcing the VM macro clock and reset low.
  #define VM_CMN_SDIF_STATUS_ADDR         0x0808                                // Access: RO Volatile: True            Description: VM SDIF Status Register.   
  #define VM_CMN_SDIF_ADDR                0x080c                                // Access: RW Volatile: False           Description: VM SDIF write data register.
  #define VM_CMN_SDIF_HALT_ADDR           0x0810                                // Access: WO Volatile: False           Description: VM SDIF halt register. Halts all SDIF data transfer and resets SDIF slave.
  #define VM_CMN_SDIF_CTRL_ADDR           0x0814                                // Access: RW Volatile: False           Description: VM SDIF programming inhibit (Active High). When asserted inhibits serial programming of the selected VM instance(s).
  #define VM_CMN_SMPL_CTRL_ADDR           0x0820                                // Access: RW Volatile: False           Description: VM SDIF sample counter control.
  #define VM_CMN_SMPL_CLR_ADDR            0x0824                                // Access: WO Volatile: False           Description: VM SDIF sample counter clear.
  #define VM_CMN_SMPL_CNT_ADDR            0x0828                                // Access: RO Volatile: True            Description: VM SDIF sample counter current value.
  #define VM_00_IRQ_ENABLE_ADDR           0x0a00                                // Access: RW Volatile: False           Description: VM-00 IRQ enable register, enables individual IRQ sources from the VM
  #define VM_00_IRQ_STATUS_ADDR           0x0a04                                // Access: RO Volatile: True            Description: VM-00 IRQ status register, reports status of individual IRQ sources from the VM
  #define VM_00_IRQ_CLEAR_ADDR            0x0a08                                // Access: WO Volatile: False           Description: VM-00 IRQ clear register, clears individual IRQ sources from the VM
  #define VM_00_IRQ_TEST_ADDR             0x0a0c                                // Access: RW Volatile: False           Description: VM-00 IRQ test register. Write to register will trigger an VM IRQ. If the IRQ enabled
  #define VM_00_IRQ_ALARMA_ENABLE_ADDR    0x0a10                                // Access: RW Volatile: False           Description: VM-00 Alarm-A IRQ enable register, enables individual IRQ sources from the VM
  #define VM_00_IRQ_ALARMA_STATUS_ADDR    0x0a14                                // Access: RO Volatile: True            Description: VM-00 Alarm-A IRQ status register, reports status of individual IRQ sources from the VM
  #define VM_00_IRQ_ALARMA_CLR_ADDR       0x0a18                                // Access: WO Volatile: False           Description: VM-00 Alarm-A IRQ clear register, clears individual IRQ sources from the VM
  #define VM_00_IRQ_ALARMA_TEST_ADDR      0x0a1c                                // Access: RW Volatile: False           Description: VM-00 Alarm-A IRQ test register. Write to register will trigger an VM IRQ. If the IRQ enabled
  #define VM_00_IRQ_ALARMB_ENABLE_ADDR    0x0a20                                // Access: RW Volatile: False           Description: VM-00 Alarm-B IRQ enable register, enables individual IRQ sources from the VM
  #define VM_00_IRQ_ALARMB_STATUS_ADDR    0x0a24                                // Access: RO Volatile: True            Description: VM-00 Alarm-B IRQ status register, reports status of individual IRQ sources from the VM
  #define VM_00_IRQ_ALARMB_CLR_ADDR       0x0a28                                // Access: WO Volatile: False           Description: VM-00 Alarm-B IRQ clear register, clears individual IRQ sources from the VM
  #define VM_00_IRQ_ALARMB_TEST_ADDR      0x0a2c                                // Access: RW Volatile: False           Description: VM-00 Alarm-B IRQ test register. Write to register will trigger an VM IRQ. If the IRQ enabled
  #define VM_00_SDIF_RDATA_ADDR           0x0a30                                // Access: RO Volatile: True            Description: VM read data register.     
  #define VM_00_SDIF_DONE_ADDR            0x0a34                                // Access: RO Volatile: True            Description: VM SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define VM_00_CH_00_SDIF_DATA_ADDR      0x0a40                                // Access: RO Volatile: True            Description: VM-00 SDIF channel 00 sample data register.
  #define VM_00_CH_01_SDIF_DATA_ADDR      0x0a44                                // Access: RO Volatile: True            Description: VM-00 SDIF channel 01 sample data register.
  #define VM_00_CH_02_SDIF_DATA_ADDR      0x0a48                                // Access: RO Volatile: True            Description: VM-00 SDIF channel 02 sample data register.
  #define VM_00_CH_03_SDIF_DATA_ADDR      0x0a4c                                // Access: RO Volatile: True            Description: VM-00 SDIF channel 03 sample data register.
  #define VM_00_CH_04_SDIF_DATA_ADDR      0x0a50                                // Access: RO Volatile: True            Description: VM-00 SDIF channel 04 sample data register.
  #define VM_00_CH_05_SDIF_DATA_ADDR      0x0a54                                // Access: RO Volatile: True            Description: VM-00 SDIF channel 05 sample data register.
  #define VM_00_CH_06_SDIF_DATA_ADDR      0x0a58                                // Access: RO Volatile: True            Description: VM-00 SDIF channel 06 sample data register.
  #define VM_00_CH_07_SDIF_DATA_ADDR      0x0a5c                                // Access: RO Volatile: True            Description: VM-00 SDIF channel 07 sample data register.
  #define VM_00_CH_08_SDIF_DATA_ADDR      0x0a60                                // Access: RO Volatile: True            Description: VM-00 SDIF channel 08 sample data register.
  #define VM_00_CH_09_SDIF_DATA_ADDR      0x0a64                                // Access: RO Volatile: True            Description: VM-00 SDIF channel 09 sample data register.
  #define VM_00_CH_10_SDIF_DATA_ADDR      0x0a68                                // Access: RO Volatile: True            Description: VM-00 SDIF channel 10 sample data register.
  #define VM_00_CH_11_SDIF_DATA_ADDR      0x0a6c                                // Access: RO Volatile: True            Description: VM-00 SDIF channel 11 sample data register.
  #define VM_00_CH_12_SDIF_DATA_ADDR      0x0a70                                // Access: RO Volatile: True            Description: VM-00 SDIF channel 12 sample data register.
  #define VM_00_CH_13_SDIF_DATA_ADDR      0x0a74                                // Access: RO Volatile: True            Description: VM-00 SDIF channel 13 sample data register.
  #define VM_00_CH_14_SDIF_DATA_ADDR      0x0a78                                // Access: RO Volatile: True            Description: VM-00 SDIF channel 14 sample data register.
  #define VM_00_CH_15_SDIF_DATA_ADDR      0x0a7c                                // Access: RO Volatile: True            Description: VM-00 SDIF channel 15 sample data register.
  #define VM_00_CH_00_ALARMA_CFG_ADDR     0x0a80                                // Access: RW Volatile: False           Description: VM-00 channel 00 Alarm A configuration.
  #define VM_00_CH_00_ALARMB_CFG_ADDR     0x0a84                                // Access: RW Volatile: False           Description: VM-00 channel 00 Alarm B configuration.
  #define VM_00_CH_00_SMPL_HILO_ADDR      0x0a88                                // Access: RO Volatile: True            Description: VM-00 channel 00 Sample max/min high/low value.
  #define VM_00_CH_00_HILO_RESET_ADDR     0x0a8c                                // Access: WO Volatile: False           Description: VM-00 channel 00 Reset sample high/low register.
  #define VM_00_CH_01_ALARMA_CFG_ADDR     0x0a90                                // Access: RW Volatile: False           Description: VM-00 channel 01 Alarm A configuration.
  #define VM_00_CH_01_ALARMB_CFG_ADDR     0x0a94                                // Access: RW Volatile: False           Description: VM-00 channel 01 Alarm B configuration.
  #define VM_00_CH_01_SMPL_HILO_ADDR      0x0a98                                // Access: RO Volatile: True            Description: VM-00 channel 01 Sample max/min high/low value.
  #define VM_00_CH_01_HILO_RESET_ADDR     0x0a9c                                // Access: WO Volatile: False           Description: VM-00 channel 01 Reset sample high/low register.
  #define VM_00_CH_02_ALARMA_CFG_ADDR     0x0aa0                                // Access: RW Volatile: False           Description: VM-00 channel 02 Alarm A configuration.
  #define VM_00_CH_02_ALARMB_CFG_ADDR     0x0aa4                                // Access: RW Volatile: False           Description: VM-00 channel 02 Alarm B configuration.
  #define VM_00_CH_02_SMPL_HILO_ADDR      0x0aa8                                // Access: RO Volatile: True            Description: VM-00 channel 02 Sample max/min high/low value.
  #define VM_00_CH_02_HILO_RESET_ADDR     0x0aac                                // Access: WO Volatile: False           Description: VM-00 channel 02 Reset sample high/low register.
  #define VM_00_CH_03_ALARMA_CFG_ADDR     0x0ab0                                // Access: RW Volatile: False           Description: VM-00 channel 03 Alarm A configuration.
  #define VM_00_CH_03_ALARMB_CFG_ADDR     0x0ab4                                // Access: RW Volatile: False           Description: VM-00 channel 03 Alarm B configuration.
  #define VM_00_CH_03_SMPL_HILO_ADDR      0x0ab8                                // Access: RO Volatile: True            Description: VM-00 channel 03 Sample max/min high/low value.
  #define VM_00_CH_03_HILO_RESET_ADDR     0x0abc                                // Access: WO Volatile: False           Description: VM-00 channel 03 Reset sample high/low register.
  #define VM_00_CH_04_ALARMA_CFG_ADDR     0x0ac0                                // Access: RW Volatile: False           Description: VM-00 channel 04 Alarm A configuration.
  #define VM_00_CH_04_ALARMB_CFG_ADDR     0x0ac4                                // Access: RW Volatile: False           Description: VM-00 channel 04 Alarm B configuration.
  #define VM_00_CH_04_SMPL_HILO_ADDR      0x0ac8                                // Access: RO Volatile: True            Description: VM-00 channel 04 Sample max/min high/low value.
  #define VM_00_CH_04_HILO_RESET_ADDR     0x0acc                                // Access: WO Volatile: False           Description: VM-00 channel 04 Reset sample high/low register.
  #define VM_00_CH_05_ALARMA_CFG_ADDR     0x0ad0                                // Access: RW Volatile: False           Description: VM-00 channel 05 Alarm A configuration.
  #define VM_00_CH_05_ALARMB_CFG_ADDR     0x0ad4                                // Access: RW Volatile: False           Description: VM-00 channel 05 Alarm B configuration.
  #define VM_00_CH_05_SMPL_HILO_ADDR      0x0ad8                                // Access: RO Volatile: True            Description: VM-00 channel 05 Sample max/min high/low value.
  #define VM_00_CH_05_HILO_RESET_ADDR     0x0adc                                // Access: WO Volatile: False           Description: VM-00 channel 05 Reset sample high/low register.
  #define VM_00_CH_06_ALARMA_CFG_ADDR     0x0ae0                                // Access: RW Volatile: False           Description: VM-00 channel 06 Alarm A configuration.
  #define VM_00_CH_06_ALARMB_CFG_ADDR     0x0ae4                                // Access: RW Volatile: False           Description: VM-00 channel 06 Alarm B configuration.
  #define VM_00_CH_06_SMPL_HILO_ADDR      0x0ae8                                // Access: RO Volatile: True            Description: VM-00 channel 06 Sample max/min high/low value.
  #define VM_00_CH_06_HILO_RESET_ADDR     0x0aec                                // Access: WO Volatile: False           Description: VM-00 channel 06 Reset sample high/low register.
  #define VM_00_CH_07_ALARMA_CFG_ADDR     0x0af0                                // Access: RW Volatile: False           Description: VM-00 channel 07 Alarm A configuration.
  #define VM_00_CH_07_ALARMB_CFG_ADDR     0x0af4                                // Access: RW Volatile: False           Description: VM-00 channel 07 Alarm B configuration.
  #define VM_00_CH_07_SMPL_HILO_ADDR      0x0af8                                // Access: RO Volatile: True            Description: VM-00 channel 07 Sample max/min high/low value.
  #define VM_00_CH_07_HILO_RESET_ADDR     0x0afc                                // Access: WO Volatile: False           Description: VM-00 channel 07 Reset sample high/low register.
  #define VM_00_CH_08_ALARMA_CFG_ADDR     0x0b00                                // Access: RW Volatile: False           Description: VM-00 channel 08 Alarm A configuration.
  #define VM_00_CH_08_ALARMB_CFG_ADDR     0x0b04                                // Access: RW Volatile: False           Description: VM-00 channel 08 Alarm B configuration.
  #define VM_00_CH_08_SMPL_HILO_ADDR      0x0b08                                // Access: RO Volatile: True            Description: VM-00 channel 08 Sample max/min high/low value.
  #define VM_00_CH_08_HILO_RESET_ADDR     0x0b0c                                // Access: WO Volatile: False           Description: VM-00 channel 08 Reset sample high/low register.
  #define VM_00_CH_09_ALARMA_CFG_ADDR     0x0b10                                // Access: RW Volatile: False           Description: VM-00 channel 09 Alarm A configuration.
  #define VM_00_CH_09_ALARMB_CFG_ADDR     0x0b14                                // Access: RW Volatile: False           Description: VM-00 channel 09 Alarm B configuration.
  #define VM_00_CH_09_SMPL_HILO_ADDR      0x0b18                                // Access: RO Volatile: True            Description: VM-00 channel 09 Sample max/min high/low value.
  #define VM_00_CH_09_HILO_RESET_ADDR     0x0b1c                                // Access: WO Volatile: False           Description: VM-00 channel 09 Reset sample high/low register.
  #define VM_00_CH_10_ALARMA_CFG_ADDR     0x0b20                                // Access: RW Volatile: False           Description: VM-00 channel 10 Alarm A configuration.
  #define VM_00_CH_10_ALARMB_CFG_ADDR     0x0b24                                // Access: RW Volatile: False           Description: VM-00 channel 10 Alarm B configuration.
  #define VM_00_CH_10_SMPL_HILO_ADDR      0x0b28                                // Access: RO Volatile: True            Description: VM-00 channel 10 Sample max/min high/low value.
  #define VM_00_CH_10_HILO_RESET_ADDR     0x0b2c                                // Access: WO Volatile: False           Description: VM-00 channel 10 Reset sample high/low register.
  #define VM_00_CH_11_ALARMA_CFG_ADDR     0x0b30                                // Access: RW Volatile: False           Description: VM-00 channel 11 Alarm A configuration.
  #define VM_00_CH_11_ALARMB_CFG_ADDR     0x0b34                                // Access: RW Volatile: False           Description: VM-00 channel 11 Alarm B configuration.
  #define VM_00_CH_11_SMPL_HILO_ADDR      0x0b38                                // Access: RO Volatile: True            Description: VM-00 channel 11 Sample max/min high/low value.
  #define VM_00_CH_11_HILO_RESET_ADDR     0x0b3c                                // Access: WO Volatile: False           Description: VM-00 channel 11 Reset sample high/low register.
  #define VM_00_CH_12_ALARMA_CFG_ADDR     0x0b40                                // Access: RW Volatile: False           Description: VM-00 channel 12 Alarm A configuration.
  #define VM_00_CH_12_ALARMB_CFG_ADDR     0x0b44                                // Access: RW Volatile: False           Description: VM-00 channel 12 Alarm B configuration.
  #define VM_00_CH_12_SMPL_HILO_ADDR      0x0b48                                // Access: RO Volatile: True            Description: VM-00 channel 12 Sample max/min high/low value.
  #define VM_00_CH_12_HILO_RESET_ADDR     0x0b4c                                // Access: WO Volatile: False           Description: VM-00 channel 12 Reset sample high/low register.
  #define VM_00_CH_13_ALARMA_CFG_ADDR     0x0b50                                // Access: RW Volatile: False           Description: VM-00 channel 13 Alarm A configuration.
  #define VM_00_CH_13_ALARMB_CFG_ADDR     0x0b54                                // Access: RW Volatile: False           Description: VM-00 channel 13 Alarm B configuration.
  #define VM_00_CH_13_SMPL_HILO_ADDR      0x0b58                                // Access: RO Volatile: True            Description: VM-00 channel 13 Sample max/min high/low value.
  #define VM_00_CH_13_HILO_RESET_ADDR     0x0b5c                                // Access: WO Volatile: False           Description: VM-00 channel 13 Reset sample high/low register.
  #define VM_00_CH_14_ALARMA_CFG_ADDR     0x0b60                                // Access: RW Volatile: False           Description: VM-00 channel 14 Alarm A configuration.
  #define VM_00_CH_14_ALARMB_CFG_ADDR     0x0b64                                // Access: RW Volatile: False           Description: VM-00 channel 14 Alarm B configuration.
  #define VM_00_CH_14_SMPL_HILO_ADDR      0x0b68                                // Access: RO Volatile: True            Description: VM-00 channel 14 Sample max/min high/low value.
  #define VM_00_CH_14_HILO_RESET_ADDR     0x0b6c                                // Access: WO Volatile: False           Description: VM-00 channel 14 Reset sample high/low register.
  #define VM_00_CH_15_ALARMA_CFG_ADDR     0x0b70                                // Access: RW Volatile: False           Description: VM-00 channel 15 Alarm A configuration.
  #define VM_00_CH_15_ALARMB_CFG_ADDR     0x0b74                                // Access: RW Volatile: False           Description: VM-00 channel 15 Alarm B configuration.
  #define VM_00_CH_15_SMPL_HILO_ADDR      0x0b78                                // Access: RO Volatile: True            Description: VM-00 channel 15 Sample max/min high/low value.
  #define VM_00_CH_15_HILO_RESET_ADDR     0x0b7c                                // Access: WO Volatile: False           Description: VM-00 channel 15 Reset sample high/low register.
  #define VM_01_IRQ_ENABLE_ADDR           0x0c00                                // Access: RW Volatile: False           Description: VM-01 IRQ enable register, enables individual IRQ sources from the VM
  #define VM_01_IRQ_STATUS_ADDR           0x0c04                                // Access: RO Volatile: True            Description: VM-01 IRQ status register, reports status of individual IRQ sources from the VM
  #define VM_01_IRQ_CLEAR_ADDR            0x0c08                                // Access: WO Volatile: False           Description: VM-01 IRQ clear register, clears individual IRQ sources from the VM
  #define VM_01_IRQ_TEST_ADDR             0x0c0c                                // Access: RW Volatile: False           Description: VM-01 IRQ test register. Write to register will trigger an VM IRQ. If the IRQ enabled
  #define VM_01_IRQ_ALARMA_ENABLE_ADDR    0x0c10                                // Access: RW Volatile: False           Description: VM-01 Alarm-A IRQ enable register, enables individual IRQ sources from the VM
  #define VM_01_IRQ_ALARMA_STATUS_ADDR    0x0c14                                // Access: RO Volatile: True            Description: VM-01 Alarm-A IRQ status register, reports status of individual IRQ sources from the VM
  #define VM_01_IRQ_ALARMA_CLR_ADDR       0x0c18                                // Access: WO Volatile: False           Description: VM-01 Alarm-A IRQ clear register, clears individual IRQ sources from the VM
  #define VM_01_IRQ_ALARMA_TEST_ADDR      0x0c1c                                // Access: RW Volatile: False           Description: VM-01 Alarm-A IRQ test register. Write to register will trigger an VM IRQ. If the IRQ enabled
  #define VM_01_IRQ_ALARMB_ENABLE_ADDR    0x0c20                                // Access: RW Volatile: False           Description: VM-01 Alarm-B IRQ enable register, enables individual IRQ sources from the VM
  #define VM_01_IRQ_ALARMB_STATUS_ADDR    0x0c24                                // Access: RO Volatile: True            Description: VM-01 Alarm-B IRQ status register, reports status of individual IRQ sources from the VM
  #define VM_01_IRQ_ALARMB_CLR_ADDR       0x0c28                                // Access: WO Volatile: False           Description: VM-01 Alarm-B IRQ clear register, clears individual IRQ sources from the VM
  #define VM_01_IRQ_ALARMB_TEST_ADDR      0x0c2c                                // Access: RW Volatile: False           Description: VM-01 Alarm-B IRQ test register. Write to register will trigger an VM IRQ. If the IRQ enabled
  #define VM_01_SDIF_RDATA_ADDR           0x0c30                                // Access: RO Volatile: True            Description: VM read data register.     
  #define VM_01_SDIF_DONE_ADDR            0x0c34                                // Access: RO Volatile: True            Description: VM SDIF sample done register. Indicates sample data is available, cleared by of SDIF sample data register.
  #define VM_01_CH_00_SDIF_DATA_ADDR      0x0c40                                // Access: RO Volatile: True            Description: VM-01 SDIF channel 00 sample data register.
  #define VM_01_CH_01_SDIF_DATA_ADDR      0x0c44                                // Access: RO Volatile: True            Description: VM-01 SDIF channel 01 sample data register.
  #define VM_01_CH_02_SDIF_DATA_ADDR      0x0c48                                // Access: RO Volatile: True            Description: VM-01 SDIF channel 02 sample data register.
  #define VM_01_CH_03_SDIF_DATA_ADDR      0x0c4c                                // Access: RO Volatile: True            Description: VM-01 SDIF channel 03 sample data register.
  #define VM_01_CH_04_SDIF_DATA_ADDR      0x0c50                                // Access: RO Volatile: True            Description: VM-01 SDIF channel 04 sample data register.
  #define VM_01_CH_05_SDIF_DATA_ADDR      0x0c54                                // Access: RO Volatile: True            Description: VM-01 SDIF channel 05 sample data register.
  #define VM_01_CH_06_SDIF_DATA_ADDR      0x0c58                                // Access: RO Volatile: True            Description: VM-01 SDIF channel 06 sample data register.
  #define VM_01_CH_07_SDIF_DATA_ADDR      0x0c5c                                // Access: RO Volatile: True            Description: VM-01 SDIF channel 07 sample data register.
  #define VM_01_CH_08_SDIF_DATA_ADDR      0x0c60                                // Access: RO Volatile: True            Description: VM-01 SDIF channel 08 sample data register.
  #define VM_01_CH_09_SDIF_DATA_ADDR      0x0c64                                // Access: RO Volatile: True            Description: VM-01 SDIF channel 09 sample data register.
  #define VM_01_CH_10_SDIF_DATA_ADDR      0x0c68                                // Access: RO Volatile: True            Description: VM-01 SDIF channel 10 sample data register.
  #define VM_01_CH_11_SDIF_DATA_ADDR      0x0c6c                                // Access: RO Volatile: True            Description: VM-01 SDIF channel 11 sample data register.
  #define VM_01_CH_12_SDIF_DATA_ADDR      0x0c70                                // Access: RO Volatile: True            Description: VM-01 SDIF channel 12 sample data register.
  #define VM_01_CH_13_SDIF_DATA_ADDR      0x0c74                                // Access: RO Volatile: True            Description: VM-01 SDIF channel 13 sample data register.
  #define VM_01_CH_14_SDIF_DATA_ADDR      0x0c78                                // Access: RO Volatile: True            Description: VM-01 SDIF channel 14 sample data register.
  #define VM_01_CH_15_SDIF_DATA_ADDR      0x0c7c                                // Access: RO Volatile: True            Description: VM-01 SDIF channel 15 sample data register.
  #define VM_01_CH_00_ALARMA_CFG_ADDR     0x0c80                                // Access: RW Volatile: False           Description: VM-01 channel 00 Alarm A configuration.
  #define VM_01_CH_00_ALARMB_CFG_ADDR     0x0c84                                // Access: RW Volatile: False           Description: VM-01 channel 00 Alarm B configuration.
  #define VM_01_CH_00_SMPL_HILO_ADDR      0x0c88                                // Access: RO Volatile: True            Description: VM-01 channel 00 Sample max/min high/low value.
  #define VM_01_CH_00_HILO_RESET_ADDR     0x0c8c                                // Access: WO Volatile: False           Description: VM-01 channel 00 Reset sample high/low register.
  #define VM_01_CH_01_ALARMA_CFG_ADDR     0x0c90                                // Access: RW Volatile: False           Description: VM-01 channel 01 Alarm A configuration.
  #define VM_01_CH_01_ALARMB_CFG_ADDR     0x0c94                                // Access: RW Volatile: False           Description: VM-01 channel 01 Alarm B configuration.
  #define VM_01_CH_01_SMPL_HILO_ADDR      0x0c98                                // Access: RO Volatile: True            Description: VM-01 channel 01 Sample max/min high/low value.
  #define VM_01_CH_01_HILO_RESET_ADDR     0x0c9c                                // Access: WO Volatile: False           Description: VM-01 channel 01 Reset sample high/low register.
  #define VM_01_CH_02_ALARMA_CFG_ADDR     0x0ca0                                // Access: RW Volatile: False           Description: VM-01 channel 02 Alarm A configuration.
  #define VM_01_CH_02_ALARMB_CFG_ADDR     0x0ca4                                // Access: RW Volatile: False           Description: VM-01 channel 02 Alarm B configuration.
  #define VM_01_CH_02_SMPL_HILO_ADDR      0x0ca8                                // Access: RO Volatile: True            Description: VM-01 channel 02 Sample max/min high/low value.
  #define VM_01_CH_02_HILO_RESET_ADDR     0x0cac                                // Access: WO Volatile: False           Description: VM-01 channel 02 Reset sample high/low register.
  #define VM_01_CH_03_ALARMA_CFG_ADDR     0x0cb0                                // Access: RW Volatile: False           Description: VM-01 channel 03 Alarm A configuration.
  #define VM_01_CH_03_ALARMB_CFG_ADDR     0x0cb4                                // Access: RW Volatile: False           Description: VM-01 channel 03 Alarm B configuration.
  #define VM_01_CH_03_SMPL_HILO_ADDR      0x0cb8                                // Access: RO Volatile: True            Description: VM-01 channel 03 Sample max/min high/low value.
  #define VM_01_CH_03_HILO_RESET_ADDR     0x0cbc                                // Access: WO Volatile: False           Description: VM-01 channel 03 Reset sample high/low register.
  #define VM_01_CH_04_ALARMA_CFG_ADDR     0x0cc0                                // Access: RW Volatile: False           Description: VM-01 channel 04 Alarm A configuration.
  #define VM_01_CH_04_ALARMB_CFG_ADDR     0x0cc4                                // Access: RW Volatile: False           Description: VM-01 channel 04 Alarm B configuration.
  #define VM_01_CH_04_SMPL_HILO_ADDR      0x0cc8                                // Access: RO Volatile: True            Description: VM-01 channel 04 Sample max/min high/low value.
  #define VM_01_CH_04_HILO_RESET_ADDR     0x0ccc                                // Access: WO Volatile: False           Description: VM-01 channel 04 Reset sample high/low register.
  #define VM_01_CH_05_ALARMA_CFG_ADDR     0x0cd0                                // Access: RW Volatile: False           Description: VM-01 channel 05 Alarm A configuration.
  #define VM_01_CH_05_ALARMB_CFG_ADDR     0x0cd4                                // Access: RW Volatile: False           Description: VM-01 channel 05 Alarm B configuration.
  #define VM_01_CH_05_SMPL_HILO_ADDR      0x0cd8                                // Access: RO Volatile: True            Description: VM-01 channel 05 Sample max/min high/low value.
  #define VM_01_CH_05_HILO_RESET_ADDR     0x0cdc                                // Access: WO Volatile: False           Description: VM-01 channel 05 Reset sample high/low register.
  #define VM_01_CH_06_ALARMA_CFG_ADDR     0x0ce0                                // Access: RW Volatile: False           Description: VM-01 channel 06 Alarm A configuration.
  #define VM_01_CH_06_ALARMB_CFG_ADDR     0x0ce4                                // Access: RW Volatile: False           Description: VM-01 channel 06 Alarm B configuration.
  #define VM_01_CH_06_SMPL_HILO_ADDR      0x0ce8                                // Access: RO Volatile: True            Description: VM-01 channel 06 Sample max/min high/low value.
  #define VM_01_CH_06_HILO_RESET_ADDR     0x0cec                                // Access: WO Volatile: False           Description: VM-01 channel 06 Reset sample high/low register.
  #define VM_01_CH_07_ALARMA_CFG_ADDR     0x0cf0                                // Access: RW Volatile: False           Description: VM-01 channel 07 Alarm A configuration.
  #define VM_01_CH_07_ALARMB_CFG_ADDR     0x0cf4                                // Access: RW Volatile: False           Description: VM-01 channel 07 Alarm B configuration.
  #define VM_01_CH_07_SMPL_HILO_ADDR      0x0cf8                                // Access: RO Volatile: True            Description: VM-01 channel 07 Sample max/min high/low value.
  #define VM_01_CH_07_HILO_RESET_ADDR     0x0cfc                                // Access: WO Volatile: False           Description: VM-01 channel 07 Reset sample high/low register.
  #define VM_01_CH_08_ALARMA_CFG_ADDR     0x0d00                                // Access: RW Volatile: False           Description: VM-01 channel 08 Alarm A configuration.
  #define VM_01_CH_08_ALARMB_CFG_ADDR     0x0d04                                // Access: RW Volatile: False           Description: VM-01 channel 08 Alarm B configuration.
  #define VM_01_CH_08_SMPL_HILO_ADDR      0x0d08                                // Access: RO Volatile: True            Description: VM-01 channel 08 Sample max/min high/low value.
  #define VM_01_CH_08_HILO_RESET_ADDR     0x0d0c                                // Access: WO Volatile: False           Description: VM-01 channel 08 Reset sample high/low register.
  #define VM_01_CH_09_ALARMA_CFG_ADDR     0x0d10                                // Access: RW Volatile: False           Description: VM-01 channel 09 Alarm A configuration.
  #define VM_01_CH_09_ALARMB_CFG_ADDR     0x0d14                                // Access: RW Volatile: False           Description: VM-01 channel 09 Alarm B configuration.
  #define VM_01_CH_09_SMPL_HILO_ADDR      0x0d18                                // Access: RO Volatile: True            Description: VM-01 channel 09 Sample max/min high/low value.
  #define VM_01_CH_09_HILO_RESET_ADDR     0x0d1c                                // Access: WO Volatile: False           Description: VM-01 channel 09 Reset sample high/low register.
  #define VM_01_CH_10_ALARMA_CFG_ADDR     0x0d20                                // Access: RW Volatile: False           Description: VM-01 channel 10 Alarm A configuration.
  #define VM_01_CH_10_ALARMB_CFG_ADDR     0x0d24                                // Access: RW Volatile: False           Description: VM-01 channel 10 Alarm B configuration.
  #define VM_01_CH_10_SMPL_HILO_ADDR      0x0d28                                // Access: RO Volatile: True            Description: VM-01 channel 10 Sample max/min high/low value.
  #define VM_01_CH_10_HILO_RESET_ADDR     0x0d2c                                // Access: WO Volatile: False           Description: VM-01 channel 10 Reset sample high/low register.
  #define VM_01_CH_11_ALARMA_CFG_ADDR     0x0d30                                // Access: RW Volatile: False           Description: VM-01 channel 11 Alarm A configuration.
  #define VM_01_CH_11_ALARMB_CFG_ADDR     0x0d34                                // Access: RW Volatile: False           Description: VM-01 channel 11 Alarm B configuration.
  #define VM_01_CH_11_SMPL_HILO_ADDR      0x0d38                                // Access: RO Volatile: True            Description: VM-01 channel 11 Sample max/min high/low value.
  #define VM_01_CH_11_HILO_RESET_ADDR     0x0d3c                                // Access: WO Volatile: False           Description: VM-01 channel 11 Reset sample high/low register.
  #define VM_01_CH_12_ALARMA_CFG_ADDR     0x0d40                                // Access: RW Volatile: False           Description: VM-01 channel 12 Alarm A configuration.
  #define VM_01_CH_12_ALARMB_CFG_ADDR     0x0d44                                // Access: RW Volatile: False           Description: VM-01 channel 12 Alarm B configuration.
  #define VM_01_CH_12_SMPL_HILO_ADDR      0x0d48                                // Access: RO Volatile: True            Description: VM-01 channel 12 Sample max/min high/low value.
  #define VM_01_CH_12_HILO_RESET_ADDR     0x0d4c                                // Access: WO Volatile: False           Description: VM-01 channel 12 Reset sample high/low register.
  #define VM_01_CH_13_ALARMA_CFG_ADDR     0x0d50                                // Access: RW Volatile: False           Description: VM-01 channel 13 Alarm A configuration.
  #define VM_01_CH_13_ALARMB_CFG_ADDR     0x0d54                                // Access: RW Volatile: False           Description: VM-01 channel 13 Alarm B configuration.
  #define VM_01_CH_13_SMPL_HILO_ADDR      0x0d58                                // Access: RO Volatile: True            Description: VM-01 channel 13 Sample max/min high/low value.
  #define VM_01_CH_13_HILO_RESET_ADDR     0x0d5c                                // Access: WO Volatile: False           Description: VM-01 channel 13 Reset sample high/low register.
  #define VM_01_CH_14_ALARMA_CFG_ADDR     0x0d60                                // Access: RW Volatile: False           Description: VM-01 channel 14 Alarm A configuration.
  #define VM_01_CH_14_ALARMB_CFG_ADDR     0x0d64                                // Access: RW Volatile: False           Description: VM-01 channel 14 Alarm B configuration.
  #define VM_01_CH_14_SMPL_HILO_ADDR      0x0d68                                // Access: RO Volatile: True            Description: VM-01 channel 14 Sample max/min high/low value.
  #define VM_01_CH_14_HILO_RESET_ADDR     0x0d6c                                // Access: WO Volatile: False           Description: VM-01 channel 14 Reset sample high/low register.
  #define VM_01_CH_15_ALARMA_CFG_ADDR     0x0d70                                // Access: RW Volatile: False           Description: VM-01 channel 15 Alarm A configuration.
  #define VM_01_CH_15_ALARMB_CFG_ADDR     0x0d74                                // Access: RW Volatile: False           Description: VM-01 channel 15 Alarm B configuration.
  #define VM_01_CH_15_SMPL_HILO_ADDR      0x0d78                                // Access: RO Volatile: True            Description: VM-01 channel 15 Sample max/min high/low value.
  #define VM_01_CH_15_HILO_RESET_ADDR     0x0d7c                                // Access: WO Volatile: False           Description: VM-01 channel 15 Reset sample high/low register.
