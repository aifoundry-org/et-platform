/* Copyright (C) 2021, Esperanto Technologies Inc.                         */
/* The copyright to the computer program(s) herein is the                  */
/* property of Esperanto Technologies, Inc. All Rights Reserved.           */
/* The program(s) may be used and/or copied only with                      */
/* the written permission of Esperanto Technologies and                    */
/* in accordance with the terms and conditions stipulated in the           */
/* agreement/contract under which the program(s) have been supplied.       */
/*                                                                         */
// From commit c08b07ff00cd040945669d5c3809831fe1669411 in esperanto-soc repo
// memshire ESRs start at 0x0100000000
#define ms_mem_ctl                   0x0180000000
#define ms_atomic_sm_ctl             0x0180000008
#define ms_mem_revision_id           0x0180000010
#define ms_clk_gate_ctl              0x0180000018
#define ms_mem_status                0x0180000020

// ddrc ESRs
#define ddrc_reset_ctl               0x0180000200
#define ddrc_clock_ctl               0x0180000208
#define ddrc_main_ctl                0x0180000210
#define ddrc_scrub1                  0x0180000218
#define ddrc_scrub2                  0x0180000220
#define ddrc_u0_mrr_data             0x0180000228
#define ddrc_u1_mrr_data             0x0180000230
#define ddrc_mrr_status              0x0180000238
#define ddrc_int_status              0x0180000240
#define ddrc_critical_int_en         0x0180000248
#define ddrc_normal_int_en           0x0180000250
#define ddrc_err_int_log             0x0180000258
#define ddrc_debug_sigs_mask0        0x0180000260
#define ddrc_debug_sigs_mask1        0x0180000268
#define ddrc_scratch                 0x01c0000270
#define ddrc_trace_ctl               0x0180000278
#define ddrc_perfmon_ctl_status      0x01c0000280
#define ddrc_perfmon_cyc_cntr        0x01c0000288
#define ddrc_perfmon_p0_cntr         0x01c0000290
#define ddrc_perfmon_p1_cntr         0x01c0000298
#define ddrc_perfmon_p0_qual         0x01c00002a0
#define ddrc_perfmon_p1_qual         0x01c00002a8
#define ddrc_perfmon_p0_qual2        0x01c00002b0
#define ddrc_perfmon_p1_qual2        0x01c00002b8

// PLL registers start at 0x0001000000
// PLL registers are only connected to memshire 0 and 4
#define mem_pll_ctl                  0x0061000000
#define mem_pll_fcw_prediv           0x0061000004
#define mem_pll_fcw_int              0x0061000008
#define mem_pll_fcw_frac             0x006100000c
#define mem_pll_dlf_locked_kp        0x0061000010
#define mem_pll_dlf_locked_ki        0x0061000014
#define mem_pll_dlf_locked_kb        0x0061000018
#define mem_pll_dlf_track_kp         0x006100001c
#define mem_pll_dlf_track_ki         0x0061000020
#define mem_pll_dlf_track_kb         0x0061000024
#define mem_pll_lock_count           0x0061000028
#define mem_pll_lock_threshold       0x006100002c
#define mem_pll_dco_gain             0x0061000030
#define mem_pll_dsm_dither           0x0061000034
#define mem_pll_postdiv              0x0061000038
#define mem_pll_reserved1            0x006100003c
#define mem_pll_reserved2            0x0061000040
#define mem_pll_reserved3            0x0061000044
#define mem_pll_postdiv_ctl          0x0061000048
#define mem_pll_hidden_13            0x006100004c
#define mem_pll_open_loop_code       0x0061000050
#define mem_pll_ldo_ctl              0x0061000054
#define mem_pll_hidden_16            0x0061000058
#define mem_pll_hidden_17            0x006100005c
#define mem_pll_hidden_18            0x0061000060
#define mem_pll_hidden_19            0x0061000064
#define mem_pll_hidden_1a            0x0061000068
#define mem_pll_hidden_1b            0x006100006c
#define mem_pll_hidden_1c            0x0061000070
#define mem_pll_hidden_1d            0x0061000074
#define mem_pll_hidden_1e            0x0061000078
#define mem_pll_hidden_1f            0x006100007c
#define mem_pll_hidden_20            0x0061000080
#define mem_pll_hidden_21            0x0061000084
#define mem_pll_hidden_22            0x0061000088
#define mem_pll_hidden_23            0x006100008c
#define mem_pll_hidden_24            0x0061000090
#define mem_pll_hidden_25            0x0061000094
#define mem_pll_hidden_26            0x0061000098
#define mem_pll_hidden_27            0x006100009c
#define mem_pll_hidden_28            0x00610000a0

#define mem_pll_reg_update_strobe    0x00610000e0
#define mem_pll_lock_detect_status   0x00610000e4
