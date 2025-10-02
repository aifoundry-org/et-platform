/* Header generated from ipxact (2019-01-09 14:47:11) */


#ifndef __DWC_MSHC_CRYPTO_MACRO_H
#define __DWC_MSHC_CRYPTO_MACRO_H



/*-------------------------------------------------------------------------------------*/
/*  SDMASA_R : BLOCKCNT_SDMASA     */
#define SDMASA_R__BLOCKCNT_SDMASA__SHIFT                                0x00000000
#define SDMASA_R__BLOCKCNT_SDMASA__WIDTH                                0x00000020
#define SDMASA_R__BLOCKCNT_SDMASA__MASK                                 0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  BLOCKSIZE_R : XFER_BLOCK_SIZE     */
#define BLOCKSIZE_R__XFER_BLOCK_SIZE__SHIFT                             0x00000000
#define BLOCKSIZE_R__XFER_BLOCK_SIZE__WIDTH                             0x0000000C
#define BLOCKSIZE_R__XFER_BLOCK_SIZE__MASK                              0x00000FFF

/*  BLOCKSIZE_R : SDMA_BUF_BDARY      */
#define BLOCKSIZE_R__SDMA_BUF_BDARY__SHIFT                              0x0000000C
#define BLOCKSIZE_R__SDMA_BUF_BDARY__WIDTH                              0x00000003
#define BLOCKSIZE_R__SDMA_BUF_BDARY__MASK                               0x00007000


/*  BLOCKSIZE_R : RSVD_BLOCKSIZE15    */
#define BLOCKSIZE_R__RSVD_BLOCKSIZE15__SHIFT                            0x0000000F
#define BLOCKSIZE_R__RSVD_BLOCKSIZE15__WIDTH                            0x00000001
#define BLOCKSIZE_R__RSVD_BLOCKSIZE15__MASK                             0x00008000


/*-------------------------------------------------------------------------------------*/
/*  BLOCKCOUNT_R : BLOCK_CNT           */
#define BLOCKCOUNT_R__BLOCK_CNT__SHIFT                                  0x00000000
#define BLOCKCOUNT_R__BLOCK_CNT__WIDTH                                  0x00000010
#define BLOCKCOUNT_R__BLOCK_CNT__MASK                                   0x0000FFFF


/*-------------------------------------------------------------------------------------*/
/*  ARGUMENT_R : ARGUMENT            */
#define ARGUMENT_R__ARGUMENT__SHIFT                                     0x00000000
#define ARGUMENT_R__ARGUMENT__WIDTH                                     0x00000020
#define ARGUMENT_R__ARGUMENT__MASK                                      0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  XFER_MODE_R : DMA_ENABLE          */
#define XFER_MODE_R__DMA_ENABLE__SHIFT                                  0x00000000
#define XFER_MODE_R__DMA_ENABLE__WIDTH                                  0x00000001
#define XFER_MODE_R__DMA_ENABLE__MASK                                   0x00000001


/*  XFER_MODE_R : BLOCK_COUNT_ENABLE  */
#define XFER_MODE_R__BLOCK_COUNT_ENABLE__SHIFT                          0x00000001
#define XFER_MODE_R__BLOCK_COUNT_ENABLE__WIDTH                          0x00000001
#define XFER_MODE_R__BLOCK_COUNT_ENABLE__MASK                           0x00000002


/*  XFER_MODE_R : AUTO_CMD_ENABLE     */
#define XFER_MODE_R__AUTO_CMD_ENABLE__SHIFT                             0x00000002
#define XFER_MODE_R__AUTO_CMD_ENABLE__WIDTH                             0x00000002
#define XFER_MODE_R__AUTO_CMD_ENABLE__MASK                              0x0000000C


/*  XFER_MODE_R : DATA_XFER_DIR       */
#define XFER_MODE_R__DATA_XFER_DIR__SHIFT                               0x00000004
#define XFER_MODE_R__DATA_XFER_DIR__WIDTH                               0x00000001
#define XFER_MODE_R__DATA_XFER_DIR__MASK                                0x00000010


/*  XFER_MODE_R : MULTI_BLK_SEL       */
#define XFER_MODE_R__MULTI_BLK_SEL__SHIFT                               0x00000005
#define XFER_MODE_R__MULTI_BLK_SEL__WIDTH                               0x00000001
#define XFER_MODE_R__MULTI_BLK_SEL__MASK                                0x00000020


/*  XFER_MODE_R : RESP_TYPE           */
#define XFER_MODE_R__RESP_TYPE__SHIFT                                   0x00000006
#define XFER_MODE_R__RESP_TYPE__WIDTH                                   0x00000001
#define XFER_MODE_R__RESP_TYPE__MASK                                    0x00000040


/*  XFER_MODE_R : RESP_ERR_CHK_ENABLE */
#define XFER_MODE_R__RESP_ERR_CHK_ENABLE__SHIFT                         0x00000007
#define XFER_MODE_R__RESP_ERR_CHK_ENABLE__WIDTH                         0x00000001
#define XFER_MODE_R__RESP_ERR_CHK_ENABLE__MASK                          0x00000080


/*  XFER_MODE_R : RESP_INT_DISABLE    */
#define XFER_MODE_R__RESP_INT_DISABLE__SHIFT                            0x00000008
#define XFER_MODE_R__RESP_INT_DISABLE__WIDTH                            0x00000001
#define XFER_MODE_R__RESP_INT_DISABLE__MASK                             0x00000100


/*  XFER_MODE_R : RSVD                */
#define XFER_MODE_R__RSVD__SHIFT                                        0x00000009
#define XFER_MODE_R__RSVD__WIDTH                                        0x00000007
#define XFER_MODE_R__RSVD__MASK                                         0x0000FE00


/*-------------------------------------------------------------------------------------*/
/*  CMD_R : RESP_TYPE_SELECT    */
#define CMD_R__RESP_TYPE_SELECT__SHIFT                                  0x00000000
#define CMD_R__RESP_TYPE_SELECT__WIDTH                                  0x00000002
#define CMD_R__RESP_TYPE_SELECT__MASK                                   0x00000003


/*  CMD_R : SUB_CMD_FLAG        */
#define CMD_R__SUB_CMD_FLAG__SHIFT                                      0x00000002
#define CMD_R__SUB_CMD_FLAG__WIDTH                                      0x00000001
#define CMD_R__SUB_CMD_FLAG__MASK                                       0x00000004


/*  CMD_R : CMD_CRC_CHK_ENABLE  */
#define CMD_R__CMD_CRC_CHK_ENABLE__SHIFT                                0x00000003
#define CMD_R__CMD_CRC_CHK_ENABLE__WIDTH                                0x00000001
#define CMD_R__CMD_CRC_CHK_ENABLE__MASK                                 0x00000008


/*  CMD_R : CMD_IDX_CHK_ENABLE  */
#define CMD_R__CMD_IDX_CHK_ENABLE__SHIFT                                0x00000004
#define CMD_R__CMD_IDX_CHK_ENABLE__WIDTH                                0x00000001
#define CMD_R__CMD_IDX_CHK_ENABLE__MASK                                 0x00000010


/*  CMD_R : DATA_PRESENT_SEL    */
#define CMD_R__DATA_PRESENT_SEL__SHIFT                                  0x00000005
#define CMD_R__DATA_PRESENT_SEL__WIDTH                                  0x00000001
#define CMD_R__DATA_PRESENT_SEL__MASK                                   0x00000020


/*  CMD_R : CMD_TYPE            */
#define CMD_R__CMD_TYPE__SHIFT                                          0x00000006
#define CMD_R__CMD_TYPE__WIDTH                                          0x00000002
#define CMD_R__CMD_TYPE__MASK                                           0x000000C0


/*  CMD_R : CMD_INDEX           */
#define CMD_R__CMD_INDEX__SHIFT                                         0x00000008
#define CMD_R__CMD_INDEX__WIDTH                                         0x00000006
#define CMD_R__CMD_INDEX__MASK                                          0x00003F00

/*  CMD_R : RSVD                */
#define CMD_R__RSVD__SHIFT                                              0x0000000E
#define CMD_R__RSVD__WIDTH                                              0x00000002
#define CMD_R__RSVD__MASK                                               0x0000C000


/*-------------------------------------------------------------------------------------*/
/*  RESP01_R : RESP01              */
#define RESP01_R__RESP01__SHIFT                                         0x00000000
#define RESP01_R__RESP01__WIDTH                                         0x00000020
#define RESP01_R__RESP01__MASK                                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  RESP23_R : RESP23              */
#define RESP23_R__RESP23__SHIFT                                         0x00000000
#define RESP23_R__RESP23__WIDTH                                         0x00000020
#define RESP23_R__RESP23__MASK                                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  RESP45_R : RESP45              */
#define RESP45_R__RESP45__SHIFT                                         0x00000000
#define RESP45_R__RESP45__WIDTH                                         0x00000020
#define RESP45_R__RESP45__MASK                                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  RESP67_R : RESP67              */
#define RESP67_R__RESP67__SHIFT                                         0x00000000
#define RESP67_R__RESP67__WIDTH                                         0x00000020
#define RESP67_R__RESP67__MASK                                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  BUF_DATA_R : BUF_DATA            */
#define BUF_DATA_R__BUF_DATA__SHIFT                                     0x00000000
#define BUF_DATA_R__BUF_DATA__WIDTH                                     0x00000020
#define BUF_DATA_R__BUF_DATA__MASK                                      0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  PSTATE_REG : CMD_INHIBIT         */
#define PSTATE_REG__CMD_INHIBIT__SHIFT                                  0x00000000
#define PSTATE_REG__CMD_INHIBIT__WIDTH                                  0x00000001
#define PSTATE_REG__CMD_INHIBIT__MASK                                   0x00000001


/*  PSTATE_REG : CMD_INHIBIT_DAT     */
#define PSTATE_REG__CMD_INHIBIT_DAT__SHIFT                              0x00000001
#define PSTATE_REG__CMD_INHIBIT_DAT__WIDTH                              0x00000001
#define PSTATE_REG__CMD_INHIBIT_DAT__MASK                               0x00000002


/*  PSTATE_REG : DAT_LINE_ACTIVE     */
#define PSTATE_REG__DAT_LINE_ACTIVE__SHIFT                              0x00000002
#define PSTATE_REG__DAT_LINE_ACTIVE__WIDTH                              0x00000001
#define PSTATE_REG__DAT_LINE_ACTIVE__MASK                               0x00000004


/*  PSTATE_REG : RE_TUNE_REQ         */
#define PSTATE_REG__RE_TUNE_REQ__SHIFT                                  0x00000003
#define PSTATE_REG__RE_TUNE_REQ__WIDTH                                  0x00000001
#define PSTATE_REG__RE_TUNE_REQ__MASK                                   0x00000008

/*  PSTATE_REG : DAT_7_4             */
#define PSTATE_REG__DAT_7_4__SHIFT                                      0x00000004
#define PSTATE_REG__DAT_7_4__WIDTH                                      0x00000004
#define PSTATE_REG__DAT_7_4__MASK                                       0x000000F0

/*  PSTATE_REG : WR_XFER_ACTIVE      */
#define PSTATE_REG__WR_XFER_ACTIVE__SHIFT                               0x00000008
#define PSTATE_REG__WR_XFER_ACTIVE__WIDTH                               0x00000001
#define PSTATE_REG__WR_XFER_ACTIVE__MASK                                0x00000100


/*  PSTATE_REG : RD_XFER_ACTIVE      */
#define PSTATE_REG__RD_XFER_ACTIVE__SHIFT                               0x00000009
#define PSTATE_REG__RD_XFER_ACTIVE__WIDTH                               0x00000001
#define PSTATE_REG__RD_XFER_ACTIVE__MASK                                0x00000200


/*  PSTATE_REG : BUF_WR_ENABLE       */
#define PSTATE_REG__BUF_WR_ENABLE__SHIFT                                0x0000000A
#define PSTATE_REG__BUF_WR_ENABLE__WIDTH                                0x00000001
#define PSTATE_REG__BUF_WR_ENABLE__MASK                                 0x00000400


/*  PSTATE_REG : BUF_RD_ENABLE       */
#define PSTATE_REG__BUF_RD_ENABLE__SHIFT                                0x0000000B
#define PSTATE_REG__BUF_RD_ENABLE__WIDTH                                0x00000001
#define PSTATE_REG__BUF_RD_ENABLE__MASK                                 0x00000800


/*  PSTATE_REG : RSVD_15_12          */
#define PSTATE_REG__RSVD_15_12__SHIFT                                   0x0000000C
#define PSTATE_REG__RSVD_15_12__WIDTH                                   0x00000004
#define PSTATE_REG__RSVD_15_12__MASK                                    0x0000F000

/*  PSTATE_REG : CARD_INSERTED       */
#define PSTATE_REG__CARD_INSERTED__SHIFT                                0x00000010
#define PSTATE_REG__CARD_INSERTED__WIDTH                                0x00000001
#define PSTATE_REG__CARD_INSERTED__MASK                                 0x00010000


/*  PSTATE_REG : CARD_STABLE         */
#define PSTATE_REG__CARD_STABLE__SHIFT                                  0x00000011
#define PSTATE_REG__CARD_STABLE__WIDTH                                  0x00000001
#define PSTATE_REG__CARD_STABLE__MASK                                   0x00020000


/*  PSTATE_REG : CARD_DETECT_PIN_LEVEL*/
#define PSTATE_REG__CARD_DETECT_PIN_LEVEL__SHIFT                        0x00000012
#define PSTATE_REG__CARD_DETECT_PIN_LEVEL__WIDTH                        0x00000001
#define PSTATE_REG__CARD_DETECT_PIN_LEVEL__MASK                         0x00040000


/*  PSTATE_REG : WR_PROTECT_SW_LVL   */
#define PSTATE_REG__WR_PROTECT_SW_LVL__SHIFT                            0x00000013
#define PSTATE_REG__WR_PROTECT_SW_LVL__WIDTH                            0x00000001
#define PSTATE_REG__WR_PROTECT_SW_LVL__MASK                             0x00080000


/*  PSTATE_REG : DAT_3_0             */
#define PSTATE_REG__DAT_3_0__SHIFT                                      0x00000014
#define PSTATE_REG__DAT_3_0__WIDTH                                      0x00000004
#define PSTATE_REG__DAT_3_0__MASK                                       0x00F00000

/*  PSTATE_REG : CMD_LINE_LVL        */
#define PSTATE_REG__CMD_LINE_LVL__SHIFT                                 0x00000018
#define PSTATE_REG__CMD_LINE_LVL__WIDTH                                 0x00000001
#define PSTATE_REG__CMD_LINE_LVL__MASK                                  0x01000000

/*  PSTATE_REG : HOST_REG_VOL        */
#define PSTATE_REG__HOST_REG_VOL__SHIFT                                 0x00000019
#define PSTATE_REG__HOST_REG_VOL__WIDTH                                 0x00000001
#define PSTATE_REG__HOST_REG_VOL__MASK                                  0x02000000


/*  PSTATE_REG : RSVD_26             */
#define PSTATE_REG__RSVD_26__SHIFT                                      0x0000001A
#define PSTATE_REG__RSVD_26__WIDTH                                      0x00000001
#define PSTATE_REG__RSVD_26__MASK                                       0x04000000

/*  PSTATE_REG : CMD_ISSUE_ERR       */
#define PSTATE_REG__CMD_ISSUE_ERR__SHIFT                                0x0000001B
#define PSTATE_REG__CMD_ISSUE_ERR__WIDTH                                0x00000001
#define PSTATE_REG__CMD_ISSUE_ERR__MASK                                 0x08000000


/*  PSTATE_REG : SUB_CMD_STAT        */
#define PSTATE_REG__SUB_CMD_STAT__SHIFT                                 0x0000001C
#define PSTATE_REG__SUB_CMD_STAT__WIDTH                                 0x00000001
#define PSTATE_REG__SUB_CMD_STAT__MASK                                  0x10000000


/*  PSTATE_REG : IN_DORMANT_ST       */
#define PSTATE_REG__IN_DORMANT_ST__SHIFT                                0x0000001D
#define PSTATE_REG__IN_DORMANT_ST__WIDTH                                0x00000001
#define PSTATE_REG__IN_DORMANT_ST__MASK                                 0x20000000


/*  PSTATE_REG : LANE_SYNC           */
#define PSTATE_REG__LANE_SYNC__SHIFT                                    0x0000001E
#define PSTATE_REG__LANE_SYNC__WIDTH                                    0x00000001
#define PSTATE_REG__LANE_SYNC__MASK                                     0x40000000


/*  PSTATE_REG : UHS2_IF_DETECT      */
#define PSTATE_REG__UHS2_IF_DETECT__SHIFT                               0x0000001F
#define PSTATE_REG__UHS2_IF_DETECT__WIDTH                               0x00000001
#define PSTATE_REG__UHS2_IF_DETECT__MASK                                0x80000000



/*-------------------------------------------------------------------------------------*/
/*  HOST_CTRL1_R : LED_CTRL            */
#define HOST_CTRL1_R__LED_CTRL__SHIFT                                   0x00000000
#define HOST_CTRL1_R__LED_CTRL__WIDTH                                   0x00000001
#define HOST_CTRL1_R__LED_CTRL__MASK                                    0x00000001


/*  HOST_CTRL1_R : DAT_XFER_WIDTH      */
#define HOST_CTRL1_R__DAT_XFER_WIDTH__SHIFT                             0x00000001
#define HOST_CTRL1_R__DAT_XFER_WIDTH__WIDTH                             0x00000001
#define HOST_CTRL1_R__DAT_XFER_WIDTH__MASK                              0x00000002


/*  HOST_CTRL1_R : HIGH_SPEED_EN       */
#define HOST_CTRL1_R__HIGH_SPEED_EN__SHIFT                              0x00000002
#define HOST_CTRL1_R__HIGH_SPEED_EN__WIDTH                              0x00000001
#define HOST_CTRL1_R__HIGH_SPEED_EN__MASK                               0x00000004


/*  HOST_CTRL1_R : DMA_SEL             */
#define HOST_CTRL1_R__DMA_SEL__SHIFT                                    0x00000003
#define HOST_CTRL1_R__DMA_SEL__WIDTH                                    0x00000002
#define HOST_CTRL1_R__DMA_SEL__MASK                                     0x00000018


/*  HOST_CTRL1_R : EXT_DAT_XFER        */
#define HOST_CTRL1_R__EXT_DAT_XFER__SHIFT                               0x00000005
#define HOST_CTRL1_R__EXT_DAT_XFER__WIDTH                               0x00000001
#define HOST_CTRL1_R__EXT_DAT_XFER__MASK                                0x00000020


/*  HOST_CTRL1_R : CARD_DETECT_TEST_LVL*/
#define HOST_CTRL1_R__CARD_DETECT_TEST_LVL__SHIFT                       0x00000006
#define HOST_CTRL1_R__CARD_DETECT_TEST_LVL__WIDTH                       0x00000001
#define HOST_CTRL1_R__CARD_DETECT_TEST_LVL__MASK                        0x00000040


/*  HOST_CTRL1_R : CARD_DETECT_SIG_SEL */
#define HOST_CTRL1_R__CARD_DETECT_SIG_SEL__SHIFT                        0x00000007
#define HOST_CTRL1_R__CARD_DETECT_SIG_SEL__WIDTH                        0x00000001
#define HOST_CTRL1_R__CARD_DETECT_SIG_SEL__MASK                         0x00000080



/*-------------------------------------------------------------------------------------*/
/*  PWR_CTRL_R : SD_BUS_PWR_VDD1     */
#define PWR_CTRL_R__SD_BUS_PWR_VDD1__SHIFT                              0x00000000
#define PWR_CTRL_R__SD_BUS_PWR_VDD1__WIDTH                              0x00000001
#define PWR_CTRL_R__SD_BUS_PWR_VDD1__MASK                               0x00000001


/*  PWR_CTRL_R : SD_BUS_VOL_VDD1     */
#define PWR_CTRL_R__SD_BUS_VOL_VDD1__SHIFT                              0x00000001
#define PWR_CTRL_R__SD_BUS_VOL_VDD1__WIDTH                              0x00000003
#define PWR_CTRL_R__SD_BUS_VOL_VDD1__MASK                               0x0000000E


/*  PWR_CTRL_R : SD_BUS_PWR_VDD2     */
#define PWR_CTRL_R__SD_BUS_PWR_VDD2__SHIFT                              0x00000004
#define PWR_CTRL_R__SD_BUS_PWR_VDD2__WIDTH                              0x00000001
#define PWR_CTRL_R__SD_BUS_PWR_VDD2__MASK                               0x00000010


/*  PWR_CTRL_R : SD_BUS_VOL_VDD2     */
#define PWR_CTRL_R__SD_BUS_VOL_VDD2__SHIFT                              0x00000005
#define PWR_CTRL_R__SD_BUS_VOL_VDD2__WIDTH                              0x00000003
#define PWR_CTRL_R__SD_BUS_VOL_VDD2__MASK                               0x000000E0



/*-------------------------------------------------------------------------------------*/
/*  BGAP_CTRL_R : STOP_BG_REQ         */
#define BGAP_CTRL_R__STOP_BG_REQ__SHIFT                                 0x00000000
#define BGAP_CTRL_R__STOP_BG_REQ__WIDTH                                 0x00000001
#define BGAP_CTRL_R__STOP_BG_REQ__MASK                                  0x00000001


/*  BGAP_CTRL_R : CONTINUE_REQ        */
#define BGAP_CTRL_R__CONTINUE_REQ__SHIFT                                0x00000001
#define BGAP_CTRL_R__CONTINUE_REQ__WIDTH                                0x00000001
#define BGAP_CTRL_R__CONTINUE_REQ__MASK                                 0x00000002


/*  BGAP_CTRL_R : RD_WAIT_CTRL        */
#define BGAP_CTRL_R__RD_WAIT_CTRL__SHIFT                                0x00000002
#define BGAP_CTRL_R__RD_WAIT_CTRL__WIDTH                                0x00000001
#define BGAP_CTRL_R__RD_WAIT_CTRL__MASK                                 0x00000004


/*  BGAP_CTRL_R : INT_AT_BGAP         */
#define BGAP_CTRL_R__INT_AT_BGAP__SHIFT                                 0x00000003
#define BGAP_CTRL_R__INT_AT_BGAP__WIDTH                                 0x00000001
#define BGAP_CTRL_R__INT_AT_BGAP__MASK                                  0x00000008


/*  BGAP_CTRL_R : RSVD_7_4            */
#define BGAP_CTRL_R__RSVD_7_4__SHIFT                                    0x00000004
#define BGAP_CTRL_R__RSVD_7_4__WIDTH                                    0x00000004
#define BGAP_CTRL_R__RSVD_7_4__MASK                                     0x000000F0


/*-------------------------------------------------------------------------------------*/
/*  WUP_CTRL_R : CARD_INT            */
#define WUP_CTRL_R__CARD_INT__SHIFT                                     0x00000000
#define WUP_CTRL_R__CARD_INT__WIDTH                                     0x00000001
#define WUP_CTRL_R__CARD_INT__MASK                                      0x00000001


/*  WUP_CTRL_R : CARD_INSERT         */
#define WUP_CTRL_R__CARD_INSERT__SHIFT                                  0x00000001
#define WUP_CTRL_R__CARD_INSERT__WIDTH                                  0x00000001
#define WUP_CTRL_R__CARD_INSERT__MASK                                   0x00000002


/*  WUP_CTRL_R : CARD_REMOVAL        */
#define WUP_CTRL_R__CARD_REMOVAL__SHIFT                                 0x00000002
#define WUP_CTRL_R__CARD_REMOVAL__WIDTH                                 0x00000001
#define WUP_CTRL_R__CARD_REMOVAL__MASK                                  0x00000004


/*  WUP_CTRL_R : RSVD_7_3            */
#define WUP_CTRL_R__RSVD_7_3__SHIFT                                     0x00000003
#define WUP_CTRL_R__RSVD_7_3__WIDTH                                     0x00000005
#define WUP_CTRL_R__RSVD_7_3__MASK                                      0x000000F8


/*-------------------------------------------------------------------------------------*/
/*  CLK_CTRL_R : INTERNAL_CLK_EN     */
#define CLK_CTRL_R__INTERNAL_CLK_EN__SHIFT                              0x00000000
#define CLK_CTRL_R__INTERNAL_CLK_EN__WIDTH                              0x00000001
#define CLK_CTRL_R__INTERNAL_CLK_EN__MASK                               0x00000001


/*  CLK_CTRL_R : INTERNAL_CLK_STABLE */
#define CLK_CTRL_R__INTERNAL_CLK_STABLE__SHIFT                          0x00000001
#define CLK_CTRL_R__INTERNAL_CLK_STABLE__WIDTH                          0x00000001
#define CLK_CTRL_R__INTERNAL_CLK_STABLE__MASK                           0x00000002


/*  CLK_CTRL_R : SD_CLK_EN           */
#define CLK_CTRL_R__SD_CLK_EN__SHIFT                                    0x00000002
#define CLK_CTRL_R__SD_CLK_EN__WIDTH                                    0x00000001
#define CLK_CTRL_R__SD_CLK_EN__MASK                                     0x00000004


/*  CLK_CTRL_R : PLL_ENABLE          */
#define CLK_CTRL_R__PLL_ENABLE__SHIFT                                   0x00000003
#define CLK_CTRL_R__PLL_ENABLE__WIDTH                                   0x00000001
#define CLK_CTRL_R__PLL_ENABLE__MASK                                    0x00000008


/*  CLK_CTRL_R : RSVD_4              */
#define CLK_CTRL_R__RSVD_4__SHIFT                                       0x00000004
#define CLK_CTRL_R__RSVD_4__WIDTH                                       0x00000001
#define CLK_CTRL_R__RSVD_4__MASK                                        0x00000010

/*  CLK_CTRL_R : CLK_GEN_SELECT      */
#define CLK_CTRL_R__CLK_GEN_SELECT__SHIFT                               0x00000005
#define CLK_CTRL_R__CLK_GEN_SELECT__WIDTH                               0x00000001
#define CLK_CTRL_R__CLK_GEN_SELECT__MASK                                0x00000020


/*  CLK_CTRL_R : UPPER_FREQ_SEL      */
#define CLK_CTRL_R__UPPER_FREQ_SEL__SHIFT                               0x00000006
#define CLK_CTRL_R__UPPER_FREQ_SEL__WIDTH                               0x00000002
#define CLK_CTRL_R__UPPER_FREQ_SEL__MASK                                0x000000C0

/*  CLK_CTRL_R : FREQ_SEL            */
#define CLK_CTRL_R__FREQ_SEL__SHIFT                                     0x00000008
#define CLK_CTRL_R__FREQ_SEL__WIDTH                                     0x00000008
#define CLK_CTRL_R__FREQ_SEL__MASK                                      0x0000FF00


/*-------------------------------------------------------------------------------------*/
/*  TOUT_CTRL_R : TOUT_CNT            */
#define TOUT_CTRL_R__TOUT_CNT__SHIFT                                    0x00000000
#define TOUT_CTRL_R__TOUT_CNT__WIDTH                                    0x00000004
#define TOUT_CTRL_R__TOUT_CNT__MASK                                     0x0000000F

/*  TOUT_CTRL_R : RSVD_7_4            */
#define TOUT_CTRL_R__RSVD_7_4__SHIFT                                    0x00000004
#define TOUT_CTRL_R__RSVD_7_4__WIDTH                                    0x00000004
#define TOUT_CTRL_R__RSVD_7_4__MASK                                     0x000000F0


/*-------------------------------------------------------------------------------------*/
/*  SW_RST_R : SW_RST_ALL          */
#define SW_RST_R__SW_RST_ALL__SHIFT                                     0x00000000
#define SW_RST_R__SW_RST_ALL__WIDTH                                     0x00000001
#define SW_RST_R__SW_RST_ALL__MASK                                      0x00000001


/*  SW_RST_R : SW_RST_CMD          */
#define SW_RST_R__SW_RST_CMD__SHIFT                                     0x00000001
#define SW_RST_R__SW_RST_CMD__WIDTH                                     0x00000001
#define SW_RST_R__SW_RST_CMD__MASK                                      0x00000002


/*  SW_RST_R : SW_RST_DAT          */
#define SW_RST_R__SW_RST_DAT__SHIFT                                     0x00000002
#define SW_RST_R__SW_RST_DAT__WIDTH                                     0x00000001
#define SW_RST_R__SW_RST_DAT__MASK                                      0x00000004


/*  SW_RST_R : RSVD_7_3            */
#define SW_RST_R__RSVD_7_3__SHIFT                                       0x00000003
#define SW_RST_R__RSVD_7_3__WIDTH                                       0x00000005
#define SW_RST_R__RSVD_7_3__MASK                                        0x000000F8


/*-------------------------------------------------------------------------------------*/
/*  NORMAL_INT_STAT_R : CMD_COMPLETE        */
#define NORMAL_INT_STAT_R__CMD_COMPLETE__SHIFT                          0x00000000
#define NORMAL_INT_STAT_R__CMD_COMPLETE__WIDTH                          0x00000001
#define NORMAL_INT_STAT_R__CMD_COMPLETE__MASK                           0x00000001


/*  NORMAL_INT_STAT_R : XFER_COMPLETE       */
#define NORMAL_INT_STAT_R__XFER_COMPLETE__SHIFT                         0x00000001
#define NORMAL_INT_STAT_R__XFER_COMPLETE__WIDTH                         0x00000001
#define NORMAL_INT_STAT_R__XFER_COMPLETE__MASK                          0x00000002


/*  NORMAL_INT_STAT_R : BGAP_EVENT          */
#define NORMAL_INT_STAT_R__BGAP_EVENT__SHIFT                            0x00000002
#define NORMAL_INT_STAT_R__BGAP_EVENT__WIDTH                            0x00000001
#define NORMAL_INT_STAT_R__BGAP_EVENT__MASK                             0x00000004


/*  NORMAL_INT_STAT_R : DMA_INTERRUPT       */
#define NORMAL_INT_STAT_R__DMA_INTERRUPT__SHIFT                         0x00000003
#define NORMAL_INT_STAT_R__DMA_INTERRUPT__WIDTH                         0x00000001
#define NORMAL_INT_STAT_R__DMA_INTERRUPT__MASK                          0x00000008


/*  NORMAL_INT_STAT_R : BUF_WR_READY        */
#define NORMAL_INT_STAT_R__BUF_WR_READY__SHIFT                          0x00000004
#define NORMAL_INT_STAT_R__BUF_WR_READY__WIDTH                          0x00000001
#define NORMAL_INT_STAT_R__BUF_WR_READY__MASK                           0x00000010


/*  NORMAL_INT_STAT_R : BUF_RD_READY        */
#define NORMAL_INT_STAT_R__BUF_RD_READY__SHIFT                          0x00000005
#define NORMAL_INT_STAT_R__BUF_RD_READY__WIDTH                          0x00000001
#define NORMAL_INT_STAT_R__BUF_RD_READY__MASK                           0x00000020


/*  NORMAL_INT_STAT_R : CARD_INSERTION      */
#define NORMAL_INT_STAT_R__CARD_INSERTION__SHIFT                        0x00000006
#define NORMAL_INT_STAT_R__CARD_INSERTION__WIDTH                        0x00000001
#define NORMAL_INT_STAT_R__CARD_INSERTION__MASK                         0x00000040


/*  NORMAL_INT_STAT_R : CARD_REMOVAL        */
#define NORMAL_INT_STAT_R__CARD_REMOVAL__SHIFT                          0x00000007
#define NORMAL_INT_STAT_R__CARD_REMOVAL__WIDTH                          0x00000001
#define NORMAL_INT_STAT_R__CARD_REMOVAL__MASK                           0x00000080


/*  NORMAL_INT_STAT_R : CARD_INTERRUPT      */
#define NORMAL_INT_STAT_R__CARD_INTERRUPT__SHIFT                        0x00000008
#define NORMAL_INT_STAT_R__CARD_INTERRUPT__WIDTH                        0x00000001
#define NORMAL_INT_STAT_R__CARD_INTERRUPT__MASK                         0x00000100


/*  NORMAL_INT_STAT_R : INT_A               */
#define NORMAL_INT_STAT_R__INT_A__SHIFT                                 0x00000009
#define NORMAL_INT_STAT_R__INT_A__WIDTH                                 0x00000001
#define NORMAL_INT_STAT_R__INT_A__MASK                                  0x00000200

/*  NORMAL_INT_STAT_R : INT_B               */
#define NORMAL_INT_STAT_R__INT_B__SHIFT                                 0x0000000A
#define NORMAL_INT_STAT_R__INT_B__WIDTH                                 0x00000001
#define NORMAL_INT_STAT_R__INT_B__MASK                                  0x00000400

/*  NORMAL_INT_STAT_R : INT_C               */
#define NORMAL_INT_STAT_R__INT_C__SHIFT                                 0x0000000B
#define NORMAL_INT_STAT_R__INT_C__WIDTH                                 0x00000001
#define NORMAL_INT_STAT_R__INT_C__MASK                                  0x00000800

/*  NORMAL_INT_STAT_R : RE_TUNE_EVENT       */
#define NORMAL_INT_STAT_R__RE_TUNE_EVENT__SHIFT                         0x0000000C
#define NORMAL_INT_STAT_R__RE_TUNE_EVENT__WIDTH                         0x00000001
#define NORMAL_INT_STAT_R__RE_TUNE_EVENT__MASK                          0x00001000

/*  NORMAL_INT_STAT_R : FX_EVENT            */
#define NORMAL_INT_STAT_R__FX_EVENT__SHIFT                              0x0000000D
#define NORMAL_INT_STAT_R__FX_EVENT__WIDTH                              0x00000001
#define NORMAL_INT_STAT_R__FX_EVENT__MASK                               0x00002000


/*  NORMAL_INT_STAT_R : CQE_EVENT           */
#define NORMAL_INT_STAT_R__CQE_EVENT__SHIFT                             0x0000000E
#define NORMAL_INT_STAT_R__CQE_EVENT__WIDTH                             0x00000001
#define NORMAL_INT_STAT_R__CQE_EVENT__MASK                              0x00004000


/*  NORMAL_INT_STAT_R : ERR_INTERRUPT       */
#define NORMAL_INT_STAT_R__ERR_INTERRUPT__SHIFT                         0x0000000F
#define NORMAL_INT_STAT_R__ERR_INTERRUPT__WIDTH                         0x00000001
#define NORMAL_INT_STAT_R__ERR_INTERRUPT__MASK                          0x00008000



/*-------------------------------------------------------------------------------------*/
/*  ERROR_INT_STAT_R : CMD_TOUT_ERR        */
#define ERROR_INT_STAT_R__CMD_TOUT_ERR__SHIFT                           0x00000000
#define ERROR_INT_STAT_R__CMD_TOUT_ERR__WIDTH                           0x00000001
#define ERROR_INT_STAT_R__CMD_TOUT_ERR__MASK                            0x00000001


/*  ERROR_INT_STAT_R : CMD_CRC_ERR         */
#define ERROR_INT_STAT_R__CMD_CRC_ERR__SHIFT                            0x00000001
#define ERROR_INT_STAT_R__CMD_CRC_ERR__WIDTH                            0x00000001
#define ERROR_INT_STAT_R__CMD_CRC_ERR__MASK                             0x00000002


/*  ERROR_INT_STAT_R : CMD_END_BIT_ERR     */
#define ERROR_INT_STAT_R__CMD_END_BIT_ERR__SHIFT                        0x00000002
#define ERROR_INT_STAT_R__CMD_END_BIT_ERR__WIDTH                        0x00000001
#define ERROR_INT_STAT_R__CMD_END_BIT_ERR__MASK                         0x00000004


/*  ERROR_INT_STAT_R : CMD_IDX_ERR         */
#define ERROR_INT_STAT_R__CMD_IDX_ERR__SHIFT                            0x00000003
#define ERROR_INT_STAT_R__CMD_IDX_ERR__WIDTH                            0x00000001
#define ERROR_INT_STAT_R__CMD_IDX_ERR__MASK                             0x00000008


/*  ERROR_INT_STAT_R : DATA_TOUT_ERR       */
#define ERROR_INT_STAT_R__DATA_TOUT_ERR__SHIFT                          0x00000004
#define ERROR_INT_STAT_R__DATA_TOUT_ERR__WIDTH                          0x00000001
#define ERROR_INT_STAT_R__DATA_TOUT_ERR__MASK                           0x00000010


/*  ERROR_INT_STAT_R : DATA_CRC_ERR        */
#define ERROR_INT_STAT_R__DATA_CRC_ERR__SHIFT                           0x00000005
#define ERROR_INT_STAT_R__DATA_CRC_ERR__WIDTH                           0x00000001
#define ERROR_INT_STAT_R__DATA_CRC_ERR__MASK                            0x00000020


/*  ERROR_INT_STAT_R : DATA_END_BIT_ERR    */
#define ERROR_INT_STAT_R__DATA_END_BIT_ERR__SHIFT                       0x00000006
#define ERROR_INT_STAT_R__DATA_END_BIT_ERR__WIDTH                       0x00000001
#define ERROR_INT_STAT_R__DATA_END_BIT_ERR__MASK                        0x00000040


/*  ERROR_INT_STAT_R : CUR_LMT_ERR         */
#define ERROR_INT_STAT_R__CUR_LMT_ERR__SHIFT                            0x00000007
#define ERROR_INT_STAT_R__CUR_LMT_ERR__WIDTH                            0x00000001
#define ERROR_INT_STAT_R__CUR_LMT_ERR__MASK                             0x00000080


/*  ERROR_INT_STAT_R : AUTO_CMD_ERR        */
#define ERROR_INT_STAT_R__AUTO_CMD_ERR__SHIFT                           0x00000008
#define ERROR_INT_STAT_R__AUTO_CMD_ERR__WIDTH                           0x00000001
#define ERROR_INT_STAT_R__AUTO_CMD_ERR__MASK                            0x00000100


/*  ERROR_INT_STAT_R : ADMA_ERR            */
#define ERROR_INT_STAT_R__ADMA_ERR__SHIFT                               0x00000009
#define ERROR_INT_STAT_R__ADMA_ERR__WIDTH                               0x00000001
#define ERROR_INT_STAT_R__ADMA_ERR__MASK                                0x00000200


/*  ERROR_INT_STAT_R : TUNING_ERR          */
#define ERROR_INT_STAT_R__TUNING_ERR__SHIFT                             0x0000000A
#define ERROR_INT_STAT_R__TUNING_ERR__WIDTH                             0x00000001
#define ERROR_INT_STAT_R__TUNING_ERR__MASK                              0x00000400


/*  ERROR_INT_STAT_R : RESP_ERR            */
#define ERROR_INT_STAT_R__RESP_ERR__SHIFT                               0x0000000B
#define ERROR_INT_STAT_R__RESP_ERR__WIDTH                               0x00000001
#define ERROR_INT_STAT_R__RESP_ERR__MASK                                0x00000800


/*  ERROR_INT_STAT_R : BOOT_ACK_ERR        */
#define ERROR_INT_STAT_R__BOOT_ACK_ERR__SHIFT                           0x0000000C
#define ERROR_INT_STAT_R__BOOT_ACK_ERR__WIDTH                           0x00000001
#define ERROR_INT_STAT_R__BOOT_ACK_ERR__MASK                            0x00001000


/*  ERROR_INT_STAT_R : VENDOR_ERR1         */
#define ERROR_INT_STAT_R__VENDOR_ERR1__SHIFT                            0x0000000D
#define ERROR_INT_STAT_R__VENDOR_ERR1__WIDTH                            0x00000001
#define ERROR_INT_STAT_R__VENDOR_ERR1__MASK                             0x00002000

/*  ERROR_INT_STAT_R : VENDOR_ERR2         */
#define ERROR_INT_STAT_R__VENDOR_ERR2__SHIFT                            0x0000000E
#define ERROR_INT_STAT_R__VENDOR_ERR2__WIDTH                            0x00000001
#define ERROR_INT_STAT_R__VENDOR_ERR2__MASK                             0x00004000

/*  ERROR_INT_STAT_R : VENDOR_ERR3         */
#define ERROR_INT_STAT_R__VENDOR_ERR3__SHIFT                            0x0000000F
#define ERROR_INT_STAT_R__VENDOR_ERR3__WIDTH                            0x00000001
#define ERROR_INT_STAT_R__VENDOR_ERR3__MASK                             0x00008000


/*-------------------------------------------------------------------------------------*/
/*  NORMAL_INT_STAT_EN_R : CMD_COMPLETE_STAT_EN*/
#define NORMAL_INT_STAT_EN_R__CMD_COMPLETE_STAT_EN__SHIFT               0x00000000
#define NORMAL_INT_STAT_EN_R__CMD_COMPLETE_STAT_EN__WIDTH               0x00000001
#define NORMAL_INT_STAT_EN_R__CMD_COMPLETE_STAT_EN__MASK                0x00000001


/*  NORMAL_INT_STAT_EN_R : XFER_COMPLETE_STAT_EN*/
#define NORMAL_INT_STAT_EN_R__XFER_COMPLETE_STAT_EN__SHIFT              0x00000001
#define NORMAL_INT_STAT_EN_R__XFER_COMPLETE_STAT_EN__WIDTH              0x00000001
#define NORMAL_INT_STAT_EN_R__XFER_COMPLETE_STAT_EN__MASK               0x00000002


/*  NORMAL_INT_STAT_EN_R : BGAP_EVENT_STAT_EN  */
#define NORMAL_INT_STAT_EN_R__BGAP_EVENT_STAT_EN__SHIFT                 0x00000002
#define NORMAL_INT_STAT_EN_R__BGAP_EVENT_STAT_EN__WIDTH                 0x00000001
#define NORMAL_INT_STAT_EN_R__BGAP_EVENT_STAT_EN__MASK                  0x00000004


/*  NORMAL_INT_STAT_EN_R : DMA_INTERRUPT_STAT_EN*/
#define NORMAL_INT_STAT_EN_R__DMA_INTERRUPT_STAT_EN__SHIFT              0x00000003
#define NORMAL_INT_STAT_EN_R__DMA_INTERRUPT_STAT_EN__WIDTH              0x00000001
#define NORMAL_INT_STAT_EN_R__DMA_INTERRUPT_STAT_EN__MASK               0x00000008


/*  NORMAL_INT_STAT_EN_R : BUF_WR_READY_STAT_EN*/
#define NORMAL_INT_STAT_EN_R__BUF_WR_READY_STAT_EN__SHIFT               0x00000004
#define NORMAL_INT_STAT_EN_R__BUF_WR_READY_STAT_EN__WIDTH               0x00000001
#define NORMAL_INT_STAT_EN_R__BUF_WR_READY_STAT_EN__MASK                0x00000010


/*  NORMAL_INT_STAT_EN_R : BUF_RD_READY_STAT_EN*/
#define NORMAL_INT_STAT_EN_R__BUF_RD_READY_STAT_EN__SHIFT               0x00000005
#define NORMAL_INT_STAT_EN_R__BUF_RD_READY_STAT_EN__WIDTH               0x00000001
#define NORMAL_INT_STAT_EN_R__BUF_RD_READY_STAT_EN__MASK                0x00000020


/*  NORMAL_INT_STAT_EN_R : CARD_INSERTION_STAT_EN*/
#define NORMAL_INT_STAT_EN_R__CARD_INSERTION_STAT_EN__SHIFT             0x00000006
#define NORMAL_INT_STAT_EN_R__CARD_INSERTION_STAT_EN__WIDTH             0x00000001
#define NORMAL_INT_STAT_EN_R__CARD_INSERTION_STAT_EN__MASK              0x00000040


/*  NORMAL_INT_STAT_EN_R : CARD_REMOVAL_STAT_EN*/
#define NORMAL_INT_STAT_EN_R__CARD_REMOVAL_STAT_EN__SHIFT               0x00000007
#define NORMAL_INT_STAT_EN_R__CARD_REMOVAL_STAT_EN__WIDTH               0x00000001
#define NORMAL_INT_STAT_EN_R__CARD_REMOVAL_STAT_EN__MASK                0x00000080


/*  NORMAL_INT_STAT_EN_R : CARD_INTERRUPT_STAT_EN*/
#define NORMAL_INT_STAT_EN_R__CARD_INTERRUPT_STAT_EN__SHIFT             0x00000008
#define NORMAL_INT_STAT_EN_R__CARD_INTERRUPT_STAT_EN__WIDTH             0x00000001
#define NORMAL_INT_STAT_EN_R__CARD_INTERRUPT_STAT_EN__MASK              0x00000100


/*  NORMAL_INT_STAT_EN_R : INT_A_STAT_EN       */
#define NORMAL_INT_STAT_EN_R__INT_A_STAT_EN__SHIFT                      0x00000009
#define NORMAL_INT_STAT_EN_R__INT_A_STAT_EN__WIDTH                      0x00000001
#define NORMAL_INT_STAT_EN_R__INT_A_STAT_EN__MASK                       0x00000200


/*  NORMAL_INT_STAT_EN_R : INT_B_STAT_EN       */
#define NORMAL_INT_STAT_EN_R__INT_B_STAT_EN__SHIFT                      0x0000000A
#define NORMAL_INT_STAT_EN_R__INT_B_STAT_EN__WIDTH                      0x00000001
#define NORMAL_INT_STAT_EN_R__INT_B_STAT_EN__MASK                       0x00000400


/*  NORMAL_INT_STAT_EN_R : INT_C_STAT_EN       */
#define NORMAL_INT_STAT_EN_R__INT_C_STAT_EN__SHIFT                      0x0000000B
#define NORMAL_INT_STAT_EN_R__INT_C_STAT_EN__WIDTH                      0x00000001
#define NORMAL_INT_STAT_EN_R__INT_C_STAT_EN__MASK                       0x00000800


/*  NORMAL_INT_STAT_EN_R : RE_TUNE_EVENT_STAT_EN*/
#define NORMAL_INT_STAT_EN_R__RE_TUNE_EVENT_STAT_EN__SHIFT              0x0000000C
#define NORMAL_INT_STAT_EN_R__RE_TUNE_EVENT_STAT_EN__WIDTH              0x00000001
#define NORMAL_INT_STAT_EN_R__RE_TUNE_EVENT_STAT_EN__MASK               0x00001000


/*  NORMAL_INT_STAT_EN_R : FX_EVENT_STAT_EN    */
#define NORMAL_INT_STAT_EN_R__FX_EVENT_STAT_EN__SHIFT                   0x0000000D
#define NORMAL_INT_STAT_EN_R__FX_EVENT_STAT_EN__WIDTH                   0x00000001
#define NORMAL_INT_STAT_EN_R__FX_EVENT_STAT_EN__MASK                    0x00002000


/*  NORMAL_INT_STAT_EN_R : CQE_EVENT_STAT_EN   */
#define NORMAL_INT_STAT_EN_R__CQE_EVENT_STAT_EN__SHIFT                  0x0000000E
#define NORMAL_INT_STAT_EN_R__CQE_EVENT_STAT_EN__WIDTH                  0x00000001
#define NORMAL_INT_STAT_EN_R__CQE_EVENT_STAT_EN__MASK                   0x00004000


/*  NORMAL_INT_STAT_EN_R : RSVD_15             */
#define NORMAL_INT_STAT_EN_R__RSVD_15__SHIFT                            0x0000000F
#define NORMAL_INT_STAT_EN_R__RSVD_15__WIDTH                            0x00000001
#define NORMAL_INT_STAT_EN_R__RSVD_15__MASK                             0x00008000


/*-------------------------------------------------------------------------------------*/
/*  ERROR_INT_STAT_EN_R : CMD_TOUT_ERR_STAT_EN*/
#define ERROR_INT_STAT_EN_R__CMD_TOUT_ERR_STAT_EN__SHIFT                0x00000000
#define ERROR_INT_STAT_EN_R__CMD_TOUT_ERR_STAT_EN__WIDTH                0x00000001
#define ERROR_INT_STAT_EN_R__CMD_TOUT_ERR_STAT_EN__MASK                 0x00000001


/*  ERROR_INT_STAT_EN_R : CMD_CRC_ERR_STAT_EN */
#define ERROR_INT_STAT_EN_R__CMD_CRC_ERR_STAT_EN__SHIFT                 0x00000001
#define ERROR_INT_STAT_EN_R__CMD_CRC_ERR_STAT_EN__WIDTH                 0x00000001
#define ERROR_INT_STAT_EN_R__CMD_CRC_ERR_STAT_EN__MASK                  0x00000002


/*  ERROR_INT_STAT_EN_R : CMD_END_BIT_ERR_STAT_EN*/
#define ERROR_INT_STAT_EN_R__CMD_END_BIT_ERR_STAT_EN__SHIFT             0x00000002
#define ERROR_INT_STAT_EN_R__CMD_END_BIT_ERR_STAT_EN__WIDTH             0x00000001
#define ERROR_INT_STAT_EN_R__CMD_END_BIT_ERR_STAT_EN__MASK              0x00000004


/*  ERROR_INT_STAT_EN_R : CMD_IDX_ERR_STAT_EN */
#define ERROR_INT_STAT_EN_R__CMD_IDX_ERR_STAT_EN__SHIFT                 0x00000003
#define ERROR_INT_STAT_EN_R__CMD_IDX_ERR_STAT_EN__WIDTH                 0x00000001
#define ERROR_INT_STAT_EN_R__CMD_IDX_ERR_STAT_EN__MASK                  0x00000008


/*  ERROR_INT_STAT_EN_R : DATA_TOUT_ERR_STAT_EN*/
#define ERROR_INT_STAT_EN_R__DATA_TOUT_ERR_STAT_EN__SHIFT               0x00000004
#define ERROR_INT_STAT_EN_R__DATA_TOUT_ERR_STAT_EN__WIDTH               0x00000001
#define ERROR_INT_STAT_EN_R__DATA_TOUT_ERR_STAT_EN__MASK                0x00000010


/*  ERROR_INT_STAT_EN_R : DATA_CRC_ERR_STAT_EN*/
#define ERROR_INT_STAT_EN_R__DATA_CRC_ERR_STAT_EN__SHIFT                0x00000005
#define ERROR_INT_STAT_EN_R__DATA_CRC_ERR_STAT_EN__WIDTH                0x00000001
#define ERROR_INT_STAT_EN_R__DATA_CRC_ERR_STAT_EN__MASK                 0x00000020


/*  ERROR_INT_STAT_EN_R : DATA_END_BIT_ERR_STAT_EN*/
#define ERROR_INT_STAT_EN_R__DATA_END_BIT_ERR_STAT_EN__SHIFT            0x00000006
#define ERROR_INT_STAT_EN_R__DATA_END_BIT_ERR_STAT_EN__WIDTH            0x00000001
#define ERROR_INT_STAT_EN_R__DATA_END_BIT_ERR_STAT_EN__MASK             0x00000040


/*  ERROR_INT_STAT_EN_R : CUR_LMT_ERR_STAT_EN */
#define ERROR_INT_STAT_EN_R__CUR_LMT_ERR_STAT_EN__SHIFT                 0x00000007
#define ERROR_INT_STAT_EN_R__CUR_LMT_ERR_STAT_EN__WIDTH                 0x00000001
#define ERROR_INT_STAT_EN_R__CUR_LMT_ERR_STAT_EN__MASK                  0x00000080


/*  ERROR_INT_STAT_EN_R : AUTO_CMD_ERR_STAT_EN*/
#define ERROR_INT_STAT_EN_R__AUTO_CMD_ERR_STAT_EN__SHIFT                0x00000008
#define ERROR_INT_STAT_EN_R__AUTO_CMD_ERR_STAT_EN__WIDTH                0x00000001
#define ERROR_INT_STAT_EN_R__AUTO_CMD_ERR_STAT_EN__MASK                 0x00000100


/*  ERROR_INT_STAT_EN_R : ADMA_ERR_STAT_EN    */
#define ERROR_INT_STAT_EN_R__ADMA_ERR_STAT_EN__SHIFT                    0x00000009
#define ERROR_INT_STAT_EN_R__ADMA_ERR_STAT_EN__WIDTH                    0x00000001
#define ERROR_INT_STAT_EN_R__ADMA_ERR_STAT_EN__MASK                     0x00000200


/*  ERROR_INT_STAT_EN_R : TUNING_ERR_STAT_EN  */
#define ERROR_INT_STAT_EN_R__TUNING_ERR_STAT_EN__SHIFT                  0x0000000A
#define ERROR_INT_STAT_EN_R__TUNING_ERR_STAT_EN__WIDTH                  0x00000001
#define ERROR_INT_STAT_EN_R__TUNING_ERR_STAT_EN__MASK                   0x00000400


/*  ERROR_INT_STAT_EN_R : RESP_ERR_STAT_EN    */
#define ERROR_INT_STAT_EN_R__RESP_ERR_STAT_EN__SHIFT                    0x0000000B
#define ERROR_INT_STAT_EN_R__RESP_ERR_STAT_EN__WIDTH                    0x00000001
#define ERROR_INT_STAT_EN_R__RESP_ERR_STAT_EN__MASK                     0x00000800


/*  ERROR_INT_STAT_EN_R : BOOT_ACK_ERR_STAT_EN*/
#define ERROR_INT_STAT_EN_R__BOOT_ACK_ERR_STAT_EN__SHIFT                0x0000000C
#define ERROR_INT_STAT_EN_R__BOOT_ACK_ERR_STAT_EN__WIDTH                0x00000001
#define ERROR_INT_STAT_EN_R__BOOT_ACK_ERR_STAT_EN__MASK                 0x00001000


/*  ERROR_INT_STAT_EN_R : VENDOR_ERR_STAT_EN1 */
#define ERROR_INT_STAT_EN_R__VENDOR_ERR_STAT_EN1__SHIFT                 0x0000000D
#define ERROR_INT_STAT_EN_R__VENDOR_ERR_STAT_EN1__WIDTH                 0x00000001
#define ERROR_INT_STAT_EN_R__VENDOR_ERR_STAT_EN1__MASK                  0x00002000


/*  ERROR_INT_STAT_EN_R : VENDOR_ERR_STAT_EN2 */
#define ERROR_INT_STAT_EN_R__VENDOR_ERR_STAT_EN2__SHIFT                 0x0000000E
#define ERROR_INT_STAT_EN_R__VENDOR_ERR_STAT_EN2__WIDTH                 0x00000001
#define ERROR_INT_STAT_EN_R__VENDOR_ERR_STAT_EN2__MASK                  0x00004000


/*  ERROR_INT_STAT_EN_R : VENDOR_ERR_STAT_EN3 */
#define ERROR_INT_STAT_EN_R__VENDOR_ERR_STAT_EN3__SHIFT                 0x0000000F
#define ERROR_INT_STAT_EN_R__VENDOR_ERR_STAT_EN3__WIDTH                 0x00000001
#define ERROR_INT_STAT_EN_R__VENDOR_ERR_STAT_EN3__MASK                  0x00008000



/*-------------------------------------------------------------------------------------*/
/*  NORMAL_INT_SIGNAL_EN_R : CMD_COMPLETE_SIGNAL_EN*/
#define NORMAL_INT_SIGNAL_EN_R__CMD_COMPLETE_SIGNAL_EN__SHIFT           0x00000000
#define NORMAL_INT_SIGNAL_EN_R__CMD_COMPLETE_SIGNAL_EN__WIDTH           0x00000001
#define NORMAL_INT_SIGNAL_EN_R__CMD_COMPLETE_SIGNAL_EN__MASK            0x00000001


/*  NORMAL_INT_SIGNAL_EN_R : XFER_COMPLETE_SIGNAL_EN*/
#define NORMAL_INT_SIGNAL_EN_R__XFER_COMPLETE_SIGNAL_EN__SHIFT          0x00000001
#define NORMAL_INT_SIGNAL_EN_R__XFER_COMPLETE_SIGNAL_EN__WIDTH          0x00000001
#define NORMAL_INT_SIGNAL_EN_R__XFER_COMPLETE_SIGNAL_EN__MASK           0x00000002


/*  NORMAL_INT_SIGNAL_EN_R : BGAP_EVENT_SIGNAL_EN*/
#define NORMAL_INT_SIGNAL_EN_R__BGAP_EVENT_SIGNAL_EN__SHIFT             0x00000002
#define NORMAL_INT_SIGNAL_EN_R__BGAP_EVENT_SIGNAL_EN__WIDTH             0x00000001
#define NORMAL_INT_SIGNAL_EN_R__BGAP_EVENT_SIGNAL_EN__MASK              0x00000004


/*  NORMAL_INT_SIGNAL_EN_R : DMA_INTERRUPT_SIGNAL_EN*/
#define NORMAL_INT_SIGNAL_EN_R__DMA_INTERRUPT_SIGNAL_EN__SHIFT          0x00000003
#define NORMAL_INT_SIGNAL_EN_R__DMA_INTERRUPT_SIGNAL_EN__WIDTH          0x00000001
#define NORMAL_INT_SIGNAL_EN_R__DMA_INTERRUPT_SIGNAL_EN__MASK           0x00000008


/*  NORMAL_INT_SIGNAL_EN_R : BUF_WR_READY_SIGNAL_EN*/
#define NORMAL_INT_SIGNAL_EN_R__BUF_WR_READY_SIGNAL_EN__SHIFT           0x00000004
#define NORMAL_INT_SIGNAL_EN_R__BUF_WR_READY_SIGNAL_EN__WIDTH           0x00000001
#define NORMAL_INT_SIGNAL_EN_R__BUF_WR_READY_SIGNAL_EN__MASK            0x00000010


/*  NORMAL_INT_SIGNAL_EN_R : BUF_RD_READY_SIGNAL_EN*/
#define NORMAL_INT_SIGNAL_EN_R__BUF_RD_READY_SIGNAL_EN__SHIFT           0x00000005
#define NORMAL_INT_SIGNAL_EN_R__BUF_RD_READY_SIGNAL_EN__WIDTH           0x00000001
#define NORMAL_INT_SIGNAL_EN_R__BUF_RD_READY_SIGNAL_EN__MASK            0x00000020


/*  NORMAL_INT_SIGNAL_EN_R : CARD_INSERTION_SIGNAL_EN*/
#define NORMAL_INT_SIGNAL_EN_R__CARD_INSERTION_SIGNAL_EN__SHIFT         0x00000006
#define NORMAL_INT_SIGNAL_EN_R__CARD_INSERTION_SIGNAL_EN__WIDTH         0x00000001
#define NORMAL_INT_SIGNAL_EN_R__CARD_INSERTION_SIGNAL_EN__MASK          0x00000040


/*  NORMAL_INT_SIGNAL_EN_R : CARD_REMOVAL_SIGNAL_EN*/
#define NORMAL_INT_SIGNAL_EN_R__CARD_REMOVAL_SIGNAL_EN__SHIFT           0x00000007
#define NORMAL_INT_SIGNAL_EN_R__CARD_REMOVAL_SIGNAL_EN__WIDTH           0x00000001
#define NORMAL_INT_SIGNAL_EN_R__CARD_REMOVAL_SIGNAL_EN__MASK            0x00000080


/*  NORMAL_INT_SIGNAL_EN_R : CARD_INTERRUPT_SIGNAL_EN*/
#define NORMAL_INT_SIGNAL_EN_R__CARD_INTERRUPT_SIGNAL_EN__SHIFT         0x00000008
#define NORMAL_INT_SIGNAL_EN_R__CARD_INTERRUPT_SIGNAL_EN__WIDTH         0x00000001
#define NORMAL_INT_SIGNAL_EN_R__CARD_INTERRUPT_SIGNAL_EN__MASK          0x00000100


/*  NORMAL_INT_SIGNAL_EN_R : INT_A_SIGNAL_EN     */
#define NORMAL_INT_SIGNAL_EN_R__INT_A_SIGNAL_EN__SHIFT                  0x00000009
#define NORMAL_INT_SIGNAL_EN_R__INT_A_SIGNAL_EN__WIDTH                  0x00000001
#define NORMAL_INT_SIGNAL_EN_R__INT_A_SIGNAL_EN__MASK                   0x00000200


/*  NORMAL_INT_SIGNAL_EN_R : INT_B_SIGNAL_EN     */
#define NORMAL_INT_SIGNAL_EN_R__INT_B_SIGNAL_EN__SHIFT                  0x0000000A
#define NORMAL_INT_SIGNAL_EN_R__INT_B_SIGNAL_EN__WIDTH                  0x00000001
#define NORMAL_INT_SIGNAL_EN_R__INT_B_SIGNAL_EN__MASK                   0x00000400


/*  NORMAL_INT_SIGNAL_EN_R : INT_C_SIGNAL_EN     */
#define NORMAL_INT_SIGNAL_EN_R__INT_C_SIGNAL_EN__SHIFT                  0x0000000B
#define NORMAL_INT_SIGNAL_EN_R__INT_C_SIGNAL_EN__WIDTH                  0x00000001
#define NORMAL_INT_SIGNAL_EN_R__INT_C_SIGNAL_EN__MASK                   0x00000800


/*  NORMAL_INT_SIGNAL_EN_R : RE_TUNE_EVENT_SIGNAL_EN*/
#define NORMAL_INT_SIGNAL_EN_R__RE_TUNE_EVENT_SIGNAL_EN__SHIFT          0x0000000C
#define NORMAL_INT_SIGNAL_EN_R__RE_TUNE_EVENT_SIGNAL_EN__WIDTH          0x00000001
#define NORMAL_INT_SIGNAL_EN_R__RE_TUNE_EVENT_SIGNAL_EN__MASK           0x00001000


/*  NORMAL_INT_SIGNAL_EN_R : FX_EVENT_SIGNAL_EN  */
#define NORMAL_INT_SIGNAL_EN_R__FX_EVENT_SIGNAL_EN__SHIFT               0x0000000D
#define NORMAL_INT_SIGNAL_EN_R__FX_EVENT_SIGNAL_EN__WIDTH               0x00000001
#define NORMAL_INT_SIGNAL_EN_R__FX_EVENT_SIGNAL_EN__MASK                0x00002000


/*  NORMAL_INT_SIGNAL_EN_R : CQE_EVENT_SIGNAL_EN */
#define NORMAL_INT_SIGNAL_EN_R__CQE_EVENT_SIGNAL_EN__SHIFT              0x0000000E
#define NORMAL_INT_SIGNAL_EN_R__CQE_EVENT_SIGNAL_EN__WIDTH              0x00000001
#define NORMAL_INT_SIGNAL_EN_R__CQE_EVENT_SIGNAL_EN__MASK               0x00004000


/*  NORMAL_INT_SIGNAL_EN_R : RSVD_15             */
#define NORMAL_INT_SIGNAL_EN_R__RSVD_15__SHIFT                          0x0000000F
#define NORMAL_INT_SIGNAL_EN_R__RSVD_15__WIDTH                          0x00000001
#define NORMAL_INT_SIGNAL_EN_R__RSVD_15__MASK                           0x00008000


/*-------------------------------------------------------------------------------------*/
/*  ERROR_INT_SIGNAL_EN_R : CMD_TOUT_ERR_SIGNAL_EN*/
#define ERROR_INT_SIGNAL_EN_R__CMD_TOUT_ERR_SIGNAL_EN__SHIFT            0x00000000
#define ERROR_INT_SIGNAL_EN_R__CMD_TOUT_ERR_SIGNAL_EN__WIDTH            0x00000001
#define ERROR_INT_SIGNAL_EN_R__CMD_TOUT_ERR_SIGNAL_EN__MASK             0x00000001


/*  ERROR_INT_SIGNAL_EN_R : CMD_CRC_ERR_SIGNAL_EN*/
#define ERROR_INT_SIGNAL_EN_R__CMD_CRC_ERR_SIGNAL_EN__SHIFT             0x00000001
#define ERROR_INT_SIGNAL_EN_R__CMD_CRC_ERR_SIGNAL_EN__WIDTH             0x00000001
#define ERROR_INT_SIGNAL_EN_R__CMD_CRC_ERR_SIGNAL_EN__MASK              0x00000002


/*  ERROR_INT_SIGNAL_EN_R : CMD_END_BIT_ERR_SIGNAL_EN*/
#define ERROR_INT_SIGNAL_EN_R__CMD_END_BIT_ERR_SIGNAL_EN__SHIFT         0x00000002
#define ERROR_INT_SIGNAL_EN_R__CMD_END_BIT_ERR_SIGNAL_EN__WIDTH         0x00000001
#define ERROR_INT_SIGNAL_EN_R__CMD_END_BIT_ERR_SIGNAL_EN__MASK          0x00000004


/*  ERROR_INT_SIGNAL_EN_R : CMD_IDX_ERR_SIGNAL_EN*/
#define ERROR_INT_SIGNAL_EN_R__CMD_IDX_ERR_SIGNAL_EN__SHIFT             0x00000003
#define ERROR_INT_SIGNAL_EN_R__CMD_IDX_ERR_SIGNAL_EN__WIDTH             0x00000001
#define ERROR_INT_SIGNAL_EN_R__CMD_IDX_ERR_SIGNAL_EN__MASK              0x00000008


/*  ERROR_INT_SIGNAL_EN_R : DATA_TOUT_ERR_SIGNAL_EN*/
#define ERROR_INT_SIGNAL_EN_R__DATA_TOUT_ERR_SIGNAL_EN__SHIFT           0x00000004
#define ERROR_INT_SIGNAL_EN_R__DATA_TOUT_ERR_SIGNAL_EN__WIDTH           0x00000001
#define ERROR_INT_SIGNAL_EN_R__DATA_TOUT_ERR_SIGNAL_EN__MASK            0x00000010


/*  ERROR_INT_SIGNAL_EN_R : DATA_CRC_ERR_SIGNAL_EN*/
#define ERROR_INT_SIGNAL_EN_R__DATA_CRC_ERR_SIGNAL_EN__SHIFT            0x00000005
#define ERROR_INT_SIGNAL_EN_R__DATA_CRC_ERR_SIGNAL_EN__WIDTH            0x00000001
#define ERROR_INT_SIGNAL_EN_R__DATA_CRC_ERR_SIGNAL_EN__MASK             0x00000020


/*  ERROR_INT_SIGNAL_EN_R : DATA_END_BIT_ERR_SIGNAL_EN*/
#define ERROR_INT_SIGNAL_EN_R__DATA_END_BIT_ERR_SIGNAL_EN__SHIFT        0x00000006
#define ERROR_INT_SIGNAL_EN_R__DATA_END_BIT_ERR_SIGNAL_EN__WIDTH        0x00000001
#define ERROR_INT_SIGNAL_EN_R__DATA_END_BIT_ERR_SIGNAL_EN__MASK         0x00000040


/*  ERROR_INT_SIGNAL_EN_R : CUR_LMT_ERR_SIGNAL_EN*/
#define ERROR_INT_SIGNAL_EN_R__CUR_LMT_ERR_SIGNAL_EN__SHIFT             0x00000007
#define ERROR_INT_SIGNAL_EN_R__CUR_LMT_ERR_SIGNAL_EN__WIDTH             0x00000001
#define ERROR_INT_SIGNAL_EN_R__CUR_LMT_ERR_SIGNAL_EN__MASK              0x00000080


/*  ERROR_INT_SIGNAL_EN_R : AUTO_CMD_ERR_SIGNAL_EN*/
#define ERROR_INT_SIGNAL_EN_R__AUTO_CMD_ERR_SIGNAL_EN__SHIFT            0x00000008
#define ERROR_INT_SIGNAL_EN_R__AUTO_CMD_ERR_SIGNAL_EN__WIDTH            0x00000001
#define ERROR_INT_SIGNAL_EN_R__AUTO_CMD_ERR_SIGNAL_EN__MASK             0x00000100


/*  ERROR_INT_SIGNAL_EN_R : ADMA_ERR_SIGNAL_EN  */
#define ERROR_INT_SIGNAL_EN_R__ADMA_ERR_SIGNAL_EN__SHIFT                0x00000009
#define ERROR_INT_SIGNAL_EN_R__ADMA_ERR_SIGNAL_EN__WIDTH                0x00000001
#define ERROR_INT_SIGNAL_EN_R__ADMA_ERR_SIGNAL_EN__MASK                 0x00000200


/*  ERROR_INT_SIGNAL_EN_R : TUNING_ERR_SIGNAL_EN*/
#define ERROR_INT_SIGNAL_EN_R__TUNING_ERR_SIGNAL_EN__SHIFT              0x0000000A
#define ERROR_INT_SIGNAL_EN_R__TUNING_ERR_SIGNAL_EN__WIDTH              0x00000001
#define ERROR_INT_SIGNAL_EN_R__TUNING_ERR_SIGNAL_EN__MASK               0x00000400


/*  ERROR_INT_SIGNAL_EN_R : RESP_ERR_SIGNAL_EN  */
#define ERROR_INT_SIGNAL_EN_R__RESP_ERR_SIGNAL_EN__SHIFT                0x0000000B
#define ERROR_INT_SIGNAL_EN_R__RESP_ERR_SIGNAL_EN__WIDTH                0x00000001
#define ERROR_INT_SIGNAL_EN_R__RESP_ERR_SIGNAL_EN__MASK                 0x00000800


/*  ERROR_INT_SIGNAL_EN_R : BOOT_ACK_ERR_SIGNAL_EN*/
#define ERROR_INT_SIGNAL_EN_R__BOOT_ACK_ERR_SIGNAL_EN__SHIFT            0x0000000C
#define ERROR_INT_SIGNAL_EN_R__BOOT_ACK_ERR_SIGNAL_EN__WIDTH            0x00000001
#define ERROR_INT_SIGNAL_EN_R__BOOT_ACK_ERR_SIGNAL_EN__MASK             0x00001000


/*  ERROR_INT_SIGNAL_EN_R : VENDOR_ERR_SIGNAL_EN1*/
#define ERROR_INT_SIGNAL_EN_R__VENDOR_ERR_SIGNAL_EN1__SHIFT             0x0000000D
#define ERROR_INT_SIGNAL_EN_R__VENDOR_ERR_SIGNAL_EN1__WIDTH             0x00000001
#define ERROR_INT_SIGNAL_EN_R__VENDOR_ERR_SIGNAL_EN1__MASK              0x00002000


/*  ERROR_INT_SIGNAL_EN_R : VENDOR_ERR_SIGNAL_EN2*/
#define ERROR_INT_SIGNAL_EN_R__VENDOR_ERR_SIGNAL_EN2__SHIFT             0x0000000E
#define ERROR_INT_SIGNAL_EN_R__VENDOR_ERR_SIGNAL_EN2__WIDTH             0x00000001
#define ERROR_INT_SIGNAL_EN_R__VENDOR_ERR_SIGNAL_EN2__MASK              0x00004000


/*  ERROR_INT_SIGNAL_EN_R : VENDOR_ERR_SIGNAL_EN3*/
#define ERROR_INT_SIGNAL_EN_R__VENDOR_ERR_SIGNAL_EN3__SHIFT             0x0000000F
#define ERROR_INT_SIGNAL_EN_R__VENDOR_ERR_SIGNAL_EN3__WIDTH             0x00000001
#define ERROR_INT_SIGNAL_EN_R__VENDOR_ERR_SIGNAL_EN3__MASK              0x00008000



/*-------------------------------------------------------------------------------------*/
/*  AUTO_CMD_STAT_R : AUTO_CMD12_NOT_EXEC */
#define AUTO_CMD_STAT_R__AUTO_CMD12_NOT_EXEC__SHIFT                     0x00000000
#define AUTO_CMD_STAT_R__AUTO_CMD12_NOT_EXEC__WIDTH                     0x00000001
#define AUTO_CMD_STAT_R__AUTO_CMD12_NOT_EXEC__MASK                      0x00000001


/*  AUTO_CMD_STAT_R : AUTO_CMD_TOUT_ERR   */
#define AUTO_CMD_STAT_R__AUTO_CMD_TOUT_ERR__SHIFT                       0x00000001
#define AUTO_CMD_STAT_R__AUTO_CMD_TOUT_ERR__WIDTH                       0x00000001
#define AUTO_CMD_STAT_R__AUTO_CMD_TOUT_ERR__MASK                        0x00000002


/*  AUTO_CMD_STAT_R : AUTO_CMD_CRC_ERR    */
#define AUTO_CMD_STAT_R__AUTO_CMD_CRC_ERR__SHIFT                        0x00000002
#define AUTO_CMD_STAT_R__AUTO_CMD_CRC_ERR__WIDTH                        0x00000001
#define AUTO_CMD_STAT_R__AUTO_CMD_CRC_ERR__MASK                         0x00000004


/*  AUTO_CMD_STAT_R : AUTO_CMD_EBIT_ERR   */
#define AUTO_CMD_STAT_R__AUTO_CMD_EBIT_ERR__SHIFT                       0x00000003
#define AUTO_CMD_STAT_R__AUTO_CMD_EBIT_ERR__WIDTH                       0x00000001
#define AUTO_CMD_STAT_R__AUTO_CMD_EBIT_ERR__MASK                        0x00000008


/*  AUTO_CMD_STAT_R : AUTO_CMD_IDX_ERR    */
#define AUTO_CMD_STAT_R__AUTO_CMD_IDX_ERR__SHIFT                        0x00000004
#define AUTO_CMD_STAT_R__AUTO_CMD_IDX_ERR__WIDTH                        0x00000001
#define AUTO_CMD_STAT_R__AUTO_CMD_IDX_ERR__MASK                         0x00000010


/*  AUTO_CMD_STAT_R : AUTO_CMD_RESP_ERR   */
#define AUTO_CMD_STAT_R__AUTO_CMD_RESP_ERR__SHIFT                       0x00000005
#define AUTO_CMD_STAT_R__AUTO_CMD_RESP_ERR__WIDTH                       0x00000001
#define AUTO_CMD_STAT_R__AUTO_CMD_RESP_ERR__MASK                        0x00000020


/*  AUTO_CMD_STAT_R : RSVD_6              */
#define AUTO_CMD_STAT_R__RSVD_6__SHIFT                                  0x00000006
#define AUTO_CMD_STAT_R__RSVD_6__WIDTH                                  0x00000001
#define AUTO_CMD_STAT_R__RSVD_6__MASK                                   0x00000040

/*  AUTO_CMD_STAT_R : CMD_NOT_ISSUED_AUTO_CMD12*/
#define AUTO_CMD_STAT_R__CMD_NOT_ISSUED_AUTO_CMD12__SHIFT               0x00000007
#define AUTO_CMD_STAT_R__CMD_NOT_ISSUED_AUTO_CMD12__WIDTH               0x00000001
#define AUTO_CMD_STAT_R__CMD_NOT_ISSUED_AUTO_CMD12__MASK                0x00000080


/*  AUTO_CMD_STAT_R : RSVD_15_8           */
#define AUTO_CMD_STAT_R__RSVD_15_8__SHIFT                               0x00000008
#define AUTO_CMD_STAT_R__RSVD_15_8__WIDTH                               0x00000008
#define AUTO_CMD_STAT_R__RSVD_15_8__MASK                                0x0000FF00


/*-------------------------------------------------------------------------------------*/
/*  HOST_CTRL2_R : UHS_MODE_SEL        */
#define HOST_CTRL2_R__UHS_MODE_SEL__SHIFT                               0x00000000
#define HOST_CTRL2_R__UHS_MODE_SEL__WIDTH                               0x00000003
#define HOST_CTRL2_R__UHS_MODE_SEL__MASK                                0x00000007


/*  HOST_CTRL2_R : SIGNALING_EN        */
#define HOST_CTRL2_R__SIGNALING_EN__SHIFT                               0x00000003
#define HOST_CTRL2_R__SIGNALING_EN__WIDTH                               0x00000001
#define HOST_CTRL2_R__SIGNALING_EN__MASK                                0x00000008


/*  HOST_CTRL2_R : DRV_STRENGTH_SEL    */
#define HOST_CTRL2_R__DRV_STRENGTH_SEL__SHIFT                           0x00000004
#define HOST_CTRL2_R__DRV_STRENGTH_SEL__WIDTH                           0x00000002
#define HOST_CTRL2_R__DRV_STRENGTH_SEL__MASK                            0x00000030


/*  HOST_CTRL2_R : EXEC_TUNING         */
#define HOST_CTRL2_R__EXEC_TUNING__SHIFT                                0x00000006
#define HOST_CTRL2_R__EXEC_TUNING__WIDTH                                0x00000001
#define HOST_CTRL2_R__EXEC_TUNING__MASK                                 0x00000040


/*  HOST_CTRL2_R : SAMPLE_CLK_SEL      */
#define HOST_CTRL2_R__SAMPLE_CLK_SEL__SHIFT                             0x00000007
#define HOST_CTRL2_R__SAMPLE_CLK_SEL__WIDTH                             0x00000001
#define HOST_CTRL2_R__SAMPLE_CLK_SEL__MASK                              0x00000080


/*  HOST_CTRL2_R : UHS2_IF_ENABLE      */
#define HOST_CTRL2_R__UHS2_IF_ENABLE__SHIFT                             0x00000008
#define HOST_CTRL2_R__UHS2_IF_ENABLE__WIDTH                             0x00000001
#define HOST_CTRL2_R__UHS2_IF_ENABLE__MASK                              0x00000100


/*  HOST_CTRL2_R : RSVD_9              */
#define HOST_CTRL2_R__RSVD_9__SHIFT                                     0x00000009
#define HOST_CTRL2_R__RSVD_9__WIDTH                                     0x00000001
#define HOST_CTRL2_R__RSVD_9__MASK                                      0x00000200

/*  HOST_CTRL2_R : ADMA2_LEN_MODE      */
#define HOST_CTRL2_R__ADMA2_LEN_MODE__SHIFT                             0x0000000A
#define HOST_CTRL2_R__ADMA2_LEN_MODE__WIDTH                             0x00000001
#define HOST_CTRL2_R__ADMA2_LEN_MODE__MASK                              0x00000400


/*  HOST_CTRL2_R : CMD23_ENABLE        */
#define HOST_CTRL2_R__CMD23_ENABLE__SHIFT                               0x0000000B
#define HOST_CTRL2_R__CMD23_ENABLE__WIDTH                               0x00000001
#define HOST_CTRL2_R__CMD23_ENABLE__MASK                                0x00000800


/*  HOST_CTRL2_R : HOST_VER4_ENABLE    */
#define HOST_CTRL2_R__HOST_VER4_ENABLE__SHIFT                           0x0000000C
#define HOST_CTRL2_R__HOST_VER4_ENABLE__WIDTH                           0x00000001
#define HOST_CTRL2_R__HOST_VER4_ENABLE__MASK                            0x00001000


/*  HOST_CTRL2_R : ADDRESSING          */
#define HOST_CTRL2_R__ADDRESSING__SHIFT                                 0x0000000D
#define HOST_CTRL2_R__ADDRESSING__WIDTH                                 0x00000001
#define HOST_CTRL2_R__ADDRESSING__MASK                                  0x00002000


/*  HOST_CTRL2_R : ASYNC_INT_ENABLE    */
#define HOST_CTRL2_R__ASYNC_INT_ENABLE__SHIFT                           0x0000000E
#define HOST_CTRL2_R__ASYNC_INT_ENABLE__WIDTH                           0x00000001
#define HOST_CTRL2_R__ASYNC_INT_ENABLE__MASK                            0x00004000


/*  HOST_CTRL2_R : PRESET_VAL_ENABLE   */
#define HOST_CTRL2_R__PRESET_VAL_ENABLE__SHIFT                          0x0000000F
#define HOST_CTRL2_R__PRESET_VAL_ENABLE__WIDTH                          0x00000001
#define HOST_CTRL2_R__PRESET_VAL_ENABLE__MASK                           0x00008000



/*-------------------------------------------------------------------------------------*/
/*  CAPABILITIES1_R : TOUT_CLK_FREQ       */
#define CAPABILITIES1_R__TOUT_CLK_FREQ__SHIFT                           0x00000000
#define CAPABILITIES1_R__TOUT_CLK_FREQ__WIDTH                           0x00000006
#define CAPABILITIES1_R__TOUT_CLK_FREQ__MASK                            0x0000003F

/*  CAPABILITIES1_R : RSVD_6              */
#define CAPABILITIES1_R__RSVD_6__SHIFT                                  0x00000006
#define CAPABILITIES1_R__RSVD_6__WIDTH                                  0x00000001
#define CAPABILITIES1_R__RSVD_6__MASK                                   0x00000040

/*  CAPABILITIES1_R : TOUT_CLK_UNIT       */
#define CAPABILITIES1_R__TOUT_CLK_UNIT__SHIFT                           0x00000007
#define CAPABILITIES1_R__TOUT_CLK_UNIT__WIDTH                           0x00000001
#define CAPABILITIES1_R__TOUT_CLK_UNIT__MASK                            0x00000080


/*  CAPABILITIES1_R : BASE_CLK_FREQ       */
#define CAPABILITIES1_R__BASE_CLK_FREQ__SHIFT                           0x00000008
#define CAPABILITIES1_R__BASE_CLK_FREQ__WIDTH                           0x00000008
#define CAPABILITIES1_R__BASE_CLK_FREQ__MASK                            0x0000FF00

/*  CAPABILITIES1_R : MAX_BLK_LEN         */
#define CAPABILITIES1_R__MAX_BLK_LEN__SHIFT                             0x00000010
#define CAPABILITIES1_R__MAX_BLK_LEN__WIDTH                             0x00000002
#define CAPABILITIES1_R__MAX_BLK_LEN__MASK                              0x00030000


/*  CAPABILITIES1_R : Embedded_8_BIT      */
#define CAPABILITIES1_R__EMBEDDED_8_BIT__SHIFT                          0x00000012
#define CAPABILITIES1_R__EMBEDDED_8_BIT__WIDTH                          0x00000001
#define CAPABILITIES1_R__EMBEDDED_8_BIT__MASK                           0x00040000


/*  CAPABILITIES1_R : ADMA2_SUPPORT       */
#define CAPABILITIES1_R__ADMA2_SUPPORT__SHIFT                           0x00000013
#define CAPABILITIES1_R__ADMA2_SUPPORT__WIDTH                           0x00000001
#define CAPABILITIES1_R__ADMA2_SUPPORT__MASK                            0x00080000


/*  CAPABILITIES1_R : RSVD_20             */
#define CAPABILITIES1_R__RSVD_20__SHIFT                                 0x00000014
#define CAPABILITIES1_R__RSVD_20__WIDTH                                 0x00000001
#define CAPABILITIES1_R__RSVD_20__MASK                                  0x00100000

/*  CAPABILITIES1_R : HIGH_SPEED_SUPPORT  */
#define CAPABILITIES1_R__HIGH_SPEED_SUPPORT__SHIFT                      0x00000015
#define CAPABILITIES1_R__HIGH_SPEED_SUPPORT__WIDTH                      0x00000001
#define CAPABILITIES1_R__HIGH_SPEED_SUPPORT__MASK                       0x00200000


/*  CAPABILITIES1_R : SDMA_SUPPORT        */
#define CAPABILITIES1_R__SDMA_SUPPORT__SHIFT                            0x00000016
#define CAPABILITIES1_R__SDMA_SUPPORT__WIDTH                            0x00000001
#define CAPABILITIES1_R__SDMA_SUPPORT__MASK                             0x00400000


/*  CAPABILITIES1_R : SUS_RES_SUPPORT     */
#define CAPABILITIES1_R__SUS_RES_SUPPORT__SHIFT                         0x00000017
#define CAPABILITIES1_R__SUS_RES_SUPPORT__WIDTH                         0x00000001
#define CAPABILITIES1_R__SUS_RES_SUPPORT__MASK                          0x00800000


/*  CAPABILITIES1_R : VOLT_33             */
#define CAPABILITIES1_R__VOLT_33__SHIFT                                 0x00000018
#define CAPABILITIES1_R__VOLT_33__WIDTH                                 0x00000001
#define CAPABILITIES1_R__VOLT_33__MASK                                  0x01000000


/*  CAPABILITIES1_R : VOLT_30             */
#define CAPABILITIES1_R__VOLT_30__SHIFT                                 0x00000019
#define CAPABILITIES1_R__VOLT_30__WIDTH                                 0x00000001
#define CAPABILITIES1_R__VOLT_30__MASK                                  0x02000000


/*  CAPABILITIES1_R : VOLT_18             */
#define CAPABILITIES1_R__VOLT_18__SHIFT                                 0x0000001A
#define CAPABILITIES1_R__VOLT_18__WIDTH                                 0x00000001
#define CAPABILITIES1_R__VOLT_18__MASK                                  0x04000000


/*  CAPABILITIES1_R : SYS_ADDR_64_V4      */
#define CAPABILITIES1_R__SYS_ADDR_64_V4__SHIFT                          0x0000001B
#define CAPABILITIES1_R__SYS_ADDR_64_V4__WIDTH                          0x00000001
#define CAPABILITIES1_R__SYS_ADDR_64_V4__MASK                           0x08000000


/*  CAPABILITIES1_R : SYS_ADDR_64_V3      */
#define CAPABILITIES1_R__SYS_ADDR_64_V3__SHIFT                          0x0000001C
#define CAPABILITIES1_R__SYS_ADDR_64_V3__WIDTH                          0x00000001
#define CAPABILITIES1_R__SYS_ADDR_64_V3__MASK                           0x10000000


/*  CAPABILITIES1_R : ASYNC_INT_SUPPORT   */
#define CAPABILITIES1_R__ASYNC_INT_SUPPORT__SHIFT                       0x0000001D
#define CAPABILITIES1_R__ASYNC_INT_SUPPORT__WIDTH                       0x00000001
#define CAPABILITIES1_R__ASYNC_INT_SUPPORT__MASK                        0x20000000


/*  CAPABILITIES1_R : SLOT_TYPE_R         */
#define CAPABILITIES1_R__SLOT_TYPE_R__SHIFT                             0x0000001E
#define CAPABILITIES1_R__SLOT_TYPE_R__WIDTH                             0x00000002
#define CAPABILITIES1_R__SLOT_TYPE_R__MASK                              0xC0000000



/*-------------------------------------------------------------------------------------*/
/*  CAPABILITIES2_R : SDR50_SUPPORT       */
#define CAPABILITIES2_R__SDR50_SUPPORT__SHIFT                           0x00000000
#define CAPABILITIES2_R__SDR50_SUPPORT__WIDTH                           0x00000001
#define CAPABILITIES2_R__SDR50_SUPPORT__MASK                            0x00000001


/*  CAPABILITIES2_R : SDR104_SUPPORT      */
#define CAPABILITIES2_R__SDR104_SUPPORT__SHIFT                          0x00000001
#define CAPABILITIES2_R__SDR104_SUPPORT__WIDTH                          0x00000001
#define CAPABILITIES2_R__SDR104_SUPPORT__MASK                           0x00000002


/*  CAPABILITIES2_R : DDR50_SUPPORT       */
#define CAPABILITIES2_R__DDR50_SUPPORT__SHIFT                           0x00000002
#define CAPABILITIES2_R__DDR50_SUPPORT__WIDTH                           0x00000001
#define CAPABILITIES2_R__DDR50_SUPPORT__MASK                            0x00000004


/*  CAPABILITIES2_R : UHS2_SUPPORT        */
#define CAPABILITIES2_R__UHS2_SUPPORT__SHIFT                            0x00000003
#define CAPABILITIES2_R__UHS2_SUPPORT__WIDTH                            0x00000001
#define CAPABILITIES2_R__UHS2_SUPPORT__MASK                             0x00000008


/*  CAPABILITIES2_R : DRV_TYPEA           */
#define CAPABILITIES2_R__DRV_TYPEA__SHIFT                               0x00000004
#define CAPABILITIES2_R__DRV_TYPEA__WIDTH                               0x00000001
#define CAPABILITIES2_R__DRV_TYPEA__MASK                                0x00000010


/*  CAPABILITIES2_R : DRV_TYPEC           */
#define CAPABILITIES2_R__DRV_TYPEC__SHIFT                               0x00000005
#define CAPABILITIES2_R__DRV_TYPEC__WIDTH                               0x00000001
#define CAPABILITIES2_R__DRV_TYPEC__MASK                                0x00000020


/*  CAPABILITIES2_R : DRV_TYPED           */
#define CAPABILITIES2_R__DRV_TYPED__SHIFT                               0x00000006
#define CAPABILITIES2_R__DRV_TYPED__WIDTH                               0x00000001
#define CAPABILITIES2_R__DRV_TYPED__MASK                                0x00000040


/*  CAPABILITIES2_R : RSVD_39             */
#define CAPABILITIES2_R__RSVD_39__SHIFT                                 0x00000007
#define CAPABILITIES2_R__RSVD_39__WIDTH                                 0x00000001
#define CAPABILITIES2_R__RSVD_39__MASK                                  0x00000080

/*  CAPABILITIES2_R : RETUNE_CNT          */
#define CAPABILITIES2_R__RETUNE_CNT__SHIFT                              0x00000008
#define CAPABILITIES2_R__RETUNE_CNT__WIDTH                              0x00000004
#define CAPABILITIES2_R__RETUNE_CNT__MASK                               0x00000F00

/*  CAPABILITIES2_R : RSVD_44             */
#define CAPABILITIES2_R__RSVD_44__SHIFT                                 0x0000000C
#define CAPABILITIES2_R__RSVD_44__WIDTH                                 0x00000001
#define CAPABILITIES2_R__RSVD_44__MASK                                  0x00001000

/*  CAPABILITIES2_R : USE_TUNING_SDR50    */
#define CAPABILITIES2_R__USE_TUNING_SDR50__SHIFT                        0x0000000D
#define CAPABILITIES2_R__USE_TUNING_SDR50__WIDTH                        0x00000001
#define CAPABILITIES2_R__USE_TUNING_SDR50__MASK                         0x00002000


/*  CAPABILITIES2_R : RE_TUNING_MODES     */
#define CAPABILITIES2_R__RE_TUNING_MODES__SHIFT                         0x0000000E
#define CAPABILITIES2_R__RE_TUNING_MODES__WIDTH                         0x00000002
#define CAPABILITIES2_R__RE_TUNING_MODES__MASK                          0x0000C000


/*  CAPABILITIES2_R : CLK_MUL             */
#define CAPABILITIES2_R__CLK_MUL__SHIFT                                 0x00000010
#define CAPABILITIES2_R__CLK_MUL__WIDTH                                 0x00000008
#define CAPABILITIES2_R__CLK_MUL__MASK                                  0x00FF0000

/*  CAPABILITIES2_R : RSVD_56_58          */
#define CAPABILITIES2_R__RSVD_56_58__SHIFT                              0x00000018
#define CAPABILITIES2_R__RSVD_56_58__WIDTH                              0x00000003
#define CAPABILITIES2_R__RSVD_56_58__MASK                               0x07000000

/*  CAPABILITIES2_R : ADMA3_SUPPORT       */
#define CAPABILITIES2_R__ADMA3_SUPPORT__SHIFT                           0x0000001B
#define CAPABILITIES2_R__ADMA3_SUPPORT__WIDTH                           0x00000001
#define CAPABILITIES2_R__ADMA3_SUPPORT__MASK                            0x08000000


/*  CAPABILITIES2_R : VDD2_18V_SUPPORT    */
#define CAPABILITIES2_R__VDD2_18V_SUPPORT__SHIFT                        0x0000001C
#define CAPABILITIES2_R__VDD2_18V_SUPPORT__WIDTH                        0x00000001
#define CAPABILITIES2_R__VDD2_18V_SUPPORT__MASK                         0x10000000


/*  CAPABILITIES2_R : RSVD_61             */
#define CAPABILITIES2_R__RSVD_61__SHIFT                                 0x0000001D
#define CAPABILITIES2_R__RSVD_61__WIDTH                                 0x00000001
#define CAPABILITIES2_R__RSVD_61__MASK                                  0x20000000

/*  CAPABILITIES2_R : RSVD_62_63          */
#define CAPABILITIES2_R__RSVD_62_63__SHIFT                              0x0000001E
#define CAPABILITIES2_R__RSVD_62_63__WIDTH                              0x00000002
#define CAPABILITIES2_R__RSVD_62_63__MASK                               0xC0000000


/*-------------------------------------------------------------------------------------*/
/*  CURR_CAPABILITIES1_R : MAX_CUR_33V         */
#define CURR_CAPABILITIES1_R__MAX_CUR_33V__SHIFT                        0x00000000
#define CURR_CAPABILITIES1_R__MAX_CUR_33V__WIDTH                        0x00000008
#define CURR_CAPABILITIES1_R__MAX_CUR_33V__MASK                         0x000000FF

/*  CURR_CAPABILITIES1_R : MAX_CUR_30V         */
#define CURR_CAPABILITIES1_R__MAX_CUR_30V__SHIFT                        0x00000008
#define CURR_CAPABILITIES1_R__MAX_CUR_30V__WIDTH                        0x00000008
#define CURR_CAPABILITIES1_R__MAX_CUR_30V__MASK                         0x0000FF00

/*  CURR_CAPABILITIES1_R : MAX_CUR_18V         */
#define CURR_CAPABILITIES1_R__MAX_CUR_18V__SHIFT                        0x00000010
#define CURR_CAPABILITIES1_R__MAX_CUR_18V__WIDTH                        0x00000008
#define CURR_CAPABILITIES1_R__MAX_CUR_18V__MASK                         0x00FF0000

/*  CURR_CAPABILITIES1_R : RSVD_31_24          */
#define CURR_CAPABILITIES1_R__RSVD_31_24__SHIFT                         0x00000018
#define CURR_CAPABILITIES1_R__RSVD_31_24__WIDTH                         0x00000008
#define CURR_CAPABILITIES1_R__RSVD_31_24__MASK                          0xFF000000


/*-------------------------------------------------------------------------------------*/
/*  CURR_CAPABILITIES2_R : MAX_CUR_VDD2_18V    */
#define CURR_CAPABILITIES2_R__MAX_CUR_VDD2_18V__SHIFT                   0x00000000
#define CURR_CAPABILITIES2_R__MAX_CUR_VDD2_18V__WIDTH                   0x00000008
#define CURR_CAPABILITIES2_R__MAX_CUR_VDD2_18V__MASK                    0x000000FF

/*  CURR_CAPABILITIES2_R : RSVD_63_40          */
#define CURR_CAPABILITIES2_R__RSVD_63_40__SHIFT                         0x00000008
#define CURR_CAPABILITIES2_R__RSVD_63_40__WIDTH                         0x00000018
#define CURR_CAPABILITIES2_R__RSVD_63_40__MASK                          0xFFFFFF00


/*-------------------------------------------------------------------------------------*/
/*  FORCE_AUTO_CMD_STAT_R : FORCE_AUTO_CMD12_NOT_EXEC*/
#define FORCE_AUTO_CMD_STAT_R__FORCE_AUTO_CMD12_NOT_EXEC__SHIFT         0x00000000
#define FORCE_AUTO_CMD_STAT_R__FORCE_AUTO_CMD12_NOT_EXEC__WIDTH         0x00000001
#define FORCE_AUTO_CMD_STAT_R__FORCE_AUTO_CMD12_NOT_EXEC__MASK          0x00000001


/*  FORCE_AUTO_CMD_STAT_R : FORCE_AUTO_CMD_TOUT_ERR*/
#define FORCE_AUTO_CMD_STAT_R__FORCE_AUTO_CMD_TOUT_ERR__SHIFT           0x00000001
#define FORCE_AUTO_CMD_STAT_R__FORCE_AUTO_CMD_TOUT_ERR__WIDTH           0x00000001
#define FORCE_AUTO_CMD_STAT_R__FORCE_AUTO_CMD_TOUT_ERR__MASK            0x00000002


/*  FORCE_AUTO_CMD_STAT_R : FORCE_AUTO_CMD_CRC_ERR*/
#define FORCE_AUTO_CMD_STAT_R__FORCE_AUTO_CMD_CRC_ERR__SHIFT            0x00000002
#define FORCE_AUTO_CMD_STAT_R__FORCE_AUTO_CMD_CRC_ERR__WIDTH            0x00000001
#define FORCE_AUTO_CMD_STAT_R__FORCE_AUTO_CMD_CRC_ERR__MASK             0x00000004


/*  FORCE_AUTO_CMD_STAT_R : FORCE_AUTO_CMD_EBIT_ERR*/
#define FORCE_AUTO_CMD_STAT_R__FORCE_AUTO_CMD_EBIT_ERR__SHIFT           0x00000003
#define FORCE_AUTO_CMD_STAT_R__FORCE_AUTO_CMD_EBIT_ERR__WIDTH           0x00000001
#define FORCE_AUTO_CMD_STAT_R__FORCE_AUTO_CMD_EBIT_ERR__MASK            0x00000008


/*  FORCE_AUTO_CMD_STAT_R : FORCE_AUTO_CMD_IDX_ERR*/
#define FORCE_AUTO_CMD_STAT_R__FORCE_AUTO_CMD_IDX_ERR__SHIFT            0x00000004
#define FORCE_AUTO_CMD_STAT_R__FORCE_AUTO_CMD_IDX_ERR__WIDTH            0x00000001
#define FORCE_AUTO_CMD_STAT_R__FORCE_AUTO_CMD_IDX_ERR__MASK             0x00000010


/*  FORCE_AUTO_CMD_STAT_R : FORCE_AUTO_CMD_RESP_ERR*/
#define FORCE_AUTO_CMD_STAT_R__FORCE_AUTO_CMD_RESP_ERR__SHIFT           0x00000005
#define FORCE_AUTO_CMD_STAT_R__FORCE_AUTO_CMD_RESP_ERR__WIDTH           0x00000001
#define FORCE_AUTO_CMD_STAT_R__FORCE_AUTO_CMD_RESP_ERR__MASK            0x00000020


/*  FORCE_AUTO_CMD_STAT_R : RSVD_6              */
#define FORCE_AUTO_CMD_STAT_R__RSVD_6__SHIFT                            0x00000006
#define FORCE_AUTO_CMD_STAT_R__RSVD_6__WIDTH                            0x00000001
#define FORCE_AUTO_CMD_STAT_R__RSVD_6__MASK                             0x00000040

/*  FORCE_AUTO_CMD_STAT_R : FORCE_CMD_NOT_ISSUED_AUTO_CMD12*/
#define FORCE_AUTO_CMD_STAT_R__FORCE_CMD_NOT_ISSUED_AUTO_CMD12__SHIFT   0x00000007
#define FORCE_AUTO_CMD_STAT_R__FORCE_CMD_NOT_ISSUED_AUTO_CMD12__WIDTH   0x00000001
#define FORCE_AUTO_CMD_STAT_R__FORCE_CMD_NOT_ISSUED_AUTO_CMD12__MASK    0x00000080


/*  FORCE_AUTO_CMD_STAT_R : RSVD_15_8           */
#define FORCE_AUTO_CMD_STAT_R__RSVD_15_8__SHIFT                         0x00000008
#define FORCE_AUTO_CMD_STAT_R__RSVD_15_8__WIDTH                         0x00000008
#define FORCE_AUTO_CMD_STAT_R__RSVD_15_8__MASK                          0x0000FF00


/*-------------------------------------------------------------------------------------*/
/*  FORCE_ERROR_INT_STAT_R : FORCE_CMD_TOUT_ERR  */
#define FORCE_ERROR_INT_STAT_R__FORCE_CMD_TOUT_ERR__SHIFT               0x00000000
#define FORCE_ERROR_INT_STAT_R__FORCE_CMD_TOUT_ERR__WIDTH               0x00000001
#define FORCE_ERROR_INT_STAT_R__FORCE_CMD_TOUT_ERR__MASK                0x00000001


/*  FORCE_ERROR_INT_STAT_R : FORCE_CMD_CRC_ERR   */
#define FORCE_ERROR_INT_STAT_R__FORCE_CMD_CRC_ERR__SHIFT                0x00000001
#define FORCE_ERROR_INT_STAT_R__FORCE_CMD_CRC_ERR__WIDTH                0x00000001
#define FORCE_ERROR_INT_STAT_R__FORCE_CMD_CRC_ERR__MASK                 0x00000002


/*  FORCE_ERROR_INT_STAT_R : FORCE_CMD_END_BIT_ERR*/
#define FORCE_ERROR_INT_STAT_R__FORCE_CMD_END_BIT_ERR__SHIFT            0x00000002
#define FORCE_ERROR_INT_STAT_R__FORCE_CMD_END_BIT_ERR__WIDTH            0x00000001
#define FORCE_ERROR_INT_STAT_R__FORCE_CMD_END_BIT_ERR__MASK             0x00000004


/*  FORCE_ERROR_INT_STAT_R : FORCE_CMD_IDX_ERR   */
#define FORCE_ERROR_INT_STAT_R__FORCE_CMD_IDX_ERR__SHIFT                0x00000003
#define FORCE_ERROR_INT_STAT_R__FORCE_CMD_IDX_ERR__WIDTH                0x00000001
#define FORCE_ERROR_INT_STAT_R__FORCE_CMD_IDX_ERR__MASK                 0x00000008


/*  FORCE_ERROR_INT_STAT_R : FORCE_DATA_TOUT_ERR */
#define FORCE_ERROR_INT_STAT_R__FORCE_DATA_TOUT_ERR__SHIFT              0x00000004
#define FORCE_ERROR_INT_STAT_R__FORCE_DATA_TOUT_ERR__WIDTH              0x00000001
#define FORCE_ERROR_INT_STAT_R__FORCE_DATA_TOUT_ERR__MASK               0x00000010


/*  FORCE_ERROR_INT_STAT_R : FORCE_DATA_CRC_ERR  */
#define FORCE_ERROR_INT_STAT_R__FORCE_DATA_CRC_ERR__SHIFT               0x00000005
#define FORCE_ERROR_INT_STAT_R__FORCE_DATA_CRC_ERR__WIDTH               0x00000001
#define FORCE_ERROR_INT_STAT_R__FORCE_DATA_CRC_ERR__MASK                0x00000020


/*  FORCE_ERROR_INT_STAT_R : FORCE_DATA_END_BIT_ERR*/
#define FORCE_ERROR_INT_STAT_R__FORCE_DATA_END_BIT_ERR__SHIFT           0x00000006
#define FORCE_ERROR_INT_STAT_R__FORCE_DATA_END_BIT_ERR__WIDTH           0x00000001
#define FORCE_ERROR_INT_STAT_R__FORCE_DATA_END_BIT_ERR__MASK            0x00000040


/*  FORCE_ERROR_INT_STAT_R : FORCE_CUR_LMT_ERR   */
#define FORCE_ERROR_INT_STAT_R__FORCE_CUR_LMT_ERR__SHIFT                0x00000007
#define FORCE_ERROR_INT_STAT_R__FORCE_CUR_LMT_ERR__WIDTH                0x00000001
#define FORCE_ERROR_INT_STAT_R__FORCE_CUR_LMT_ERR__MASK                 0x00000080


/*  FORCE_ERROR_INT_STAT_R : FORCE_AUTO_CMD_ERR  */
#define FORCE_ERROR_INT_STAT_R__FORCE_AUTO_CMD_ERR__SHIFT               0x00000008
#define FORCE_ERROR_INT_STAT_R__FORCE_AUTO_CMD_ERR__WIDTH               0x00000001
#define FORCE_ERROR_INT_STAT_R__FORCE_AUTO_CMD_ERR__MASK                0x00000100


/*  FORCE_ERROR_INT_STAT_R : FORCE_ADMA_ERR      */
#define FORCE_ERROR_INT_STAT_R__FORCE_ADMA_ERR__SHIFT                   0x00000009
#define FORCE_ERROR_INT_STAT_R__FORCE_ADMA_ERR__WIDTH                   0x00000001
#define FORCE_ERROR_INT_STAT_R__FORCE_ADMA_ERR__MASK                    0x00000200


/*  FORCE_ERROR_INT_STAT_R : FORCE_TUNING_ERR    */
#define FORCE_ERROR_INT_STAT_R__FORCE_TUNING_ERR__SHIFT                 0x0000000A
#define FORCE_ERROR_INT_STAT_R__FORCE_TUNING_ERR__WIDTH                 0x00000001
#define FORCE_ERROR_INT_STAT_R__FORCE_TUNING_ERR__MASK                  0x00000400


/*  FORCE_ERROR_INT_STAT_R : FORCE_RESP_ERR      */
#define FORCE_ERROR_INT_STAT_R__FORCE_RESP_ERR__SHIFT                   0x0000000B
#define FORCE_ERROR_INT_STAT_R__FORCE_RESP_ERR__WIDTH                   0x00000001
#define FORCE_ERROR_INT_STAT_R__FORCE_RESP_ERR__MASK                    0x00000800


/*  FORCE_ERROR_INT_STAT_R : FORCE_BOOT_ACK_ERR  */
#define FORCE_ERROR_INT_STAT_R__FORCE_BOOT_ACK_ERR__SHIFT               0x0000000C
#define FORCE_ERROR_INT_STAT_R__FORCE_BOOT_ACK_ERR__WIDTH               0x00000001
#define FORCE_ERROR_INT_STAT_R__FORCE_BOOT_ACK_ERR__MASK                0x00001000


/*  FORCE_ERROR_INT_STAT_R : FORCE_VENDOR_ERR1   */
#define FORCE_ERROR_INT_STAT_R__FORCE_VENDOR_ERR1__SHIFT                0x0000000D
#define FORCE_ERROR_INT_STAT_R__FORCE_VENDOR_ERR1__WIDTH                0x00000001
#define FORCE_ERROR_INT_STAT_R__FORCE_VENDOR_ERR1__MASK                 0x00002000

/*  FORCE_ERROR_INT_STAT_R : FORCE_VENDOR_ERR2   */
#define FORCE_ERROR_INT_STAT_R__FORCE_VENDOR_ERR2__SHIFT                0x0000000E
#define FORCE_ERROR_INT_STAT_R__FORCE_VENDOR_ERR2__WIDTH                0x00000001
#define FORCE_ERROR_INT_STAT_R__FORCE_VENDOR_ERR2__MASK                 0x00004000

/*  FORCE_ERROR_INT_STAT_R : FORCE_VENDOR_ERR3   */
#define FORCE_ERROR_INT_STAT_R__FORCE_VENDOR_ERR3__SHIFT                0x0000000F
#define FORCE_ERROR_INT_STAT_R__FORCE_VENDOR_ERR3__WIDTH                0x00000001
#define FORCE_ERROR_INT_STAT_R__FORCE_VENDOR_ERR3__MASK                 0x00008000


/*-------------------------------------------------------------------------------------*/
/*  ADMA_ERR_STAT_R : ADMA_ERR_STATES     */
#define ADMA_ERR_STAT_R__ADMA_ERR_STATES__SHIFT                         0x00000000
#define ADMA_ERR_STAT_R__ADMA_ERR_STATES__WIDTH                         0x00000002
#define ADMA_ERR_STAT_R__ADMA_ERR_STATES__MASK                          0x00000003


/*  ADMA_ERR_STAT_R : ADMA_LEN_ERR        */
#define ADMA_ERR_STAT_R__ADMA_LEN_ERR__SHIFT                            0x00000002
#define ADMA_ERR_STAT_R__ADMA_LEN_ERR__WIDTH                            0x00000001
#define ADMA_ERR_STAT_R__ADMA_LEN_ERR__MASK                             0x00000004


/*  ADMA_ERR_STAT_R : RSVD_7_3            */
#define ADMA_ERR_STAT_R__RSVD_7_3__SHIFT                                0x00000003
#define ADMA_ERR_STAT_R__RSVD_7_3__WIDTH                                0x00000005
#define ADMA_ERR_STAT_R__RSVD_7_3__MASK                                 0x000000F8


/*-------------------------------------------------------------------------------------*/
/*  ADMA_SA_LOW_R : ADMA_SA_LOW         */
#define ADMA_SA_LOW_R__ADMA_SA_LOW__SHIFT                               0x00000000
#define ADMA_SA_LOW_R__ADMA_SA_LOW__WIDTH                               0x00000020
#define ADMA_SA_LOW_R__ADMA_SA_LOW__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  ADMA_SA_HIGH_R : ADMA_SA_HIGH        */
#define ADMA_SA_HIGH_R__ADMA_SA_HIGH__SHIFT                             0x00000000
#define ADMA_SA_HIGH_R__ADMA_SA_HIGH__WIDTH                             0x00000020
#define ADMA_SA_HIGH_R__ADMA_SA_HIGH__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  PRESET_INIT_R : FREQ_SEL_VAL        */
#define PRESET_INIT_R__FREQ_SEL_VAL__SHIFT                              0x00000000
#define PRESET_INIT_R__FREQ_SEL_VAL__WIDTH                              0x0000000A
#define PRESET_INIT_R__FREQ_SEL_VAL__MASK                               0x000003FF

/*  PRESET_INIT_R : CLK_GEN_SEL_VAL     */
#define PRESET_INIT_R__CLK_GEN_SEL_VAL__SHIFT                           0x0000000A
#define PRESET_INIT_R__CLK_GEN_SEL_VAL__WIDTH                           0x00000001
#define PRESET_INIT_R__CLK_GEN_SEL_VAL__MASK                            0x00000400


/*  PRESET_INIT_R : RSVD_13_11          */
#define PRESET_INIT_R__RSVD_13_11__SHIFT                                0x0000000B
#define PRESET_INIT_R__RSVD_13_11__WIDTH                                0x00000003
#define PRESET_INIT_R__RSVD_13_11__MASK                                 0x00003800

/*  PRESET_INIT_R : DRV_SEL_VAL         */
#define PRESET_INIT_R__DRV_SEL_VAL__SHIFT                               0x0000000E
#define PRESET_INIT_R__DRV_SEL_VAL__WIDTH                               0x00000002
#define PRESET_INIT_R__DRV_SEL_VAL__MASK                                0x0000C000



/*-------------------------------------------------------------------------------------*/
/*  PRESET_DS_R : FREQ_SEL_VAL        */
#define PRESET_DS_R__FREQ_SEL_VAL__SHIFT                                0x00000000
#define PRESET_DS_R__FREQ_SEL_VAL__WIDTH                                0x0000000A
#define PRESET_DS_R__FREQ_SEL_VAL__MASK                                 0x000003FF

/*  PRESET_DS_R : CLK_GEN_SEL_VAL     */
#define PRESET_DS_R__CLK_GEN_SEL_VAL__SHIFT                             0x0000000A
#define PRESET_DS_R__CLK_GEN_SEL_VAL__WIDTH                             0x00000001
#define PRESET_DS_R__CLK_GEN_SEL_VAL__MASK                              0x00000400


/*  PRESET_DS_R : RSVD_13_11          */
#define PRESET_DS_R__RSVD_13_11__SHIFT                                  0x0000000B
#define PRESET_DS_R__RSVD_13_11__WIDTH                                  0x00000003
#define PRESET_DS_R__RSVD_13_11__MASK                                   0x00003800

/*  PRESET_DS_R : DRV_SEL_VAL         */
#define PRESET_DS_R__DRV_SEL_VAL__SHIFT                                 0x0000000E
#define PRESET_DS_R__DRV_SEL_VAL__WIDTH                                 0x00000002
#define PRESET_DS_R__DRV_SEL_VAL__MASK                                  0x0000C000



/*-------------------------------------------------------------------------------------*/
/*  PRESET_HS_R : FREQ_SEL_VAL        */
#define PRESET_HS_R__FREQ_SEL_VAL__SHIFT                                0x00000000
#define PRESET_HS_R__FREQ_SEL_VAL__WIDTH                                0x0000000A
#define PRESET_HS_R__FREQ_SEL_VAL__MASK                                 0x000003FF

/*  PRESET_HS_R : CLK_GEN_SEL_VAL     */
#define PRESET_HS_R__CLK_GEN_SEL_VAL__SHIFT                             0x0000000A
#define PRESET_HS_R__CLK_GEN_SEL_VAL__WIDTH                             0x00000001
#define PRESET_HS_R__CLK_GEN_SEL_VAL__MASK                              0x00000400


/*  PRESET_HS_R : RSVD_13_11          */
#define PRESET_HS_R__RSVD_13_11__SHIFT                                  0x0000000B
#define PRESET_HS_R__RSVD_13_11__WIDTH                                  0x00000003
#define PRESET_HS_R__RSVD_13_11__MASK                                   0x00003800

/*  PRESET_HS_R : DRV_SEL_VAL         */
#define PRESET_HS_R__DRV_SEL_VAL__SHIFT                                 0x0000000E
#define PRESET_HS_R__DRV_SEL_VAL__WIDTH                                 0x00000002
#define PRESET_HS_R__DRV_SEL_VAL__MASK                                  0x0000C000



/*-------------------------------------------------------------------------------------*/
/*  PRESET_SDR12_R : FREQ_SEL_VAL        */
#define PRESET_SDR12_R__FREQ_SEL_VAL__SHIFT                             0x00000000
#define PRESET_SDR12_R__FREQ_SEL_VAL__WIDTH                             0x0000000A
#define PRESET_SDR12_R__FREQ_SEL_VAL__MASK                              0x000003FF

/*  PRESET_SDR12_R : CLK_GEN_SEL_VAL     */
#define PRESET_SDR12_R__CLK_GEN_SEL_VAL__SHIFT                          0x0000000A
#define PRESET_SDR12_R__CLK_GEN_SEL_VAL__WIDTH                          0x00000001
#define PRESET_SDR12_R__CLK_GEN_SEL_VAL__MASK                           0x00000400


/*  PRESET_SDR12_R : RSVD_13_11          */
#define PRESET_SDR12_R__RSVD_13_11__SHIFT                               0x0000000B
#define PRESET_SDR12_R__RSVD_13_11__WIDTH                               0x00000003
#define PRESET_SDR12_R__RSVD_13_11__MASK                                0x00003800

/*  PRESET_SDR12_R : DRV_SEL_VAL         */
#define PRESET_SDR12_R__DRV_SEL_VAL__SHIFT                              0x0000000E
#define PRESET_SDR12_R__DRV_SEL_VAL__WIDTH                              0x00000002
#define PRESET_SDR12_R__DRV_SEL_VAL__MASK                               0x0000C000



/*-------------------------------------------------------------------------------------*/
/*  PRESET_SDR25_R : FREQ_SEL_VAL        */
#define PRESET_SDR25_R__FREQ_SEL_VAL__SHIFT                             0x00000000
#define PRESET_SDR25_R__FREQ_SEL_VAL__WIDTH                             0x0000000A
#define PRESET_SDR25_R__FREQ_SEL_VAL__MASK                              0x000003FF

/*  PRESET_SDR25_R : CLK_GEN_SEL_VAL     */
#define PRESET_SDR25_R__CLK_GEN_SEL_VAL__SHIFT                          0x0000000A
#define PRESET_SDR25_R__CLK_GEN_SEL_VAL__WIDTH                          0x00000001
#define PRESET_SDR25_R__CLK_GEN_SEL_VAL__MASK                           0x00000400


/*  PRESET_SDR25_R : RSVD_13_11          */
#define PRESET_SDR25_R__RSVD_13_11__SHIFT                               0x0000000B
#define PRESET_SDR25_R__RSVD_13_11__WIDTH                               0x00000003
#define PRESET_SDR25_R__RSVD_13_11__MASK                                0x00003800

/*  PRESET_SDR25_R : DRV_SEL_VAL         */
#define PRESET_SDR25_R__DRV_SEL_VAL__SHIFT                              0x0000000E
#define PRESET_SDR25_R__DRV_SEL_VAL__WIDTH                              0x00000002
#define PRESET_SDR25_R__DRV_SEL_VAL__MASK                               0x0000C000



/*-------------------------------------------------------------------------------------*/
/*  PRESET_SDR50_R : FREQ_SEL_VAL        */
#define PRESET_SDR50_R__FREQ_SEL_VAL__SHIFT                             0x00000000
#define PRESET_SDR50_R__FREQ_SEL_VAL__WIDTH                             0x0000000A
#define PRESET_SDR50_R__FREQ_SEL_VAL__MASK                              0x000003FF

/*  PRESET_SDR50_R : CLK_GEN_SEL_VAL     */
#define PRESET_SDR50_R__CLK_GEN_SEL_VAL__SHIFT                          0x0000000A
#define PRESET_SDR50_R__CLK_GEN_SEL_VAL__WIDTH                          0x00000001
#define PRESET_SDR50_R__CLK_GEN_SEL_VAL__MASK                           0x00000400


/*  PRESET_SDR50_R : RSVD_13_11          */
#define PRESET_SDR50_R__RSVD_13_11__SHIFT                               0x0000000B
#define PRESET_SDR50_R__RSVD_13_11__WIDTH                               0x00000003
#define PRESET_SDR50_R__RSVD_13_11__MASK                                0x00003800

/*  PRESET_SDR50_R : DRV_SEL_VAL         */
#define PRESET_SDR50_R__DRV_SEL_VAL__SHIFT                              0x0000000E
#define PRESET_SDR50_R__DRV_SEL_VAL__WIDTH                              0x00000002
#define PRESET_SDR50_R__DRV_SEL_VAL__MASK                               0x0000C000



/*-------------------------------------------------------------------------------------*/
/*  PRESET_SDR104_R : FREQ_SEL_VAL        */
#define PRESET_SDR104_R__FREQ_SEL_VAL__SHIFT                            0x00000000
#define PRESET_SDR104_R__FREQ_SEL_VAL__WIDTH                            0x0000000A
#define PRESET_SDR104_R__FREQ_SEL_VAL__MASK                             0x000003FF

/*  PRESET_SDR104_R : CLK_GEN_SEL_VAL     */
#define PRESET_SDR104_R__CLK_GEN_SEL_VAL__SHIFT                         0x0000000A
#define PRESET_SDR104_R__CLK_GEN_SEL_VAL__WIDTH                         0x00000001
#define PRESET_SDR104_R__CLK_GEN_SEL_VAL__MASK                          0x00000400


/*  PRESET_SDR104_R : RSVD_13_11          */
#define PRESET_SDR104_R__RSVD_13_11__SHIFT                              0x0000000B
#define PRESET_SDR104_R__RSVD_13_11__WIDTH                              0x00000003
#define PRESET_SDR104_R__RSVD_13_11__MASK                               0x00003800

/*  PRESET_SDR104_R : DRV_SEL_VAL         */
#define PRESET_SDR104_R__DRV_SEL_VAL__SHIFT                             0x0000000E
#define PRESET_SDR104_R__DRV_SEL_VAL__WIDTH                             0x00000002
#define PRESET_SDR104_R__DRV_SEL_VAL__MASK                              0x0000C000



/*-------------------------------------------------------------------------------------*/
/*  PRESET_DDR50_R : FREQ_SEL_VAL        */
#define PRESET_DDR50_R__FREQ_SEL_VAL__SHIFT                             0x00000000
#define PRESET_DDR50_R__FREQ_SEL_VAL__WIDTH                             0x0000000A
#define PRESET_DDR50_R__FREQ_SEL_VAL__MASK                              0x000003FF

/*  PRESET_DDR50_R : CLK_GEN_SEL_VAL     */
#define PRESET_DDR50_R__CLK_GEN_SEL_VAL__SHIFT                          0x0000000A
#define PRESET_DDR50_R__CLK_GEN_SEL_VAL__WIDTH                          0x00000001
#define PRESET_DDR50_R__CLK_GEN_SEL_VAL__MASK                           0x00000400


/*  PRESET_DDR50_R : RSVD_13_11          */
#define PRESET_DDR50_R__RSVD_13_11__SHIFT                               0x0000000B
#define PRESET_DDR50_R__RSVD_13_11__WIDTH                               0x00000003
#define PRESET_DDR50_R__RSVD_13_11__MASK                                0x00003800

/*  PRESET_DDR50_R : DRV_SEL_VAL         */
#define PRESET_DDR50_R__DRV_SEL_VAL__SHIFT                              0x0000000E
#define PRESET_DDR50_R__DRV_SEL_VAL__WIDTH                              0x00000002
#define PRESET_DDR50_R__DRV_SEL_VAL__MASK                               0x0000C000



/*-------------------------------------------------------------------------------------*/
/*  PRESET_UHS2_R : FREQ_SEL_VAL        */
#define PRESET_UHS2_R__FREQ_SEL_VAL__SHIFT                              0x00000000
#define PRESET_UHS2_R__FREQ_SEL_VAL__WIDTH                              0x0000000A
#define PRESET_UHS2_R__FREQ_SEL_VAL__MASK                               0x000003FF

/*  PRESET_UHS2_R : CLK_GEN_SEL_VAL     */
#define PRESET_UHS2_R__CLK_GEN_SEL_VAL__SHIFT                           0x0000000A
#define PRESET_UHS2_R__CLK_GEN_SEL_VAL__WIDTH                           0x00000001
#define PRESET_UHS2_R__CLK_GEN_SEL_VAL__MASK                            0x00000400


/*  PRESET_UHS2_R : RSVD_13_11          */
#define PRESET_UHS2_R__RSVD_13_11__SHIFT                                0x0000000B
#define PRESET_UHS2_R__RSVD_13_11__WIDTH                                0x00000003
#define PRESET_UHS2_R__RSVD_13_11__MASK                                 0x00003800

/*  PRESET_UHS2_R : DRV_SEL_VAL         */
#define PRESET_UHS2_R__DRV_SEL_VAL__SHIFT                               0x0000000E
#define PRESET_UHS2_R__DRV_SEL_VAL__WIDTH                               0x00000002
#define PRESET_UHS2_R__DRV_SEL_VAL__MASK                                0x0000C000



/*-------------------------------------------------------------------------------------*/
/*  ADMA_ID_LOW_R : ADMA_ID_LOW         */
#define ADMA_ID_LOW_R__ADMA_ID_LOW__SHIFT                               0x00000000
#define ADMA_ID_LOW_R__ADMA_ID_LOW__WIDTH                               0x00000020
#define ADMA_ID_LOW_R__ADMA_ID_LOW__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  ADMA_ID_HIGH_R : ADMA_ID_HIGH        */
#define ADMA_ID_HIGH_R__ADMA_ID_HIGH__SHIFT                             0x00000000
#define ADMA_ID_HIGH_R__ADMA_ID_HIGH__WIDTH                             0x00000020
#define ADMA_ID_HIGH_R__ADMA_ID_HIGH__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  P_EMBEDDED_CNTRL : REG_OFFSET_ADDR     */
#define P_EMBEDDED_CNTRL__REG_OFFSET_ADDR__SHIFT                        0x00000000
#define P_EMBEDDED_CNTRL__REG_OFFSET_ADDR__WIDTH                        0x0000000C
#define P_EMBEDDED_CNTRL__REG_OFFSET_ADDR__MASK                         0x00000FFF

/*  P_EMBEDDED_CNTRL : RESERVED_15_12      */
#define P_EMBEDDED_CNTRL__RESERVED_15_12__SHIFT                         0x0000000C
#define P_EMBEDDED_CNTRL__RESERVED_15_12__WIDTH                         0x00000004
#define P_EMBEDDED_CNTRL__RESERVED_15_12__MASK                          0x0000F000


/*-------------------------------------------------------------------------------------*/
/*  P_VENDOR_SPECIFIC_AREA : REG_OFFSET_ADDR     */
#define P_VENDOR_SPECIFIC_AREA__REG_OFFSET_ADDR__SHIFT                  0x00000000
#define P_VENDOR_SPECIFIC_AREA__REG_OFFSET_ADDR__WIDTH                  0x0000000C
#define P_VENDOR_SPECIFIC_AREA__REG_OFFSET_ADDR__MASK                   0x00000FFF

/*  P_VENDOR_SPECIFIC_AREA : RESERVED_15_12      */
#define P_VENDOR_SPECIFIC_AREA__RESERVED_15_12__SHIFT                   0x0000000C
#define P_VENDOR_SPECIFIC_AREA__RESERVED_15_12__WIDTH                   0x00000004
#define P_VENDOR_SPECIFIC_AREA__RESERVED_15_12__MASK                    0x0000F000


/*-------------------------------------------------------------------------------------*/
/*  P_VENDOR2_SPECIFIC_AREA : REG_OFFSET_ADDR     */
#define P_VENDOR2_SPECIFIC_AREA__REG_OFFSET_ADDR__SHIFT                 0x00000000
#define P_VENDOR2_SPECIFIC_AREA__REG_OFFSET_ADDR__WIDTH                 0x00000010
#define P_VENDOR2_SPECIFIC_AREA__REG_OFFSET_ADDR__MASK                  0x0000FFFF


/*-------------------------------------------------------------------------------------*/
/*  SLOT_INTR_STATUS_R : INTR_SLOT           */
#define SLOT_INTR_STATUS_R__INTR_SLOT__SHIFT                            0x00000000
#define SLOT_INTR_STATUS_R__INTR_SLOT__WIDTH                            0x00000008
#define SLOT_INTR_STATUS_R__INTR_SLOT__MASK                             0x000000FF

/*  SLOT_INTR_STATUS_R : RESERVED_15_8       */
#define SLOT_INTR_STATUS_R__RESERVED_15_8__SHIFT                        0x00000008
#define SLOT_INTR_STATUS_R__RESERVED_15_8__WIDTH                        0x00000008
#define SLOT_INTR_STATUS_R__RESERVED_15_8__MASK                         0x0000FF00


/*-------------------------------------------------------------------------------------*/
/*  HOST_CNTRL_VERS_R : SPEC_VERSION_NUM    */
#define HOST_CNTRL_VERS_R__SPEC_VERSION_NUM__SHIFT                      0x00000000
#define HOST_CNTRL_VERS_R__SPEC_VERSION_NUM__WIDTH                      0x00000008
#define HOST_CNTRL_VERS_R__SPEC_VERSION_NUM__MASK                       0x000000FF


/*  HOST_CNTRL_VERS_R : VENDOR_VERSION_NUM  */
#define HOST_CNTRL_VERS_R__VENDOR_VERSION_NUM__SHIFT                    0x00000008
#define HOST_CNTRL_VERS_R__VENDOR_VERSION_NUM__WIDTH                    0x00000008
#define HOST_CNTRL_VERS_R__VENDOR_VERSION_NUM__MASK                     0x0000FF00


/*-------------------------------------------------------------------------------------*/
/*  MSHC_VER_ID_R : MSHC_VER_ID         */
#define MSHC_VER_ID_R__MSHC_VER_ID__SHIFT                               0x00000000
#define MSHC_VER_ID_R__MSHC_VER_ID__WIDTH                               0x00000020
#define MSHC_VER_ID_R__MSHC_VER_ID__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  MSHC_VER_TYPE_R : MSHC_VER_TYPE       */
#define MSHC_VER_TYPE_R__MSHC_VER_TYPE__SHIFT                           0x00000000
#define MSHC_VER_TYPE_R__MSHC_VER_TYPE__WIDTH                           0x00000020
#define MSHC_VER_TYPE_R__MSHC_VER_TYPE__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  MSHC_CTRL_R : CMD_CONFLICT_CHECK  */
#define MSHC_CTRL_R__CMD_CONFLICT_CHECK__SHIFT                          0x00000000
#define MSHC_CTRL_R__CMD_CONFLICT_CHECK__WIDTH                          0x00000001
#define MSHC_CTRL_R__CMD_CONFLICT_CHECK__MASK                           0x00000001


/*  MSHC_CTRL_R : RSVD1               */
#define MSHC_CTRL_R__RSVD1__SHIFT                                       0x00000001
#define MSHC_CTRL_R__RSVD1__WIDTH                                       0x00000003
#define MSHC_CTRL_R__RSVD1__MASK                                        0x0000000E

/*  MSHC_CTRL_R : SW_CG_DIS           */
#define MSHC_CTRL_R__SW_CG_DIS__SHIFT                                   0x00000004
#define MSHC_CTRL_R__SW_CG_DIS__WIDTH                                   0x00000001
#define MSHC_CTRL_R__SW_CG_DIS__MASK                                    0x00000010


/*  MSHC_CTRL_R : BE_KEY              */
#define MSHC_CTRL_R__BE_KEY__SHIFT                                      0x00000005
#define MSHC_CTRL_R__BE_KEY__WIDTH                                      0x00000001
#define MSHC_CTRL_R__BE_KEY__MASK                                       0x00000020


/*  MSHC_CTRL_R : BE_SLV_BUS          */
#define MSHC_CTRL_R__BE_SLV_BUS__SHIFT                                  0x00000006
#define MSHC_CTRL_R__BE_SLV_BUS__WIDTH                                  0x00000001
#define MSHC_CTRL_R__BE_SLV_BUS__MASK                                   0x00000040


/*  MSHC_CTRL_R : BE_MAS_BUS          */
#define MSHC_CTRL_R__BE_MAS_BUS__SHIFT                                  0x00000007
#define MSHC_CTRL_R__BE_MAS_BUS__WIDTH                                  0x00000001
#define MSHC_CTRL_R__BE_MAS_BUS__MASK                                   0x00000080



/*-------------------------------------------------------------------------------------*/
/*  MBIU_CTRL_R : UNDEFL_INCR_EN      */
#define MBIU_CTRL_R__UNDEFL_INCR_EN__SHIFT                              0x00000000
#define MBIU_CTRL_R__UNDEFL_INCR_EN__WIDTH                              0x00000001
#define MBIU_CTRL_R__UNDEFL_INCR_EN__MASK                               0x00000001


/*  MBIU_CTRL_R : BURST_INCR4_EN      */
#define MBIU_CTRL_R__BURST_INCR4_EN__SHIFT                              0x00000001
#define MBIU_CTRL_R__BURST_INCR4_EN__WIDTH                              0x00000001
#define MBIU_CTRL_R__BURST_INCR4_EN__MASK                               0x00000002


/*  MBIU_CTRL_R : BURST_INCR8_EN      */
#define MBIU_CTRL_R__BURST_INCR8_EN__SHIFT                              0x00000002
#define MBIU_CTRL_R__BURST_INCR8_EN__WIDTH                              0x00000001
#define MBIU_CTRL_R__BURST_INCR8_EN__MASK                               0x00000004


/*  MBIU_CTRL_R : BURST_INCR16_EN     */
#define MBIU_CTRL_R__BURST_INCR16_EN__SHIFT                             0x00000003
#define MBIU_CTRL_R__BURST_INCR16_EN__WIDTH                             0x00000001
#define MBIU_CTRL_R__BURST_INCR16_EN__MASK                              0x00000008


/*  MBIU_CTRL_R : RSVD                */
#define MBIU_CTRL_R__RSVD__SHIFT                                        0x00000004
#define MBIU_CTRL_R__RSVD__WIDTH                                        0x00000004
#define MBIU_CTRL_R__RSVD__MASK                                         0x000000F0


/*-------------------------------------------------------------------------------------*/
/*  EMMC_CTRL_R : CARD_IS_EMMC        */
#define EMMC_CTRL_R__CARD_IS_EMMC__SHIFT                                0x00000000
#define EMMC_CTRL_R__CARD_IS_EMMC__WIDTH                                0x00000001
#define EMMC_CTRL_R__CARD_IS_EMMC__MASK                                 0x00000001


/*  EMMC_CTRL_R : DISABLE_DATA_CRC_CHK*/
#define EMMC_CTRL_R__DISABLE_DATA_CRC_CHK__SHIFT                        0x00000001
#define EMMC_CTRL_R__DISABLE_DATA_CRC_CHK__WIDTH                        0x00000001
#define EMMC_CTRL_R__DISABLE_DATA_CRC_CHK__MASK                         0x00000002


/*  EMMC_CTRL_R : EMMC_RST_N          */
#define EMMC_CTRL_R__EMMC_RST_N__SHIFT                                  0x00000002
#define EMMC_CTRL_R__EMMC_RST_N__WIDTH                                  0x00000001
#define EMMC_CTRL_R__EMMC_RST_N__MASK                                   0x00000004


/*  EMMC_CTRL_R : EMMC_RST_N_OE       */
#define EMMC_CTRL_R__EMMC_RST_N_OE__SHIFT                               0x00000003
#define EMMC_CTRL_R__EMMC_RST_N_OE__WIDTH                               0x00000001
#define EMMC_CTRL_R__EMMC_RST_N_OE__MASK                                0x00000008


/*  EMMC_CTRL_R : ENH_STROBE_ENABLE   */
#define EMMC_CTRL_R__ENH_STROBE_ENABLE__SHIFT                           0x00000008
#define EMMC_CTRL_R__ENH_STROBE_ENABLE__WIDTH                           0x00000001
#define EMMC_CTRL_R__ENH_STROBE_ENABLE__MASK                            0x00000100


/*  EMMC_CTRL_R : RSVD                */
#define EMMC_CTRL_R__RSVD__SHIFT                                        0x0000000B
#define EMMC_CTRL_R__RSVD__WIDTH                                        0x00000005
#define EMMC_CTRL_R__RSVD__MASK                                         0x0000F800


/*-------------------------------------------------------------------------------------*/
/*  BOOT_CTRL_R : MAN_BOOT_EN         */
#define BOOT_CTRL_R__MAN_BOOT_EN__SHIFT                                 0x00000000
#define BOOT_CTRL_R__MAN_BOOT_EN__WIDTH                                 0x00000001
#define BOOT_CTRL_R__MAN_BOOT_EN__MASK                                  0x00000001


/*  BOOT_CTRL_R : RSVD_6_1            */
#define BOOT_CTRL_R__RSVD_6_1__SHIFT                                    0x00000001
#define BOOT_CTRL_R__RSVD_6_1__WIDTH                                    0x00000006
#define BOOT_CTRL_R__RSVD_6_1__MASK                                     0x0000007E

/*  BOOT_CTRL_R : VALIDATE_BOOT       */
#define BOOT_CTRL_R__VALIDATE_BOOT__SHIFT                               0x00000007
#define BOOT_CTRL_R__VALIDATE_BOOT__WIDTH                               0x00000001
#define BOOT_CTRL_R__VALIDATE_BOOT__MASK                                0x00000080


/*  BOOT_CTRL_R : BOOT_ACK_ENABLE     */
#define BOOT_CTRL_R__BOOT_ACK_ENABLE__SHIFT                             0x00000008
#define BOOT_CTRL_R__BOOT_ACK_ENABLE__WIDTH                             0x00000001
#define BOOT_CTRL_R__BOOT_ACK_ENABLE__MASK                              0x00000100


/*  BOOT_CTRL_R : RSVD_11_9           */
#define BOOT_CTRL_R__RSVD_11_9__SHIFT                                   0x00000009
#define BOOT_CTRL_R__RSVD_11_9__WIDTH                                   0x00000003
#define BOOT_CTRL_R__RSVD_11_9__MASK                                    0x00000E00

/*  BOOT_CTRL_R : BOOT_TOUT_CNT       */
#define BOOT_CTRL_R__BOOT_TOUT_CNT__SHIFT                               0x0000000C
#define BOOT_CTRL_R__BOOT_TOUT_CNT__WIDTH                               0x00000004
#define BOOT_CTRL_R__BOOT_TOUT_CNT__MASK                                0x0000F000


/*-------------------------------------------------------------------------------------*/
/*  AT_CTRL_R : AT_EN               */
#define AT_CTRL_R__AT_EN__SHIFT                                         0x00000000
#define AT_CTRL_R__AT_EN__WIDTH                                         0x00000001
#define AT_CTRL_R__AT_EN__MASK                                          0x00000001


/*  AT_CTRL_R : CI_SEL              */
#define AT_CTRL_R__CI_SEL__SHIFT                                        0x00000001
#define AT_CTRL_R__CI_SEL__WIDTH                                        0x00000001
#define AT_CTRL_R__CI_SEL__MASK                                         0x00000002


/*  AT_CTRL_R : SWIN_TH_EN          */
#define AT_CTRL_R__SWIN_TH_EN__SHIFT                                    0x00000002
#define AT_CTRL_R__SWIN_TH_EN__WIDTH                                    0x00000001
#define AT_CTRL_R__SWIN_TH_EN__MASK                                     0x00000004


/*  AT_CTRL_R : RPT_TUNE_ERR        */
#define AT_CTRL_R__RPT_TUNE_ERR__SHIFT                                  0x00000003
#define AT_CTRL_R__RPT_TUNE_ERR__WIDTH                                  0x00000001
#define AT_CTRL_R__RPT_TUNE_ERR__MASK                                   0x00000008


/*  AT_CTRL_R : SW_TUNE_EN          */
#define AT_CTRL_R__SW_TUNE_EN__SHIFT                                    0x00000004
#define AT_CTRL_R__SW_TUNE_EN__WIDTH                                    0x00000001
#define AT_CTRL_R__SW_TUNE_EN__MASK                                     0x00000010


/*  AT_CTRL_R : RSDV2               */
#define AT_CTRL_R__RSDV2__SHIFT                                         0x00000005
#define AT_CTRL_R__RSDV2__WIDTH                                         0x00000003
#define AT_CTRL_R__RSDV2__MASK                                          0x000000E0

/*  AT_CTRL_R : WIN_EDGE_SEL        */
#define AT_CTRL_R__WIN_EDGE_SEL__SHIFT                                  0x00000008
#define AT_CTRL_R__WIN_EDGE_SEL__WIDTH                                  0x00000004
#define AT_CTRL_R__WIN_EDGE_SEL__MASK                                   0x00000F00

/*  AT_CTRL_R : RSDV3               */
#define AT_CTRL_R__RSDV3__SHIFT                                         0x0000000C
#define AT_CTRL_R__RSDV3__WIDTH                                         0x00000004
#define AT_CTRL_R__RSDV3__MASK                                          0x0000F000

/*  AT_CTRL_R : TUNE_CLK_STOP_EN    */
#define AT_CTRL_R__TUNE_CLK_STOP_EN__SHIFT                              0x00000010
#define AT_CTRL_R__TUNE_CLK_STOP_EN__WIDTH                              0x00000001
#define AT_CTRL_R__TUNE_CLK_STOP_EN__MASK                               0x00010000


/*  AT_CTRL_R : PRE_CHANGE_DLY      */
#define AT_CTRL_R__PRE_CHANGE_DLY__SHIFT                                0x00000011
#define AT_CTRL_R__PRE_CHANGE_DLY__WIDTH                                0x00000002
#define AT_CTRL_R__PRE_CHANGE_DLY__MASK                                 0x00060000


/*  AT_CTRL_R : POST_CHANGE_DLY     */
#define AT_CTRL_R__POST_CHANGE_DLY__SHIFT                               0x00000013
#define AT_CTRL_R__POST_CHANGE_DLY__WIDTH                               0x00000002
#define AT_CTRL_R__POST_CHANGE_DLY__MASK                                0x00180000


/*  AT_CTRL_R : SWIN_TH_VAL         */
#define AT_CTRL_R__SWIN_TH_VAL__SHIFT                                   0x00000018
#define AT_CTRL_R__SWIN_TH_VAL__WIDTH                                   0x00000003
#define AT_CTRL_R__SWIN_TH_VAL__MASK                                    0x07000000


/*-------------------------------------------------------------------------------------*/
/*  AT_STAT_R : CENTER_PH_CODE      */
#define AT_STAT_R__CENTER_PH_CODE__SHIFT                                0x00000000
#define AT_STAT_R__CENTER_PH_CODE__WIDTH                                0x00000008
#define AT_STAT_R__CENTER_PH_CODE__MASK                                 0x000000FF

/*  AT_STAT_R : R_EDGE_PH_CODE      */
#define AT_STAT_R__R_EDGE_PH_CODE__SHIFT                                0x00000008
#define AT_STAT_R__R_EDGE_PH_CODE__WIDTH                                0x00000008
#define AT_STAT_R__R_EDGE_PH_CODE__MASK                                 0x0000FF00

/*  AT_STAT_R : L_EDGE_PH_CODE      */
#define AT_STAT_R__L_EDGE_PH_CODE__SHIFT                                0x00000010
#define AT_STAT_R__L_EDGE_PH_CODE__WIDTH                                0x00000008
#define AT_STAT_R__L_EDGE_PH_CODE__MASK                                 0x00FF0000

/*  AT_STAT_R : RSDV1               */
#define AT_STAT_R__RSDV1__SHIFT                                         0x00000018
#define AT_STAT_R__RSDV1__WIDTH                                         0x00000008
#define AT_STAT_R__RSDV1__MASK                                          0xFF000000


/*-------------------------------------------------------------------------------------*/
/*  EMBEDDED_CTRL_R : NUM_CLK_PIN         */
#define EMBEDDED_CTRL_R__NUM_CLK_PIN__SHIFT                             0x00000000
#define EMBEDDED_CTRL_R__NUM_CLK_PIN__WIDTH                             0x00000003
#define EMBEDDED_CTRL_R__NUM_CLK_PIN__MASK                              0x00000007

/*  EMBEDDED_CTRL_R : RSVD_3              */
#define EMBEDDED_CTRL_R__RSVD_3__SHIFT                                  0x00000003
#define EMBEDDED_CTRL_R__RSVD_3__WIDTH                                  0x00000001
#define EMBEDDED_CTRL_R__RSVD_3__MASK                                   0x00000008

/*  EMBEDDED_CTRL_R : NUM_INT_PIN         */
#define EMBEDDED_CTRL_R__NUM_INT_PIN__SHIFT                             0x00000004
#define EMBEDDED_CTRL_R__NUM_INT_PIN__WIDTH                             0x00000002
#define EMBEDDED_CTRL_R__NUM_INT_PIN__MASK                              0x00000030

/*  EMBEDDED_CTRL_R : RSVD_7_6            */
#define EMBEDDED_CTRL_R__RSVD_7_6__SHIFT                                0x00000006
#define EMBEDDED_CTRL_R__RSVD_7_6__WIDTH                                0x00000002
#define EMBEDDED_CTRL_R__RSVD_7_6__MASK                                 0x000000C0

/*  EMBEDDED_CTRL_R : BUS_WIDTH_PRESET    */
#define EMBEDDED_CTRL_R__BUS_WIDTH_PRESET__SHIFT                        0x00000008
#define EMBEDDED_CTRL_R__BUS_WIDTH_PRESET__WIDTH                        0x00000007
#define EMBEDDED_CTRL_R__BUS_WIDTH_PRESET__MASK                         0x00007F00

/*  EMBEDDED_CTRL_R : RSVD_15             */
#define EMBEDDED_CTRL_R__RSVD_15__SHIFT                                 0x0000000F
#define EMBEDDED_CTRL_R__RSVD_15__WIDTH                                 0x00000001
#define EMBEDDED_CTRL_R__RSVD_15__MASK                                  0x00008000

/*  EMBEDDED_CTRL_R : CLK_PIN_SEL         */
#define EMBEDDED_CTRL_R__CLK_PIN_SEL__SHIFT                             0x00000010
#define EMBEDDED_CTRL_R__CLK_PIN_SEL__WIDTH                             0x00000003
#define EMBEDDED_CTRL_R__CLK_PIN_SEL__MASK                              0x00070000

/*  EMBEDDED_CTRL_R : RSVD_19             */
#define EMBEDDED_CTRL_R__RSVD_19__SHIFT                                 0x00000013
#define EMBEDDED_CTRL_R__RSVD_19__WIDTH                                 0x00000001
#define EMBEDDED_CTRL_R__RSVD_19__MASK                                  0x00080000

/*  EMBEDDED_CTRL_R : INT_PIN_SEL         */
#define EMBEDDED_CTRL_R__INT_PIN_SEL__SHIFT                             0x00000014
#define EMBEDDED_CTRL_R__INT_PIN_SEL__WIDTH                             0x00000003
#define EMBEDDED_CTRL_R__INT_PIN_SEL__MASK                              0x00700000

/*  EMBEDDED_CTRL_R : RSVD_23             */
#define EMBEDDED_CTRL_R__RSVD_23__SHIFT                                 0x00000017
#define EMBEDDED_CTRL_R__RSVD_23__WIDTH                                 0x00000001
#define EMBEDDED_CTRL_R__RSVD_23__MASK                                  0x00800000

/*  EMBEDDED_CTRL_R : BACK_END_PWR_CTRL   */
#define EMBEDDED_CTRL_R__BACK_END_PWR_CTRL__SHIFT                       0x00000018
#define EMBEDDED_CTRL_R__BACK_END_PWR_CTRL__WIDTH                       0x00000007
#define EMBEDDED_CTRL_R__BACK_END_PWR_CTRL__MASK                        0x7F000000

/*  EMBEDDED_CTRL_R : RSVD_31             */
#define EMBEDDED_CTRL_R__RSVD_31__SHIFT                                 0x0000001F
#define EMBEDDED_CTRL_R__RSVD_31__WIDTH                                 0x00000001
#define EMBEDDED_CTRL_R__RSVD_31__MASK                                  0x80000000


/*-------------------------------------------------------------------------------------*/
/*  CQCAP : ITCFVAL             */
#define CQCAP__ITCFVAL__SHIFT                                           0x00000000
#define CQCAP__ITCFVAL__WIDTH                                           0x0000000A
#define CQCAP__ITCFVAL__MASK                                            0x000003FF

/*  CQCAP : CQCCAP_RSVD1        */
#define CQCAP__CQCCAP_RSVD1__SHIFT                                      0x0000000A
#define CQCAP__CQCCAP_RSVD1__WIDTH                                      0x00000002
#define CQCAP__CQCCAP_RSVD1__MASK                                       0x00000C00

/*  CQCAP : ITCFMUL             */
#define CQCAP__ITCFMUL__SHIFT                                           0x0000000C
#define CQCAP__ITCFMUL__WIDTH                                           0x00000004
#define CQCAP__ITCFMUL__MASK                                            0x0000F000


/*  CQCAP : CQCCAP_RSVD2        */
#define CQCAP__CQCCAP_RSVD2__SHIFT                                      0x00000010
#define CQCAP__CQCCAP_RSVD2__WIDTH                                      0x0000000C
#define CQCAP__CQCCAP_RSVD2__MASK                                       0x0FFF0000

/*  CQCAP : CRYPTO_SUPPORT      */
#define CQCAP__CRYPTO_SUPPORT__SHIFT                                    0x0000001C
#define CQCAP__CRYPTO_SUPPORT__WIDTH                                    0x00000001
#define CQCAP__CRYPTO_SUPPORT__MASK                                     0x10000000


/*  CQCAP : CQCCAP_RSVD3        */
#define CQCAP__CQCCAP_RSVD3__SHIFT                                      0x0000001D
#define CQCAP__CQCCAP_RSVD3__WIDTH                                      0x00000003
#define CQCAP__CQCCAP_RSVD3__MASK                                       0xE0000000


/*-------------------------------------------------------------------------------------*/
/*  CQCFG : CR_GENERAL_EN       */
#define CQCFG__CR_GENERAL_EN__SHIFT                                     0x00000001
#define CQCFG__CR_GENERAL_EN__WIDTH                                     0x00000001
#define CQCFG__CR_GENERAL_EN__MASK                                      0x00000002


/*  CQCFG : CQCCFG_RSVD1        */
#define CQCFG__CQCCFG_RSVD1__SHIFT                                      0x00000002
#define CQCFG__CQCCFG_RSVD1__WIDTH                                      0x00000006
#define CQCFG__CQCCFG_RSVD1__MASK                                       0x000000FC

/*  CQCFG : CQCCFG_RSVD2        */
#define CQCFG__CQCCFG_RSVD2__SHIFT                                      0x00000009
#define CQCFG__CQCCFG_RSVD2__WIDTH                                      0x00000003
#define CQCFG__CQCCFG_RSVD2__MASK                                       0x00000E00

/*  CQCFG : CQCCFG_RSVD3        */
#define CQCFG__CQCCFG_RSVD3__SHIFT                                      0x0000000D
#define CQCFG__CQCCFG_RSVD3__WIDTH                                      0x00000013
#define CQCFG__CQCCFG_RSVD3__MASK                                       0xFFFFE000


/*-------------------------------------------------------------------------------------*/
/*  CRNQP : CCI                 */
#define CRNQP__CCI__SHIFT                                               0x00000000
#define CRNQP__CCI__WIDTH                                               0x00000008
#define CRNQP__CCI__MASK                                                0x000000FF

/*  CRNQP : CE                  */
#define CRNQP__CE__SHIFT                                                0x00000008
#define CRNQP__CE__WIDTH                                                0x00000001
#define CRNQP__CE__MASK                                                 0x00000100


/*  CRNQP : CRNQP_RSDV1         */
#define CRNQP__CRNQP_RSDV1__SHIFT                                       0x00000009
#define CRNQP__CRNQP_RSDV1__WIDTH                                       0x00000017
#define CRNQP__CRNQP_RSDV1__MASK                                        0xFFFFFE00


/*-------------------------------------------------------------------------------------*/
/*  CRNQDUN : DUN                 */
#define CRNQDUN__DUN__SHIFT                                             0x00000000
#define CRNQDUN__DUN__WIDTH                                             0x00000020
#define CRNQDUN__DUN__MASK                                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRNQIS : GCE                 */
#define CRNQIS__GCE__SHIFT                                              0x00000000
#define CRNQIS__GCE__WIDTH                                              0x00000001
#define CRNQIS__GCE__MASK                                               0x00000001


/*  CRNQIS : ICCE                */
#define CRNQIS__ICCE__SHIFT                                             0x00000001
#define CRNQIS__ICCE__WIDTH                                             0x00000001
#define CRNQIS__ICCE__MASK                                              0x00000002


/*  CRNQIS : CRNQIS_RSVD1        */
#define CRNQIS__CRNQIS_RSVD1__SHIFT                                     0x00000002
#define CRNQIS__CRNQIS_RSVD1__WIDTH                                     0x0000001E
#define CRNQIS__CRNQIS_RSVD1__MASK                                      0xFFFFFFFC


/*-------------------------------------------------------------------------------------*/
/*  CRNQIE : GCE_IE              */
#define CRNQIE__GCE_IE__SHIFT                                           0x00000000
#define CRNQIE__GCE_IE__WIDTH                                           0x00000001
#define CRNQIE__GCE_IE__MASK                                            0x00000001


/*  CRNQIE : ICCE_IE             */
#define CRNQIE__ICCE_IE__SHIFT                                          0x00000001
#define CRNQIE__ICCE_IE__WIDTH                                          0x00000001
#define CRNQIE__ICCE_IE__MASK                                           0x00000002


/*  CRNQIE : CRNQIE_RSVD1        */
#define CRNQIE__CRNQIE_RSVD1__SHIFT                                     0x00000002
#define CRNQIE__CRNQIE_RSVD1__WIDTH                                     0x00000014
#define CRNQIE__CRNQIE_RSVD1__MASK                                      0x003FFFFC


/*-------------------------------------------------------------------------------------*/
/*  CRCAP : CC                  */
#define CRCAP__CC__SHIFT                                                0x00000000
#define CRCAP__CC__WIDTH                                                0x00000008
#define CRCAP__CC__MASK                                                 0x000000FF

/*  CRCAP : CFGC                */
#define CRCAP__CFGC__SHIFT                                              0x00000008
#define CRCAP__CFGC__WIDTH                                              0x00000008
#define CRCAP__CFGC__MASK                                               0x0000FF00

/*  CRCAP : CRCAP_RSVD1         */
#define CRCAP__CRCAP_RSVD1__SHIFT                                       0x00000010
#define CRCAP__CRCAP_RSVD1__WIDTH                                       0x00000008
#define CRCAP__CRCAP_RSVD1__MASK                                        0x00FF0000

/*  CRCAP : CFGPTR              */
#define CRCAP__CFGPTR__SHIFT                                            0x00000018
#define CRCAP__CFGPTR__WIDTH                                            0x00000008
#define CRCAP__CFGPTR__MASK                                             0xFF000000


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCAP_0 : ALGID               */
#define CRYPTOCAP_0__ALGID__SHIFT                                       0x00000000
#define CRYPTOCAP_0__ALGID__WIDTH                                       0x00000008
#define CRYPTOCAP_0__ALGID__MASK                                        0x000000FF

/*  CRYPTOCAP_0 : SDUSB               */
#define CRYPTOCAP_0__SDUSB__SHIFT                                       0x00000008
#define CRYPTOCAP_0__SDUSB__WIDTH                                       0x00000008
#define CRYPTOCAP_0__SDUSB__MASK                                        0x0000FF00

/*  CRYPTOCAP_0 : KS                  */
#define CRYPTOCAP_0__KS__SHIFT                                          0x00000010
#define CRYPTOCAP_0__KS__WIDTH                                          0x00000008
#define CRYPTOCAP_0__KS__MASK                                           0x00FF0000

/*  CRYPTOCAP_0 : CRYPTPCAP_0_RSVD1   */
#define CRYPTOCAP_0__CRYPTPCAP_0_RSVD1__SHIFT                           0x00000018
#define CRYPTOCAP_0__CRYPTPCAP_0_RSVD1__WIDTH                           0x00000008
#define CRYPTOCAP_0__CRYPTPCAP_0_RSVD1__MASK                            0xFF000000


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCAP_1 : ALGID               */
#define CRYPTOCAP_1__ALGID__SHIFT                                       0x00000000
#define CRYPTOCAP_1__ALGID__WIDTH                                       0x00000008
#define CRYPTOCAP_1__ALGID__MASK                                        0x000000FF

/*  CRYPTOCAP_1 : SDUSB               */
#define CRYPTOCAP_1__SDUSB__SHIFT                                       0x00000008
#define CRYPTOCAP_1__SDUSB__WIDTH                                       0x00000008
#define CRYPTOCAP_1__SDUSB__MASK                                        0x0000FF00

/*  CRYPTOCAP_1 : KS                  */
#define CRYPTOCAP_1__KS__SHIFT                                          0x00000010
#define CRYPTOCAP_1__KS__WIDTH                                          0x00000008
#define CRYPTOCAP_1__KS__MASK                                           0x00FF0000

/*  CRYPTOCAP_1 : CRYPTPCAP_1_RSVD1   */
#define CRYPTOCAP_1__CRYPTPCAP_1_RSVD1__SHIFT                           0x00000018
#define CRYPTOCAP_1__CRYPTPCAP_1_RSVD1__WIDTH                           0x00000008
#define CRYPTOCAP_1__CRYPTPCAP_1_RSVD1__MASK                            0xFF000000


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCAP_2 : ALGID               */
#define CRYPTOCAP_2__ALGID__SHIFT                                       0x00000000
#define CRYPTOCAP_2__ALGID__WIDTH                                       0x00000008
#define CRYPTOCAP_2__ALGID__MASK                                        0x000000FF

/*  CRYPTOCAP_2 : SDUSB               */
#define CRYPTOCAP_2__SDUSB__SHIFT                                       0x00000008
#define CRYPTOCAP_2__SDUSB__WIDTH                                       0x00000008
#define CRYPTOCAP_2__SDUSB__MASK                                        0x0000FF00

/*  CRYPTOCAP_2 : KS                  */
#define CRYPTOCAP_2__KS__SHIFT                                          0x00000010
#define CRYPTOCAP_2__KS__WIDTH                                          0x00000008
#define CRYPTOCAP_2__KS__MASK                                           0x00FF0000

/*  CRYPTOCAP_2 : CRYPTPCAP_2_RSVD1   */
#define CRYPTOCAP_2__CRYPTPCAP_2_RSVD1__SHIFT                           0x00000018
#define CRYPTOCAP_2__CRYPTPCAP_2_RSVD1__WIDTH                           0x00000008
#define CRYPTOCAP_2__CRYPTPCAP_2_RSVD1__MASK                            0xFF000000


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCAP_3 : ALGID               */
#define CRYPTOCAP_3__ALGID__SHIFT                                       0x00000000
#define CRYPTOCAP_3__ALGID__WIDTH                                       0x00000008
#define CRYPTOCAP_3__ALGID__MASK                                        0x000000FF

/*  CRYPTOCAP_3 : SDUSB               */
#define CRYPTOCAP_3__SDUSB__SHIFT                                       0x00000008
#define CRYPTOCAP_3__SDUSB__WIDTH                                       0x00000008
#define CRYPTOCAP_3__SDUSB__MASK                                        0x0000FF00

/*  CRYPTOCAP_3 : KS                  */
#define CRYPTOCAP_3__KS__SHIFT                                          0x00000010
#define CRYPTOCAP_3__KS__WIDTH                                          0x00000008
#define CRYPTOCAP_3__KS__MASK                                           0x00FF0000

/*  CRYPTOCAP_3 : CRYPTPCAP_3_RSVD1   */
#define CRYPTOCAP_3__CRYPTPCAP_3_RSVD1__SHIFT                           0x00000018
#define CRYPTOCAP_3__CRYPTPCAP_3_RSVD1__WIDTH                           0x00000008
#define CRYPTOCAP_3__CRYPTPCAP_3_RSVD1__MASK                            0xFF000000


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_0 : CRYPTOKEY_0_0       */
#define CRYPTOCFG_0_0__CRYPTOKEY_0_0__SHIFT                             0x00000000
#define CRYPTOCFG_0_0__CRYPTOKEY_0_0__WIDTH                             0x00000020
#define CRYPTOCFG_0_0__CRYPTOKEY_0_0__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_1 : CRYPTOKEY_0_1       */
#define CRYPTOCFG_0_1__CRYPTOKEY_0_1__SHIFT                             0x00000000
#define CRYPTOCFG_0_1__CRYPTOKEY_0_1__WIDTH                             0x00000020
#define CRYPTOCFG_0_1__CRYPTOKEY_0_1__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_2 : CRYPTOKEY_0_2       */
#define CRYPTOCFG_0_2__CRYPTOKEY_0_2__SHIFT                             0x00000000
#define CRYPTOCFG_0_2__CRYPTOKEY_0_2__WIDTH                             0x00000020
#define CRYPTOCFG_0_2__CRYPTOKEY_0_2__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_3 : CRYPTOKEY_0_3       */
#define CRYPTOCFG_0_3__CRYPTOKEY_0_3__SHIFT                             0x00000000
#define CRYPTOCFG_0_3__CRYPTOKEY_0_3__WIDTH                             0x00000020
#define CRYPTOCFG_0_3__CRYPTOKEY_0_3__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_4 : CRYPTOKEY_0_4       */
#define CRYPTOCFG_0_4__CRYPTOKEY_0_4__SHIFT                             0x00000000
#define CRYPTOCFG_0_4__CRYPTOKEY_0_4__WIDTH                             0x00000020
#define CRYPTOCFG_0_4__CRYPTOKEY_0_4__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_5 : CRYPTOKEY_0_5       */
#define CRYPTOCFG_0_5__CRYPTOKEY_0_5__SHIFT                             0x00000000
#define CRYPTOCFG_0_5__CRYPTOKEY_0_5__WIDTH                             0x00000020
#define CRYPTOCFG_0_5__CRYPTOKEY_0_5__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_6 : CRYPTOKEY_0_6       */
#define CRYPTOCFG_0_6__CRYPTOKEY_0_6__SHIFT                             0x00000000
#define CRYPTOCFG_0_6__CRYPTOKEY_0_6__WIDTH                             0x00000020
#define CRYPTOCFG_0_6__CRYPTOKEY_0_6__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_7 : CRYPTOKEY_0_7       */
#define CRYPTOCFG_0_7__CRYPTOKEY_0_7__SHIFT                             0x00000000
#define CRYPTOCFG_0_7__CRYPTOKEY_0_7__WIDTH                             0x00000020
#define CRYPTOCFG_0_7__CRYPTOKEY_0_7__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_8 : CRYPTOKEY_0_8       */
#define CRYPTOCFG_0_8__CRYPTOKEY_0_8__SHIFT                             0x00000000
#define CRYPTOCFG_0_8__CRYPTOKEY_0_8__WIDTH                             0x00000020
#define CRYPTOCFG_0_8__CRYPTOKEY_0_8__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_9 : CRYPTOKEY_0_9       */
#define CRYPTOCFG_0_9__CRYPTOKEY_0_9__SHIFT                             0x00000000
#define CRYPTOCFG_0_9__CRYPTOKEY_0_9__WIDTH                             0x00000020
#define CRYPTOCFG_0_9__CRYPTOKEY_0_9__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_10 : CRYPTOKEY_0_10      */
#define CRYPTOCFG_0_10__CRYPTOKEY_0_10__SHIFT                           0x00000000
#define CRYPTOCFG_0_10__CRYPTOKEY_0_10__WIDTH                           0x00000020
#define CRYPTOCFG_0_10__CRYPTOKEY_0_10__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_11 : CRYPTOKEY_0_11      */
#define CRYPTOCFG_0_11__CRYPTOKEY_0_11__SHIFT                           0x00000000
#define CRYPTOCFG_0_11__CRYPTOKEY_0_11__WIDTH                           0x00000020
#define CRYPTOCFG_0_11__CRYPTOKEY_0_11__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_12 : CRYPTOKEY_0_12      */
#define CRYPTOCFG_0_12__CRYPTOKEY_0_12__SHIFT                           0x00000000
#define CRYPTOCFG_0_12__CRYPTOKEY_0_12__WIDTH                           0x00000020
#define CRYPTOCFG_0_12__CRYPTOKEY_0_12__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_13 : CRYPTOKEY_0_13      */
#define CRYPTOCFG_0_13__CRYPTOKEY_0_13__SHIFT                           0x00000000
#define CRYPTOCFG_0_13__CRYPTOKEY_0_13__WIDTH                           0x00000020
#define CRYPTOCFG_0_13__CRYPTOKEY_0_13__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_14 : CRYPTOKEY_0_14      */
#define CRYPTOCFG_0_14__CRYPTOKEY_0_14__SHIFT                           0x00000000
#define CRYPTOCFG_0_14__CRYPTOKEY_0_14__WIDTH                           0x00000020
#define CRYPTOCFG_0_14__CRYPTOKEY_0_14__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_15 : CRYPTOKEY_0_15      */
#define CRYPTOCFG_0_15__CRYPTOKEY_0_15__SHIFT                           0x00000000
#define CRYPTOCFG_0_15__CRYPTOKEY_0_15__WIDTH                           0x00000020
#define CRYPTOCFG_0_15__CRYPTOKEY_0_15__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_16 : DUSIZE_0            */
#define CRYPTOCFG_0_16__DUSIZE_0__SHIFT                                 0x00000000
#define CRYPTOCFG_0_16__DUSIZE_0__WIDTH                                 0x00000008
#define CRYPTOCFG_0_16__DUSIZE_0__MASK                                  0x000000FF

/*  CRYPTOCFG_0_16 : CAPIDX_0            */
#define CRYPTOCFG_0_16__CAPIDX_0__SHIFT                                 0x00000008
#define CRYPTOCFG_0_16__CAPIDX_0__WIDTH                                 0x00000008
#define CRYPTOCFG_0_16__CAPIDX_0__MASK                                  0x0000FF00

/*  CRYPTOCFG_0_16 : CRYPTPCFG_RSVD_0    */
#define CRYPTOCFG_0_16__CRYPTPCFG_RSVD_0__SHIFT                         0x00000010
#define CRYPTOCFG_0_16__CRYPTPCFG_RSVD_0__WIDTH                         0x0000000F
#define CRYPTOCFG_0_16__CRYPTPCFG_RSVD_0__MASK                          0x7FFF0000

/*  CRYPTOCFG_0_16 : CFGE_0              */
#define CRYPTOCFG_0_16__CFGE_0__SHIFT                                   0x0000001F
#define CRYPTOCFG_0_16__CFGE_0__WIDTH                                   0x00000001
#define CRYPTOCFG_0_16__CFGE_0__MASK                                    0x80000000



/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_17 : RESERVED_0          */
#define CRYPTOCFG_0_17__RESERVED_0__SHIFT                               0x00000000
#define CRYPTOCFG_0_17__RESERVED_0__WIDTH                               0x00000020
#define CRYPTOCFG_0_17__RESERVED_0__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_18 : RESERVED_0          */
#define CRYPTOCFG_0_18__RESERVED_0__SHIFT                               0x00000000
#define CRYPTOCFG_0_18__RESERVED_0__WIDTH                               0x00000020
#define CRYPTOCFG_0_18__RESERVED_0__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_19 : RESERVED_0          */
#define CRYPTOCFG_0_19__RESERVED_0__SHIFT                               0x00000000
#define CRYPTOCFG_0_19__RESERVED_0__WIDTH                               0x00000020
#define CRYPTOCFG_0_19__RESERVED_0__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_20 : RESERVED_0          */
#define CRYPTOCFG_0_20__RESERVED_0__SHIFT                               0x00000000
#define CRYPTOCFG_0_20__RESERVED_0__WIDTH                               0x00000020
#define CRYPTOCFG_0_20__RESERVED_0__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_21 : RESERVED_0          */
#define CRYPTOCFG_0_21__RESERVED_0__SHIFT                               0x00000000
#define CRYPTOCFG_0_21__RESERVED_0__WIDTH                               0x00000020
#define CRYPTOCFG_0_21__RESERVED_0__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_22 : RESERVED_0          */
#define CRYPTOCFG_0_22__RESERVED_0__SHIFT                               0x00000000
#define CRYPTOCFG_0_22__RESERVED_0__WIDTH                               0x00000020
#define CRYPTOCFG_0_22__RESERVED_0__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_23 : RESERVED_0          */
#define CRYPTOCFG_0_23__RESERVED_0__SHIFT                               0x00000000
#define CRYPTOCFG_0_23__RESERVED_0__WIDTH                               0x00000020
#define CRYPTOCFG_0_23__RESERVED_0__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_24 : RESERVED_0          */
#define CRYPTOCFG_0_24__RESERVED_0__SHIFT                               0x00000000
#define CRYPTOCFG_0_24__RESERVED_0__WIDTH                               0x00000020
#define CRYPTOCFG_0_24__RESERVED_0__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_25 : RESERVED_0          */
#define CRYPTOCFG_0_25__RESERVED_0__SHIFT                               0x00000000
#define CRYPTOCFG_0_25__RESERVED_0__WIDTH                               0x00000020
#define CRYPTOCFG_0_25__RESERVED_0__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_26 : RESERVED_0          */
#define CRYPTOCFG_0_26__RESERVED_0__SHIFT                               0x00000000
#define CRYPTOCFG_0_26__RESERVED_0__WIDTH                               0x00000020
#define CRYPTOCFG_0_26__RESERVED_0__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_27 : RESERVED_0          */
#define CRYPTOCFG_0_27__RESERVED_0__SHIFT                               0x00000000
#define CRYPTOCFG_0_27__RESERVED_0__WIDTH                               0x00000020
#define CRYPTOCFG_0_27__RESERVED_0__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_28 : RESERVED_0          */
#define CRYPTOCFG_0_28__RESERVED_0__SHIFT                               0x00000000
#define CRYPTOCFG_0_28__RESERVED_0__WIDTH                               0x00000020
#define CRYPTOCFG_0_28__RESERVED_0__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_29 : RESERVED_0          */
#define CRYPTOCFG_0_29__RESERVED_0__SHIFT                               0x00000000
#define CRYPTOCFG_0_29__RESERVED_0__WIDTH                               0x00000020
#define CRYPTOCFG_0_29__RESERVED_0__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_30 : RESERVED_0          */
#define CRYPTOCFG_0_30__RESERVED_0__SHIFT                               0x00000000
#define CRYPTOCFG_0_30__RESERVED_0__WIDTH                               0x00000020
#define CRYPTOCFG_0_30__RESERVED_0__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_0_31 : RESERVED_0          */
#define CRYPTOCFG_0_31__RESERVED_0__SHIFT                               0x00000000
#define CRYPTOCFG_0_31__RESERVED_0__WIDTH                               0x00000020
#define CRYPTOCFG_0_31__RESERVED_0__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_0 : CRYPTOKEY_1_0       */
#define CRYPTOCFG_1_0__CRYPTOKEY_1_0__SHIFT                             0x00000000
#define CRYPTOCFG_1_0__CRYPTOKEY_1_0__WIDTH                             0x00000020
#define CRYPTOCFG_1_0__CRYPTOKEY_1_0__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_1 : CRYPTOKEY_1_1       */
#define CRYPTOCFG_1_1__CRYPTOKEY_1_1__SHIFT                             0x00000000
#define CRYPTOCFG_1_1__CRYPTOKEY_1_1__WIDTH                             0x00000020
#define CRYPTOCFG_1_1__CRYPTOKEY_1_1__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_2 : CRYPTOKEY_1_2       */
#define CRYPTOCFG_1_2__CRYPTOKEY_1_2__SHIFT                             0x00000000
#define CRYPTOCFG_1_2__CRYPTOKEY_1_2__WIDTH                             0x00000020
#define CRYPTOCFG_1_2__CRYPTOKEY_1_2__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_3 : CRYPTOKEY_1_3       */
#define CRYPTOCFG_1_3__CRYPTOKEY_1_3__SHIFT                             0x00000000
#define CRYPTOCFG_1_3__CRYPTOKEY_1_3__WIDTH                             0x00000020
#define CRYPTOCFG_1_3__CRYPTOKEY_1_3__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_4 : CRYPTOKEY_1_4       */
#define CRYPTOCFG_1_4__CRYPTOKEY_1_4__SHIFT                             0x00000000
#define CRYPTOCFG_1_4__CRYPTOKEY_1_4__WIDTH                             0x00000020
#define CRYPTOCFG_1_4__CRYPTOKEY_1_4__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_5 : CRYPTOKEY_1_5       */
#define CRYPTOCFG_1_5__CRYPTOKEY_1_5__SHIFT                             0x00000000
#define CRYPTOCFG_1_5__CRYPTOKEY_1_5__WIDTH                             0x00000020
#define CRYPTOCFG_1_5__CRYPTOKEY_1_5__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_6 : CRYPTOKEY_1_6       */
#define CRYPTOCFG_1_6__CRYPTOKEY_1_6__SHIFT                             0x00000000
#define CRYPTOCFG_1_6__CRYPTOKEY_1_6__WIDTH                             0x00000020
#define CRYPTOCFG_1_6__CRYPTOKEY_1_6__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_7 : CRYPTOKEY_1_7       */
#define CRYPTOCFG_1_7__CRYPTOKEY_1_7__SHIFT                             0x00000000
#define CRYPTOCFG_1_7__CRYPTOKEY_1_7__WIDTH                             0x00000020
#define CRYPTOCFG_1_7__CRYPTOKEY_1_7__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_8 : CRYPTOKEY_1_8       */
#define CRYPTOCFG_1_8__CRYPTOKEY_1_8__SHIFT                             0x00000000
#define CRYPTOCFG_1_8__CRYPTOKEY_1_8__WIDTH                             0x00000020
#define CRYPTOCFG_1_8__CRYPTOKEY_1_8__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_9 : CRYPTOKEY_1_9       */
#define CRYPTOCFG_1_9__CRYPTOKEY_1_9__SHIFT                             0x00000000
#define CRYPTOCFG_1_9__CRYPTOKEY_1_9__WIDTH                             0x00000020
#define CRYPTOCFG_1_9__CRYPTOKEY_1_9__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_10 : CRYPTOKEY_1_10      */
#define CRYPTOCFG_1_10__CRYPTOKEY_1_10__SHIFT                           0x00000000
#define CRYPTOCFG_1_10__CRYPTOKEY_1_10__WIDTH                           0x00000020
#define CRYPTOCFG_1_10__CRYPTOKEY_1_10__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_11 : CRYPTOKEY_1_11      */
#define CRYPTOCFG_1_11__CRYPTOKEY_1_11__SHIFT                           0x00000000
#define CRYPTOCFG_1_11__CRYPTOKEY_1_11__WIDTH                           0x00000020
#define CRYPTOCFG_1_11__CRYPTOKEY_1_11__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_12 : CRYPTOKEY_1_12      */
#define CRYPTOCFG_1_12__CRYPTOKEY_1_12__SHIFT                           0x00000000
#define CRYPTOCFG_1_12__CRYPTOKEY_1_12__WIDTH                           0x00000020
#define CRYPTOCFG_1_12__CRYPTOKEY_1_12__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_13 : CRYPTOKEY_1_13      */
#define CRYPTOCFG_1_13__CRYPTOKEY_1_13__SHIFT                           0x00000000
#define CRYPTOCFG_1_13__CRYPTOKEY_1_13__WIDTH                           0x00000020
#define CRYPTOCFG_1_13__CRYPTOKEY_1_13__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_14 : CRYPTOKEY_1_14      */
#define CRYPTOCFG_1_14__CRYPTOKEY_1_14__SHIFT                           0x00000000
#define CRYPTOCFG_1_14__CRYPTOKEY_1_14__WIDTH                           0x00000020
#define CRYPTOCFG_1_14__CRYPTOKEY_1_14__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_15 : CRYPTOKEY_1_15      */
#define CRYPTOCFG_1_15__CRYPTOKEY_1_15__SHIFT                           0x00000000
#define CRYPTOCFG_1_15__CRYPTOKEY_1_15__WIDTH                           0x00000020
#define CRYPTOCFG_1_15__CRYPTOKEY_1_15__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_16 : DUSIZE_1            */
#define CRYPTOCFG_1_16__DUSIZE_1__SHIFT                                 0x00000000
#define CRYPTOCFG_1_16__DUSIZE_1__WIDTH                                 0x00000008
#define CRYPTOCFG_1_16__DUSIZE_1__MASK                                  0x000000FF

/*  CRYPTOCFG_1_16 : CAPIDX_1            */
#define CRYPTOCFG_1_16__CAPIDX_1__SHIFT                                 0x00000008
#define CRYPTOCFG_1_16__CAPIDX_1__WIDTH                                 0x00000008
#define CRYPTOCFG_1_16__CAPIDX_1__MASK                                  0x0000FF00

/*  CRYPTOCFG_1_16 : CRYPTPCFG_RSVD_1    */
#define CRYPTOCFG_1_16__CRYPTPCFG_RSVD_1__SHIFT                         0x00000010
#define CRYPTOCFG_1_16__CRYPTPCFG_RSVD_1__WIDTH                         0x0000000F
#define CRYPTOCFG_1_16__CRYPTPCFG_RSVD_1__MASK                          0x7FFF0000

/*  CRYPTOCFG_1_16 : CFGE_1              */
#define CRYPTOCFG_1_16__CFGE_1__SHIFT                                   0x0000001F
#define CRYPTOCFG_1_16__CFGE_1__WIDTH                                   0x00000001
#define CRYPTOCFG_1_16__CFGE_1__MASK                                    0x80000000



/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_17 : RESERVED_1          */
#define CRYPTOCFG_1_17__RESERVED_1__SHIFT                               0x00000000
#define CRYPTOCFG_1_17__RESERVED_1__WIDTH                               0x00000020
#define CRYPTOCFG_1_17__RESERVED_1__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_18 : RESERVED_1          */
#define CRYPTOCFG_1_18__RESERVED_1__SHIFT                               0x00000000
#define CRYPTOCFG_1_18__RESERVED_1__WIDTH                               0x00000020
#define CRYPTOCFG_1_18__RESERVED_1__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_19 : RESERVED_1          */
#define CRYPTOCFG_1_19__RESERVED_1__SHIFT                               0x00000000
#define CRYPTOCFG_1_19__RESERVED_1__WIDTH                               0x00000020
#define CRYPTOCFG_1_19__RESERVED_1__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_20 : RESERVED_1          */
#define CRYPTOCFG_1_20__RESERVED_1__SHIFT                               0x00000000
#define CRYPTOCFG_1_20__RESERVED_1__WIDTH                               0x00000020
#define CRYPTOCFG_1_20__RESERVED_1__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_21 : RESERVED_1          */
#define CRYPTOCFG_1_21__RESERVED_1__SHIFT                               0x00000000
#define CRYPTOCFG_1_21__RESERVED_1__WIDTH                               0x00000020
#define CRYPTOCFG_1_21__RESERVED_1__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_22 : RESERVED_1          */
#define CRYPTOCFG_1_22__RESERVED_1__SHIFT                               0x00000000
#define CRYPTOCFG_1_22__RESERVED_1__WIDTH                               0x00000020
#define CRYPTOCFG_1_22__RESERVED_1__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_23 : RESERVED_1          */
#define CRYPTOCFG_1_23__RESERVED_1__SHIFT                               0x00000000
#define CRYPTOCFG_1_23__RESERVED_1__WIDTH                               0x00000020
#define CRYPTOCFG_1_23__RESERVED_1__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_24 : RESERVED_1          */
#define CRYPTOCFG_1_24__RESERVED_1__SHIFT                               0x00000000
#define CRYPTOCFG_1_24__RESERVED_1__WIDTH                               0x00000020
#define CRYPTOCFG_1_24__RESERVED_1__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_25 : RESERVED_1          */
#define CRYPTOCFG_1_25__RESERVED_1__SHIFT                               0x00000000
#define CRYPTOCFG_1_25__RESERVED_1__WIDTH                               0x00000020
#define CRYPTOCFG_1_25__RESERVED_1__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_26 : RESERVED_1          */
#define CRYPTOCFG_1_26__RESERVED_1__SHIFT                               0x00000000
#define CRYPTOCFG_1_26__RESERVED_1__WIDTH                               0x00000020
#define CRYPTOCFG_1_26__RESERVED_1__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_27 : RESERVED_1          */
#define CRYPTOCFG_1_27__RESERVED_1__SHIFT                               0x00000000
#define CRYPTOCFG_1_27__RESERVED_1__WIDTH                               0x00000020
#define CRYPTOCFG_1_27__RESERVED_1__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_28 : RESERVED_1          */
#define CRYPTOCFG_1_28__RESERVED_1__SHIFT                               0x00000000
#define CRYPTOCFG_1_28__RESERVED_1__WIDTH                               0x00000020
#define CRYPTOCFG_1_28__RESERVED_1__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_29 : RESERVED_1          */
#define CRYPTOCFG_1_29__RESERVED_1__SHIFT                               0x00000000
#define CRYPTOCFG_1_29__RESERVED_1__WIDTH                               0x00000020
#define CRYPTOCFG_1_29__RESERVED_1__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_30 : RESERVED_1          */
#define CRYPTOCFG_1_30__RESERVED_1__SHIFT                               0x00000000
#define CRYPTOCFG_1_30__RESERVED_1__WIDTH                               0x00000020
#define CRYPTOCFG_1_30__RESERVED_1__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_1_31 : RESERVED_1          */
#define CRYPTOCFG_1_31__RESERVED_1__SHIFT                               0x00000000
#define CRYPTOCFG_1_31__RESERVED_1__WIDTH                               0x00000020
#define CRYPTOCFG_1_31__RESERVED_1__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_0 : CRYPTOKEY_2_0       */
#define CRYPTOCFG_2_0__CRYPTOKEY_2_0__SHIFT                             0x00000000
#define CRYPTOCFG_2_0__CRYPTOKEY_2_0__WIDTH                             0x00000020
#define CRYPTOCFG_2_0__CRYPTOKEY_2_0__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_1 : CRYPTOKEY_2_1       */
#define CRYPTOCFG_2_1__CRYPTOKEY_2_1__SHIFT                             0x00000000
#define CRYPTOCFG_2_1__CRYPTOKEY_2_1__WIDTH                             0x00000020
#define CRYPTOCFG_2_1__CRYPTOKEY_2_1__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_2 : CRYPTOKEY_2_2       */
#define CRYPTOCFG_2_2__CRYPTOKEY_2_2__SHIFT                             0x00000000
#define CRYPTOCFG_2_2__CRYPTOKEY_2_2__WIDTH                             0x00000020
#define CRYPTOCFG_2_2__CRYPTOKEY_2_2__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_3 : CRYPTOKEY_2_3       */
#define CRYPTOCFG_2_3__CRYPTOKEY_2_3__SHIFT                             0x00000000
#define CRYPTOCFG_2_3__CRYPTOKEY_2_3__WIDTH                             0x00000020
#define CRYPTOCFG_2_3__CRYPTOKEY_2_3__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_4 : CRYPTOKEY_2_4       */
#define CRYPTOCFG_2_4__CRYPTOKEY_2_4__SHIFT                             0x00000000
#define CRYPTOCFG_2_4__CRYPTOKEY_2_4__WIDTH                             0x00000020
#define CRYPTOCFG_2_4__CRYPTOKEY_2_4__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_5 : CRYPTOKEY_2_5       */
#define CRYPTOCFG_2_5__CRYPTOKEY_2_5__SHIFT                             0x00000000
#define CRYPTOCFG_2_5__CRYPTOKEY_2_5__WIDTH                             0x00000020
#define CRYPTOCFG_2_5__CRYPTOKEY_2_5__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_6 : CRYPTOKEY_2_6       */
#define CRYPTOCFG_2_6__CRYPTOKEY_2_6__SHIFT                             0x00000000
#define CRYPTOCFG_2_6__CRYPTOKEY_2_6__WIDTH                             0x00000020
#define CRYPTOCFG_2_6__CRYPTOKEY_2_6__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_7 : CRYPTOKEY_2_7       */
#define CRYPTOCFG_2_7__CRYPTOKEY_2_7__SHIFT                             0x00000000
#define CRYPTOCFG_2_7__CRYPTOKEY_2_7__WIDTH                             0x00000020
#define CRYPTOCFG_2_7__CRYPTOKEY_2_7__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_8 : CRYPTOKEY_2_8       */
#define CRYPTOCFG_2_8__CRYPTOKEY_2_8__SHIFT                             0x00000000
#define CRYPTOCFG_2_8__CRYPTOKEY_2_8__WIDTH                             0x00000020
#define CRYPTOCFG_2_8__CRYPTOKEY_2_8__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_9 : CRYPTOKEY_2_9       */
#define CRYPTOCFG_2_9__CRYPTOKEY_2_9__SHIFT                             0x00000000
#define CRYPTOCFG_2_9__CRYPTOKEY_2_9__WIDTH                             0x00000020
#define CRYPTOCFG_2_9__CRYPTOKEY_2_9__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_10 : CRYPTOKEY_2_10      */
#define CRYPTOCFG_2_10__CRYPTOKEY_2_10__SHIFT                           0x00000000
#define CRYPTOCFG_2_10__CRYPTOKEY_2_10__WIDTH                           0x00000020
#define CRYPTOCFG_2_10__CRYPTOKEY_2_10__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_11 : CRYPTOKEY_2_11      */
#define CRYPTOCFG_2_11__CRYPTOKEY_2_11__SHIFT                           0x00000000
#define CRYPTOCFG_2_11__CRYPTOKEY_2_11__WIDTH                           0x00000020
#define CRYPTOCFG_2_11__CRYPTOKEY_2_11__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_12 : CRYPTOKEY_2_12      */
#define CRYPTOCFG_2_12__CRYPTOKEY_2_12__SHIFT                           0x00000000
#define CRYPTOCFG_2_12__CRYPTOKEY_2_12__WIDTH                           0x00000020
#define CRYPTOCFG_2_12__CRYPTOKEY_2_12__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_13 : CRYPTOKEY_2_13      */
#define CRYPTOCFG_2_13__CRYPTOKEY_2_13__SHIFT                           0x00000000
#define CRYPTOCFG_2_13__CRYPTOKEY_2_13__WIDTH                           0x00000020
#define CRYPTOCFG_2_13__CRYPTOKEY_2_13__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_14 : CRYPTOKEY_2_14      */
#define CRYPTOCFG_2_14__CRYPTOKEY_2_14__SHIFT                           0x00000000
#define CRYPTOCFG_2_14__CRYPTOKEY_2_14__WIDTH                           0x00000020
#define CRYPTOCFG_2_14__CRYPTOKEY_2_14__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_15 : CRYPTOKEY_2_15      */
#define CRYPTOCFG_2_15__CRYPTOKEY_2_15__SHIFT                           0x00000000
#define CRYPTOCFG_2_15__CRYPTOKEY_2_15__WIDTH                           0x00000020
#define CRYPTOCFG_2_15__CRYPTOKEY_2_15__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_16 : DUSIZE_2            */
#define CRYPTOCFG_2_16__DUSIZE_2__SHIFT                                 0x00000000
#define CRYPTOCFG_2_16__DUSIZE_2__WIDTH                                 0x00000008
#define CRYPTOCFG_2_16__DUSIZE_2__MASK                                  0x000000FF

/*  CRYPTOCFG_2_16 : CAPIDX_2            */
#define CRYPTOCFG_2_16__CAPIDX_2__SHIFT                                 0x00000008
#define CRYPTOCFG_2_16__CAPIDX_2__WIDTH                                 0x00000008
#define CRYPTOCFG_2_16__CAPIDX_2__MASK                                  0x0000FF00

/*  CRYPTOCFG_2_16 : CRYPTPCFG_RSVD_2    */
#define CRYPTOCFG_2_16__CRYPTPCFG_RSVD_2__SHIFT                         0x00000010
#define CRYPTOCFG_2_16__CRYPTPCFG_RSVD_2__WIDTH                         0x0000000F
#define CRYPTOCFG_2_16__CRYPTPCFG_RSVD_2__MASK                          0x7FFF0000

/*  CRYPTOCFG_2_16 : CFGE_2              */
#define CRYPTOCFG_2_16__CFGE_2__SHIFT                                   0x0000001F
#define CRYPTOCFG_2_16__CFGE_2__WIDTH                                   0x00000001
#define CRYPTOCFG_2_16__CFGE_2__MASK                                    0x80000000



/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_17 : RESERVED_2          */
#define CRYPTOCFG_2_17__RESERVED_2__SHIFT                               0x00000000
#define CRYPTOCFG_2_17__RESERVED_2__WIDTH                               0x00000020
#define CRYPTOCFG_2_17__RESERVED_2__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_18 : RESERVED_2          */
#define CRYPTOCFG_2_18__RESERVED_2__SHIFT                               0x00000000
#define CRYPTOCFG_2_18__RESERVED_2__WIDTH                               0x00000020
#define CRYPTOCFG_2_18__RESERVED_2__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_19 : RESERVED_2          */
#define CRYPTOCFG_2_19__RESERVED_2__SHIFT                               0x00000000
#define CRYPTOCFG_2_19__RESERVED_2__WIDTH                               0x00000020
#define CRYPTOCFG_2_19__RESERVED_2__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_20 : RESERVED_2          */
#define CRYPTOCFG_2_20__RESERVED_2__SHIFT                               0x00000000
#define CRYPTOCFG_2_20__RESERVED_2__WIDTH                               0x00000020
#define CRYPTOCFG_2_20__RESERVED_2__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_21 : RESERVED_2          */
#define CRYPTOCFG_2_21__RESERVED_2__SHIFT                               0x00000000
#define CRYPTOCFG_2_21__RESERVED_2__WIDTH                               0x00000020
#define CRYPTOCFG_2_21__RESERVED_2__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_22 : RESERVED_2          */
#define CRYPTOCFG_2_22__RESERVED_2__SHIFT                               0x00000000
#define CRYPTOCFG_2_22__RESERVED_2__WIDTH                               0x00000020
#define CRYPTOCFG_2_22__RESERVED_2__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_23 : RESERVED_2          */
#define CRYPTOCFG_2_23__RESERVED_2__SHIFT                               0x00000000
#define CRYPTOCFG_2_23__RESERVED_2__WIDTH                               0x00000020
#define CRYPTOCFG_2_23__RESERVED_2__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_24 : RESERVED_2          */
#define CRYPTOCFG_2_24__RESERVED_2__SHIFT                               0x00000000
#define CRYPTOCFG_2_24__RESERVED_2__WIDTH                               0x00000020
#define CRYPTOCFG_2_24__RESERVED_2__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_25 : RESERVED_2          */
#define CRYPTOCFG_2_25__RESERVED_2__SHIFT                               0x00000000
#define CRYPTOCFG_2_25__RESERVED_2__WIDTH                               0x00000020
#define CRYPTOCFG_2_25__RESERVED_2__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_26 : RESERVED_2          */
#define CRYPTOCFG_2_26__RESERVED_2__SHIFT                               0x00000000
#define CRYPTOCFG_2_26__RESERVED_2__WIDTH                               0x00000020
#define CRYPTOCFG_2_26__RESERVED_2__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_27 : RESERVED_2          */
#define CRYPTOCFG_2_27__RESERVED_2__SHIFT                               0x00000000
#define CRYPTOCFG_2_27__RESERVED_2__WIDTH                               0x00000020
#define CRYPTOCFG_2_27__RESERVED_2__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_28 : RESERVED_2          */
#define CRYPTOCFG_2_28__RESERVED_2__SHIFT                               0x00000000
#define CRYPTOCFG_2_28__RESERVED_2__WIDTH                               0x00000020
#define CRYPTOCFG_2_28__RESERVED_2__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_29 : RESERVED_2          */
#define CRYPTOCFG_2_29__RESERVED_2__SHIFT                               0x00000000
#define CRYPTOCFG_2_29__RESERVED_2__WIDTH                               0x00000020
#define CRYPTOCFG_2_29__RESERVED_2__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_30 : RESERVED_2          */
#define CRYPTOCFG_2_30__RESERVED_2__SHIFT                               0x00000000
#define CRYPTOCFG_2_30__RESERVED_2__WIDTH                               0x00000020
#define CRYPTOCFG_2_30__RESERVED_2__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_2_31 : RESERVED_2          */
#define CRYPTOCFG_2_31__RESERVED_2__SHIFT                               0x00000000
#define CRYPTOCFG_2_31__RESERVED_2__WIDTH                               0x00000020
#define CRYPTOCFG_2_31__RESERVED_2__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_0 : CRYPTOKEY_3_0       */
#define CRYPTOCFG_3_0__CRYPTOKEY_3_0__SHIFT                             0x00000000
#define CRYPTOCFG_3_0__CRYPTOKEY_3_0__WIDTH                             0x00000020
#define CRYPTOCFG_3_0__CRYPTOKEY_3_0__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_1 : CRYPTOKEY_3_1       */
#define CRYPTOCFG_3_1__CRYPTOKEY_3_1__SHIFT                             0x00000000
#define CRYPTOCFG_3_1__CRYPTOKEY_3_1__WIDTH                             0x00000020
#define CRYPTOCFG_3_1__CRYPTOKEY_3_1__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_2 : CRYPTOKEY_3_2       */
#define CRYPTOCFG_3_2__CRYPTOKEY_3_2__SHIFT                             0x00000000
#define CRYPTOCFG_3_2__CRYPTOKEY_3_2__WIDTH                             0x00000020
#define CRYPTOCFG_3_2__CRYPTOKEY_3_2__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_3 : CRYPTOKEY_3_3       */
#define CRYPTOCFG_3_3__CRYPTOKEY_3_3__SHIFT                             0x00000000
#define CRYPTOCFG_3_3__CRYPTOKEY_3_3__WIDTH                             0x00000020
#define CRYPTOCFG_3_3__CRYPTOKEY_3_3__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_4 : CRYPTOKEY_3_4       */
#define CRYPTOCFG_3_4__CRYPTOKEY_3_4__SHIFT                             0x00000000
#define CRYPTOCFG_3_4__CRYPTOKEY_3_4__WIDTH                             0x00000020
#define CRYPTOCFG_3_4__CRYPTOKEY_3_4__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_5 : CRYPTOKEY_3_5       */
#define CRYPTOCFG_3_5__CRYPTOKEY_3_5__SHIFT                             0x00000000
#define CRYPTOCFG_3_5__CRYPTOKEY_3_5__WIDTH                             0x00000020
#define CRYPTOCFG_3_5__CRYPTOKEY_3_5__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_6 : CRYPTOKEY_3_6       */
#define CRYPTOCFG_3_6__CRYPTOKEY_3_6__SHIFT                             0x00000000
#define CRYPTOCFG_3_6__CRYPTOKEY_3_6__WIDTH                             0x00000020
#define CRYPTOCFG_3_6__CRYPTOKEY_3_6__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_7 : CRYPTOKEY_3_7       */
#define CRYPTOCFG_3_7__CRYPTOKEY_3_7__SHIFT                             0x00000000
#define CRYPTOCFG_3_7__CRYPTOKEY_3_7__WIDTH                             0x00000020
#define CRYPTOCFG_3_7__CRYPTOKEY_3_7__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_8 : CRYPTOKEY_3_8       */
#define CRYPTOCFG_3_8__CRYPTOKEY_3_8__SHIFT                             0x00000000
#define CRYPTOCFG_3_8__CRYPTOKEY_3_8__WIDTH                             0x00000020
#define CRYPTOCFG_3_8__CRYPTOKEY_3_8__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_9 : CRYPTOKEY_3_9       */
#define CRYPTOCFG_3_9__CRYPTOKEY_3_9__SHIFT                             0x00000000
#define CRYPTOCFG_3_9__CRYPTOKEY_3_9__WIDTH                             0x00000020
#define CRYPTOCFG_3_9__CRYPTOKEY_3_9__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_10 : CRYPTOKEY_3_10      */
#define CRYPTOCFG_3_10__CRYPTOKEY_3_10__SHIFT                           0x00000000
#define CRYPTOCFG_3_10__CRYPTOKEY_3_10__WIDTH                           0x00000020
#define CRYPTOCFG_3_10__CRYPTOKEY_3_10__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_11 : CRYPTOKEY_3_11      */
#define CRYPTOCFG_3_11__CRYPTOKEY_3_11__SHIFT                           0x00000000
#define CRYPTOCFG_3_11__CRYPTOKEY_3_11__WIDTH                           0x00000020
#define CRYPTOCFG_3_11__CRYPTOKEY_3_11__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_12 : CRYPTOKEY_3_12      */
#define CRYPTOCFG_3_12__CRYPTOKEY_3_12__SHIFT                           0x00000000
#define CRYPTOCFG_3_12__CRYPTOKEY_3_12__WIDTH                           0x00000020
#define CRYPTOCFG_3_12__CRYPTOKEY_3_12__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_13 : CRYPTOKEY_3_13      */
#define CRYPTOCFG_3_13__CRYPTOKEY_3_13__SHIFT                           0x00000000
#define CRYPTOCFG_3_13__CRYPTOKEY_3_13__WIDTH                           0x00000020
#define CRYPTOCFG_3_13__CRYPTOKEY_3_13__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_14 : CRYPTOKEY_3_14      */
#define CRYPTOCFG_3_14__CRYPTOKEY_3_14__SHIFT                           0x00000000
#define CRYPTOCFG_3_14__CRYPTOKEY_3_14__WIDTH                           0x00000020
#define CRYPTOCFG_3_14__CRYPTOKEY_3_14__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_15 : CRYPTOKEY_3_15      */
#define CRYPTOCFG_3_15__CRYPTOKEY_3_15__SHIFT                           0x00000000
#define CRYPTOCFG_3_15__CRYPTOKEY_3_15__WIDTH                           0x00000020
#define CRYPTOCFG_3_15__CRYPTOKEY_3_15__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_16 : DUSIZE_3            */
#define CRYPTOCFG_3_16__DUSIZE_3__SHIFT                                 0x00000000
#define CRYPTOCFG_3_16__DUSIZE_3__WIDTH                                 0x00000008
#define CRYPTOCFG_3_16__DUSIZE_3__MASK                                  0x000000FF

/*  CRYPTOCFG_3_16 : CAPIDX_3            */
#define CRYPTOCFG_3_16__CAPIDX_3__SHIFT                                 0x00000008
#define CRYPTOCFG_3_16__CAPIDX_3__WIDTH                                 0x00000008
#define CRYPTOCFG_3_16__CAPIDX_3__MASK                                  0x0000FF00

/*  CRYPTOCFG_3_16 : CRYPTPCFG_RSVD_3    */
#define CRYPTOCFG_3_16__CRYPTPCFG_RSVD_3__SHIFT                         0x00000010
#define CRYPTOCFG_3_16__CRYPTPCFG_RSVD_3__WIDTH                         0x0000000F
#define CRYPTOCFG_3_16__CRYPTPCFG_RSVD_3__MASK                          0x7FFF0000

/*  CRYPTOCFG_3_16 : CFGE_3              */
#define CRYPTOCFG_3_16__CFGE_3__SHIFT                                   0x0000001F
#define CRYPTOCFG_3_16__CFGE_3__WIDTH                                   0x00000001
#define CRYPTOCFG_3_16__CFGE_3__MASK                                    0x80000000



/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_17 : RESERVED_3          */
#define CRYPTOCFG_3_17__RESERVED_3__SHIFT                               0x00000000
#define CRYPTOCFG_3_17__RESERVED_3__WIDTH                               0x00000020
#define CRYPTOCFG_3_17__RESERVED_3__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_18 : RESERVED_3          */
#define CRYPTOCFG_3_18__RESERVED_3__SHIFT                               0x00000000
#define CRYPTOCFG_3_18__RESERVED_3__WIDTH                               0x00000020
#define CRYPTOCFG_3_18__RESERVED_3__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_19 : RESERVED_3          */
#define CRYPTOCFG_3_19__RESERVED_3__SHIFT                               0x00000000
#define CRYPTOCFG_3_19__RESERVED_3__WIDTH                               0x00000020
#define CRYPTOCFG_3_19__RESERVED_3__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_20 : RESERVED_3          */
#define CRYPTOCFG_3_20__RESERVED_3__SHIFT                               0x00000000
#define CRYPTOCFG_3_20__RESERVED_3__WIDTH                               0x00000020
#define CRYPTOCFG_3_20__RESERVED_3__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_21 : RESERVED_3          */
#define CRYPTOCFG_3_21__RESERVED_3__SHIFT                               0x00000000
#define CRYPTOCFG_3_21__RESERVED_3__WIDTH                               0x00000020
#define CRYPTOCFG_3_21__RESERVED_3__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_22 : RESERVED_3          */
#define CRYPTOCFG_3_22__RESERVED_3__SHIFT                               0x00000000
#define CRYPTOCFG_3_22__RESERVED_3__WIDTH                               0x00000020
#define CRYPTOCFG_3_22__RESERVED_3__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_23 : RESERVED_3          */
#define CRYPTOCFG_3_23__RESERVED_3__SHIFT                               0x00000000
#define CRYPTOCFG_3_23__RESERVED_3__WIDTH                               0x00000020
#define CRYPTOCFG_3_23__RESERVED_3__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_24 : RESERVED_3          */
#define CRYPTOCFG_3_24__RESERVED_3__SHIFT                               0x00000000
#define CRYPTOCFG_3_24__RESERVED_3__WIDTH                               0x00000020
#define CRYPTOCFG_3_24__RESERVED_3__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_25 : RESERVED_3          */
#define CRYPTOCFG_3_25__RESERVED_3__SHIFT                               0x00000000
#define CRYPTOCFG_3_25__RESERVED_3__WIDTH                               0x00000020
#define CRYPTOCFG_3_25__RESERVED_3__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_26 : RESERVED_3          */
#define CRYPTOCFG_3_26__RESERVED_3__SHIFT                               0x00000000
#define CRYPTOCFG_3_26__RESERVED_3__WIDTH                               0x00000020
#define CRYPTOCFG_3_26__RESERVED_3__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_27 : RESERVED_3          */
#define CRYPTOCFG_3_27__RESERVED_3__SHIFT                               0x00000000
#define CRYPTOCFG_3_27__RESERVED_3__WIDTH                               0x00000020
#define CRYPTOCFG_3_27__RESERVED_3__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_28 : RESERVED_3          */
#define CRYPTOCFG_3_28__RESERVED_3__SHIFT                               0x00000000
#define CRYPTOCFG_3_28__RESERVED_3__WIDTH                               0x00000020
#define CRYPTOCFG_3_28__RESERVED_3__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_29 : RESERVED_3          */
#define CRYPTOCFG_3_29__RESERVED_3__SHIFT                               0x00000000
#define CRYPTOCFG_3_29__RESERVED_3__WIDTH                               0x00000020
#define CRYPTOCFG_3_29__RESERVED_3__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_30 : RESERVED_3          */
#define CRYPTOCFG_3_30__RESERVED_3__SHIFT                               0x00000000
#define CRYPTOCFG_3_30__RESERVED_3__WIDTH                               0x00000020
#define CRYPTOCFG_3_30__RESERVED_3__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_3_31 : RESERVED_3          */
#define CRYPTOCFG_3_31__RESERVED_3__SHIFT                               0x00000000
#define CRYPTOCFG_3_31__RESERVED_3__WIDTH                               0x00000020
#define CRYPTOCFG_3_31__RESERVED_3__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_0 : CRYPTOKEY_4_0       */
#define CRYPTOCFG_4_0__CRYPTOKEY_4_0__SHIFT                             0x00000000
#define CRYPTOCFG_4_0__CRYPTOKEY_4_0__WIDTH                             0x00000020
#define CRYPTOCFG_4_0__CRYPTOKEY_4_0__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_1 : CRYPTOKEY_4_1       */
#define CRYPTOCFG_4_1__CRYPTOKEY_4_1__SHIFT                             0x00000000
#define CRYPTOCFG_4_1__CRYPTOKEY_4_1__WIDTH                             0x00000020
#define CRYPTOCFG_4_1__CRYPTOKEY_4_1__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_2 : CRYPTOKEY_4_2       */
#define CRYPTOCFG_4_2__CRYPTOKEY_4_2__SHIFT                             0x00000000
#define CRYPTOCFG_4_2__CRYPTOKEY_4_2__WIDTH                             0x00000020
#define CRYPTOCFG_4_2__CRYPTOKEY_4_2__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_3 : CRYPTOKEY_4_3       */
#define CRYPTOCFG_4_3__CRYPTOKEY_4_3__SHIFT                             0x00000000
#define CRYPTOCFG_4_3__CRYPTOKEY_4_3__WIDTH                             0x00000020
#define CRYPTOCFG_4_3__CRYPTOKEY_4_3__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_4 : CRYPTOKEY_4_4       */
#define CRYPTOCFG_4_4__CRYPTOKEY_4_4__SHIFT                             0x00000000
#define CRYPTOCFG_4_4__CRYPTOKEY_4_4__WIDTH                             0x00000020
#define CRYPTOCFG_4_4__CRYPTOKEY_4_4__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_5 : CRYPTOKEY_4_5       */
#define CRYPTOCFG_4_5__CRYPTOKEY_4_5__SHIFT                             0x00000000
#define CRYPTOCFG_4_5__CRYPTOKEY_4_5__WIDTH                             0x00000020
#define CRYPTOCFG_4_5__CRYPTOKEY_4_5__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_6 : CRYPTOKEY_4_6       */
#define CRYPTOCFG_4_6__CRYPTOKEY_4_6__SHIFT                             0x00000000
#define CRYPTOCFG_4_6__CRYPTOKEY_4_6__WIDTH                             0x00000020
#define CRYPTOCFG_4_6__CRYPTOKEY_4_6__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_7 : CRYPTOKEY_4_7       */
#define CRYPTOCFG_4_7__CRYPTOKEY_4_7__SHIFT                             0x00000000
#define CRYPTOCFG_4_7__CRYPTOKEY_4_7__WIDTH                             0x00000020
#define CRYPTOCFG_4_7__CRYPTOKEY_4_7__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_8 : CRYPTOKEY_4_8       */
#define CRYPTOCFG_4_8__CRYPTOKEY_4_8__SHIFT                             0x00000000
#define CRYPTOCFG_4_8__CRYPTOKEY_4_8__WIDTH                             0x00000020
#define CRYPTOCFG_4_8__CRYPTOKEY_4_8__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_9 : CRYPTOKEY_4_9       */
#define CRYPTOCFG_4_9__CRYPTOKEY_4_9__SHIFT                             0x00000000
#define CRYPTOCFG_4_9__CRYPTOKEY_4_9__WIDTH                             0x00000020
#define CRYPTOCFG_4_9__CRYPTOKEY_4_9__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_10 : CRYPTOKEY_4_10      */
#define CRYPTOCFG_4_10__CRYPTOKEY_4_10__SHIFT                           0x00000000
#define CRYPTOCFG_4_10__CRYPTOKEY_4_10__WIDTH                           0x00000020
#define CRYPTOCFG_4_10__CRYPTOKEY_4_10__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_11 : CRYPTOKEY_4_11      */
#define CRYPTOCFG_4_11__CRYPTOKEY_4_11__SHIFT                           0x00000000
#define CRYPTOCFG_4_11__CRYPTOKEY_4_11__WIDTH                           0x00000020
#define CRYPTOCFG_4_11__CRYPTOKEY_4_11__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_12 : CRYPTOKEY_4_12      */
#define CRYPTOCFG_4_12__CRYPTOKEY_4_12__SHIFT                           0x00000000
#define CRYPTOCFG_4_12__CRYPTOKEY_4_12__WIDTH                           0x00000020
#define CRYPTOCFG_4_12__CRYPTOKEY_4_12__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_13 : CRYPTOKEY_4_13      */
#define CRYPTOCFG_4_13__CRYPTOKEY_4_13__SHIFT                           0x00000000
#define CRYPTOCFG_4_13__CRYPTOKEY_4_13__WIDTH                           0x00000020
#define CRYPTOCFG_4_13__CRYPTOKEY_4_13__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_14 : CRYPTOKEY_4_14      */
#define CRYPTOCFG_4_14__CRYPTOKEY_4_14__SHIFT                           0x00000000
#define CRYPTOCFG_4_14__CRYPTOKEY_4_14__WIDTH                           0x00000020
#define CRYPTOCFG_4_14__CRYPTOKEY_4_14__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_15 : CRYPTOKEY_4_15      */
#define CRYPTOCFG_4_15__CRYPTOKEY_4_15__SHIFT                           0x00000000
#define CRYPTOCFG_4_15__CRYPTOKEY_4_15__WIDTH                           0x00000020
#define CRYPTOCFG_4_15__CRYPTOKEY_4_15__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_16 : DUSIZE_4            */
#define CRYPTOCFG_4_16__DUSIZE_4__SHIFT                                 0x00000000
#define CRYPTOCFG_4_16__DUSIZE_4__WIDTH                                 0x00000008
#define CRYPTOCFG_4_16__DUSIZE_4__MASK                                  0x000000FF

/*  CRYPTOCFG_4_16 : CAPIDX_4            */
#define CRYPTOCFG_4_16__CAPIDX_4__SHIFT                                 0x00000008
#define CRYPTOCFG_4_16__CAPIDX_4__WIDTH                                 0x00000008
#define CRYPTOCFG_4_16__CAPIDX_4__MASK                                  0x0000FF00

/*  CRYPTOCFG_4_16 : CRYPTPCFG_RSVD_4    */
#define CRYPTOCFG_4_16__CRYPTPCFG_RSVD_4__SHIFT                         0x00000010
#define CRYPTOCFG_4_16__CRYPTPCFG_RSVD_4__WIDTH                         0x0000000F
#define CRYPTOCFG_4_16__CRYPTPCFG_RSVD_4__MASK                          0x7FFF0000

/*  CRYPTOCFG_4_16 : CFGE_4              */
#define CRYPTOCFG_4_16__CFGE_4__SHIFT                                   0x0000001F
#define CRYPTOCFG_4_16__CFGE_4__WIDTH                                   0x00000001
#define CRYPTOCFG_4_16__CFGE_4__MASK                                    0x80000000



/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_17 : RESERVED_4          */
#define CRYPTOCFG_4_17__RESERVED_4__SHIFT                               0x00000000
#define CRYPTOCFG_4_17__RESERVED_4__WIDTH                               0x00000020
#define CRYPTOCFG_4_17__RESERVED_4__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_18 : RESERVED_4          */
#define CRYPTOCFG_4_18__RESERVED_4__SHIFT                               0x00000000
#define CRYPTOCFG_4_18__RESERVED_4__WIDTH                               0x00000020
#define CRYPTOCFG_4_18__RESERVED_4__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_19 : RESERVED_4          */
#define CRYPTOCFG_4_19__RESERVED_4__SHIFT                               0x00000000
#define CRYPTOCFG_4_19__RESERVED_4__WIDTH                               0x00000020
#define CRYPTOCFG_4_19__RESERVED_4__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_20 : RESERVED_4          */
#define CRYPTOCFG_4_20__RESERVED_4__SHIFT                               0x00000000
#define CRYPTOCFG_4_20__RESERVED_4__WIDTH                               0x00000020
#define CRYPTOCFG_4_20__RESERVED_4__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_21 : RESERVED_4          */
#define CRYPTOCFG_4_21__RESERVED_4__SHIFT                               0x00000000
#define CRYPTOCFG_4_21__RESERVED_4__WIDTH                               0x00000020
#define CRYPTOCFG_4_21__RESERVED_4__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_22 : RESERVED_4          */
#define CRYPTOCFG_4_22__RESERVED_4__SHIFT                               0x00000000
#define CRYPTOCFG_4_22__RESERVED_4__WIDTH                               0x00000020
#define CRYPTOCFG_4_22__RESERVED_4__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_23 : RESERVED_4          */
#define CRYPTOCFG_4_23__RESERVED_4__SHIFT                               0x00000000
#define CRYPTOCFG_4_23__RESERVED_4__WIDTH                               0x00000020
#define CRYPTOCFG_4_23__RESERVED_4__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_24 : RESERVED_4          */
#define CRYPTOCFG_4_24__RESERVED_4__SHIFT                               0x00000000
#define CRYPTOCFG_4_24__RESERVED_4__WIDTH                               0x00000020
#define CRYPTOCFG_4_24__RESERVED_4__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_25 : RESERVED_4          */
#define CRYPTOCFG_4_25__RESERVED_4__SHIFT                               0x00000000
#define CRYPTOCFG_4_25__RESERVED_4__WIDTH                               0x00000020
#define CRYPTOCFG_4_25__RESERVED_4__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_26 : RESERVED_4          */
#define CRYPTOCFG_4_26__RESERVED_4__SHIFT                               0x00000000
#define CRYPTOCFG_4_26__RESERVED_4__WIDTH                               0x00000020
#define CRYPTOCFG_4_26__RESERVED_4__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_27 : RESERVED_4          */
#define CRYPTOCFG_4_27__RESERVED_4__SHIFT                               0x00000000
#define CRYPTOCFG_4_27__RESERVED_4__WIDTH                               0x00000020
#define CRYPTOCFG_4_27__RESERVED_4__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_28 : RESERVED_4          */
#define CRYPTOCFG_4_28__RESERVED_4__SHIFT                               0x00000000
#define CRYPTOCFG_4_28__RESERVED_4__WIDTH                               0x00000020
#define CRYPTOCFG_4_28__RESERVED_4__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_29 : RESERVED_4          */
#define CRYPTOCFG_4_29__RESERVED_4__SHIFT                               0x00000000
#define CRYPTOCFG_4_29__RESERVED_4__WIDTH                               0x00000020
#define CRYPTOCFG_4_29__RESERVED_4__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_30 : RESERVED_4          */
#define CRYPTOCFG_4_30__RESERVED_4__SHIFT                               0x00000000
#define CRYPTOCFG_4_30__RESERVED_4__WIDTH                               0x00000020
#define CRYPTOCFG_4_30__RESERVED_4__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_4_31 : RESERVED_4          */
#define CRYPTOCFG_4_31__RESERVED_4__SHIFT                               0x00000000
#define CRYPTOCFG_4_31__RESERVED_4__WIDTH                               0x00000020
#define CRYPTOCFG_4_31__RESERVED_4__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_0 : CRYPTOKEY_5_0       */
#define CRYPTOCFG_5_0__CRYPTOKEY_5_0__SHIFT                             0x00000000
#define CRYPTOCFG_5_0__CRYPTOKEY_5_0__WIDTH                             0x00000020
#define CRYPTOCFG_5_0__CRYPTOKEY_5_0__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_1 : CRYPTOKEY_5_1       */
#define CRYPTOCFG_5_1__CRYPTOKEY_5_1__SHIFT                             0x00000000
#define CRYPTOCFG_5_1__CRYPTOKEY_5_1__WIDTH                             0x00000020
#define CRYPTOCFG_5_1__CRYPTOKEY_5_1__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_2 : CRYPTOKEY_5_2       */
#define CRYPTOCFG_5_2__CRYPTOKEY_5_2__SHIFT                             0x00000000
#define CRYPTOCFG_5_2__CRYPTOKEY_5_2__WIDTH                             0x00000020
#define CRYPTOCFG_5_2__CRYPTOKEY_5_2__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_3 : CRYPTOKEY_5_3       */
#define CRYPTOCFG_5_3__CRYPTOKEY_5_3__SHIFT                             0x00000000
#define CRYPTOCFG_5_3__CRYPTOKEY_5_3__WIDTH                             0x00000020
#define CRYPTOCFG_5_3__CRYPTOKEY_5_3__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_4 : CRYPTOKEY_5_4       */
#define CRYPTOCFG_5_4__CRYPTOKEY_5_4__SHIFT                             0x00000000
#define CRYPTOCFG_5_4__CRYPTOKEY_5_4__WIDTH                             0x00000020
#define CRYPTOCFG_5_4__CRYPTOKEY_5_4__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_5 : CRYPTOKEY_5_5       */
#define CRYPTOCFG_5_5__CRYPTOKEY_5_5__SHIFT                             0x00000000
#define CRYPTOCFG_5_5__CRYPTOKEY_5_5__WIDTH                             0x00000020
#define CRYPTOCFG_5_5__CRYPTOKEY_5_5__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_6 : CRYPTOKEY_5_6       */
#define CRYPTOCFG_5_6__CRYPTOKEY_5_6__SHIFT                             0x00000000
#define CRYPTOCFG_5_6__CRYPTOKEY_5_6__WIDTH                             0x00000020
#define CRYPTOCFG_5_6__CRYPTOKEY_5_6__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_7 : CRYPTOKEY_5_7       */
#define CRYPTOCFG_5_7__CRYPTOKEY_5_7__SHIFT                             0x00000000
#define CRYPTOCFG_5_7__CRYPTOKEY_5_7__WIDTH                             0x00000020
#define CRYPTOCFG_5_7__CRYPTOKEY_5_7__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_8 : CRYPTOKEY_5_8       */
#define CRYPTOCFG_5_8__CRYPTOKEY_5_8__SHIFT                             0x00000000
#define CRYPTOCFG_5_8__CRYPTOKEY_5_8__WIDTH                             0x00000020
#define CRYPTOCFG_5_8__CRYPTOKEY_5_8__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_9 : CRYPTOKEY_5_9       */
#define CRYPTOCFG_5_9__CRYPTOKEY_5_9__SHIFT                             0x00000000
#define CRYPTOCFG_5_9__CRYPTOKEY_5_9__WIDTH                             0x00000020
#define CRYPTOCFG_5_9__CRYPTOKEY_5_9__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_10 : CRYPTOKEY_5_10      */
#define CRYPTOCFG_5_10__CRYPTOKEY_5_10__SHIFT                           0x00000000
#define CRYPTOCFG_5_10__CRYPTOKEY_5_10__WIDTH                           0x00000020
#define CRYPTOCFG_5_10__CRYPTOKEY_5_10__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_11 : CRYPTOKEY_5_11      */
#define CRYPTOCFG_5_11__CRYPTOKEY_5_11__SHIFT                           0x00000000
#define CRYPTOCFG_5_11__CRYPTOKEY_5_11__WIDTH                           0x00000020
#define CRYPTOCFG_5_11__CRYPTOKEY_5_11__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_12 : CRYPTOKEY_5_12      */
#define CRYPTOCFG_5_12__CRYPTOKEY_5_12__SHIFT                           0x00000000
#define CRYPTOCFG_5_12__CRYPTOKEY_5_12__WIDTH                           0x00000020
#define CRYPTOCFG_5_12__CRYPTOKEY_5_12__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_13 : CRYPTOKEY_5_13      */
#define CRYPTOCFG_5_13__CRYPTOKEY_5_13__SHIFT                           0x00000000
#define CRYPTOCFG_5_13__CRYPTOKEY_5_13__WIDTH                           0x00000020
#define CRYPTOCFG_5_13__CRYPTOKEY_5_13__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_14 : CRYPTOKEY_5_14      */
#define CRYPTOCFG_5_14__CRYPTOKEY_5_14__SHIFT                           0x00000000
#define CRYPTOCFG_5_14__CRYPTOKEY_5_14__WIDTH                           0x00000020
#define CRYPTOCFG_5_14__CRYPTOKEY_5_14__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_15 : CRYPTOKEY_5_15      */
#define CRYPTOCFG_5_15__CRYPTOKEY_5_15__SHIFT                           0x00000000
#define CRYPTOCFG_5_15__CRYPTOKEY_5_15__WIDTH                           0x00000020
#define CRYPTOCFG_5_15__CRYPTOKEY_5_15__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_16 : DUSIZE_5            */
#define CRYPTOCFG_5_16__DUSIZE_5__SHIFT                                 0x00000000
#define CRYPTOCFG_5_16__DUSIZE_5__WIDTH                                 0x00000008
#define CRYPTOCFG_5_16__DUSIZE_5__MASK                                  0x000000FF

/*  CRYPTOCFG_5_16 : CAPIDX_5            */
#define CRYPTOCFG_5_16__CAPIDX_5__SHIFT                                 0x00000008
#define CRYPTOCFG_5_16__CAPIDX_5__WIDTH                                 0x00000008
#define CRYPTOCFG_5_16__CAPIDX_5__MASK                                  0x0000FF00

/*  CRYPTOCFG_5_16 : CRYPTPCFG_RSVD_5    */
#define CRYPTOCFG_5_16__CRYPTPCFG_RSVD_5__SHIFT                         0x00000010
#define CRYPTOCFG_5_16__CRYPTPCFG_RSVD_5__WIDTH                         0x0000000F
#define CRYPTOCFG_5_16__CRYPTPCFG_RSVD_5__MASK                          0x7FFF0000

/*  CRYPTOCFG_5_16 : CFGE_5              */
#define CRYPTOCFG_5_16__CFGE_5__SHIFT                                   0x0000001F
#define CRYPTOCFG_5_16__CFGE_5__WIDTH                                   0x00000001
#define CRYPTOCFG_5_16__CFGE_5__MASK                                    0x80000000



/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_17 : RESERVED_5          */
#define CRYPTOCFG_5_17__RESERVED_5__SHIFT                               0x00000000
#define CRYPTOCFG_5_17__RESERVED_5__WIDTH                               0x00000020
#define CRYPTOCFG_5_17__RESERVED_5__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_18 : RESERVED_5          */
#define CRYPTOCFG_5_18__RESERVED_5__SHIFT                               0x00000000
#define CRYPTOCFG_5_18__RESERVED_5__WIDTH                               0x00000020
#define CRYPTOCFG_5_18__RESERVED_5__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_19 : RESERVED_5          */
#define CRYPTOCFG_5_19__RESERVED_5__SHIFT                               0x00000000
#define CRYPTOCFG_5_19__RESERVED_5__WIDTH                               0x00000020
#define CRYPTOCFG_5_19__RESERVED_5__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_20 : RESERVED_5          */
#define CRYPTOCFG_5_20__RESERVED_5__SHIFT                               0x00000000
#define CRYPTOCFG_5_20__RESERVED_5__WIDTH                               0x00000020
#define CRYPTOCFG_5_20__RESERVED_5__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_21 : RESERVED_5          */
#define CRYPTOCFG_5_21__RESERVED_5__SHIFT                               0x00000000
#define CRYPTOCFG_5_21__RESERVED_5__WIDTH                               0x00000020
#define CRYPTOCFG_5_21__RESERVED_5__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_22 : RESERVED_5          */
#define CRYPTOCFG_5_22__RESERVED_5__SHIFT                               0x00000000
#define CRYPTOCFG_5_22__RESERVED_5__WIDTH                               0x00000020
#define CRYPTOCFG_5_22__RESERVED_5__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_23 : RESERVED_5          */
#define CRYPTOCFG_5_23__RESERVED_5__SHIFT                               0x00000000
#define CRYPTOCFG_5_23__RESERVED_5__WIDTH                               0x00000020
#define CRYPTOCFG_5_23__RESERVED_5__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_24 : RESERVED_5          */
#define CRYPTOCFG_5_24__RESERVED_5__SHIFT                               0x00000000
#define CRYPTOCFG_5_24__RESERVED_5__WIDTH                               0x00000020
#define CRYPTOCFG_5_24__RESERVED_5__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_25 : RESERVED_5          */
#define CRYPTOCFG_5_25__RESERVED_5__SHIFT                               0x00000000
#define CRYPTOCFG_5_25__RESERVED_5__WIDTH                               0x00000020
#define CRYPTOCFG_5_25__RESERVED_5__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_26 : RESERVED_5          */
#define CRYPTOCFG_5_26__RESERVED_5__SHIFT                               0x00000000
#define CRYPTOCFG_5_26__RESERVED_5__WIDTH                               0x00000020
#define CRYPTOCFG_5_26__RESERVED_5__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_27 : RESERVED_5          */
#define CRYPTOCFG_5_27__RESERVED_5__SHIFT                               0x00000000
#define CRYPTOCFG_5_27__RESERVED_5__WIDTH                               0x00000020
#define CRYPTOCFG_5_27__RESERVED_5__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_28 : RESERVED_5          */
#define CRYPTOCFG_5_28__RESERVED_5__SHIFT                               0x00000000
#define CRYPTOCFG_5_28__RESERVED_5__WIDTH                               0x00000020
#define CRYPTOCFG_5_28__RESERVED_5__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_29 : RESERVED_5          */
#define CRYPTOCFG_5_29__RESERVED_5__SHIFT                               0x00000000
#define CRYPTOCFG_5_29__RESERVED_5__WIDTH                               0x00000020
#define CRYPTOCFG_5_29__RESERVED_5__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_30 : RESERVED_5          */
#define CRYPTOCFG_5_30__RESERVED_5__SHIFT                               0x00000000
#define CRYPTOCFG_5_30__RESERVED_5__WIDTH                               0x00000020
#define CRYPTOCFG_5_30__RESERVED_5__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_5_31 : RESERVED_5          */
#define CRYPTOCFG_5_31__RESERVED_5__SHIFT                               0x00000000
#define CRYPTOCFG_5_31__RESERVED_5__WIDTH                               0x00000020
#define CRYPTOCFG_5_31__RESERVED_5__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_0 : CRYPTOKEY_6_0       */
#define CRYPTOCFG_6_0__CRYPTOKEY_6_0__SHIFT                             0x00000000
#define CRYPTOCFG_6_0__CRYPTOKEY_6_0__WIDTH                             0x00000020
#define CRYPTOCFG_6_0__CRYPTOKEY_6_0__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_1 : CRYPTOKEY_6_1       */
#define CRYPTOCFG_6_1__CRYPTOKEY_6_1__SHIFT                             0x00000000
#define CRYPTOCFG_6_1__CRYPTOKEY_6_1__WIDTH                             0x00000020
#define CRYPTOCFG_6_1__CRYPTOKEY_6_1__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_2 : CRYPTOKEY_6_2       */
#define CRYPTOCFG_6_2__CRYPTOKEY_6_2__SHIFT                             0x00000000
#define CRYPTOCFG_6_2__CRYPTOKEY_6_2__WIDTH                             0x00000020
#define CRYPTOCFG_6_2__CRYPTOKEY_6_2__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_3 : CRYPTOKEY_6_3       */
#define CRYPTOCFG_6_3__CRYPTOKEY_6_3__SHIFT                             0x00000000
#define CRYPTOCFG_6_3__CRYPTOKEY_6_3__WIDTH                             0x00000020
#define CRYPTOCFG_6_3__CRYPTOKEY_6_3__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_4 : CRYPTOKEY_6_4       */
#define CRYPTOCFG_6_4__CRYPTOKEY_6_4__SHIFT                             0x00000000
#define CRYPTOCFG_6_4__CRYPTOKEY_6_4__WIDTH                             0x00000020
#define CRYPTOCFG_6_4__CRYPTOKEY_6_4__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_5 : CRYPTOKEY_6_5       */
#define CRYPTOCFG_6_5__CRYPTOKEY_6_5__SHIFT                             0x00000000
#define CRYPTOCFG_6_5__CRYPTOKEY_6_5__WIDTH                             0x00000020
#define CRYPTOCFG_6_5__CRYPTOKEY_6_5__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_6 : CRYPTOKEY_6_6       */
#define CRYPTOCFG_6_6__CRYPTOKEY_6_6__SHIFT                             0x00000000
#define CRYPTOCFG_6_6__CRYPTOKEY_6_6__WIDTH                             0x00000020
#define CRYPTOCFG_6_6__CRYPTOKEY_6_6__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_7 : CRYPTOKEY_6_7       */
#define CRYPTOCFG_6_7__CRYPTOKEY_6_7__SHIFT                             0x00000000
#define CRYPTOCFG_6_7__CRYPTOKEY_6_7__WIDTH                             0x00000020
#define CRYPTOCFG_6_7__CRYPTOKEY_6_7__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_8 : CRYPTOKEY_6_8       */
#define CRYPTOCFG_6_8__CRYPTOKEY_6_8__SHIFT                             0x00000000
#define CRYPTOCFG_6_8__CRYPTOKEY_6_8__WIDTH                             0x00000020
#define CRYPTOCFG_6_8__CRYPTOKEY_6_8__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_9 : CRYPTOKEY_6_9       */
#define CRYPTOCFG_6_9__CRYPTOKEY_6_9__SHIFT                             0x00000000
#define CRYPTOCFG_6_9__CRYPTOKEY_6_9__WIDTH                             0x00000020
#define CRYPTOCFG_6_9__CRYPTOKEY_6_9__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_10 : CRYPTOKEY_6_10      */
#define CRYPTOCFG_6_10__CRYPTOKEY_6_10__SHIFT                           0x00000000
#define CRYPTOCFG_6_10__CRYPTOKEY_6_10__WIDTH                           0x00000020
#define CRYPTOCFG_6_10__CRYPTOKEY_6_10__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_11 : CRYPTOKEY_6_11      */
#define CRYPTOCFG_6_11__CRYPTOKEY_6_11__SHIFT                           0x00000000
#define CRYPTOCFG_6_11__CRYPTOKEY_6_11__WIDTH                           0x00000020
#define CRYPTOCFG_6_11__CRYPTOKEY_6_11__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_12 : CRYPTOKEY_6_12      */
#define CRYPTOCFG_6_12__CRYPTOKEY_6_12__SHIFT                           0x00000000
#define CRYPTOCFG_6_12__CRYPTOKEY_6_12__WIDTH                           0x00000020
#define CRYPTOCFG_6_12__CRYPTOKEY_6_12__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_13 : CRYPTOKEY_6_13      */
#define CRYPTOCFG_6_13__CRYPTOKEY_6_13__SHIFT                           0x00000000
#define CRYPTOCFG_6_13__CRYPTOKEY_6_13__WIDTH                           0x00000020
#define CRYPTOCFG_6_13__CRYPTOKEY_6_13__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_14 : CRYPTOKEY_6_14      */
#define CRYPTOCFG_6_14__CRYPTOKEY_6_14__SHIFT                           0x00000000
#define CRYPTOCFG_6_14__CRYPTOKEY_6_14__WIDTH                           0x00000020
#define CRYPTOCFG_6_14__CRYPTOKEY_6_14__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_15 : CRYPTOKEY_6_15      */
#define CRYPTOCFG_6_15__CRYPTOKEY_6_15__SHIFT                           0x00000000
#define CRYPTOCFG_6_15__CRYPTOKEY_6_15__WIDTH                           0x00000020
#define CRYPTOCFG_6_15__CRYPTOKEY_6_15__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_16 : DUSIZE_6            */
#define CRYPTOCFG_6_16__DUSIZE_6__SHIFT                                 0x00000000
#define CRYPTOCFG_6_16__DUSIZE_6__WIDTH                                 0x00000008
#define CRYPTOCFG_6_16__DUSIZE_6__MASK                                  0x000000FF

/*  CRYPTOCFG_6_16 : CAPIDX_6            */
#define CRYPTOCFG_6_16__CAPIDX_6__SHIFT                                 0x00000008
#define CRYPTOCFG_6_16__CAPIDX_6__WIDTH                                 0x00000008
#define CRYPTOCFG_6_16__CAPIDX_6__MASK                                  0x0000FF00

/*  CRYPTOCFG_6_16 : CRYPTPCFG_RSVD_6    */
#define CRYPTOCFG_6_16__CRYPTPCFG_RSVD_6__SHIFT                         0x00000010
#define CRYPTOCFG_6_16__CRYPTPCFG_RSVD_6__WIDTH                         0x0000000F
#define CRYPTOCFG_6_16__CRYPTPCFG_RSVD_6__MASK                          0x7FFF0000

/*  CRYPTOCFG_6_16 : CFGE_6              */
#define CRYPTOCFG_6_16__CFGE_6__SHIFT                                   0x0000001F
#define CRYPTOCFG_6_16__CFGE_6__WIDTH                                   0x00000001
#define CRYPTOCFG_6_16__CFGE_6__MASK                                    0x80000000



/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_17 : RESERVED_6          */
#define CRYPTOCFG_6_17__RESERVED_6__SHIFT                               0x00000000
#define CRYPTOCFG_6_17__RESERVED_6__WIDTH                               0x00000020
#define CRYPTOCFG_6_17__RESERVED_6__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_18 : RESERVED_6          */
#define CRYPTOCFG_6_18__RESERVED_6__SHIFT                               0x00000000
#define CRYPTOCFG_6_18__RESERVED_6__WIDTH                               0x00000020
#define CRYPTOCFG_6_18__RESERVED_6__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_19 : RESERVED_6          */
#define CRYPTOCFG_6_19__RESERVED_6__SHIFT                               0x00000000
#define CRYPTOCFG_6_19__RESERVED_6__WIDTH                               0x00000020
#define CRYPTOCFG_6_19__RESERVED_6__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_20 : RESERVED_6          */
#define CRYPTOCFG_6_20__RESERVED_6__SHIFT                               0x00000000
#define CRYPTOCFG_6_20__RESERVED_6__WIDTH                               0x00000020
#define CRYPTOCFG_6_20__RESERVED_6__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_21 : RESERVED_6          */
#define CRYPTOCFG_6_21__RESERVED_6__SHIFT                               0x00000000
#define CRYPTOCFG_6_21__RESERVED_6__WIDTH                               0x00000020
#define CRYPTOCFG_6_21__RESERVED_6__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_22 : RESERVED_6          */
#define CRYPTOCFG_6_22__RESERVED_6__SHIFT                               0x00000000
#define CRYPTOCFG_6_22__RESERVED_6__WIDTH                               0x00000020
#define CRYPTOCFG_6_22__RESERVED_6__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_23 : RESERVED_6          */
#define CRYPTOCFG_6_23__RESERVED_6__SHIFT                               0x00000000
#define CRYPTOCFG_6_23__RESERVED_6__WIDTH                               0x00000020
#define CRYPTOCFG_6_23__RESERVED_6__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_24 : RESERVED_6          */
#define CRYPTOCFG_6_24__RESERVED_6__SHIFT                               0x00000000
#define CRYPTOCFG_6_24__RESERVED_6__WIDTH                               0x00000020
#define CRYPTOCFG_6_24__RESERVED_6__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_25 : RESERVED_6          */
#define CRYPTOCFG_6_25__RESERVED_6__SHIFT                               0x00000000
#define CRYPTOCFG_6_25__RESERVED_6__WIDTH                               0x00000020
#define CRYPTOCFG_6_25__RESERVED_6__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_26 : RESERVED_6          */
#define CRYPTOCFG_6_26__RESERVED_6__SHIFT                               0x00000000
#define CRYPTOCFG_6_26__RESERVED_6__WIDTH                               0x00000020
#define CRYPTOCFG_6_26__RESERVED_6__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_27 : RESERVED_6          */
#define CRYPTOCFG_6_27__RESERVED_6__SHIFT                               0x00000000
#define CRYPTOCFG_6_27__RESERVED_6__WIDTH                               0x00000020
#define CRYPTOCFG_6_27__RESERVED_6__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_28 : RESERVED_6          */
#define CRYPTOCFG_6_28__RESERVED_6__SHIFT                               0x00000000
#define CRYPTOCFG_6_28__RESERVED_6__WIDTH                               0x00000020
#define CRYPTOCFG_6_28__RESERVED_6__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_29 : RESERVED_6          */
#define CRYPTOCFG_6_29__RESERVED_6__SHIFT                               0x00000000
#define CRYPTOCFG_6_29__RESERVED_6__WIDTH                               0x00000020
#define CRYPTOCFG_6_29__RESERVED_6__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_30 : RESERVED_6          */
#define CRYPTOCFG_6_30__RESERVED_6__SHIFT                               0x00000000
#define CRYPTOCFG_6_30__RESERVED_6__WIDTH                               0x00000020
#define CRYPTOCFG_6_30__RESERVED_6__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_6_31 : RESERVED_6          */
#define CRYPTOCFG_6_31__RESERVED_6__SHIFT                               0x00000000
#define CRYPTOCFG_6_31__RESERVED_6__WIDTH                               0x00000020
#define CRYPTOCFG_6_31__RESERVED_6__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_0 : CRYPTOKEY_7_0       */
#define CRYPTOCFG_7_0__CRYPTOKEY_7_0__SHIFT                             0x00000000
#define CRYPTOCFG_7_0__CRYPTOKEY_7_0__WIDTH                             0x00000020
#define CRYPTOCFG_7_0__CRYPTOKEY_7_0__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_1 : CRYPTOKEY_7_1       */
#define CRYPTOCFG_7_1__CRYPTOKEY_7_1__SHIFT                             0x00000000
#define CRYPTOCFG_7_1__CRYPTOKEY_7_1__WIDTH                             0x00000020
#define CRYPTOCFG_7_1__CRYPTOKEY_7_1__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_2 : CRYPTOKEY_7_2       */
#define CRYPTOCFG_7_2__CRYPTOKEY_7_2__SHIFT                             0x00000000
#define CRYPTOCFG_7_2__CRYPTOKEY_7_2__WIDTH                             0x00000020
#define CRYPTOCFG_7_2__CRYPTOKEY_7_2__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_3 : CRYPTOKEY_7_3       */
#define CRYPTOCFG_7_3__CRYPTOKEY_7_3__SHIFT                             0x00000000
#define CRYPTOCFG_7_3__CRYPTOKEY_7_3__WIDTH                             0x00000020
#define CRYPTOCFG_7_3__CRYPTOKEY_7_3__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_4 : CRYPTOKEY_7_4       */
#define CRYPTOCFG_7_4__CRYPTOKEY_7_4__SHIFT                             0x00000000
#define CRYPTOCFG_7_4__CRYPTOKEY_7_4__WIDTH                             0x00000020
#define CRYPTOCFG_7_4__CRYPTOKEY_7_4__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_5 : CRYPTOKEY_7_5       */
#define CRYPTOCFG_7_5__CRYPTOKEY_7_5__SHIFT                             0x00000000
#define CRYPTOCFG_7_5__CRYPTOKEY_7_5__WIDTH                             0x00000020
#define CRYPTOCFG_7_5__CRYPTOKEY_7_5__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_6 : CRYPTOKEY_7_6       */
#define CRYPTOCFG_7_6__CRYPTOKEY_7_6__SHIFT                             0x00000000
#define CRYPTOCFG_7_6__CRYPTOKEY_7_6__WIDTH                             0x00000020
#define CRYPTOCFG_7_6__CRYPTOKEY_7_6__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_7 : CRYPTOKEY_7_7       */
#define CRYPTOCFG_7_7__CRYPTOKEY_7_7__SHIFT                             0x00000000
#define CRYPTOCFG_7_7__CRYPTOKEY_7_7__WIDTH                             0x00000020
#define CRYPTOCFG_7_7__CRYPTOKEY_7_7__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_8 : CRYPTOKEY_7_8       */
#define CRYPTOCFG_7_8__CRYPTOKEY_7_8__SHIFT                             0x00000000
#define CRYPTOCFG_7_8__CRYPTOKEY_7_8__WIDTH                             0x00000020
#define CRYPTOCFG_7_8__CRYPTOKEY_7_8__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_9 : CRYPTOKEY_7_9       */
#define CRYPTOCFG_7_9__CRYPTOKEY_7_9__SHIFT                             0x00000000
#define CRYPTOCFG_7_9__CRYPTOKEY_7_9__WIDTH                             0x00000020
#define CRYPTOCFG_7_9__CRYPTOKEY_7_9__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_10 : CRYPTOKEY_7_10      */
#define CRYPTOCFG_7_10__CRYPTOKEY_7_10__SHIFT                           0x00000000
#define CRYPTOCFG_7_10__CRYPTOKEY_7_10__WIDTH                           0x00000020
#define CRYPTOCFG_7_10__CRYPTOKEY_7_10__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_11 : CRYPTOKEY_7_11      */
#define CRYPTOCFG_7_11__CRYPTOKEY_7_11__SHIFT                           0x00000000
#define CRYPTOCFG_7_11__CRYPTOKEY_7_11__WIDTH                           0x00000020
#define CRYPTOCFG_7_11__CRYPTOKEY_7_11__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_12 : CRYPTOKEY_7_12      */
#define CRYPTOCFG_7_12__CRYPTOKEY_7_12__SHIFT                           0x00000000
#define CRYPTOCFG_7_12__CRYPTOKEY_7_12__WIDTH                           0x00000020
#define CRYPTOCFG_7_12__CRYPTOKEY_7_12__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_13 : CRYPTOKEY_7_13      */
#define CRYPTOCFG_7_13__CRYPTOKEY_7_13__SHIFT                           0x00000000
#define CRYPTOCFG_7_13__CRYPTOKEY_7_13__WIDTH                           0x00000020
#define CRYPTOCFG_7_13__CRYPTOKEY_7_13__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_14 : CRYPTOKEY_7_14      */
#define CRYPTOCFG_7_14__CRYPTOKEY_7_14__SHIFT                           0x00000000
#define CRYPTOCFG_7_14__CRYPTOKEY_7_14__WIDTH                           0x00000020
#define CRYPTOCFG_7_14__CRYPTOKEY_7_14__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_15 : CRYPTOKEY_7_15      */
#define CRYPTOCFG_7_15__CRYPTOKEY_7_15__SHIFT                           0x00000000
#define CRYPTOCFG_7_15__CRYPTOKEY_7_15__WIDTH                           0x00000020
#define CRYPTOCFG_7_15__CRYPTOKEY_7_15__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_16 : DUSIZE_7            */
#define CRYPTOCFG_7_16__DUSIZE_7__SHIFT                                 0x00000000
#define CRYPTOCFG_7_16__DUSIZE_7__WIDTH                                 0x00000008
#define CRYPTOCFG_7_16__DUSIZE_7__MASK                                  0x000000FF

/*  CRYPTOCFG_7_16 : CAPIDX_7            */
#define CRYPTOCFG_7_16__CAPIDX_7__SHIFT                                 0x00000008
#define CRYPTOCFG_7_16__CAPIDX_7__WIDTH                                 0x00000008
#define CRYPTOCFG_7_16__CAPIDX_7__MASK                                  0x0000FF00

/*  CRYPTOCFG_7_16 : CRYPTPCFG_RSVD_7    */
#define CRYPTOCFG_7_16__CRYPTPCFG_RSVD_7__SHIFT                         0x00000010
#define CRYPTOCFG_7_16__CRYPTPCFG_RSVD_7__WIDTH                         0x0000000F
#define CRYPTOCFG_7_16__CRYPTPCFG_RSVD_7__MASK                          0x7FFF0000

/*  CRYPTOCFG_7_16 : CFGE_7              */
#define CRYPTOCFG_7_16__CFGE_7__SHIFT                                   0x0000001F
#define CRYPTOCFG_7_16__CFGE_7__WIDTH                                   0x00000001
#define CRYPTOCFG_7_16__CFGE_7__MASK                                    0x80000000



/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_17 : RESERVED_7          */
#define CRYPTOCFG_7_17__RESERVED_7__SHIFT                               0x00000000
#define CRYPTOCFG_7_17__RESERVED_7__WIDTH                               0x00000020
#define CRYPTOCFG_7_17__RESERVED_7__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_18 : RESERVED_7          */
#define CRYPTOCFG_7_18__RESERVED_7__SHIFT                               0x00000000
#define CRYPTOCFG_7_18__RESERVED_7__WIDTH                               0x00000020
#define CRYPTOCFG_7_18__RESERVED_7__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_19 : RESERVED_7          */
#define CRYPTOCFG_7_19__RESERVED_7__SHIFT                               0x00000000
#define CRYPTOCFG_7_19__RESERVED_7__WIDTH                               0x00000020
#define CRYPTOCFG_7_19__RESERVED_7__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_20 : RESERVED_7          */
#define CRYPTOCFG_7_20__RESERVED_7__SHIFT                               0x00000000
#define CRYPTOCFG_7_20__RESERVED_7__WIDTH                               0x00000020
#define CRYPTOCFG_7_20__RESERVED_7__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_21 : RESERVED_7          */
#define CRYPTOCFG_7_21__RESERVED_7__SHIFT                               0x00000000
#define CRYPTOCFG_7_21__RESERVED_7__WIDTH                               0x00000020
#define CRYPTOCFG_7_21__RESERVED_7__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_22 : RESERVED_7          */
#define CRYPTOCFG_7_22__RESERVED_7__SHIFT                               0x00000000
#define CRYPTOCFG_7_22__RESERVED_7__WIDTH                               0x00000020
#define CRYPTOCFG_7_22__RESERVED_7__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_23 : RESERVED_7          */
#define CRYPTOCFG_7_23__RESERVED_7__SHIFT                               0x00000000
#define CRYPTOCFG_7_23__RESERVED_7__WIDTH                               0x00000020
#define CRYPTOCFG_7_23__RESERVED_7__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_24 : RESERVED_7          */
#define CRYPTOCFG_7_24__RESERVED_7__SHIFT                               0x00000000
#define CRYPTOCFG_7_24__RESERVED_7__WIDTH                               0x00000020
#define CRYPTOCFG_7_24__RESERVED_7__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_25 : RESERVED_7          */
#define CRYPTOCFG_7_25__RESERVED_7__SHIFT                               0x00000000
#define CRYPTOCFG_7_25__RESERVED_7__WIDTH                               0x00000020
#define CRYPTOCFG_7_25__RESERVED_7__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_26 : RESERVED_7          */
#define CRYPTOCFG_7_26__RESERVED_7__SHIFT                               0x00000000
#define CRYPTOCFG_7_26__RESERVED_7__WIDTH                               0x00000020
#define CRYPTOCFG_7_26__RESERVED_7__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_27 : RESERVED_7          */
#define CRYPTOCFG_7_27__RESERVED_7__SHIFT                               0x00000000
#define CRYPTOCFG_7_27__RESERVED_7__WIDTH                               0x00000020
#define CRYPTOCFG_7_27__RESERVED_7__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_28 : RESERVED_7          */
#define CRYPTOCFG_7_28__RESERVED_7__SHIFT                               0x00000000
#define CRYPTOCFG_7_28__RESERVED_7__WIDTH                               0x00000020
#define CRYPTOCFG_7_28__RESERVED_7__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_29 : RESERVED_7          */
#define CRYPTOCFG_7_29__RESERVED_7__SHIFT                               0x00000000
#define CRYPTOCFG_7_29__RESERVED_7__WIDTH                               0x00000020
#define CRYPTOCFG_7_29__RESERVED_7__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_30 : RESERVED_7          */
#define CRYPTOCFG_7_30__RESERVED_7__SHIFT                               0x00000000
#define CRYPTOCFG_7_30__RESERVED_7__WIDTH                               0x00000020
#define CRYPTOCFG_7_30__RESERVED_7__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_7_31 : RESERVED_7          */
#define CRYPTOCFG_7_31__RESERVED_7__SHIFT                               0x00000000
#define CRYPTOCFG_7_31__RESERVED_7__WIDTH                               0x00000020
#define CRYPTOCFG_7_31__RESERVED_7__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_0 : CRYPTOKEY_8_0       */
#define CRYPTOCFG_8_0__CRYPTOKEY_8_0__SHIFT                             0x00000000
#define CRYPTOCFG_8_0__CRYPTOKEY_8_0__WIDTH                             0x00000020
#define CRYPTOCFG_8_0__CRYPTOKEY_8_0__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_1 : CRYPTOKEY_8_1       */
#define CRYPTOCFG_8_1__CRYPTOKEY_8_1__SHIFT                             0x00000000
#define CRYPTOCFG_8_1__CRYPTOKEY_8_1__WIDTH                             0x00000020
#define CRYPTOCFG_8_1__CRYPTOKEY_8_1__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_2 : CRYPTOKEY_8_2       */
#define CRYPTOCFG_8_2__CRYPTOKEY_8_2__SHIFT                             0x00000000
#define CRYPTOCFG_8_2__CRYPTOKEY_8_2__WIDTH                             0x00000020
#define CRYPTOCFG_8_2__CRYPTOKEY_8_2__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_3 : CRYPTOKEY_8_3       */
#define CRYPTOCFG_8_3__CRYPTOKEY_8_3__SHIFT                             0x00000000
#define CRYPTOCFG_8_3__CRYPTOKEY_8_3__WIDTH                             0x00000020
#define CRYPTOCFG_8_3__CRYPTOKEY_8_3__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_4 : CRYPTOKEY_8_4       */
#define CRYPTOCFG_8_4__CRYPTOKEY_8_4__SHIFT                             0x00000000
#define CRYPTOCFG_8_4__CRYPTOKEY_8_4__WIDTH                             0x00000020
#define CRYPTOCFG_8_4__CRYPTOKEY_8_4__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_5 : CRYPTOKEY_8_5       */
#define CRYPTOCFG_8_5__CRYPTOKEY_8_5__SHIFT                             0x00000000
#define CRYPTOCFG_8_5__CRYPTOKEY_8_5__WIDTH                             0x00000020
#define CRYPTOCFG_8_5__CRYPTOKEY_8_5__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_6 : CRYPTOKEY_8_6       */
#define CRYPTOCFG_8_6__CRYPTOKEY_8_6__SHIFT                             0x00000000
#define CRYPTOCFG_8_6__CRYPTOKEY_8_6__WIDTH                             0x00000020
#define CRYPTOCFG_8_6__CRYPTOKEY_8_6__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_7 : CRYPTOKEY_8_7       */
#define CRYPTOCFG_8_7__CRYPTOKEY_8_7__SHIFT                             0x00000000
#define CRYPTOCFG_8_7__CRYPTOKEY_8_7__WIDTH                             0x00000020
#define CRYPTOCFG_8_7__CRYPTOKEY_8_7__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_8 : CRYPTOKEY_8_8       */
#define CRYPTOCFG_8_8__CRYPTOKEY_8_8__SHIFT                             0x00000000
#define CRYPTOCFG_8_8__CRYPTOKEY_8_8__WIDTH                             0x00000020
#define CRYPTOCFG_8_8__CRYPTOKEY_8_8__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_9 : CRYPTOKEY_8_9       */
#define CRYPTOCFG_8_9__CRYPTOKEY_8_9__SHIFT                             0x00000000
#define CRYPTOCFG_8_9__CRYPTOKEY_8_9__WIDTH                             0x00000020
#define CRYPTOCFG_8_9__CRYPTOKEY_8_9__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_10 : CRYPTOKEY_8_10      */
#define CRYPTOCFG_8_10__CRYPTOKEY_8_10__SHIFT                           0x00000000
#define CRYPTOCFG_8_10__CRYPTOKEY_8_10__WIDTH                           0x00000020
#define CRYPTOCFG_8_10__CRYPTOKEY_8_10__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_11 : CRYPTOKEY_8_11      */
#define CRYPTOCFG_8_11__CRYPTOKEY_8_11__SHIFT                           0x00000000
#define CRYPTOCFG_8_11__CRYPTOKEY_8_11__WIDTH                           0x00000020
#define CRYPTOCFG_8_11__CRYPTOKEY_8_11__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_12 : CRYPTOKEY_8_12      */
#define CRYPTOCFG_8_12__CRYPTOKEY_8_12__SHIFT                           0x00000000
#define CRYPTOCFG_8_12__CRYPTOKEY_8_12__WIDTH                           0x00000020
#define CRYPTOCFG_8_12__CRYPTOKEY_8_12__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_13 : CRYPTOKEY_8_13      */
#define CRYPTOCFG_8_13__CRYPTOKEY_8_13__SHIFT                           0x00000000
#define CRYPTOCFG_8_13__CRYPTOKEY_8_13__WIDTH                           0x00000020
#define CRYPTOCFG_8_13__CRYPTOKEY_8_13__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_14 : CRYPTOKEY_8_14      */
#define CRYPTOCFG_8_14__CRYPTOKEY_8_14__SHIFT                           0x00000000
#define CRYPTOCFG_8_14__CRYPTOKEY_8_14__WIDTH                           0x00000020
#define CRYPTOCFG_8_14__CRYPTOKEY_8_14__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_15 : CRYPTOKEY_8_15      */
#define CRYPTOCFG_8_15__CRYPTOKEY_8_15__SHIFT                           0x00000000
#define CRYPTOCFG_8_15__CRYPTOKEY_8_15__WIDTH                           0x00000020
#define CRYPTOCFG_8_15__CRYPTOKEY_8_15__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_16 : DUSIZE_8            */
#define CRYPTOCFG_8_16__DUSIZE_8__SHIFT                                 0x00000000
#define CRYPTOCFG_8_16__DUSIZE_8__WIDTH                                 0x00000008
#define CRYPTOCFG_8_16__DUSIZE_8__MASK                                  0x000000FF

/*  CRYPTOCFG_8_16 : CAPIDX_8            */
#define CRYPTOCFG_8_16__CAPIDX_8__SHIFT                                 0x00000008
#define CRYPTOCFG_8_16__CAPIDX_8__WIDTH                                 0x00000008
#define CRYPTOCFG_8_16__CAPIDX_8__MASK                                  0x0000FF00

/*  CRYPTOCFG_8_16 : CRYPTPCFG_RSVD_8    */
#define CRYPTOCFG_8_16__CRYPTPCFG_RSVD_8__SHIFT                         0x00000010
#define CRYPTOCFG_8_16__CRYPTPCFG_RSVD_8__WIDTH                         0x0000000F
#define CRYPTOCFG_8_16__CRYPTPCFG_RSVD_8__MASK                          0x7FFF0000

/*  CRYPTOCFG_8_16 : CFGE_8              */
#define CRYPTOCFG_8_16__CFGE_8__SHIFT                                   0x0000001F
#define CRYPTOCFG_8_16__CFGE_8__WIDTH                                   0x00000001
#define CRYPTOCFG_8_16__CFGE_8__MASK                                    0x80000000



/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_17 : RESERVED_8          */
#define CRYPTOCFG_8_17__RESERVED_8__SHIFT                               0x00000000
#define CRYPTOCFG_8_17__RESERVED_8__WIDTH                               0x00000020
#define CRYPTOCFG_8_17__RESERVED_8__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_18 : RESERVED_8          */
#define CRYPTOCFG_8_18__RESERVED_8__SHIFT                               0x00000000
#define CRYPTOCFG_8_18__RESERVED_8__WIDTH                               0x00000020
#define CRYPTOCFG_8_18__RESERVED_8__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_19 : RESERVED_8          */
#define CRYPTOCFG_8_19__RESERVED_8__SHIFT                               0x00000000
#define CRYPTOCFG_8_19__RESERVED_8__WIDTH                               0x00000020
#define CRYPTOCFG_8_19__RESERVED_8__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_20 : RESERVED_8          */
#define CRYPTOCFG_8_20__RESERVED_8__SHIFT                               0x00000000
#define CRYPTOCFG_8_20__RESERVED_8__WIDTH                               0x00000020
#define CRYPTOCFG_8_20__RESERVED_8__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_21 : RESERVED_8          */
#define CRYPTOCFG_8_21__RESERVED_8__SHIFT                               0x00000000
#define CRYPTOCFG_8_21__RESERVED_8__WIDTH                               0x00000020
#define CRYPTOCFG_8_21__RESERVED_8__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_22 : RESERVED_8          */
#define CRYPTOCFG_8_22__RESERVED_8__SHIFT                               0x00000000
#define CRYPTOCFG_8_22__RESERVED_8__WIDTH                               0x00000020
#define CRYPTOCFG_8_22__RESERVED_8__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_23 : RESERVED_8          */
#define CRYPTOCFG_8_23__RESERVED_8__SHIFT                               0x00000000
#define CRYPTOCFG_8_23__RESERVED_8__WIDTH                               0x00000020
#define CRYPTOCFG_8_23__RESERVED_8__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_24 : RESERVED_8          */
#define CRYPTOCFG_8_24__RESERVED_8__SHIFT                               0x00000000
#define CRYPTOCFG_8_24__RESERVED_8__WIDTH                               0x00000020
#define CRYPTOCFG_8_24__RESERVED_8__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_25 : RESERVED_8          */
#define CRYPTOCFG_8_25__RESERVED_8__SHIFT                               0x00000000
#define CRYPTOCFG_8_25__RESERVED_8__WIDTH                               0x00000020
#define CRYPTOCFG_8_25__RESERVED_8__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_26 : RESERVED_8          */
#define CRYPTOCFG_8_26__RESERVED_8__SHIFT                               0x00000000
#define CRYPTOCFG_8_26__RESERVED_8__WIDTH                               0x00000020
#define CRYPTOCFG_8_26__RESERVED_8__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_27 : RESERVED_8          */
#define CRYPTOCFG_8_27__RESERVED_8__SHIFT                               0x00000000
#define CRYPTOCFG_8_27__RESERVED_8__WIDTH                               0x00000020
#define CRYPTOCFG_8_27__RESERVED_8__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_28 : RESERVED_8          */
#define CRYPTOCFG_8_28__RESERVED_8__SHIFT                               0x00000000
#define CRYPTOCFG_8_28__RESERVED_8__WIDTH                               0x00000020
#define CRYPTOCFG_8_28__RESERVED_8__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_29 : RESERVED_8          */
#define CRYPTOCFG_8_29__RESERVED_8__SHIFT                               0x00000000
#define CRYPTOCFG_8_29__RESERVED_8__WIDTH                               0x00000020
#define CRYPTOCFG_8_29__RESERVED_8__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_30 : RESERVED_8          */
#define CRYPTOCFG_8_30__RESERVED_8__SHIFT                               0x00000000
#define CRYPTOCFG_8_30__RESERVED_8__WIDTH                               0x00000020
#define CRYPTOCFG_8_30__RESERVED_8__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_8_31 : RESERVED_8          */
#define CRYPTOCFG_8_31__RESERVED_8__SHIFT                               0x00000000
#define CRYPTOCFG_8_31__RESERVED_8__WIDTH                               0x00000020
#define CRYPTOCFG_8_31__RESERVED_8__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_0 : CRYPTOKEY_9_0       */
#define CRYPTOCFG_9_0__CRYPTOKEY_9_0__SHIFT                             0x00000000
#define CRYPTOCFG_9_0__CRYPTOKEY_9_0__WIDTH                             0x00000020
#define CRYPTOCFG_9_0__CRYPTOKEY_9_0__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_1 : CRYPTOKEY_9_1       */
#define CRYPTOCFG_9_1__CRYPTOKEY_9_1__SHIFT                             0x00000000
#define CRYPTOCFG_9_1__CRYPTOKEY_9_1__WIDTH                             0x00000020
#define CRYPTOCFG_9_1__CRYPTOKEY_9_1__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_2 : CRYPTOKEY_9_2       */
#define CRYPTOCFG_9_2__CRYPTOKEY_9_2__SHIFT                             0x00000000
#define CRYPTOCFG_9_2__CRYPTOKEY_9_2__WIDTH                             0x00000020
#define CRYPTOCFG_9_2__CRYPTOKEY_9_2__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_3 : CRYPTOKEY_9_3       */
#define CRYPTOCFG_9_3__CRYPTOKEY_9_3__SHIFT                             0x00000000
#define CRYPTOCFG_9_3__CRYPTOKEY_9_3__WIDTH                             0x00000020
#define CRYPTOCFG_9_3__CRYPTOKEY_9_3__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_4 : CRYPTOKEY_9_4       */
#define CRYPTOCFG_9_4__CRYPTOKEY_9_4__SHIFT                             0x00000000
#define CRYPTOCFG_9_4__CRYPTOKEY_9_4__WIDTH                             0x00000020
#define CRYPTOCFG_9_4__CRYPTOKEY_9_4__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_5 : CRYPTOKEY_9_5       */
#define CRYPTOCFG_9_5__CRYPTOKEY_9_5__SHIFT                             0x00000000
#define CRYPTOCFG_9_5__CRYPTOKEY_9_5__WIDTH                             0x00000020
#define CRYPTOCFG_9_5__CRYPTOKEY_9_5__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_6 : CRYPTOKEY_9_6       */
#define CRYPTOCFG_9_6__CRYPTOKEY_9_6__SHIFT                             0x00000000
#define CRYPTOCFG_9_6__CRYPTOKEY_9_6__WIDTH                             0x00000020
#define CRYPTOCFG_9_6__CRYPTOKEY_9_6__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_7 : CRYPTOKEY_9_7       */
#define CRYPTOCFG_9_7__CRYPTOKEY_9_7__SHIFT                             0x00000000
#define CRYPTOCFG_9_7__CRYPTOKEY_9_7__WIDTH                             0x00000020
#define CRYPTOCFG_9_7__CRYPTOKEY_9_7__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_8 : CRYPTOKEY_9_8       */
#define CRYPTOCFG_9_8__CRYPTOKEY_9_8__SHIFT                             0x00000000
#define CRYPTOCFG_9_8__CRYPTOKEY_9_8__WIDTH                             0x00000020
#define CRYPTOCFG_9_8__CRYPTOKEY_9_8__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_9 : CRYPTOKEY_9_9       */
#define CRYPTOCFG_9_9__CRYPTOKEY_9_9__SHIFT                             0x00000000
#define CRYPTOCFG_9_9__CRYPTOKEY_9_9__WIDTH                             0x00000020
#define CRYPTOCFG_9_9__CRYPTOKEY_9_9__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_10 : CRYPTOKEY_9_10      */
#define CRYPTOCFG_9_10__CRYPTOKEY_9_10__SHIFT                           0x00000000
#define CRYPTOCFG_9_10__CRYPTOKEY_9_10__WIDTH                           0x00000020
#define CRYPTOCFG_9_10__CRYPTOKEY_9_10__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_11 : CRYPTOKEY_9_11      */
#define CRYPTOCFG_9_11__CRYPTOKEY_9_11__SHIFT                           0x00000000
#define CRYPTOCFG_9_11__CRYPTOKEY_9_11__WIDTH                           0x00000020
#define CRYPTOCFG_9_11__CRYPTOKEY_9_11__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_12 : CRYPTOKEY_9_12      */
#define CRYPTOCFG_9_12__CRYPTOKEY_9_12__SHIFT                           0x00000000
#define CRYPTOCFG_9_12__CRYPTOKEY_9_12__WIDTH                           0x00000020
#define CRYPTOCFG_9_12__CRYPTOKEY_9_12__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_13 : CRYPTOKEY_9_13      */
#define CRYPTOCFG_9_13__CRYPTOKEY_9_13__SHIFT                           0x00000000
#define CRYPTOCFG_9_13__CRYPTOKEY_9_13__WIDTH                           0x00000020
#define CRYPTOCFG_9_13__CRYPTOKEY_9_13__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_14 : CRYPTOKEY_9_14      */
#define CRYPTOCFG_9_14__CRYPTOKEY_9_14__SHIFT                           0x00000000
#define CRYPTOCFG_9_14__CRYPTOKEY_9_14__WIDTH                           0x00000020
#define CRYPTOCFG_9_14__CRYPTOKEY_9_14__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_15 : CRYPTOKEY_9_15      */
#define CRYPTOCFG_9_15__CRYPTOKEY_9_15__SHIFT                           0x00000000
#define CRYPTOCFG_9_15__CRYPTOKEY_9_15__WIDTH                           0x00000020
#define CRYPTOCFG_9_15__CRYPTOKEY_9_15__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_16 : DUSIZE_9            */
#define CRYPTOCFG_9_16__DUSIZE_9__SHIFT                                 0x00000000
#define CRYPTOCFG_9_16__DUSIZE_9__WIDTH                                 0x00000008
#define CRYPTOCFG_9_16__DUSIZE_9__MASK                                  0x000000FF

/*  CRYPTOCFG_9_16 : CAPIDX_9            */
#define CRYPTOCFG_9_16__CAPIDX_9__SHIFT                                 0x00000008
#define CRYPTOCFG_9_16__CAPIDX_9__WIDTH                                 0x00000008
#define CRYPTOCFG_9_16__CAPIDX_9__MASK                                  0x0000FF00

/*  CRYPTOCFG_9_16 : CRYPTPCFG_RSVD_9    */
#define CRYPTOCFG_9_16__CRYPTPCFG_RSVD_9__SHIFT                         0x00000010
#define CRYPTOCFG_9_16__CRYPTPCFG_RSVD_9__WIDTH                         0x0000000F
#define CRYPTOCFG_9_16__CRYPTPCFG_RSVD_9__MASK                          0x7FFF0000

/*  CRYPTOCFG_9_16 : CFGE_9              */
#define CRYPTOCFG_9_16__CFGE_9__SHIFT                                   0x0000001F
#define CRYPTOCFG_9_16__CFGE_9__WIDTH                                   0x00000001
#define CRYPTOCFG_9_16__CFGE_9__MASK                                    0x80000000



/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_17 : RESERVED_9          */
#define CRYPTOCFG_9_17__RESERVED_9__SHIFT                               0x00000000
#define CRYPTOCFG_9_17__RESERVED_9__WIDTH                               0x00000020
#define CRYPTOCFG_9_17__RESERVED_9__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_18 : RESERVED_9          */
#define CRYPTOCFG_9_18__RESERVED_9__SHIFT                               0x00000000
#define CRYPTOCFG_9_18__RESERVED_9__WIDTH                               0x00000020
#define CRYPTOCFG_9_18__RESERVED_9__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_19 : RESERVED_9          */
#define CRYPTOCFG_9_19__RESERVED_9__SHIFT                               0x00000000
#define CRYPTOCFG_9_19__RESERVED_9__WIDTH                               0x00000020
#define CRYPTOCFG_9_19__RESERVED_9__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_20 : RESERVED_9          */
#define CRYPTOCFG_9_20__RESERVED_9__SHIFT                               0x00000000
#define CRYPTOCFG_9_20__RESERVED_9__WIDTH                               0x00000020
#define CRYPTOCFG_9_20__RESERVED_9__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_21 : RESERVED_9          */
#define CRYPTOCFG_9_21__RESERVED_9__SHIFT                               0x00000000
#define CRYPTOCFG_9_21__RESERVED_9__WIDTH                               0x00000020
#define CRYPTOCFG_9_21__RESERVED_9__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_22 : RESERVED_9          */
#define CRYPTOCFG_9_22__RESERVED_9__SHIFT                               0x00000000
#define CRYPTOCFG_9_22__RESERVED_9__WIDTH                               0x00000020
#define CRYPTOCFG_9_22__RESERVED_9__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_23 : RESERVED_9          */
#define CRYPTOCFG_9_23__RESERVED_9__SHIFT                               0x00000000
#define CRYPTOCFG_9_23__RESERVED_9__WIDTH                               0x00000020
#define CRYPTOCFG_9_23__RESERVED_9__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_24 : RESERVED_9          */
#define CRYPTOCFG_9_24__RESERVED_9__SHIFT                               0x00000000
#define CRYPTOCFG_9_24__RESERVED_9__WIDTH                               0x00000020
#define CRYPTOCFG_9_24__RESERVED_9__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_25 : RESERVED_9          */
#define CRYPTOCFG_9_25__RESERVED_9__SHIFT                               0x00000000
#define CRYPTOCFG_9_25__RESERVED_9__WIDTH                               0x00000020
#define CRYPTOCFG_9_25__RESERVED_9__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_26 : RESERVED_9          */
#define CRYPTOCFG_9_26__RESERVED_9__SHIFT                               0x00000000
#define CRYPTOCFG_9_26__RESERVED_9__WIDTH                               0x00000020
#define CRYPTOCFG_9_26__RESERVED_9__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_27 : RESERVED_9          */
#define CRYPTOCFG_9_27__RESERVED_9__SHIFT                               0x00000000
#define CRYPTOCFG_9_27__RESERVED_9__WIDTH                               0x00000020
#define CRYPTOCFG_9_27__RESERVED_9__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_28 : RESERVED_9          */
#define CRYPTOCFG_9_28__RESERVED_9__SHIFT                               0x00000000
#define CRYPTOCFG_9_28__RESERVED_9__WIDTH                               0x00000020
#define CRYPTOCFG_9_28__RESERVED_9__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_29 : RESERVED_9          */
#define CRYPTOCFG_9_29__RESERVED_9__SHIFT                               0x00000000
#define CRYPTOCFG_9_29__RESERVED_9__WIDTH                               0x00000020
#define CRYPTOCFG_9_29__RESERVED_9__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_30 : RESERVED_9          */
#define CRYPTOCFG_9_30__RESERVED_9__SHIFT                               0x00000000
#define CRYPTOCFG_9_30__RESERVED_9__WIDTH                               0x00000020
#define CRYPTOCFG_9_30__RESERVED_9__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_9_31 : RESERVED_9          */
#define CRYPTOCFG_9_31__RESERVED_9__SHIFT                               0x00000000
#define CRYPTOCFG_9_31__RESERVED_9__WIDTH                               0x00000020
#define CRYPTOCFG_9_31__RESERVED_9__MASK                                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_0 : CRYPTOKEY_10_0      */
#define CRYPTOCFG_10_0__CRYPTOKEY_10_0__SHIFT                           0x00000000
#define CRYPTOCFG_10_0__CRYPTOKEY_10_0__WIDTH                           0x00000020
#define CRYPTOCFG_10_0__CRYPTOKEY_10_0__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_1 : CRYPTOKEY_10_1      */
#define CRYPTOCFG_10_1__CRYPTOKEY_10_1__SHIFT                           0x00000000
#define CRYPTOCFG_10_1__CRYPTOKEY_10_1__WIDTH                           0x00000020
#define CRYPTOCFG_10_1__CRYPTOKEY_10_1__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_2 : CRYPTOKEY_10_2      */
#define CRYPTOCFG_10_2__CRYPTOKEY_10_2__SHIFT                           0x00000000
#define CRYPTOCFG_10_2__CRYPTOKEY_10_2__WIDTH                           0x00000020
#define CRYPTOCFG_10_2__CRYPTOKEY_10_2__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_3 : CRYPTOKEY_10_3      */
#define CRYPTOCFG_10_3__CRYPTOKEY_10_3__SHIFT                           0x00000000
#define CRYPTOCFG_10_3__CRYPTOKEY_10_3__WIDTH                           0x00000020
#define CRYPTOCFG_10_3__CRYPTOKEY_10_3__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_4 : CRYPTOKEY_10_4      */
#define CRYPTOCFG_10_4__CRYPTOKEY_10_4__SHIFT                           0x00000000
#define CRYPTOCFG_10_4__CRYPTOKEY_10_4__WIDTH                           0x00000020
#define CRYPTOCFG_10_4__CRYPTOKEY_10_4__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_5 : CRYPTOKEY_10_5      */
#define CRYPTOCFG_10_5__CRYPTOKEY_10_5__SHIFT                           0x00000000
#define CRYPTOCFG_10_5__CRYPTOKEY_10_5__WIDTH                           0x00000020
#define CRYPTOCFG_10_5__CRYPTOKEY_10_5__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_6 : CRYPTOKEY_10_6      */
#define CRYPTOCFG_10_6__CRYPTOKEY_10_6__SHIFT                           0x00000000
#define CRYPTOCFG_10_6__CRYPTOKEY_10_6__WIDTH                           0x00000020
#define CRYPTOCFG_10_6__CRYPTOKEY_10_6__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_7 : CRYPTOKEY_10_7      */
#define CRYPTOCFG_10_7__CRYPTOKEY_10_7__SHIFT                           0x00000000
#define CRYPTOCFG_10_7__CRYPTOKEY_10_7__WIDTH                           0x00000020
#define CRYPTOCFG_10_7__CRYPTOKEY_10_7__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_8 : CRYPTOKEY_10_8      */
#define CRYPTOCFG_10_8__CRYPTOKEY_10_8__SHIFT                           0x00000000
#define CRYPTOCFG_10_8__CRYPTOKEY_10_8__WIDTH                           0x00000020
#define CRYPTOCFG_10_8__CRYPTOKEY_10_8__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_9 : CRYPTOKEY_10_9      */
#define CRYPTOCFG_10_9__CRYPTOKEY_10_9__SHIFT                           0x00000000
#define CRYPTOCFG_10_9__CRYPTOKEY_10_9__WIDTH                           0x00000020
#define CRYPTOCFG_10_9__CRYPTOKEY_10_9__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_10 : CRYPTOKEY_10_10     */
#define CRYPTOCFG_10_10__CRYPTOKEY_10_10__SHIFT                         0x00000000
#define CRYPTOCFG_10_10__CRYPTOKEY_10_10__WIDTH                         0x00000020
#define CRYPTOCFG_10_10__CRYPTOKEY_10_10__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_11 : CRYPTOKEY_10_11     */
#define CRYPTOCFG_10_11__CRYPTOKEY_10_11__SHIFT                         0x00000000
#define CRYPTOCFG_10_11__CRYPTOKEY_10_11__WIDTH                         0x00000020
#define CRYPTOCFG_10_11__CRYPTOKEY_10_11__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_12 : CRYPTOKEY_10_12     */
#define CRYPTOCFG_10_12__CRYPTOKEY_10_12__SHIFT                         0x00000000
#define CRYPTOCFG_10_12__CRYPTOKEY_10_12__WIDTH                         0x00000020
#define CRYPTOCFG_10_12__CRYPTOKEY_10_12__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_13 : CRYPTOKEY_10_13     */
#define CRYPTOCFG_10_13__CRYPTOKEY_10_13__SHIFT                         0x00000000
#define CRYPTOCFG_10_13__CRYPTOKEY_10_13__WIDTH                         0x00000020
#define CRYPTOCFG_10_13__CRYPTOKEY_10_13__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_14 : CRYPTOKEY_10_14     */
#define CRYPTOCFG_10_14__CRYPTOKEY_10_14__SHIFT                         0x00000000
#define CRYPTOCFG_10_14__CRYPTOKEY_10_14__WIDTH                         0x00000020
#define CRYPTOCFG_10_14__CRYPTOKEY_10_14__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_15 : CRYPTOKEY_10_15     */
#define CRYPTOCFG_10_15__CRYPTOKEY_10_15__SHIFT                         0x00000000
#define CRYPTOCFG_10_15__CRYPTOKEY_10_15__WIDTH                         0x00000020
#define CRYPTOCFG_10_15__CRYPTOKEY_10_15__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_16 : DUSIZE_10           */
#define CRYPTOCFG_10_16__DUSIZE_10__SHIFT                               0x00000000
#define CRYPTOCFG_10_16__DUSIZE_10__WIDTH                               0x00000008
#define CRYPTOCFG_10_16__DUSIZE_10__MASK                                0x000000FF

/*  CRYPTOCFG_10_16 : CAPIDX_10           */
#define CRYPTOCFG_10_16__CAPIDX_10__SHIFT                               0x00000008
#define CRYPTOCFG_10_16__CAPIDX_10__WIDTH                               0x00000008
#define CRYPTOCFG_10_16__CAPIDX_10__MASK                                0x0000FF00

/*  CRYPTOCFG_10_16 : CRYPTPCFG_RSVD_10   */
#define CRYPTOCFG_10_16__CRYPTPCFG_RSVD_10__SHIFT                       0x00000010
#define CRYPTOCFG_10_16__CRYPTPCFG_RSVD_10__WIDTH                       0x0000000F
#define CRYPTOCFG_10_16__CRYPTPCFG_RSVD_10__MASK                        0x7FFF0000

/*  CRYPTOCFG_10_16 : CFGE_10             */
#define CRYPTOCFG_10_16__CFGE_10__SHIFT                                 0x0000001F
#define CRYPTOCFG_10_16__CFGE_10__WIDTH                                 0x00000001
#define CRYPTOCFG_10_16__CFGE_10__MASK                                  0x80000000



/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_17 : RESERVED_10         */
#define CRYPTOCFG_10_17__RESERVED_10__SHIFT                             0x00000000
#define CRYPTOCFG_10_17__RESERVED_10__WIDTH                             0x00000020
#define CRYPTOCFG_10_17__RESERVED_10__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_18 : RESERVED_10         */
#define CRYPTOCFG_10_18__RESERVED_10__SHIFT                             0x00000000
#define CRYPTOCFG_10_18__RESERVED_10__WIDTH                             0x00000020
#define CRYPTOCFG_10_18__RESERVED_10__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_19 : RESERVED_10         */
#define CRYPTOCFG_10_19__RESERVED_10__SHIFT                             0x00000000
#define CRYPTOCFG_10_19__RESERVED_10__WIDTH                             0x00000020
#define CRYPTOCFG_10_19__RESERVED_10__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_20 : RESERVED_10         */
#define CRYPTOCFG_10_20__RESERVED_10__SHIFT                             0x00000000
#define CRYPTOCFG_10_20__RESERVED_10__WIDTH                             0x00000020
#define CRYPTOCFG_10_20__RESERVED_10__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_21 : RESERVED_10         */
#define CRYPTOCFG_10_21__RESERVED_10__SHIFT                             0x00000000
#define CRYPTOCFG_10_21__RESERVED_10__WIDTH                             0x00000020
#define CRYPTOCFG_10_21__RESERVED_10__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_22 : RESERVED_10         */
#define CRYPTOCFG_10_22__RESERVED_10__SHIFT                             0x00000000
#define CRYPTOCFG_10_22__RESERVED_10__WIDTH                             0x00000020
#define CRYPTOCFG_10_22__RESERVED_10__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_23 : RESERVED_10         */
#define CRYPTOCFG_10_23__RESERVED_10__SHIFT                             0x00000000
#define CRYPTOCFG_10_23__RESERVED_10__WIDTH                             0x00000020
#define CRYPTOCFG_10_23__RESERVED_10__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_24 : RESERVED_10         */
#define CRYPTOCFG_10_24__RESERVED_10__SHIFT                             0x00000000
#define CRYPTOCFG_10_24__RESERVED_10__WIDTH                             0x00000020
#define CRYPTOCFG_10_24__RESERVED_10__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_25 : RESERVED_10         */
#define CRYPTOCFG_10_25__RESERVED_10__SHIFT                             0x00000000
#define CRYPTOCFG_10_25__RESERVED_10__WIDTH                             0x00000020
#define CRYPTOCFG_10_25__RESERVED_10__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_26 : RESERVED_10         */
#define CRYPTOCFG_10_26__RESERVED_10__SHIFT                             0x00000000
#define CRYPTOCFG_10_26__RESERVED_10__WIDTH                             0x00000020
#define CRYPTOCFG_10_26__RESERVED_10__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_27 : RESERVED_10         */
#define CRYPTOCFG_10_27__RESERVED_10__SHIFT                             0x00000000
#define CRYPTOCFG_10_27__RESERVED_10__WIDTH                             0x00000020
#define CRYPTOCFG_10_27__RESERVED_10__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_28 : RESERVED_10         */
#define CRYPTOCFG_10_28__RESERVED_10__SHIFT                             0x00000000
#define CRYPTOCFG_10_28__RESERVED_10__WIDTH                             0x00000020
#define CRYPTOCFG_10_28__RESERVED_10__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_29 : RESERVED_10         */
#define CRYPTOCFG_10_29__RESERVED_10__SHIFT                             0x00000000
#define CRYPTOCFG_10_29__RESERVED_10__WIDTH                             0x00000020
#define CRYPTOCFG_10_29__RESERVED_10__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_30 : RESERVED_10         */
#define CRYPTOCFG_10_30__RESERVED_10__SHIFT                             0x00000000
#define CRYPTOCFG_10_30__RESERVED_10__WIDTH                             0x00000020
#define CRYPTOCFG_10_30__RESERVED_10__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_10_31 : RESERVED_10         */
#define CRYPTOCFG_10_31__RESERVED_10__SHIFT                             0x00000000
#define CRYPTOCFG_10_31__RESERVED_10__WIDTH                             0x00000020
#define CRYPTOCFG_10_31__RESERVED_10__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_0 : CRYPTOKEY_11_0      */
#define CRYPTOCFG_11_0__CRYPTOKEY_11_0__SHIFT                           0x00000000
#define CRYPTOCFG_11_0__CRYPTOKEY_11_0__WIDTH                           0x00000020
#define CRYPTOCFG_11_0__CRYPTOKEY_11_0__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_1 : CRYPTOKEY_11_1      */
#define CRYPTOCFG_11_1__CRYPTOKEY_11_1__SHIFT                           0x00000000
#define CRYPTOCFG_11_1__CRYPTOKEY_11_1__WIDTH                           0x00000020
#define CRYPTOCFG_11_1__CRYPTOKEY_11_1__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_2 : CRYPTOKEY_11_2      */
#define CRYPTOCFG_11_2__CRYPTOKEY_11_2__SHIFT                           0x00000000
#define CRYPTOCFG_11_2__CRYPTOKEY_11_2__WIDTH                           0x00000020
#define CRYPTOCFG_11_2__CRYPTOKEY_11_2__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_3 : CRYPTOKEY_11_3      */
#define CRYPTOCFG_11_3__CRYPTOKEY_11_3__SHIFT                           0x00000000
#define CRYPTOCFG_11_3__CRYPTOKEY_11_3__WIDTH                           0x00000020
#define CRYPTOCFG_11_3__CRYPTOKEY_11_3__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_4 : CRYPTOKEY_11_4      */
#define CRYPTOCFG_11_4__CRYPTOKEY_11_4__SHIFT                           0x00000000
#define CRYPTOCFG_11_4__CRYPTOKEY_11_4__WIDTH                           0x00000020
#define CRYPTOCFG_11_4__CRYPTOKEY_11_4__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_5 : CRYPTOKEY_11_5      */
#define CRYPTOCFG_11_5__CRYPTOKEY_11_5__SHIFT                           0x00000000
#define CRYPTOCFG_11_5__CRYPTOKEY_11_5__WIDTH                           0x00000020
#define CRYPTOCFG_11_5__CRYPTOKEY_11_5__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_6 : CRYPTOKEY_11_6      */
#define CRYPTOCFG_11_6__CRYPTOKEY_11_6__SHIFT                           0x00000000
#define CRYPTOCFG_11_6__CRYPTOKEY_11_6__WIDTH                           0x00000020
#define CRYPTOCFG_11_6__CRYPTOKEY_11_6__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_7 : CRYPTOKEY_11_7      */
#define CRYPTOCFG_11_7__CRYPTOKEY_11_7__SHIFT                           0x00000000
#define CRYPTOCFG_11_7__CRYPTOKEY_11_7__WIDTH                           0x00000020
#define CRYPTOCFG_11_7__CRYPTOKEY_11_7__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_8 : CRYPTOKEY_11_8      */
#define CRYPTOCFG_11_8__CRYPTOKEY_11_8__SHIFT                           0x00000000
#define CRYPTOCFG_11_8__CRYPTOKEY_11_8__WIDTH                           0x00000020
#define CRYPTOCFG_11_8__CRYPTOKEY_11_8__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_9 : CRYPTOKEY_11_9      */
#define CRYPTOCFG_11_9__CRYPTOKEY_11_9__SHIFT                           0x00000000
#define CRYPTOCFG_11_9__CRYPTOKEY_11_9__WIDTH                           0x00000020
#define CRYPTOCFG_11_9__CRYPTOKEY_11_9__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_10 : CRYPTOKEY_11_10     */
#define CRYPTOCFG_11_10__CRYPTOKEY_11_10__SHIFT                         0x00000000
#define CRYPTOCFG_11_10__CRYPTOKEY_11_10__WIDTH                         0x00000020
#define CRYPTOCFG_11_10__CRYPTOKEY_11_10__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_11 : CRYPTOKEY_11_11     */
#define CRYPTOCFG_11_11__CRYPTOKEY_11_11__SHIFT                         0x00000000
#define CRYPTOCFG_11_11__CRYPTOKEY_11_11__WIDTH                         0x00000020
#define CRYPTOCFG_11_11__CRYPTOKEY_11_11__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_12 : CRYPTOKEY_11_12     */
#define CRYPTOCFG_11_12__CRYPTOKEY_11_12__SHIFT                         0x00000000
#define CRYPTOCFG_11_12__CRYPTOKEY_11_12__WIDTH                         0x00000020
#define CRYPTOCFG_11_12__CRYPTOKEY_11_12__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_13 : CRYPTOKEY_11_13     */
#define CRYPTOCFG_11_13__CRYPTOKEY_11_13__SHIFT                         0x00000000
#define CRYPTOCFG_11_13__CRYPTOKEY_11_13__WIDTH                         0x00000020
#define CRYPTOCFG_11_13__CRYPTOKEY_11_13__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_14 : CRYPTOKEY_11_14     */
#define CRYPTOCFG_11_14__CRYPTOKEY_11_14__SHIFT                         0x00000000
#define CRYPTOCFG_11_14__CRYPTOKEY_11_14__WIDTH                         0x00000020
#define CRYPTOCFG_11_14__CRYPTOKEY_11_14__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_15 : CRYPTOKEY_11_15     */
#define CRYPTOCFG_11_15__CRYPTOKEY_11_15__SHIFT                         0x00000000
#define CRYPTOCFG_11_15__CRYPTOKEY_11_15__WIDTH                         0x00000020
#define CRYPTOCFG_11_15__CRYPTOKEY_11_15__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_16 : DUSIZE_11           */
#define CRYPTOCFG_11_16__DUSIZE_11__SHIFT                               0x00000000
#define CRYPTOCFG_11_16__DUSIZE_11__WIDTH                               0x00000008
#define CRYPTOCFG_11_16__DUSIZE_11__MASK                                0x000000FF

/*  CRYPTOCFG_11_16 : CAPIDX_11           */
#define CRYPTOCFG_11_16__CAPIDX_11__SHIFT                               0x00000008
#define CRYPTOCFG_11_16__CAPIDX_11__WIDTH                               0x00000008
#define CRYPTOCFG_11_16__CAPIDX_11__MASK                                0x0000FF00

/*  CRYPTOCFG_11_16 : CRYPTPCFG_RSVD_11   */
#define CRYPTOCFG_11_16__CRYPTPCFG_RSVD_11__SHIFT                       0x00000010
#define CRYPTOCFG_11_16__CRYPTPCFG_RSVD_11__WIDTH                       0x0000000F
#define CRYPTOCFG_11_16__CRYPTPCFG_RSVD_11__MASK                        0x7FFF0000

/*  CRYPTOCFG_11_16 : CFGE_11             */
#define CRYPTOCFG_11_16__CFGE_11__SHIFT                                 0x0000001F
#define CRYPTOCFG_11_16__CFGE_11__WIDTH                                 0x00000001
#define CRYPTOCFG_11_16__CFGE_11__MASK                                  0x80000000



/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_17 : RESERVED_11         */
#define CRYPTOCFG_11_17__RESERVED_11__SHIFT                             0x00000000
#define CRYPTOCFG_11_17__RESERVED_11__WIDTH                             0x00000020
#define CRYPTOCFG_11_17__RESERVED_11__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_18 : RESERVED_11         */
#define CRYPTOCFG_11_18__RESERVED_11__SHIFT                             0x00000000
#define CRYPTOCFG_11_18__RESERVED_11__WIDTH                             0x00000020
#define CRYPTOCFG_11_18__RESERVED_11__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_19 : RESERVED_11         */
#define CRYPTOCFG_11_19__RESERVED_11__SHIFT                             0x00000000
#define CRYPTOCFG_11_19__RESERVED_11__WIDTH                             0x00000020
#define CRYPTOCFG_11_19__RESERVED_11__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_20 : RESERVED_11         */
#define CRYPTOCFG_11_20__RESERVED_11__SHIFT                             0x00000000
#define CRYPTOCFG_11_20__RESERVED_11__WIDTH                             0x00000020
#define CRYPTOCFG_11_20__RESERVED_11__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_21 : RESERVED_11         */
#define CRYPTOCFG_11_21__RESERVED_11__SHIFT                             0x00000000
#define CRYPTOCFG_11_21__RESERVED_11__WIDTH                             0x00000020
#define CRYPTOCFG_11_21__RESERVED_11__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_22 : RESERVED_11         */
#define CRYPTOCFG_11_22__RESERVED_11__SHIFT                             0x00000000
#define CRYPTOCFG_11_22__RESERVED_11__WIDTH                             0x00000020
#define CRYPTOCFG_11_22__RESERVED_11__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_23 : RESERVED_11         */
#define CRYPTOCFG_11_23__RESERVED_11__SHIFT                             0x00000000
#define CRYPTOCFG_11_23__RESERVED_11__WIDTH                             0x00000020
#define CRYPTOCFG_11_23__RESERVED_11__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_24 : RESERVED_11         */
#define CRYPTOCFG_11_24__RESERVED_11__SHIFT                             0x00000000
#define CRYPTOCFG_11_24__RESERVED_11__WIDTH                             0x00000020
#define CRYPTOCFG_11_24__RESERVED_11__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_25 : RESERVED_11         */
#define CRYPTOCFG_11_25__RESERVED_11__SHIFT                             0x00000000
#define CRYPTOCFG_11_25__RESERVED_11__WIDTH                             0x00000020
#define CRYPTOCFG_11_25__RESERVED_11__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_26 : RESERVED_11         */
#define CRYPTOCFG_11_26__RESERVED_11__SHIFT                             0x00000000
#define CRYPTOCFG_11_26__RESERVED_11__WIDTH                             0x00000020
#define CRYPTOCFG_11_26__RESERVED_11__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_27 : RESERVED_11         */
#define CRYPTOCFG_11_27__RESERVED_11__SHIFT                             0x00000000
#define CRYPTOCFG_11_27__RESERVED_11__WIDTH                             0x00000020
#define CRYPTOCFG_11_27__RESERVED_11__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_28 : RESERVED_11         */
#define CRYPTOCFG_11_28__RESERVED_11__SHIFT                             0x00000000
#define CRYPTOCFG_11_28__RESERVED_11__WIDTH                             0x00000020
#define CRYPTOCFG_11_28__RESERVED_11__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_29 : RESERVED_11         */
#define CRYPTOCFG_11_29__RESERVED_11__SHIFT                             0x00000000
#define CRYPTOCFG_11_29__RESERVED_11__WIDTH                             0x00000020
#define CRYPTOCFG_11_29__RESERVED_11__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_30 : RESERVED_11         */
#define CRYPTOCFG_11_30__RESERVED_11__SHIFT                             0x00000000
#define CRYPTOCFG_11_30__RESERVED_11__WIDTH                             0x00000020
#define CRYPTOCFG_11_30__RESERVED_11__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_11_31 : RESERVED_11         */
#define CRYPTOCFG_11_31__RESERVED_11__SHIFT                             0x00000000
#define CRYPTOCFG_11_31__RESERVED_11__WIDTH                             0x00000020
#define CRYPTOCFG_11_31__RESERVED_11__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_0 : CRYPTOKEY_12_0      */
#define CRYPTOCFG_12_0__CRYPTOKEY_12_0__SHIFT                           0x00000000
#define CRYPTOCFG_12_0__CRYPTOKEY_12_0__WIDTH                           0x00000020
#define CRYPTOCFG_12_0__CRYPTOKEY_12_0__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_1 : CRYPTOKEY_12_1      */
#define CRYPTOCFG_12_1__CRYPTOKEY_12_1__SHIFT                           0x00000000
#define CRYPTOCFG_12_1__CRYPTOKEY_12_1__WIDTH                           0x00000020
#define CRYPTOCFG_12_1__CRYPTOKEY_12_1__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_2 : CRYPTOKEY_12_2      */
#define CRYPTOCFG_12_2__CRYPTOKEY_12_2__SHIFT                           0x00000000
#define CRYPTOCFG_12_2__CRYPTOKEY_12_2__WIDTH                           0x00000020
#define CRYPTOCFG_12_2__CRYPTOKEY_12_2__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_3 : CRYPTOKEY_12_3      */
#define CRYPTOCFG_12_3__CRYPTOKEY_12_3__SHIFT                           0x00000000
#define CRYPTOCFG_12_3__CRYPTOKEY_12_3__WIDTH                           0x00000020
#define CRYPTOCFG_12_3__CRYPTOKEY_12_3__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_4 : CRYPTOKEY_12_4      */
#define CRYPTOCFG_12_4__CRYPTOKEY_12_4__SHIFT                           0x00000000
#define CRYPTOCFG_12_4__CRYPTOKEY_12_4__WIDTH                           0x00000020
#define CRYPTOCFG_12_4__CRYPTOKEY_12_4__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_5 : CRYPTOKEY_12_5      */
#define CRYPTOCFG_12_5__CRYPTOKEY_12_5__SHIFT                           0x00000000
#define CRYPTOCFG_12_5__CRYPTOKEY_12_5__WIDTH                           0x00000020
#define CRYPTOCFG_12_5__CRYPTOKEY_12_5__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_6 : CRYPTOKEY_12_6      */
#define CRYPTOCFG_12_6__CRYPTOKEY_12_6__SHIFT                           0x00000000
#define CRYPTOCFG_12_6__CRYPTOKEY_12_6__WIDTH                           0x00000020
#define CRYPTOCFG_12_6__CRYPTOKEY_12_6__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_7 : CRYPTOKEY_12_7      */
#define CRYPTOCFG_12_7__CRYPTOKEY_12_7__SHIFT                           0x00000000
#define CRYPTOCFG_12_7__CRYPTOKEY_12_7__WIDTH                           0x00000020
#define CRYPTOCFG_12_7__CRYPTOKEY_12_7__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_8 : CRYPTOKEY_12_8      */
#define CRYPTOCFG_12_8__CRYPTOKEY_12_8__SHIFT                           0x00000000
#define CRYPTOCFG_12_8__CRYPTOKEY_12_8__WIDTH                           0x00000020
#define CRYPTOCFG_12_8__CRYPTOKEY_12_8__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_9 : CRYPTOKEY_12_9      */
#define CRYPTOCFG_12_9__CRYPTOKEY_12_9__SHIFT                           0x00000000
#define CRYPTOCFG_12_9__CRYPTOKEY_12_9__WIDTH                           0x00000020
#define CRYPTOCFG_12_9__CRYPTOKEY_12_9__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_10 : CRYPTOKEY_12_10     */
#define CRYPTOCFG_12_10__CRYPTOKEY_12_10__SHIFT                         0x00000000
#define CRYPTOCFG_12_10__CRYPTOKEY_12_10__WIDTH                         0x00000020
#define CRYPTOCFG_12_10__CRYPTOKEY_12_10__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_11 : CRYPTOKEY_12_11     */
#define CRYPTOCFG_12_11__CRYPTOKEY_12_11__SHIFT                         0x00000000
#define CRYPTOCFG_12_11__CRYPTOKEY_12_11__WIDTH                         0x00000020
#define CRYPTOCFG_12_11__CRYPTOKEY_12_11__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_12 : CRYPTOKEY_12_12     */
#define CRYPTOCFG_12_12__CRYPTOKEY_12_12__SHIFT                         0x00000000
#define CRYPTOCFG_12_12__CRYPTOKEY_12_12__WIDTH                         0x00000020
#define CRYPTOCFG_12_12__CRYPTOKEY_12_12__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_13 : CRYPTOKEY_12_13     */
#define CRYPTOCFG_12_13__CRYPTOKEY_12_13__SHIFT                         0x00000000
#define CRYPTOCFG_12_13__CRYPTOKEY_12_13__WIDTH                         0x00000020
#define CRYPTOCFG_12_13__CRYPTOKEY_12_13__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_14 : CRYPTOKEY_12_14     */
#define CRYPTOCFG_12_14__CRYPTOKEY_12_14__SHIFT                         0x00000000
#define CRYPTOCFG_12_14__CRYPTOKEY_12_14__WIDTH                         0x00000020
#define CRYPTOCFG_12_14__CRYPTOKEY_12_14__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_15 : CRYPTOKEY_12_15     */
#define CRYPTOCFG_12_15__CRYPTOKEY_12_15__SHIFT                         0x00000000
#define CRYPTOCFG_12_15__CRYPTOKEY_12_15__WIDTH                         0x00000020
#define CRYPTOCFG_12_15__CRYPTOKEY_12_15__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_16 : DUSIZE_12           */
#define CRYPTOCFG_12_16__DUSIZE_12__SHIFT                               0x00000000
#define CRYPTOCFG_12_16__DUSIZE_12__WIDTH                               0x00000008
#define CRYPTOCFG_12_16__DUSIZE_12__MASK                                0x000000FF

/*  CRYPTOCFG_12_16 : CAPIDX_12           */
#define CRYPTOCFG_12_16__CAPIDX_12__SHIFT                               0x00000008
#define CRYPTOCFG_12_16__CAPIDX_12__WIDTH                               0x00000008
#define CRYPTOCFG_12_16__CAPIDX_12__MASK                                0x0000FF00

/*  CRYPTOCFG_12_16 : CRYPTPCFG_RSVD_12   */
#define CRYPTOCFG_12_16__CRYPTPCFG_RSVD_12__SHIFT                       0x00000010
#define CRYPTOCFG_12_16__CRYPTPCFG_RSVD_12__WIDTH                       0x0000000F
#define CRYPTOCFG_12_16__CRYPTPCFG_RSVD_12__MASK                        0x7FFF0000

/*  CRYPTOCFG_12_16 : CFGE_12             */
#define CRYPTOCFG_12_16__CFGE_12__SHIFT                                 0x0000001F
#define CRYPTOCFG_12_16__CFGE_12__WIDTH                                 0x00000001
#define CRYPTOCFG_12_16__CFGE_12__MASK                                  0x80000000



/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_17 : RESERVED_12         */
#define CRYPTOCFG_12_17__RESERVED_12__SHIFT                             0x00000000
#define CRYPTOCFG_12_17__RESERVED_12__WIDTH                             0x00000020
#define CRYPTOCFG_12_17__RESERVED_12__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_18 : RESERVED_12         */
#define CRYPTOCFG_12_18__RESERVED_12__SHIFT                             0x00000000
#define CRYPTOCFG_12_18__RESERVED_12__WIDTH                             0x00000020
#define CRYPTOCFG_12_18__RESERVED_12__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_19 : RESERVED_12         */
#define CRYPTOCFG_12_19__RESERVED_12__SHIFT                             0x00000000
#define CRYPTOCFG_12_19__RESERVED_12__WIDTH                             0x00000020
#define CRYPTOCFG_12_19__RESERVED_12__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_20 : RESERVED_12         */
#define CRYPTOCFG_12_20__RESERVED_12__SHIFT                             0x00000000
#define CRYPTOCFG_12_20__RESERVED_12__WIDTH                             0x00000020
#define CRYPTOCFG_12_20__RESERVED_12__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_21 : RESERVED_12         */
#define CRYPTOCFG_12_21__RESERVED_12__SHIFT                             0x00000000
#define CRYPTOCFG_12_21__RESERVED_12__WIDTH                             0x00000020
#define CRYPTOCFG_12_21__RESERVED_12__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_22 : RESERVED_12         */
#define CRYPTOCFG_12_22__RESERVED_12__SHIFT                             0x00000000
#define CRYPTOCFG_12_22__RESERVED_12__WIDTH                             0x00000020
#define CRYPTOCFG_12_22__RESERVED_12__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_23 : RESERVED_12         */
#define CRYPTOCFG_12_23__RESERVED_12__SHIFT                             0x00000000
#define CRYPTOCFG_12_23__RESERVED_12__WIDTH                             0x00000020
#define CRYPTOCFG_12_23__RESERVED_12__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_24 : RESERVED_12         */
#define CRYPTOCFG_12_24__RESERVED_12__SHIFT                             0x00000000
#define CRYPTOCFG_12_24__RESERVED_12__WIDTH                             0x00000020
#define CRYPTOCFG_12_24__RESERVED_12__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_25 : RESERVED_12         */
#define CRYPTOCFG_12_25__RESERVED_12__SHIFT                             0x00000000
#define CRYPTOCFG_12_25__RESERVED_12__WIDTH                             0x00000020
#define CRYPTOCFG_12_25__RESERVED_12__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_26 : RESERVED_12         */
#define CRYPTOCFG_12_26__RESERVED_12__SHIFT                             0x00000000
#define CRYPTOCFG_12_26__RESERVED_12__WIDTH                             0x00000020
#define CRYPTOCFG_12_26__RESERVED_12__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_27 : RESERVED_12         */
#define CRYPTOCFG_12_27__RESERVED_12__SHIFT                             0x00000000
#define CRYPTOCFG_12_27__RESERVED_12__WIDTH                             0x00000020
#define CRYPTOCFG_12_27__RESERVED_12__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_28 : RESERVED_12         */
#define CRYPTOCFG_12_28__RESERVED_12__SHIFT                             0x00000000
#define CRYPTOCFG_12_28__RESERVED_12__WIDTH                             0x00000020
#define CRYPTOCFG_12_28__RESERVED_12__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_29 : RESERVED_12         */
#define CRYPTOCFG_12_29__RESERVED_12__SHIFT                             0x00000000
#define CRYPTOCFG_12_29__RESERVED_12__WIDTH                             0x00000020
#define CRYPTOCFG_12_29__RESERVED_12__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_30 : RESERVED_12         */
#define CRYPTOCFG_12_30__RESERVED_12__SHIFT                             0x00000000
#define CRYPTOCFG_12_30__RESERVED_12__WIDTH                             0x00000020
#define CRYPTOCFG_12_30__RESERVED_12__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_12_31 : RESERVED_12         */
#define CRYPTOCFG_12_31__RESERVED_12__SHIFT                             0x00000000
#define CRYPTOCFG_12_31__RESERVED_12__WIDTH                             0x00000020
#define CRYPTOCFG_12_31__RESERVED_12__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_0 : CRYPTOKEY_13_0      */
#define CRYPTOCFG_13_0__CRYPTOKEY_13_0__SHIFT                           0x00000000
#define CRYPTOCFG_13_0__CRYPTOKEY_13_0__WIDTH                           0x00000020
#define CRYPTOCFG_13_0__CRYPTOKEY_13_0__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_1 : CRYPTOKEY_13_1      */
#define CRYPTOCFG_13_1__CRYPTOKEY_13_1__SHIFT                           0x00000000
#define CRYPTOCFG_13_1__CRYPTOKEY_13_1__WIDTH                           0x00000020
#define CRYPTOCFG_13_1__CRYPTOKEY_13_1__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_2 : CRYPTOKEY_13_2      */
#define CRYPTOCFG_13_2__CRYPTOKEY_13_2__SHIFT                           0x00000000
#define CRYPTOCFG_13_2__CRYPTOKEY_13_2__WIDTH                           0x00000020
#define CRYPTOCFG_13_2__CRYPTOKEY_13_2__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_3 : CRYPTOKEY_13_3      */
#define CRYPTOCFG_13_3__CRYPTOKEY_13_3__SHIFT                           0x00000000
#define CRYPTOCFG_13_3__CRYPTOKEY_13_3__WIDTH                           0x00000020
#define CRYPTOCFG_13_3__CRYPTOKEY_13_3__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_4 : CRYPTOKEY_13_4      */
#define CRYPTOCFG_13_4__CRYPTOKEY_13_4__SHIFT                           0x00000000
#define CRYPTOCFG_13_4__CRYPTOKEY_13_4__WIDTH                           0x00000020
#define CRYPTOCFG_13_4__CRYPTOKEY_13_4__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_5 : CRYPTOKEY_13_5      */
#define CRYPTOCFG_13_5__CRYPTOKEY_13_5__SHIFT                           0x00000000
#define CRYPTOCFG_13_5__CRYPTOKEY_13_5__WIDTH                           0x00000020
#define CRYPTOCFG_13_5__CRYPTOKEY_13_5__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_6 : CRYPTOKEY_13_6      */
#define CRYPTOCFG_13_6__CRYPTOKEY_13_6__SHIFT                           0x00000000
#define CRYPTOCFG_13_6__CRYPTOKEY_13_6__WIDTH                           0x00000020
#define CRYPTOCFG_13_6__CRYPTOKEY_13_6__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_7 : CRYPTOKEY_13_7      */
#define CRYPTOCFG_13_7__CRYPTOKEY_13_7__SHIFT                           0x00000000
#define CRYPTOCFG_13_7__CRYPTOKEY_13_7__WIDTH                           0x00000020
#define CRYPTOCFG_13_7__CRYPTOKEY_13_7__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_8 : CRYPTOKEY_13_8      */
#define CRYPTOCFG_13_8__CRYPTOKEY_13_8__SHIFT                           0x00000000
#define CRYPTOCFG_13_8__CRYPTOKEY_13_8__WIDTH                           0x00000020
#define CRYPTOCFG_13_8__CRYPTOKEY_13_8__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_9 : CRYPTOKEY_13_9      */
#define CRYPTOCFG_13_9__CRYPTOKEY_13_9__SHIFT                           0x00000000
#define CRYPTOCFG_13_9__CRYPTOKEY_13_9__WIDTH                           0x00000020
#define CRYPTOCFG_13_9__CRYPTOKEY_13_9__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_10 : CRYPTOKEY_13_10     */
#define CRYPTOCFG_13_10__CRYPTOKEY_13_10__SHIFT                         0x00000000
#define CRYPTOCFG_13_10__CRYPTOKEY_13_10__WIDTH                         0x00000020
#define CRYPTOCFG_13_10__CRYPTOKEY_13_10__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_11 : CRYPTOKEY_13_11     */
#define CRYPTOCFG_13_11__CRYPTOKEY_13_11__SHIFT                         0x00000000
#define CRYPTOCFG_13_11__CRYPTOKEY_13_11__WIDTH                         0x00000020
#define CRYPTOCFG_13_11__CRYPTOKEY_13_11__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_12 : CRYPTOKEY_13_12     */
#define CRYPTOCFG_13_12__CRYPTOKEY_13_12__SHIFT                         0x00000000
#define CRYPTOCFG_13_12__CRYPTOKEY_13_12__WIDTH                         0x00000020
#define CRYPTOCFG_13_12__CRYPTOKEY_13_12__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_13 : CRYPTOKEY_13_13     */
#define CRYPTOCFG_13_13__CRYPTOKEY_13_13__SHIFT                         0x00000000
#define CRYPTOCFG_13_13__CRYPTOKEY_13_13__WIDTH                         0x00000020
#define CRYPTOCFG_13_13__CRYPTOKEY_13_13__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_14 : CRYPTOKEY_13_14     */
#define CRYPTOCFG_13_14__CRYPTOKEY_13_14__SHIFT                         0x00000000
#define CRYPTOCFG_13_14__CRYPTOKEY_13_14__WIDTH                         0x00000020
#define CRYPTOCFG_13_14__CRYPTOKEY_13_14__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_15 : CRYPTOKEY_13_15     */
#define CRYPTOCFG_13_15__CRYPTOKEY_13_15__SHIFT                         0x00000000
#define CRYPTOCFG_13_15__CRYPTOKEY_13_15__WIDTH                         0x00000020
#define CRYPTOCFG_13_15__CRYPTOKEY_13_15__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_16 : DUSIZE_13           */
#define CRYPTOCFG_13_16__DUSIZE_13__SHIFT                               0x00000000
#define CRYPTOCFG_13_16__DUSIZE_13__WIDTH                               0x00000008
#define CRYPTOCFG_13_16__DUSIZE_13__MASK                                0x000000FF

/*  CRYPTOCFG_13_16 : CAPIDX_13           */
#define CRYPTOCFG_13_16__CAPIDX_13__SHIFT                               0x00000008
#define CRYPTOCFG_13_16__CAPIDX_13__WIDTH                               0x00000008
#define CRYPTOCFG_13_16__CAPIDX_13__MASK                                0x0000FF00

/*  CRYPTOCFG_13_16 : CRYPTPCFG_RSVD_13   */
#define CRYPTOCFG_13_16__CRYPTPCFG_RSVD_13__SHIFT                       0x00000010
#define CRYPTOCFG_13_16__CRYPTPCFG_RSVD_13__WIDTH                       0x0000000F
#define CRYPTOCFG_13_16__CRYPTPCFG_RSVD_13__MASK                        0x7FFF0000

/*  CRYPTOCFG_13_16 : CFGE_13             */
#define CRYPTOCFG_13_16__CFGE_13__SHIFT                                 0x0000001F
#define CRYPTOCFG_13_16__CFGE_13__WIDTH                                 0x00000001
#define CRYPTOCFG_13_16__CFGE_13__MASK                                  0x80000000



/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_17 : RESERVED_13         */
#define CRYPTOCFG_13_17__RESERVED_13__SHIFT                             0x00000000
#define CRYPTOCFG_13_17__RESERVED_13__WIDTH                             0x00000020
#define CRYPTOCFG_13_17__RESERVED_13__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_18 : RESERVED_13         */
#define CRYPTOCFG_13_18__RESERVED_13__SHIFT                             0x00000000
#define CRYPTOCFG_13_18__RESERVED_13__WIDTH                             0x00000020
#define CRYPTOCFG_13_18__RESERVED_13__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_19 : RESERVED_13         */
#define CRYPTOCFG_13_19__RESERVED_13__SHIFT                             0x00000000
#define CRYPTOCFG_13_19__RESERVED_13__WIDTH                             0x00000020
#define CRYPTOCFG_13_19__RESERVED_13__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_20 : RESERVED_13         */
#define CRYPTOCFG_13_20__RESERVED_13__SHIFT                             0x00000000
#define CRYPTOCFG_13_20__RESERVED_13__WIDTH                             0x00000020
#define CRYPTOCFG_13_20__RESERVED_13__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_21 : RESERVED_13         */
#define CRYPTOCFG_13_21__RESERVED_13__SHIFT                             0x00000000
#define CRYPTOCFG_13_21__RESERVED_13__WIDTH                             0x00000020
#define CRYPTOCFG_13_21__RESERVED_13__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_22 : RESERVED_13         */
#define CRYPTOCFG_13_22__RESERVED_13__SHIFT                             0x00000000
#define CRYPTOCFG_13_22__RESERVED_13__WIDTH                             0x00000020
#define CRYPTOCFG_13_22__RESERVED_13__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_23 : RESERVED_13         */
#define CRYPTOCFG_13_23__RESERVED_13__SHIFT                             0x00000000
#define CRYPTOCFG_13_23__RESERVED_13__WIDTH                             0x00000020
#define CRYPTOCFG_13_23__RESERVED_13__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_24 : RESERVED_13         */
#define CRYPTOCFG_13_24__RESERVED_13__SHIFT                             0x00000000
#define CRYPTOCFG_13_24__RESERVED_13__WIDTH                             0x00000020
#define CRYPTOCFG_13_24__RESERVED_13__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_25 : RESERVED_13         */
#define CRYPTOCFG_13_25__RESERVED_13__SHIFT                             0x00000000
#define CRYPTOCFG_13_25__RESERVED_13__WIDTH                             0x00000020
#define CRYPTOCFG_13_25__RESERVED_13__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_26 : RESERVED_13         */
#define CRYPTOCFG_13_26__RESERVED_13__SHIFT                             0x00000000
#define CRYPTOCFG_13_26__RESERVED_13__WIDTH                             0x00000020
#define CRYPTOCFG_13_26__RESERVED_13__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_27 : RESERVED_13         */
#define CRYPTOCFG_13_27__RESERVED_13__SHIFT                             0x00000000
#define CRYPTOCFG_13_27__RESERVED_13__WIDTH                             0x00000020
#define CRYPTOCFG_13_27__RESERVED_13__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_28 : RESERVED_13         */
#define CRYPTOCFG_13_28__RESERVED_13__SHIFT                             0x00000000
#define CRYPTOCFG_13_28__RESERVED_13__WIDTH                             0x00000020
#define CRYPTOCFG_13_28__RESERVED_13__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_29 : RESERVED_13         */
#define CRYPTOCFG_13_29__RESERVED_13__SHIFT                             0x00000000
#define CRYPTOCFG_13_29__RESERVED_13__WIDTH                             0x00000020
#define CRYPTOCFG_13_29__RESERVED_13__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_30 : RESERVED_13         */
#define CRYPTOCFG_13_30__RESERVED_13__SHIFT                             0x00000000
#define CRYPTOCFG_13_30__RESERVED_13__WIDTH                             0x00000020
#define CRYPTOCFG_13_30__RESERVED_13__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_13_31 : RESERVED_13         */
#define CRYPTOCFG_13_31__RESERVED_13__SHIFT                             0x00000000
#define CRYPTOCFG_13_31__RESERVED_13__WIDTH                             0x00000020
#define CRYPTOCFG_13_31__RESERVED_13__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_0 : CRYPTOKEY_14_0      */
#define CRYPTOCFG_14_0__CRYPTOKEY_14_0__SHIFT                           0x00000000
#define CRYPTOCFG_14_0__CRYPTOKEY_14_0__WIDTH                           0x00000020
#define CRYPTOCFG_14_0__CRYPTOKEY_14_0__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_1 : CRYPTOKEY_14_1      */
#define CRYPTOCFG_14_1__CRYPTOKEY_14_1__SHIFT                           0x00000000
#define CRYPTOCFG_14_1__CRYPTOKEY_14_1__WIDTH                           0x00000020
#define CRYPTOCFG_14_1__CRYPTOKEY_14_1__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_2 : CRYPTOKEY_14_2      */
#define CRYPTOCFG_14_2__CRYPTOKEY_14_2__SHIFT                           0x00000000
#define CRYPTOCFG_14_2__CRYPTOKEY_14_2__WIDTH                           0x00000020
#define CRYPTOCFG_14_2__CRYPTOKEY_14_2__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_3 : CRYPTOKEY_14_3      */
#define CRYPTOCFG_14_3__CRYPTOKEY_14_3__SHIFT                           0x00000000
#define CRYPTOCFG_14_3__CRYPTOKEY_14_3__WIDTH                           0x00000020
#define CRYPTOCFG_14_3__CRYPTOKEY_14_3__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_4 : CRYPTOKEY_14_4      */
#define CRYPTOCFG_14_4__CRYPTOKEY_14_4__SHIFT                           0x00000000
#define CRYPTOCFG_14_4__CRYPTOKEY_14_4__WIDTH                           0x00000020
#define CRYPTOCFG_14_4__CRYPTOKEY_14_4__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_5 : CRYPTOKEY_14_5      */
#define CRYPTOCFG_14_5__CRYPTOKEY_14_5__SHIFT                           0x00000000
#define CRYPTOCFG_14_5__CRYPTOKEY_14_5__WIDTH                           0x00000020
#define CRYPTOCFG_14_5__CRYPTOKEY_14_5__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_6 : CRYPTOKEY_14_6      */
#define CRYPTOCFG_14_6__CRYPTOKEY_14_6__SHIFT                           0x00000000
#define CRYPTOCFG_14_6__CRYPTOKEY_14_6__WIDTH                           0x00000020
#define CRYPTOCFG_14_6__CRYPTOKEY_14_6__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_7 : CRYPTOKEY_14_7      */
#define CRYPTOCFG_14_7__CRYPTOKEY_14_7__SHIFT                           0x00000000
#define CRYPTOCFG_14_7__CRYPTOKEY_14_7__WIDTH                           0x00000020
#define CRYPTOCFG_14_7__CRYPTOKEY_14_7__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_8 : CRYPTOKEY_14_8      */
#define CRYPTOCFG_14_8__CRYPTOKEY_14_8__SHIFT                           0x00000000
#define CRYPTOCFG_14_8__CRYPTOKEY_14_8__WIDTH                           0x00000020
#define CRYPTOCFG_14_8__CRYPTOKEY_14_8__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_9 : CRYPTOKEY_14_9      */
#define CRYPTOCFG_14_9__CRYPTOKEY_14_9__SHIFT                           0x00000000
#define CRYPTOCFG_14_9__CRYPTOKEY_14_9__WIDTH                           0x00000020
#define CRYPTOCFG_14_9__CRYPTOKEY_14_9__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_10 : CRYPTOKEY_14_10     */
#define CRYPTOCFG_14_10__CRYPTOKEY_14_10__SHIFT                         0x00000000
#define CRYPTOCFG_14_10__CRYPTOKEY_14_10__WIDTH                         0x00000020
#define CRYPTOCFG_14_10__CRYPTOKEY_14_10__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_11 : CRYPTOKEY_14_11     */
#define CRYPTOCFG_14_11__CRYPTOKEY_14_11__SHIFT                         0x00000000
#define CRYPTOCFG_14_11__CRYPTOKEY_14_11__WIDTH                         0x00000020
#define CRYPTOCFG_14_11__CRYPTOKEY_14_11__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_12 : CRYPTOKEY_14_12     */
#define CRYPTOCFG_14_12__CRYPTOKEY_14_12__SHIFT                         0x00000000
#define CRYPTOCFG_14_12__CRYPTOKEY_14_12__WIDTH                         0x00000020
#define CRYPTOCFG_14_12__CRYPTOKEY_14_12__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_13 : CRYPTOKEY_14_13     */
#define CRYPTOCFG_14_13__CRYPTOKEY_14_13__SHIFT                         0x00000000
#define CRYPTOCFG_14_13__CRYPTOKEY_14_13__WIDTH                         0x00000020
#define CRYPTOCFG_14_13__CRYPTOKEY_14_13__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_14 : CRYPTOKEY_14_14     */
#define CRYPTOCFG_14_14__CRYPTOKEY_14_14__SHIFT                         0x00000000
#define CRYPTOCFG_14_14__CRYPTOKEY_14_14__WIDTH                         0x00000020
#define CRYPTOCFG_14_14__CRYPTOKEY_14_14__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_15 : CRYPTOKEY_14_15     */
#define CRYPTOCFG_14_15__CRYPTOKEY_14_15__SHIFT                         0x00000000
#define CRYPTOCFG_14_15__CRYPTOKEY_14_15__WIDTH                         0x00000020
#define CRYPTOCFG_14_15__CRYPTOKEY_14_15__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_16 : DUSIZE_14           */
#define CRYPTOCFG_14_16__DUSIZE_14__SHIFT                               0x00000000
#define CRYPTOCFG_14_16__DUSIZE_14__WIDTH                               0x00000008
#define CRYPTOCFG_14_16__DUSIZE_14__MASK                                0x000000FF

/*  CRYPTOCFG_14_16 : CAPIDX_14           */
#define CRYPTOCFG_14_16__CAPIDX_14__SHIFT                               0x00000008
#define CRYPTOCFG_14_16__CAPIDX_14__WIDTH                               0x00000008
#define CRYPTOCFG_14_16__CAPIDX_14__MASK                                0x0000FF00

/*  CRYPTOCFG_14_16 : CRYPTPCFG_RSVD_14   */
#define CRYPTOCFG_14_16__CRYPTPCFG_RSVD_14__SHIFT                       0x00000010
#define CRYPTOCFG_14_16__CRYPTPCFG_RSVD_14__WIDTH                       0x0000000F
#define CRYPTOCFG_14_16__CRYPTPCFG_RSVD_14__MASK                        0x7FFF0000

/*  CRYPTOCFG_14_16 : CFGE_14             */
#define CRYPTOCFG_14_16__CFGE_14__SHIFT                                 0x0000001F
#define CRYPTOCFG_14_16__CFGE_14__WIDTH                                 0x00000001
#define CRYPTOCFG_14_16__CFGE_14__MASK                                  0x80000000



/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_17 : RESERVED_14         */
#define CRYPTOCFG_14_17__RESERVED_14__SHIFT                             0x00000000
#define CRYPTOCFG_14_17__RESERVED_14__WIDTH                             0x00000020
#define CRYPTOCFG_14_17__RESERVED_14__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_18 : RESERVED_14         */
#define CRYPTOCFG_14_18__RESERVED_14__SHIFT                             0x00000000
#define CRYPTOCFG_14_18__RESERVED_14__WIDTH                             0x00000020
#define CRYPTOCFG_14_18__RESERVED_14__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_19 : RESERVED_14         */
#define CRYPTOCFG_14_19__RESERVED_14__SHIFT                             0x00000000
#define CRYPTOCFG_14_19__RESERVED_14__WIDTH                             0x00000020
#define CRYPTOCFG_14_19__RESERVED_14__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_20 : RESERVED_14         */
#define CRYPTOCFG_14_20__RESERVED_14__SHIFT                             0x00000000
#define CRYPTOCFG_14_20__RESERVED_14__WIDTH                             0x00000020
#define CRYPTOCFG_14_20__RESERVED_14__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_21 : RESERVED_14         */
#define CRYPTOCFG_14_21__RESERVED_14__SHIFT                             0x00000000
#define CRYPTOCFG_14_21__RESERVED_14__WIDTH                             0x00000020
#define CRYPTOCFG_14_21__RESERVED_14__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_22 : RESERVED_14         */
#define CRYPTOCFG_14_22__RESERVED_14__SHIFT                             0x00000000
#define CRYPTOCFG_14_22__RESERVED_14__WIDTH                             0x00000020
#define CRYPTOCFG_14_22__RESERVED_14__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_23 : RESERVED_14         */
#define CRYPTOCFG_14_23__RESERVED_14__SHIFT                             0x00000000
#define CRYPTOCFG_14_23__RESERVED_14__WIDTH                             0x00000020
#define CRYPTOCFG_14_23__RESERVED_14__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_24 : RESERVED_14         */
#define CRYPTOCFG_14_24__RESERVED_14__SHIFT                             0x00000000
#define CRYPTOCFG_14_24__RESERVED_14__WIDTH                             0x00000020
#define CRYPTOCFG_14_24__RESERVED_14__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_25 : RESERVED_14         */
#define CRYPTOCFG_14_25__RESERVED_14__SHIFT                             0x00000000
#define CRYPTOCFG_14_25__RESERVED_14__WIDTH                             0x00000020
#define CRYPTOCFG_14_25__RESERVED_14__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_26 : RESERVED_14         */
#define CRYPTOCFG_14_26__RESERVED_14__SHIFT                             0x00000000
#define CRYPTOCFG_14_26__RESERVED_14__WIDTH                             0x00000020
#define CRYPTOCFG_14_26__RESERVED_14__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_27 : RESERVED_14         */
#define CRYPTOCFG_14_27__RESERVED_14__SHIFT                             0x00000000
#define CRYPTOCFG_14_27__RESERVED_14__WIDTH                             0x00000020
#define CRYPTOCFG_14_27__RESERVED_14__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_28 : RESERVED_14         */
#define CRYPTOCFG_14_28__RESERVED_14__SHIFT                             0x00000000
#define CRYPTOCFG_14_28__RESERVED_14__WIDTH                             0x00000020
#define CRYPTOCFG_14_28__RESERVED_14__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_29 : RESERVED_14         */
#define CRYPTOCFG_14_29__RESERVED_14__SHIFT                             0x00000000
#define CRYPTOCFG_14_29__RESERVED_14__WIDTH                             0x00000020
#define CRYPTOCFG_14_29__RESERVED_14__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_30 : RESERVED_14         */
#define CRYPTOCFG_14_30__RESERVED_14__SHIFT                             0x00000000
#define CRYPTOCFG_14_30__RESERVED_14__WIDTH                             0x00000020
#define CRYPTOCFG_14_30__RESERVED_14__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_14_31 : RESERVED_14         */
#define CRYPTOCFG_14_31__RESERVED_14__SHIFT                             0x00000000
#define CRYPTOCFG_14_31__RESERVED_14__WIDTH                             0x00000020
#define CRYPTOCFG_14_31__RESERVED_14__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_0 : CRYPTOKEY_15_0      */
#define CRYPTOCFG_15_0__CRYPTOKEY_15_0__SHIFT                           0x00000000
#define CRYPTOCFG_15_0__CRYPTOKEY_15_0__WIDTH                           0x00000020
#define CRYPTOCFG_15_0__CRYPTOKEY_15_0__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_1 : CRYPTOKEY_15_1      */
#define CRYPTOCFG_15_1__CRYPTOKEY_15_1__SHIFT                           0x00000000
#define CRYPTOCFG_15_1__CRYPTOKEY_15_1__WIDTH                           0x00000020
#define CRYPTOCFG_15_1__CRYPTOKEY_15_1__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_2 : CRYPTOKEY_15_2      */
#define CRYPTOCFG_15_2__CRYPTOKEY_15_2__SHIFT                           0x00000000
#define CRYPTOCFG_15_2__CRYPTOKEY_15_2__WIDTH                           0x00000020
#define CRYPTOCFG_15_2__CRYPTOKEY_15_2__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_3 : CRYPTOKEY_15_3      */
#define CRYPTOCFG_15_3__CRYPTOKEY_15_3__SHIFT                           0x00000000
#define CRYPTOCFG_15_3__CRYPTOKEY_15_3__WIDTH                           0x00000020
#define CRYPTOCFG_15_3__CRYPTOKEY_15_3__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_4 : CRYPTOKEY_15_4      */
#define CRYPTOCFG_15_4__CRYPTOKEY_15_4__SHIFT                           0x00000000
#define CRYPTOCFG_15_4__CRYPTOKEY_15_4__WIDTH                           0x00000020
#define CRYPTOCFG_15_4__CRYPTOKEY_15_4__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_5 : CRYPTOKEY_15_5      */
#define CRYPTOCFG_15_5__CRYPTOKEY_15_5__SHIFT                           0x00000000
#define CRYPTOCFG_15_5__CRYPTOKEY_15_5__WIDTH                           0x00000020
#define CRYPTOCFG_15_5__CRYPTOKEY_15_5__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_6 : CRYPTOKEY_15_6      */
#define CRYPTOCFG_15_6__CRYPTOKEY_15_6__SHIFT                           0x00000000
#define CRYPTOCFG_15_6__CRYPTOKEY_15_6__WIDTH                           0x00000020
#define CRYPTOCFG_15_6__CRYPTOKEY_15_6__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_7 : CRYPTOKEY_15_7      */
#define CRYPTOCFG_15_7__CRYPTOKEY_15_7__SHIFT                           0x00000000
#define CRYPTOCFG_15_7__CRYPTOKEY_15_7__WIDTH                           0x00000020
#define CRYPTOCFG_15_7__CRYPTOKEY_15_7__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_8 : CRYPTOKEY_15_8      */
#define CRYPTOCFG_15_8__CRYPTOKEY_15_8__SHIFT                           0x00000000
#define CRYPTOCFG_15_8__CRYPTOKEY_15_8__WIDTH                           0x00000020
#define CRYPTOCFG_15_8__CRYPTOKEY_15_8__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_9 : CRYPTOKEY_15_9      */
#define CRYPTOCFG_15_9__CRYPTOKEY_15_9__SHIFT                           0x00000000
#define CRYPTOCFG_15_9__CRYPTOKEY_15_9__WIDTH                           0x00000020
#define CRYPTOCFG_15_9__CRYPTOKEY_15_9__MASK                            0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_10 : CRYPTOKEY_15_10     */
#define CRYPTOCFG_15_10__CRYPTOKEY_15_10__SHIFT                         0x00000000
#define CRYPTOCFG_15_10__CRYPTOKEY_15_10__WIDTH                         0x00000020
#define CRYPTOCFG_15_10__CRYPTOKEY_15_10__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_11 : CRYPTOKEY_15_11     */
#define CRYPTOCFG_15_11__CRYPTOKEY_15_11__SHIFT                         0x00000000
#define CRYPTOCFG_15_11__CRYPTOKEY_15_11__WIDTH                         0x00000020
#define CRYPTOCFG_15_11__CRYPTOKEY_15_11__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_12 : CRYPTOKEY_15_12     */
#define CRYPTOCFG_15_12__CRYPTOKEY_15_12__SHIFT                         0x00000000
#define CRYPTOCFG_15_12__CRYPTOKEY_15_12__WIDTH                         0x00000020
#define CRYPTOCFG_15_12__CRYPTOKEY_15_12__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_13 : CRYPTOKEY_15_13     */
#define CRYPTOCFG_15_13__CRYPTOKEY_15_13__SHIFT                         0x00000000
#define CRYPTOCFG_15_13__CRYPTOKEY_15_13__WIDTH                         0x00000020
#define CRYPTOCFG_15_13__CRYPTOKEY_15_13__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_14 : CRYPTOKEY_15_14     */
#define CRYPTOCFG_15_14__CRYPTOKEY_15_14__SHIFT                         0x00000000
#define CRYPTOCFG_15_14__CRYPTOKEY_15_14__WIDTH                         0x00000020
#define CRYPTOCFG_15_14__CRYPTOKEY_15_14__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_15 : CRYPTOKEY_15_15     */
#define CRYPTOCFG_15_15__CRYPTOKEY_15_15__SHIFT                         0x00000000
#define CRYPTOCFG_15_15__CRYPTOKEY_15_15__WIDTH                         0x00000020
#define CRYPTOCFG_15_15__CRYPTOKEY_15_15__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_16 : DUSIZE_15           */
#define CRYPTOCFG_15_16__DUSIZE_15__SHIFT                               0x00000000
#define CRYPTOCFG_15_16__DUSIZE_15__WIDTH                               0x00000008
#define CRYPTOCFG_15_16__DUSIZE_15__MASK                                0x000000FF

/*  CRYPTOCFG_15_16 : CAPIDX_15           */
#define CRYPTOCFG_15_16__CAPIDX_15__SHIFT                               0x00000008
#define CRYPTOCFG_15_16__CAPIDX_15__WIDTH                               0x00000008
#define CRYPTOCFG_15_16__CAPIDX_15__MASK                                0x0000FF00

/*  CRYPTOCFG_15_16 : CRYPTPCFG_RSVD_15   */
#define CRYPTOCFG_15_16__CRYPTPCFG_RSVD_15__SHIFT                       0x00000010
#define CRYPTOCFG_15_16__CRYPTPCFG_RSVD_15__WIDTH                       0x0000000F
#define CRYPTOCFG_15_16__CRYPTPCFG_RSVD_15__MASK                        0x7FFF0000

/*  CRYPTOCFG_15_16 : CFGE_15             */
#define CRYPTOCFG_15_16__CFGE_15__SHIFT                                 0x0000001F
#define CRYPTOCFG_15_16__CFGE_15__WIDTH                                 0x00000001
#define CRYPTOCFG_15_16__CFGE_15__MASK                                  0x80000000



/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_17 : RESERVED_15         */
#define CRYPTOCFG_15_17__RESERVED_15__SHIFT                             0x00000000
#define CRYPTOCFG_15_17__RESERVED_15__WIDTH                             0x00000020
#define CRYPTOCFG_15_17__RESERVED_15__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_18 : RESERVED_15         */
#define CRYPTOCFG_15_18__RESERVED_15__SHIFT                             0x00000000
#define CRYPTOCFG_15_18__RESERVED_15__WIDTH                             0x00000020
#define CRYPTOCFG_15_18__RESERVED_15__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_19 : RESERVED_15         */
#define CRYPTOCFG_15_19__RESERVED_15__SHIFT                             0x00000000
#define CRYPTOCFG_15_19__RESERVED_15__WIDTH                             0x00000020
#define CRYPTOCFG_15_19__RESERVED_15__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_20 : RESERVED_15         */
#define CRYPTOCFG_15_20__RESERVED_15__SHIFT                             0x00000000
#define CRYPTOCFG_15_20__RESERVED_15__WIDTH                             0x00000020
#define CRYPTOCFG_15_20__RESERVED_15__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_21 : RESERVED_15         */
#define CRYPTOCFG_15_21__RESERVED_15__SHIFT                             0x00000000
#define CRYPTOCFG_15_21__RESERVED_15__WIDTH                             0x00000020
#define CRYPTOCFG_15_21__RESERVED_15__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_22 : RESERVED_15         */
#define CRYPTOCFG_15_22__RESERVED_15__SHIFT                             0x00000000
#define CRYPTOCFG_15_22__RESERVED_15__WIDTH                             0x00000020
#define CRYPTOCFG_15_22__RESERVED_15__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_23 : RESERVED_15         */
#define CRYPTOCFG_15_23__RESERVED_15__SHIFT                             0x00000000
#define CRYPTOCFG_15_23__RESERVED_15__WIDTH                             0x00000020
#define CRYPTOCFG_15_23__RESERVED_15__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_24 : RESERVED_15         */
#define CRYPTOCFG_15_24__RESERVED_15__SHIFT                             0x00000000
#define CRYPTOCFG_15_24__RESERVED_15__WIDTH                             0x00000020
#define CRYPTOCFG_15_24__RESERVED_15__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_25 : RESERVED_15         */
#define CRYPTOCFG_15_25__RESERVED_15__SHIFT                             0x00000000
#define CRYPTOCFG_15_25__RESERVED_15__WIDTH                             0x00000020
#define CRYPTOCFG_15_25__RESERVED_15__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_26 : RESERVED_15         */
#define CRYPTOCFG_15_26__RESERVED_15__SHIFT                             0x00000000
#define CRYPTOCFG_15_26__RESERVED_15__WIDTH                             0x00000020
#define CRYPTOCFG_15_26__RESERVED_15__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_27 : RESERVED_15         */
#define CRYPTOCFG_15_27__RESERVED_15__SHIFT                             0x00000000
#define CRYPTOCFG_15_27__RESERVED_15__WIDTH                             0x00000020
#define CRYPTOCFG_15_27__RESERVED_15__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_28 : RESERVED_15         */
#define CRYPTOCFG_15_28__RESERVED_15__SHIFT                             0x00000000
#define CRYPTOCFG_15_28__RESERVED_15__WIDTH                             0x00000020
#define CRYPTOCFG_15_28__RESERVED_15__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_29 : RESERVED_15         */
#define CRYPTOCFG_15_29__RESERVED_15__SHIFT                             0x00000000
#define CRYPTOCFG_15_29__RESERVED_15__WIDTH                             0x00000020
#define CRYPTOCFG_15_29__RESERVED_15__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_30 : RESERVED_15         */
#define CRYPTOCFG_15_30__RESERVED_15__SHIFT                             0x00000000
#define CRYPTOCFG_15_30__RESERVED_15__WIDTH                             0x00000020
#define CRYPTOCFG_15_30__RESERVED_15__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  CRYPTOCFG_15_31 : RESERVED_15         */
#define CRYPTOCFG_15_31__RESERVED_15__SHIFT                             0x00000000
#define CRYPTOCFG_15_31__RESERVED_15__WIDTH                             0x00000020
#define CRYPTOCFG_15_31__RESERVED_15__MASK                              0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  PHY_CNFG : PHY_RSTN            */
#define PHY_CNFG__PHY_RSTN__SHIFT                                       0x00000000
#define PHY_CNFG__PHY_RSTN__WIDTH                                       0x00000001
#define PHY_CNFG__PHY_RSTN__MASK                                        0x00000001

/*  PHY_CNFG : PHY_PWRGOOD         */
#define PHY_CNFG__PHY_PWRGOOD__SHIFT                                    0x00000001
#define PHY_CNFG__PHY_PWRGOOD__WIDTH                                    0x00000001
#define PHY_CNFG__PHY_PWRGOOD__MASK                                     0x00000002

/*  PHY_CNFG : PAD_SP              */
#define PHY_CNFG__PAD_SP__SHIFT                                         0x00000010
#define PHY_CNFG__PAD_SP__WIDTH                                         0x00000004
#define PHY_CNFG__PAD_SP__MASK                                          0x000F0000

/*  PHY_CNFG : PAD_SN              */
#define PHY_CNFG__PAD_SN__SHIFT                                         0x00000014
#define PHY_CNFG__PAD_SN__WIDTH                                         0x00000004
#define PHY_CNFG__PAD_SN__MASK                                          0x00F00000


/*-------------------------------------------------------------------------------------*/
/*  CMDPAD_CNFG : RXSEL               */
#define CMDPAD_CNFG__RXSEL__SHIFT                                       0x00000000
#define CMDPAD_CNFG__RXSEL__WIDTH                                       0x00000003
#define CMDPAD_CNFG__RXSEL__MASK                                        0x00000007

/*  CMDPAD_CNFG : WEAKPULL_EN         */
#define CMDPAD_CNFG__WEAKPULL_EN__SHIFT                                 0x00000003
#define CMDPAD_CNFG__WEAKPULL_EN__WIDTH                                 0x00000002
#define CMDPAD_CNFG__WEAKPULL_EN__MASK                                  0x00000018


/*  CMDPAD_CNFG : TXSLEW_CTRL_P       */
#define CMDPAD_CNFG__TXSLEW_CTRL_P__SHIFT                               0x00000005
#define CMDPAD_CNFG__TXSLEW_CTRL_P__WIDTH                               0x00000004
#define CMDPAD_CNFG__TXSLEW_CTRL_P__MASK                                0x000001E0

/*  CMDPAD_CNFG : TXSLEW_CTRL_N       */
#define CMDPAD_CNFG__TXSLEW_CTRL_N__SHIFT                               0x00000009
#define CMDPAD_CNFG__TXSLEW_CTRL_N__WIDTH                               0x00000004
#define CMDPAD_CNFG__TXSLEW_CTRL_N__MASK                                0x00001E00


/*-------------------------------------------------------------------------------------*/
/*  DATPAD_CNFG : RXSEL               */
#define DATPAD_CNFG__RXSEL__SHIFT                                       0x00000000
#define DATPAD_CNFG__RXSEL__WIDTH                                       0x00000003
#define DATPAD_CNFG__RXSEL__MASK                                        0x00000007

/*  DATPAD_CNFG : WEAKPULL_EN         */
#define DATPAD_CNFG__WEAKPULL_EN__SHIFT                                 0x00000003
#define DATPAD_CNFG__WEAKPULL_EN__WIDTH                                 0x00000002
#define DATPAD_CNFG__WEAKPULL_EN__MASK                                  0x00000018


/*  DATPAD_CNFG : TXSLEW_CTRL_P       */
#define DATPAD_CNFG__TXSLEW_CTRL_P__SHIFT                               0x00000005
#define DATPAD_CNFG__TXSLEW_CTRL_P__WIDTH                               0x00000004
#define DATPAD_CNFG__TXSLEW_CTRL_P__MASK                                0x000001E0

/*  DATPAD_CNFG : TXSLEW_CTRL_N       */
#define DATPAD_CNFG__TXSLEW_CTRL_N__SHIFT                               0x00000009
#define DATPAD_CNFG__TXSLEW_CTRL_N__WIDTH                               0x00000004
#define DATPAD_CNFG__TXSLEW_CTRL_N__MASK                                0x00001E00


/*-------------------------------------------------------------------------------------*/
/*  CLKPAD_CNFG : RXSEL               */
#define CLKPAD_CNFG__RXSEL__SHIFT                                       0x00000000
#define CLKPAD_CNFG__RXSEL__WIDTH                                       0x00000003
#define CLKPAD_CNFG__RXSEL__MASK                                        0x00000007

/*  CLKPAD_CNFG : WEAKPULL_EN         */
#define CLKPAD_CNFG__WEAKPULL_EN__SHIFT                                 0x00000003
#define CLKPAD_CNFG__WEAKPULL_EN__WIDTH                                 0x00000002
#define CLKPAD_CNFG__WEAKPULL_EN__MASK                                  0x00000018


/*  CLKPAD_CNFG : TXSLEW_CTRL_P       */
#define CLKPAD_CNFG__TXSLEW_CTRL_P__SHIFT                               0x00000005
#define CLKPAD_CNFG__TXSLEW_CTRL_P__WIDTH                               0x00000004
#define CLKPAD_CNFG__TXSLEW_CTRL_P__MASK                                0x000001E0

/*  CLKPAD_CNFG : TXSLEW_CTRL_N       */
#define CLKPAD_CNFG__TXSLEW_CTRL_N__SHIFT                               0x00000009
#define CLKPAD_CNFG__TXSLEW_CTRL_N__WIDTH                               0x00000004
#define CLKPAD_CNFG__TXSLEW_CTRL_N__MASK                                0x00001E00


/*-------------------------------------------------------------------------------------*/
/*  STBPAD_CNFG : RXSEL               */
#define STBPAD_CNFG__RXSEL__SHIFT                                       0x00000000
#define STBPAD_CNFG__RXSEL__WIDTH                                       0x00000003
#define STBPAD_CNFG__RXSEL__MASK                                        0x00000007

/*  STBPAD_CNFG : WEAKPULL_EN         */
#define STBPAD_CNFG__WEAKPULL_EN__SHIFT                                 0x00000003
#define STBPAD_CNFG__WEAKPULL_EN__WIDTH                                 0x00000002
#define STBPAD_CNFG__WEAKPULL_EN__MASK                                  0x00000018


/*  STBPAD_CNFG : TXSLEW_CTRL_P       */
#define STBPAD_CNFG__TXSLEW_CTRL_P__SHIFT                               0x00000005
#define STBPAD_CNFG__TXSLEW_CTRL_P__WIDTH                               0x00000004
#define STBPAD_CNFG__TXSLEW_CTRL_P__MASK                                0x000001E0

/*  STBPAD_CNFG : TXSLEW_CTRL_N       */
#define STBPAD_CNFG__TXSLEW_CTRL_N__SHIFT                               0x00000009
#define STBPAD_CNFG__TXSLEW_CTRL_N__WIDTH                               0x00000004
#define STBPAD_CNFG__TXSLEW_CTRL_N__MASK                                0x00001E00


/*-------------------------------------------------------------------------------------*/
/*  RSTNPAD_CNFG : RXSEL               */
#define RSTNPAD_CNFG__RXSEL__SHIFT                                      0x00000000
#define RSTNPAD_CNFG__RXSEL__WIDTH                                      0x00000003
#define RSTNPAD_CNFG__RXSEL__MASK                                       0x00000007

/*  RSTNPAD_CNFG : WEAKPULL_EN         */
#define RSTNPAD_CNFG__WEAKPULL_EN__SHIFT                                0x00000003
#define RSTNPAD_CNFG__WEAKPULL_EN__WIDTH                                0x00000002
#define RSTNPAD_CNFG__WEAKPULL_EN__MASK                                 0x00000018


/*  RSTNPAD_CNFG : TXSLEW_CTRL_P       */
#define RSTNPAD_CNFG__TXSLEW_CTRL_P__SHIFT                              0x00000005
#define RSTNPAD_CNFG__TXSLEW_CTRL_P__WIDTH                              0x00000004
#define RSTNPAD_CNFG__TXSLEW_CTRL_P__MASK                               0x000001E0

/*  RSTNPAD_CNFG : TXSLEW_CTRL_N       */
#define RSTNPAD_CNFG__TXSLEW_CTRL_N__SHIFT                              0x00000009
#define RSTNPAD_CNFG__TXSLEW_CTRL_N__WIDTH                              0x00000004
#define RSTNPAD_CNFG__TXSLEW_CTRL_N__MASK                               0x00001E00


/*-------------------------------------------------------------------------------------*/
/*  PADTEST_CNFG : TESTMODE_EN         */
#define PADTEST_CNFG__TESTMODE_EN__SHIFT                                0x00000000
#define PADTEST_CNFG__TESTMODE_EN__WIDTH                                0x00000001
#define PADTEST_CNFG__TESTMODE_EN__MASK                                 0x00000001


/*  PADTEST_CNFG : RSVD_1              */
#define PADTEST_CNFG__RSVD_1__SHIFT                                     0x00000001
#define PADTEST_CNFG__RSVD_1__WIDTH                                     0x00000003
#define PADTEST_CNFG__RSVD_1__MASK                                      0x0000000E

/*  PADTEST_CNFG : TEST_OE             */
#define PADTEST_CNFG__TEST_OE__SHIFT                                    0x00000004
#define PADTEST_CNFG__TEST_OE__WIDTH                                    0x0000000C
#define PADTEST_CNFG__TEST_OE__MASK                                     0x0000FFF0


/*-------------------------------------------------------------------------------------*/
/*  PADTEST_OUT : TESTDATA_OUT        */
#define PADTEST_OUT__TESTDATA_OUT__SHIFT                                0x00000000
#define PADTEST_OUT__TESTDATA_OUT__WIDTH                                0x0000000C
#define PADTEST_OUT__TESTDATA_OUT__MASK                                 0x00000FFF


/*-------------------------------------------------------------------------------------*/
/*  PADTEST_IN : TESTDATA_IN         */
#define PADTEST_IN__TESTDATA_IN__SHIFT                                  0x00000000
#define PADTEST_IN__TESTDATA_IN__WIDTH                                  0x0000000C
#define PADTEST_IN__TESTDATA_IN__MASK                                   0x00000FFF


/*-------------------------------------------------------------------------------------*/
/*  COMMDL_CNFG : DLSTEP_SEL          */
#define COMMDL_CNFG__DLSTEP_SEL__SHIFT                                  0x00000000
#define COMMDL_CNFG__DLSTEP_SEL__WIDTH                                  0x00000001
#define COMMDL_CNFG__DLSTEP_SEL__MASK                                   0x00000001

/*  COMMDL_CNFG : DLOUT_EN            */
#define COMMDL_CNFG__DLOUT_EN__SHIFT                                    0x00000001
#define COMMDL_CNFG__DLOUT_EN__WIDTH                                    0x00000001
#define COMMDL_CNFG__DLOUT_EN__MASK                                     0x00000002



/*-------------------------------------------------------------------------------------*/
/*  SDCLKDL_CNFG : EXTDLY_EN           */
#define SDCLKDL_CNFG__EXTDLY_EN__SHIFT                                  0x00000000
#define SDCLKDL_CNFG__EXTDLY_EN__WIDTH                                  0x00000001
#define SDCLKDL_CNFG__EXTDLY_EN__MASK                                   0x00000001


/*  SDCLKDL_CNFG : BYPASS_EN           */
#define SDCLKDL_CNFG__BYPASS_EN__SHIFT                                  0x00000001
#define SDCLKDL_CNFG__BYPASS_EN__WIDTH                                  0x00000001
#define SDCLKDL_CNFG__BYPASS_EN__MASK                                   0x00000002


/*  SDCLKDL_CNFG : INPSEL_CNFG         */
#define SDCLKDL_CNFG__INPSEL_CNFG__SHIFT                                0x00000002
#define SDCLKDL_CNFG__INPSEL_CNFG__WIDTH                                0x00000002
#define SDCLKDL_CNFG__INPSEL_CNFG__MASK                                 0x0000000C

/*  SDCLKDL_CNFG : UPDATE_DC           */
#define SDCLKDL_CNFG__UPDATE_DC__SHIFT                                  0x00000004
#define SDCLKDL_CNFG__UPDATE_DC__WIDTH                                  0x00000001
#define SDCLKDL_CNFG__UPDATE_DC__MASK                                   0x00000010



/*-------------------------------------------------------------------------------------*/
/*  SDCLKDL_DC : CCKDL_DC            */
#define SDCLKDL_DC__CCKDL_DC__SHIFT                                     0x00000000
#define SDCLKDL_DC__CCKDL_DC__WIDTH                                     0x00000007
#define SDCLKDL_DC__CCKDL_DC__MASK                                      0x0000007F


/*-------------------------------------------------------------------------------------*/
/*  SMPLDL_CNFG : EXTDLY_EN           */
#define SMPLDL_CNFG__EXTDLY_EN__SHIFT                                   0x00000000
#define SMPLDL_CNFG__EXTDLY_EN__WIDTH                                   0x00000001
#define SMPLDL_CNFG__EXTDLY_EN__MASK                                    0x00000001


/*  SMPLDL_CNFG : BYPASS_EN           */
#define SMPLDL_CNFG__BYPASS_EN__SHIFT                                   0x00000001
#define SMPLDL_CNFG__BYPASS_EN__WIDTH                                   0x00000001
#define SMPLDL_CNFG__BYPASS_EN__MASK                                    0x00000002


/*  SMPLDL_CNFG : INPSEL_CNFG         */
#define SMPLDL_CNFG__INPSEL_CNFG__SHIFT                                 0x00000002
#define SMPLDL_CNFG__INPSEL_CNFG__WIDTH                                 0x00000002
#define SMPLDL_CNFG__INPSEL_CNFG__MASK                                  0x0000000C

/*  SMPLDL_CNFG : INPSEL_OVERRIDE     */
#define SMPLDL_CNFG__INPSEL_OVERRIDE__SHIFT                             0x00000004
#define SMPLDL_CNFG__INPSEL_OVERRIDE__WIDTH                             0x00000001
#define SMPLDL_CNFG__INPSEL_OVERRIDE__MASK                              0x00000010


/*-------------------------------------------------------------------------------------*/
/*  ATDL_CNFG : EXTDLY_EN           */
#define ATDL_CNFG__EXTDLY_EN__SHIFT                                     0x00000000
#define ATDL_CNFG__EXTDLY_EN__WIDTH                                     0x00000001
#define ATDL_CNFG__EXTDLY_EN__MASK                                      0x00000001


/*  ATDL_CNFG : BYPASS_EN           */
#define ATDL_CNFG__BYPASS_EN__SHIFT                                     0x00000001
#define ATDL_CNFG__BYPASS_EN__WIDTH                                     0x00000001
#define ATDL_CNFG__BYPASS_EN__MASK                                      0x00000002


/*  ATDL_CNFG : INPSEL_CNFG         */
#define ATDL_CNFG__INPSEL_CNFG__SHIFT                                   0x00000002
#define ATDL_CNFG__INPSEL_CNFG__WIDTH                                   0x00000002
#define ATDL_CNFG__INPSEL_CNFG__MASK                                    0x0000000C


/*-------------------------------------------------------------------------------------*/
/*  DLL_CTRL : DLL_EN              */
#define DLL_CTRL__DLL_EN__SHIFT                                         0x00000000
#define DLL_CTRL__DLL_EN__WIDTH                                         0x00000001
#define DLL_CTRL__DLL_EN__MASK                                          0x00000001


/*  DLL_CTRL : OFFST_EN            */
#define DLL_CTRL__OFFST_EN__SHIFT                                       0x00000001
#define DLL_CTRL__OFFST_EN__WIDTH                                       0x00000001
#define DLL_CTRL__OFFST_EN__MASK                                        0x00000002



/*-------------------------------------------------------------------------------------*/
/*  DLL_CNFG1 : WAITCYCLE           */
#define DLL_CNFG1__WAITCYCLE__SHIFT                                     0x00000000
#define DLL_CNFG1__WAITCYCLE__WIDTH                                     0x00000003
#define DLL_CNFG1__WAITCYCLE__MASK                                      0x00000007

/*  DLL_CNFG1 : SLVDLY              */
#define DLL_CNFG1__SLVDLY__SHIFT                                        0x00000004
#define DLL_CNFG1__SLVDLY__WIDTH                                        0x00000002
#define DLL_CNFG1__SLVDLY__MASK                                         0x00000030


/*-------------------------------------------------------------------------------------*/
/*  DLL_CNFG2 : JUMPSTEP            */
#define DLL_CNFG2__JUMPSTEP__SHIFT                                      0x00000000
#define DLL_CNFG2__JUMPSTEP__WIDTH                                      0x00000007
#define DLL_CNFG2__JUMPSTEP__MASK                                       0x0000007F


/*-------------------------------------------------------------------------------------*/
/*  DLLDL_CNFG : MST_EXTDLYEN        */
#define DLLDL_CNFG__MST_EXTDLYEN__SHIFT                                 0x00000000
#define DLLDL_CNFG__MST_EXTDLYEN__WIDTH                                 0x00000001
#define DLLDL_CNFG__MST_EXTDLYEN__MASK                                  0x00000001

/*  DLLDL_CNFG : MST_INPSEL          */
#define DLLDL_CNFG__MST_INPSEL__SHIFT                                   0x00000001
#define DLLDL_CNFG__MST_INPSEL__WIDTH                                   0x00000002
#define DLLDL_CNFG__MST_INPSEL__MASK                                    0x00000006

/*  DLLDL_CNFG : MST_BYPASS          */
#define DLLDL_CNFG__MST_BYPASS__SHIFT                                   0x00000003
#define DLLDL_CNFG__MST_BYPASS__WIDTH                                   0x00000001
#define DLLDL_CNFG__MST_BYPASS__MASK                                    0x00000008

/*  DLLDL_CNFG : SLV_EXTDLYEN        */
#define DLLDL_CNFG__SLV_EXTDLYEN__SHIFT                                 0x00000004
#define DLLDL_CNFG__SLV_EXTDLYEN__WIDTH                                 0x00000001
#define DLLDL_CNFG__SLV_EXTDLYEN__MASK                                  0x00000010

/*  DLLDL_CNFG : SLV_INPSEL          */
#define DLLDL_CNFG__SLV_INPSEL__SHIFT                                   0x00000005
#define DLLDL_CNFG__SLV_INPSEL__WIDTH                                   0x00000002
#define DLLDL_CNFG__SLV_INPSEL__MASK                                    0x00000060

/*  DLLDL_CNFG : SLV_BYPASS          */
#define DLLDL_CNFG__SLV_BYPASS__SHIFT                                   0x00000007
#define DLLDL_CNFG__SLV_BYPASS__WIDTH                                   0x00000001
#define DLLDL_CNFG__SLV_BYPASS__MASK                                    0x00000080


/*-------------------------------------------------------------------------------------*/
/*  DLL_OFFST : OFFST               */
#define DLL_OFFST__OFFST__SHIFT                                         0x00000000
#define DLL_OFFST__OFFST__WIDTH                                         0x00000007
#define DLL_OFFST__OFFST__MASK                                          0x0000007F


/*-------------------------------------------------------------------------------------*/
/*  DLLMST_TSTDC : MSTTST_DC           */
#define DLLMST_TSTDC__MSTTST_DC__SHIFT                                  0x00000000
#define DLLMST_TSTDC__MSTTST_DC__WIDTH                                  0x00000007
#define DLLMST_TSTDC__MSTTST_DC__MASK                                   0x0000007F


/*-------------------------------------------------------------------------------------*/
/*  DLLLBT_CNFG : LBT_LOADVAL         */
#define DLLLBT_CNFG__LBT_LOADVAL__SHIFT                                 0x00000000
#define DLLLBT_CNFG__LBT_LOADVAL__WIDTH                                 0x00000010
#define DLLLBT_CNFG__LBT_LOADVAL__MASK                                  0x0000FFFF


/*-------------------------------------------------------------------------------------*/
/*  DLL_STATUS : LOCK_STS            */
#define DLL_STATUS__LOCK_STS__SHIFT                                     0x00000000
#define DLL_STATUS__LOCK_STS__WIDTH                                     0x00000001
#define DLL_STATUS__LOCK_STS__MASK                                      0x00000001


/*  DLL_STATUS : ERROR_STS           */
#define DLL_STATUS__ERROR_STS__SHIFT                                    0x00000001
#define DLL_STATUS__ERROR_STS__WIDTH                                    0x00000001
#define DLL_STATUS__ERROR_STS__MASK                                     0x00000002



/*-------------------------------------------------------------------------------------*/
/*  DLLDBG_MLKDC : MSTLKDC             */
#define DLLDBG_MLKDC__MSTLKDC__SHIFT                                    0x00000000
#define DLLDBG_MLKDC__MSTLKDC__WIDTH                                    0x00000007
#define DLLDBG_MLKDC__MSTLKDC__MASK                                     0x0000007F


/*-------------------------------------------------------------------------------------*/
/*  DLLDBG_SLKDC : SLVLKDC             */
#define DLLDBG_SLKDC__SLVLKDC__SHIFT                                    0x00000000
#define DLLDBG_SLKDC__SLVLKDC__WIDTH                                    0x00000007
#define DLLDBG_SLKDC__SLVLKDC__MASK                                     0x0000007F




#endif /* __DWC_MSHC_CRYPTO_MACRO_H */

