/*-------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "../src/utils.h"
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
} err_stats_t;

typedef struct {
  uint64_t errCount;
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
    , stop_(false) {
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
  void collectMemStats(void);
  void collectErrStats(void);
  void collectAerStats(void);
  void collectVqStats(void);
  void collectSpStats(void);
  void collectMmStats(void);

  int devNum_;
  bool stop_;
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

void EtTop::collectErrStats(void) {
  uint64_t num;
  std::string dummy;

  errStats_.ceCount = 0;
  std::istringstream attrFileCeCount(dl_->getDeviceAttribute(devNum_, "err_stats/ce_count"));
  for (std::string line; std::getline(attrFileCeCount, line);) {
    std::stringstream ss;
    ss << line;
    ss >> dummy >> num;
    // XXX save individual stats in addition to accumulating
    if (num > 0) {
      errStats_.ceCount += num;
    }
  }

  errStats_.uceCount = 0;
  std::istringstream attrFileUceCount(dl_->getDeviceAttribute(devNum_, "err_stats/uce_count"));
  for (std::string line; std::getline(attrFileUceCount, line);) {
    std::stringstream ss;
    ss << line;
    ss >> dummy >> num;
    // XXX save individual stats in addition to accumulating
    if (num > 0) {
      errStats_.uceCount += num;
    }
  }

  return;
}

void EtTop::collectAerStats(void) {
  // XXX read aer_dev_fatal, aer_dev_nonfatal, and aer_dev_correctable files
  aerStats_.errCount = 0;
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
    } else if (ch == 'h') {
      help = true;
      system("clear");
      std::cout << "h\tPrint this help message\n";
      std::cout << "q\tQuit\n\n";
      std::cout << "Type 'q' or <ESC> to continue ";
    }
  } while (help || (!stop_ && rc == 1));

  return;
}

void EtTop::displayStats(void) {
  time_t now;

  system("clear");
  time(&now);
  std::cout << "ET device " << devNum_ << " stats " << ctime(&now);

  printf("\nContiguous Mem Alloc: %6lu MB %6lu MB/sec\n", memStats_.cmaAllocated, memStats_.cmaAllocationRate);
  printf("Uncorrectable Errors: %6lu\n", errStats_.uceCount);
  printf("Correctable Errors:   %6lu\n", errStats_.ceCount);
  printf("PCI AER Errors:       %6lu\n", aerStats_.errCount);

  std::cout << "Queues:\n";
  for (uint32_t i = 0; i < vqStats_.size(); i++) {
    printf("\t%5s msgs: %-10lu msgs/sec: %-10lu util%%: %-10lu\n", vqStats_[i].qname.data(), vqStats_[i].msgCount,
           vqStats_[i].msgRate, vqStats_[i].utilPercent);
  }

  std::cout << "\nWatts:                                            Temp(C):\n";
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
