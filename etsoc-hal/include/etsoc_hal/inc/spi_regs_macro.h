#ifndef __REG_SPI_REGS_MACRO_H__
#define __REG_SPI_REGS_MACRO_H__

/*macros for reg offset */
#define SPI_CTRLR0_OFFSET                                                                   0x00000000U
#define SPI_CTRLR1_OFFSET                                                                   0x00000004U
#define SPI_SSIENR_OFFSET                                                                   0x00000008U
#define SPI_MWCR_OFFSET                                                                     0x0000000CU
#define SPI_SER_OFFSET                                                                      0x00000010U
#define SPI_BAUDR_OFFSET                                                                    0x00000014U
#define SPI_TXFTLR_OFFSET                                                                   0x00000018U
#define SPI_RXFTLR_OFFSET                                                                   0x0000001CU
#define SPI_TXFLR_OFFSET                                                                    0x00000020U
#define SPI_RXFLR_OFFSET                                                                    0x00000024U
#define SPI_SR_OFFSET                                                                       0x00000028U
#define SPI_IMR_OFFSET                                                                      0x0000002CU
#define SPI_ISR_OFFSET                                                                      0x00000030U
#define SPI_RISR_OFFSET                                                                     0x00000034U
#define SPI_TXOICR_OFFSET                                                                   0x00000038U
#define SPI_RXOICR_OFFSET                                                                   0x0000003CU
#define SPI_RXUICR_OFFSET                                                                   0x00000040U
#define SPI_MSTICR_OFFSET                                                                   0x40000004U
#define SPI_ICR_OFFSET                                                                      0x40000008U
#define SPI_DMACR_OFFSET                                                                    0x0000004CU
#define SPI_DMATDLR_OFFSET                                                                  0x00000050U
#define SPI_DMARDLR_OFFSET                                                                  0x00000054U
#define SPI_IDR_OFFSET                                                                      0x00000058U
#define SPI_SSI_VERSIO_ID_OFFSET                                                            0x0000005CU
#define SPI_DR_OFFSET                                                                       0x00000060U
#define SPI_RX_SAMPLE_DLY_OFFSET                                                            0x000000F0U
#define SPI_SPI_CRTLR0_OFFSET                                                               0x000000F4U
#define SPI_TXD_DRIVE_EDGE_OFFSET                                                           0x000000F8U
#define SPI_RSVD_OFFSET                                                                     0x000000FCU


/* macros for spi_ctrlr0_reg */
#ifndef __SPI_CTRLR0_REG_MACRO__
#define __SPI_CTRLR0_REG_MACRO__

/* macros for field DFS */
#define SPI_CTRLR0_REG__DFS__SHIFT                                                          0
#define SPI_CTRLR0_REG__DFS__WIDTH                                                          4
#define SPI_CTRLR0_REG__DFS__MASK                                                           0x0000000FU
#define SPI_CTRLR0_REG__DFS__READ(src)                                                      (((uint32_t)(src) & SPI_CTRLR0_REG__DFS__MASK) >> SPI_CTRLR0_REG__DFS__SHIFT)
#define SPI_CTRLR0_REG__DFS__WRITE(src)                                                     (((uint32_t)(src) << SPI_CTRLR0_REG__DFS__SHIFT) & SPI_CTRLR0_REG__DFS__MASK)

/* macros for field FRF */
#define SPI_CTRLR0_REG__FRF__SHIFT                                                          4
#define SPI_CTRLR0_REG__FRF__WIDTH                                                          2
#define SPI_CTRLR0_REG__FRF__MASK                                                           0x00000030U
#define SPI_CTRLR0_REG__FRF__READ(src)                                                      (((uint32_t)(src) & SPI_CTRLR0_REG__FRF__MASK) >> SPI_CTRLR0_REG__FRF__SHIFT)
#define SPI_CTRLR0_REG__FRF__WRITE(src)                                                     (((uint32_t)(src) << SPI_CTRLR0_REG__FRF__SHIFT) & SPI_CTRLR0_REG__FRF__MASK)

/* macros for field SCHP */
#define SPI_CTRLR0_REG__SCHP__SHIFT                                                         6
#define SPI_CTRLR0_REG__SCHP__WIDTH                                                         1
#define SPI_CTRLR0_REG__SCHP__MASK                                                          0x00000040U
#define SPI_CTRLR0_REG__SCHP__READ(src)                                                     (((uint32_t)(src) & SPI_CTRLR0_REG__SCHP__MASK) >> SPI_CTRLR0_REG__SCHP__SHIFT)
#define SPI_CTRLR0_REG__SCHP__WRITE(src)                                                    (((uint32_t)(src) << SPI_CTRLR0_REG__SCHP__SHIFT) & SPI_CTRLR0_REG__SCHP__MASK)

/* macros for field SCPOL */
#define SPI_CTRLR0_REG__SCPOL__SHIFT                                                        7
#define SPI_CTRLR0_REG__SCPOL__WIDTH                                                        1
#define SPI_CTRLR0_REG__SCPOL__MASK                                                         0x00000080U
#define SPI_CTRLR0_REG__SCPOL__READ(src)                                                    (((uint32_t)(src) & SPI_CTRLR0_REG__SCPOL__MASK) >> SPI_CTRLR0_REG__SCPOL__SHIFT)
#define SPI_CTRLR0_REG__SCPOL__WRITE(src)                                                   (((uint32_t)(src) << SPI_CTRLR0_REG__SCPOL__SHIFT) & SPI_CTRLR0_REG__SCPOL__MASK)

/* macros for field TMOD */
#define SPI_CTRLR0_REG__TMOD__SHIFT                                                         8
#define SPI_CTRLR0_REG__TMOD__WIDTH                                                         2
#define SPI_CTRLR0_REG__TMOD__MASK                                                          0x00000300U
#define SPI_CTRLR0_REG__TMOD__READ(src)                                                     (((uint32_t)(src) & SPI_CTRLR0_REG__TMOD__MASK) >> SPI_CTRLR0_REG__TMOD__SHIFT)
#define SPI_CTRLR0_REG__TMOD__WRITE(src)                                                    (((uint32_t)(src) << SPI_CTRLR0_REG__TMOD__SHIFT) & SPI_CTRLR0_REG__TMOD__MASK)

/* macros for field SLV_OE */
#define SPI_CTRLR0_REG__SLV_OE__SHIFT                                                       10
#define SPI_CTRLR0_REG__SLV_OE__WIDTH                                                       1
#define SPI_CTRLR0_REG__SLV_OE__MASK                                                        0x00000400U
#define SPI_CTRLR0_REG__SLV_OE__READ(src)                                                   (((uint32_t)(src) & SPI_CTRLR0_REG__SLV_OE__MASK) >> SPI_CTRLR0_REG__SLV_OE__SHIFT)
#define SPI_CTRLR0_REG__SLV_OE__WRITE(src)                                                  (((uint32_t)(src) << SPI_CTRLR0_REG__SLV_OE__SHIFT) & SPI_CTRLR0_REG__SLV_OE__MASK)

/* macros for field SRL */
#define SPI_CTRLR0_REG__SRL__SHIFT                                                          11
#define SPI_CTRLR0_REG__SRL__WIDTH                                                          1
#define SPI_CTRLR0_REG__SRL__MASK                                                           0x00000800U
#define SPI_CTRLR0_REG__SRL__READ(src)                                                      (((uint32_t)(src) & SPI_CTRLR0_REG__SRL__MASK) >> SPI_CTRLR0_REG__SRL__SHIFT)
#define SPI_CTRLR0_REG__SRL__WRITE(src)                                                     (((uint32_t)(src) << SPI_CTRLR0_REG__SRL__SHIFT) & SPI_CTRLR0_REG__SRL__MASK)

/* macros for field CFS */
#define SPI_CTRLR0_REG__CFS__SHIFT                                                          12
#define SPI_CTRLR0_REG__CFS__WIDTH                                                          4
#define SPI_CTRLR0_REG__CFS__MASK                                                           0x0000F000U
#define SPI_CTRLR0_REG__CFS__READ(src)                                                      (((uint32_t)(src) & SPI_CTRLR0_REG__CFS__MASK) >> SPI_CTRLR0_REG__CFS__SHIFT)
#define SPI_CTRLR0_REG__CFS__WRITE(src)                                                     (((uint32_t)(src) << SPI_CTRLR0_REG__CFS__SHIFT) & SPI_CTRLR0_REG__CFS__MASK)

/* macros for field DFS_32 */
#define SPI_CTRLR0_REG__DFS_32__SHIFT                                                       16
#define SPI_CTRLR0_REG__DFS_32__WIDTH                                                       5
#define SPI_CTRLR0_REG__DFS_32__MASK                                                        0x001F0000U
#define SPI_CTRLR0_REG__DFS_32__READ(src)                                                   (((uint32_t)(src) & SPI_CTRLR0_REG__DFS_32__MASK) >> SPI_CTRLR0_REG__DFS_32__SHIFT)
#define SPI_CTRLR0_REG__DFS_32__WRITE(src)                                                  (((uint32_t)(src) << SPI_CTRLR0_REG__DFS_32__SHIFT) & SPI_CTRLR0_REG__DFS_32__MASK)

/* macros for field SPI_FRF */
#define SPI_CTRLR0_REG__SPI_FRF__SHIFT                                                      21
#define SPI_CTRLR0_REG__SPI_FRF__WIDTH                                                      2
#define SPI_CTRLR0_REG__SPI_FRF__MASK                                                       0x00600000U
#define SPI_CTRLR0_REG__SPI_FRF__READ(src)                                                  (((uint32_t)(src) & SPI_CTRLR0_REG__SPI_FRF__MASK) >> SPI_CTRLR0_REG__SPI_FRF__SHIFT)
#define SPI_CTRLR0_REG__SPI_FRF__WRITE(src)                                                 (((uint32_t)(src) << SPI_CTRLR0_REG__SPI_FRF__SHIFT) & SPI_CTRLR0_REG__SPI_FRF__MASK)

/* macros for field RSVD_CTRLR0_23 */
#define SPI_CTRLR0_REG__RSVD_CTRLR0_23__SHIFT                                               23
#define SPI_CTRLR0_REG__RSVD_CTRLR0_23__WIDTH                                               1
#define SPI_CTRLR0_REG__RSVD_CTRLR0_23__MASK                                                0x00800000U
#define SPI_CTRLR0_REG__RSVD_CTRLR0_23__READ(src)                                           (((uint32_t)(src) & SPI_CTRLR0_REG__RSVD_CTRLR0_23__MASK) >> SPI_CTRLR0_REG__RSVD_CTRLR0_23__SHIFT)
#define SPI_CTRLR0_REG__RSVD_CTRLR0_23__WRITE(src)                                          (((uint32_t)(src) << SPI_CTRLR0_REG__RSVD_CTRLR0_23__SHIFT) & SPI_CTRLR0_REG__RSVD_CTRLR0_23__MASK)

/* macros for field SSTE */
#define SPI_CTRLR0_REG__SSTE__SHIFT                                                         24
#define SPI_CTRLR0_REG__SSTE__WIDTH                                                         1
#define SPI_CTRLR0_REG__SSTE__MASK                                                          0x01000000U
#define SPI_CTRLR0_REG__SSTE__READ(src)                                                     (((uint32_t)(src) & SPI_CTRLR0_REG__SSTE__MASK) >> SPI_CTRLR0_REG__SSTE__SHIFT)
#define SPI_CTRLR0_REG__SSTE__WRITE(src)                                                    (((uint32_t)(src) << SPI_CTRLR0_REG__SSTE__SHIFT) & SPI_CTRLR0_REG__SSTE__MASK)

/* macros for field RSVD_CTRLR0 */
#define SPI_CTRLR0_REG__RSVD_CTRLR0__SHIFT                                                  25
#define SPI_CTRLR0_REG__RSVD_CTRLR0__WIDTH                                                  7
#define SPI_CTRLR0_REG__RSVD_CTRLR0__MASK                                                   0xFE000000U
#define SPI_CTRLR0_REG__RSVD_CTRLR0__READ(src)                                              (((uint32_t)(src) & SPI_CTRLR0_REG__RSVD_CTRLR0__MASK) >> SPI_CTRLR0_REG__RSVD_CTRLR0__SHIFT)
#define SPI_CTRLR0_REG__RSVD_CTRLR0__WRITE(src)                                             (((uint32_t)(src) << SPI_CTRLR0_REG__RSVD_CTRLR0__SHIFT) & SPI_CTRLR0_REG__RSVD_CTRLR0__MASK)

#endif /* __SPI_CTRLR0_REG_MACRO__ */

/* macros for spi_ctrlr1_reg */
#ifndef __SPI_CTRLR1_REG_MACRO__
#define __SPI_CTRLR1_REG_MACRO__

/* macros for field NDF */
#define SPI_CTRLR1_REG__NDF__SHIFT                                                          0
#define SPI_CTRLR1_REG__NDF__WIDTH                                                          16
#define SPI_CTRLR1_REG__NDF__MASK                                                           0x0000FFFFU
#define SPI_CTRLR1_REG__NDF__READ(src)                                                      (((uint32_t)(src) & SPI_CTRLR1_REG__NDF__MASK) >> SPI_CTRLR1_REG__NDF__SHIFT)
#define SPI_CTRLR1_REG__NDF__WRITE(src)                                                     (((uint32_t)(src) << SPI_CTRLR1_REG__NDF__SHIFT) & SPI_CTRLR1_REG__NDF__MASK)

/* macros for field RSVD_CTRLR1 */
#define SPI_CTRLR1_REG__RSVD_CTRLR1__SHIFT                                                  16
#define SPI_CTRLR1_REG__RSVD_CTRLR1__WIDTH                                                  16
#define SPI_CTRLR1_REG__RSVD_CTRLR1__MASK                                                   0xFFFF0000U
#define SPI_CTRLR1_REG__RSVD_CTRLR1__READ(src)                                              (((uint32_t)(src) & SPI_CTRLR1_REG__RSVD_CTRLR1__MASK) >> SPI_CTRLR1_REG__RSVD_CTRLR1__SHIFT)
#define SPI_CTRLR1_REG__RSVD_CTRLR1__WRITE(src)                                             (((uint32_t)(src) << SPI_CTRLR1_REG__RSVD_CTRLR1__SHIFT) & SPI_CTRLR1_REG__RSVD_CTRLR1__MASK)

#endif /* __SPI_CTRLR1_REG_MACRO__ */

/* macros for spi_ssienr_reg */
#ifndef __SPI_SSIENR_REG_MACRO__
#define __SPI_SSIENR_REG_MACRO__

/* macros for field SSI_EN */
#define SPI_SSIENR_REG__SSI_EN__SHIFT                                                       0
#define SPI_SSIENR_REG__SSI_EN__WIDTH                                                       1
#define SPI_SSIENR_REG__SSI_EN__MASK                                                        0x00000001U
#define SPI_SSIENR_REG__SSI_EN__READ(src)                                                   (((uint32_t)(src) & SPI_SSIENR_REG__SSI_EN__MASK) >> SPI_SSIENR_REG__SSI_EN__SHIFT)
#define SPI_SSIENR_REG__SSI_EN__WRITE(src)                                                  (((uint32_t)(src) << SPI_SSIENR_REG__SSI_EN__SHIFT) & SPI_SSIENR_REG__SSI_EN__MASK)

/* macros for field RSVD_SSIENR */
#define SPI_SSIENR_REG__RSVD_SSIENR__SHIFT                                                  1
#define SPI_SSIENR_REG__RSVD_SSIENR__WIDTH                                                  31
#define SPI_SSIENR_REG__RSVD_SSIENR__MASK                                                   0xFFFFFFFEU
#define SPI_SSIENR_REG__RSVD_SSIENR__READ(src)                                              (((uint32_t)(src) & SPI_SSIENR_REG__RSVD_SSIENR__MASK) >> SPI_SSIENR_REG__RSVD_SSIENR__SHIFT)
#define SPI_SSIENR_REG__RSVD_SSIENR__WRITE(src)                                             ((uint32_t)(src) << SPI_SSIENR_REG__RSVD_SSIENR__SHIFT) & SPI_SSIENR_REG__RSVD_SSIENR__MASK)

#endif /* __SPI_SSIENR_REG_MACRO__ */

/* macros for spi_mwcr_reg */
#ifndef __SPI_MWCR_REG_MACRO__
#define __SPI_MWCR_REG_MACRO__

/* macros for field MWMOD */							
#define SPI_MWCR_REG__MWMOD__SHIFT                                                          0
#define SPI_MWCR_REG__MWMOD__WIDTH                                                          1
#define SPI_MWCR_REG__MWMOD__MASK                                                           0x00000001U
#define SPI_MWCR_REG__MWMOD__READ(src)                                                      (((uint32_t)(src) & SPI_MWCR_REG__MWMOD__MASK) >> SPI_MWCR_REG__MWMOD__SHIFT)
#define SPI_MWCR_REG__MWMOD__WRITE(src)                                                     (((uint32_t)(src) << SPI_MWCR_REG__MWMOD__SHIFT) & SPI_MWCR_REG__MWMOD__MASK)

/* macros for field MDD */
#define SPI_MWCR_REG__MDD__SHIFT                                                            1
#define SPI_MWCR_REG__MDD__WIDTH                                                            1
#define SPI_MWCR_REG__MDD__MASK                                                             0x00000002U
#define SPI_MWCR_REG__MDD__READ(src)                                                        (((uint32_t)(src) & SPI_MWCR_REG__MDD__MASK) >> SPI_MWCR_REG__MDD__SHIFT)
#define SPI_MWCR_REG__MDD__WRITE(src)                                                       (((uint32_t)(src) << SPI_MWCR_REG__MDD__SHIFT) & SPI_MWCR_REG__MDD__MASK)

/* macros for field MHS */
#define SPI_MWCR_REG__MHS__SHIFT                                                            2
#define SPI_MWCR_REG__MHS__WIDTH                                                            1
#define SPI_MWCR_REG__MHS__MASK                                                             0x00000004U
#define SPI_MWCR_REG__MHS__READ(src)                                                        (((uint32_t)(src) & SPI_MWCR_REG__MHS__MASK) >> SPI_MWCR_REG__MHS__SHIFT)
#define SPI_MWCR_REG__MHS__WRITE(src)                                                       (((uint32_t)(src) << SPI_MWCR_REG__MHS__SHIFT) & SPI_MWCR_REG__MHS__MASK)

/* macros for field RSVD_MWCR */
#define SPI_MWCR_REG__RSVD_MWCR__SHIFT                                                      3
#define SPI_MWCR_REG__RSVD_MWCR__WIDTH                                                      29
#define SPI_MWCR_REG__RSVD_MWCR__MASK                                                       0xFFFFFFF8U
#define SPI_MWCR_REG__RSVD_MWCR__READ(src)                                                  (((uint32_t)(src) & SPI_MWCR_REG__RSVD_MWCR__MASK) >> SPI_MWCR_REG__RSVD_MWCR__SHIFT)
#define SPI_MWCR_REG__RSVD_MWCR__WRITE(src)                                                 (((uint32_t)(src) << SPI_MWCR_REG__RSVD_MWCR__SHIFT) & SPI_MWCR_REG__RSVD_MWCR__MASK)

#endif /* __SPI_SSIENR_REG_MACRO__ */

/* macros for spi_ser_reg */
#ifndef __SPI_SER_REG_MACRO__
#define __SPI_SER_REG_MACRO__

/* macros for field SER */
#define SPI_SER_REG__SER__SHIFT                                                             0
#define SPI_SER_REG__SER__WIDTH(src)                                                        (src)
#define SPI_SER_REG__SER__MASK(src)                                                         ~(~(0x00000001U << (SPI_SER_REG__SER__WIDTH +1)) + 1)
#define SPI_SER_REG__SER__READ(src)                                                         (((uint32_t)(src) & SPI_SER_REG__SER__MASK) >> SPI_SER_REG__SER__SHIFT)
#define SPI_SER_REG__SER__WRITE(src)                                                        (((uint32_t)(src) << SPI_SER_REG__SER__SHIFT) & SPI_SER_REG__SER__MASK)

/* macros for field RSVD_SER */
#define SPI_SER_REG__RSVD_SER__SHIFT                                                        SPI_SER_REG__SER__WIDTH
#define SPI_SER_REG__RSVD_SER__WIDTH                                                        32 - SPI_SER_REG__SER__WIDTH
#define SPI_SER_REG__RSVD_SER__MASK                                                         ~SPI_SER_REG__SER__MASK
#define SPI_SER_REG__RSVD_SER__READ(src)                                                    (((uint32_t)(src) & SPI_SER_REG__RSVD_SER__MASK) >> SPI_SER_REG__RSVD_SER__SHIFT)
#define SPI_SER_REG__RSVD_SER__WRITE(src)                                                   (((uint32_t)(src) << SPI_SER_REG__RSVD_SER__SHIFT) & SPI_SER_REG__RSVD_SER__MASK)

#endif /* __SPI_SER_REG_MACRO__ */

/* macros for spi_baudr_reg */
#ifndef __SPI_BAUDR_REG_MACRO__
#define __SPI_BAUDR_REG_MACRO__

/* macros for field SCKDV */
#define SPI_BAUDR_REG__SCKDV__SHIFT                                                         0
#define SPI_BAUDR_REG__SCKDV__WIDTH                                                         16
#define SPI_BAUDR_REG__SCKDV__MASK                                                          0x0000FFFFU
#define SPI_BAUDR_REG__SCKDV__READ(src)	                                                    (((uint32_t)(src) & SPI_BAUDR_REG__SCKDV__MASK) >> SPI_BAUDR_REG__SCKDV__SHIFT)
#define SPI_BAUDR_REG__SCKDV__WRITE(src)                                                    (((uint32_t)(src) << SPI_BAUDR_REG__SCKDV__SHIFT) & SPI_BAUDR_REG__SCKDV__MASK)

/* macros for field RSVD_BAUDR */
#define SPI_BAUDR_REG__RSVD_BAUDR__SHIFT                                                    16
#define SPI_BAUDR_REG__RSVD_BAUDR__WIDTH                                                    16
#define SPI_BAUDR_REG__RSVD_BAUDR__MASK                                                     0x0000FFFFU
#define SPI_BAUDR_REG__RSVD_BAUDR__READ(src)                                                (((uint32_t)(src) & SPI_BAUDR_REG__RSVD_BAUDR__MASK) >> SPI_BAUDR_REG__RSVD_BAUDR__SHIFT)
#define SPI_BAUDR_REG__RSVD_BAUDR__WRITE(src)                                               (((uint32_t)(src) << SPI_BAUDR_REG__RSVD_BAUDR__SHIFT) & SPI_BAUDR_REG__RSVD_BAUDR__MASK)

#endif /* __SPI_BAUDR_REG_MACRO__ */

/* macros for spi_txftlr_reg */
#ifndef __SPI_TXFTLR_REG_MACRO__
#define __SPI_TXFTLR_REG_MACRO__

/* macros for field TXFTLR */
#define SPI_TXFTLR_REG__TFT__SHIFT                                                          0
#define SPI_TXFTLR_REG__TFT__WIDTH(src)                                                     (src)
#define SPI_TXFTLR_REG__TFT__MASK(src)                                                      ~(~(0x00000001U << (SPI_TXFTLR_REG__TFT__WIDTH +1)) + 1)
#define SPI_TXFTLR_REG__TFT__READ(src)	                                                    ((uint32_t)(src) & SPI_TXFTLR_REG__TFT__MASK) >> SPI_TXFTLR_REG__TFT__SHIFT)
#define SPI_TXFTLR_REG__TFT__WRITE(src)                                                     (((uint32_t)(src) << SPI_TXFTLR_REG__TFT__SHIFT) & SPI_TXFTLR_REG__TFT__MASK)

/* macros for field RSVD_TXFTLR */
#define SPI_TXFTLR_REG__RSVD_TXFTLR__SHIFT                                                  SPI_TXFTLR_REG__TFT__WIDTH
#define SPI_TXFTLR_REG__RSVD_TXFTLR__WIDTH                                                  32 - SPI_TXFTLR_REG__TFT__WIDTH
#define SPI_TXFTLR_REG__RSVD_TXFTLR__MASK                                                   ~SPI_TXFTLR_REG__TFT__MASK
#define SPI_TXFTLR_REG__RSVD_TXFTLR__READ(src)                                              (((uint32_t)(src) & SPI_TXFTLR_REG__RSVD_TXFTLR__MASK) >> SPI_TXFTLR_REG__RSVD_TXFTLR__SHIFT)
#define SPI_TXFTLR_REG__RSVD_TXFTLR__WRITE(src)                                             (((uint32_t)(src) << SPI_TXFTLR_REG__RSVD_TXFTLR__SHIFT) & SPI_TXFTLR_REG__RSVD_TXFTLR__MASK

#endif /* __SPI_TXFTLR_REG_MACRO__ */

/* macros for spi_rxftlr_reg */
#ifndef __SPI_RXFTLR_REG_MACRO__
#define __SPI_RXFTLR_REG_MACRO__

/* macros for field RXFTLR */
#define SPI_RXFTLR_REG__RFT__SHIFT                                                          0
#define SPI_RXFTLR_REG__RFT__WIDTH(src)                                                     (src)
#define SPI_RXFTLR_REG__RFT__MASK(src)                                                      ~(~(0x00000001U << (SPI_RXFTLR_REG__RFT__WIDTH +1)) + 1)
#define SPI_RXFTLR_REG__RFT__READ(src)                                                      (((uint32_t)(src) & SPI_RXFTLR_REG__RFT__MASK) >> SPI_RXFTLR_REG__RFT__SHIFT)
#define SPI_RXFTLR_REG__RFT__WRITE(src)                                                     (((uint32_t)(src) << SPI_RXFTLR_REG__RFT__SHIFT) & SPI_RXFTLR_REG__RFT__MASK)

/* macros for field RSVD_RXFTLR */
#define SPI_RXFTLR_REG__RSVD_RXFTLR__SHIFT                                                  SPI_RXFTLR_REG__RFT__WIDTH
#define SPI_RXFTLR_REG__RSVD_RXFTLR__WIDTH                                                  32 - SPI_RXFTLR_REG__RFT__WIDTH
#define SPI_RXFTLR_REG__RSVD_RXFTLR__MASK                                                   ~SPI_RXFTLR_REG__RFT__MASK
#define SPI_RXFTLR_REG__RSVD_RXFTLR__READ(src)                                              (((uint32_t)(src) & SPI_RXFTLR_REG__RSVD_RXFTLR__MASK) >> SPI_RXFTLR_REG__RSVD_RXFTLR__SHIFT)
#define SPI_RXFTLR_REG__RSVD_RXFTLR__WRITE(src)                                             (((uint32_t)(src) << SPI_RXFTLR_REG__RSVD_RXFTLR__SHIFT) & SPI_RXFTLR_REG__RSVD_RXFTLR__MASK

#endif /* __SPI_RXFTLR_REG_MACRO__ */

/* macros for spi_txflr_reg */
#ifndef __SPI_TXFLR_REG_MACRO__
#define __SPI_TXFLR_REG_MACRO__

/* macros for field TXFLR */
#define SPI_TXFLR_REG__TXTFL__SHIFT                                                         0
#define SPI_TXFLR_REG__TXTFL__WIDTH(src)                                                    (src)
#define SPI_TXFLR_REG__TXTFL__MASK(src)                                                     ~(~(0x00000001U << (SPI_TXFLR_REG__TXTFL__WIDTH +1)) + 1)
#define SPI_TXFLR_REG__TXTFL__READ(src)                                                     (((uint32_t)(src) & SPI_TXFLR_REG__TXTFL__MASK) >> SPI_TXFLR_REG__TXTFL__SHIFT)
#define SPI_TXFLR_REG__TXTFL__WRITE(src)                                                    (((uint32_t)(src) << SPI_TXFLR_REG__TXTFL__SHIFT) & SPI_TXFLR_REG__TXTFL__MASK)

/* macros for field RSVD_TXFLR */
#define SPI_TXFLR_REG__RSVD_TXFLR__SHIFT                                                    SPI_TXFLR_REG__TXTFL__WIDTH
#define SPI_TXFLR_REG__RSVD_TXFLR__WIDTH                                                    32 - SPI_TXFLR_REG__TXTFL__WIDTH
#define SPI_TXFLR_REG__RSVD_TXFLR__MASK                                                     ~SPI_TXFLR_REG__TXTFL__MASK
#define SPI_TXFLR_REG__RSVD_TXFLR__READ(src)                                                ((uint32_t)(src) & SPI_TXFLR_REG__RSVD_TXFLR__MASK) >> SPI_TXFLR_REG__RSVD_TXFLR__SHIFT)
#define SPI_TXFLR_REG__RSVD_TXFLR__WRITE(src)                                               ((uint32_t)(src) << SPI_TXFLR_REG__RSVD_TXFLR__SHIFT) & SPI_TXFLR_REG__RSVD_TXFLR__MASK

#endif /* __SPI_TXFLR_REG_MACRO__ */

/* macros for spi_rxflr_reg */
#ifndef __SPI_RXFLR_REG_MACRO__
#define __SPI_RXFLR_REG_MACRO__

/* macros for field RXTFL */
#define SPI_RXFLR_REG__RXTFL__SHIFT                                                         0
#define SPI_RXFLR_REG__RXTFL__WIDTH(src)                                                    (src)
#define SPI_RXFLR_REG__RXTFL__MASK(src)                                                     ~(~(0x00000001U << (SPI_RXFLR_REG__RXFLR__WIDTH +1)) + 1)
#define SPI_RXFLR_REG__RXTFL__READ(src)                                                     (((uint32_t)(src) & SPI_RXFLR_REG__RXFLR__MASK) >> SPI_RXFLR_REG__RXFLR__SHIFT)
#define SPI_RXFLR_REG__RXTFL__WRITE(src)                                                    (((uint32_t)(src) << SPI_RXFLR_REG__RXFLR__SHIFT) & SPI_RXFLR_REG__RXFLR__MASK)

/* macros for field RSVD_RXFLR */
#define SPI_RXFLR_REG__RSVD_RXFLR__SHIFT                                                    SPI_RXFLR_REG__RXFLR__WIDTH
#define SPI_RXFLR_REG__RSVD_RXFLR__WIDTH                                                    32 - SPI_RXFLR_REG__RXFLR__WIDTH
#define SPI_RXFLR_REG__RSVD_RXFLR__MASK                                                     ~SPI_RXFLR_REG__RXFLR__MASK
#define SPI_RXFLR_REG__RSVD_RXFLR__READ(src)                                                (((uint32_t)(src) & SPI_RXFLR_REG__RSVD_RXFLR__MASK) >> SPI_RXFLR_REG__RSVD_RXFLR__SHIFT)
#define SPI_RXFLR_REG__RSVD_RXFLR__WRITE(src)                                               (((uint32_t)(src) << SPI_RXFLR_REG__RSVD_RXFLR__SHIFT) & SPI_RXFLR_REG__RSVD_RXFLR__MASK

#endif /* __SPI_RXFLR_REG_MACRO__ */

/* macros for spi_sr_reg */
#ifndef __SPI_SR_REG_MACRO__
#define __SPI_SR_REG_MACRO__

/* macros for field BUSY */
#define SPI_SR_REG__BUSY__SHIFT                                                             0
#define SPI_SR_REG__BUSY__WIDTH                                                             1
#define SPI_SR_REG__BUSY__MASK                                                              0x00000001U
#define SPI_SR_REG__BUSY__READ(src)                                                         (((uint32_t)(src) & SPI_SR_REG__BUSY__MASK) >> SPI_SR_REG__BUSY__SHIFT)
#define SPI_SR_REG__BUSY__WRITE(src)                                                        (((uint32_t)(src) << SPI_SR_REG__BUSY__SHIFT) & SPI_SR_REG__BUSY__MASK)

/* macros for field TFNF */
#define SPI_SR_REG__TFNF__SHIFT                                                             1
#define SPI_SR_REG__TFNF__WIDTH                                                             1
#define SPI_SR_REG__TFNF__MASK                                                              0x00000002U
#define SPI_SR_REG__TFNF__READ(src)                                                         (((uint32_t)(src) & SPI_SR_REG__TFNF__MASK) >> SPI_SR_REG__TFNF__SHIFT)
#define SPI_SR_REG__TFNF__WRITE(src)                                                        (((uint32_t)(src) << SPI_SR_REG__TFNF__SHIFT) & SPI_SR_REG__TFNF__MASK)

/* macros for field TFE */
#define SPI_SR_REG__TFE__SHIFT                                                              2
#define SPI_SR_REG__TFE__WIDTH                                                              1
#define SPI_SR_REG__TFE__MASK                                                               0x00000004U
#define SPI_SR_REG__TFE__READ(src)                                                          (((uint32_t)(src) & SPI_SR_REG__TFE__MASK) >> SPI_SR_REG__TFE__SHIFT)
#define SPI_SR_REG__TFE__WRITE(src)                                                         (((uint32_t)(src) << SPI_SR_REG__TFE__SHIFT) & SPI_SR_REG__TFE__MASK)

/* macros for field RFNE */
#define SPI_SR_REG__RFNE__SHIFT                                                             3
#define SPI_SR_REG__RFNE__WIDTH                                                             1
#define SPI_SR_REG__RFNE__MASK                                                              0x00000008U
#define SPI_SR_REG__RFNE__READ(src)                                                         (((uint32_t)(src) & SPI_SR_REG__RFNE__MASK) >> SPI_SR_REG__RFNE__SHIFT)
#define SPI_SR_REG__RFNE__WRITE(src)                                                        (((uint32_t)(src) << SPI_SR_REG__RFNE__SHIFT) & SPI_SR_REG__RFNE__MASK)

/* macros for field RFF */
#define SPI_SR_REG__RFF__SHIFT                                                              4
#define SPI_SR_REG__RFF__WIDTH                                                              1
#define SPI_SR_REG__RFF__MASK                                                               0x00000010U
#define SPI_SR_REG__RFF__READ(src)                                                          (((uint32_t)(src) & SPI_SR_REG__RFF__MASK) >> SPI_SR_REG__RFF__SHIFT)
#define SPI_SR_REG__RFF__WRITE(src)                                                         (((uint32_t)(src) << SPI_SR_REG__RFF__SHIFT) & SPI_SR_REG__RFF__MASK)

/* macros for field TXE */
#define SPI_SR_REG__TXE__SHIFT                                                              5
#define SPI_SR_REG__TXE__WIDTH                                                              1
#define SPI_SR_REG__TXE__MASK                                                               0x00000020U
#define SPI_SR_REG__TXE__READ(src)                                                          (((uint32_t)(src) & SPI_SR_REG__TXE__MASK) >> SPI_SR_REG__TXE__SHIFT)
#define SPI_SR_REG__TXE__WRITE(src)                                                         (((uint32_t)(src) << SPI_SR_REG__TXE__SHIFT) & SPI_SR_REG__TXE__MASK)

/* macros for field DCOL */
#define SPI_SR_REG__DCOL__SHIFT                                                             6
#define SPI_SR_REG__DCOL__WIDTH                                                             1
#define SPI_SR_REG__DCOL__MASK                                                              0x00000040U
#define SPI_SR_REG__DCOL__READ(src)                                                         (((uint32_t)(src) & SPI_SR_REG__DCOL__MASK) >> SPI_SR_REG__DCOL__SHIFT)
#define SPI_SR_REG__DCOL__WRITE(src)                                                        (((uint32_t)(src) << SPI_SR_REG__DCOL__SHIFT) & SPI_SR_REG__DCOL__MASK)

/* macros for field RSVD_SR */
#define SPI_SR_REG__RSVD_SR__SHIFT                                                          7
#define SPI_SR_REG__RSVD_SR__WIDTH                                                          25
#define SPI_SR_REG__RSVD_SR__MASK                                                           0xFFFFFF80U
#define SPI_SR_REG__RSVD_SR__READ(src)                                                      (((uint32_t)(src) & SPI_SR_REG__RSVD_SR__MASK) >> SPI_SR_REG__RSVD_SR__SHIFT)
#define SPI_SR_REG__RSVD_SR__WRITE(src)                                                     (((uint32_t)(src) << SPI_SR_REG__RSVD_SR__SHIFT) & SPI_SR_REG__RSVD_SR__MASK)

#endif /* __SPI_SR_REG_MACRO__ */

/* macros for spi_imr_reg */
#ifndef __SPI_IMR_REG_MACRO__
#define __SPI_IMR_REG_MACRO__

/* macros for field TXEIM */
#define SPI_IMR_REG__TXEIM__SHIFT                                                           0
#define SPI_IMR_REG__TXEIM__WIDTH                                                           1
#define SPI_IMR_REG__TXEIM__MASK                                                            0x00000001U
#define SPI_IMR_REG__TXEIM__READ(src)                                                       (((uint32_t)(src) & SPI_IMR_REG__TXEIM__MASK) >> SPI_IMR_REG__TXEIM__SHIFT)
#define SPI_IMR_REG__TXEIM__WRITE(src)                                                      (((uint32_t)(src) << SPI_IMR_REG__TXEIM__SHIFT) & SPI_IMR_REG__TXEIM__MASK)

/* macros for field TXOIM */
#define SPI_IMR_REG__TXOIM__SHIFT                                                           1
#define SPI_IMR_REG__TXOIM__WIDTH                                                           1
#define SPI_IMR_REG__TXOIM__MASK                                                            0x00000002U
#define SPI_IMR_REG__TXOIM__READ(src)                                                       (((uint32_t)(src) & SPI_IMR_REG__TXOIM__MASK) >> SPI_IMR_REG__TXOIM__SHIFT)
#define SPI_IMR_REG__TXOIM__WRITE(src)                                                      (((uint32_t)(src) << SPI_IMR_REG__TXOIM__SHIFT) & SPI_IMR_REG__TXOIM__MASK)

/* macros for field RXUIM */
#define SPI_IMR_REG__RXUIM__SHIFT                                                           2
#define SPI_IMR_REG__RXUIM__WIDTH                                                           1
#define SPI_IMR_REG__RXUIM__MASK                                                            0x00000004U
#define SPI_IMR_REG__RXUIM__READ(src)                                                       (((uint32_t)(src) & SPI_IMR_REG__RXUIM__MASK) >> SPI_IMR_REG__RXUIM__SHIFT)
#define SPI_IMR_REG__RXUIM__WRITE(src)                                                      (((uint32_t)(src) << SPI_IMR_REG__RXUIM__SHIFT) & SPI_IMR_REG__RXUIM__MASK)

/* macros for field RXOIM */
#define SPI_IMR_REG__RXOIM__SHIFT                                                           3
#define SPI_IMR_REG__RXOIM__WIDTH                                                           1
#define SPI_IMR_REG__RXOIM__MASK                                                            0x00000008U
#define SPI_IMR_REG__RXOIM__READ(src)                                                       (((uint32_t)(src) & SPI_IMR_REG__RXOIM__MASK) >> SPI_IMR_REG__RXOIM__SHIFT)
#define SPI_IMR_REG__RXOIM__WRITE(src)                                                      (((uint32_t)(src) << SPI_IMR_REG__RXOIM__SHIFT) & SPI_IMR_REG__RXOIM__MASK)

/* macros for field RXFIM */
#define SPI_IMR_REG__RXFIM__SHIFT                                                           4
#define SPI_IMR_REG__RXFIM__WIDTH                                                           1
#define SPI_IMR_REG__RXFIM__MASK                                                            0x00000010U
#define SPI_IMR_REG__RXFIM__READ(src)                                                       (((uint32_t)(src) & SPI_IMR_REG__RXFIM__MASK) >> SPI_IMR_REG__RXFIM__SHIFT)
#define SPI_IMR_REG__RXFIM__WRITE(src)                                                      (((uint32_t)(src) << SPI_IMR_REG__RXFIM__SHIFT) & SPI_IMR_REG__RXFIM__MASK)

/* macros for field MSTIM */
#define SPI_IMR_REG__MSTIM__SHIFT                                                           5
#define SPI_IMR_REG__MSTIM__WIDTH                                                           1
#define SPI_IMR_REG__MSTIM__MASK                                                            0x00000020U
#define SPI_IMR_REG__MSTIM__READ(src)                                                       (((uint32_t)(src) & SPI_IMR_REG__MSTIM__MASK) >> SPI_IMR_REG__MSTIM__SHIFT)
#define SPI_IMR_REG__MSTIM__WRITE(src)                                                      (((uint32_t)(src) << SPI_IMR_REG__MSTIM__SHIFT) & SPI_IMR_REG__MSTIM__MASK)

/* macros for field RSVD_IMR */
#define SPI_IMR_REG__RSVD_IMR__SHIFT                                                        6
#define SPI_IMR_REG__RSVD_IMR__WIDTH                                                        26
#define SPI_IMR_REG__RSVD_IMR__MASK                                                         0xFFFFFFC0U
#define SPI_IMR_REG__RSVD_IMR__READ(src)                                                    (((uint32_t)(src) & SPI_IMR_REG__RSVD_IMR__MASK) >> SPI_IMR_REG__RSVD_IMR__SHIFT)
#define SPI_IMR_REG__RSVD_IMR__WRITE(src)                                                   (((uint32_t)(src) << SPI_IMR_REG__RSVD_IMR__SHIFT) & SPI_IMR_REG__RSVD_IMR__MASK)

#endif /* __SPI_IMR_REG_MACRO__ */

/* macros for spi_isr_reg */
#ifndef __SPI_ISR_REG_MACRO__
#define __SPI_ISR_REG_MACRO__

/* macros for field TXEIS */
#define SPI_ISR_REG__TXEIS__SHIFT                                                           0
#define SPI_ISR_REG__TXEIS__WIDTH                                                           1
#define SPI_ISR_REG__TXEIS__MASK                                                            0x00000001U
#define SPI_ISR_REG__TXEIS__READ(src)                                                       (((uint32_t)(src) & SPI_ISR_REG__TXEIS__MASK) >> SPI_ISR_REG__TXEIS__SHIFT)
#define SPI_ISR_REG__TXEIS__WRITE(src)                                                      (((uint32_t)(src) << SPI_ISR_REG__TXEIS__SHIFT) & SPI_ISR_REG__TXEIS__MASK)

/* macros for field TXOIS */
#define SPI_ISR_REG__TXOIS__SHIFT                                                           1
#define SPI_ISR_REG__TXOIS__WIDTH                                                           1
#define SPI_ISR_REG__TXOIS__MASK                                                            0x00000002U
#define SPI_ISR_REG__TXOIS__READ(src)                                                       (((uint32_t)(src) & SPI_ISR_REG__TXOIS__MASK) >> SPI_ISR_REG__TXOIS__SHIFT)
#define SPI_ISR_REG__TXOIS__WRITE(src)                                                      (((uint32_t)(src) << SPI_ISR_REG__TXOIS__SHIFT) & SPI_ISR_REG__TXOIS__MASK)

/* macros for field RXUIS */
#define SPI_ISR_REG__RXUIS__SHIFT                                                           2
#define SPI_ISR_REG__RXUIS__WIDTH                                                           1
#define SPI_ISR_REG__RXUIS__MASK                                                            0x00000004U
#define SPI_ISR_REG__RXUIS__READ(src)                                                       (((uint32_t)(src) & SPI_ISR_REG__RXUIS__MASK) >> SPI_ISR_REG__RXUIS__SHIFT)
#define SPI_ISR_REG__RXUIS__WRITE(src)                                                      (((uint32_t)(src) << SPI_ISR_REG__RXUIS__SHIFT) & SPI_ISR_REG__RXUIS__MASK)

/* macros for field RXOIM */
#define SPI_ISR_REG__RXOIS__SHIFT                                                           3
#define SPI_ISR_REG__RXOIS__WIDTH                                                           1
#define SPI_ISR_REG__RXOIS__MASK                                                            0x00000008U
#define SPI_ISR_REG__RXOIS__READ(src)                                                       (((uint32_t)(src) & SPI_ISR_REG__RXOIS__MASK) >> SPI_ISR_REG__RXOIS__SHIFT)
#define SPI_ISR_REG__RXOIS__WRITE(src)                                                      (((uint32_t)(src) << SPI_ISR_REG__RXOIS__SHIFT) & SPI_ISR_REG__RXOIS__MASK)

/* macros for field RXFIS */
#define SPI_ISR_REG__RXFIS__SHIFT                                                           4
#define SPI_ISR_REG__RXFIS__WIDTH                                                           1
#define SPI_ISR_REG__RXFIS__MASK                                                            0x00000010U
#define SPI_ISR_REG__RXFIS__READ(src)                                                       (((uint32_t)(src) & SPI_ISR_REG__RXFIS__MASK) >> SPI_ISR_REG__RXFIS__SHIFT)
#define SPI_ISR_REG__RXFIS__WRITE(src)                                                      (((uint32_t)(src) << SPI_ISR_REG__RXFIS__SHIFT) & SPI_ISR_REG__RXFIS__MASK)

/* macros for field MSTIS */
#define SPI_ISR_REG__MSTIS__SHIFT                                                           5
#define SPI_ISR_REG__MSTIS__WIDTH                                                           1
#define SPI_ISR_REG__MSTIS__MASK                                                            0x00000020U
#define SPI_ISR_REG__MSTIS__READ(src)                                                       (((uint32_t)(src) & SPI_ISR_REG__MSTIS__MASK) >> SPI_ISR_REG__MSTIS__SHIFT)
#define SPI_ISR_REG__MSTIS__WRITE(src)                                                      (((uint32_t)(src) << SPI_ISR_REG__MSTIS__SHIFT) & SPI_ISR_REG__MSTIS__MASK)

/* macros for field RSVD_ISR */
#define SPI_ISR_REG__RSVD_ISR__SHIFT                                                        6
#define SPI_ISR_REG__RSVD_ISR__WIDTH                                                        26
#define SPI_ISR_REG__RSVD_ISR__MASK                                                         0xFFFFFFC0U
#define SPI_ISR_REG__RSVD_ISR__READ(src)                                                    (((uint32_t)(src) & SPI_ISR_REG__RSVD_ISR__MASK) >> SPI_ISR_REG__RSVD_ISR__SHIFT)
#define SPI_ISR_REG__RSVD_ISR__WRITE(src)                                                   (((uint32_t)(src) << SPI_ISR_REG__RSVD_ISR__SHIFT) & SPI_ISR_REG__RSVD_ISR__MASK)

#endif /* __SPI_ISR_REG_MACRO__ */

/* macros for spi_risr_reg */
#ifndef __SPI_RISR_REG_MACRO__
#define __SPI_RISR_REG_MACRO__

/* macros for field TXEIR */
#define SPI_RISR_REG__TXEIR__SHIFT                                                          0
#define SPI_RISR_REG__TXEIR__WIDTH                                                          1
#define SPI_RISR_REG__TXEIR__MASK                                                           0x00000001U
#define SPI_RISR_REG__TXEIR__READ(src)                                                      (((uint32_t)(src) & SPI_RISR_REG__TXEIR__MASK) >> SPI_RISR_REG__TXEIR__SHIFT)
#define SPI_RISR_REG__TXEIR__WRITE(src)                                                     (((uint32_t)(src) << SPI_RISR_REG__TXEIR__SHIFT) & SPI_RISR_REG__TXEIR__MASK)

/* macros for field TXOIR */
#define SPI_RISR_REG__TXOIR__SHIFT                                                          1
#define SPI_RISR_REG__TXOIR__WIDTH                                                          1
#define SPI_RISR_REG__TXOIR__MASK                                                           0x00000002U
#define SPI_RISR_REG__TXOIR__READ(src)                                                      (((uint32_t)(src) & SPI_RISR_REG__TXOIR__MASK) >> SPI_RISR_REG__TXOIR__SHIFT)
#define SPI_RISR_REG__TXOIR__WRITE(src)                                                     (((uint32_t)(src) << SPI_RISR_REG__TXOIR__SHIFT) & SPI_RISR_REG__TXOIR__MASK)

/* macros for field RXUIR */
#define SPI_RISR_REG__RXUIR__SHIFT                                                          2
#define SPI_RISR_REG__RXUIR__WIDTH                                                          1
#define SPI_RISR_REG__RXUIR__MASK                                                           0x00000004U
#define SPI_RISR_REG__RXUIR__READ(src)                                                      (((uint32_t)(src) & SPI_RISR_REG__RXUIR__MASK) >> SPI_RISR_REG__RXUIR__SHIFT)
#define SPI_RISR_REG__RXUIR__WRITE(src)                                                     (((uint32_t)(src) << SPI_RISR_REG__RXUIR__SHIFT) & SPI_RISR_REG__RXUIR__MASK)

/* macros for field RXOIM */
#define SPI_RISR_REG__RXOIR__SHIFT                                                          3
#define SPI_RISR_REG__RXOIR__WIDTH                                                          1
#define SPI_RISR_REG__RXOIR__MASK                                                           0x00000008U
#define SPI_RISR_REG__RXOIR__READ(src)                                                      (((uint32_t)(src) & SPI_RISR_REG__RXOIR__MASK) >> SPI_RISR_REG__RXOIR__SHIFT)
#define SPI_RISR_REG__RXOIR__WRITE(src)                                                     (((uint32_t)(src) << SPI_RISR_REG__RXOIR__SHIFT) & SPI_RISR_REG__RXOIR__MASK)

/* macros for field RXFIR */
#define SPI_RISR_REG__RXFIR__SHIFT                                                          4
#define SPI_RISR_REG__RXFIR__WIDTH                                                          1
#define SPI_RISR_REG__RXFIR__MASK                                                           0x00000010U
#define SPI_RISR_REG__RXFIR__READ(src)                                                      (((uint32_t)(src) & SPI_RISR_REG__RXFIR__MASK) >> SPI_RISR_REG__RXFIR__SHIFT)
#define SPI_RISR_REG__RXFIR__WRITE(src)                                                     (((uint32_t)(src) << SPI_RISR_REG__RXFIR__SHIFT) & SPI_RISR_REG__RXFIR__MASK)

/* macros for field MSTIR */
#define SPI_RISR_REG__MSTIR__SHIFT                                                          5
#define SPI_RISR_REG__MSTIR__WIDTH                                                          1
#define SPI_RISR_REG__MSTIR__MASK                                                           0x00000020U
#define SPI_RISR_REG__MSTIR__READ(src)                                                      (((uint32_t)(src) & SPI_RISR_REG__MSTIR__MASK) >> SPI_RISR_REG__MSTIR__SHIFT)
#define SPI_RISR_REG__MSTIR__WRITE(src)                                                     (((uint32_t)(src) << SPI_RISR_REG__MSTIR__SHIFT) & SPI_RISR_REG__MSTIR__MASK)

/* macros for field RSVD_RISR */
#define SPI_RISR_REG__RSVD_RISR__SHIFT                                                      6
#define SPI_RISR_REG__RSVD_RISR__WIDTH                                                      26
#define SPI_RISR_REG__RSVD_RISR__MASK                                                       0xFFFFFFC0U
#define SPI_RISR_REG__RSVD_RISR__READ(src)                                                  (((uint32_t)(src) & SPI_RISR_REG__RSVD_RISR__MASK) >> SPI_RISR_REG__RSVD_RISR__SHIFT)
#define SPI_RISR_REG__RSVD_RISR__WRITE(src)                                                 (((uint32_t)(src) << SPI_RISR_REG__RSVD_RISR__SHIFT) & SPI_RISR_REG__RSVD_RISR__MASK)

#endif /* __SPI_RISR_REG_MACRO__ */

/* macros for spi_txoicr_reg */
#ifndef __SPI_TXOICR_REG_MACRO__
#define __SPI_TXOICR_REG_MACRO__

/* macros for field TXOICR */
#define SPI_TXOICR_REG__TXOICR__SHIFT                                                       0
#define SPI_TXOICR_REG__TXOICR__WIDTH                                                       1
#define SPI_TXOICR_REG__TXOICR__MASK                                                        0x00000001U
#define SPI_TXOICR_REG__TXOICR__READ(src)                                                   (((uint32_t)(src) & SPI_TXOICR_REG__TXOICR__MASK) >> SPI_TXOICR_REG__TXOICR__SHIFT)
#define SPI_TXOICR_REG__TXOICR__WRITE(src)                                                  (((uint32_t)(src) << SPI_TXOICR_REG__TXOICR__SHIFT) & SPI_RISR_REG__TXOICR__MASK)

/* macros for field RSVD_TXOICR */
#define SPI_TXOICR_REG__RSVD_TXOICR__SHIFT                                                  1
#define SPI_TXOICR_REG__RSVD_TXOICR__WIDTH                                                  31
#define SPI_TXOICR_REG__RSVD_TXOICR__MASK                                                   0xFFFFFFFEU
#define SPI_TXOICR_REG__RSVD_TXOICR__READ(src)                                              (((uint32_t)(src) & SPI_TXOICR_REG__TXOICR__MASK) >> SPI_TXOICR_REG__TXOICR__SHIFT)
#define SPI_TXOICR_REG__RSVD_TXOICR__WRITE(src)                                             (((uint32_t)(src) << SPI_TXOICR_REG__TXOICR__SHIFT) & SPI_RISR_REG__TXOICR__MASK)

#endif /* __SPI_TXOICR_REG_MACRO__ */

/* macros for spi_rxoicr_reg */
#ifndef __SPI_RXOICR_REG_MACRO__
#define __SPI_RXOICR_REG_MACRO__

/* macros for field RXOICR */
#define SPI_RXOICR_REG__RXOICR__SHIFT                                                       0
#define SPI_RXOICR_REG__RXOICR__WIDTH                                                       1
#define SPI_RXOICR_REG__RXOICR__MASK                                                        0x00000001U
#define SPI_RXOICR_REG__RXOICR__READ(src)                                                   (((uint32_t)(src) & SPI_RXOICR_REG__RXOICR__MASK) >> SPI_RXOICR_REG__RXOICR__SHIFT)
#define SPI_RXOICR_REG__RXOICR__WRITE(src)                                                  (((uint32_t)(src) << SPI_RXOICR_REG__RXOICR__SHIFT) & SPI_RISR_REG__RXOICR__MASK)

/* macros for field RSVD_RXOICR */
#define SPI_RXOICR_REG__RSVD_RXOICR__SHIFT                                                  1
#define SPI_RXOICR_REG__RSVD_RXOICR__WIDTH                                                  31
#define SPI_RXOICR_REG__RSVD_RXOICR__MASK                                                   0xFFFFFFFEU
#define SPI_RXOICR_REG__RSVD_RXOICR__READ(src)                                              (((uint32_t)(src) & SPI_RXOICR_REG__RXOICR__MASK) >> SPI_RXOICR_REG__RXOICR__SHIFT)
#define SPI_RXOICR_REG__RSVD_RXOICR__WRITE(src)                                             (((uint32_t)(src) << SPI_RXOICR_REG__RXOICR__SHIFT) & SPI_RISR_REG__RXOICR__MASK)

#endif /* __SPI_RXOICR_REG_MACRO__ */

/* macros for spi_rxuicr_reg */
#ifndef __SPI_RXUICR_REG_MACRO__
#define __SPI_RXUICR_REG_MACRO__

/* macros for field RXUICR */
#define SPI_RXUICR_REG__RXUICR__SHIFT                                                       0
#define SPI_RXUICR_REG__RXUICR__WIDTH                                                       1
#define SPI_RXUICR_REG__RXUICR__MASK                                                        0x00000001U
#define SPI_RXUICR_REG__RXUICR__READ(src)                                                   (((uint32_t)(src) & SPI_RXUICR_REG__RXUICR__MASK) >> SPI_RXUICR_REG__RXUICR__SHIFT)
#define SPI_RXUICR_REG__RXUICR__WRITE(src)                                                  (((uint32_t)(src) << SPI_RXUICR_REG__RXUICR__SHIFT) & SPI_RISR_REG__RXUICR__MASK)

/* macros for field RSVD_RXUICR */
#define SPI_RXUICR_REG__RSVD_RXUICR__SHIFT                                                  1
#define SPI_RXUICR_REG__RSVD_RXUICR__WIDTH                                                  31
#define SPI_RXUICR_REG__RSVD_RXUICR__MASK                                                   0xFFFFFFFEU
#define SPI_RXUICR_REG__RSVD_RXUICR__READ(src)                                              (((uint32_t)(src) & SPI_RXUICR_REG__RXUICR__MASK) >> SPI_RXUICR_REG__RXUICR__SHIFT)
#define SPI_RXUICR_REG__RSVD_RXUICR__WRITE(src)                                             (((uint32_t)(src) << SPI_RXUICR_REG__RXUICR__SHIFT) & SPI_RISR_REG__RXUICR__MASK)

#endif /* __SPI_RXUICR_REG_MACRO__ */

/* macros for spi_msticr_reg */
#ifndef __SPI_MSTICR_REG_MACRO__
#define __SPI_MSTICR_REG_MACRO__

/* macros for field MSTICR */
#define SPI_MSTICR_REG__MSTICR__SHIFT                                                       0
#define SPI_MSTICR_REG__MSTICR__WIDTH                                                       1
#define SPI_MSTICR_REG__MSTICR__MASK                                                        0x00000001U
#define SPI_MSTICR_REG__MSTICR__READ(src)                                                   (((uint32_t)(src) & SPI_MSTICR_REG__MSTICR__MASK) >> SPI_MSTICR_REG__MSTICR__SHIFT)
#define SPI_MSTICR_REG__MSTICR__WRITE(src)                                                  (((uint32_t)(src) << SPI_MSTICR_REG__MSTICR__SHIFT) & SPI_RISR_REG__MSTICR__MASK)

/* macros for field RSVD_MSTICR */
#define SPI_MSTICR_REG__RSVD_MSTICR__SHIFT                                                  1
#define SPI_MSTICR_REG__RSVD_MSTICR__WIDTH                                                  31
#define SPI_MSTICR_REG__RSVD_MSTICR__MASK                                                   0xFFFFFFFEU
#define SPI_MSTICR_REG__RSVD_MSTICR__READ(src)                                              (((uint32_t)(src) & SPI_MSTICR_REG__MSTICR__MASK) >> SPI_MSTICR_REG__MSTICR__SHIFT)
#define SPI_MSTICR_REG__RSVD_MSTICR__WRITE(src)                                             (((uint32_t)(src) << SPI_MSTICR_REG__MSTICR__SHIFT) & SPI_RISR_REG__MSTICR__MASK)

#endif /* __SPI_MSTICR_REG_MACRO__ */

/* macros for spi_icr_reg */
#ifndef __SPI_ICR_REG_MACRO__
#define __SPI_ICR_REG_MACRO__

/* macros for field ICR */
#define SPI_ICR_REG__ICR__SHIFT                                                             0
#define SPI_ICR_REG__ICR__WIDTH                                                             1
#define SPI_ICR_REG__ICR__MASK                                                              0x00000001U
#define SPI_ICR_REG__ICR__READ(src)                                                         (((uint32_t)(src) & SPI_ICR_REG__ICR__MASK) >> SPI_ICR_REG__ICR__SHIFT)
#define SPI_ICR_REG__ICR__WRITE(src)                                                        (((uint32_t)(src) << SPI_ICR_REG__ICR__SHIFT) & SPI_RISR_REG__ICR__MASK)

/* macros for field RSVD_ICR */
#define SPI_ICR_REG__RSVD_ICR__SHIFT                                                        1
#define SPI_ICR_REG__RSVD_ICR__WIDTH                                                        31
#define SPI_ICR_REG__RSVD_ICR__MASK                                                         0xFFFFFFFEU
#define SPI_ICR_REG__RSVD_ICR__READ(src)                                                    (((uint32_t)(src) & SPI_ICR_REG__ICR__MASK) >> SPI_ICR_REG__ICR__SHIFT)
#define SPI_ICR_REG__RSVD_ICR__WRITE(src)                                                   (((uint32_t)(src) << SPI_ICR_REG__ICR__SHIFT) & SPI_RISR_REG__ICR__MASK)

#endif /* __SPI_ICR_REG_MACRO__ */

/* macros for spi_dmacr_reg */
#ifndef __SPI_DMACR_REG_MACRO__
#define __SPI_DMACR_REG_MACRO__

/* macros for field RDMAE */
#define SPI_DMACR_REG__RDMAE__SHIFT                                                         0
#define SPI_DMACR_REG__RDMAE__WIDTH                                                         1
#define SPI_DMACR_REG__RDMAE__MASK                                                          0x00000001U
#define SPI_DMACR_REG__RDMAE__READ(src)                                                     (((uint32_t)(src) & SPI_DMACR_REG__RDMAE__MASK) >> SPI_DMACR_REG__RDMAE__SHIFT)
#define SPI_DMACR_REG__RDMAE__WRITE(src)                                                    (((uint32_t)(src) << SPI_DMACR_REG__RDMAE__SHIFT) & SPI_RISR_REG__RDMAE__MASK)

/* macros for field TDMAE */
#define SPI_DMACR_REG__TDMAE__SHIFT                                                         1
#define SPI_DMACR_REG__TDMAE__WIDTH                                                         1
#define SPI_DMACR_REG__TDMAE__MASK                                                          0x00000002U
#define SPI_DMACR_REG__TDMAE__READ(src)                                                     (((uint32_t)(src) & SPI_DMACR_REG__TDMAE__MASK) >> SPI_DMACR_REG__TDMAE__SHIFT)
#define SPI_DMACR_REG__TDMAE__WRITE(src)                                                    (((uint32_t)(src) << SPI_DMACR_REG__TDMAE__SHIFT) & SPI_RISR_REG__TDMAE__MASK)

/* macros for field RSVD_DMACR */
#define SPI_DMACR_REG__RSVD_DMACR__SHIFT                                                    1
#define SPI_DMACR_REG__RSVD_DMACR__WIDTH                                                    31
#define SPI_DMACR_REG__RSVD_DMACR__MASK                                                     0xFFFFFFFCU
#define SPI_DMACR_REG__RSVD_DMACR__READ(src)                                                (((uint32_t)(src) & SPI_DMACR_REG__DMACR__MASK) >> SPI_DMACR_REG__DMACR__SHIFT)
#define SPI_DMACR_REG__RSVD_DMACR__WRITE(src)                                               (((uint32_t)(src) << SPI_DMACR_REG__DMACR__SHIFT) & SPI_RISR_REG__DMACR__MASK)

#endif /* __SPI_DMACR_REG_MACRO__ */

/* macros for spi_dmatdlr_reg */
#ifndef __SPI_DMATDLR_REG_MACRO__
#define __SPI_DMATDLR_REG_MACRO__

/* macros for field DMATDL */
#define SPI_DMATDLR_REG__DMATDL__SHIFT                                                      0
#define SPI_DMATDLR_REG__DMATDL__WIDTH(src)                                                 (src)
#define SPI_DMATDLR_REG__DMATDL__MASK(src)                                                  ~(~(0x00000001U << (SPI_DMATDLR_REG__DMATDL__WIDTH +1)) + 1)
#define SPI_DMATDLR_REG__DMATDL__READ(src)                                                  (((uint32_t)(src) & SPI_DMATDLR_REG__DMATDL__MASK) >> SPI_DMATDLR_REG__DMATDL__SHIFT)
#define SPI_DMATDLR_REG__DMATDL__WRITE(src)                                                 (((uint32_t)(src) << SPI_DMATDLR_REG__DMATDL__SHIFT) & SPI_DMATDLR_REG__DMATDL__MASK)

/* macros for field RSVD_DMATDLR */
#define SPI_DMATDLR_REG__RSVD_DMATDLR__SHIFT                                                SPI_DMATDLR_REG__DMATDL__WIDTH
#define SPI_DMATDLR_REG__RSVD_DMATDLR__WIDTH                                                32 - SPI_DMATDLR_REG__DMATDL__WIDTH
#define SPI_DMATDLR_REG__RSVD_DMATDLR__MASK                                                 ~SPI_DMATDLR_REG__DMATDL__MASK
#define SPI_DMATDLR_REG__RSVD_DMATDLR__READ(src)                                            (((uint32_t)(src) & SPI_DMATDLR_REG__RSVD_DMATDLR__MASK) >> SPI_DMATDLR_REG__RSVD_DMATDLR__SHIFT)
#define SPI_DMATDLR_REG__RSVD_DMATDLR__WRITE(src)                                           (((uint32_t)(src) << SPI_DMATDLR_REG__RSVD_DMATDLR__SHIFT) & SPI_DMATDLR_REG__RSVD_DMATDLR__MASK

#endif /* __SPI_DMATDLR_REG_MACRO__ */

/* macros for spi_dmardlr_reg */
#ifndef __SPI_DMARDLR_REG_MACRO__
#define __SPI_DMARDLR_REG_MACRO__

/* macros for field DMATDL */
#define SPI_DMARDLR_REG__DMATDL__SHIFT                                                      0
#define SPI_DMARDLR_REG__DMATDL__WIDTH(src)                                                 (src)
#define SPI_DMARDLR_REG__DMATDL__MASK(src)                                                  ~(~(0x00000001U << (SPI_DMARDLR_REG__DMATDL__WIDTH +1)) + 1)
#define SPI_DMARDLR_REG__DMATDL__READ(src)                                                  (((uint32_t)(src) & SPI_DMARDLR_REG__DMATDL__MASK) >> SPI_DMARDLR_REG__DMATDL__SHIFT)
#define SPI_DMARDLR_REG__DMATDL__WRITE(src)                                                 (((uint32_t)(src) << SPI_DMARDLR_REG__DMATDL__SHIFT) & SPI_DMARDLR_REG__DMATDL__MASK)

/* macros for field RSVD_DMARDLR */
#define SPI_DMARDLR_REG__RSVD_DMARDLR__SHIFT                                                SPI_DMARDLR_REG__DMATDL__WIDTH
#define SPI_DMARDLR_REG__RSVD_DMARDLR__WIDTH                                                32 - SPI_DMARDLR_REG__DMATDL__WIDTH
#define SPI_DMARDLR_REG__RSVD_DMARDLR__MASK                                                 ~SPI_DMARDLR_REG__DMATDL__MASK
#define SPI_DMARDLR_REG__RSVD_DMARDLR__READ(src)                                            (((uint32_t)(src) & SPI_DMARDLR_REG__RSVD_DMARDLR__MASK) >> SPI_DMARDLR_REG__RSVD_DMARDLR__SHIFT)
#define SPI_DMARDLR_REG__RSVD_DMARDLR__WRITE(src)                                           (((uint32_t)(src) << SPI_DMARDLR_REG__RSVD_DMARDLR__SHIFT) & SPI_DMARDLR_REG__RSVD_DMARDLR__MASK

#endif /* __SPI_DMARDLR_REG_MACRO__ */

/* macros for spi_idr_reg */
#ifndef __SPI_IDR_REG_MACRO__
#define __SPI_IDR_REG_MACRO__

/* macros for field IDCODE */
#define SPI_IDR_REG__IDCODE__SHIFT                                                          0
#define SPI_IDR_REG__IDCODE__WIDTH                                                          32
#define SPI_IDR_REG__IDCODE__MASK                                                           0xFFFFFFFFU
#define SPI_IDR_REG__IDCODE__READ(src)                                                      (((uint32_t)(src) & SPI_IDR_REG__IDCODE__MASK) >> SPI_IDR_REG__IDCODE__SHIFT)
#define SPI_IDR_REG__IDCODE__WRITE(src)                                                     (((uint32_t)(src) << SPI_IDR_REG__IDCODE__SHIFT) & SPI_IDR_REG__IDCODE__MASK)

#endif /* __SPI_IDR_REG_MACRO__ */

/* macros for spi_ssi_version_id_reg */
#ifndef __SPI_SSI_VERSION_ID_REG_MACRO__
#define __SPI_SSI_VERSION_ID_REG_MACRO__

/* macros for field IDCODE */
#define SPI_SSI_VERSION_ID_REG__SSI_COMP_VERSION__SHIFT                                     0
#define SPI_SSI_VERSION_ID_REG__SSI_COMP_VERSION__WIDTH                                     32
#define SPI_SSI_VERSION_ID_REG__SSI_COMP_VERSION__MASK                                      0xFFFFFFFFU
#define SPI_SSI_VERSION_ID_REG__SSI_COMP_VERSION__READ(src)                                 (((uint32_t)(src) & SPI_SSI_VERSION_ID_REG__SSI_COMP_VERSION__MASK) >> SPI_SSI_VERSION_ID_REG__SSI_COMP_VERSION__SHIFT)
#define SPI_SSI_VERSION_ID_REG__SSI_COMP_VERSION__WRITE(src)                                (((uint32_t)(src) << SPI_SSI_VERSION_ID_REG__SSI_COMP_VERSION__SHIFT) & SPI_SSI_VERSION_ID_REG__SSI_COMP_VERSION__MASK)

#endif /* __SPI_SSI_VERSION_ID_REG_MACRO__ */

/* macros for spi_dr_reg */
#ifndef __SPI_DR_REG_MACRO__
#define __SPI_DR_REG_MACRO__

/* macros for field DR */
#define SPI_DR_REG__DR__SHIFT                                                               0
#define SPI_DR_REG__DR__WIDTH                                                               32
#define SPI_DR_REG__DR__MASK                                                                0xFFFFFFFFU
#define SPI_DR_REG__DR__READ(src)                                                           (((uint32_t)(src) & SPI_DR_REG__DR__MASK) >> SPI_DR_REG__DR__SHIFT)
#define SPI_DR_REG__DR__WRITE(src)                                                          (((uint32_t)(src) << SPI_DR_REG__DR__SHIFT) & SPI_DR_REG__DR__MASK)

#endif /* __SPI_DR_REG_MACRO__ */

/* macros for spi_rx_sample_dly_reg */
#ifndef __SPI_RX_SAMPLE_DLY_REG_MACRO__
#define __SPI_RX_SAMPLE_DLY_REG_MACRO__

/* macros for field RSD */
#define SPI_RX_SAMPLE_DLY_REG__RSD__SHIFT                                                   0
#define SPI_RX_SAMPLE_DLY_REG__RSD__WIDTH                                                   8
#define SPI_RX_SAMPLE_DLY_REG__RSD__MASK                                                    0x000000FFU
#define SPI_RX_SAMPLE_DLY_REG__RSD__READ(src)                                               (((uint32_t)(src) & SPI_RX_SAMPLE_DLY_REG__RSD__MASK) >> SPI_RX_SAMPLE_DLY_REG__RSD__SHIFT)
#define SPI_RX_SAMPLE_DLY_REG__RSD__WRITE(src)                                              (((uint32_t)(src) << SPI_RX_SAMPLE_DLY_REG__RSD_SHIFT) & SPI_RX_SAMPLE_DLY_REG__RSD__MASK)

/* macros for field RSVD_RX_SAMPLE_DLY */
#define SPI_RX_SAMPLE_DLY_REG__RSVD_RX_SAMPLE_DLY__SHIFT                                    8
#define SPI_RX_SAMPLE_DLY_REG__RSVD_RX_SAMPLE_DLY__WIDTH                                    24
#define SPI_RX_SAMPLE_DLY_REG__RSVD_RX_SAMPLE_DLY__MASK                                     0xFFFFFF00U
#define SPI_RX_SAMPLE_DLY_REG__RSVD_RX_SAMPLE_DLY__READ(src)                                (((uint32_t)(src) & SPI_RX_SAMPLE_DLY_REG__RSVD_RX_SAMPLE_DLY__MASK) >> SPI_RX_RSVD_SAMPLE_DLY_REG__RX_SAMPLE_DLY__SHIFT)
#define SPI_RX_SAMPLE_DLY_REG__RSVD_RX_SAMPLE_DLY__WRITE(src)                               (((uint32_t)(src) << SPI_RX_SAMPLE_DLY_REG__RSVD_RX_SAMPLE_DLY__SHIFT) & SPI__RX_SAMPLE_DLY_REG__RSVD_RX_SAMPLE_DLY__MASK)

#endif /* __SPI_RX_SAMPLE_DLY_REG_MACRO__ */

/* macros for spi_spi_crtlr0_reg */
#ifndef __SPI_SPI_CTRLR0_REG_MACRO__
#define __SPI_SPI_CTRLR0_REG_MACRO__

/* macros for field TRANS_TYPE */
#define SPI_SPI_CTRLR0_REG__TRANS_TYPE__SHIFT                                               0
#define SPI_SPI_CTRLR0_REG__TRANS_TYPE__WIDTH                                               2
#define SPI_SPI_CTRLR0_REG__TRANS_TYPE__MASK                                                0x00000003U
#define SPI_SPI_CTRLR0_REG__TRANS_TYPE__READ(src)                                           (((uint32_t)(src) & SPI_SPI_CTRLR0_REG__TRANS_TYPE__MASK) >> SPI_SPI_CTRLR0_REG__TRANS_TYPE__SHIFT)
#define SPI_SPI_CTRLR0_REG__TRANS_TYPE__WRITE(src)                                          (((uint32_t)(src) << SPI_SPI_CTRLR0_REG__TRANS_TYPE__SHIFT) & SPI_RISR_REG__TRANS_TYPE__MASK)

/* macros for field ADDR_L */
#define SPI_SPI_CTRLR0_REG__ADDR_L__SHIFT                                                   2
#define SPI_SPI_CTRLR0_REG__ADDR_L__WIDTH                                                   4
#define SPI_SPI_CTRLR0_REG__ADDR_L__MASK                                                    0x0000003CU
#define SPI_SPI_CTRLR0_REG__ADDR_L__READ(src)                                               (((uint32_t)(src) & SPI_SPI_CTRLR0_REG__ADDR_L__MASK) >> SPI_SPI_CTRLR0_REG__ADDR_L__SHIFT)
#define SPI_SPI_CTRLR0_REG__ADDR_L__WRITE(src)                                              (((uint32_t)(src) << SPI_SPI_CTRLR0_REG__ADDR_L__SHIFT) & SPI_RISR_REG__ADDR_L__MASK)

/* macros for field RSVD_SPI_CTRLR0_6_7 */
#define SPI_SPI_CTRLR0_REG__RSVD_SPI_CTRLR0_6_7__SHIFT                                      6
#define SPI_SPI_CTRLR0_REG__RSVD_SPI_CTRLR0_6_7__WIDTH                                      2
#define SPI_SPI_CTRLR0_REG__RSVD_SPI_CTRLR0_6_7__MASK                                       0x0000000C0U
#define SPI_SPI_CTRLR0_REG__RSVD_SPI_CTRLR0_6_7__READ(src)                                  (((uint32_t)(src) & SPI_SPI_CTRLR0_REG__RSVD_SPI_CTRLR0_6_7__MASK) >> SPI_SPI_CTRLR0_REG__RSVD_SPI_CTRLR0_6_7__SHIFT)
#define SPI_SPI_CTRLR0_REG__RSVD_SPI_CTRLR0_6_7__WRITE(src)                                 (((uint32_t)(src) << SPI_SPI_CTRLR0_REG__RSVD_SPI_CTRLR0_6_7__SHIFT) & SPI_RISR_REG__RSVD_SPI_CTRLR0_6_7__MASK)

/* macros for field INST_L */
#define SPI_SPI_CTRLR0_REG__INST_L__SHIFT                                                   8
#define SPI_SPI_CTRLR0_REG__INST_L__WIDTH                                                   2
#define SPI_SPI_CTRLR0_REG__INST_L__MASK                                                    0x000000300U
#define SPI_SPI_CTRLR0_REG__INST_L__READ(src)                                               (((uint32_t)(src) & SPI_SPI_CTRLR0_REG__INST_L__MASK) >> SPI_SPI_CTRLR0_REG__INST_L__SHIFT)
#define SPI_SPI_CTRLR0_REG__INST_L__WRITE(src)                                              (((uint32_t)(src) << SPI_SPI_CTRLR0_REG__INST_L__SHIFT) & SPI_RISR_REG__INST_L__MASK)

/* macros for field RSVD_SPI_CTRLR0_10 */
#define SPI_SPI_CTRLR0_REG__RSVD_SPI_CTRLR0_10__SHIFT                                       10
#define SPI_SPI_CTRLR0_REG__RSVD_SPI_CTRLR0_10__WIDTH                                       1
#define SPI_SPI_CTRLR0_REG__RSVD_SPI_CTRLR0_10__MASK                                        0x000000400U
#define SPI_SPI_CTRLR0_REG__RSVD_SPI_CTRLR0_10__READ(src)                                   (((uint32_t)(src) & SPI_SPI_CTRLR0_REG__RSVD_SPI_CTRLR0_10__MASK) >> SPI_SPI_CTRLR0_REG__RSVD_SPI_CTRLR0_10__SHIFT)
#define SPI_SPI_CTRLR0_REG__RSVD_SPI_CTRLR0_10__WRITE(src)                                  (((uint32_t)(src) << SPI_SPI_CTRLR0_REG__RSVD_SPI_CTRLR0_10__SHIFT) & SPI_RISR_REG__RSVD_SPI_CTRLR0_10__MASK)

/* macros for field WAIT_CYCLES */
#define SPI_SPI_CTRLR0_REG__WAIT_CYCLES__SHIFT                                              11
#define SPI_SPI_CTRLR0_REG__WAIT_CYCLES__WIDTH                                              5
#define SPI_SPI_CTRLR0_REG__WAIT_CYCLES__MASK                                               0x00000F800U
#define SPI_SPI_CTRLR0_REG__WAIT_CYCLES__READ(src)                                          (((uint32_t)(src) & SPI_SPI_CTRLR0_REG__WAIT_CYCLES__MASK) >> SPI_SPI_CTRLR0_REG__WAIT_CYCLES__SHIFT)
#define SPI_SPI_CTRLR0_REG__WAIT_CYCLES__WRITE(src)                                         (((uint32_t)(src) << SPI_SPI_CTRLR0_REG__WAIT_CYCLES__SHIFT) & SPI_RISR_REG__WAIT_CYCLES__MASK)

/* macros for field SPI_DDR_EN */
#define SPI_SPI_CTRLR0_REG__SPI_DDR_EN__SHIFT                                               16
#define SPI_SPI_CTRLR0_REG__SPI_DDR_EN__WIDTH                                               1
#define SPI_SPI_CTRLR0_REG__SPI_DDR_EN__MASK                                                0x000010000U
#define SPI_SPI_CTRLR0_REG__SPI_DDR_EN__READ(src)                                           (((uint32_t)(src) & SPI_SPI_CTRLR0_REG__SPI_DDR_EN__MASK) >> SPI_SPI_CTRLR0_REG__SPI_DDR_EN__SHIFT)
#define SPI_SPI_CTRLR0_REG__SPI_DDR_EN__WRITE(src)                                          (((uint32_t)(src) << SPI_SPI_CTRLR0_REG__SPI_DDR_EN__SHIFT) & SPI_RISR_REG__SPI_DDR_EN__MASK)

/* macros for field INST_DDR_EN */
#define SPI_SPI_CTRLR0_REG__INST_DDR_EN__SHIFT                                              17
#define SPI_SPI_CTRLR0_REG__INST_DDR_EN__WIDTH                                              1
#define SPI_SPI_CTRLR0_REG__INST_DDR_EN__MASK                                               0x000020000U
#define SPI_SPI_CTRLR0_REG__INST_DDR_EN__READ(src)                                          (((uint32_t)(src) & SPI_SPI_CTRLR0_REG__INST_DDR_EN__MASK) >> SPI_SPI_CTRLR0_REG__INST_DDR_EN__SHIFT)
#define SPI_SPI_CTRLR0_REG__INST_DDR_EN__WRITE(src)                                         (((uint32_t)(src) << SPI_SPI_CTRLR0_REG__INST_DDR_EN__SHIFT) & SPI_RISR_REG__INST_DDR_EN__MASK)

/* macros for field SPI_RXDS_EN */
#define SPI_SPI_CTRLR0_REG__SPI_RXDS_EN__SHIFT                                              18
#define SPI_SPI_CTRLR0_REG__SPI_RXDS_EN__WIDTH                                              1
#define SPI_SPI_CTRLR0_REG__SPI_RXDS_EN__MASK                                               0x000040000U
#define SPI_SPI_CTRLR0_REG__SPI_RXDS_EN__READ(src)                                          (((uint32_t)(src) & SPI_SPI_CTRLR0_REG__SPI_RXDS_EN__MASK) >> SPI_SPI_CTRLR0_REG__SPI_RXDS_EN__SHIFT)
#define SPI_SPI_CTRLR0_REG__SPI_RXDS_EN__WRITE(src)                                         (((uint32_t)(src) << SPI_SPI_CTRLR0_REG__SPI_RXDS_EN__SHIFT) & SPI_RISR_REG__SPI_RXDS_EN__MASK)

/* macros for field RSVD_SPI_CRTLR0 */
#define SPI_SPI_CTRLR0_REG__RSVD_SPI_CRTLR0__SHIFT                                          19
#define SPI_SPI_CTRLR0_REG__RSVD_SPI_CRTLR0__WIDTH                                          13
#define SPI_SPI_CTRLR0_REG__RSVD_SPI_CRTLR0__MASK                                           0xFFFF80000U
#define SPI_SPI_CTRLR0_REG__RSVD_SPI_CRTLR0__READ(src)                                      (((uint32_t)(src) & SPI_SPI_CTRLR0_REG__RSVD_SPI_CRTLR0__MASK) >> SPI_SPI_CTRLR0_REG__RSVD_SPI_CRTLR0__SHIFT)
#define SPI_SPI_CTRLR0_REG__RSVD_SPI_CRTLR0__WRITE(src)                                     (((uint32_t)(src) << SPI_SPI_CTRLR0_REG__RSVD_SPI_CRTLR0__SHIFT) & SPI_RISR_REG__RSVD_SPI_CRTLR0__MASK)

#endif /* __SPI_SPI_CTRLR0_REG_MACRO__ */

/* macros for spi_txd_drive_edge_reg */
#ifndef __SPI_TXD_DRIVE_EDGE_REG_MACRO__
#define __SPI_TXD_DRIVE_EDGE_REG_MACRO__

/* macros for field RSD */
#define SPI_TXD_DRIVE_EDGE_REG__TDE__SHIFT                                                  0
#define SPI_TXD_DRIVE_EDGE_REG__TDE__WIDTH                                                  8
#define SPI_TXD_DRIVE_EDGE_REG__TDE__MASK                                                   0x000000FFU
#define SPI_TXD_DRIVE_EDGE_REG__TDE__READ(src)                                              (((uint32_t)(src) & SPI_TXD_DRIVE_EDGE_REG__TDE__MASK) >> SPI_TXD_DRIVE_EDGE_REG__TDE__SHIFT)
#define SPI_TXD_DRIVE_EDGE_REG__TDE__WRITE(src)                                             (((uint32_t)(src) << SPI_TXD_DRIVE_EDGE_REG__TDE_SHIFT) & SPI_TXD_DRIVE_EDGE_REG__TDE__MASK)

/* macros for field RSVD_TXD_DRIVE_EDGE */
#define SPI_TXD_DRIVE_EDGE_REG__RSVD_TXD_DRIVE_EDGE__SHIFT                                  8
#define SPI_TXD_DRIVE_EDGE_REG__RSVD_TXD_DRIVE_EDGE__WIDTH                                  31
#define SPI_TXD_DRIVE_EDGE_REG__RSVD_TXD_DRIVE_EDGE__MASK                                   0xFFFFFF00U
#define SPI_TXD_DRIVE_EDGE_REG__RSVD_TXD_DRIVE_EDGE__READ(src)                              (((uint32_t)(src) & SPI_TXD_DRIVE_EDGE_REG__RSVD_TXD_DRIVE_EDGE__MASK) >> SPI_RX_RSVD_SAMPLE_DLY_REG__TXD_DRIVE_EDGE__SHIFT)
#define SPI_TXD_DRIVE_EDGE_REG__RSVD_TXD_DRIVE_EDGE__WRITE(src)                             (((uint32_t)(src) << SPI_TXD_DRIVE_EDGE_REG__RSVD_TXD_DRIVE_EDGE__SHIFT) & SPI__TXD_DRIVE_EDGE_REG__RSVD_TXD_DRIVE_EDGE__MASK)

#endif /* __SPI_TXD_DRIVE_EDGE_REG_MACRO__ */

/* macros for spi_rsvd_reg */
#ifndef __SPI_RSVD_REG_MACRO__
#define __SPI_RSVD_REG_MACRO__

/* macros for field RSD */
#define SPI_RSVD_REG__RSVD__SHIFT                                                           0
#define SPI_RSVD_REG__RSVD__WIDTH                                                           8
#define SPI_RSVD_REG__RSVD__MASK                                                            0x000000FFU
#define SPI_RSVD_REG__RSVD__READ(src)                                                       (((uint32_t)(src) & SPI_RSVD_REG__RSVD__MASK) >> SPI_RSVD_REG__RSVD__SHIFT)
#define SPI_RSVD_REG__RSVD__WRITE(src)                                                      (((uint32_t)(src) << SPI_RSVD_REG__RSVD_SHIFT) & SPI_RSVD_REG__RSVD__MASK)

#endif /* __SPI_RSVD_REG_MACRO__ */



 


#endif /* __REG_SPI_REGS_MACRO_H__ */
