/*-------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <sstream>
#include "deviceLayer/IDeviceLayer.h"
#include "deviceManagement/DeviceManagement.h"
#define ET_TRACE_DECODER_IMPL
#include "esperanto/et-trace/decoder.h"
#include "esperanto/et-trace/layout.h"

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
  EtTop(int devNum, std::unique_ptr<dev::IDeviceLayer> &dl,
        device_management::DeviceManagement &dm)
      : devNum_(devNum), dl_(dl), dm_(dm) {
    vqStats_[0].qname = "SQ0:";
    vqStats_[1].qname = "SQ1:";
    vqStats_[2].qname = "CQ0:";
    spStats_.cycle = 0;
  }

  void displayStats(void);
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
  std::unique_ptr<dev::IDeviceLayer> &dl_;
  device_management::DeviceManagement &dm_;

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

  std::istringstream attrFileAlloc(
      dl_->getDeviceAttribute(devNum_, "mem_stats/cma_allocated"));
  for (std::string line; std::getline(attrFileAlloc, line);) {
    std::stringstream ss;
    ss << line;
    ss >> memStats_.cmaAllocated >> dummy;
  }

  std::istringstream attrFileRate(
      dl_->getDeviceAttribute(devNum_, "mem_stats/cma_allocation_rate"));
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

  std::istringstream attrFileCount(
      dl_->getDeviceAttribute(devNum_, "ops_vq_stats/msg_count"));
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

  std::istringstream attrFileRate(
      dl_->getDeviceAttribute(devNum_, "ops_vq_stats/msg_rate"));
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

  std::istringstream attrFileUtil(
      dl_->getDeviceAttribute(devNum_, "ops_vq_stats/utilization_percent"));
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
  std::string line;

  errStats_.ceCount = 0;
  std::istringstream attrFileCeCount(
      dl_->getDeviceAttribute(devNum_, "err_stats/ce_count"));
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
  std::istringstream attrFileUceCount(
      dl_->getDeviceAttribute(devNum_, "err_stats/uce_count"));
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

  if (dm_.getTraceBufferServiceProcessor(
          devNum_, TraceBufferType::TraceBufferSPStats, response) !=
      device_mgmt_api::DM_STATUS_SUCCESS) {
    std::cerr << "getTraceBufferServiceProcessor fail\n";
    exit(1);
  }

  struct trace_buffer_std_header_t *tb_hdr;
  tb_hdr =
      reinterpret_cast<struct trace_buffer_std_header_t *>(response.data());
  if (tb_hdr->type != TRACE_SP_STATS_BUFFER) {
    std::cerr << "Trace buffer invalid type: " << tb_hdr->type
              << " (must be TRACE_SP_STATS_BUFFER " << TRACE_SP_STATS_BUFFER
              << ")" << std::endl;
    exit(1);
  }

  const struct trace_entry_header_t *entry = NULL;
  while ((entry = Trace_Decode(tb_hdr, entry))) {
    if (entry->type != TRACE_TYPE_CUSTOM_EVENT) {
      std::cerr << "Trace type not custom event fail: " << entry->type
                << std::endl;
      exit(1);
    }

    if (entry->cycle <= spStats_.cycle) {
      continue;
    }
    spStats_.cycle = entry->cycle;

    const struct trace_custom_event_t *cev;
    cev = reinterpret_cast<const struct trace_custom_event_t *>(entry);
    if (cev->custom_type != TRACE_CUSTOM_TYPE_SP_OP_STATS) {
      continue;
    }

    const struct op_stats_t *op_stats;
    op_stats = reinterpret_cast<const struct op_stats_t *>(cev->payload);
    spStats_.op = *op_stats;
  }

  return;
}

void EtTop::collectMmStats(void) {
  std::vector<std::byte> response;

  if (dm_.getTraceBufferServiceProcessor(
          devNum_, TraceBufferType::TraceBufferMMStats, response) !=
      device_mgmt_api::DM_STATUS_SUCCESS) {
    std::cerr << "getTraceBufferServiceProcessor fail\n";
    exit(1);
  }

  struct trace_buffer_std_header_t *tb_hdr;
  tb_hdr =
      reinterpret_cast<struct trace_buffer_std_header_t *>(response.data());
  if (tb_hdr->type != TRACE_MM_STATS_BUFFER) {
    std::cerr << "Trace buffer invalid type: " << tb_hdr->type
              << " (must be TRACE_MM_STATS_BUFFER " << TRACE_MM_STATS_BUFFER
              << ")" << std::endl;
    exit(1);
  }

  /* XXX only uses data from the last entry */
  const struct trace_entry_header_t *entry = NULL;
  while ((entry = Trace_Decode(tb_hdr, entry))) {
    if (entry->type != TRACE_TYPE_CUSTOM_EVENT) {
      std::cerr << "Trace type not custom event fail: " << entry->type
                << std::endl;
      exit(1);
    }

    const struct trace_custom_event_t *cev;
    cev = reinterpret_cast<const struct trace_custom_event_t *>(entry);
    if (cev->custom_type != TRACE_CUSTOM_TYPE_MM_COMPUTE_RESOURCES) {
      continue;
    }

    const struct compute_resources_sample *crsPtr;
    crsPtr =
        reinterpret_cast<const struct compute_resources_sample *>(cev->payload);
    mmStats_.computeResources = *crsPtr;
  }

  return;
}

void EtTop::displayStats(void) {
  time_t now;

  system("clear");
  time(&now);
  std::cout << "ET device " << devNum_ << " stats " << ctime(&now);

  printf("\nContiguous Mem Alloc: %6lu MB %6lu MB/sec\n",
         memStats_.cmaAllocated, memStats_.cmaAllocationRate);
  printf("Uncorrectable Errors: %6lu\n", errStats_.uceCount);
  printf("Correctable Errors:   %6lu\n", errStats_.ceCount);
  printf("PCI AER Errors:       %6lu\n", aerStats_.errCount);

  std::cout << "Queues:\n";
  for (uint32_t i = 0; i < vqStats_.size(); i++) {
    printf("\t%5s msgs: %-10lu msgs/sec: %-10lu util%%: %-10lu\n",
           vqStats_[i].qname.data(), vqStats_[i].msgCount, vqStats_[i].msgRate,
           vqStats_[i].utilPercent);
  }

  std::cout << "Watts:                                      Temp(C):\n";
  struct op_value *power = &spStats_.op.minion.power;
  struct op_value *temp = &spStats_.op.minion.temperature;
  printf(
      "\tMINION avg: %-4u min: %-4u max: %-4u        avg: %-4u min: %-4u max: "
      "%-4u\n",
      power->avg, power->min, power->max, temp->avg, temp->min, temp->max);

  power = &spStats_.op.sram.power;
  temp = &spStats_.op.sram.temperature;
  printf(
      "\tSRAM   avg: %-4u min: %-4u max: %-4u        avg: %-4u min: %-4u max: "
      "%-4u\n",
      power->avg, power->min, power->max, temp->avg, temp->min, temp->max);

  power = &spStats_.op.noc.power;
  temp = &spStats_.op.noc.temperature;
  printf(
      "\tNOC    avg: %-4u min: %-4u max: %-4u        avg: %-4u min: %-4u max: "
      "%-4u\n",
      power->avg, power->min, power->max, temp->avg, temp->min, temp->max);

  power = &spStats_.op.system.power;
  temp = &spStats_.op.system.temperature;
  printf(
      "\tSYSTEM avg: %-4u min: %-4u max: %-4u        avg: %-4u min: %-4u max: "
      "%-4u\n",
      power->avg, power->min, power->max, temp->avg, temp->min, temp->max);

  std::cout << "Compute:\n";
  const resource_value *cm = &mmStats_.computeResources.cm_utilization;
  printf("\tUtil%%            avg: %-11lu min: %-11lu max: %-11lu\n", cm->avg,
         cm->min, cm->max);

  struct resource_value *rd_bw = &mmStats_.computeResources.ddr_read_bw;
  struct resource_value *wr_bw = &mmStats_.computeResources.ddr_write_bw;
  printf("\tDDR BW     Read  avg: %-11lu min: %-11lu max: %-11lu\n", rd_bw->avg,
         rd_bw->min, rd_bw->max);
  printf("\t           Write avg: %-11lu min: %-11lu max: %-11lu\n", wr_bw->avg,
         wr_bw->min, wr_bw->max);

  rd_bw = &mmStats_.computeResources.l2_l3_read_bw;
  wr_bw = &mmStats_.computeResources.l2_l3_write_bw;
  printf("\tL2/L3 BW   Read  avg: %-11lu min: %-11lu max: %-11lu\n", rd_bw->avg,
         rd_bw->min, rd_bw->max);
  printf("\t           Write avg: %-11lu min: %-11lu max: %-11lu\n", wr_bw->avg,
         wr_bw->min, wr_bw->max);

  rd_bw = &mmStats_.computeResources.pcie_dma_read_bw;
  wr_bw = &mmStats_.computeResources.pcie_dma_write_bw;
  printf("\tPCI DMA BW Read  avg: %-11lu min: %-11lu max: %-11lu\n", rd_bw->avg,
         rd_bw->min, rd_bw->max);
  printf("\t           Write avg: %-11lu min: %-11lu max: %-11lu\n", wr_bw->avg,
         wr_bw->min, wr_bw->max);

  return;
}

int main(int argc, char **argv) {
  int devNum = 0;
  bool usageError = false;
  const int32_t kMaxDeviceNum = 63;

  /*
   * Validate the target device
   */
  if (argc != 2) {
    usageError = true;
  } else {
    devNum = atoi(argv[1]);
    if (devNum < 0 || devNum > kMaxDeviceNum) {
      usageError = true;
    }
  }

  if (usageError) {
    std::cerr << "Usage: " << argv[0] << " DEVICE (where DEVICE is 0-"
              << kMaxDeviceNum << ")\n";
    exit(1);
  }

  char devName[16];
  int rc = snprintf(devName, sizeof(devName), "/dev/et%d_mgmt", devNum);
  if (rc < 0 || rc >= (int)sizeof(devName)) {
    std::cerr << "Fatal snprintf error: " << rc << std::endl;
    exit(1);
  }

  struct stat buf;
  if (stat(devName, &buf) != 0) {
    std::cerr << "stat call failed on " << devName << " with errno: " << errno
              << std::endl;
    exit(1);
  }

  /*
   * Set up access to the target device stats
   */
  device_management::getDM_t dmi;
  void *handle = dlopen("libDM.so", RTLD_LAZY);
  if (!handle) {
    std::cerr << "dlopen fail error: " << dlerror();
    exit(1);
  } else {
    const char *error;

    dmi = reinterpret_cast<device_management::getDM_t>(
        dlsym(handle, "getInstance"));
    if ((error = dlerror())) {
      std::cerr << "dlsym fail error: " << error << std::endl;
      exit(1);
    }
  }

  std::unique_ptr<dev::IDeviceLayer> dl =
      dev::IDeviceLayer::createPcieDeviceLayer(false, true);
  system("clear");  // XXX eliminate the need for this
  device_management::DeviceManagement &dm = (*dmi)(dl.get());

  /*
   * Enter stats processing loop
   */
  EtTop etTop(devNum, dl, dm);

  while (1) {
    etTop.collectStats();
    etTop.displayStats();
    sleep(2);
  }

  return 0;
}
