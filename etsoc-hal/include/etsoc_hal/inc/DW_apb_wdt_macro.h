/* Header generated from ipxact (2019-01-09 14:47:11) */


#ifndef __DW_APB_WDT_MACRO_H
#define __DW_APB_WDT_MACRO_H



/*-------------------------------------------------------------------------------------*/
/*  WDT_CR : WDT_EN              */
#define WDT_CR__WDT_EN__SHIFT                                           0x00000000
#define WDT_CR__WDT_EN__WIDTH                                           0x00000001
#define WDT_CR__WDT_EN__MASK                                            0x00000001


/*  WDT_CR : RMOD                */
#define WDT_CR__RMOD__SHIFT                                             0x00000001
#define WDT_CR__RMOD__WIDTH                                             0x00000001
#define WDT_CR__RMOD__MASK                                              0x00000002


/*  WDT_CR : RPL                 */
#define WDT_CR__RPL__SHIFT                                              0x00000002
#define WDT_CR__RPL__WIDTH                                              0x00000003
#define WDT_CR__RPL__MASK                                               0x0000001C


/*  WDT_CR : NO_NAME             */
#define WDT_CR__NO_NAME__SHIFT                                          0x00000005
#define WDT_CR__NO_NAME__WIDTH                                          0x00000001
#define WDT_CR__NO_NAME__MASK                                           0x00000020

/*  WDT_CR : RSVD_WDT_CR         */
#define WDT_CR__RSVD_WDT_CR__SHIFT                                      0x00000006
#define WDT_CR__RSVD_WDT_CR__WIDTH                                      0x0000001A
#define WDT_CR__RSVD_WDT_CR__MASK                                       0xFFFFFFC0


/*-------------------------------------------------------------------------------------*/
/*  WDT_TORR : TOP                 */
#define WDT_TORR__TOP__SHIFT                                            0x00000000
#define WDT_TORR__TOP__WIDTH                                            0x00000004
#define WDT_TORR__TOP__MASK                                             0x0000000F


/*  WDT_TORR : RSVD_TOP_INIT       */
#define WDT_TORR__RSVD_TOP_INIT__SHIFT                                  0x00000004
#define WDT_TORR__RSVD_TOP_INIT__WIDTH                                  0x00000004
#define WDT_TORR__RSVD_TOP_INIT__MASK                                   0x000000F0

/*  WDT_TORR : RSVD_WDT_TORR       */
#define WDT_TORR__RSVD_WDT_TORR__SHIFT                                  0x00000008
#define WDT_TORR__RSVD_WDT_TORR__WIDTH                                  0x00000018
#define WDT_TORR__RSVD_WDT_TORR__MASK                                   0xFFFFFF00


/*-------------------------------------------------------------------------------------*/
/*  WDT_CCVR : WDT_CCVR            */
#define WDT_CCVR__WDT_CCVR__SHIFT                                       0x00000000
#define WDT_CCVR__WDT_CCVR__WIDTH                                       0x00000020
#define WDT_CCVR__WDT_CCVR__MASK                                        0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  WDT_CRR : WDT_CRR             */
#define WDT_CRR__WDT_CRR__SHIFT                                         0x00000000
#define WDT_CRR__WDT_CRR__WIDTH                                         0x00000008
#define WDT_CRR__WDT_CRR__MASK                                          0x000000FF


/*  WDT_CRR : RSVD_WDT_CRR        */
#define WDT_CRR__RSVD_WDT_CRR__SHIFT                                    0x00000008
#define WDT_CRR__RSVD_WDT_CRR__WIDTH                                    0x00000018
#define WDT_CRR__RSVD_WDT_CRR__MASK                                     0xFFFFFF00


/*-------------------------------------------------------------------------------------*/
/*  WDT_STAT : WDT_STAT            */
#define WDT_STAT__WDT_STAT__SHIFT                                       0x00000000
#define WDT_STAT__WDT_STAT__WIDTH                                       0x00000001
#define WDT_STAT__WDT_STAT__MASK                                        0x00000001


/*  WDT_STAT : RSVD_WDT_STAT       */
#define WDT_STAT__RSVD_WDT_STAT__SHIFT                                  0x00000001
#define WDT_STAT__RSVD_WDT_STAT__WIDTH                                  0x0000001F
#define WDT_STAT__RSVD_WDT_STAT__MASK                                   0xFFFFFFFE


/*-------------------------------------------------------------------------------------*/
/*  WDT_EOI : WDT_EOI             */
#define WDT_EOI__WDT_EOI__SHIFT                                         0x00000000
#define WDT_EOI__WDT_EOI__WIDTH                                         0x00000001
#define WDT_EOI__WDT_EOI__MASK                                          0x00000001

/*  WDT_EOI : RSVD_WDT_EOI        */
#define WDT_EOI__RSVD_WDT_EOI__SHIFT                                    0x00000001
#define WDT_EOI__RSVD_WDT_EOI__WIDTH                                    0x0000001F
#define WDT_EOI__RSVD_WDT_EOI__MASK                                     0xFFFFFFFE


/*-------------------------------------------------------------------------------------*/
/*  WDT_COMP_PARAM_5 : CP_WDT_USER_TOP_MAX */
#define WDT_COMP_PARAM_5__CP_WDT_USER_TOP_MAX__SHIFT                    0x00000000
#define WDT_COMP_PARAM_5__CP_WDT_USER_TOP_MAX__WIDTH                    0x00000020
#define WDT_COMP_PARAM_5__CP_WDT_USER_TOP_MAX__MASK                     0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  WDT_COMP_PARAM_4 : CP_WDT_USER_TOP_INIT_MAX*/
#define WDT_COMP_PARAM_4__CP_WDT_USER_TOP_INIT_MAX__SHIFT               0x00000000
#define WDT_COMP_PARAM_4__CP_WDT_USER_TOP_INIT_MAX__WIDTH               0x00000020
#define WDT_COMP_PARAM_4__CP_WDT_USER_TOP_INIT_MAX__MASK                0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  WDT_COMP_PARAM_3 : CD_WDT_TOP_RST      */
#define WDT_COMP_PARAM_3__CD_WDT_TOP_RST__SHIFT                         0x00000000
#define WDT_COMP_PARAM_3__CD_WDT_TOP_RST__WIDTH                         0x00000020
#define WDT_COMP_PARAM_3__CD_WDT_TOP_RST__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  WDT_COMP_PARAM_2 : CP_WDT_CNT_RST      */
#define WDT_COMP_PARAM_2__CP_WDT_CNT_RST__SHIFT                         0x00000000
#define WDT_COMP_PARAM_2__CP_WDT_CNT_RST__WIDTH                         0x00000020
#define WDT_COMP_PARAM_2__CP_WDT_CNT_RST__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  WDT_COMP_PARAM_1 : WDT_ALWAYS_EN       */
#define WDT_COMP_PARAM_1__WDT_ALWAYS_EN__SHIFT                          0x00000000
#define WDT_COMP_PARAM_1__WDT_ALWAYS_EN__WIDTH                          0x00000001
#define WDT_COMP_PARAM_1__WDT_ALWAYS_EN__MASK                           0x00000001


/*  WDT_COMP_PARAM_1 : WDT_DFLT_RMOD       */
#define WDT_COMP_PARAM_1__WDT_DFLT_RMOD__SHIFT                          0x00000001
#define WDT_COMP_PARAM_1__WDT_DFLT_RMOD__WIDTH                          0x00000001
#define WDT_COMP_PARAM_1__WDT_DFLT_RMOD__MASK                           0x00000002


/*  WDT_COMP_PARAM_1 : WDT_DUAL_TOP        */
#define WDT_COMP_PARAM_1__WDT_DUAL_TOP__SHIFT                           0x00000002
#define WDT_COMP_PARAM_1__WDT_DUAL_TOP__WIDTH                           0x00000001
#define WDT_COMP_PARAM_1__WDT_DUAL_TOP__MASK                            0x00000004


/*  WDT_COMP_PARAM_1 : WDT_HC_RMOD         */
#define WDT_COMP_PARAM_1__WDT_HC_RMOD__SHIFT                            0x00000003
#define WDT_COMP_PARAM_1__WDT_HC_RMOD__WIDTH                            0x00000001
#define WDT_COMP_PARAM_1__WDT_HC_RMOD__MASK                             0x00000008


/*  WDT_COMP_PARAM_1 : WDT_HC_RPL          */
#define WDT_COMP_PARAM_1__WDT_HC_RPL__SHIFT                             0x00000004
#define WDT_COMP_PARAM_1__WDT_HC_RPL__WIDTH                             0x00000001
#define WDT_COMP_PARAM_1__WDT_HC_RPL__MASK                              0x00000010


/*  WDT_COMP_PARAM_1 : WDT_HC_TOP          */
#define WDT_COMP_PARAM_1__WDT_HC_TOP__SHIFT                             0x00000005
#define WDT_COMP_PARAM_1__WDT_HC_TOP__WIDTH                             0x00000001
#define WDT_COMP_PARAM_1__WDT_HC_TOP__MASK                              0x00000020


/*  WDT_COMP_PARAM_1 : WDT_USE_FIX_TOP     */
#define WDT_COMP_PARAM_1__WDT_USE_FIX_TOP__SHIFT                        0x00000006
#define WDT_COMP_PARAM_1__WDT_USE_FIX_TOP__WIDTH                        0x00000001
#define WDT_COMP_PARAM_1__WDT_USE_FIX_TOP__MASK                         0x00000040


/*  WDT_COMP_PARAM_1 : WDT_PAUSE           */
#define WDT_COMP_PARAM_1__WDT_PAUSE__SHIFT                              0x00000007
#define WDT_COMP_PARAM_1__WDT_PAUSE__WIDTH                              0x00000001
#define WDT_COMP_PARAM_1__WDT_PAUSE__MASK                               0x00000080


/*  WDT_COMP_PARAM_1 : APB_DATA_WIDTH      */
#define WDT_COMP_PARAM_1__APB_DATA_WIDTH__SHIFT                         0x00000008
#define WDT_COMP_PARAM_1__APB_DATA_WIDTH__WIDTH                         0x00000002
#define WDT_COMP_PARAM_1__APB_DATA_WIDTH__MASK                          0x00000300


/*  WDT_COMP_PARAM_1 : WDT_DFLT_RPL        */
#define WDT_COMP_PARAM_1__WDT_DFLT_RPL__SHIFT                           0x0000000A
#define WDT_COMP_PARAM_1__WDT_DFLT_RPL__WIDTH                           0x00000003
#define WDT_COMP_PARAM_1__WDT_DFLT_RPL__MASK                            0x00001C00

/*  WDT_COMP_PARAM_1 : RSVD_15_13          */
#define WDT_COMP_PARAM_1__RSVD_15_13__SHIFT                             0x0000000D
#define WDT_COMP_PARAM_1__RSVD_15_13__WIDTH                             0x00000003
#define WDT_COMP_PARAM_1__RSVD_15_13__MASK                              0x0000E000

/*  WDT_COMP_PARAM_1 : WDT_DFLT_TOP        */
#define WDT_COMP_PARAM_1__WDT_DFLT_TOP__SHIFT                           0x00000010
#define WDT_COMP_PARAM_1__WDT_DFLT_TOP__WIDTH                           0x00000004
#define WDT_COMP_PARAM_1__WDT_DFLT_TOP__MASK                            0x000F0000

/*  WDT_COMP_PARAM_1 : WDT_DFLT_TOP_INIT   */
#define WDT_COMP_PARAM_1__WDT_DFLT_TOP_INIT__SHIFT                      0x00000014
#define WDT_COMP_PARAM_1__WDT_DFLT_TOP_INIT__WIDTH                      0x00000004
#define WDT_COMP_PARAM_1__WDT_DFLT_TOP_INIT__MASK                       0x00F00000

/*  WDT_COMP_PARAM_1 : WDT_CNT_WIDTH       */
#define WDT_COMP_PARAM_1__WDT_CNT_WIDTH__SHIFT                          0x00000018
#define WDT_COMP_PARAM_1__WDT_CNT_WIDTH__WIDTH                          0x00000005
#define WDT_COMP_PARAM_1__WDT_CNT_WIDTH__MASK                           0x1F000000

/*  WDT_COMP_PARAM_1 : RSVD_31_29          */
#define WDT_COMP_PARAM_1__RSVD_31_29__SHIFT                             0x0000001D
#define WDT_COMP_PARAM_1__RSVD_31_29__WIDTH                             0x00000003
#define WDT_COMP_PARAM_1__RSVD_31_29__MASK                              0xE0000000


/*-------------------------------------------------------------------------------------*/
/*  WDT_COMP_VERSION : WDT_COMP_VERSION    */
#define WDT_COMP_VERSION__WDT_COMP_VERSION__SHIFT                       0x00000000
#define WDT_COMP_VERSION__WDT_COMP_VERSION__WIDTH                       0x00000020
#define WDT_COMP_VERSION__WDT_COMP_VERSION__MASK                        0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  WDT_COMP_TYPE : WDT_COMP_TYPE       */
#define WDT_COMP_TYPE__WDT_COMP_TYPE__SHIFT                             0x00000000
#define WDT_COMP_TYPE__WDT_COMP_TYPE__WIDTH                             0x00000020
#define WDT_COMP_TYPE__WDT_COMP_TYPE__MASK                              0xFFFFFFFF




#endif /* __DW_APB_WDT_MACRO_H */

