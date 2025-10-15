/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

#ifndef __PLL_MOVELLUS_REGS_MACRO_H__
#define __PLL_MOVELLUS_REGS_MACRO_H__

/* Macros for register REG00 (address=0x00) */
#ifndef __REG00_REG_MACRO__
#define __REG00_REG_MACRO__

/* Macros for field reserved (REG00[15:12]) */
#define REG00__RESERVED__SHIFT          			12
#define REG00__RESERVED__WIDTH          			4
#define REG00__RESERVED__MASK           			0xF000U
#define REG00__RESERVED__READ(src)      			(((uint32_t)(src) & 0xF000U) >> 12)
#define REG00__RESERVED__WRITE(src)     			(((uint32_t)(src) << 12) & 0xF000U)
#define REG00__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xF000U) | ((uint32_t(src) << 12) & 0xF000U))
#define REG00__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 12) ^ (dst)) & 0xF000U))
#define REG00__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xF000U) & (uint32_t(1) << 12))
#define REG00__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xF000U) | (uint32_t(0) << 12))
/* Macros for field open_loop_bypass_enable (REG00[11]) */
#define REG00__OPEN_LOOP_BYPASS_ENABLE__SHIFT          			11
#define REG00__OPEN_LOOP_BYPASS_ENABLE__WIDTH          			1
#define REG00__OPEN_LOOP_BYPASS_ENABLE__MASK           			0x0800U
#define REG00__OPEN_LOOP_BYPASS_ENABLE__READ(src)      			(((uint32_t)(src) & 0x0800U) >> 11)
#define REG00__OPEN_LOOP_BYPASS_ENABLE__WRITE(src)     			(((uint32_t)(src) << 11) & 0x0800U)
#define REG00__OPEN_LOOP_BYPASS_ENABLE__MODIFY(dst,src)			(dst) = (((dst) & ~0x0800U) | ((uint32_t(src) << 11) & 0x0800U))
#define REG00__OPEN_LOOP_BYPASS_ENABLE__VERIFY(dst,src)			(!((((uint32_t)(src) << 11) ^ (dst)) & 0x0800U))
#define REG00__OPEN_LOOP_BYPASS_ENABLE__SET(dst)       			(dst) = (((dst) & ~0x0800U) & (uint32_t(1) << 11))
#define REG00__OPEN_LOOP_BYPASS_ENABLE__CLR(dst)       			(dst) = (((dst) & ~0x0800U) | (uint32_t(0) << 11))
/* Macros for field dsm_dither_enable (REG00[10]) */
#define REG00__DSM_DITHER_ENABLE__SHIFT          			10
#define REG00__DSM_DITHER_ENABLE__WIDTH          			1
#define REG00__DSM_DITHER_ENABLE__MASK           			0x0400U
#define REG00__DSM_DITHER_ENABLE__READ(src)      			(((uint32_t)(src) & 0x0400U) >> 10)
#define REG00__DSM_DITHER_ENABLE__WRITE(src)     			(((uint32_t)(src) << 10) & 0x0400U)
#define REG00__DSM_DITHER_ENABLE__MODIFY(dst,src)			(dst) = (((dst) & ~0x0400U) | ((uint32_t(src) << 10) & 0x0400U))
#define REG00__DSM_DITHER_ENABLE__VERIFY(dst,src)			(!((((uint32_t)(src) << 10) ^ (dst)) & 0x0400U))
#define REG00__DSM_DITHER_ENABLE__SET(dst)       			(dst) = (((dst) & ~0x0400U) & (uint32_t(1) << 10))
#define REG00__DSM_DITHER_ENABLE__CLR(dst)       			(dst) = (((dst) & ~0x0400U) | (uint32_t(0) << 10))
/* Macros for field dsm_enable (REG00[9]) */
#define REG00__DSM_ENABLE__SHIFT          			9
#define REG00__DSM_ENABLE__WIDTH          			1
#define REG00__DSM_ENABLE__MASK           			0x0200U
#define REG00__DSM_ENABLE__READ(src)      			(((uint32_t)(src) & 0x0200U) >> 9)
#define REG00__DSM_ENABLE__WRITE(src)     			(((uint32_t)(src) << 9) & 0x0200U)
#define REG00__DSM_ENABLE__MODIFY(dst,src)			(dst) = (((dst) & ~0x0200U) | ((uint32_t(src) << 9) & 0x0200U))
#define REG00__DSM_ENABLE__VERIFY(dst,src)			(!((((uint32_t)(src) << 9) ^ (dst)) & 0x0200U))
#define REG00__DSM_ENABLE__SET(dst)       			(dst) = (((dst) & ~0x0200U) & (uint32_t(1) << 9))
#define REG00__DSM_ENABLE__CLR(dst)       			(dst) = (((dst) & ~0x0200U) | (uint32_t(0) << 9))
/* Macros for field freq_acq_enable (REG00[8]) */
#define REG00__FREQ_ACQ_ENABLE__SHIFT          			8
#define REG00__FREQ_ACQ_ENABLE__WIDTH          			1
#define REG00__FREQ_ACQ_ENABLE__MASK           			0x0100U
#define REG00__FREQ_ACQ_ENABLE__READ(src)      			(((uint32_t)(src) & 0x0100U) >> 8)
#define REG00__FREQ_ACQ_ENABLE__WRITE(src)     			(((uint32_t)(src) << 8) & 0x0100U)
#define REG00__FREQ_ACQ_ENABLE__MODIFY(dst,src)			(dst) = (((dst) & ~0x0100U) | ((uint32_t(src) << 8) & 0x0100U))
#define REG00__FREQ_ACQ_ENABLE__VERIFY(dst,src)			(!((((uint32_t)(src) << 8) ^ (dst)) & 0x0100U))
#define REG00__FREQ_ACQ_ENABLE__SET(dst)       			(dst) = (((dst) & ~0x0100U) & (uint32_t(1) << 8))
#define REG00__FREQ_ACQ_ENABLE__CLR(dst)       			(dst) = (((dst) & ~0x0100U) | (uint32_t(0) << 8))
/* Macros for field dco_normalization_enable (REG00[7]) */
#define REG00__DCO_NORMALIZATION_ENABLE__SHIFT          			7
#define REG00__DCO_NORMALIZATION_ENABLE__WIDTH          			1
#define REG00__DCO_NORMALIZATION_ENABLE__MASK           			0x0080U
#define REG00__DCO_NORMALIZATION_ENABLE__READ(src)      			(((uint32_t)(src) & 0x0080U) >> 7)
#define REG00__DCO_NORMALIZATION_ENABLE__WRITE(src)     			(((uint32_t)(src) << 7) & 0x0080U)
#define REG00__DCO_NORMALIZATION_ENABLE__MODIFY(dst,src)			(dst) = (((dst) & ~0x0080U) | ((uint32_t(src) << 7) & 0x0080U))
#define REG00__DCO_NORMALIZATION_ENABLE__VERIFY(dst,src)			(!((((uint32_t)(src) << 7) ^ (dst)) & 0x0080U))
#define REG00__DCO_NORMALIZATION_ENABLE__SET(dst)       			(dst) = (((dst) & ~0x0080U) & (uint32_t(1) << 7))
#define REG00__DCO_NORMALIZATION_ENABLE__CLR(dst)       			(dst) = (((dst) & ~0x0080U) | (uint32_t(0) << 7))
/* Macros for field dlf_enable (REG00[6]) */
#define REG00__DLF_ENABLE__SHIFT          			6
#define REG00__DLF_ENABLE__WIDTH          			1
#define REG00__DLF_ENABLE__MASK           			0x0040U
#define REG00__DLF_ENABLE__READ(src)      			(((uint32_t)(src) & 0x0040U) >> 6)
#define REG00__DLF_ENABLE__WRITE(src)     			(((uint32_t)(src) << 6) & 0x0040U)
#define REG00__DLF_ENABLE__MODIFY(dst,src)			(dst) = (((dst) & ~0x0040U) | ((uint32_t(src) << 6) & 0x0040U))
#define REG00__DLF_ENABLE__VERIFY(dst,src)			(!((((uint32_t)(src) << 6) ^ (dst)) & 0x0040U))
#define REG00__DLF_ENABLE__SET(dst)       			(dst) = (((dst) & ~0x0040U) & (uint32_t(1) << 6))
#define REG00__DLF_ENABLE__CLR(dst)       			(dst) = (((dst) & ~0x0040U) | (uint32_t(0) << 6))
/* Macros for field clkref_switch_enable (REG00[5]) */
#define REG00__CLKREF_SWITCH_ENABLE__SHIFT          			5
#define REG00__CLKREF_SWITCH_ENABLE__WIDTH          			1
#define REG00__CLKREF_SWITCH_ENABLE__MASK           			0x0020U
#define REG00__CLKREF_SWITCH_ENABLE__READ(src)      			(((uint32_t)(src) & 0x0020U) >> 5)
#define REG00__CLKREF_SWITCH_ENABLE__WRITE(src)     			(((uint32_t)(src) << 5) & 0x0020U)
#define REG00__CLKREF_SWITCH_ENABLE__MODIFY(dst,src)			(dst) = (((dst) & ~0x0020U) | ((uint32_t(src) << 5) & 0x0020U))
#define REG00__CLKREF_SWITCH_ENABLE__VERIFY(dst,src)			(!((((uint32_t)(src) << 5) ^ (dst)) & 0x0020U))
#define REG00__CLKREF_SWITCH_ENABLE__SET(dst)       			(dst) = (((dst) & ~0x0020U) & (uint32_t(1) << 5))
#define REG00__CLKREF_SWITCH_ENABLE__CLR(dst)       			(dst) = (((dst) & ~0x0020U) | (uint32_t(0) << 5))
/* Macros for field lock_timeout_enable (REG00[4]) */
#define REG00__LOCK_TIMEOUT_ENABLE__SHIFT          			4
#define REG00__LOCK_TIMEOUT_ENABLE__WIDTH          			1
#define REG00__LOCK_TIMEOUT_ENABLE__MASK           			0x0010U
#define REG00__LOCK_TIMEOUT_ENABLE__READ(src)      			(((uint32_t)(src) & 0x0010U) >> 4)
#define REG00__LOCK_TIMEOUT_ENABLE__WRITE(src)     			(((uint32_t)(src) << 4) & 0x0010U)
#define REG00__LOCK_TIMEOUT_ENABLE__MODIFY(dst,src)			(dst) = (((dst) & ~0x0010U) | ((uint32_t(src) << 4) & 0x0010U))
#define REG00__LOCK_TIMEOUT_ENABLE__VERIFY(dst,src)			(!((((uint32_t)(src) << 4) ^ (dst)) & 0x0010U))
#define REG00__LOCK_TIMEOUT_ENABLE__SET(dst)       			(dst) = (((dst) & ~0x0010U) & (uint32_t(1) << 4))
#define REG00__LOCK_TIMEOUT_ENABLE__CLR(dst)       			(dst) = (((dst) & ~0x0010U) | (uint32_t(0) << 4))
/* Macros for field pll_enable (REG00[3]) */
#define REG00__PLL_ENABLE__SHIFT          			3
#define REG00__PLL_ENABLE__WIDTH          			1
#define REG00__PLL_ENABLE__MASK           			0x0008U
#define REG00__PLL_ENABLE__READ(src)      			(((uint32_t)(src) & 0x0008U) >> 3)
#define REG00__PLL_ENABLE__WRITE(src)     			(((uint32_t)(src) << 3) & 0x0008U)
#define REG00__PLL_ENABLE__MODIFY(dst,src)			(dst) = (((dst) & ~0x0008U) | ((uint32_t(src) << 3) & 0x0008U))
#define REG00__PLL_ENABLE__VERIFY(dst,src)			(!((((uint32_t)(src) << 3) ^ (dst)) & 0x0008U))
#define REG00__PLL_ENABLE__SET(dst)       			(dst) = (((dst) & ~0x0008U) & (uint32_t(1) << 3))
#define REG00__PLL_ENABLE__CLR(dst)       			(dst) = (((dst) & ~0x0008U) | (uint32_t(0) << 3))
/* Macros for field clkpll_bypass (REG00[2]) */
#define REG00__CLKPLL_BYPASS__SHIFT          			2
#define REG00__CLKPLL_BYPASS__WIDTH          			1
#define REG00__CLKPLL_BYPASS__MASK           			0x0004U
#define REG00__CLKPLL_BYPASS__READ(src)      			(((uint32_t)(src) & 0x0004U) >> 2)
#define REG00__CLKPLL_BYPASS__WRITE(src)     			(((uint32_t)(src) << 2) & 0x0004U)
#define REG00__CLKPLL_BYPASS__MODIFY(dst,src)			(dst) = (((dst) & ~0x0004U) | ((uint32_t(src) << 2) & 0x0004U))
#define REG00__CLKPLL_BYPASS__VERIFY(dst,src)			(!((((uint32_t)(src) << 2) ^ (dst)) & 0x0004U))
#define REG00__CLKPLL_BYPASS__SET(dst)       			(dst) = (((dst) & ~0x0004U) & (uint32_t(1) << 2))
#define REG00__CLKPLL_BYPASS__CLR(dst)       			(dst) = (((dst) & ~0x0004U) | (uint32_t(0) << 2))
/* Macros for field reserved (REG00[1]) */
#define REG00__RESERVED1__SHIFT          			1
#define REG00__RESERVED1__WIDTH          			1
#define REG00__RESERVED1__MASK           			0x0002U
#define REG00__RESERVED1__READ(src)      			(((uint32_t)(src) & 0x0002U) >> 1)
#define REG00__RESERVED1__WRITE(src)     			(((uint32_t)(src) << 1) & 0x0002U)
#define REG00__RESERVED1__MODIFY(dst,src)			(dst) = (((dst) & ~0x0002U) | ((uint32_t(src) << 1) & 0x0002U))
#define REG00__RESERVED1__VERIFY(dst,src)			(!((((uint32_t)(src) << 1) ^ (dst)) & 0x0002U))
#define REG00__RESERVED1__SET(dst)       			(dst) = (((dst) & ~0x0002U) & (uint32_t(1) << 1))
#define REG00__RESERVED1__CLR(dst)       			(dst) = (((dst) & ~0x0002U) | (uint32_t(0) << 1))
/* Macros for field clkref_select (REG00[0]) */
#define REG00__CLKREF_SELECT__SHIFT          			0
#define REG00__CLKREF_SELECT__WIDTH          			1
#define REG00__CLKREF_SELECT__MASK           			0x0001U
#define REG00__CLKREF_SELECT__READ(src)      			(((uint32_t)(src) & 0x0001U) >> 0)
#define REG00__CLKREF_SELECT__WRITE(src)     			(((uint32_t)(src) << 0) & 0x0001U)
#define REG00__CLKREF_SELECT__MODIFY(dst,src)			(dst) = (((dst) & ~0x0001U) | ((uint32_t(src) << 0) & 0x0001U))
#define REG00__CLKREF_SELECT__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0x0001U))
#define REG00__CLKREF_SELECT__SET(dst)       			(dst) = (((dst) & ~0x0001U) & (uint32_t(1) << 0))
#define REG00__CLKREF_SELECT__CLR(dst)       			(dst) = (((dst) & ~0x0001U) | (uint32_t(0) << 0))

#endif /* __REG00_REG_MACRO__ */

/* Macros for register REG01 (address=0x01) */
#ifndef __REG01_REG_MACRO__
#define __REG01_REG_MACRO__

/* Macros for field reserved (REG01[15:9]) */
#define REG01__RESERVED__SHIFT          			9
#define REG01__RESERVED__WIDTH          			7
#define REG01__RESERVED__MASK           			0xFE00U
#define REG01__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFE00U) >> 9)
#define REG01__RESERVED__WRITE(src)     			(((uint32_t)(src) << 9) & 0xFE00U)
#define REG01__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFE00U) | ((uint32_t(src) << 9) & 0xFE00U))
#define REG01__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 9) ^ (dst)) & 0xFE00U))
#define REG01__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFE00U) & (uint32_t(1) << 9))
#define REG01__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFE00U) | (uint32_t(0) << 9))
/* Macros for field fcw_prediv (REG01[8:0]) */
#define REG01__FCW_PREDIV__SHIFT          			0
#define REG01__FCW_PREDIV__WIDTH          			9
#define REG01__FCW_PREDIV__MASK           			0x01FFU
#define REG01__FCW_PREDIV__READ(src)      			(((uint32_t)(src) & 0x01FFU) >> 0)
#define REG01__FCW_PREDIV__WRITE(src)     			(((uint32_t)(src) << 0) & 0x01FFU)
#define REG01__FCW_PREDIV__MODIFY(dst,src)			(dst) = (((dst) & ~0x01FFU) | ((uint32_t(src) << 0) & 0x01FFU))
#define REG01__FCW_PREDIV__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0x01FFU))
#define REG01__FCW_PREDIV__SET(dst)       			(dst) = (((dst) & ~0x01FFU) & (uint32_t(1) << 0))
#define REG01__FCW_PREDIV__CLR(dst)       			(dst) = (((dst) & ~0x01FFU) | (uint32_t(0) << 0))

#endif /* __REG01_REG_MACRO__ */

/* Macros for register REG02 (address=0x02) */
#ifndef __REG02_REG_MACRO__
#define __REG02_REG_MACRO__

/* Macros for field reserved (REG02[15:10]) */
#define REG02__RESERVED__SHIFT          			10
#define REG02__RESERVED__WIDTH          			6
#define REG02__RESERVED__MASK           			0xFC00U
#define REG02__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFC00U) >> 10)
#define REG02__RESERVED__WRITE(src)     			(((uint32_t)(src) << 10) & 0xFC00U)
#define REG02__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFC00U) | ((uint32_t(src) << 10) & 0xFC00U))
#define REG02__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 10) ^ (dst)) & 0xFC00U))
#define REG02__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFC00U) & (uint32_t(1) << 10))
#define REG02__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFC00U) | (uint32_t(0) << 10))
/* Macros for field fcw_int (REG02[9:0]) */
#define REG02__FCW_INT__SHIFT          			0
#define REG02__FCW_INT__WIDTH          			10
#define REG02__FCW_INT__MASK           			0x03FFU
#define REG02__FCW_INT__READ(src)      			(((uint32_t)(src) & 0x03FFU) >> 0)
#define REG02__FCW_INT__WRITE(src)     			(((uint32_t)(src) << 0) & 0x03FFU)
#define REG02__FCW_INT__MODIFY(dst,src)			(dst) = (((dst) & ~0x03FFU) | ((uint32_t(src) << 0) & 0x03FFU))
#define REG02__FCW_INT__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0x03FFU))
#define REG02__FCW_INT__SET(dst)       			(dst) = (((dst) & ~0x03FFU) & (uint32_t(1) << 0))
#define REG02__FCW_INT__CLR(dst)       			(dst) = (((dst) & ~0x03FFU) | (uint32_t(0) << 0))

#endif /* __REG02_REG_MACRO__ */

/* Macros for register REG03 (address=0x03) */
#ifndef __REG03_REG_MACRO__
#define __REG03_REG_MACRO__

/* Macros for field fcw_frac (REG03[15:0]) */
#define REG03__FCW_FRAC__SHIFT          			0
#define REG03__FCW_FRAC__WIDTH          			16
#define REG03__FCW_FRAC__MASK           			0xFFFFU
#define REG03__FCW_FRAC__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG03__FCW_FRAC__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG03__FCW_FRAC__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG03__FCW_FRAC__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG03__FCW_FRAC__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG03__FCW_FRAC__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG03_REG_MACRO__ */

/* Macros for register REG04 (address=0x04) */
#ifndef __REG04_REG_MACRO__
#define __REG04_REG_MACRO__

/* Macros for field reserved (REG04[15:14]) */
#define REG04__RESERVED__SHIFT          			14
#define REG04__RESERVED__WIDTH          			2
#define REG04__RESERVED__MASK           			0xC000U
#define REG04__RESERVED__READ(src)      			(((uint32_t)(src) & 0xC000U) >> 14)
#define REG04__RESERVED__WRITE(src)     			(((uint32_t)(src) << 14) & 0xC000U)
#define REG04__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xC000U) | ((uint32_t(src) << 14) & 0xC000U))
#define REG04__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 14) ^ (dst)) & 0xC000U))
#define REG04__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xC000U) & (uint32_t(1) << 14))
#define REG04__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xC000U) | (uint32_t(0) << 14))
/* Macros for field dlf_locked_kp (REG04[13:0]) */
#define REG04__DLF_LOCKED_KP__SHIFT          			0
#define REG04__DLF_LOCKED_KP__WIDTH          			14
#define REG04__DLF_LOCKED_KP__MASK           			0x3FFFU
#define REG04__DLF_LOCKED_KP__READ(src)      			(((uint32_t)(src) & 0x3FFFU) >> 0)
#define REG04__DLF_LOCKED_KP__WRITE(src)     			(((uint32_t)(src) << 0) & 0x3FFFU)
#define REG04__DLF_LOCKED_KP__MODIFY(dst,src)			(dst) = (((dst) & ~0x3FFFU) | ((uint32_t(src) << 0) & 0x3FFFU))
#define REG04__DLF_LOCKED_KP__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0x3FFFU))
#define REG04__DLF_LOCKED_KP__SET(dst)       			(dst) = (((dst) & ~0x3FFFU) & (uint32_t(1) << 0))
#define REG04__DLF_LOCKED_KP__CLR(dst)       			(dst) = (((dst) & ~0x3FFFU) | (uint32_t(0) << 0))

#endif /* __REG04_REG_MACRO__ */

/* Macros for register REG05 (address=0x05) */
#ifndef __REG05_REG_MACRO__
#define __REG05_REG_MACRO__

/* Macros for field reserved (REG05[15:14]) */
#define REG05__RESERVED__SHIFT          			14
#define REG05__RESERVED__WIDTH          			2
#define REG05__RESERVED__MASK           			0xC000U
#define REG05__RESERVED__READ(src)      			(((uint32_t)(src) & 0xC000U) >> 14)
#define REG05__RESERVED__WRITE(src)     			(((uint32_t)(src) << 14) & 0xC000U)
#define REG05__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xC000U) | ((uint32_t(src) << 14) & 0xC000U))
#define REG05__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 14) ^ (dst)) & 0xC000U))
#define REG05__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xC000U) & (uint32_t(1) << 14))
#define REG05__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xC000U) | (uint32_t(0) << 14))
/* Macros for field dlf_locked_ki (REG05[13:0]) */
#define REG05__DLF_LOCKED_KI__SHIFT          			0
#define REG05__DLF_LOCKED_KI__WIDTH          			14
#define REG05__DLF_LOCKED_KI__MASK           			0x3FFFU
#define REG05__DLF_LOCKED_KI__READ(src)      			(((uint32_t)(src) & 0x3FFFU) >> 0)
#define REG05__DLF_LOCKED_KI__WRITE(src)     			(((uint32_t)(src) << 0) & 0x3FFFU)
#define REG05__DLF_LOCKED_KI__MODIFY(dst,src)			(dst) = (((dst) & ~0x3FFFU) | ((uint32_t(src) << 0) & 0x3FFFU))
#define REG05__DLF_LOCKED_KI__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0x3FFFU))
#define REG05__DLF_LOCKED_KI__SET(dst)       			(dst) = (((dst) & ~0x3FFFU) & (uint32_t(1) << 0))
#define REG05__DLF_LOCKED_KI__CLR(dst)       			(dst) = (((dst) & ~0x3FFFU) | (uint32_t(0) << 0))

#endif /* __REG05_REG_MACRO__ */

/* Macros for register REG06 (address=0x06) */
#ifndef __REG06_REG_MACRO__
#define __REG06_REG_MACRO__

/* Macros for field reserved (REG06[15:14]) */
#define REG06__RESERVED__SHIFT          			14
#define REG06__RESERVED__WIDTH          			2
#define REG06__RESERVED__MASK           			0xC000U
#define REG06__RESERVED__READ(src)      			(((uint32_t)(src) & 0xC000U) >> 14)
#define REG06__RESERVED__WRITE(src)     			(((uint32_t)(src) << 14) & 0xC000U)
#define REG06__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xC000U) | ((uint32_t(src) << 14) & 0xC000U))
#define REG06__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 14) ^ (dst)) & 0xC000U))
#define REG06__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xC000U) & (uint32_t(1) << 14))
#define REG06__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xC000U) | (uint32_t(0) << 14))
/* Macros for field dlf_locked_kb (REG06[13:0]) */
#define REG06__DLF_LOCKED_KB__SHIFT          			0
#define REG06__DLF_LOCKED_KB__WIDTH          			14
#define REG06__DLF_LOCKED_KB__MASK           			0x3FFFU
#define REG06__DLF_LOCKED_KB__READ(src)      			(((uint32_t)(src) & 0x3FFFU) >> 0)
#define REG06__DLF_LOCKED_KB__WRITE(src)     			(((uint32_t)(src) << 0) & 0x3FFFU)
#define REG06__DLF_LOCKED_KB__MODIFY(dst,src)			(dst) = (((dst) & ~0x3FFFU) | ((uint32_t(src) << 0) & 0x3FFFU))
#define REG06__DLF_LOCKED_KB__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0x3FFFU))
#define REG06__DLF_LOCKED_KB__SET(dst)       			(dst) = (((dst) & ~0x3FFFU) & (uint32_t(1) << 0))
#define REG06__DLF_LOCKED_KB__CLR(dst)       			(dst) = (((dst) & ~0x3FFFU) | (uint32_t(0) << 0))

#endif /* __REG06_REG_MACRO__ */

/* Macros for register REG07 (address=0x07) */
#ifndef __REG07_REG_MACRO__
#define __REG07_REG_MACRO__

/* Macros for field reserved (REG07[15:14]) */
#define REG07__RESERVED__SHIFT          			14
#define REG07__RESERVED__WIDTH          			2
#define REG07__RESERVED__MASK           			0xC000U
#define REG07__RESERVED__READ(src)      			(((uint32_t)(src) & 0xC000U) >> 14)
#define REG07__RESERVED__WRITE(src)     			(((uint32_t)(src) << 14) & 0xC000U)
#define REG07__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xC000U) | ((uint32_t(src) << 14) & 0xC000U))
#define REG07__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 14) ^ (dst)) & 0xC000U))
#define REG07__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xC000U) & (uint32_t(1) << 14))
#define REG07__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xC000U) | (uint32_t(0) << 14))
/* Macros for field dlf_track_kp (REG07[13:0]) */
#define REG07__DLF_TRACK_KP__SHIFT          			0
#define REG07__DLF_TRACK_KP__WIDTH          			14
#define REG07__DLF_TRACK_KP__MASK           			0x3FFFU
#define REG07__DLF_TRACK_KP__READ(src)      			(((uint32_t)(src) & 0x3FFFU) >> 0)
#define REG07__DLF_TRACK_KP__WRITE(src)     			(((uint32_t)(src) << 0) & 0x3FFFU)
#define REG07__DLF_TRACK_KP__MODIFY(dst,src)			(dst) = (((dst) & ~0x3FFFU) | ((uint32_t(src) << 0) & 0x3FFFU))
#define REG07__DLF_TRACK_KP__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0x3FFFU))
#define REG07__DLF_TRACK_KP__SET(dst)       			(dst) = (((dst) & ~0x3FFFU) & (uint32_t(1) << 0))
#define REG07__DLF_TRACK_KP__CLR(dst)       			(dst) = (((dst) & ~0x3FFFU) | (uint32_t(0) << 0))

#endif /* __REG07_REG_MACRO__ */

/* Macros for register REG08 (address=0x08) */
#ifndef __REG08_REG_MACRO__
#define __REG08_REG_MACRO__

/* Macros for field reserved (REG08[15:14]) */
#define REG08__RESERVED__SHIFT          			14
#define REG08__RESERVED__WIDTH          			2
#define REG08__RESERVED__MASK           			0xC000U
#define REG08__RESERVED__READ(src)      			(((uint32_t)(src) & 0xC000U) >> 14)
#define REG08__RESERVED__WRITE(src)     			(((uint32_t)(src) << 14) & 0xC000U)
#define REG08__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xC000U) | ((uint32_t(src) << 14) & 0xC000U))
#define REG08__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 14) ^ (dst)) & 0xC000U))
#define REG08__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xC000U) & (uint32_t(1) << 14))
#define REG08__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xC000U) | (uint32_t(0) << 14))
/* Macros for field dlf_track_ki (REG08[13:0]) */
#define REG08__DLF_TRACK_KI__SHIFT          			0
#define REG08__DLF_TRACK_KI__WIDTH          			14
#define REG08__DLF_TRACK_KI__MASK           			0x3FFFU
#define REG08__DLF_TRACK_KI__READ(src)      			(((uint32_t)(src) & 0x3FFFU) >> 0)
#define REG08__DLF_TRACK_KI__WRITE(src)     			(((uint32_t)(src) << 0) & 0x3FFFU)
#define REG08__DLF_TRACK_KI__MODIFY(dst,src)			(dst) = (((dst) & ~0x3FFFU) | ((uint32_t(src) << 0) & 0x3FFFU))
#define REG08__DLF_TRACK_KI__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0x3FFFU))
#define REG08__DLF_TRACK_KI__SET(dst)       			(dst) = (((dst) & ~0x3FFFU) & (uint32_t(1) << 0))
#define REG08__DLF_TRACK_KI__CLR(dst)       			(dst) = (((dst) & ~0x3FFFU) | (uint32_t(0) << 0))

#endif /* __REG08_REG_MACRO__ */

/* Macros for register REG09 (address=0x09) */
#ifndef __REG09_REG_MACRO__
#define __REG09_REG_MACRO__

/* Macros for field reserved (REG09[15:14]) */
#define REG09__RESERVED__SHIFT          			14
#define REG09__RESERVED__WIDTH          			2
#define REG09__RESERVED__MASK           			0xC000U
#define REG09__RESERVED__READ(src)      			(((uint32_t)(src) & 0xC000U) >> 14)
#define REG09__RESERVED__WRITE(src)     			(((uint32_t)(src) << 14) & 0xC000U)
#define REG09__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xC000U) | ((uint32_t(src) << 14) & 0xC000U))
#define REG09__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 14) ^ (dst)) & 0xC000U))
#define REG09__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xC000U) & (uint32_t(1) << 14))
#define REG09__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xC000U) | (uint32_t(0) << 14))
/* Macros for field dlf_track_kb (REG09[13:0]) */
#define REG09__DLF_TRACK_KB__SHIFT          			0
#define REG09__DLF_TRACK_KB__WIDTH          			14
#define REG09__DLF_TRACK_KB__MASK           			0x3FFFU
#define REG09__DLF_TRACK_KB__READ(src)      			(((uint32_t)(src) & 0x3FFFU) >> 0)
#define REG09__DLF_TRACK_KB__WRITE(src)     			(((uint32_t)(src) << 0) & 0x3FFFU)
#define REG09__DLF_TRACK_KB__MODIFY(dst,src)			(dst) = (((dst) & ~0x3FFFU) | ((uint32_t(src) << 0) & 0x3FFFU))
#define REG09__DLF_TRACK_KB__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0x3FFFU))
#define REG09__DLF_TRACK_KB__SET(dst)       			(dst) = (((dst) & ~0x3FFFU) & (uint32_t(1) << 0))
#define REG09__DLF_TRACK_KB__CLR(dst)       			(dst) = (((dst) & ~0x3FFFU) | (uint32_t(0) << 0))

#endif /* __REG09_REG_MACRO__ */

/* Macros for register REG0A (address=0x0A) */
#ifndef __REG0A_REG_MACRO__
#define __REG0A_REG_MACRO__

/* Macros for field reserved (REG0A[15:9]) */
#define REG0A__RESERVED__SHIFT          			9
#define REG0A__RESERVED__WIDTH          			7
#define REG0A__RESERVED__MASK           			0xFE00U
#define REG0A__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFE00U) >> 9)
#define REG0A__RESERVED__WRITE(src)     			(((uint32_t)(src) << 9) & 0xFE00U)
#define REG0A__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFE00U) | ((uint32_t(src) << 9) & 0xFE00U))
#define REG0A__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 9) ^ (dst)) & 0xFE00U))
#define REG0A__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFE00U) & (uint32_t(1) << 9))
#define REG0A__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFE00U) | (uint32_t(0) << 9))
/* Macros for field fcw_lol (REG0A[8]) */
#define REG0A__FCW_LOL__SHIFT          			8
#define REG0A__FCW_LOL__WIDTH          			1
#define REG0A__FCW_LOL__MASK           			0x0100U
#define REG0A__FCW_LOL__READ(src)      			(((uint32_t)(src) & 0x0100U) >> 8)
#define REG0A__FCW_LOL__WRITE(src)     			(((uint32_t)(src) << 8) & 0x0100U)
#define REG0A__FCW_LOL__MODIFY(dst,src)			(dst) = (((dst) & ~0x0100U) | ((uint32_t(src) << 8) & 0x0100U))
#define REG0A__FCW_LOL__VERIFY(dst,src)			(!((((uint32_t)(src) << 8) ^ (dst)) & 0x0100U))
#define REG0A__FCW_LOL__SET(dst)       			(dst) = (((dst) & ~0x0100U) & (uint32_t(1) << 8))
#define REG0A__FCW_LOL__CLR(dst)       			(dst) = (((dst) & ~0x0100U) | (uint32_t(0) << 8))
/* Macros for field lock_count (REG0A[7:0]) */
#define REG0A__LOCK_COUNT__SHIFT          			0
#define REG0A__LOCK_COUNT__WIDTH          			8
#define REG0A__LOCK_COUNT__MASK           			0x00FFU
#define REG0A__LOCK_COUNT__READ(src)      			(((uint32_t)(src) & 0x00FFU) >> 0)
#define REG0A__LOCK_COUNT__WRITE(src)     			(((uint32_t)(src) << 0) & 0x00FFU)
#define REG0A__LOCK_COUNT__MODIFY(dst,src)			(dst) = (((dst) & ~0x00FFU) | ((uint32_t(src) << 0) & 0x00FFU))
#define REG0A__LOCK_COUNT__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0x00FFU))
#define REG0A__LOCK_COUNT__SET(dst)       			(dst) = (((dst) & ~0x00FFU) & (uint32_t(1) << 0))
#define REG0A__LOCK_COUNT__CLR(dst)       			(dst) = (((dst) & ~0x00FFU) | (uint32_t(0) << 0))

#endif /* __REG0A_REG_MACRO__ */

/* Macros for register REG0B (address=0x0B) */
#ifndef __REG0B_REG_MACRO__
#define __REG0B_REG_MACRO__

/* Macros for field lock_threshold (REG0B[15:0]) */
#define REG0B__LOCK_THRESHOLD__SHIFT          			0
#define REG0B__LOCK_THRESHOLD__WIDTH          			16
#define REG0B__LOCK_THRESHOLD__MASK           			0xFFFFU
#define REG0B__LOCK_THRESHOLD__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG0B__LOCK_THRESHOLD__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG0B__LOCK_THRESHOLD__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG0B__LOCK_THRESHOLD__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG0B__LOCK_THRESHOLD__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG0B__LOCK_THRESHOLD__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG0B_REG_MACRO__ */

/* Macros for register REG0C (address=0x0C) */
#ifndef __REG0C_REG_MACRO__
#define __REG0C_REG_MACRO__

/* Macros for field reserved (REG0C[15:12]) */
#define REG0C__RESERVED__SHIFT          			12
#define REG0C__RESERVED__WIDTH          			4
#define REG0C__RESERVED__MASK           			0xF000U
#define REG0C__RESERVED__READ(src)      			(((uint32_t)(src) & 0xF000U) >> 12)
#define REG0C__RESERVED__WRITE(src)     			(((uint32_t)(src) << 12) & 0xF000U)
#define REG0C__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xF000U) | ((uint32_t(src) << 12) & 0xF000U))
#define REG0C__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 12) ^ (dst)) & 0xF000U))
#define REG0C__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xF000U) & (uint32_t(1) << 12))
#define REG0C__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xF000U) | (uint32_t(0) << 12))
/* Macros for field dco_gain_to_ref_freq (REG0C[11:0]) */
#define REG0C__DCO_GAIN_TO_REF_FREQ__SHIFT          			0
#define REG0C__DCO_GAIN_TO_REF_FREQ__WIDTH          			12
#define REG0C__DCO_GAIN_TO_REF_FREQ__MASK           			0x0FFFU
#define REG0C__DCO_GAIN_TO_REF_FREQ__READ(src)      			(((uint32_t)(src) & 0x0FFFU) >> 0)
#define REG0C__DCO_GAIN_TO_REF_FREQ__WRITE(src)     			(((uint32_t)(src) << 0) & 0x0FFFU)
#define REG0C__DCO_GAIN_TO_REF_FREQ__MODIFY(dst,src)			(dst) = (((dst) & ~0x0FFFU) | ((uint32_t(src) << 0) & 0x0FFFU))
#define REG0C__DCO_GAIN_TO_REF_FREQ__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0x0FFFU))
#define REG0C__DCO_GAIN_TO_REF_FREQ__SET(dst)       			(dst) = (((dst) & ~0x0FFFU) & (uint32_t(1) << 0))
#define REG0C__DCO_GAIN_TO_REF_FREQ__CLR(dst)       			(dst) = (((dst) & ~0x0FFFU) | (uint32_t(0) << 0))

#endif /* __REG0C_REG_MACRO__ */

/* Macros for register REG0D (address=0x0D) */
#ifndef __REG0D_REG_MACRO__
#define __REG0D_REG_MACRO__

/* Macros for field reserved (REG0D[15:6]) */
#define REG0D__RESERVED__SHIFT          			6
#define REG0D__RESERVED__WIDTH          			10
#define REG0D__RESERVED__MASK           			0xFFC0U
#define REG0D__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFC0U) >> 6)
#define REG0D__RESERVED__WRITE(src)     			(((uint32_t)(src) << 6) & 0xFFC0U)
#define REG0D__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFC0U) | ((uint32_t(src) << 6) & 0xFFC0U))
#define REG0D__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 6) ^ (dst)) & 0xFFC0U))
#define REG0D__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFC0U) & (uint32_t(1) << 6))
#define REG0D__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFC0U) | (uint32_t(0) << 6))
/* Macros for field dsm_dither_width (REG0D[5:4]) */
#define REG0D__DSM_DITHER_WIDTH__SHIFT          			4
#define REG0D__DSM_DITHER_WIDTH__WIDTH          			2
#define REG0D__DSM_DITHER_WIDTH__MASK           			0x0030U
#define REG0D__DSM_DITHER_WIDTH__READ(src)      			(((uint32_t)(src) & 0x0030U) >> 4)
#define REG0D__DSM_DITHER_WIDTH__WRITE(src)     			(((uint32_t)(src) << 4) & 0x0030U)
#define REG0D__DSM_DITHER_WIDTH__MODIFY(dst,src)			(dst) = (((dst) & ~0x0030U) | ((uint32_t(src) << 4) & 0x0030U))
#define REG0D__DSM_DITHER_WIDTH__VERIFY(dst,src)			(!((((uint32_t)(src) << 4) ^ (dst)) & 0x0030U))
#define REG0D__DSM_DITHER_WIDTH__SET(dst)       			(dst) = (((dst) & ~0x0030U) & (uint32_t(1) << 4))
#define REG0D__DSM_DITHER_WIDTH__CLR(dst)       			(dst) = (((dst) & ~0x0030U) | (uint32_t(0) << 4))
/* Macros for field dsm_dither_position (REG0D[3:0]) */
#define REG0D__DSM_DITHER_POSITION__SHIFT          			0
#define REG0D__DSM_DITHER_POSITION__WIDTH          			4
#define REG0D__DSM_DITHER_POSITION__MASK           			0x000FU
#define REG0D__DSM_DITHER_POSITION__READ(src)      			(((uint32_t)(src) & 0x000FU) >> 0)
#define REG0D__DSM_DITHER_POSITION__WRITE(src)     			(((uint32_t)(src) << 0) & 0x000FU)
#define REG0D__DSM_DITHER_POSITION__MODIFY(dst,src)			(dst) = (((dst) & ~0x000FU) | ((uint32_t(src) << 0) & 0x000FU))
#define REG0D__DSM_DITHER_POSITION__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0x000FU))
#define REG0D__DSM_DITHER_POSITION__SET(dst)       			(dst) = (((dst) & ~0x000FU) & (uint32_t(1) << 0))
#define REG0D__DSM_DITHER_POSITION__CLR(dst)       			(dst) = (((dst) & ~0x000FU) | (uint32_t(0) << 0))

#endif /* __REG0D_REG_MACRO__ */

/* Macros for register REG0E (address=0x0E) */
#ifndef __REG0E_REG_MACRO__
#define __REG0E_REG_MACRO__

/* Macros for field reserved (REG0E[15:9]) */
#define REG0E__RESERVED__SHIFT          			9
#define REG0E__RESERVED__WIDTH          			7
#define REG0E__RESERVED__MASK           			0xFE00U
#define REG0E__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFE00U) >> 9)
#define REG0E__RESERVED__WRITE(src)     			(((uint32_t)(src) << 9) & 0xFE00U)
#define REG0E__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFE00U) | ((uint32_t(src) << 9) & 0xFE00U))
#define REG0E__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 9) ^ (dst)) & 0xFE00U))
#define REG0E__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFE00U) & (uint32_t(1) << 9))
#define REG0E__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFE00U) | (uint32_t(0) << 9))
/* Macros for field postdiv (REG0E[8:0]) */
#define REG0E__POSTDIV__SHIFT          			0
#define REG0E__POSTDIV__WIDTH          			9
#define REG0E__POSTDIV__MASK           			0x01FFU
#define REG0E__POSTDIV__READ(src)      			(((uint32_t)(src) & 0x01FFU) >> 0)
#define REG0E__POSTDIV__WRITE(src)     			(((uint32_t)(src) << 0) & 0x01FFU)
#define REG0E__POSTDIV__MODIFY(dst,src)			(dst) = (((dst) & ~0x01FFU) | ((uint32_t(src) << 0) & 0x01FFU))
#define REG0E__POSTDIV__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0x01FFU))
#define REG0E__POSTDIV__SET(dst)       			(dst) = (((dst) & ~0x01FFU) & (uint32_t(1) << 0))
#define REG0E__POSTDIV__CLR(dst)       			(dst) = (((dst) & ~0x01FFU) | (uint32_t(0) << 0))

#endif /* __REG0E_REG_MACRO__ */

/* Macros for register REG0F (address=0x0F) */
#ifndef __REG0F_REG_MACRO__
#define __REG0F_REG_MACRO__

/* Macros for field reserved (REG0F[15:0]) */
#define REG0F__RESERVED__SHIFT          			0
#define REG0F__RESERVED__WIDTH          			16
#define REG0F__RESERVED__MASK           			0xFFFFU
#define REG0F__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG0F__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG0F__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG0F__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG0F__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG0F__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG0F_REG_MACRO__ */

/* Macros for register REG10 (address=0x10) */
#ifndef __REG10_REG_MACRO__
#define __REG10_REG_MACRO__

/* Macros for field reserved (REG10[15:0]) */
#define REG10__RESERVED__SHIFT          			0
#define REG10__RESERVED__WIDTH          			16
#define REG10__RESERVED__MASK           			0xFFFFU
#define REG10__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG10__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG10__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG10__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG10__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG10__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG10_REG_MACRO__ */

/* Macros for register REG11 (address=0x11) */
#ifndef __REG11_REG_MACRO__
#define __REG11_REG_MACRO__

/* Macros for field reserved (REG11[15:0]) */
#define REG11__RESERVED__SHIFT          			0
#define REG11__RESERVED__WIDTH          			16
#define REG11__RESERVED__MASK           			0xFFFFU
#define REG11__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG11__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG11__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG11__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG11__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG11__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG11_REG_MACRO__ */

/* Macros for register REG12 (address=0x12) */
#ifndef __REG12_REG_MACRO__
#define __REG12_REG_MACRO__

/* Macros for field reserved (REG12[15:5]) */
#define REG12__RESERVED__SHIFT          			5
#define REG12__RESERVED__WIDTH          			11
#define REG12__RESERVED__MASK           			0xFFE0U
#define REG12__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFE0U) >> 5)
#define REG12__RESERVED__WRITE(src)     			(((uint32_t)(src) << 5) & 0xFFE0U)
#define REG12__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFE0U) | ((uint32_t(src) << 5) & 0xFFE0U))
#define REG12__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 5) ^ (dst)) & 0xFFE0U))
#define REG12__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFE0U) & (uint32_t(1) << 5))
#define REG12__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFE0U) | (uint32_t(0) << 5))
/* Macros for field postdiv0_bypass_clksel (REG12[4]) */
#define REG12__POSTDIV0_BYPASS_CLKSEL__SHIFT          			4
#define REG12__POSTDIV0_BYPASS_CLKSEL__WIDTH          			1
#define REG12__POSTDIV0_BYPASS_CLKSEL__MASK           			0x0010U
#define REG12__POSTDIV0_BYPASS_CLKSEL__READ(src)      			(((uint32_t)(src) & 0x0010U) >> 4)
#define REG12__POSTDIV0_BYPASS_CLKSEL__WRITE(src)     			(((uint32_t)(src) << 4) & 0x0010U)
#define REG12__POSTDIV0_BYPASS_CLKSEL__MODIFY(dst,src)			(dst) = (((dst) & ~0x0010U) | ((uint32_t(src) << 4) & 0x0010U))
#define REG12__POSTDIV0_BYPASS_CLKSEL__VERIFY(dst,src)			(!((((uint32_t)(src) << 4) ^ (dst)) & 0x0010U))
#define REG12__POSTDIV0_BYPASS_CLKSEL__SET(dst)       			(dst) = (((dst) & ~0x0010U) & (uint32_t(1) << 4))
#define REG12__POSTDIV0_BYPASS_CLKSEL__CLR(dst)       			(dst) = (((dst) & ~0x0010U) | (uint32_t(0) << 4))
/* Macros for field postdiv0_bypass_enable (REG12[3]) */
#define REG12__POSTDIV0_BYPASS_ENABLE__SHIFT          			3
#define REG12__POSTDIV0_BYPASS_ENABLE__WIDTH          			1
#define REG12__POSTDIV0_BYPASS_ENABLE__MASK           			0x0008U
#define REG12__POSTDIV0_BYPASS_ENABLE__READ(src)      			(((uint32_t)(src) & 0x0008U) >> 3)
#define REG12__POSTDIV0_BYPASS_ENABLE__WRITE(src)     			(((uint32_t)(src) << 3) & 0x0008U)
#define REG12__POSTDIV0_BYPASS_ENABLE__MODIFY(dst,src)			(dst) = (((dst) & ~0x0008U) | ((uint32_t(src) << 3) & 0x0008U))
#define REG12__POSTDIV0_BYPASS_ENABLE__VERIFY(dst,src)			(!((((uint32_t)(src) << 3) ^ (dst)) & 0x0008U))
#define REG12__POSTDIV0_BYPASS_ENABLE__SET(dst)       			(dst) = (((dst) & ~0x0008U) & (uint32_t(1) << 3))
#define REG12__POSTDIV0_BYPASS_ENABLE__CLR(dst)       			(dst) = (((dst) & ~0x0008U) | (uint32_t(0) << 3))
/* Macros for field postdiv0_powerdown (REG12[2]) */
#define REG12__POSTDIV0_POWERDOWN__SHIFT          			2
#define REG12__POSTDIV0_POWERDOWN__WIDTH          			1
#define REG12__POSTDIV0_POWERDOWN__MASK           			0x0004U
#define REG12__POSTDIV0_POWERDOWN__READ(src)      			(((uint32_t)(src) & 0x0004U) >> 2)
#define REG12__POSTDIV0_POWERDOWN__WRITE(src)     			(((uint32_t)(src) << 2) & 0x0004U)
#define REG12__POSTDIV0_POWERDOWN__MODIFY(dst,src)			(dst) = (((dst) & ~0x0004U) | ((uint32_t(src) << 2) & 0x0004U))
#define REG12__POSTDIV0_POWERDOWN__VERIFY(dst,src)			(!((((uint32_t)(src) << 2) ^ (dst)) & 0x0004U))
#define REG12__POSTDIV0_POWERDOWN__SET(dst)       			(dst) = (((dst) & ~0x0004U) & (uint32_t(1) << 2))
#define REG12__POSTDIV0_POWERDOWN__CLR(dst)       			(dst) = (((dst) & ~0x0004U) | (uint32_t(0) << 2))
/* Macros for field reserved (REG12[1:0]) */
#define REG12__RESERVED1__SHIFT          			0
#define REG12__RESERVED1__WIDTH          			2
#define REG12__RESERVED1__MASK           			0x0003U
#define REG12__RESERVED1__READ(src)      			(((uint32_t)(src) & 0x0003U) >> 0)
#define REG12__RESERVED1__WRITE(src)     			(((uint32_t)(src) << 0) & 0x0003U)
#define REG12__RESERVED1__MODIFY(dst,src)			(dst) = (((dst) & ~0x0003U) | ((uint32_t(src) << 0) & 0x0003U))
#define REG12__RESERVED1__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0x0003U))
#define REG12__RESERVED1__SET(dst)       			(dst) = (((dst) & ~0x0003U) & (uint32_t(1) << 0))
#define REG12__RESERVED1__CLR(dst)       			(dst) = (((dst) & ~0x0003U) | (uint32_t(0) << 0))

#endif /* __REG12_REG_MACRO__ */

/* Macros for register REG13 (address=0x13) */
#ifndef __REG13_REG_MACRO__
#define __REG13_REG_MACRO__

/* Macros for field reserved (REG13[15:0]) */
#define REG13__RESERVED__SHIFT          			0
#define REG13__RESERVED__WIDTH          			16
#define REG13__RESERVED__MASK           			0xFFFFU
#define REG13__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG13__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG13__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG13__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG13__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG13__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG13_REG_MACRO__ */

/* Macros for register REG14 (address=0x14) */
#ifndef __REG14_REG_MACRO__
#define __REG14_REG_MACRO__

/* Macros for field reserved (REG14[15:10]) */
#define REG14__RESERVED__SHIFT          			10
#define REG14__RESERVED__WIDTH          			6
#define REG14__RESERVED__MASK           			0xFC00U
#define REG14__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFC00U) >> 10)
#define REG14__RESERVED__WRITE(src)     			(((uint32_t)(src) << 10) & 0xFC00U)
#define REG14__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFC00U) | ((uint32_t(src) << 10) & 0xFC00U))
#define REG14__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 10) ^ (dst)) & 0xFC00U))
#define REG14__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFC00U) & (uint32_t(1) << 10))
#define REG14__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFC00U) | (uint32_t(0) << 10))
/* Macros for field open_loop_code (REG14[9:0]) */
#define REG14__OPEN_LOOP_CODE__SHIFT          			0
#define REG14__OPEN_LOOP_CODE__WIDTH          			10
#define REG14__OPEN_LOOP_CODE__MASK           			0x03FFU
#define REG14__OPEN_LOOP_CODE__READ(src)      			(((uint32_t)(src) & 0x03FFU) >> 0)
#define REG14__OPEN_LOOP_CODE__WRITE(src)     			(((uint32_t)(src) << 0) & 0x03FFU)
#define REG14__OPEN_LOOP_CODE__MODIFY(dst,src)			(dst) = (((dst) & ~0x03FFU) | ((uint32_t(src) << 0) & 0x03FFU))
#define REG14__OPEN_LOOP_CODE__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0x03FFU))
#define REG14__OPEN_LOOP_CODE__SET(dst)       			(dst) = (((dst) & ~0x03FFU) & (uint32_t(1) << 0))
#define REG14__OPEN_LOOP_CODE__CLR(dst)       			(dst) = (((dst) & ~0x03FFU) | (uint32_t(0) << 0))

#endif /* __REG14_REG_MACRO__ */

/* Macros for register REG15 (address=0x15) */
#ifndef __REG15_REG_MACRO__
#define __REG15_REG_MACRO__

/* Macros for field reserved (REG15[15:10]) */
#define REG15__RESERVED__SHIFT          			10
#define REG15__RESERVED__WIDTH          			6
#define REG15__RESERVED__MASK           			0xFC00U
#define REG15__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFC00U) >> 10)
#define REG15__RESERVED__WRITE(src)     			(((uint32_t)(src) << 10) & 0xFC00U)
#define REG15__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFC00U) | ((uint32_t(src) << 10) & 0xFC00U))
#define REG15__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 10) ^ (dst)) & 0xFC00U))
#define REG15__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFC00U) & (uint32_t(1) << 10))
#define REG15__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFC00U) | (uint32_t(0) << 10))
/* Macros for field ldo_test (REG15[9:8]) */
#define REG15__LDO_TEST__SHIFT          			8
#define REG15__LDO_TEST__WIDTH          			2
#define REG15__LDO_TEST__MASK           			0x0300U
#define REG15__LDO_TEST__READ(src)      			(((uint32_t)(src) & 0x0300U) >> 8)
#define REG15__LDO_TEST__WRITE(src)     			(((uint32_t)(src) << 8) & 0x0300U)
#define REG15__LDO_TEST__MODIFY(dst,src)			(dst) = (((dst) & ~0x0300U) | ((uint32_t(src) << 8) & 0x0300U))
#define REG15__LDO_TEST__VERIFY(dst,src)			(!((((uint32_t)(src) << 8) ^ (dst)) & 0x0300U))
#define REG15__LDO_TEST__SET(dst)       			(dst) = (((dst) & ~0x0300U) & (uint32_t(1) << 8))
#define REG15__LDO_TEST__CLR(dst)       			(dst) = (((dst) & ~0x0300U) | (uint32_t(0) << 8))
/* Macros for field ldo_tc_trim (REG15[7:6]) */
#define REG15__LDO_TC_TRIM__SHIFT          			6
#define REG15__LDO_TC_TRIM__WIDTH          			2
#define REG15__LDO_TC_TRIM__MASK           			0x00C0U
#define REG15__LDO_TC_TRIM__READ(src)      			(((uint32_t)(src) & 0x00C0U) >> 6)
#define REG15__LDO_TC_TRIM__WRITE(src)     			(((uint32_t)(src) << 6) & 0x00C0U)
#define REG15__LDO_TC_TRIM__MODIFY(dst,src)			(dst) = (((dst) & ~0x00C0U) | ((uint32_t(src) << 6) & 0x00C0U))
#define REG15__LDO_TC_TRIM__VERIFY(dst,src)			(!((((uint32_t)(src) << 6) ^ (dst)) & 0x00C0U))
#define REG15__LDO_TC_TRIM__SET(dst)       			(dst) = (((dst) & ~0x00C0U) & (uint32_t(1) << 6))
#define REG15__LDO_TC_TRIM__CLR(dst)       			(dst) = (((dst) & ~0x00C0U) | (uint32_t(0) << 6))
/* Macros for field Ldo_ref_trim (REG15[5:2]) */
#define REG15__LDO_REF_TRIM__SHIFT          			2
#define REG15__LDO_REF_TRIM__WIDTH          			4
#define REG15__LDO_REF_TRIM__MASK           			0x003CU
#define REG15__LDO_REF_TRIM__READ(src)      			(((uint32_t)(src) & 0x003CU) >> 2)
#define REG15__LDO_REF_TRIM__WRITE(src)     			(((uint32_t)(src) << 2) & 0x003CU)
#define REG15__LDO_REF_TRIM__MODIFY(dst,src)			(dst) = (((dst) & ~0x003CU) | ((uint32_t(src) << 2) & 0x003CU))
#define REG15__LDO_REF_TRIM__VERIFY(dst,src)			(!((((uint32_t)(src) << 2) ^ (dst)) & 0x003CU))
#define REG15__LDO_REF_TRIM__SET(dst)       			(dst) = (((dst) & ~0x003CU) & (uint32_t(1) << 2))
#define REG15__LDO_REF_TRIM__CLR(dst)       			(dst) = (((dst) & ~0x003CU) | (uint32_t(0) << 2))
/* Macros for field ldo_power_down (REG15[1]) */
#define REG15__LDO_POWER_DOWN__SHIFT          			1
#define REG15__LDO_POWER_DOWN__WIDTH          			1
#define REG15__LDO_POWER_DOWN__MASK           			0x0002U
#define REG15__LDO_POWER_DOWN__READ(src)      			(((uint32_t)(src) & 0x0002U) >> 1)
#define REG15__LDO_POWER_DOWN__WRITE(src)     			(((uint32_t)(src) << 1) & 0x0002U)
#define REG15__LDO_POWER_DOWN__MODIFY(dst,src)			(dst) = (((dst) & ~0x0002U) | ((uint32_t(src) << 1) & 0x0002U))
#define REG15__LDO_POWER_DOWN__VERIFY(dst,src)			(!((((uint32_t)(src) << 1) ^ (dst)) & 0x0002U))
#define REG15__LDO_POWER_DOWN__SET(dst)       			(dst) = (((dst) & ~0x0002U) & (uint32_t(1) << 1))
#define REG15__LDO_POWER_DOWN__CLR(dst)       			(dst) = (((dst) & ~0x0002U) | (uint32_t(0) << 1))
/* Macros for field ldo_bypass (REG15[0]) */
#define REG15__LDO_BYPASS__SHIFT          			0
#define REG15__LDO_BYPASS__WIDTH          			1
#define REG15__LDO_BYPASS__MASK           			0x0001U
#define REG15__LDO_BYPASS__READ(src)      			(((uint32_t)(src) & 0x0001U) >> 0)
#define REG15__LDO_BYPASS__WRITE(src)     			(((uint32_t)(src) << 0) & 0x0001U)
#define REG15__LDO_BYPASS__MODIFY(dst,src)			(dst) = (((dst) & ~0x0001U) | ((uint32_t(src) << 0) & 0x0001U))
#define REG15__LDO_BYPASS__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0x0001U))
#define REG15__LDO_BYPASS__SET(dst)       			(dst) = (((dst) & ~0x0001U) & (uint32_t(1) << 0))
#define REG15__LDO_BYPASS__CLR(dst)       			(dst) = (((dst) & ~0x0001U) | (uint32_t(0) << 0))

#endif /* __REG15_REG_MACRO__ */

/* Macros for register REG16 (address=0x16) */
#ifndef __REG16_REG_MACRO__
#define __REG16_REG_MACRO__

/* Macros for field ldo_reserved0 (REG16[15:0]) */
#define REG16__LDO_RESERVED0__SHIFT          			0
#define REG16__LDO_RESERVED0__WIDTH          			16
#define REG16__LDO_RESERVED0__MASK           			0xFFFFU
#define REG16__LDO_RESERVED0__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG16__LDO_RESERVED0__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG16__LDO_RESERVED0__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG16__LDO_RESERVED0__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG16__LDO_RESERVED0__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG16__LDO_RESERVED0__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG16_REG_MACRO__ */

/* Macros for register REG17 (address=0x17) */
#ifndef __REG17_REG_MACRO__
#define __REG17_REG_MACRO__

/* Macros for field ldo_reserved1 (REG17[15:0]) */
#define REG17__LDO_RESERVED1__SHIFT          			0
#define REG17__LDO_RESERVED1__WIDTH          			16
#define REG17__LDO_RESERVED1__MASK           			0xFFFFU
#define REG17__LDO_RESERVED1__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG17__LDO_RESERVED1__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG17__LDO_RESERVED1__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG17__LDO_RESERVED1__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG17__LDO_RESERVED1__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG17__LDO_RESERVED1__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG17_REG_MACRO__ */

/* Macros for register REG18 (address=0x18) */
#ifndef __REG18_REG_MACRO__
#define __REG18_REG_MACRO__

/* Macros for field ldo_reserved2 (REG18[15:0]) */
#define REG18__LDO_RESERVED2__SHIFT          			0
#define REG18__LDO_RESERVED2__WIDTH          			16
#define REG18__LDO_RESERVED2__MASK           			0xFFFFU
#define REG18__LDO_RESERVED2__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG18__LDO_RESERVED2__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG18__LDO_RESERVED2__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG18__LDO_RESERVED2__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG18__LDO_RESERVED2__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG18__LDO_RESERVED2__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG18_REG_MACRO__ */

/* Macros for register REG19 (address=0x19) */
#ifndef __REG19_REG_MACRO__
#define __REG19_REG_MACRO__

/* Macros for field ldo_reserved3 (REG19[15:0]) */
#define REG19__LDO_RESERVED3__SHIFT          			0
#define REG19__LDO_RESERVED3__WIDTH          			16
#define REG19__LDO_RESERVED3__MASK           			0xFFFFU
#define REG19__LDO_RESERVED3__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG19__LDO_RESERVED3__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG19__LDO_RESERVED3__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG19__LDO_RESERVED3__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG19__LDO_RESERVED3__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG19__LDO_RESERVED3__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG19_REG_MACRO__ */

/* Macros for register REG1A (address=0x1A) */
#ifndef __REG1A_REG_MACRO__
#define __REG1A_REG_MACRO__

/* Macros for field ldo_reserved4 (REG1A[15:0]) */
#define REG1A__LDO_RESERVED4__SHIFT          			0
#define REG1A__LDO_RESERVED4__WIDTH          			16
#define REG1A__LDO_RESERVED4__MASK           			0xFFFFU
#define REG1A__LDO_RESERVED4__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG1A__LDO_RESERVED4__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG1A__LDO_RESERVED4__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG1A__LDO_RESERVED4__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG1A__LDO_RESERVED4__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG1A__LDO_RESERVED4__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG1A_REG_MACRO__ */

/* Macros for register REG1B (address=0x1B) */
#ifndef __REG1B_REG_MACRO__
#define __REG1B_REG_MACRO__

/* Macros for field ldo_reserved5 (REG1B[15:0]) */
#define REG1B__LDO_RESERVED5__SHIFT          			0
#define REG1B__LDO_RESERVED5__WIDTH          			16
#define REG1B__LDO_RESERVED5__MASK           			0xFFFFU
#define REG1B__LDO_RESERVED5__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG1B__LDO_RESERVED5__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG1B__LDO_RESERVED5__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG1B__LDO_RESERVED5__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG1B__LDO_RESERVED5__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG1B__LDO_RESERVED5__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG1B_REG_MACRO__ */

/* Macros for register REG1C (address=0x1C) */
#ifndef __REG1C_REG_MACRO__
#define __REG1C_REG_MACRO__

/* Macros for field ldo_reserved6 (REG1C[15:0]) */
#define REG1C__LDO_RESERVED6__SHIFT          			0
#define REG1C__LDO_RESERVED6__WIDTH          			16
#define REG1C__LDO_RESERVED6__MASK           			0xFFFFU
#define REG1C__LDO_RESERVED6__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG1C__LDO_RESERVED6__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG1C__LDO_RESERVED6__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG1C__LDO_RESERVED6__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG1C__LDO_RESERVED6__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG1C__LDO_RESERVED6__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG1C_REG_MACRO__ */

/* Macros for register REG1D (address=0x1D) */
#ifndef __REG1D_REG_MACRO__
#define __REG1D_REG_MACRO__

/* Macros for field ldo_reserved7 (REG1D[15:0]) */
#define REG1D__LDO_RESERVED7__SHIFT          			0
#define REG1D__LDO_RESERVED7__WIDTH          			16
#define REG1D__LDO_RESERVED7__MASK           			0xFFFFU
#define REG1D__LDO_RESERVED7__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG1D__LDO_RESERVED7__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG1D__LDO_RESERVED7__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG1D__LDO_RESERVED7__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG1D__LDO_RESERVED7__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG1D__LDO_RESERVED7__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG1D_REG_MACRO__ */

/* Macros for register REG1E (address=0x1E) */
#ifndef __REG1E_REG_MACRO__
#define __REG1E_REG_MACRO__

/* Macros for field reserved (REG1E[15:0]) */
#define REG1E__RESERVED__SHIFT          			0
#define REG1E__RESERVED__WIDTH          			16
#define REG1E__RESERVED__MASK           			0xFFFFU
#define REG1E__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG1E__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG1E__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG1E__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG1E__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG1E__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG1E_REG_MACRO__ */

/* Macros for register REG1F (address=0x1F) */
#ifndef __REG1F_REG_MACRO__
#define __REG1F_REG_MACRO__

/* Macros for field reserved (REG1F[15:0]) */
#define REG1F__RESERVED__SHIFT          			0
#define REG1F__RESERVED__WIDTH          			16
#define REG1F__RESERVED__MASK           			0xFFFFU
#define REG1F__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG1F__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG1F__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG1F__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG1F__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG1F__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG1F_REG_MACRO__ */

/* Macros for register REG20 (address=0x20) */
#ifndef __REG20_REG_MACRO__
#define __REG20_REG_MACRO__

/* Macros for field reserved (REG20[15:0]) */
#define REG20__RESERVED__SHIFT          			0
#define REG20__RESERVED__WIDTH          			16
#define REG20__RESERVED__MASK           			0xFFFFU
#define REG20__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG20__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG20__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG20__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG20__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG20__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG20_REG_MACRO__ */

/* Macros for register REG21 (address=0x21) */
#ifndef __REG21_REG_MACRO__
#define __REG21_REG_MACRO__

/* Macros for field reserved (REG21[15:0]) */
#define REG21__RESERVED__SHIFT          			0
#define REG21__RESERVED__WIDTH          			16
#define REG21__RESERVED__MASK           			0xFFFFU
#define REG21__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG21__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG21__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG21__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG21__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG21__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG21_REG_MACRO__ */

/* Macros for register REG22 (address=0x22) */
#ifndef __REG22_REG_MACRO__
#define __REG22_REG_MACRO__

/* Macros for field reserved (REG22[15:0]) */
#define REG22__RESERVED__SHIFT          			0
#define REG22__RESERVED__WIDTH          			16
#define REG22__RESERVED__MASK           			0xFFFFU
#define REG22__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG22__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG22__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG22__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG22__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG22__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG22_REG_MACRO__ */

/* Macros for register REG23 (address=0x23) */
#ifndef __REG23_REG_MACRO__
#define __REG23_REG_MACRO__

/* Macros for field reserved (REG23[15:0]) */
#define REG23__RESERVED__SHIFT          			0
#define REG23__RESERVED__WIDTH          			16
#define REG23__RESERVED__MASK           			0xFFFFU
#define REG23__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG23__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG23__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG23__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG23__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG23__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG23_REG_MACRO__ */

/* Macros for register REG24 (address=0x24) */
#ifndef __REG24_REG_MACRO__
#define __REG24_REG_MACRO__

/* Macros for field reserved (REG24[15:0]) */
#define REG24__RESERVED__SHIFT          			0
#define REG24__RESERVED__WIDTH          			16
#define REG24__RESERVED__MASK           			0xFFFFU
#define REG24__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG24__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG24__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG24__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG24__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG24__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG24_REG_MACRO__ */

/* Macros for register REG25 (address=0x25) */
#ifndef __REG25_REG_MACRO__
#define __REG25_REG_MACRO__

/* Macros for field reserved (REG25[15:0]) */
#define REG25__RESERVED__SHIFT          			0
#define REG25__RESERVED__WIDTH          			16
#define REG25__RESERVED__MASK           			0xFFFFU
#define REG25__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG25__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG25__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG25__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG25__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG25__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG25_REG_MACRO__ */

/* Macros for register REG26 (address=0x26) */
#ifndef __REG26_REG_MACRO__
#define __REG26_REG_MACRO__

/* Macros for field reserved (REG26[15:0]) */
#define REG26__RESERVED__SHIFT          			0
#define REG26__RESERVED__WIDTH          			16
#define REG26__RESERVED__MASK           			0xFFFFU
#define REG26__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG26__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG26__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG26__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG26__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG26__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG26_REG_MACRO__ */

/* Macros for register REG27 (address=0x27) */
#ifndef __REG27_REG_MACRO__
#define __REG27_REG_MACRO__

/* Macros for field reserved (REG27[15:0]) */
#define REG27__RESERVED__SHIFT          			0
#define REG27__RESERVED__WIDTH          			16
#define REG27__RESERVED__MASK           			0xFFFFU
#define REG27__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG27__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG27__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG27__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG27__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG27__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG27_REG_MACRO__ */

/* Macros for register REG28 (address=0x28) */
#ifndef __REG28_REG_MACRO__
#define __REG28_REG_MACRO__

/* Macros for field reserved (REG28[15:0]) */
#define REG28__RESERVED__SHIFT          			0
#define REG28__RESERVED__WIDTH          			16
#define REG28__RESERVED__MASK           			0xFFFFU
#define REG28__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG28__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG28__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG28__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG28__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG28__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG28_REG_MACRO__ */

/* Macros for register REG29 (address=0x29) */
#ifndef __REG29_REG_MACRO__
#define __REG29_REG_MACRO__

/* Macros for field reserved (REG29[15:0]) */
#define REG29__RESERVED__SHIFT          			0
#define REG29__RESERVED__WIDTH          			16
#define REG29__RESERVED__MASK           			0xFFFFU
#define REG29__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG29__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG29__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG29__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG29__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG29__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG29_REG_MACRO__ */

/* Macros for register REG2A (address=0x2A) */
#ifndef __REG2A_REG_MACRO__
#define __REG2A_REG_MACRO__

/* Macros for field reserved (REG2A[15:0]) */
#define REG2A__RESERVED__SHIFT          			0
#define REG2A__RESERVED__WIDTH          			16
#define REG2A__RESERVED__MASK           			0xFFFFU
#define REG2A__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG2A__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG2A__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG2A__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG2A__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG2A__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG2A_REG_MACRO__ */

/* Macros for register REG2B (address=0x29) */
#ifndef __REG2B_REG_MACRO__
#define __REG2B_REG_MACRO__

/* Macros for field reserved (REG2B[15:0]) */
#define REG2B__RESERVED__SHIFT          			0
#define REG2B__RESERVED__WIDTH          			16
#define REG2B__RESERVED__MASK           			0xFFFFU
#define REG2B__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG2B__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG2B__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG2B__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG2B__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG2B__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG2B_REG_MACRO__ */

/* Macros for register REG2C (address=0x29) */
#ifndef __REG2C_REG_MACRO__
#define __REG2C_REG_MACRO__

/* Macros for field reserved (REG2C[15:0]) */
#define REG2C__RESERVED__SHIFT          			0
#define REG2C__RESERVED__WIDTH          			16
#define REG2C__RESERVED__MASK           			0xFFFFU
#define REG2C__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG2C__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG2C__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG2C__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG2C__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG2C__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG2C_REG_MACRO__ */

/* Macros for register REG2D (address=0x29) */
#ifndef __REG2D_REG_MACRO__
#define __REG2D_REG_MACRO__

/* Macros for field reserved (REG2D[15:0]) */
#define REG2D__RESERVED__SHIFT          			0
#define REG2D__RESERVED__WIDTH          			16
#define REG2D__RESERVED__MASK           			0xFFFFU
#define REG2D__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG2D__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG2D__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG2D__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG2D__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG2D__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG2D_REG_MACRO__ */

/* Macros for register REG2E (address=0x29) */
#ifndef __REG2E_REG_MACRO__
#define __REG2E_REG_MACRO__

/* Macros for field reserved (REG2E[15:0]) */
#define REG2E__RESERVED__SHIFT          			0
#define REG2E__RESERVED__WIDTH          			16
#define REG2E__RESERVED__MASK           			0xFFFFU
#define REG2E__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG2E__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG2E__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG2E__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG2E__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG2E__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG2E_REG_MACRO__ */

/* Macros for register REG2F (address=0x29) */
#ifndef __REG2F_REG_MACRO__
#define __REG2F_REG_MACRO__

/* Macros for field reserved (REG2F[15:0]) */
#define REG2F__RESERVED__SHIFT          			0
#define REG2F__RESERVED__WIDTH          			16
#define REG2F__RESERVED__MASK           			0xFFFFU
#define REG2F__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG2F__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG2F__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG2F__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG2F__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG2F__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG2F_REG_MACRO__ */

/* Macros for register REG30 (address=0x30) */
#ifndef __REG30_REG_MACRO__
#define __REG30_REG_MACRO__

/* Macros for field reserved (REG30[15:0]) */
#define REG30__RESERVED__SHIFT          			0
#define REG30__RESERVED__WIDTH          			16
#define REG30__RESERVED__MASK           			0xFFFFU
#define REG30__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG30__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG30__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG30__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG30__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG30__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG30_REG_MACRO__ */

/* Macros for register REG31 (address=0x31) */
#ifndef __REG31_REG_MACRO__
#define __REG31_REG_MACRO__

/* Macros for field reserved (REG31[15:0]) */
#define REG31__RESERVED__SHIFT          			0
#define REG31__RESERVED__WIDTH          			16
#define REG31__RESERVED__MASK           			0xFFFFU
#define REG31__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG31__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG31__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG31__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG31__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG31__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG31_REG_MACRO__ */

/* Macros for register REG32 (address=0x32) */
#ifndef __REG32_REG_MACRO__
#define __REG32_REG_MACRO__

/* Macros for field reserved (REG32[15:0]) */
#define REG32__RESERVED__SHIFT          			0
#define REG32__RESERVED__WIDTH          			16
#define REG32__RESERVED__MASK           			0xFFFFU
#define REG32__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG32__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG32__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG32__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG32__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG32__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG32_REG_MACRO__ */

/* Macros for register REG33 (address=0x33) */
#ifndef __REG33_REG_MACRO__
#define __REG33_REG_MACRO__

/* Macros for field reserved (REG33[15:0]) */
#define REG33__RESERVED__SHIFT          			0
#define REG33__RESERVED__WIDTH          			16
#define REG33__RESERVED__MASK           			0xFFFFU
#define REG33__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG33__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG33__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG33__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG33__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG33__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG33_REG_MACRO__ */

/* Macros for register REG34 (address=0x34) */
#ifndef __REG34_REG_MACRO__
#define __REG34_REG_MACRO__

/* Macros for field reserved (REG34[15:0]) */
#define REG34__RESERVED__SHIFT          			0
#define REG34__RESERVED__WIDTH          			16
#define REG34__RESERVED__MASK           			0xFFFFU
#define REG34__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG34__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG34__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG34__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG34__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG34__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG34_REG_MACRO__ */

/* Macros for register REG35 (address=0x35) */
#ifndef __REG35_REG_MACRO__
#define __REG35_REG_MACRO__

/* Macros for field reserved (REG35[15:0]) */
#define REG35__RESERVED__SHIFT          			0
#define REG35__RESERVED__WIDTH          			16
#define REG35__RESERVED__MASK           			0xFFFFU
#define REG35__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG35__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG35__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG35__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG35__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG35__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG35_REG_MACRO__ */

/* Macros for register REG36 (address=0x36) */
#ifndef __REG36_REG_MACRO__
#define __REG36_REG_MACRO__

/* Macros for field reserved (REG36[15:0]) */
#define REG36__RESERVED__SHIFT          			0
#define REG36__RESERVED__WIDTH          			16
#define REG36__RESERVED__MASK           			0xFFFFU
#define REG36__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG36__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG36__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG36__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG36__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG36__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG36_REG_MACRO__ */

/* Macros for register REG37 (address=0x37) */
#ifndef __REG37_REG_MACRO__
#define __REG37_REG_MACRO__

/* Macros for field reserved (REG37[15:0]) */
#define REG37__RESERVED__SHIFT          			0
#define REG37__RESERVED__WIDTH          			16
#define REG37__RESERVED__MASK           			0xFFFFU
#define REG37__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG37__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG37__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG37__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG37__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG37__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG37_REG_MACRO__ */

/* Macros for register REG38 (address=0x38) */
#ifndef __REG38_REG_MACRO__
#define __REG38_REG_MACRO__

/* Macros for field reserved (REG38[15:1]) */
#define REG38__RESERVED__SHIFT          			1
#define REG38__RESERVED__WIDTH          			15
#define REG38__RESERVED__MASK           			0xFFFEU
#define REG38__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFEU) >> 1)
#define REG38__RESERVED__WRITE(src)     			(((uint32_t)(src) << 1) & 0xFFFEU)
#define REG38__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFEU) | ((uint32_t(src) << 1) & 0xFFFEU))
#define REG38__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 1) ^ (dst)) & 0xFFFEU))
#define REG38__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFEU) & (uint32_t(1) << 1))
#define REG38__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFEU) | (uint32_t(0) << 1))
/* Macros for field reg_update_strobe (REG38[0]) */
#define REG38__REG_UPDATE_STROBE__SHIFT          			0
#define REG38__REG_UPDATE_STROBE__WIDTH          			1
#define REG38__REG_UPDATE_STROBE__MASK           			0x0001U
#define REG38__REG_UPDATE_STROBE__READ(src)      			(((uint32_t)(src) & 0x0001U) >> 0)
#define REG38__REG_UPDATE_STROBE__WRITE(src)     			(((uint32_t)(src) << 0) & 0x0001U)
#define REG38__REG_UPDATE_STROBE__MODIFY(dst,src)			(dst) = (((dst) & ~0x0001U) | ((uint32_t(src) << 0) & 0x0001U))
#define REG38__REG_UPDATE_STROBE__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0x0001U))
#define REG38__REG_UPDATE_STROBE__SET(dst)       			(dst) = (((dst) & ~0x0001U) & (uint32_t(1) << 0))
#define REG38__REG_UPDATE_STROBE__CLR(dst)       			(dst) = (((dst) & ~0x0001U) | (uint32_t(0) << 0))

#endif /* __REG38_REG_MACRO__ */

/* Macros for register REG39 (address=0x39) */
#ifndef __REG39_REG_MACRO__
#define __REG39_REG_MACRO__

/* Macros for field reserved (REG39[15:1]) */
#define REG39__RESERVED__SHIFT          			1
#define REG39__RESERVED__WIDTH          			15
#define REG39__RESERVED__MASK           			0xFFFEU
#define REG39__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFEU) >> 1)
#define REG39__RESERVED__WRITE(src)     			(((uint32_t)(src) << 1) & 0xFFFEU)
#define REG39__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFEU) | ((uint32_t(src) << 1) & 0xFFFEU))
#define REG39__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 1) ^ (dst)) & 0xFFFEU))
#define REG39__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFEU) & (uint32_t(1) << 1))
#define REG39__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFEU) | (uint32_t(0) << 1))
/* Macros for field lock_detect_status (REG39[0]) */
#define REG39__LOCK_DETECT_STATUS__SHIFT          			0
#define REG39__LOCK_DETECT_STATUS__WIDTH          			1
#define REG39__LOCK_DETECT_STATUS__MASK           			0x0001U
#define REG39__LOCK_DETECT_STATUS__READ(src)      			(((uint32_t)(src) & 0x0001U) >> 0)
#define REG39__LOCK_DETECT_STATUS__WRITE(src)     			(((uint32_t)(src) << 0) & 0x0001U)
#define REG39__LOCK_DETECT_STATUS__MODIFY(dst,src)			(dst) = (((dst) & ~0x0001U) | ((uint32_t(src) << 0) & 0x0001U))
#define REG39__LOCK_DETECT_STATUS__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0x0001U))
#define REG39__LOCK_DETECT_STATUS__SET(dst)       			(dst) = (((dst) & ~0x0001U) & (uint32_t(1) << 0))
#define REG39__LOCK_DETECT_STATUS__CLR(dst)       			(dst) = (((dst) & ~0x0001U) | (uint32_t(0) << 0))

#endif /* __REG39_REG_MACRO__ */

/* Macros for register REG3A (address=0x3A) */
#ifndef __REG3A_REG_MACRO__
#define __REG3A_REG_MACRO__

/* Macros for field reserved (REG3A[15:0]) */
#define REG3A__RESERVED__SHIFT          			0
#define REG3A__RESERVED__WIDTH          			16
#define REG3A__RESERVED__MASK           			0xFFFFU
#define REG3A__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG3A__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG3A__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG3A__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG3A__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG3A__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG3A_REG_MACRO__ */

/* Macros for register REG3B (address=0x3B) */
#ifndef __REG3B_REG_MACRO__
#define __REG3B_REG_MACRO__

/* Macros for field reserved (REG3B[15:0]) */
#define REG3B__RESERVED__SHIFT          			0
#define REG3B__RESERVED__WIDTH          			16
#define REG3B__RESERVED__MASK           			0xFFFFU
#define REG3B__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG3B__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG3B__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG3B__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG3B__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG3B__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG3B_REG_MACRO__ */

/* Macros for register REG3C (address=0x3C) */
#ifndef __REG3C_REG_MACRO__
#define __REG3C_REG_MACRO__

/* Macros for field reserved (REG3C[15:0]) */
#define REG3C__RESERVED__SHIFT          			0
#define REG3C__RESERVED__WIDTH          			16
#define REG3C__RESERVED__MASK           			0xFFFFU
#define REG3C__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG3C__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG3C__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG3C__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG3C__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG3C__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG3C_REG_MACRO__ */

/* Macros for register REG3D (address=0x3D) */
#ifndef __REG3D_REG_MACRO__
#define __REG3D_REG_MACRO__

/* Macros for field reserved (REG3D[15:0]) */
#define REG3D__RESERVED__SHIFT          			0
#define REG3D__RESERVED__WIDTH          			16
#define REG3D__RESERVED__MASK           			0xFFFFU
#define REG3D__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG3D__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG3D__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG3D__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG3D__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG3D__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG3D_REG_MACRO__ */

/* Macros for register REG3E (address=0x3E) */
#ifndef __REG3E_REG_MACRO__
#define __REG3E_REG_MACRO__

/* Macros for field reserved (REG3E[15:0]) */
#define REG3E__RESERVED__SHIFT          			0
#define REG3E__RESERVED__WIDTH          			16
#define REG3E__RESERVED__MASK           			0xFFFFU
#define REG3E__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG3E__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG3E__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG3E__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG3E__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG3E__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG3E_REG_MACRO__ */

/* Macros for register REG3F (address=0x3F) */
#ifndef __REG3F_REG_MACRO__
#define __REG3F_REG_MACRO__

/* Macros for field reserved (REG3F[15:0]) */
#define REG3F__RESERVED__SHIFT          			0
#define REG3F__RESERVED__WIDTH          			16
#define REG3F__RESERVED__MASK           			0xFFFFU
#define REG3F__RESERVED__READ(src)      			(((uint32_t)(src) & 0xFFFFU) >> 0)
#define REG3F__RESERVED__WRITE(src)     			(((uint32_t)(src) << 0) & 0xFFFFU)
#define REG3F__RESERVED__MODIFY(dst,src)			(dst) = (((dst) & ~0xFFFFU) | ((uint32_t(src) << 0) & 0xFFFFU))
#define REG3F__RESERVED__VERIFY(dst,src)			(!((((uint32_t)(src) << 0) ^ (dst)) & 0xFFFFU))
#define REG3F__RESERVED__SET(dst)       			(dst) = (((dst) & ~0xFFFFU) & (uint32_t(1) << 0))
#define REG3F__RESERVED__CLR(dst)       			(dst) = (((dst) & ~0xFFFFU) | (uint32_t(0) << 0))

#endif /* __REG3F_REG_MACRO__ */


#endif /* __PLL_MOVELLUS_REGS_MACRO_H__ */

/*     <EOF>     */

