/* Header generated from ipxact (2019-01-09 14:47:11) */


#ifndef __DW_APB_TIMERS_MACRO_H
#define __DW_APB_TIMERS_MACRO_H



/*-------------------------------------------------------------------------------------*/
/*  TIMER1LOADCOUNT : TimerNLoadCount     */
#define TIMER1LOADCOUNT__TIMERNLOADCOUNT__SHIFT                         0x00000000
#define TIMER1LOADCOUNT__TIMERNLOADCOUNT__WIDTH                         0x00000020
#define TIMER1LOADCOUNT__TIMERNLOADCOUNT__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER1CURRENTVAL : TimerNCurrentValue  */
#define TIMER1CURRENTVAL__TIMERNCURRENTVALUE__SHIFT                     0x00000000
#define TIMER1CURRENTVAL__TIMERNCURRENTVALUE__WIDTH                     0x00000020
#define TIMER1CURRENTVAL__TIMERNCURRENTVALUE__MASK                      0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER1CONTROLREG : TIMER_ENABLE        */
#define TIMER1CONTROLREG__TIMER_ENABLE__SHIFT                           0x00000000
#define TIMER1CONTROLREG__TIMER_ENABLE__WIDTH                           0x00000001
#define TIMER1CONTROLREG__TIMER_ENABLE__MASK                            0x00000001


/*  TIMER1CONTROLREG : TIMER_MODE          */
#define TIMER1CONTROLREG__TIMER_MODE__SHIFT                             0x00000001
#define TIMER1CONTROLREG__TIMER_MODE__WIDTH                             0x00000001
#define TIMER1CONTROLREG__TIMER_MODE__MASK                              0x00000002


/*  TIMER1CONTROLREG : TIMER_INTERRUPT_MASK*/
#define TIMER1CONTROLREG__TIMER_INTERRUPT_MASK__SHIFT                   0x00000002
#define TIMER1CONTROLREG__TIMER_INTERRUPT_MASK__WIDTH                   0x00000001
#define TIMER1CONTROLREG__TIMER_INTERRUPT_MASK__MASK                    0x00000004


/*  TIMER1CONTROLREG : TIMER_PWM           */
#define TIMER1CONTROLREG__TIMER_PWM__SHIFT                              0x00000003
#define TIMER1CONTROLREG__TIMER_PWM__WIDTH                              0x00000001
#define TIMER1CONTROLREG__TIMER_PWM__MASK                               0x00000008


/*  TIMER1CONTROLREG : RSVD_TIMER_0N100PWM_EN*/
#define TIMER1CONTROLREG__RSVD_TIMER_0N100PWM_EN__SHIFT                 0x00000004
#define TIMER1CONTROLREG__RSVD_TIMER_0N100PWM_EN__WIDTH                 0x00000001
#define TIMER1CONTROLREG__RSVD_TIMER_0N100PWM_EN__MASK                  0x00000010

/*  TIMER1CONTROLREG : RSVD_TimerNControlReg*/
#define TIMER1CONTROLREG__RSVD_TIMERNCONTROLREG__SHIFT                  0x00000005
#define TIMER1CONTROLREG__RSVD_TIMERNCONTROLREG__WIDTH                  0x0000001B
#define TIMER1CONTROLREG__RSVD_TIMERNCONTROLREG__MASK                   0xFFFFFFE0


/*-------------------------------------------------------------------------------------*/
/*  TIMER1EOI : TimerNEOI           */
#define TIMER1EOI__TIMERNEOI__SHIFT                                     0x00000000
#define TIMER1EOI__TIMERNEOI__WIDTH                                     0x00000001
#define TIMER1EOI__TIMERNEOI__MASK                                      0x00000001

/*  TIMER1EOI : RSVD_TimerNEOI      */
#define TIMER1EOI__RSVD_TIMERNEOI__SHIFT                                0x00000001
#define TIMER1EOI__RSVD_TIMERNEOI__WIDTH                                0x0000001F
#define TIMER1EOI__RSVD_TIMERNEOI__MASK                                 0xFFFFFFFE


/*-------------------------------------------------------------------------------------*/
/*  TIMER1INTSTAT : TimerNIntStatus     */
#define TIMER1INTSTAT__TIMERNINTSTATUS__SHIFT                           0x00000000
#define TIMER1INTSTAT__TIMERNINTSTATUS__WIDTH                           0x00000001
#define TIMER1INTSTAT__TIMERNINTSTATUS__MASK                            0x00000001


/*  TIMER1INTSTAT : RSVD_TimerNIntStatus*/
#define TIMER1INTSTAT__RSVD_TIMERNINTSTATUS__SHIFT                      0x00000001
#define TIMER1INTSTAT__RSVD_TIMERNINTSTATUS__WIDTH                      0x0000001F
#define TIMER1INTSTAT__RSVD_TIMERNINTSTATUS__MASK                       0xFFFFFFFE


/*-------------------------------------------------------------------------------------*/
/*  TIMER2LOADCOUNT : TIMER2LOADCOUNT     */
#define TIMER2LOADCOUNT__TIMER2LOADCOUNT__SHIFT                         0x00000000
#define TIMER2LOADCOUNT__TIMER2LOADCOUNT__WIDTH                         0x00000020
#define TIMER2LOADCOUNT__TIMER2LOADCOUNT__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER2CURRENTVAL : TIMER2CURRENTVAL    */
#define TIMER2CURRENTVAL__TIMER2CURRENTVAL__SHIFT                       0x00000000
#define TIMER2CURRENTVAL__TIMER2CURRENTVAL__WIDTH                       0x00000020
#define TIMER2CURRENTVAL__TIMER2CURRENTVAL__MASK                        0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER2CONTROLREG : TIMER_ENABLE        */
#define TIMER2CONTROLREG__TIMER_ENABLE__SHIFT                           0x00000000
#define TIMER2CONTROLREG__TIMER_ENABLE__WIDTH                           0x00000001
#define TIMER2CONTROLREG__TIMER_ENABLE__MASK                            0x00000001


/*  TIMER2CONTROLREG : TIMER_MODE          */
#define TIMER2CONTROLREG__TIMER_MODE__SHIFT                             0x00000001
#define TIMER2CONTROLREG__TIMER_MODE__WIDTH                             0x00000001
#define TIMER2CONTROLREG__TIMER_MODE__MASK                              0x00000002


/*  TIMER2CONTROLREG : TIMER_INTERRUPT_MASK*/
#define TIMER2CONTROLREG__TIMER_INTERRUPT_MASK__SHIFT                   0x00000002
#define TIMER2CONTROLREG__TIMER_INTERRUPT_MASK__WIDTH                   0x00000001
#define TIMER2CONTROLREG__TIMER_INTERRUPT_MASK__MASK                    0x00000004


/*  TIMER2CONTROLREG : TIMER_PWM           */
#define TIMER2CONTROLREG__TIMER_PWM__SHIFT                              0x00000003
#define TIMER2CONTROLREG__TIMER_PWM__WIDTH                              0x00000001
#define TIMER2CONTROLREG__TIMER_PWM__MASK                               0x00000008


/*  TIMER2CONTROLREG : RSVD_TIMER_0N100PWM_EN*/
#define TIMER2CONTROLREG__RSVD_TIMER_0N100PWM_EN__SHIFT                 0x00000004
#define TIMER2CONTROLREG__RSVD_TIMER_0N100PWM_EN__WIDTH                 0x00000001
#define TIMER2CONTROLREG__RSVD_TIMER_0N100PWM_EN__MASK                  0x00000010

/*  TIMER2CONTROLREG : RSVD_TIMER2CONTROLREG*/
#define TIMER2CONTROLREG__RSVD_TIMER2CONTROLREG__SHIFT                  0x00000005
#define TIMER2CONTROLREG__RSVD_TIMER2CONTROLREG__WIDTH                  0x0000001B
#define TIMER2CONTROLREG__RSVD_TIMER2CONTROLREG__MASK                   0xFFFFFFE0


/*-------------------------------------------------------------------------------------*/
/*  TIMER2EOI : TIMER2EOI           */
#define TIMER2EOI__TIMER2EOI__SHIFT                                     0x00000000
#define TIMER2EOI__TIMER2EOI__WIDTH                                     0x00000001
#define TIMER2EOI__TIMER2EOI__MASK                                      0x00000001

/*  TIMER2EOI : RSVD_TIMER2EOI      */
#define TIMER2EOI__RSVD_TIMER2EOI__SHIFT                                0x00000001
#define TIMER2EOI__RSVD_TIMER2EOI__WIDTH                                0x0000001F
#define TIMER2EOI__RSVD_TIMER2EOI__MASK                                 0xFFFFFFFE


/*-------------------------------------------------------------------------------------*/
/*  TIMER2INTSTAT : TIMER2INTSTAT       */
#define TIMER2INTSTAT__TIMER2INTSTAT__SHIFT                             0x00000000
#define TIMER2INTSTAT__TIMER2INTSTAT__WIDTH                             0x00000001
#define TIMER2INTSTAT__TIMER2INTSTAT__MASK                              0x00000001


/*  TIMER2INTSTAT : RSVD_TIMER2INTSTAT  */
#define TIMER2INTSTAT__RSVD_TIMER2INTSTAT__SHIFT                        0x00000001
#define TIMER2INTSTAT__RSVD_TIMER2INTSTAT__WIDTH                        0x0000001F
#define TIMER2INTSTAT__RSVD_TIMER2INTSTAT__MASK                         0xFFFFFFFE


/*-------------------------------------------------------------------------------------*/
/*  TIMER3LOADCOUNT : TIMER3LOADCOUNT     */
#define TIMER3LOADCOUNT__TIMER3LOADCOUNT__SHIFT                         0x00000000
#define TIMER3LOADCOUNT__TIMER3LOADCOUNT__WIDTH                         0x00000020
#define TIMER3LOADCOUNT__TIMER3LOADCOUNT__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER3CURRENTVAL : TIMER3CURRENTVAL    */
#define TIMER3CURRENTVAL__TIMER3CURRENTVAL__SHIFT                       0x00000000
#define TIMER3CURRENTVAL__TIMER3CURRENTVAL__WIDTH                       0x00000020
#define TIMER3CURRENTVAL__TIMER3CURRENTVAL__MASK                        0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER3CONTROLREG : TIMER_ENABLE        */
#define TIMER3CONTROLREG__TIMER_ENABLE__SHIFT                           0x00000000
#define TIMER3CONTROLREG__TIMER_ENABLE__WIDTH                           0x00000001
#define TIMER3CONTROLREG__TIMER_ENABLE__MASK                            0x00000001


/*  TIMER3CONTROLREG : TIMER_MODE          */
#define TIMER3CONTROLREG__TIMER_MODE__SHIFT                             0x00000001
#define TIMER3CONTROLREG__TIMER_MODE__WIDTH                             0x00000001
#define TIMER3CONTROLREG__TIMER_MODE__MASK                              0x00000002


/*  TIMER3CONTROLREG : TIMER_INTERRUPT_MASK*/
#define TIMER3CONTROLREG__TIMER_INTERRUPT_MASK__SHIFT                   0x00000002
#define TIMER3CONTROLREG__TIMER_INTERRUPT_MASK__WIDTH                   0x00000001
#define TIMER3CONTROLREG__TIMER_INTERRUPT_MASK__MASK                    0x00000004


/*  TIMER3CONTROLREG : TIMER_PWM           */
#define TIMER3CONTROLREG__TIMER_PWM__SHIFT                              0x00000003
#define TIMER3CONTROLREG__TIMER_PWM__WIDTH                              0x00000001
#define TIMER3CONTROLREG__TIMER_PWM__MASK                               0x00000008


/*  TIMER3CONTROLREG : RSVD_TIMER_0N100PWM_EN*/
#define TIMER3CONTROLREG__RSVD_TIMER_0N100PWM_EN__SHIFT                 0x00000004
#define TIMER3CONTROLREG__RSVD_TIMER_0N100PWM_EN__WIDTH                 0x00000001
#define TIMER3CONTROLREG__RSVD_TIMER_0N100PWM_EN__MASK                  0x00000010

/*  TIMER3CONTROLREG : RSVD_TIMER3CONTROLREG*/
#define TIMER3CONTROLREG__RSVD_TIMER3CONTROLREG__SHIFT                  0x00000005
#define TIMER3CONTROLREG__RSVD_TIMER3CONTROLREG__WIDTH                  0x0000001B
#define TIMER3CONTROLREG__RSVD_TIMER3CONTROLREG__MASK                   0xFFFFFFE0


/*-------------------------------------------------------------------------------------*/
/*  TIMER3EOI : TIMER3EOI           */
#define TIMER3EOI__TIMER3EOI__SHIFT                                     0x00000000
#define TIMER3EOI__TIMER3EOI__WIDTH                                     0x00000001
#define TIMER3EOI__TIMER3EOI__MASK                                      0x00000001

/*  TIMER3EOI : RSVD_TIMER3EOI      */
#define TIMER3EOI__RSVD_TIMER3EOI__SHIFT                                0x00000001
#define TIMER3EOI__RSVD_TIMER3EOI__WIDTH                                0x0000001F
#define TIMER3EOI__RSVD_TIMER3EOI__MASK                                 0xFFFFFFFE


/*-------------------------------------------------------------------------------------*/
/*  TIMER3INTSTAT : TIMER3INTSTAT       */
#define TIMER3INTSTAT__TIMER3INTSTAT__SHIFT                             0x00000000
#define TIMER3INTSTAT__TIMER3INTSTAT__WIDTH                             0x00000001
#define TIMER3INTSTAT__TIMER3INTSTAT__MASK                              0x00000001


/*  TIMER3INTSTAT : RSVD_TIMER3INTSTAT  */
#define TIMER3INTSTAT__RSVD_TIMER3INTSTAT__SHIFT                        0x00000001
#define TIMER3INTSTAT__RSVD_TIMER3INTSTAT__WIDTH                        0x0000001F
#define TIMER3INTSTAT__RSVD_TIMER3INTSTAT__MASK                         0xFFFFFFFE


/*-------------------------------------------------------------------------------------*/
/*  TIMER4LOADCOUNT : TIMER4LOADCOUNT     */
#define TIMER4LOADCOUNT__TIMER4LOADCOUNT__SHIFT                         0x00000000
#define TIMER4LOADCOUNT__TIMER4LOADCOUNT__WIDTH                         0x00000020
#define TIMER4LOADCOUNT__TIMER4LOADCOUNT__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER4CURRENTVAL : TIMER4CURRENTVAL    */
#define TIMER4CURRENTVAL__TIMER4CURRENTVAL__SHIFT                       0x00000000
#define TIMER4CURRENTVAL__TIMER4CURRENTVAL__WIDTH                       0x00000020
#define TIMER4CURRENTVAL__TIMER4CURRENTVAL__MASK                        0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER4CONTROLREG : TIMER_ENABLE        */
#define TIMER4CONTROLREG__TIMER_ENABLE__SHIFT                           0x00000000
#define TIMER4CONTROLREG__TIMER_ENABLE__WIDTH                           0x00000001
#define TIMER4CONTROLREG__TIMER_ENABLE__MASK                            0x00000001


/*  TIMER4CONTROLREG : TIMER_MODE          */
#define TIMER4CONTROLREG__TIMER_MODE__SHIFT                             0x00000001
#define TIMER4CONTROLREG__TIMER_MODE__WIDTH                             0x00000001
#define TIMER4CONTROLREG__TIMER_MODE__MASK                              0x00000002


/*  TIMER4CONTROLREG : TIMER_INTERRUPT_MASK*/
#define TIMER4CONTROLREG__TIMER_INTERRUPT_MASK__SHIFT                   0x00000002
#define TIMER4CONTROLREG__TIMER_INTERRUPT_MASK__WIDTH                   0x00000001
#define TIMER4CONTROLREG__TIMER_INTERRUPT_MASK__MASK                    0x00000004


/*  TIMER4CONTROLREG : TIMER_PWM           */
#define TIMER4CONTROLREG__TIMER_PWM__SHIFT                              0x00000003
#define TIMER4CONTROLREG__TIMER_PWM__WIDTH                              0x00000001
#define TIMER4CONTROLREG__TIMER_PWM__MASK                               0x00000008


/*  TIMER4CONTROLREG : RSVD_TIMER_0N100PWM_EN*/
#define TIMER4CONTROLREG__RSVD_TIMER_0N100PWM_EN__SHIFT                 0x00000004
#define TIMER4CONTROLREG__RSVD_TIMER_0N100PWM_EN__WIDTH                 0x00000001
#define TIMER4CONTROLREG__RSVD_TIMER_0N100PWM_EN__MASK                  0x00000010

/*  TIMER4CONTROLREG : RSVD_TIMER4CONTROLREG*/
#define TIMER4CONTROLREG__RSVD_TIMER4CONTROLREG__SHIFT                  0x00000005
#define TIMER4CONTROLREG__RSVD_TIMER4CONTROLREG__WIDTH                  0x0000001B
#define TIMER4CONTROLREG__RSVD_TIMER4CONTROLREG__MASK                   0xFFFFFFE0


/*-------------------------------------------------------------------------------------*/
/*  TIMER4EOI : TIMER4EOI           */
#define TIMER4EOI__TIMER4EOI__SHIFT                                     0x00000000
#define TIMER4EOI__TIMER4EOI__WIDTH                                     0x00000001
#define TIMER4EOI__TIMER4EOI__MASK                                      0x00000001

/*  TIMER4EOI : RSVD_TIMER4EOI      */
#define TIMER4EOI__RSVD_TIMER4EOI__SHIFT                                0x00000001
#define TIMER4EOI__RSVD_TIMER4EOI__WIDTH                                0x0000001F
#define TIMER4EOI__RSVD_TIMER4EOI__MASK                                 0xFFFFFFFE


/*-------------------------------------------------------------------------------------*/
/*  TIMER4INTSTAT : TIMER4INTSTAT       */
#define TIMER4INTSTAT__TIMER4INTSTAT__SHIFT                             0x00000000
#define TIMER4INTSTAT__TIMER4INTSTAT__WIDTH                             0x00000001
#define TIMER4INTSTAT__TIMER4INTSTAT__MASK                              0x00000001


/*  TIMER4INTSTAT : RSVD_TIMER4INTSTAT  */
#define TIMER4INTSTAT__RSVD_TIMER4INTSTAT__SHIFT                        0x00000001
#define TIMER4INTSTAT__RSVD_TIMER4INTSTAT__WIDTH                        0x0000001F
#define TIMER4INTSTAT__RSVD_TIMER4INTSTAT__MASK                         0xFFFFFFFE


/*-------------------------------------------------------------------------------------*/
/*  TIMER5LOADCOUNT : TIMER5LOADCOUNT     */
#define TIMER5LOADCOUNT__TIMER5LOADCOUNT__SHIFT                         0x00000000
#define TIMER5LOADCOUNT__TIMER5LOADCOUNT__WIDTH                         0x00000020
#define TIMER5LOADCOUNT__TIMER5LOADCOUNT__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER5CURRENTVAL : TIMER5CURRENTVAL    */
#define TIMER5CURRENTVAL__TIMER5CURRENTVAL__SHIFT                       0x00000000
#define TIMER5CURRENTVAL__TIMER5CURRENTVAL__WIDTH                       0x00000020
#define TIMER5CURRENTVAL__TIMER5CURRENTVAL__MASK                        0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER5CONTROLREG : TIMER_ENABLE        */
#define TIMER5CONTROLREG__TIMER_ENABLE__SHIFT                           0x00000000
#define TIMER5CONTROLREG__TIMER_ENABLE__WIDTH                           0x00000001
#define TIMER5CONTROLREG__TIMER_ENABLE__MASK                            0x00000001


/*  TIMER5CONTROLREG : TIMER_MODE          */
#define TIMER5CONTROLREG__TIMER_MODE__SHIFT                             0x00000001
#define TIMER5CONTROLREG__TIMER_MODE__WIDTH                             0x00000001
#define TIMER5CONTROLREG__TIMER_MODE__MASK                              0x00000002


/*  TIMER5CONTROLREG : TIMER_INTERRUPT_MASK*/
#define TIMER5CONTROLREG__TIMER_INTERRUPT_MASK__SHIFT                   0x00000002
#define TIMER5CONTROLREG__TIMER_INTERRUPT_MASK__WIDTH                   0x00000001
#define TIMER5CONTROLREG__TIMER_INTERRUPT_MASK__MASK                    0x00000004


/*  TIMER5CONTROLREG : TIMER_PWM           */
#define TIMER5CONTROLREG__TIMER_PWM__SHIFT                              0x00000003
#define TIMER5CONTROLREG__TIMER_PWM__WIDTH                              0x00000001
#define TIMER5CONTROLREG__TIMER_PWM__MASK                               0x00000008


/*  TIMER5CONTROLREG : RSVD_TIMER_0N100PWM_EN*/
#define TIMER5CONTROLREG__RSVD_TIMER_0N100PWM_EN__SHIFT                 0x00000004
#define TIMER5CONTROLREG__RSVD_TIMER_0N100PWM_EN__WIDTH                 0x00000001
#define TIMER5CONTROLREG__RSVD_TIMER_0N100PWM_EN__MASK                  0x00000010

/*  TIMER5CONTROLREG : RSVD_TIMER5CONTROLREG*/
#define TIMER5CONTROLREG__RSVD_TIMER5CONTROLREG__SHIFT                  0x00000005
#define TIMER5CONTROLREG__RSVD_TIMER5CONTROLREG__WIDTH                  0x0000001B
#define TIMER5CONTROLREG__RSVD_TIMER5CONTROLREG__MASK                   0xFFFFFFE0


/*-------------------------------------------------------------------------------------*/
/*  TIMER5EOI : TIMER5EOI           */
#define TIMER5EOI__TIMER5EOI__SHIFT                                     0x00000000
#define TIMER5EOI__TIMER5EOI__WIDTH                                     0x00000001
#define TIMER5EOI__TIMER5EOI__MASK                                      0x00000001

/*  TIMER5EOI : RSVD_TIMER5EOI      */
#define TIMER5EOI__RSVD_TIMER5EOI__SHIFT                                0x00000001
#define TIMER5EOI__RSVD_TIMER5EOI__WIDTH                                0x0000001F
#define TIMER5EOI__RSVD_TIMER5EOI__MASK                                 0xFFFFFFFE


/*-------------------------------------------------------------------------------------*/
/*  TIMER5INTSTAT : TIMER5INTSTAT       */
#define TIMER5INTSTAT__TIMER5INTSTAT__SHIFT                             0x00000000
#define TIMER5INTSTAT__TIMER5INTSTAT__WIDTH                             0x00000001
#define TIMER5INTSTAT__TIMER5INTSTAT__MASK                              0x00000001


/*  TIMER5INTSTAT : RSVD_TIMER5INTSTAT  */
#define TIMER5INTSTAT__RSVD_TIMER5INTSTAT__SHIFT                        0x00000001
#define TIMER5INTSTAT__RSVD_TIMER5INTSTAT__WIDTH                        0x0000001F
#define TIMER5INTSTAT__RSVD_TIMER5INTSTAT__MASK                         0xFFFFFFFE


/*-------------------------------------------------------------------------------------*/
/*  TIMER6LOADCOUNT : TIMER6LOADCOUNT     */
#define TIMER6LOADCOUNT__TIMER6LOADCOUNT__SHIFT                         0x00000000
#define TIMER6LOADCOUNT__TIMER6LOADCOUNT__WIDTH                         0x00000020
#define TIMER6LOADCOUNT__TIMER6LOADCOUNT__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER6CURRENTVAL : TIMER6CURRENTVAL    */
#define TIMER6CURRENTVAL__TIMER6CURRENTVAL__SHIFT                       0x00000000
#define TIMER6CURRENTVAL__TIMER6CURRENTVAL__WIDTH                       0x00000020
#define TIMER6CURRENTVAL__TIMER6CURRENTVAL__MASK                        0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER6CONTROLREG : TIMER_ENABLE        */
#define TIMER6CONTROLREG__TIMER_ENABLE__SHIFT                           0x00000000
#define TIMER6CONTROLREG__TIMER_ENABLE__WIDTH                           0x00000001
#define TIMER6CONTROLREG__TIMER_ENABLE__MASK                            0x00000001


/*  TIMER6CONTROLREG : TIMER_MODE          */
#define TIMER6CONTROLREG__TIMER_MODE__SHIFT                             0x00000001
#define TIMER6CONTROLREG__TIMER_MODE__WIDTH                             0x00000001
#define TIMER6CONTROLREG__TIMER_MODE__MASK                              0x00000002


/*  TIMER6CONTROLREG : TIMER_INTERRUPT_MASK*/
#define TIMER6CONTROLREG__TIMER_INTERRUPT_MASK__SHIFT                   0x00000002
#define TIMER6CONTROLREG__TIMER_INTERRUPT_MASK__WIDTH                   0x00000001
#define TIMER6CONTROLREG__TIMER_INTERRUPT_MASK__MASK                    0x00000004


/*  TIMER6CONTROLREG : TIMER_PWM           */
#define TIMER6CONTROLREG__TIMER_PWM__SHIFT                              0x00000003
#define TIMER6CONTROLREG__TIMER_PWM__WIDTH                              0x00000001
#define TIMER6CONTROLREG__TIMER_PWM__MASK                               0x00000008


/*  TIMER6CONTROLREG : RSVD_TIMER_0N100PWM_EN*/
#define TIMER6CONTROLREG__RSVD_TIMER_0N100PWM_EN__SHIFT                 0x00000004
#define TIMER6CONTROLREG__RSVD_TIMER_0N100PWM_EN__WIDTH                 0x00000001
#define TIMER6CONTROLREG__RSVD_TIMER_0N100PWM_EN__MASK                  0x00000010

/*  TIMER6CONTROLREG : RSVD_TIMER6CONTROLREG*/
#define TIMER6CONTROLREG__RSVD_TIMER6CONTROLREG__SHIFT                  0x00000005
#define TIMER6CONTROLREG__RSVD_TIMER6CONTROLREG__WIDTH                  0x0000001B
#define TIMER6CONTROLREG__RSVD_TIMER6CONTROLREG__MASK                   0xFFFFFFE0


/*-------------------------------------------------------------------------------------*/
/*  TIMER6EOI : TIMER6EOI           */
#define TIMER6EOI__TIMER6EOI__SHIFT                                     0x00000000
#define TIMER6EOI__TIMER6EOI__WIDTH                                     0x00000001
#define TIMER6EOI__TIMER6EOI__MASK                                      0x00000001

/*  TIMER6EOI : RSVD_TIMER6EOI      */
#define TIMER6EOI__RSVD_TIMER6EOI__SHIFT                                0x00000001
#define TIMER6EOI__RSVD_TIMER6EOI__WIDTH                                0x0000001F
#define TIMER6EOI__RSVD_TIMER6EOI__MASK                                 0xFFFFFFFE


/*-------------------------------------------------------------------------------------*/
/*  TIMER6INTSTAT : TIMER6INTSTAT       */
#define TIMER6INTSTAT__TIMER6INTSTAT__SHIFT                             0x00000000
#define TIMER6INTSTAT__TIMER6INTSTAT__WIDTH                             0x00000001
#define TIMER6INTSTAT__TIMER6INTSTAT__MASK                              0x00000001


/*  TIMER6INTSTAT : RSVD_TIMER6INTSTAT  */
#define TIMER6INTSTAT__RSVD_TIMER6INTSTAT__SHIFT                        0x00000001
#define TIMER6INTSTAT__RSVD_TIMER6INTSTAT__WIDTH                        0x0000001F
#define TIMER6INTSTAT__RSVD_TIMER6INTSTAT__MASK                         0xFFFFFFFE


/*-------------------------------------------------------------------------------------*/
/*  TIMER7LOADCOUNT : TIMER7LOADCOUNT     */
#define TIMER7LOADCOUNT__TIMER7LOADCOUNT__SHIFT                         0x00000000
#define TIMER7LOADCOUNT__TIMER7LOADCOUNT__WIDTH                         0x00000020
#define TIMER7LOADCOUNT__TIMER7LOADCOUNT__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER7CURRENTVAL : TIMER7CURRENTVAL    */
#define TIMER7CURRENTVAL__TIMER7CURRENTVAL__SHIFT                       0x00000000
#define TIMER7CURRENTVAL__TIMER7CURRENTVAL__WIDTH                       0x00000020
#define TIMER7CURRENTVAL__TIMER7CURRENTVAL__MASK                        0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER7CONTROLREG : TIMER_ENABLE        */
#define TIMER7CONTROLREG__TIMER_ENABLE__SHIFT                           0x00000000
#define TIMER7CONTROLREG__TIMER_ENABLE__WIDTH                           0x00000001
#define TIMER7CONTROLREG__TIMER_ENABLE__MASK                            0x00000001


/*  TIMER7CONTROLREG : TIMER_MODE          */
#define TIMER7CONTROLREG__TIMER_MODE__SHIFT                             0x00000001
#define TIMER7CONTROLREG__TIMER_MODE__WIDTH                             0x00000001
#define TIMER7CONTROLREG__TIMER_MODE__MASK                              0x00000002


/*  TIMER7CONTROLREG : TIMER_INTERRUPT_MASK*/
#define TIMER7CONTROLREG__TIMER_INTERRUPT_MASK__SHIFT                   0x00000002
#define TIMER7CONTROLREG__TIMER_INTERRUPT_MASK__WIDTH                   0x00000001
#define TIMER7CONTROLREG__TIMER_INTERRUPT_MASK__MASK                    0x00000004


/*  TIMER7CONTROLREG : TIMER_PWM           */
#define TIMER7CONTROLREG__TIMER_PWM__SHIFT                              0x00000003
#define TIMER7CONTROLREG__TIMER_PWM__WIDTH                              0x00000001
#define TIMER7CONTROLREG__TIMER_PWM__MASK                               0x00000008


/*  TIMER7CONTROLREG : RSVD_TIMER_0N100PWM_EN*/
#define TIMER7CONTROLREG__RSVD_TIMER_0N100PWM_EN__SHIFT                 0x00000004
#define TIMER7CONTROLREG__RSVD_TIMER_0N100PWM_EN__WIDTH                 0x00000001
#define TIMER7CONTROLREG__RSVD_TIMER_0N100PWM_EN__MASK                  0x00000010

/*  TIMER7CONTROLREG : RSVD_TIMER7CONTROLREG*/
#define TIMER7CONTROLREG__RSVD_TIMER7CONTROLREG__SHIFT                  0x00000005
#define TIMER7CONTROLREG__RSVD_TIMER7CONTROLREG__WIDTH                  0x0000001B
#define TIMER7CONTROLREG__RSVD_TIMER7CONTROLREG__MASK                   0xFFFFFFE0


/*-------------------------------------------------------------------------------------*/
/*  TIMER7EOI : TIMER7EOI           */
#define TIMER7EOI__TIMER7EOI__SHIFT                                     0x00000000
#define TIMER7EOI__TIMER7EOI__WIDTH                                     0x00000001
#define TIMER7EOI__TIMER7EOI__MASK                                      0x00000001

/*  TIMER7EOI : RSVD_TIMER7EOI      */
#define TIMER7EOI__RSVD_TIMER7EOI__SHIFT                                0x00000001
#define TIMER7EOI__RSVD_TIMER7EOI__WIDTH                                0x0000001F
#define TIMER7EOI__RSVD_TIMER7EOI__MASK                                 0xFFFFFFFE


/*-------------------------------------------------------------------------------------*/
/*  TIMER7INTSTAT : TIMER7INTSTAT       */
#define TIMER7INTSTAT__TIMER7INTSTAT__SHIFT                             0x00000000
#define TIMER7INTSTAT__TIMER7INTSTAT__WIDTH                             0x00000001
#define TIMER7INTSTAT__TIMER7INTSTAT__MASK                              0x00000001


/*  TIMER7INTSTAT : RSVD_TIMER7INTSTAT  */
#define TIMER7INTSTAT__RSVD_TIMER7INTSTAT__SHIFT                        0x00000001
#define TIMER7INTSTAT__RSVD_TIMER7INTSTAT__WIDTH                        0x0000001F
#define TIMER7INTSTAT__RSVD_TIMER7INTSTAT__MASK                         0xFFFFFFFE


/*-------------------------------------------------------------------------------------*/
/*  TIMER8LOADCOUNT : TIMER8LOADCOUNT     */
#define TIMER8LOADCOUNT__TIMER8LOADCOUNT__SHIFT                         0x00000000
#define TIMER8LOADCOUNT__TIMER8LOADCOUNT__WIDTH                         0x00000020
#define TIMER8LOADCOUNT__TIMER8LOADCOUNT__MASK                          0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER8CURRENTVAL : TIMER8CURRENTVAL    */
#define TIMER8CURRENTVAL__TIMER8CURRENTVAL__SHIFT                       0x00000000
#define TIMER8CURRENTVAL__TIMER8CURRENTVAL__WIDTH                       0x00000020
#define TIMER8CURRENTVAL__TIMER8CURRENTVAL__MASK                        0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER8CONTROLREG : TIMER_ENABLE        */
#define TIMER8CONTROLREG__TIMER_ENABLE__SHIFT                           0x00000000
#define TIMER8CONTROLREG__TIMER_ENABLE__WIDTH                           0x00000001
#define TIMER8CONTROLREG__TIMER_ENABLE__MASK                            0x00000001


/*  TIMER8CONTROLREG : TIMER_MODE          */
#define TIMER8CONTROLREG__TIMER_MODE__SHIFT                             0x00000001
#define TIMER8CONTROLREG__TIMER_MODE__WIDTH                             0x00000001
#define TIMER8CONTROLREG__TIMER_MODE__MASK                              0x00000002


/*  TIMER8CONTROLREG : TIMER_INTERRUPT_MASK*/
#define TIMER8CONTROLREG__TIMER_INTERRUPT_MASK__SHIFT                   0x00000002
#define TIMER8CONTROLREG__TIMER_INTERRUPT_MASK__WIDTH                   0x00000001
#define TIMER8CONTROLREG__TIMER_INTERRUPT_MASK__MASK                    0x00000004


/*  TIMER8CONTROLREG : TIMER_PWM           */
#define TIMER8CONTROLREG__TIMER_PWM__SHIFT                              0x00000003
#define TIMER8CONTROLREG__TIMER_PWM__WIDTH                              0x00000001
#define TIMER8CONTROLREG__TIMER_PWM__MASK                               0x00000008


/*  TIMER8CONTROLREG : RSVD_TIMER_0N100PWM_EN*/
#define TIMER8CONTROLREG__RSVD_TIMER_0N100PWM_EN__SHIFT                 0x00000004
#define TIMER8CONTROLREG__RSVD_TIMER_0N100PWM_EN__WIDTH                 0x00000001
#define TIMER8CONTROLREG__RSVD_TIMER_0N100PWM_EN__MASK                  0x00000010

/*  TIMER8CONTROLREG : RSVD_TIMER8CONTROLREG*/
#define TIMER8CONTROLREG__RSVD_TIMER8CONTROLREG__SHIFT                  0x00000005
#define TIMER8CONTROLREG__RSVD_TIMER8CONTROLREG__WIDTH                  0x0000001B
#define TIMER8CONTROLREG__RSVD_TIMER8CONTROLREG__MASK                   0xFFFFFFE0


/*-------------------------------------------------------------------------------------*/
/*  TIMER8EOI : TIMER8EOI           */
#define TIMER8EOI__TIMER8EOI__SHIFT                                     0x00000000
#define TIMER8EOI__TIMER8EOI__WIDTH                                     0x00000001
#define TIMER8EOI__TIMER8EOI__MASK                                      0x00000001

/*  TIMER8EOI : RSVD_TIMER8EOI      */
#define TIMER8EOI__RSVD_TIMER8EOI__SHIFT                                0x00000001
#define TIMER8EOI__RSVD_TIMER8EOI__WIDTH                                0x0000001F
#define TIMER8EOI__RSVD_TIMER8EOI__MASK                                 0xFFFFFFFE


/*-------------------------------------------------------------------------------------*/
/*  TIMER8INTSTAT : TIMER8INTSTAT       */
#define TIMER8INTSTAT__TIMER8INTSTAT__SHIFT                             0x00000000
#define TIMER8INTSTAT__TIMER8INTSTAT__WIDTH                             0x00000001
#define TIMER8INTSTAT__TIMER8INTSTAT__MASK                              0x00000001


/*  TIMER8INTSTAT : RSVD_TIMER8INTSTAT  */
#define TIMER8INTSTAT__RSVD_TIMER8INTSTAT__SHIFT                        0x00000001
#define TIMER8INTSTAT__RSVD_TIMER8INTSTAT__WIDTH                        0x0000001F
#define TIMER8INTSTAT__RSVD_TIMER8INTSTAT__MASK                         0xFFFFFFFE


/*-------------------------------------------------------------------------------------*/
/*  TIMERSINTSTATUS : TimersIntStatus     */
#define TIMERSINTSTATUS__TIMERSINTSTATUS__SHIFT                         0x00000000
#define TIMERSINTSTATUS__TIMERSINTSTATUS__WIDTH                         0x00000008
#define TIMERSINTSTATUS__TIMERSINTSTATUS__MASK                          0x000000FF


/*  TIMERSINTSTATUS : RSVD_TimersIntStatus*/
#define TIMERSINTSTATUS__RSVD_TIMERSINTSTATUS__SHIFT                    0x00000008
#define TIMERSINTSTATUS__RSVD_TIMERSINTSTATUS__WIDTH                    0x00000018
#define TIMERSINTSTATUS__RSVD_TIMERSINTSTATUS__MASK                     0xFFFFFF00


/*-------------------------------------------------------------------------------------*/
/*  TIMERSEOI : TIMERSEOI           */
#define TIMERSEOI__TIMERSEOI__SHIFT                                     0x00000000
#define TIMERSEOI__TIMERSEOI__WIDTH                                     0x00000008
#define TIMERSEOI__TIMERSEOI__MASK                                      0x000000FF

/*  TIMERSEOI : RSVD_TIMERSEOI      */
#define TIMERSEOI__RSVD_TIMERSEOI__SHIFT                                0x00000008
#define TIMERSEOI__RSVD_TIMERSEOI__WIDTH                                0x00000018
#define TIMERSEOI__RSVD_TIMERSEOI__MASK                                 0xFFFFFF00


/*-------------------------------------------------------------------------------------*/
/*  TIMERSRAWINTSTATUS : TIMERSRAWINTSTAT    */
#define TIMERSRAWINTSTATUS__TIMERSRAWINTSTAT__SHIFT                     0x00000000
#define TIMERSRAWINTSTATUS__TIMERSRAWINTSTAT__WIDTH                     0x00000008
#define TIMERSRAWINTSTATUS__TIMERSRAWINTSTAT__MASK                      0x000000FF


/*  TIMERSRAWINTSTATUS : RSVD_TIMERSRAWINTSTAT*/
#define TIMERSRAWINTSTATUS__RSVD_TIMERSRAWINTSTAT__SHIFT                0x00000008
#define TIMERSRAWINTSTATUS__RSVD_TIMERSRAWINTSTAT__WIDTH                0x00000018
#define TIMERSRAWINTSTATUS__RSVD_TIMERSRAWINTSTAT__MASK                 0xFFFFFF00


/*-------------------------------------------------------------------------------------*/
/*  TIMERS_COMP_VERSION : TIMERSCOMPVERSION   */
#define TIMERS_COMP_VERSION__TIMERSCOMPVERSION__SHIFT                   0x00000000
#define TIMERS_COMP_VERSION__TIMERSCOMPVERSION__WIDTH                   0x00000020
#define TIMERS_COMP_VERSION__TIMERSCOMPVERSION__MASK                    0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER1LOADCOUNT2 : TIMERNLOADCOUNT2    */
#define TIMER1LOADCOUNT2__TIMERNLOADCOUNT2__SHIFT                       0x00000000
#define TIMER1LOADCOUNT2__TIMERNLOADCOUNT2__WIDTH                       0x00000020
#define TIMER1LOADCOUNT2__TIMERNLOADCOUNT2__MASK                        0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER2LOADCOUNT2 : TIMER2LOADCOUNT2    */
#define TIMER2LOADCOUNT2__TIMER2LOADCOUNT2__SHIFT                       0x00000000
#define TIMER2LOADCOUNT2__TIMER2LOADCOUNT2__WIDTH                       0x00000020
#define TIMER2LOADCOUNT2__TIMER2LOADCOUNT2__MASK                        0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER3LOADCOUNT2 : TIMER3LOADCOUNT2    */
#define TIMER3LOADCOUNT2__TIMER3LOADCOUNT2__SHIFT                       0x00000000
#define TIMER3LOADCOUNT2__TIMER3LOADCOUNT2__WIDTH                       0x00000020
#define TIMER3LOADCOUNT2__TIMER3LOADCOUNT2__MASK                        0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER4LOADCOUNT2 : TIMER4LOADCOUNT2    */
#define TIMER4LOADCOUNT2__TIMER4LOADCOUNT2__SHIFT                       0x00000000
#define TIMER4LOADCOUNT2__TIMER4LOADCOUNT2__WIDTH                       0x00000020
#define TIMER4LOADCOUNT2__TIMER4LOADCOUNT2__MASK                        0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER5LOADCOUNT2 : TIMER5LOADCOUNT2    */
#define TIMER5LOADCOUNT2__TIMER5LOADCOUNT2__SHIFT                       0x00000000
#define TIMER5LOADCOUNT2__TIMER5LOADCOUNT2__WIDTH                       0x00000020
#define TIMER5LOADCOUNT2__TIMER5LOADCOUNT2__MASK                        0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER6LOADCOUNT2 : TIMER6LOADCOUNT2    */
#define TIMER6LOADCOUNT2__TIMER6LOADCOUNT2__SHIFT                       0x00000000
#define TIMER6LOADCOUNT2__TIMER6LOADCOUNT2__WIDTH                       0x00000020
#define TIMER6LOADCOUNT2__TIMER6LOADCOUNT2__MASK                        0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER7LOADCOUNT2 : TIMER7LOADCOUNT2    */
#define TIMER7LOADCOUNT2__TIMER7LOADCOUNT2__SHIFT                       0x00000000
#define TIMER7LOADCOUNT2__TIMER7LOADCOUNT2__WIDTH                       0x00000020
#define TIMER7LOADCOUNT2__TIMER7LOADCOUNT2__MASK                        0xFFFFFFFF


/*-------------------------------------------------------------------------------------*/
/*  TIMER8LOADCOUNT2 : TIMER8LOADCOUNT2    */
#define TIMER8LOADCOUNT2__TIMER8LOADCOUNT2__SHIFT                       0x00000000
#define TIMER8LOADCOUNT2__TIMER8LOADCOUNT2__WIDTH                       0x00000020
#define TIMER8LOADCOUNT2__TIMER8LOADCOUNT2__MASK                        0xFFFFFFFF




#endif /* __DW_APB_TIMERS_MACRO_H */

