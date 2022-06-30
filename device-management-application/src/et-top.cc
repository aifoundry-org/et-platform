/*-------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "deviceLayer/IDeviceLayer.h"
#include "deviceManagement/DeviceManagement.h"
#define ET_TRACE_DECODER_IMPL
#include "esperanto/et-trace/decoder.h"
#include "esperanto/et-trace/layout.h"
#include <hostUtils/logging/Logging.h>

#include <cstdio>
#include <cstring>
#include <ctime>
#include <dlfcn.h>
#include <glog/logging.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#define DV_LOG(severity) ET_LOG(ET_TOP, severity)
#define DV_DLOG(severity) ET_DLOG(ET_TOP, severity)
#define DV_VLOG(level) ET_VLOG(ET_TOP, level)

static const int32_t kMaxDeviceNum = 63;
static const int32_t kOpsCqNum = 1;
static const int32_t kOpsSqNum = 2;

static struct termios orig_termios;

static void restoreTTY(void) {
  if (tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios) != 0) {
    DV_LOG(WARNING) << "tcsetattr restore on stdin error: " << std::strerror(errno);
  }
  return;
}

typedef struct {
  std::string qname;
  uint64_t msgCount;
  uint64_t msgRate;
  uint64_t utilPercent;
} vq_stats_t;

typedef struct {
  uint64_t cmaAllocated;
  uint64_t cmaAllocationRate;
} mem_stats_t;

typedef struct {
  uint64_t ceCount;
  uint64_t uceCount;
  std::map<std::string, uint64_t> ce;
  std::map<std::string, uint64_t> uce;
} err_stats_t;

typedef struct {
  uint64_t aerCount;
  uint64_t fatalCount;
  uint64_t nonfatalCount;
  uint64_t correctableCount;
  std::map<std::string, uint64_t> fatal;
  std::map<std::string, uint64_t> nonfatal;
  std::map<std::string, uint64_t> correctable;
} aer_stats_t;

typedef struct {
  uint64_t cycle;
  op_stats_t op;
} sp_stats_t;

typedef struct {
  struct compute_resources_sample computeResources;
} mm_stats_t;

class EtTop {
public:
  EtTop(int devNum, std::unique_ptr<dev::IDeviceLayer>& dl, device_management::DeviceManagement& dm)
    : devNum_(devNum)
    , dl_(dl)
    , dm_(dm)
    , stop_(false)
    , displayErrorDetails_(false) {
    vqStats_[0].qname = "SQ0:";
    vqStats_[1].qname = "SQ1:";
    vqStats_[2].qname = "CQ0:";
    spStats_.cycle = 0;
  }

  void processInput(void);
  void displayStats(void);
  bool stopStats(void) {
    return stop_;
  }
  void collectStats(void) {
    collectMemStats();
    collectErrStats();
    collectAerStats();
    collectVqStats();
    collectSpStats();
    collectMmStats();
    return;
  }

private:
  bool processErrorFile(std::string relAttrPath, std::map<std::string, uint64_t>& error, uint64_t& total);
  void displayComputeStat(const std::string stat, const struct resource_value& rv, const bool convertMBtoGB = false);
  void displayErrorDetails(std::map<std::string, uint64_t>& error, bool addColon = false, std::string prefix = "");
  void collectMemStats(void);
  void collectErrStats(void);
  void collectAerStats(void);
  void collectVqStats(void);
  void collectSpStats(void);
  void collectMmStats(void);

  int devNum_;
  bool stop_;
  bool displayErrorDetails_;
  std::unique_ptr<dev::IDeviceLayer>& dl_;
  device_management::DeviceManagement& dm_;

  std::array<vq_stats_t, kOpsSqNum + kOpsCqNum> vqStats_;
  mem_stats_t memStats_;
  err_stats_t errStats_;
  aer_stats_t aerStats_;
  sp_stats_t spStats_;
  mm_stats_t mmStats_;
};

void EtTop::collectMemStats(void) {
  std::string dummy;
  std::string line;

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

    for (uint32_t i = 0; i < vqStats_.size(); i++) {
      if (qname == vqStats_[i].qname) {
        vqStats_[i].msgCount = num;
        break;
      }
    }
  }

  std::istringstream attrFileRate(dl_->getDeviceAttribute(devNum_, "ops_vq_stats/msg_rate"));
  for (std::string line; std::getline(attrFileRate, line);) {
    std::stringstream ss;
    ss << line;
    ss >> qname >> num >> dummy;

    for (uint32_t i = 0; i < vqStats_.size(); i++) {
      if (qname == vqStats_[i].qname) {
        vqStats_[i].msgRate = num;
        break;
      }
    }
  }

  std::istringstream attrFileUtil(dl_->getDeviceAttribute(devNum_, "ops_vq_stats/utilization_percent"));
  for (std::string line; std::getline(attrFileUtil, line);) {
    std::stringstream ss;
    ss << line;
    ss >> qname >> num >> dummy;

    for (uint32_t i = 0; i < vqStats_.size(); i++) {
      if (qname == vqStats_[i].qname) {
        vqStats_[i].utilPercent = num;
        break;
      }
    }
  }

  return;
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

    uint64_t total = aerStats_.fatal["TOT_ERR_FATAL"];
    aerStats_.fatalCount >>= 1;
    if (aerStats_.fatalCount != total) {
      DV_LOG(WARNING) << "aer_dev_fatal count mismatch: " << aerStats_.fatalCount << " != " << total;
    }
    aerStats_.fatal.erase("TOT_ERR_FATAL");
  }

  if (!nonfatalFileError) {
    nonfatalFileError = !processErrorFile("aer_dev_nonfatal", aerStats_.nonfatal, aerStats_.nonfatalCount);

    uint64_t total = aerStats_.nonfatal["TOT_ERR_NONFATAL"];
    aerStats_.nonfatalCount >>= 1;
    if (aerStats_.nonfatalCount != total) {
      DV_LOG(WARNING) << "aer_dev_nonfatal count mismatch: " << aerStats_.nonfatalCount << " != " << total;
    }
    aerStats_.nonfatal.erase("TOT_ERR_NONFATAL");
  }

  if (!correctableFileError) {
    correctableFileError = !processErrorFile("aer_dev_correctable", aerStats_.correctable, aerStats_.correctableCount);

    uint64_t total = aerStats_.correctable["TOT_ERR_COR"];
    aerStats_.correctableCount >>= 1;
    if (aerStats_.correctableCount != total) {
      DV_LOG(WARNING) << "aer_dev_correctable count mismatch: " << aerStats_.correctableCount << " != " << total;
    }
    aerStats_.correctable.erase("TOT_ERR_COR");
  }

  aerStats_.aerCount = aerStats_.fatalCount + aerStats_.fatalCount + aerStats_.correctableCount;

  return;
}
void EtTop::collectSpStats(void) {
  std::vector<std::byte> response;

  if (dm_.getTraceBufferServiceProcessor(devNum_, TraceBufferType::TraceBufferSPStats, response) !=
      device_mgmt_api::DM_STATUS_SUCCESS) {
    DV_LOG(ERROR) << "getTraceBufferServiceProcessor error";
    exit(1);
  }

  struct trace_buffer_std_header_t* tb_hdr;
  tb_hdr = reinterpret_cast<struct trace_buffer_std_header_t*>(response.data());
  if (tb_hdr->type != TRACE_SP_STATS_BUFFER) {
    DV_LOG(ERROR) << "Trace buffer invalid type: " << tb_hdr->type << " (must be TRACE_SP_STATS_BUFFER "
                  << TRACE_SP_STATS_BUFFER << ")";
    exit(1);
  }

  const struct trace_entry_header_t* entry = NULL;
  while ((entry = Trace_Decode(tb_hdr, entry))) {
    if (entry->type != TRACE_TYPE_CUSTOM_EVENT) {
      DV_LOG(ERROR) << "Trace type not custom event error: " << entry->type;
      exit(1);
    }

    if (entry->cycle <= spStats_.cycle) {
      continue;
    }
    spStats_.cycle = entry->cycle;

    const struct trace_custom_event_t* cev;
    cev = reinterpret_cast<const struct trace_custom_event_t*>(entry);
    if (cev->custom_type != TRACE_CUSTOM_TYPE_SP_OP_STATS) {
      continue;
    }

    const struct op_stats_t* op_stats;
    op_stats = reinterpret_cast<const struct op_stats_t*>(cev->payload);
    spStats_.op = *op_stats;
  }

  return;
}

void EtTop::collectMmStats(void) {
  std::vector<std::byte> response;

  if (dm_.getTraceBufferServiceProcessor(devNum_, TraceBufferType::TraceBufferMMStats, response) !=
      device_mgmt_api::DM_STATUS_SUCCESS) {
    DV_LOG(ERROR) << "getTraceBufferServiceProcessor error";
    exit(1);
  }

  struct trace_buffer_std_header_t* tb_hdr;
  tb_hdr = reinterpret_cast<struct trace_buffer_std_header_t*>(response.data());
  if (tb_hdr->type != TRACE_MM_STATS_BUFFER) {
    DV_LOG(ERROR) << "Trace buffer invalid type: " << tb_hdr->type << " (must be TRACE_MM_STATS_BUFFER "
                  << TRACE_MM_STATS_BUFFER << ")";
    exit(1);
  }

  /* XXX only uses data from the last entry */
  const struct trace_entry_header_t* entry = NULL;
  while ((entry = Trace_Decode(tb_hdr, entry))) {
    if (entry->type != TRACE_TYPE_CUSTOM_EVENT) {
      DV_LOG(ERROR) << "Trace type not custom event error: " << entry->type;
      exit(1);
    }

    const struct trace_custom_event_t* cev;
    cev = reinterpret_cast<const struct trace_custom_event_t*>(entry);
    if (cev->custom_type != TRACE_CUSTOM_TYPE_MM_COMPUTE_RESOURCES) {
      continue;
    }

    const struct compute_resources_sample* crsPtr;
    crsPtr = reinterpret_cast<const struct compute_resources_sample*>(cev->payload);
    mmStats_.computeResources = *crsPtr;
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
      std::cout << std::endl;
    } else if (ch == 'e') {
      displayErrorDetails_ = !displayErrorDetails_;
    } else if (ch == 'h') {
      help = true;
      system("clear");
      std::cout << "e\tToggle display of error details\n"
                << "h\tPrint this help message\n"
                << "q\tQuit\n"
                << "Type 'q' or <ESC> to continue ";
    }
  } while (help || (!stop_ && rc == 1));

  return;
}

void EtTop::displayComputeStat(const std::string stat, const struct resource_value& rv, const bool convertMBtoGB) {
  uint64_t avg = rv.avg;
  uint64_t min = rv.min;
  uint64_t max = rv.max;

  if (convertMBtoGB) {
    avg /= 1000;
    min /= 1000;
    max /= 1000;
  }
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

void EtTop::displayStats(void) {
  time_t now;
  char nowbuf[30];
  char* hhmmss = &nowbuf[10];

  system("clear");
  time(&now);
  ctime_r(&now, nowbuf);
  nowbuf[20] = '\0';
  std::cout << "et-top device " << devNum_ << hhmmss << std::endl;
  std::cout << "Contiguous Mem Alloc: " << std::setw(6) << std::right << memStats_.cmaAllocated << " MB "
            << std::setw(6) << std::right << memStats_.cmaAllocationRate << " MB/sec\n";

  std::cout << "Watts:                                            Temp(C):\n";
  struct op_value* power = &spStats_.op.system.power;
  std::cout << "\tCARD        avg: " << std::setw(4) << std::left << power->avg << " min: " << std::setw(4) << std::left
            << power->min << " max: " << std::setw(4) << std::left << power->max << std::endl;

  uint64_t avg = spStats_.op.minion.power.avg + spStats_.op.sram.power.avg + spStats_.op.noc.power.avg;
  uint64_t min = spStats_.op.minion.power.min + spStats_.op.sram.power.min + spStats_.op.noc.power.min;
  uint64_t max = spStats_.op.minion.power.max + spStats_.op.sram.power.max + spStats_.op.noc.power.max;
  struct op_value* temp = &spStats_.op.system.temperature;
  std::cout << "\t- ETSOC     avg: " << std::setw(4) << std::left << avg << " min: " << std::setw(4) << std::left << min
            << " max: " << std::setw(4) << std::left << max << " ETSOC     avg: " << std::setw(4) << std::left
            << temp->avg << " min: " << std::setw(4) << std::left << temp->min << " max: " << std::setw(4) << std::left
            << temp->max << std::endl;

  power = &spStats_.op.minion.power;
  temp = &spStats_.op.minion.temperature;
  std::cout << "\t  - MINION  avg: " << std::setw(4) << std::left << power->avg << " min: " << std::setw(4) << std::left
            << power->min << " max: " << std::setw(4) << std::left << power->max << " - MINION  avg: " << std::setw(4)
            << std::left << temp->avg << " min: " << std::setw(4) << std::left << temp->min << " max: " << std::setw(4)
            << std::left << temp->max << std::endl;

  power = &spStats_.op.sram.power;
  std::cout << "\t  - SRAM    avg: " << std::setw(4) << std::left << power->avg << " min: " << std::setw(4) << std::left
            << power->min << " max: " << std::setw(4) << std::left << power->max << std::endl;

  power = &spStats_.op.noc.power;
  std::cout << "\t  - NOC     avg: " << std::setw(4) << std::left << power->avg << " min: " << std::setw(4) << std::left
            << power->min << " max: " << std::setw(4) << std::left << power->max << std::endl;

  std::cout << "Compute:\n";
  resource_value dummy = {0, 0, 0}; // XXX eliminate this when implementation is ready
  displayComputeStat("Throughput  Kernel/sec   ", mmStats_.computeResources.cm_utilization);
  displayComputeStat("Util        Minion(%)    ", dummy);
  displayComputeStat("            DMA Chan(%)  ", dummy);
  displayComputeStat("DDR BW      Read  (MB/s) ", mmStats_.computeResources.ddr_read_bw);
  displayComputeStat("            Write (MB/s) ", mmStats_.computeResources.ddr_write_bw);
  displayComputeStat("L3 BW       Read  (MB/s) ", mmStats_.computeResources.l2_l3_read_bw);
  displayComputeStat("            Write (MB/s) ", mmStats_.computeResources.l2_l3_write_bw);
  displayComputeStat("PCI DMA BW  Read  (GB/s) ", mmStats_.computeResources.pcie_dma_read_bw, true);
  displayComputeStat("            Write (GB/s) ", mmStats_.computeResources.pcie_dma_write_bw, true);

  std::cout << "Queues:\n";
  for (uint32_t i = 0; i < vqStats_.size(); i++) {
    std::cout << "\t" << std::setw(4) << vqStats_[i].qname.data() << " msgs: " << std::setw(10) << std::left
              << vqStats_[i].msgCount << " msgs/sec: " << std::setw(10) << std::left << vqStats_[i].msgRate
              << " util%: " << std::setw(10) << std::left << vqStats_[i].utilPercent << std::endl;
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

  std::cout << "Type 'h' for help ";
  return;
}

int main(int argc, char** argv) {
  char* endptr = NULL;
  long int devNum = 0;
  long int delay = 1;
  bool usageError = false;

  /*
   * Validate the inputs
   */
  if (argc != 2 && argc != 3) {
    usageError = true;
  } else {
    devNum = strtol(argv[1], &endptr, 10);
    if (*endptr || devNum < 0 || devNum > kMaxDeviceNum) {
      usageError = true;
    } else if (argc == 3) {
      delay = strtol(argv[2], &endptr, 10);
      if (*endptr || delay < 0) {
        usageError = true;
      }
    }
  }

  if (usageError) {
    std::cerr << "Usage: " << argv[0] << " DEVNO [DELAY]\n"
              << "\t\twhere DEVNO is 0-" << kMaxDeviceNum << " and optional update DELAY is in seconds\n";
    exit(1);
  }

  std::string devName = "/dev/et" + std::to_string(devNum) + "_mgmt";
  struct stat buf;
  if (stat(devName.data(), &buf) != 0) {
    std::cerr << devName.data() << " file error: " << std::strerror(errno) << std::endl;
    exit(1);
  }

  if (isatty(STDIN_FILENO) == 0) {
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

  /*
   * Set up access to the target device stats
   */
  device_management::getDM_t dmi;
  void* handle = dlopen("libDM.so", RTLD_LAZY);
  if (!handle) {
    DV_LOG(ERROR) << "dlopen error " << dlerror();
    exit(1);
  } else {
    const char* error;

    dmi = reinterpret_cast<device_management::getDM_t>(dlsym(handle, "getInstance"));
    if ((error = dlerror())) {
      DV_LOG(ERROR) << "dlsym error " << error;
      exit(1);
    }
  }

  std::unique_ptr<dev::IDeviceLayer> dl = dev::IDeviceLayer::createPcieDeviceLayer(false, true);
  device_management::DeviceManagement& dm = (*dmi)(dl.get());

  /*
   * Enter stats processing loop
   */
  EtTop etTop(devNum, dl, dm);
  int elapsed = delay;

  while (1) {
    if (delay > 0 && delay > elapsed) {
      elapsed++;
    } else {
      elapsed = 0;
      etTop.collectStats();
      etTop.displayStats();
    }

    etTop.processInput();

    if (etTop.stopStats()) {
      break;
    }

    if (delay > 0) {
      sleep(1);
    }
  }

  return 0;
}
