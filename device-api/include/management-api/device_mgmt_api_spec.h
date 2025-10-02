/*------------------------------------------------------------------------------
 * Copyright (C) 2019, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

#include "esperanto/device-apis/device_apis_build_configuration.h"
#include <stdint.h>

#ifndef ET_DEVICE_MGMT_API_SPEC_H
#define ET_DEVICE_MGMT_API_SPEC_H

/* High level API specifications */

/*! \def DEVICE_MGMT_API_HASH
    \brief Git hash of the device-mgmt-api spec
*/
#define DEVICE_MGMT_API_HASH ESPERANTO_DEVICE_APIS_GIT_HASH

/*! \def DEVICE_MGMT_API_MAJOR
    \brief API Major
*/
#define DEVICE_MGMT_API_MAJOR ESPERANTO_DEVICE_APIS_VERSION_MAJOR

/*! \def DEVICE_MGMT_API_MINOR
    \brief API Minor
*/
#define DEVICE_MGMT_API_MINOR ESPERANTO_DEVICE_APIS_VERSION_MINOR

/*! \def DEVICE_MGMT_API_PATCH
    \brief API Patch
*/
#define DEVICE_MGMT_API_PATCH ESPERANTO_DEVICE_APIS_VERSION_PATCH

/* Device Mgmt API Enumerations */

typedef uint32_t dm_cmd_e;

/*! \enum DM_CMD
    \brief Device Management command IDs
*/
enum DM_CMD
{
    DM_CMD_GET_MODULE_MANUFACTURE_NAME = 0,             /**<  */
    DM_CMD_GET_MODULE_PART_NUMBER = 1,                  /**<  */
    DM_CMD_GET_MODULE_SERIAL_NUMBER = 2,                /**<  */
    DM_CMD_GET_ASIC_CHIP_REVISION = 3,                  /**<  */
    DM_CMD_GET_MODULE_DRIVER_REVISION = 4,              /**<  */
    DM_CMD_GET_MODULE_PCIE_ADDR = 5,                    /**<  */
    DM_CMD_GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED = 6,     /**<  */
    DM_CMD_GET_MODULE_MEMORY_SIZE_MB = 7,               /**<  */
    DM_CMD_GET_MODULE_REVISION = 8,                     /**<  */
    DM_CMD_GET_MODULE_FORM_FACTOR = 9,                  /**<  */
    DM_CMD_GET_MODULE_MEMORY_VENDOR_PART_NUMBER = 10,   /**<  */
    DM_CMD_GET_MODULE_MEMORY_TYPE = 11,                 /**<  */
    DM_CMD_SET_MODULE_PART_NUMBER = 12,                 /**<  */
    DM_CMD_GET_FUSED_PUBLIC_KEYS = 13,                  /**<  */
    DM_CMD_GET_MODULE_FIRMWARE_REVISIONS = 14,          /**<  */
    DM_CMD_SET_FIRMWARE_UPDATE = 15,                    /**<  */
    DM_CMD_GET_FIRMWARE_BOOT_STATUS = 16,               /**<  */
    DM_CMD_SET_SP_BOOT_ROOT_CERT = 17,                  /**<  */
    DM_CMD_SET_SW_BOOT_ROOT_CERT = 18,                  /**<  */
    DM_CMD_SET_FIRMWARE_VERSION_COUNTER = 19,           /**<  */
    DM_CMD_SET_FIRMWARE_VALID = 20,                     /**<  */
    DM_CMD_GET_MODULE_TEMPERATURE_THRESHOLDS = 21,      /**<  */
    DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS = 22,      /**<  */
    DM_CMD_GET_MODULE_POWER_STATE = 23,                 /**<  */
    DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT = 24,     /**<  */
    DM_CMD_GET_MODULE_STATIC_TDP_LEVEL = 25,            /**<  */
    DM_CMD_SET_MODULE_STATIC_TDP_LEVEL = 26,            /**<  */
    DM_CMD_GET_MODULE_CURRENT_TEMPERATURE = 27,         /**<  */
    DM_CMD_GET_MODULE_TEMPERATURE_THROTTLE_STATUS = 28, /**<  */
    DM_CMD_GET_MODULE_RESIDENCY_THROTTLE_STATES = 29,   /**<  */
    DM_CMD_GET_MODULE_UPTIME = 30,                      /**<  */
    DM_CMD_GET_MODULE_VOLTAGE = 31,                     /**<  */
    DM_CMD_GET_ASIC_VOLTAGE = 32,                       /**<  */
    DM_CMD_GET_MODULE_POWER = 33,                       /**<  */
    DM_CMD_GET_MODULE_RESIDENCY_POWER_STATES = 34,      /**<  */
    DM_CMD_SET_MODULE_VOLTAGE = 35,                     /**<  */
    DM_CMD_SET_THROTTLE_POWER_STATE_TEST = 36,          /**<  */
    DM_CMD_SET_FREQUENCY = 37,                          /**<  */
    DM_CMD_GET_MODULE_MAX_TEMPERATURE = 38,             /**<  */
    DM_CMD_GET_MODULE_MAX_DDR_BW = 39,                  /**<  */
    DM_CMD_GET_MAX_MEMORY_ERROR = 40,                   /**<  */
    DM_CMD_SET_DDR_ECC_COUNT = 41,                      /**<  */
    DM_CMD_SET_PCIE_ECC_COUNT = 42,                     /**<  */
    DM_CMD_SET_SRAM_ECC_COUNT = 43,                     /**<  */
    DM_CMD_SET_PCIE_RESET = 44,                         /**<  */
    DM_CMD_GET_MODULE_PCIE_ECC_UECC = 45,               /**<  */
    DM_CMD_GET_MODULE_DDR_BW_COUNTER = 46,              /**<  */
    DM_CMD_GET_MODULE_DDR_ECC_UECC = 47,                /**<  */
    DM_CMD_GET_MODULE_SRAM_ECC_UECC = 48,               /**<  */
    DM_CMD_SET_PCIE_MAX_LINK_SPEED = 49,                /**<  */
    DM_CMD_SET_PCIE_LANE_WIDTH = 50,                    /**<  */
    DM_CMD_SET_PCIE_RETRAIN_PHY = 51,                   /**<  */
    DM_CMD_RESET_ETSOC = 52,                            /**<  */
    DM_CMD_GET_ASIC_FREQUENCIES = 53,                   /**<  */
    DM_CMD_GET_DRAM_BANDWIDTH = 54,                     /**<  */
    DM_CMD_GET_DRAM_CAPACITY_UTILIZATION = 55,          /**<  */
    DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION = 56, /**<  */
    DM_CMD_GET_ASIC_UTILIZATION = 57,                   /**<  */
    DM_CMD_GET_ASIC_STALLS = 58,                        /**<  */
    DM_CMD_GET_ASIC_LATENCY = 59,                       /**<  */
    DM_CMD_GET_SP_STATS = 60,                           /**<  */
    DM_CMD_GET_MM_STATS = 61,                           /**<  */
    DM_CMD_SET_STATS_RUN_CONTROL = 62,                  /**<  */
    DM_CMD_GET_MM_ERROR_COUNT = 63,                     /**<  */
    DM_CMD_MM_RESET = 64,                               /**<  */
    DM_CMD_GET_DEVICE_ERROR_EVENTS = 65,                /**<  */
    DM_CMD_SET_DM_TRACE_RUN_CONTROL = 66,               /**<  */
    DM_CMD_SET_DM_TRACE_CONFIG = 67,                    /**<  */
    DM_CMD_SET_SHIRE_CACHE_CONFIG = 68,                 /**<  */
    DM_CMD_GET_SHIRE_CACHE_CONFIG = 69,                 /**<  */
    DM_CMD_SET_FRU = 70,                                /**<  */
    DM_CMD_GET_FRU = 71,                                /**<  */
    DM_CMD_SET_VMIN_LUT = 72,                           /**<  */
    DM_CMD_GET_VMIN_LUT = 73,                           /**<  */
    DM_CMD_MDI_BEGIN = 128,                             /**<  */
    DM_CMD_MDI_SELECT_HART = 128,                       /**<  */
    DM_CMD_MDI_UNSELECT_HART = 129,                     /**<  */
    DM_CMD_MDI_RESET_HART = 130,                        /**<  */
    DM_CMD_MDI_HALT_HART = 131,                         /**<  */
    DM_CMD_MDI_RESUME_HART = 132,                       /**<  */
    DM_CMD_MDI_GET_HART_STATUS = 133,                   /**<  */
    DM_CMD_MDI_SET_BREAKPOINT = 134,                    /**<  */
    DM_CMD_MDI_UNSET_BREAKPOINT = 135,                  /**<  */
    DM_CMD_MDI_ENABLE_SINGLE_STEP = 136,                /**<  */
    DM_CMD_MDI_DISABLE_SINGLE_STEP = 137,               /**<  */
    DM_CMD_MDI_READ_GPR = 138,                          /**<  */
    DM_CMD_MDI_DUMP_GPR = 139,                          /**<  */
    DM_CMD_MDI_WRITE_GPR = 140,                         /**<  */
    DM_CMD_MDI_READ_CSR = 141,                          /**<  */
    DM_CMD_MDI_WRITE_CSR = 142,                         /**<  */
    DM_CMD_MDI_READ_MEM = 143,                          /**<  */
    DM_CMD_MDI_WRITE_MEM = 144,                         /**<  */
    DM_CMD_MDI_SET_BREAKPOINT_EVENT = 192,              /**<  */
    DM_CMD_MDI_END = 255,                               /**<  */
    /* 256 - 511 are SP error Events */
    DM_EVENT_BEGIN = 512,                /* Total 128 DM events */
    DM_EVENT_SP_TRACE_BUFFER_FULL = 512, /**< Critical SP trace buffer full event */
    DM_EVENT_END = 639,
};

typedef uint8_t power_state_e;

/*! \enum POWER_STATE
    \brief Power State of the ETSOC
*/
enum POWER_STATE
{
    POWER_STATE_MAX_POWER = 0,     /**<  */
    POWER_STATE_MANAGED_POWER = 1, /**<  */
    POWER_STATE_SAFE_POWER = 2,    /**<  */
    POWER_STATE_LOW_POWER = 3,     /**<  */
    POWER_STATE_INVALID = 4,       /**<  */
};

typedef uint8_t module_e;

/*! \enum MODULE
    \brief Module type
*/
enum MODULE
{
    MODULE_DDR = 0,        /**<  */
    MODULE_L2CACHE = 1,    /**<  */
    MODULE_MAXION = 2,     /**<  */
    MODULE_MINION = 3,     /**<  */
    MODULE_PCIE = 4,       /**<  */
    MODULE_NOC = 5,        /**<  */
    MODULE_PCIE_LOGIC = 6, /**<  */
    MODULE_VDDQLP = 7,     /**<  */
    MODULE_VDDQ = 8,       /**<  */
    MODULE_INVALID = 9,    /**<  */
};

typedef uint32_t dm_status_e;

/*! \enum DM_STATUS
    \brief Device management command execution status
*/
enum DM_STATUS
{
    DM_STATUS_SUCCESS = 0, /**<  */
    DM_STATUS_ERROR = 1,   /**<  */
};

typedef uint32_t firmware_status_e;

/*! \enum FIRMWARE_STATUS
    \brief Firmware update status
*/
enum FIRMWARE_STATUS
{
    FIRMWARE_STATUS_PASS = 0,   /**<  */
    FIRMWARE_STATUS_FAILED = 1, /**<  */
};

typedef uint32_t pcie_reset_e;

/*! \enum PCIE_RESET
    \brief PCIE Reset type
*/
enum PCIE_RESET
{
    PCIE_RESET_FLR = 0,  /**<  */
    PCIE_RESET_HOT = 1,  /**<  */
    PCIE_RESET_WARM = 2, /**<  */
};

typedef uint32_t pcie_link_speed_e;

/*! \enum PCIE_LINK_SPEED
    \brief PCIE Link Speed
*/
enum PCIE_LINK_SPEED
{
    PCIE_LINK_SPEED_GEN3 = 0, /**<  */
    PCIE_LINK_SPEED_GEN4 = 1, /**<  */
};

typedef uint32_t pcie_lane_w_split_e;

/*! \enum PCIE_LANE_W_SPLIT
    \brief PCIE Lane Width Split
*/
enum PCIE_LANE_W_SPLIT
{
    PCIE_LANE_W_SPLIT_x4 = 0, /**<  */
    PCIE_LANE_W_SPLIT_x8 = 1, /**<  */
};

typedef uint32_t trace_control_e;

/*! \enum TRACE_CONTROL
    \brief Trace enable/disable and UART log
*/
enum TRACE_CONTROL
{
    TRACE_CONTROL_TRACE_ENABLE = 1,       /**<  */
    TRACE_CONTROL_TRACE_DISABLE = 0,      /**<  */
    TRACE_CONTROL_TRACE_UART_ENABLE = 2,  /**<  */
    TRACE_CONTROL_TRACE_UART_DISABLE = 0, /**<  */
    TRACE_CONTROL_RESET_TRACEBUF = 4,     /**<  */
};

typedef uint32_t trace_configure_e;

/*! \enum TRACE_CONFIGURE
    \brief Trace configure events
*/
enum TRACE_CONFIGURE
{
    TRACE_CONFIGURE_EVENT_STRING = 1,                    /**<  */
    TRACE_CONFIGURE_EVENT_PMC = 2,                       /**<  */
    TRACE_CONFIGURE_EVENT_MARKER = 4,                    /**<  */
    TRACE_CONFIGURE_TRACE_EVENT_ENABLE_ALL = 4294967295, /**<  */
};

typedef uint32_t trace_configure_filter_mask_e;

/*! \enum TRACE_CONFIGURE_FILTER_MASK
    \brief Trace string events filters
*/
enum TRACE_CONFIGURE_FILTER_MASK
{
    TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_CRITICAL = 0, /**<  */
    TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_ERROR = 1,    /**<  */
    TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_WARNING = 2,  /**<  */
    TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_INFO = 3,     /**<  */
    TRACE_CONFIGURE_FILTER_MASK_EVENT_STRING_DEBUG = 4,    /**<  */
};

typedef uint8_t power_throttle_state_e;

/*! \enum POWER_THROTTLE_STATE
    \brief Power throttle states
*/
enum POWER_THROTTLE_STATE
{
    POWER_THROTTLE_STATE_POWER_IDLE = 0,
    POWER_THROTTLE_STATE_POWER_UP,
    POWER_THROTTLE_STATE_POWER_DOWN,
    POWER_THROTTLE_STATE_POWER_SAFE,
    // Must be the last one
    POWER_THROTTLE_STATE_UNKNOWN,
};

typedef uint8_t active_power_management_e;

/*! \enum ACTIVE_POWER_MANAGEMENT
    \brief Power Management of the ETSOC
*/
enum ACTIVE_POWER_MANAGEMENT
{
    ACTIVE_POWER_MANAGEMENT_TURN_OFF = 0, /**<  */
    ACTIVE_POWER_MANAGEMENT_TURN_ON = 1,  /**<  */
    ACTIVE_POWER_MANAGEMENT_INVALID = 2,  /**<  */
};

typedef uint8_t mdi_hart_selection_flag_e;

/*! \enum MDI_HART_SELECTION_FLAG
    \brief Select or Unselect option for Hart
*/
enum MDI_HART_SELECTION_FLAG
{
    MDI_HART_SELECTION_FLAG_SELECT_HART = 0,   /**<  */
    MDI_HART_SELECTION_FLAG_UNSELECT_HART = 1, /**<  */
};

typedef uint8_t mdi_hart_ctrl_flag_e;

/*! \enum MDI_HART_CTRL_FLAG
    \brief Hart Control flag
*/
enum MDI_HART_CTRL_FLAG
{
    MDI_HART_CTRL_FLAG_HALT_HART = 0,   /**<  */
    MDI_HART_CTRL_FLAG_RESUME_HART = 1, /**<  */
    MDI_HART_CTRL_FLAG_RESET_HART = 2,  /**<  */
    MDI_HART_CTRL_FLAG_HART_STATUS = 3, /**<  */
};

typedef uint8_t mdi_hart_status_e;

/*! \enum MDI_HART_STATUS
    \brief Hart status
*/
enum MDI_HART_STATUS
{
    MDI_HART_STATUS_HALTED = 0,    /**<  */
    MDI_HART_STATUS_RUNNING = 1,   /**<  */
    MDI_HART_STATUS_EXCEPTION = 2, /**<  */
    MDI_HART_STATUS_ERROR = 3,     /**<  */
};

typedef uint8_t mdi_bp_ctrl_flag_e;

/*! \enum MDI_BP_CTRL_FLAG
    \brief Breakpoint Control flag
*/
enum MDI_BP_CTRL_FLAG
{
    MDI_BP_CTRL_FLAG_SET_BREAKPOINT = 0,   /**<  */
    MDI_BP_CTRL_FLAG_UNSET_BREAKPOINT = 1, /**<  */
};

typedef uint8_t mdi_ss_ctrl_flag_e;

/*! \enum MDI_SS_CTRL_FLAG
    \brief Single Step Control(Enable/Disable) flag
*/
enum MDI_SS_CTRL_FLAG
{
    MDI_SS_CTRL_FLAG_ENABLE_SS = 0,  /**<  */
    MDI_SS_CTRL_FLAG_DISABLE_SS = 1, /**<  */
};

typedef uint64_t priv_mask_e;

/*! \enum PRIV_MASK
    \brief Minion mode
*/
enum PRIV_MASK
{
    PRIV_MASK_PRIV_NONE = 0,  /**<  */
    PRIV_MASK_PRIV_UMODE = 1, /**<  */
    PRIV_MASK_PRIV_SMODE = 2, /**<  */
    PRIV_MASK_PRIV_MMODE = 8, /**<  */
    PRIV_MASK_PRIV_ALL = 11,  /**<  */
};

typedef uint32_t mdi_event_type_e;

/*! \enum MDI_EVENT_TYPE
    \brief MDI BP Event status
*/
enum MDI_EVENT_TYPE
{
    MDI_EVENT_TYPE_NONE = 0,            /**<  */
    MDI_EVENT_TYPE_BP_HALT_SUCCESS = 1, /**<  */
    MDI_EVENT_TYPE_BP_HALT_FAILED = 2,  /**<  */
};

typedef uint8_t pll_id_e;

/*! \enum PLL_ID
    \brief PLL id for frequency change
*/
enum PLL_ID
{
    PLL_ID_NOC_PLL = 0,    /**<  */
    PLL_ID_MINION_PLL = 1, /**<  */
};

typedef uint8_t use_step_e;

/*! \enum USE_STEP
    \brief Use step clock for Minions
*/
enum USE_STEP
{
    USE_STEP_CLOCK_FALSE = 0, /**<  */
    USE_STEP_CLOCK_TRUE = 1,  /**<  */
};

typedef uint8_t mem_access_type_e;

/*! \enum MEM_ACCESS_TYPE
    \brief Minion memory access type
*/
enum MEM_ACCESS_TYPE
{
    MEM_ACCESS_TYPE_GLOBAL_ATOMIC = 0, /**<  */
    MEM_ACCESS_TYPE_LOCAL_ATOMIC = 1,  /**<  */
    MEM_ACCESS_TYPE_NORMAL = 2,        /**<  */
};

typedef uint32_t stats_type_e;

/*! \enum STATS_TYPE
    \brief Stats type
*/
enum STATS_TYPE
{
    STATS_TYPE_SP = 1, /**<  */
    STATS_TYPE_MM = 2, /**<  */
};

typedef uint32_t stats_control_e;

/*! \enum STATS_CONTROL
    \brief Stats Control
*/
enum STATS_CONTROL
{
    STATS_CONTROL_TRACE_DISABLE = 0,  /**<  */
    STATS_CONTROL_TRACE_ENABLE = 1,   /**<  */
    STATS_CONTROL_RESET_COUNTER = 2,  /**<  */
    STATS_CONTROL_RESET_TRACEBUF = 4, /**<  */
};

#endif /* ET_DEVICE_MGMT_API_SPEC_H */
