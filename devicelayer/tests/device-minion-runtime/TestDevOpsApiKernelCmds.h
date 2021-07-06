//******************************************************************************
// Copyright (C) 2020 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "testHelper/TestDevOpsApi.h"

namespace dev::dl_tests {

enum class KernelTypes {
  ADD_KERNEL_TYPE = 0,
  EXCEP_KERNEL_TYPE,
  HANG_KERNEL_TYPE,
  UBER_KERNEL_TYPE,
  EMPTY_KERNEL_TYPE,
  CMUMODE_KERNEL_TYPE,
};

struct PerDevPerQueKernels {
  int deviceIdx;
  int queueIdx;
  std::vector<KernelTypes> kernels;
};

typedef std::vector<PerDevPerQueKernels> kernelContainer_t;

struct AddKerParams_t {
  uint64_t vAAddr;
  uint64_t vBAddr;
  uint64_t resultABAddr;
  int numElemts;
};

struct UberLayerParameters_t {
  uint64_t data_ptr;
  uint64_t length;
  uint32_t shire_count;
};

struct AddKerInfo {
  int devIdx;
  int queueIdx;
  std::vector<int> data;
};

struct ExcepContextInfo {
  device_ops_api::tag_id_t tagId;
  int queueIdx;
  void *data;
};

class TestDevOpsApiKernelCmds : public TestDevOpsApi {
protected:
  /* Kernel Functional Tests */
  void launchAddVectorKernel_PositiveTesting_4_1(uint64_t shire_mask, std::string kernelName = "add_vector.elf");
  void launchUberKernel_PositiveTesting_4_4(uint64_t shire_mask);
  void launchEmptyKernel_PositiveTesting_4_5(uint64_t shire_mask);
  void launchExceptionKernel_NegativeTesting_4_6(uint64_t shire_mask);
  void launchHangKernel(uint64_t shire_mask, bool sendAbortCmd);

  /* Kernel Stress Tests*/
  void backToBackSameKernelLaunchCmds_3_1(bool singleDevice, bool singleQueue, uint64_t totalKer, uint64_t shire_mask);
  void backToBackDifferentKernelLaunchCmds_3_2(bool singleDevice, bool singleQueue, uint64_t totalKer,
                                               uint64_t shire_mask);
  void varifyAddKernelLaunchKernel(bool singleDevice, bool singleQueue,
      const std::vector<std::vector<int>> sumOfVectorAB,
      const std::vector<std::vector<int>> resultOfVectorAB,
      std::unordered_map<size_t, int> kerCountStorage);
  void backToBackEmptyKernelLaunch_3_3(uint64_t totalKer, uint64_t shire_mask, bool flushL3);
  /* Kernel Negative Tests */
  void kernelAbortCmd_InvalidTagIdNegativeTesting_6_2();


  /**********************************************************
   *                                                         *
   *          Kernel DMA LIST Functions                      *
   *                                                         *
   **********************************************************/

  /* Kernel Functional Tests */
  void launchAddVectorKernelListCmd(uint64_t shire_mask, KernelTypes kernelType = KernelTypes::ADD_KERNEL_TYPE);
  void launchUberKernelListCmd(uint64_t shire_mask);
  void launchEmptyKernelListCmd(uint64_t shire_mask);
  void launchExceptionKernelListCmd(uint64_t shire_mask);
  void launchHangKernelListCmd(uint64_t shire_mask, bool sendAbortCmd);

  /* Kernel Stress Tests*/
  void backToBackSameKernelLaunchListCmds(bool singleDevice, bool singleQueue, uint64_t totalKer, uint64_t shire_mask);
  void backToBackDifferentKernelLaunchListCmds(bool singleDevice, bool singleQueue, uint64_t totalKer, uint64_t shire_mask);
  void backToBackEmptyKernelLaunchListCmds(uint64_t totalKer, uint64_t shire_mask, bool flushL3);

  /* Kernel Negative Tests*/
  void kernelLaunchCmd_NegativeTesting(bool invalidAddr, int size, uint64_t shireMask, device_ops_api::dev_ops_api_kernel_launch_response_e status);
  void kernelLaunchCmd_InvalidAddressNegativeTesting(uint64_t shireMask);
  void kernelLaunchCmd_InvalidArgSizeNegativeTesting(uint64_t shireMask);
  void kernelLaunchCmd_InvalidShireMaskNegativeTesting(uint64_t shireMask);

private:
  void launchKernelDMAListCmds(uint64_t shireMask, std::vector<KernelTypes> totalKer, bool singleDevice = false, bool singleQueue = false);
  kernelContainer_t buildKernelsInfo(std::vector<KernelTypes> totalKer, bool singleDevice, bool singleQueue, bool calculateDmaListSize=true);
  void launchKernelDMAListPerQueue(uint64_t shireMask, std::vector<KernelTypes> totalKer, bool singleDevice = false, bool singleQueue = false);
  uint64_t loadElf(int deviceIdx, KernelTypes kerType, device_ops_api::dma_write_node &node);
  device_ops_api::dma_write_node fillDMAWriteNode(uint64_t srcHostVirtAddr, uint64_t dstDevPhyAddr, uint32_t size) const;
  device_ops_api::dma_read_node fillDMAReadNode(uint64_t dstHostVirtAddr, uint64_t srcDevPhyAddr, uint32_t size) const;
  uint64_t kernelExceptionSpace(int deviceIdx);
  std::vector<KernelTypes> generateKernelTypes(KernelTypes kernelType, uint64_t totalKernel = 1, int numKernelTypes = 1) const;

  // DMA management
  void calculateDMABuffer(const kernelContainer_t, int totalDevices);
  uint8_t* getMmapBuffer(int devicesIdx, size_t bytes, bool write=true);
  void cleanDMABuffer();

  // Add kernel functions
  uint64_t handleAddKernelDMAListCmd(int, uint64_t, std::vector<int>&, std::vector<CmdTag>&);
  void addKernelResultReadBackPerQueue(int, std::vector<int*>&, std::vector<uint64_t>, std::vector<CmdTag>&);
  void validataAddKernel(std::vector<AddKerInfo> addKerneksumAB, std::vector<int*> addKernelResultAB) const;

  // Exception and hang kernel functions
  void handleExceptionOrAbortKernelDMAListCmd(int, int, uint64_t, KernelTypes, std::vector<ExcepContextInfo>&, std::vector<CmdTag>&);
  void printExceptionContext(const std::vector<ExcepContextInfo>) const;

  // Empty kernel functions
  uint64_t handleEmptyKernelDMAListCmd(int deviceIdx, uint64_t shireMask, std::vector<CmdTag> &stream);
  void emptyKernelReadBackResults(int, std::vector<uint64_t>, std::vector<CmdTag> &stream);

  // Uber kernel functions
  void handleUberKernelDMAListCmd(int deviceIdx, uint64_t shireMask, std::vector<uint64_t*> &layersResult, std::vector<CmdTag> &stream);
  void validateUberKernelResult(std::vector<uint64_t*> layersResults) const;

  int addKerNumElems_ = 4 * 1024;
  bool isReadExceptionContext_ = true;
  uint64_t exceptionShireMask_ = 0x3;
  bool flushL3_ = false;
  bool sendAbortCmd_ = true;
  KernelTypes addKernelType_ = KernelTypes::ADD_KERNEL_TYPE;
  std::map<int, uint8_t*> allocatedWriteDMABuffers_;
  std::map<int, uint8_t*> allocatedReadDMABuffers_;
  std::vector<void*> dmaBufferRefernces_;
  static std::map<KernelTypes, std::string> kernelELFNames_;

};

} // namespace dev::dl_tests
