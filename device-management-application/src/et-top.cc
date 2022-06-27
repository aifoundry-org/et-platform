/*-------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "../../deviceManagement/src/utils.h"
#include "deviceLayer/IDeviceLayer.h"
#include "deviceManagement/DeviceManagement.h"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <dlfcn.h>
#include <glog/logging.h>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#define ET_TRACE_DECODER_IMPL
#include "esperanto/et-trace/decoder.h"
#include "esperanto/et-trace/layout.h"

static struct termios orig_termios;

static void restoreTTY(void) {
  if (tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios) != 0) {
    DV_LOG(ERROR) << "tcsetattr restore on stdin error: " << std::strerror(errno);
  }
  return;
}

typedef struct {
  std::string qname;
  uint64_t msgCount;
  uint64_t msgRate;
  uint64_t utilPercent;
} vq_stats_t;

typedef std::array<vq_stats_t, 3> vq_stats_array_t;

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

  vq_stats_array_t vqStats_;
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
      DV_LOG(ERROR) << "aer_dev_fatal count mismatch: " << aerStats_.fatalCount << " != " << total;
    }
    aerStats_.fatal.erase("TOT_ERR_FATAL");
  }

  if (!nonfatalFileError) {
    nonfatalFileError = !processErrorFile("aer_dev_nonfatal", aerStats_.nonfatal, aerStats_.nonfatalCount);

    uint64_t total = aerStats_.nonfatal["TOT_ERR_NONFATAL"];
    aerStats_.nonfatalCount >>= 1;
    if (aerStats_.nonfatalCount != total) {
      DV_LOG(ERROR) << "aer_dev_nonfatal count mismatch: " << aerStats_.nonfatalCount << " != " << total;
    }
    aerStats_.nonfatal.erase("TOT_ERR_NONFATAL");
  }

  if (!correctableFileError) {
    correctableFileError = !processErrorFile("aer_dev_correctable", aerStats_.correctable, aerStats_.correctableCount);

    uint64_t total = aerStats_.correctable["TOT_ERR_COR"];
    aerStats_.correctableCount >>= 1;
    if (aerStats_.correctableCount != total) {
      DV_LOG(ERROR) << "aer_dev_correctable count mismatch: " << aerStats_.correctableCount << " != " << total;
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

void EtTop::displayErrorDetails(std::map<std::string, uint64_t>& error, bool addColon, std::string prefix) {
  for (auto const& [key, val] : error) {
    if (val > 0) {
      std::string str = prefix + key + (addColon ? ": " : " ");
      printf("\t\t%-30s %-6lu\n", str.data(), val);
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

  printf("Contiguous Mem Alloc: %6lu MB %6lu MB/sec\n", memStats_.cmaAllocated, memStats_.cmaAllocationRate);

  std::cout << "Watts:                                            Temp(C):\n";
  struct op_value* power = &spStats_.op.system.power;
  printf("\tCARD        avg: %-4u min: %-4u max: %-4u\n", power->avg, power->min, power->max);

  struct op_value* temp = &spStats_.op.system.temperature;
  printf("\t- ETSOC     avg: %-4u min: %-4u max: %-4u ETSOC     avg: %-4u min: %-4u max: %-4u\n",
         spStats_.op.minion.power.avg + spStats_.op.sram.power.min + spStats_.op.noc.power.max,
         spStats_.op.minion.power.avg + spStats_.op.sram.power.min + spStats_.op.noc.power.max,
         spStats_.op.minion.power.avg + spStats_.op.sram.power.min + spStats_.op.noc.power.max, temp->avg, temp->min,
         temp->max);

  power = &spStats_.op.minion.power;
  temp = &spStats_.op.minion.temperature;
  printf("\t  - MINION  avg: %-4u min: %-4u max: %-4u - MINION  avg: %-4u min: %-4u max: %-4u\n", power->avg,
         power->min, power->max, temp->avg, temp->min, temp->max);

  power = &spStats_.op.sram.power;
  printf("\t  - SRAM    avg: %-4u min: %-4u max: %-4u\n", power->avg, power->min, power->max);

  power = &spStats_.op.noc.power;
  printf("\t  - NOC     avg: %-4u min: %-4u max: %-4u\n", power->avg, power->min, power->max);

  // XXX sanitize values for display until the mm stat implementation is finished
  std::array<struct resource_value*, 7> a = {
    &mmStats_.computeResources.cm_utilization,   &mmStats_.computeResources.ddr_read_bw,
    &mmStats_.computeResources.ddr_write_bw,     &mmStats_.computeResources.l2_l3_read_bw,
    &mmStats_.computeResources.l2_l3_write_bw,   &mmStats_.computeResources.pcie_dma_read_bw,
    &mmStats_.computeResources.pcie_dma_write_bw};
  for (int i = 0; i < a.size(); i++) {
    a[i]->avg %= 100000UL;
    a[i]->min %= 100000UL;
    a[i]->max %= 100000UL;
  }

  std::cout << "Compute:\n";
  // XXX  waiting for implementation of this data
  printf("\tThru put    Kernel/sec    avg: %-6lu min: %-6lu max: %-6lu\n", 0UL, 0UL, 0UL);

  const resource_value* cm = &mmStats_.computeResources.cm_utilization;
  printf("\tUtil        Minion(%)     avg: %-6lu min: %-6lu max: %-6lu\n", cm->avg, cm->min, cm->max);
  // XXX  waiting for implementation of this data
  printf("\t            DMA Chan(%)   avg: %-6lu min: %-6lu max: %-6lu\n", 0UL, 0UL, 0UL);

  struct resource_value* rd_bw = &mmStats_.computeResources.ddr_read_bw;
  struct resource_value* wr_bw = &mmStats_.computeResources.ddr_write_bw;
  printf("\tDDR BW      Read  (MB/s)  avg: %-6lu min: %-6lu max: %-6lu\n", rd_bw->avg, rd_bw->min, rd_bw->max);
  printf("\t            Write (MB/s)  avg: %-6lu min: %-6lu max: %-6lu\n", wr_bw->avg, wr_bw->min, wr_bw->max);

  rd_bw = &mmStats_.computeResources.l2_l3_read_bw;
  wr_bw = &mmStats_.computeResources.l2_l3_write_bw;
  printf("\tL3 BW       Read  (MB/s)  avg: %-6lu min: %-6lu max: %-6lu\n", rd_bw->avg, rd_bw->min, rd_bw->max);
  printf("\t            Write (MB/s)  avg: %-6lu min: %-6lu max: %-6lu\n", wr_bw->avg, wr_bw->min, wr_bw->max);

  rd_bw = &mmStats_.computeResources.pcie_dma_read_bw;
  wr_bw = &mmStats_.computeResources.pcie_dma_write_bw;
  printf("\tPCI DMA BW  Read  (GB/s)  avg: %-6lu min: %-6lu max: %-6lu\n", rd_bw->avg, rd_bw->min, rd_bw->max);
  printf("\t            Write (GB/s)  avg: %-6lu min: %-6lu max: %-6lu\n", wr_bw->avg, wr_bw->min, wr_bw->max);

  std::cout << "Queues:\n";
  for (uint32_t i = 0; i < vqStats_.size(); i++) {
    printf("\t%4s msgs: %-10lu msgs/sec: %-10lu util%%: %-10lu\n", vqStats_[i].qname.data(), vqStats_[i].msgCount,
           vqStats_[i].msgRate, vqStats_[i].utilPercent);
  }

  bool errors = errStats_.uceCount > 0 || errStats_.ceCount > 0 || aerStats_.aerCount > 0;
  if (errors || displayErrorDetails_) {
    std::cout << "Errors:\n";
    printf("\tUncorrectable: %-6lu\n", errStats_.uceCount);
    if (displayErrorDetails_ && errStats_.uceCount > 0) {
      displayErrorDetails(errStats_.uce);
    }
    printf("\tCorrectable:   %-6lu\n", errStats_.ceCount);
    if (displayErrorDetails_ && errStats_.ceCount > 0) {
      displayErrorDetails(errStats_.ce);
    }
    printf("\tPCI AER:       %-6lu\n", aerStats_.aerCount);
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
  printf("Type 'h' for help ");

  return;
}

int main(int argc, char** argv) {
  char* endptr = NULL;
  long int devNum = 0;
  long int delay = 1;
  bool usageError = false;
  const int32_t kMaxDeviceNum = 63;

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
  system("clear"); // XXX eliminate the need for this
  device_management::DeviceManagement& dm = (*dmi)(dl.get());

  /*
   * Enter stats processing loop
   */
  EtTop etTop(devNum, dl, dm);
  int elapsed = delay;

  while (!etTop.stopStats()) {
    etTop.processInput();

    if (delay > 0 && delay > elapsed) {
      elapsed++;
    } else {
      elapsed = 0;
      etTop.collectStats();
      etTop.displayStats();
    }

    if (delay > 0) {
      sleep(1);
    }
  }

  return 0;
}
