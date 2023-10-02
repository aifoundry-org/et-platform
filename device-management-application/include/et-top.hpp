/*-------------------------------------------------------------------------
 * Copyright (C) 2023, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#pragma once
#include "deviceLayer/IDeviceLayer.h"
#include "deviceManagement/DeviceManagement.h"
#include "esperanto/et-trace/layout.h"
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <esperanto/et-trace/decoder.h>
#include <hostUtils/logging/Logging.h>

#include <fstream>
#include <functional>
#include <glog/logging.h>
#include <iomanip>
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

#define DV_LOG(severity) ET_LOG(ET_TOP, severity)
#define DV_DLOG(severity) ET_DLOG(ET_TOP, severity)
#define DV_VLOG(level) ET_VLOG(ET_TOP, level)
#define ET_TOP "et-powertop"
#define BIN2VOLTAGE(REG_VALUE, BASE, MULTIPLIER, DIVIDER) (BASE + ((REG_VALUE * MULTIPLIER) / DIVIDER))
#define VOLTAGE2BIN(VOL_VALUE, BASE, MULTIPLIER, DIVIDER) (((VOL_VALUE - BASE) * DIVIDER) / MULTIPLIER)
#define POWER_10MW_TO_W(pwr10mw) (pwr10mw / 100.0)
#define POWER_MW_TO_W(pwrMw) (pwrMw / 1000.0)

static const uint32_t kDmServiceRequestTimeout = 100000;
static const uint32_t kUpdateDelayMS = 1000;
static const int32_t kMaxDeviceNum = 63;
inline const uint16_t kOtherPower = 3750;

enum op_value_unit {
  OP_VALUE_UNIT_POWER_WATTS,
  OP_VALUE_UNIT_POWER_MILLIWATTS,
  OP_VALUE_UNIT_POWER_10MILLIWATTS,
  OP_VALUE_UNIT_TEMPERATURE_CELCIUS,
  OP_VALUE_UNIT_VOLTAGE_MILLIVOLTS,
  OP_VALUE_UNIT_FREQ_MEGAHERTZ,
};

extern struct mm_stats_t {
  uint64_t cycle;
  struct compute_resources_sample computeResources;
} mm_stats_t;

struct sp_stats_t {
  op_stats_t op;
};
extern struct sp_stats_t spStats_;

struct mem_stats_t {
  uint64_t cmaAllocated;
  uint64_t cmaAllocationRate;
};

struct err_stats_t {
  uint64_t ceCount;
  uint64_t uceCount;
  std::map<std::string, uint64_t> ce;
  std::map<std::string, uint64_t> uce;
};

struct aer_stats_t {
  uint64_t aerCount;
  uint64_t fatalCount;
  uint64_t nonfatalCount;
  uint64_t correctableCount;
  std::map<std::string, uint64_t> fatal;
  std::map<std::string, uint64_t> nonfatal;
  std::map<std::string, uint64_t> correctable;
};

static const int32_t kOpsCqNum = 1;
static const int32_t kOpsSqNum = 2;

struct vq_stats_t {
  std::string qname;
  uint64_t msgCount;
  uint64_t msgRate;
  uint64_t utilPercent;
};

class EtTop {
public:
  EtTop(int defDevNum, std::unique_ptr<dev::IDeviceLayer>& dl, device_management::DeviceManagement& dm, bool batchMode,
        uint64_t updateLimit);
  void processInput(void);
  void displayStatsGraph(void);
  void displayStats(void);
  void resetStats(void);
  bool stopStats(void);
  void collectStats(void);
  struct sp_stats_t getSpStats(void);
  struct mm_stats_t getMmStats(void);
  struct mem_stats_t getMemStats(void);
  device_mgmt_api::asic_frequencies_t getFreqStats(void);
  device_mgmt_api::module_voltage_t getModuleVoltStats(void);

private:
  void collectDeviceDetails(void);
  bool processErrorFile(std::string relAttrPath, std::map<std::string, uint64_t>& error, uint64_t& total);
  void displayOpStat(const std::string stat, const struct op_value& ov, const op_value_unit unit,
                     const bool isPower = false, const float cardMax = 0.0, const bool addBarLabels = false);
  void displayFreqStat(const std::string stat, bool endLine, uint64_t frequency);
  void displayFreqStat(const std::string stat, const struct op_value& frequency);
  void displayVoltStat(const std::string stat, bool endLine, uint64_t moduleVoltage, uint64_t asicVoltage);
  void displayVoltStat(const std::string stat, uint64_t moduleVoltage, const struct op_value& asicVoltage);
  void displayComputeStat(const std::string stat, const struct resource_value& rv);
  void displayErrorDetails(std::map<std::string, uint64_t>& error, bool addColon = false, std::string prefix = "");
  void collectMemStats(void);
  void collectErrStats(void);
  void collectAerStats(void);
  void collectVqStats(void);
  void collectSpStats(void);
  void collectMmStats(void);
  void collectFreqStats(void);
  void collectVoltStats(void);

  int devNum_;
  bool batchMode_;
  uint64_t updateLimit_;
  bool stop_;
  bool refreshDeviceDetails_;
  bool displayWattsBars_;
  bool dumpNextSpStatsBuffer_;
  bool dumpNextMmStatsBuffer_;
  bool displayErrorDetails_;
  bool displayFreqDetails_;
  bool displayVoltDetails_;
  bool graphView_;
  std::unique_ptr<dev::IDeviceLayer>& dl_;
  device_management::DeviceManagement& dm_;
  std::string cardId_;
  std::string fwVersion_;
  std::string pmicFwVersion_;

  std::array<vq_stats_t, kOpsSqNum + kOpsCqNum> vqStats_;
  struct mem_stats_t memStats_;
  struct err_stats_t errStats_;
  struct aer_stats_t aerStats_;
  struct sp_stats_t spStats_;
  struct mm_stats_t mmStats_;
  device_mgmt_api::asic_frequencies_t freqStats_;
  device_mgmt_api::module_voltage_t moduleVoltStats_;
  device_mgmt_api::asic_voltage_t asicVoltStats_;
};

const char* opUnitToString(op_value_unit unit);