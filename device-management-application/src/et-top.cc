/*-------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#define ET_TRACE_DECODER_IMPL
#include "render.hpp"

static struct termios orig_termios;

static void restoreTTY(void) {
  if (tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios) != 0) {
    DV_LOG(WARNING) << "tcsetattr restore on stdin error: " << std::strerror(errno);
  }
  return;
}

static inline std::string formatVersion(uint32_t ver) {
  return "v" + std::to_string((ver >> 24) & 0xff) + "." + std::to_string((ver >> 16) & 0xff) + "." +
         std::to_string((ver >> 8) & 0xff);
}

static inline std::string getNewFileName(int devNum, bool isSp) {
  std::stringstream ss;
  const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  ss << "dev" << devNum << "_" << (isSp ? "sp" : "mm") << "_stats_"
     << std::put_time(std::localtime(&time), "%Y:%m:%d_%X") << ".bin";
  return ss.str();
}

const char* opUnitToString(op_value_unit unit) {
  switch (unit) {
  case OP_VALUE_UNIT_POWER_WATTS:
    return "W";
  case OP_VALUE_UNIT_POWER_MILLIWATTS:
    return "mW";
  case OP_VALUE_UNIT_POWER_10MILLIWATTS:
    return "10mW";
  case OP_VALUE_UNIT_TEMPERATURE_CELCIUS:
    return "C";
  case OP_VALUE_UNIT_VOLTAGE_MILLIVOLTS:
    return "mV";
  case OP_VALUE_UNIT_FREQ_MEGAHERTZ:
    return "MHz";
  default:
    return "[Unknown Unit]";
  }
}

static inline char getCharFromStdin(bool echoOnStdout = false) {
  char ch = 0;
  while (true) {
    auto rc = read(STDIN_FILENO, &ch, 1);
    if (rc == -1) {
      std::cerr << "read on stdin error: " << std::strerror(errno);
      exit(1);
    } else if (rc > 0) {
      if (echoOnStdout) {
        std::cout << ch;
      }
      break;
    }
  }
  return ch;
}

static inline std::string getLineFromStdin(size_t strLength, bool echoOnStdout = false) {
  std::vector<char> vec(strLength + 1 /* to terminate with null character */, 0);
  for (unsigned int i = 0; i < strLength; i++) {
    if (auto ch = getCharFromStdin(echoOnStdout); ch != '\n') {
      vec[i] = ch;
    } else {
      break;
    }
  }
  return std::string(vec.begin(), vec.end());
}

EtTop::EtTop(int defDevNum, std::unique_ptr<dev::IDeviceLayer>& dl, device_management::DeviceManagement& dm,
             bool batchMode, uint64_t updateLimit)
  : devNum_(defDevNum)
  , batchMode_(batchMode)
  , updateLimit_(updateLimit)
  , stop_(false)
  , refreshDeviceDetails_(true)
  , displayWattsBars_(false)
  , dumpNextSpStatsBuffer_(false)
  , dumpNextMmStatsBuffer_(false)
  , displayErrorDetails_(batchMode)
  , displayFreqDetails_(batchMode)
  , displayVoltDetails_(batchMode)
  , graphView_(batchMode)
  , dl_(dl)
  , dm_(dm) {
  vqStats_[0].qname = "SQ0:";
  vqStats_[1].qname = "SQ1:";
  vqStats_[2].qname = "CQ0:";
  mmStats_.cycle = 0;
}

bool EtTop::stopStats(void) {
  return stop_;
}

void EtTop::collectStats(void) {
  if (refreshDeviceDetails_) {
    collectDeviceDetails();
    refreshDeviceDetails_ = false;
  }
  collectMemStats();
  collectErrStats();
  collectAerStats();
  collectVqStats();
  collectSpStats();
  collectMmStats();
  if (displayFreqDetails_) {
    collectFreqStats();
  }
  if (displayVoltDetails_) {
    collectVoltStats();
  }
  return;
}

void EtTop::collectMemStats(void) {
  std::string dummy;
  std::istringstream attrFileAlloc(dl_->getDeviceAttribute(devNum_, "mem_stats/cma_allocated"));
  for (std::string line; std::getline(attrFileAlloc, line);) {
    std::stringstream ss;
    ss << line;
    ss >> memStats_.cmaAllocated >> dummy;
  }

  std::istringstream attrFileRate(dl_->getDeviceAttribute(devNum_, "mem_stats/cma_allocation_rate"));
  for (std::string line; std::getline(attrFileRate, line);) {
    std::stringstream ss;
    ss << line;
    ss >> memStats_.cmaAllocationRate >> dummy;
  }

  return;
}

void EtTop::collectVqStats(void) {
  std::string qname;
  uint64_t num;
  std::string dummy;

  std::istringstream attrFileCount(dl_->getDeviceAttribute(devNum_, "ops_vq_stats/msg_count"));
  for (std::string line; std::getline(attrFileCount, line);) {
    std::stringstream ss;
    ss << line;
    ss >> qname >> num >> dummy;

    auto it = std::find_if(vqStats_.begin(), vqStats_.end(), [qname](const auto& e) { return qname == e.qname; });
    if (it != vqStats_.end()) {
      (*it).msgCount = num;
    }
  }

  std::istringstream attrFileRate(dl_->getDeviceAttribute(devNum_, "ops_vq_stats/msg_rate"));
  for (std::string line; std::getline(attrFileRate, line);) {
    std::stringstream ss;
    ss << line;
    ss >> qname >> num >> dummy;

    auto it = std::find_if(vqStats_.begin(), vqStats_.end(), [qname](const auto& e) { return qname == e.qname; });
    if (it != vqStats_.end()) {
      (*it).msgRate = num;
    }
  }

  std::istringstream attrFileUtil(dl_->getDeviceAttribute(devNum_, "ops_vq_stats/utilization_percent"));
  for (std::string line; std::getline(attrFileUtil, line);) {
    std::stringstream ss;
    ss << line;
    ss >> qname >> num >> dummy;

    auto it = std::find_if(vqStats_.begin(), vqStats_.end(), [qname](const auto& e) { return qname == e.qname; });
    if (it != vqStats_.end()) {
      (*it).utilPercent = num;
    }
  }

  return;
}

struct sp_stats_t EtTop::getSpStats(void) {
  return spStats_;
}
struct mm_stats_t EtTop::getMmStats(void) {
  return mmStats_;
}
struct mem_stats_t EtTop::getMemStats(void) {
  return memStats_;
}
device_mgmt_api::asic_frequencies_t EtTop::getFreqStats(void) {
  return freqStats_;
}
device_mgmt_api::module_voltage_t EtTop::getModuleVoltStats(void) {
  return moduleVoltStats_;
}

bool EtTop::processErrorFile(std::string relAttrPath, std::map<std::string, uint64_t>& error, uint64_t& total) {
  total = 0;

  try {
    std::istringstream attrFile(dl_->getDeviceAttribute(devNum_, relAttrPath));

    for (std::string line; std::getline(attrFile, line);) {
      uint64_t count;
      std::string name;
      std::stringstream ss;

      ss << line;
      ss >> name >> count;
      if (error[name] > count) {
        DV_LOG(WARNING) << "file " << relAttrPath << "error " << name << "decreased from " << error[name] << "to "
                        << count << std::endl;
      }
      error[name] = count;
      total += count;
    }
  } catch (const dev::Exception& ex) {
    // just warn for now if the file is not present, which is
    // the case currently for pci aer files during simulation
    DV_LOG(WARNING) << "unable to processs file " << relAttrPath;
    return false;
  }

  return true;
}

void EtTop::collectErrStats(void) {
  if (!processErrorFile("err_stats/ce_count", errStats_.ce, errStats_.ceCount)) {
    DV_LOG(ERROR) << "unable to processs file err_stats/ce_count";
    exit(1);
  }

  if (!processErrorFile("err_stats/uce_count", errStats_.uce, errStats_.uceCount)) {
    DV_LOG(ERROR) << "unable to processs file err_stats/uce_count";
    exit(1);
  }
  return;
}

void EtTop::collectAerStats(void) {
  // These files aren't currently present during simulation, so just skip
  // processing them for now after trying once if there was a problem.
  static bool fatalFileError = false;
  static bool nonfatalFileError = false;
  static bool correctableFileError = false;

  // These files contain a total in addition to the individual counts, so correct for double
  // counting and erase it from the map.
  if (!fatalFileError) {
    fatalFileError = !processErrorFile("aer_dev_fatal", aerStats_.fatal, aerStats_.fatalCount);

    uint64_t total = aerStats_.fatal["TOTAL_ERR_FATAL"];
    aerStats_.fatalCount >>= 1;
    if (aerStats_.fatalCount != total) {
      DV_LOG(WARNING) << "aer_dev_fatal count mismatch: " << aerStats_.fatalCount << " != " << total;
    }
    aerStats_.fatal.erase("TOTAL_ERR_FATAL");
  }

  if (!nonfatalFileError) {
    nonfatalFileError = !processErrorFile("aer_dev_nonfatal", aerStats_.nonfatal, aerStats_.nonfatalCount);

    uint64_t total = aerStats_.nonfatal["TOTAL_ERR_NONFATAL"];
    aerStats_.nonfatalCount >>= 1;
    if (aerStats_.nonfatalCount != total) {
      DV_LOG(WARNING) << "aer_dev_nonfatal count mismatch: " << aerStats_.nonfatalCount << " != " << total;
    }
    aerStats_.nonfatal.erase("TOTAL_ERR_NONFATAL");
  }

  if (!correctableFileError) {
    correctableFileError = !processErrorFile("aer_dev_correctable", aerStats_.correctable, aerStats_.correctableCount);

    uint64_t total = aerStats_.correctable["TOTAL_ERR_COR"];
    aerStats_.correctableCount >>= 1;
    if (aerStats_.correctableCount != total) {
      DV_LOG(WARNING) << "aer_dev_correctable count mismatch: " << aerStats_.correctableCount << " != " << total;
    }
    aerStats_.correctable.erase("TOTAL_ERR_COR");
  }

  aerStats_.aerCount = aerStats_.fatalCount + aerStats_.fatalCount + aerStats_.correctableCount;

  return;
}

void EtTop::collectSpStats(void) {
  uint32_t hostLatency;
  uint64_t deviceLatency;
  std::vector<char> outputBuff(sizeof(device_mgmt_api::get_sp_stats_t), 0);

  auto ret = dm_.serviceRequest(devNum_, device_mgmt_api::DM_CMD::DM_CMD_GET_SP_STATS, nullptr, 0, outputBuff.data(),
                                outputBuff.size(), &hostLatency, &deviceLatency, kDmServiceRequestTimeout);
  if (ret != device_mgmt_api::DM_STATUS_SUCCESS) {
    DV_LOG(ERROR) << "Service request get sp stats failed with return code: " << std::dec << ret << std::endl;
  } else {
    auto* sp_stats = static_cast<device_mgmt_api::get_sp_stats_t*>(static_cast<void*>(outputBuff.data()));

    spStats_.op.system.power.avg = sp_stats->system_power_avg;
    spStats_.op.system.power.min = sp_stats->system_power_min;
    spStats_.op.system.power.max = sp_stats->system_power_max;
    spStats_.op.system.temperature.avg = sp_stats->system_temperature_avg;
    spStats_.op.system.temperature.min = sp_stats->system_temperature_min;
    spStats_.op.system.temperature.max = sp_stats->system_temperature_max;

    spStats_.op.minion.power.avg = sp_stats->minion_power_avg;
    spStats_.op.minion.power.min = sp_stats->minion_power_min;
    spStats_.op.minion.power.max = sp_stats->minion_power_max;
    spStats_.op.minion.temperature.avg = sp_stats->minion_temperature_avg;
    spStats_.op.minion.temperature.min = sp_stats->minion_temperature_min;
    spStats_.op.minion.temperature.max = sp_stats->minion_temperature_max;
    spStats_.op.minion.voltage.avg = sp_stats->minion_voltage_avg;
    spStats_.op.minion.voltage.min = sp_stats->minion_voltage_min;
    spStats_.op.minion.voltage.max = sp_stats->minion_voltage_max;
    spStats_.op.minion.freq.avg = sp_stats->minion_freq_avg;
    spStats_.op.minion.freq.min = sp_stats->minion_freq_min;
    spStats_.op.minion.freq.max = sp_stats->minion_freq_max;

    spStats_.op.sram.power.avg = sp_stats->sram_power_avg;
    spStats_.op.sram.power.min = sp_stats->sram_power_min;
    spStats_.op.sram.power.max = sp_stats->sram_power_max;
    spStats_.op.sram.temperature.avg = sp_stats->sram_temperature_avg;
    spStats_.op.sram.temperature.min = sp_stats->sram_temperature_min;
    spStats_.op.sram.temperature.max = sp_stats->sram_temperature_max;
    spStats_.op.sram.voltage.avg = sp_stats->sram_voltage_avg;
    spStats_.op.sram.voltage.min = sp_stats->sram_voltage_min;
    spStats_.op.sram.voltage.max = sp_stats->sram_voltage_max;
    spStats_.op.sram.freq.avg = sp_stats->sram_freq_avg;
    spStats_.op.sram.freq.min = sp_stats->sram_freq_min;
    spStats_.op.sram.freq.max = sp_stats->sram_freq_max;

    spStats_.op.noc.power.avg = sp_stats->noc_power_avg;
    spStats_.op.noc.power.min = sp_stats->noc_power_min;
    spStats_.op.noc.power.max = sp_stats->noc_power_max;
    spStats_.op.noc.temperature.avg = sp_stats->noc_temperature_avg;
    spStats_.op.noc.temperature.min = sp_stats->noc_temperature_min;
    spStats_.op.noc.temperature.max = sp_stats->noc_temperature_max;
    spStats_.op.noc.voltage.avg = sp_stats->noc_voltage_avg;
    spStats_.op.noc.voltage.min = sp_stats->noc_voltage_min;
    spStats_.op.noc.voltage.max = sp_stats->noc_voltage_max;
    spStats_.op.noc.freq.avg = sp_stats->noc_freq_avg;
    spStats_.op.noc.freq.min = sp_stats->noc_freq_min;
    spStats_.op.noc.freq.max = sp_stats->noc_freq_max;
  }

  if (dumpNextSpStatsBuffer_) {
    dumpNextSpStatsBuffer_ = false;

    std::vector<std::byte> response;
    if (dm_.getTraceBufferServiceProcessor(devNum_, TraceBufferType::TraceBufferSPStats, response) !=
        device_mgmt_api::DM_STATUS_SUCCESS) {
      DV_LOG(ERROR) << "getTraceBufferServiceProcessor SPStats error";
    } else {
      auto fileName = getNewFileName(devNum_, true /* SP */);
      std::ofstream spTrace;
      spTrace.open(fileName, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
      if (!spTrace.is_open()) {
        DV_LOG(ERROR) << "Error: unable to open file " << fileName << std::endl;
      } else {
        spTrace.write(reinterpret_cast<const char*>(response.data()), response.size());
        spTrace.close();
      }
    }
  }

  return;
}

void EtTop::collectDeviceDetails(void) {
  uint32_t hostLatency;
  uint64_t deviceLatency;
  device_mgmt_api::asset_info_t assetInfo = {0};
  auto ret = dm_.serviceRequest(devNum_, device_mgmt_api::DM_CMD_GET_MODULE_PART_NUMBER, nullptr, 0,
                                static_cast<char*>(static_cast<void*>(&assetInfo)), sizeof(assetInfo), &hostLatency,
                                &deviceLatency, kDmServiceRequestTimeout);
  if (ret != device_mgmt_api::DM_STATUS_SUCCESS) {
    DV_LOG(ERROR) << "Service request get asset info failed with return code: " << std::dec << ret << std::endl;
  } else {
    std::stringstream sstream;
    sstream << *static_cast<uint32_t*>(static_cast<void*>(assetInfo.asset));
    cardId_ = sstream.str();
  }

  device_mgmt_api::firmware_version_t versions = {0};
  ret = dm_.serviceRequest(devNum_, device_mgmt_api::DM_CMD_GET_MODULE_FIRMWARE_REVISIONS, nullptr, 0,
                           static_cast<char*>(static_cast<void*>(&versions)), sizeof(versions), &hostLatency,
                           &deviceLatency, kDmServiceRequestTimeout);
  if (ret != device_mgmt_api::DM_STATUS_SUCCESS) {
    DV_LOG(ERROR) << "Service request get firmware version failed with return code: " << std::dec << ret << std::endl;
  } else {
    fwVersion_ = formatVersion(versions.fw_release_rev);
    pmicFwVersion_ = formatVersion(versions.pmic_v);
  }
}

void EtTop::collectFreqStats(void) {
  uint32_t hostLatency;
  uint64_t deviceLatency;
  auto ret = dm_.serviceRequest(devNum_, device_mgmt_api::DM_CMD_GET_ASIC_FREQUENCIES, nullptr, 0,
                                static_cast<char*>(static_cast<void*>(&freqStats_)), sizeof(freqStats_), &hostLatency,
                                &deviceLatency, kDmServiceRequestTimeout);
  if (ret != device_mgmt_api::DM_STATUS_SUCCESS) {
    DV_LOG(ERROR) << "Service request get asic frequencies failed with return code: " << std::dec << ret << std::endl;
  }
}

void EtTop::collectVoltStats(void) {
  uint32_t hostLatency;
  uint64_t deviceLatency;
  auto ret = dm_.serviceRequest(devNum_, device_mgmt_api::DM_CMD_GET_MODULE_VOLTAGE, nullptr, 0,
                                static_cast<char*>(static_cast<void*>(&moduleVoltStats_)), sizeof(moduleVoltStats_),
                                &hostLatency, &deviceLatency, kDmServiceRequestTimeout);
  if (ret != device_mgmt_api::DM_STATUS_SUCCESS) {
    DV_LOG(ERROR) << "Service request get module voltage failed with return code: " << std::dec << ret << std::endl;
  }
  ret = dm_.serviceRequest(devNum_, device_mgmt_api::DM_CMD_GET_ASIC_VOLTAGE, nullptr, 0,
                           static_cast<char*>(static_cast<void*>(&asicVoltStats_)), sizeof(asicVoltStats_),
                           &hostLatency, &deviceLatency, kDmServiceRequestTimeout);
  if (ret != device_mgmt_api::DM_STATUS_SUCCESS) {
    DV_LOG(ERROR) << "Service request get asic voltage failed with return code: " << std::dec << ret << std::endl;
  }
}

void EtTop::collectMmStats(void) {
  uint32_t hostLatency;
  uint64_t deviceLatency;
  std::vector<char> outputBuff(sizeof(device_mgmt_api::get_mm_stats_t), 0);

  auto ret = dm_.serviceRequest(devNum_, device_mgmt_api::DM_CMD::DM_CMD_GET_MM_STATS, nullptr, 0, outputBuff.data(),
                                outputBuff.size(), &hostLatency, &deviceLatency, kDmServiceRequestTimeout);

  if (ret != device_mgmt_api::DM_STATUS_SUCCESS) {
    DV_LOG(ERROR) << "Service request get mm stats failed with return code: " << std::dec << ret << std::endl;
  } else {
    auto* mm_stats = static_cast<device_mgmt_api::get_mm_stats_t*>(static_cast<void*>(outputBuff.data()));

    mmStats_.computeResources.cm_bw.avg = mm_stats->cm_bw_avg;
    mmStats_.computeResources.cm_bw.min = mm_stats->cm_bw_min;
    mmStats_.computeResources.cm_bw.max = mm_stats->cm_bw_max;
    mmStats_.computeResources.cm_utilization.avg = mm_stats->cm_utilization_avg;
    mmStats_.computeResources.cm_utilization.min = mm_stats->cm_utilization_min;
    mmStats_.computeResources.cm_utilization.max = mm_stats->cm_utilization_max;

    mmStats_.computeResources.pcie_dma_read_utilization.avg = mm_stats->pcie_dma_read_utilization_avg;
    mmStats_.computeResources.pcie_dma_read_utilization.min = mm_stats->pcie_dma_read_utilization_min;
    mmStats_.computeResources.pcie_dma_read_utilization.max = mm_stats->pcie_dma_read_utilization_max;
    mmStats_.computeResources.pcie_dma_write_utilization.avg = mm_stats->pcie_dma_write_utilization_avg;
    mmStats_.computeResources.pcie_dma_write_utilization.min = mm_stats->pcie_dma_write_utilization_min;
    mmStats_.computeResources.pcie_dma_write_utilization.max = mm_stats->pcie_dma_write_utilization_max;

    mmStats_.computeResources.pcie_dma_read_bw.avg = mm_stats->pcie_dma_read_bw_avg;
    mmStats_.computeResources.pcie_dma_read_bw.min = mm_stats->pcie_dma_read_bw_min;
    mmStats_.computeResources.pcie_dma_read_bw.max = mm_stats->pcie_dma_read_bw_max;
    mmStats_.computeResources.pcie_dma_write_bw.avg = mm_stats->pcie_dma_write_bw_avg;
    mmStats_.computeResources.pcie_dma_write_bw.min = mm_stats->pcie_dma_write_bw_min;
    mmStats_.computeResources.pcie_dma_write_bw.max = mm_stats->pcie_dma_write_bw_max;

    mmStats_.computeResources.ddr_read_bw.avg = mm_stats->ddr_read_bw_avg;
    mmStats_.computeResources.ddr_read_bw.min = mm_stats->ddr_read_bw_min;
    mmStats_.computeResources.ddr_read_bw.max = mm_stats->ddr_read_bw_max;
    mmStats_.computeResources.ddr_write_bw.avg = mm_stats->ddr_write_bw_avg;
    mmStats_.computeResources.ddr_write_bw.min = mm_stats->ddr_write_bw_min;
    mmStats_.computeResources.ddr_write_bw.max = mm_stats->ddr_write_bw_max;

    mmStats_.computeResources.l2_l3_read_bw.avg = mm_stats->l2_l3_read_bw_avg;
    mmStats_.computeResources.l2_l3_read_bw.min = mm_stats->l2_l3_read_bw_min;
    mmStats_.computeResources.l2_l3_read_bw.max = mm_stats->l2_l3_read_bw_max;
    mmStats_.computeResources.l2_l3_write_bw.avg = mm_stats->l2_l3_write_bw_avg;
    mmStats_.computeResources.l2_l3_write_bw.min = mm_stats->l2_l3_write_bw_min;
    mmStats_.computeResources.l2_l3_write_bw.max = mm_stats->l2_l3_write_bw_max;
  }

  if (dumpNextMmStatsBuffer_) {
    dumpNextMmStatsBuffer_ = false;

    std::vector<std::byte> response;
    if (dm_.getTraceBufferServiceProcessor(devNum_, TraceBufferType::TraceBufferMMStats, response) !=
        device_mgmt_api::DM_STATUS_SUCCESS) {
      DV_LOG(ERROR) << "getTraceBufferServiceProcessor MMStats error";
    } else {
      auto fileName = getNewFileName(devNum_, false /* MM */);
      std::ofstream mmTrace;

      mmTrace.open(fileName, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
      if (!mmTrace.is_open()) {
        DV_LOG(ERROR) << "Error: unable to open file " << fileName << std::endl;
      } else {
        mmTrace.write(reinterpret_cast<const char*>(response.data()), response.size());
        mmTrace.close();
      }
    }
  }

  return;
}

void EtTop::resetStats(void) {
  uint32_t hostLatency;
  uint64_t deviceLatency;
  std::vector<char> inputBuff(sizeof(device_mgmt_api::stats_type_e) + sizeof(device_mgmt_api::stats_control_e), 0);
  device_mgmt_api::stats_type_e type = device_mgmt_api::STATS_TYPE_SP | device_mgmt_api::STATS_TYPE_MM;
  device_mgmt_api::stats_control_e control = device_mgmt_api::STATS_CONTROL_TRACE_ENABLE |
                                             device_mgmt_api::STATS_CONTROL_RESET_TRACEBUF |
                                             device_mgmt_api::STATS_CONTROL_RESET_COUNTER;
  memcpy(inputBuff.data(), &type, sizeof(type));
  memcpy(inputBuff.data() + sizeof(type), &control, sizeof(control));

  auto ret = dm_.serviceRequest(devNum_, device_mgmt_api::DM_CMD::DM_CMD_SET_STATS_RUN_CONTROL, inputBuff.data(),
                                inputBuff.size(), nullptr, 0, &hostLatency, &deviceLatency, kDmServiceRequestTimeout);
  if (ret != device_mgmt_api::DM_STATUS_SUCCESS) {
    DV_LOG(ERROR) << "Service request stats run control failed with return code: " << std::dec << ret << std::endl;
  }

  return;
}

void EtTop::processInput(void) {
  int rc;
  char ch;
  bool help = false;
  do {
    rc = read(STDIN_FILENO, &ch, 1);
    if (rc == -1) {
      DV_LOG(ERROR) << "read on stdin error: " << std::strerror(errno);
      exit(1);
    }

    if (rc == 0) {
      continue;
    }

    if (help) {
      if (ch == 'q' || ch == 0x1B) {
        help = false;
      }
    } else if (ch == 'q') {
      stop_ = true;
    } else if (ch == 'r') {
      resetStats();
    } else if (ch == 'w') {
      displayWattsBars_ = !displayWattsBars_;
    } else if (ch == 'd') {
      dumpNextSpStatsBuffer_ = true;
      dumpNextMmStatsBuffer_ = true;
    } else if (ch == 'e') {
      displayErrorDetails_ = !displayErrorDetails_;
    } else if (ch == 'f') {
      displayFreqDetails_ = !displayFreqDetails_;
    } else if (ch == 's' && dl_->getDevicesCount() > 1) {
      system("clear");
      std::cout << "Switch to device [0-" << dl_->getDevicesCount() - 1 << "] (<ENTER> to continue): ";
      while (true) {
        auto str = getLineFromStdin(2 /* 2-digit string */, true);
        if (str.empty() || str.at(0) == '\0') {
          // Enter results in empty string
          break;
        }
        std::smatch m;
        if (std::regex re("[0-9]+"); std::regex_search(str, m, re) && !m.empty()) {
          if (int devNum = std::stoul(m[0].str()); devNum >= 0 && devNum < dl_->getDevicesCount()) {
            devNum_ = devNum;
            refreshDeviceDetails_ = true;
            break;
          }
        }
        std::cout << "\nInvalid device: " << str << ", please select in range [0-" << dl_->getDevicesCount() - 1
                  << "]: ";
      }
    } else if (ch == 'v') {
      displayVoltDetails_ = !displayVoltDetails_;
    } else if (ch == 'h') {
      help = true;
      system("clear");
      std::cout << "d\tDump next trace stats buffers to mm_stats.bin and sp_stats.bin files\n"
                << "e\tToggle display of error details\n"
                << "f\tToggle display of frequency details\n"
                << "h\tPrint this help message\n"
                << "q\tQuit\n"
                << "r\tReset statistics\n"
                << "s\tSwitch device\n"
                << "v\tToggle display of voltage details\n"
                << "w\tToggle display of watts info\n"
                << "Type 'q' or <ESC> to continue ";
    }
  } while (help || (!stop_ && rc == 1));

  return;
}

void EtTop::displayOpStat(const std::string stat, const struct op_value& ov, const op_value_unit unit,
                          const bool isPower, const float cardMax, const bool addBarLabels) {
  if (!displayWattsBars_ && addBarLabels) {
    std::cout << std::endl;
  }

  if (!isPower) {
    std::cout << "\t" + stat + "   avg: " << std::setw(5) << std::left << ov.avg << "  min: " << std::setw(5)
              << std::left << ov.min << "  max: " << std::setw(5) << std::left << ov.max << std::endl;
  } else {
    float avg, min, max;
    // convert to watts
    switch (unit) {
    case OP_VALUE_UNIT_POWER_MILLIWATTS:
      avg = POWER_MW_TO_W(ov.avg);
      min = POWER_MW_TO_W(ov.min);
      max = POWER_MW_TO_W(ov.max);
      break;
    case OP_VALUE_UNIT_POWER_10MILLIWATTS:
      avg = POWER_10MW_TO_W(ov.avg);
      min = POWER_10MW_TO_W(ov.min);
      max = POWER_10MW_TO_W(ov.max);
      break;
    default:
      avg = ov.avg;
      min = ov.min;
      max = ov.max;
      break;
    }

    if (!displayWattsBars_) {
      std::cout << std::setprecision(2) << std::fixed;
      std::cout << "\t" + stat + " avg: " << std::setw(5) << std::right << avg << "  min: " << std::setw(5)
                << std::right << min << "  max: " << std::setw(5) << std::right << max << std::endl;
    } else {
      const uint32_t kHbarSize = 50;
      char hbar[kHbarSize + 1];
      const float wattPciCardMax = 75.0;
      const float barMax = cardMax >= (wattPciCardMax - 5.0) ? wattPciCardMax : cardMax + 5.0;
      const float interval = barMax / kHbarSize;

      memset(hbar, ' ', sizeof(hbar));
      hbar[kHbarSize] = '\0';

      for (uint32_t i = 0; i < kHbarSize; i++) {
        if (avg > 0 && avg >= interval * i) {
          hbar[i] = '+';
        } else if (max > 0 && max >= interval * i) {
          hbar[i] = '-';
        } else {
          break;
        }
      }

      if (addBarLabels) {
        const uint32_t labelCount = 3;
        char spacing[stat.length() + 5];
        char labelBar[kHbarSize + 1];

        memset(spacing, ' ', sizeof(spacing));
        spacing[sizeof(spacing) - 1] = '\0';
        memset(labelBar, ' ', sizeof(labelBar));
        labelBar[kHbarSize] = '\0';

        for (uint32_t i = 1; i <= labelCount; i++) {
          uint32_t index = kHbarSize / (labelCount + 1) * i;
          uint32_t label = (int32_t)barMax * i / (labelCount + 1);
          std::string labelStr = std::to_string(label) + "W";
          strncpy(&labelBar[index], labelStr.data(), labelStr.size());
        }

        std::cout << "\t" << spacing << labelBar << std::endl;
      }

      std::cout << "\t" + stat + " 0W[" + hbar + "]" << std::setw(1) << (int32_t)barMax << "W\n";
    }
  }

  return;
}

void EtTop::displayVoltStat(const std::string stat, bool endLine, uint64_t moduleVoltage, uint64_t asicVoltage) {
  std::stringstream asicVoltFormat;
  asicVoltFormat << "(" << asicVoltage << ")";
  std::cout << "\t" + stat + ": " << std::setw(5) << std::left << moduleVoltage << std::setw(7) << std::right
            << asicVoltFormat.str() << std::left << (endLine ? "\n" : " ");
}

void EtTop::displayVoltStat(const std::string stat, uint64_t moduleVoltage, const struct op_value& asicVoltage) {
  std::stringstream asicVoltFormat;
  asicVoltFormat << "(avg: " << std::setw(5) << std::left << asicVoltage.avg << " min: " << std::setw(5) << std::left
                 << asicVoltage.min << " max: " << std::left << asicVoltage.max << ")";
  std::cout << "\t" + stat + ": " << std::setw(5) << std::left << moduleVoltage << std::setw(7) << std::right
            << asicVoltFormat.str() << std::left << std::endl;
}

void EtTop::displayFreqStat(const std::string stat, bool endLine, uint64_t frequency) {
  std::cout << "\t" + stat + ": " << std::setw(6) << std::left << frequency << (endLine ? "\n" : "");
}

void EtTop::displayFreqStat(const std::string stat, const struct op_value& frequency) {
  std::cout << "\t" + stat + "   avg: " << std::setw(5) << std::left << frequency.avg << "  min: " << std::setw(5)
            << std::left << frequency.min << "  max: " << std::setw(5) << std::left << frequency.max << std::endl;
}

void EtTop::displayComputeStat(const std::string stat, const struct resource_value& rv) {
  uint64_t avg = rv.avg;
  uint64_t min = rv.min;
  uint64_t max = rv.max;

  std::cout << "\t" + stat + " avg: " << std::setw(6) << std::left << avg << " min: " << std::setw(6) << std::left
            << min << " max: " << std::setw(6) << std::left << max << std::endl;
  return;
}

void EtTop::displayErrorDetails(std::map<std::string, uint64_t>& error, bool addColon, std::string prefix) {
  for (auto const& [key, val] : error) {
    if (val > 0) {
      std::string str = prefix + key + (addColon ? ": " : " ");
      std::cout << "\t\t" << std::setw(30) << std::left << str.data() << std::setw(6) << std::left << val << std::endl;
    }
  }
  return;
}

/**
 * @brief Generates a graphical view for the performance metrics displayed by et_powertop.
 *
 * This function calls rendering function for various views represents them on screen
 */
void EtTop::displayStatsGraph(void) {
  graph_init();
  displayFreqDetails_ = true;
  displayVoltDetails_ = true;
  auto powerView = powerViewRenderer();
  auto computeView = computeViewRenderer();
  auto tempView = tempViewRenderer();
  auto freqView = freqViewRenderer();
  auto voltView = voltViewRenderer();
  auto utilizationView = utilizationViewRenderer();
  auto throughputView = throughputViewRenderer();

  renderMainDisplay(powerView, computeView, tempView, freqView, voltView, utilizationView, throughputView, this);

  displayFreqDetails_ = false;
  displayVoltDetails_ = false;
}

void EtTop::displayStats(void) {

  if (updateLimit_ > 0 && --updateLimit_ == 0) {
    stop_ = true;
  }

  if (batchMode_) {
    std::cout << std::endl;
  } else {
    system("clear");
  }
  time_t now;
  char nowbuf[30];
  char* hhmmss = &nowbuf[10];
  time(&now);
  ctime_r(&now, nowbuf);
  nowbuf[20] = '\0';

  std::cout << ET_TOP " v" ET_TOP_VERSION " device " << devNum_ << hhmmss << std::endl;
  std::cout << "Card ID: " << std::setw(10) << std::left << cardId_ << "\tETSOC FW: " << std::setw(12) << std::left
            << fwVersion_ << "\tPMIC FW: " << std::setw(12) << std::left << pmicFwVersion_ << std::endl;
  std::cout << "Contiguous Mem Alloc: " << std::setw(6) << std::right << memStats_.cmaAllocated << " MB "
            << std::setw(6) << std::right << memStats_.cmaAllocationRate << " MB/sec\n";

  struct op_value etsocPower;
  struct op_value otherPower;
  const struct op_stats_t& op = spStats_.op;
  const float cardMax = POWER_10MW_TO_W(op.system.power.max);

  otherPower.avg = kOtherPower;
  otherPower.min = kOtherPower;
  otherPower.max = kOtherPower;
  etsocPower.avg = op.minion.power.avg + op.sram.power.avg + op.noc.power.avg + otherPower.avg;
  etsocPower.min = op.minion.power.min + op.sram.power.min + op.noc.power.min + otherPower.min;
  etsocPower.max = op.minion.power.max + op.sram.power.max + op.noc.power.max + otherPower.max;

  std::cout << "Watts:";
  displayOpStat("CARD       ", op.system.power, OP_VALUE_UNIT_POWER_10MILLIWATTS, true, cardMax, true);
  displayOpStat("- ETSOC    ", etsocPower, OP_VALUE_UNIT_POWER_MILLIWATTS, true, cardMax);
  displayOpStat("  - MINION ", op.minion.power, OP_VALUE_UNIT_POWER_MILLIWATTS, true, cardMax);
  displayOpStat("  - SRAM   ", op.sram.power, OP_VALUE_UNIT_POWER_MILLIWATTS, true, cardMax);
  displayOpStat("  - NOC    ", op.noc.power, OP_VALUE_UNIT_POWER_MILLIWATTS, true, cardMax);
  displayOpStat("  - OTHER  ", otherPower, OP_VALUE_UNIT_POWER_MILLIWATTS, true, cardMax);

  std::cout << "Temp(C):\n";
  displayOpStat("ETSOC    ", op.system.temperature, OP_VALUE_UNIT_TEMPERATURE_CELCIUS);
  displayOpStat("- MINION ", op.minion.temperature, OP_VALUE_UNIT_TEMPERATURE_CELCIUS);

  std::cout << "Compute:\n";
  displayComputeStat("Utilization Minion   (%) ", mmStats_.computeResources.cm_utilization);
  displayComputeStat("            DMA Read (%) ", mmStats_.computeResources.pcie_dma_read_utilization);
  displayComputeStat("            DMA Write(%) ", mmStats_.computeResources.pcie_dma_write_utilization);
  displayComputeStat("Throughput  Kernel/sec   ", mmStats_.computeResources.cm_bw);
  displayComputeStat("DDR BW      Read  (MB/s) ", mmStats_.computeResources.ddr_read_bw);
  displayComputeStat("            Write (MB/s) ", mmStats_.computeResources.ddr_write_bw);
  displayComputeStat("SC Bank BW  Read  (MB/s) ", mmStats_.computeResources.l2_l3_read_bw);
  displayComputeStat("            Write (MB/s) ", mmStats_.computeResources.l2_l3_write_bw);
  displayComputeStat("PCI DMA BW  Read  (MB/s) ", mmStats_.computeResources.pcie_dma_read_bw);
  displayComputeStat("            Write (MB/s) ", mmStats_.computeResources.pcie_dma_write_bw);

  std::cout << "Queues:\n";
  for (uint32_t i = 0; i < vqStats_.size(); i++) {
    std::cout << "\t" << std::setw(4) << vqStats_[i].qname.data() << " msgs: " << std::setw(10) << std::left
              << vqStats_[i].msgCount << " msgs/sec: " << std::setw(10) << std::left << vqStats_[i].msgRate
              << " util%: " << std::setw(10) << std::left << vqStats_[i].utilPercent << std::endl;
  }

  if (displayFreqDetails_) {
    std::cout << "Frequencies(MHz):\n";
    displayOpStat("MINION     ", op.minion.freq, OP_VALUE_UNIT_FREQ_MEGAHERTZ);
    displayOpStat("SRAM       ", op.sram.freq, OP_VALUE_UNIT_FREQ_MEGAHERTZ);
    displayOpStat("NOC        ", op.noc.freq, OP_VALUE_UNIT_FREQ_MEGAHERTZ);
    displayFreqStat("DDR       ", false, freqStats_.ddr_mhz);
    displayFreqStat("IO SHIRE  ", true, freqStats_.io_shire_mhz);
    displayFreqStat("MEM SHIRE ", false, freqStats_.mem_shire_mhz);
    displayFreqStat("PCIE SHIRE", true, freqStats_.pcie_shire_mhz);
  }

  if (displayVoltDetails_) {
    std::cout << "Voltages(mV):\n";
    displayVoltStat("MINION       ", BIN2VOLTAGE(moduleVoltStats_.minion, 250, 5, 1), op.minion.voltage);
    displayVoltStat("SRAM         ", BIN2VOLTAGE(moduleVoltStats_.l2_cache, 250, 5, 1), op.sram.voltage);
    displayVoltStat("NOC          ", BIN2VOLTAGE(moduleVoltStats_.noc, 250, 5, 1), op.noc.voltage);
    displayVoltStat("MAXION       ", false, BIN2VOLTAGE(moduleVoltStats_.maxion, 250, 5, 1), asicVoltStats_.maxion);
    displayVoltStat("DDR          ", true, BIN2VOLTAGE(moduleVoltStats_.ddr, 250, 5, 1), asicVoltStats_.ddr);
    displayVoltStat("PShire(0p75) ", false, BIN2VOLTAGE(moduleVoltStats_.pcie_logic, 600, 625, 100),
                    asicVoltStats_.pshire_0p75);
    displayVoltStat("IOShire(0p75)", true, BIN2VOLTAGE(moduleVoltStats_.pcie_logic, 600, 625, 100),
                    asicVoltStats_.ioshire_0p75);
    displayVoltStat("VDDQ         ", false, BIN2VOLTAGE(moduleVoltStats_.vddq, 250, 10, 1),
                    BIN2VOLTAGE(asicVoltStats_.vddq, 250, 10, 1));
    displayVoltStat("VDDQLP       ", true, BIN2VOLTAGE(moduleVoltStats_.vddqlp, 250, 10, 1),
                    BIN2VOLTAGE(asicVoltStats_.vddqlp, 250, 10, 1));
  }

  bool errors = errStats_.uceCount > 0 || errStats_.ceCount > 0 || aerStats_.aerCount > 0;
  if (errors || displayErrorDetails_) {
    std::cout << "Errors:\n";
    std::cout << "\tUncorrectable: " << std::setw(6) << std::left << errStats_.uceCount << std::endl;
    if (displayErrorDetails_ && errStats_.uceCount > 0) {
      displayErrorDetails(errStats_.uce);
    }
    std::cout << "\tCorrectable:   " << std::setw(6) << std::left << errStats_.ceCount << std::endl;
    if (displayErrorDetails_ && errStats_.ceCount > 0) {
      displayErrorDetails(errStats_.ce);
    }
    std::cout << "\tPCI AER:       " << std::setw(6) << std::left << aerStats_.aerCount << std::endl;
    if (displayErrorDetails_ && aerStats_.aerCount > 0) {
      if (aerStats_.fatalCount > 0) {
        displayErrorDetails(aerStats_.fatal, true, "Fatal ");
      }
      if (aerStats_.nonfatalCount > 0) {
        displayErrorDetails(aerStats_.nonfatal, true, "Non-Fatal ");
      }
      if (aerStats_.correctableCount > 0) {
        displayErrorDetails(aerStats_.correctable, true, "Correctable ");
      }
    }
  }
  if (!batchMode_) {
    std::cout << "Type 'h' for help ";
  }

  return;
}

int main(int argc, char** argv) {
  char* endptr = NULL;
  long int devNum = 0;
  std::chrono::duration delay = std::chrono::milliseconds(kUpdateDelayMS);
  long int updateLimit = 0;
  bool graphMode = false;
  bool batchMode = false;
  bool usageError = false;
  bool resetStats = false;

  /*
   * Validate the inputs
   */
  if (argc < 2 || argc > 6) {
    usageError = true;
  } else {
    devNum = strtol(argv[1], &endptr, 10);
    if (*endptr || devNum < 0 || devNum > kMaxDeviceNum) {
      usageError = true;
    } else if (argc > 2) {
      int i = 2;

      if (argv[i][0] != '-') {
        auto delayCount = strtol(argv[i], &endptr, 10);
        if (*endptr || delayCount < 0) {
          usageError = true;
        } else {
          delay = std::chrono::milliseconds(delayCount);
        }
        i++;
      }

      while (!usageError && i < argc) {
        if (!graphMode && !strcmp(argv[i], "-g")) {
          graphMode = true;
        } else if (!batchMode && !strcmp(argv[i], "-b")) {
          batchMode = true;
        } else if (!resetStats && !strcmp(argv[i], "-r")) {
          resetStats = true;
        } else if (updateLimit == 0 && !strcmp(argv[i], "-n") && ++i < argc) {
          updateLimit = strtol(argv[i], &endptr, 10);
          usageError = *endptr || updateLimit < 1;
        } else {
          usageError = true;
        }
        i++;
      }
    }
  }

  if (usageError) {
    std::cerr << ET_TOP " version " << ET_TOP_VERSION << "\nUsage: " << argv[0] << " DEVNO [DELAY] [OPTION]\n"
              << "\tDEVNO is 0-" << kMaxDeviceNum << std::endl
              << "\tDELAY is an optional update delay in milliseconds (default " << kUpdateDelayMS << ")\n"
              << "\tOPTION is one or more of the following:\n"
              << "\t  -b operate in batch mode (accept no input and run until -n limit or killed)\n"
              << "\t  -n [NUM] display stats a maximum of NUM times before ending (NUM > 0)\n"
              << "\t  -r reset statistics at start\n";
    exit(1);
  }

  std::string devName = "/dev/et" + std::to_string(devNum) + "_mgmt";
  struct stat buf;
  if (stat(devName.data(), &buf) != 0) {
    std::cerr << devName.data() << " file error: " << std::strerror(errno) << std::endl;
    exit(1);
  }

  if (!batchMode && isatty(STDIN_FILENO) == 0) {
    std::cerr << argv[0] << ": error stdin must be a tty\n";
    exit(1);
  }

  /*
   * Enable logging of any internal errors and configure the tty for processing
   */
  logging::LoggerDefault loggerDefault_;
  g3::log_levels::disable(DEBUG);
  google::InitGoogleLogging(argv[0]);
  setbuf(stdout, NULL);

  if (!batchMode) {
    if (tcgetattr(STDIN_FILENO, &orig_termios) != 0) {
      DV_LOG(ERROR) << "tcgetattr on stdin error: " << std::strerror(errno);
      exit(1);
    }

    if (atexit(restoreTTY) != 0) {
      DV_LOG(ERROR) << "atexit error";
      exit(1);
    }

    struct termios new_termios = orig_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VTIME] = 1;
    new_termios.c_cc[VMIN] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) != 0) {
      DV_LOG(ERROR) << "tcsetattr on stdin error: " << std::strerror(errno);
      exit(1);
    }
  }

  /*
   * Enter stats processing loop
   */
  std::unique_ptr<dev::IDeviceLayer> dl = dev::IDeviceLayer::createPcieDeviceLayer(false, true);
  device_management::DeviceManagement& dm = device_management::DeviceManagement::getInstance(dl.get());
  EtTop etTop(devNum, dl, dm, batchMode, updateLimit);

  auto checkPoint = std::chrono::steady_clock::now();
  if (resetStats) {
    etTop.resetStats();
    checkPoint += delay;
  }
  if (graphMode) {
    etTop.displayStatsGraph();
  } else {
    while (!etTop.stopStats()) {
      if (!batchMode) {
        etTop.processInput();
      }

      if (checkPoint > std::chrono::steady_clock::now()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      } else {
        checkPoint = std::chrono::steady_clock::now() + delay;
        etTop.collectStats();
        etTop.displayStats();
      }
    }
    std::cout << std::endl;
  }
  return 0;
}
