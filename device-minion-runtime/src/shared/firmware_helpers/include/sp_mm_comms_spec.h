/*------------------------------------------------------------------------------
 * Copyright (C) 2019, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

// WARNING: this file is auto-generated do not edit directly

#ifndef MM_SP_CMD_SPEC_H
#define MM_SP_CMD_SPEC_H

#include <inttypes.h>

enum mm_sp_msg_e {
    MM2SP_CMD_OFFSET = 128,
    MM2SP_CMD_ECHO,
    MM2SP_RSP_ECHO,
    MM2SP_CMD_GET_ACTIVE_SHIRE_MASK,
    MM2SP_RSP_GET_ACTIVE_SHIRE_MASK,
    MM2SP_CMD_GET_CM_BOOT_FREQ,
    MM2SP_RSP_GET_CM_BOOT_FREQ,
    MM2SP_CMD_REPORT_EXCEPTION_ERROR,
    MM2SP_CMD_REPORT_HANG_ERROR
};

enum sp_mm_msg_e {
    SP2MM_CMD_ECHO = 256,
    SP2MM_RSP_ECHO,
    SP2MM_CMD_UPDATE_FREQ,
    SP2MM_RSP_UPDATE_FREQ,
    SP2MM_CMD_TEARDOWN_MM,
    SP2MM_RSP_TEARDOWN_MM,
    SP2MM_CMD_QUIESCE_TRAFFIC,
    SP2MM_RSP_QUIESCE_TRAFFIC,
    SP2MM_CMD_GET_TRACE_BUFF_CONTROL_STRUCT,
    SP2MM_RSP_GET_TRACE_BUFF_CONTROL_STRUCT,
    SP2MM_CMD_KERNEL_LAUNCH,
    SP2MM_RSP_KERNEL_LAUNCH
};

/* TODO: We should invent a new cmd and rsp header for the mrt messages
the device api cmd and rsp headers being used now have some redundant
fields */

struct dev_cmd_hdr_t {
  uint16_t msg_size;
  uint16_t msg_id;
  uint8_t pad[4];
};

struct mm2sp_echo_cmd_t {
  struct dev_cmd_hdr_t  msg_hdr;
  uint32_t  payload;
};

struct mm2sp_echo_rsp_t {
  struct dev_cmd_hdr_t  msg_hdr;
  uint32_t  payload;
};

struct mm2sp_get_active_shire_mask_cmd_t {
  struct dev_cmd_hdr_t  msg_hdr;
};

struct mm2sp_get_active_shire_mask_rsp_t {
  struct dev_cmd_hdr_t  msg_hdr;
  uint64_t  active_shire_mask;
};

struct mm2sp_get_cm_boot_freq_cmd_t {
  struct dev_cmd_hdr_t  msg_hdr;
};

struct mm2sp_get_cm_boot_freq_rsp_t {
  struct dev_cmd_hdr_t  msg_hdr;
  uint32_t  cm_boot_freq;
};

struct mm2sp_report_error_cmd_t {
  struct dev_cmd_hdr_t  msg_hdr;
  uint16_t error_type;
  uint8_t worker_type; /* Source worker type of the error from where it is originated. */
};

struct sp2mm_echo_cmd_t {
  struct dev_cmd_hdr_t  msg_hdr;
  uint32_t  payload;
};

struct sp2mm_echo_rsp_t {
  struct dev_cmd_hdr_t  msg_hdr;
  uint32_t  payload;
};

struct sp2mm_update_freq_cmd_t {
  struct dev_cmd_hdr_t  msg_hdr;
  uint32_t  freq;
};

struct sp2mm_update_freq_rsp_t {
  struct dev_cmd_hdr_t  msg_hdr;
  int32_t  status; /* TODO: Define status as a enum once all error states are defined */
};

struct sp2mm_teardown_mm_cmd_t {
  struct dev_cmd_hdr_t  msg_hdr;
};

struct sp2mm_teardown_mm_rsp_t {
  struct dev_cmd_hdr_t  msg_hdr;
  int32_t  status; /* TODO: Define status as a enum once all error states are defined */
};

struct sp2mm_quiesce_traffic_cmd_t {
  struct dev_cmd_hdr_t  msg_hdr;
};

struct sp2mm_quiesce_traffic_rsp_t {
  struct dev_cmd_hdr_t  msg_hdr;
  int32_t  status; /* TODO: Define status as a enum once all error states are defined */
};

struct sp2mm_kernel_launch_cmd_t {
  struct dev_cmd_hdr_t  msg_hdr;
  uint64_t  shire_mask;
  void *args;
};

struct sp2mm_kernel_launch_rsp_t {
  struct dev_cmd_hdr_t  msg_hdr;
  int32_t  status; /* TODO: Define status as a enum once all error states are defined */
};

#endif /* MM_SP_CMD_SPEC_H */
