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

class TestDevOpsApiDmaCmds : public TestDevOpsApi {
protected:
  /* DMA Basic Testing Functions */
  void dataRWCmdMixed_3_5();
  void dataRWCmdMixedWithVarSize_3_6();
  void dataRWCmdAllChannels_3_7();

  /* DMA Positive Testing Functions */
  void dataRWCmd_PositiveTesting_3_1();
  void dataRWCmdWithBarrier_PositiveTesting_3_10();

  /* DMA Stress Testing Functions */
  void dataWRStressSize_2_1(uint8_t maxExp2);
  void dataWRStressSpeed_2_2(uint8_t maxExp2);
  void dataWRStressChannels(bool singleDevice, bool singleQueue, uint32_t numOfLoopbackCmds);

  /* DMA List Basic Testing Functions */
  void dataRWListCmdWithBasicCmds();
  void dataRWListCmdMixed();
  void dataRWListCmdMixedWithVarSize();
  void dataRWListCmdAllChannels();

  /* DMA List Positive Testing Functions */
  void dataRWListCmd_PositiveTesting_3_11();
  void dataRWListCmdWithBarrier_PositiveTesting();

  /* DMA List Negative Testing Functions */
  void dataRWListCmd_NegativeTesting_3_12();

  /* DMA List Stress Testing Functions */
  void dmaListWrAndRd(bool singleDevice, bool singleQueue, uint32_t numOfDmaEntries);
  void dataWRListStressSize(uint8_t maxExp2);
  void dataWRListStressSpeed(uint8_t maxExp2);

private:
  enum class DmaType { DMA_WRITE, DMA_READ };

  /* Fill a stream for specified device and queue. Only the first read command will be sent with barrier. */
  bool fillDmaStream(int deviceIdx, std::vector<CmdTag>& stream,
                     const std::vector<std::pair<DmaType, size_t>>& cmdSequence,
                     std::vector<std::vector<uint8_t>>& dmaWrBufs, std::vector<std::vector<uint8_t>>& dmaRdBufs);

  /* Execute same sequence of DMA commands on either multiple/single device(s) and multiple/single queue(s). */
  bool executeDmaCmds(bool singleDevice, bool singleQueue, bool isAsync,
                      const std::vector<std::pair<DmaType, size_t>>& cmdSequence,
                      std::vector<std::vector<uint8_t>>& dmaWrBufs, std::vector<std::vector<uint8_t>>& dmaRdBufs);

  /* Allocate memory using allocDmaBuffer(). The total memory for one device is allocated at once in a chunk and then
   * it is broken down while filling the commands. */
  bool allocDmaBufferSequence(bool singleDevice, bool singleQueue,
                              const std::vector<std::pair<DmaType, size_t>>& dmaMoveSequence,
                              std::unordered_map<size_t, std::vector<uint8_t*>>& dmaWrBufs,
                              std::unordered_map<size_t, std::vector<uint8_t*>>& dmaRdBufs);

  /* Validate data if validateData is set and then free up the memory using freeDmaBuffer(). */
  bool validateAndDeallocDmaBufferSequence(bool singleDevice, bool singleQueue, bool validateData,
                                           const std::vector<std::pair<DmaType, size_t>>& dmaMoveSequence,
                                           std::unordered_map<size_t, std::vector<uint8_t*>>& dmaWrBufs,
                                           std::unordered_map<size_t, std::vector<uint8_t*>>& dmaRdBufs);

  /* Fill a stream for specified device and queue. It merges consecutive DMA moves in single DMA list command. Only the
   * first DMA readlist command will be sent with barrier. */
  bool fillDmaListStream(int deviceIdx, int queueIdx, std::vector<CmdTag>& stream,
                         const std::vector<std::pair<DmaType, size_t>>& dmaMoveSequence,
                         std::unordered_map<size_t, std::vector<uint8_t*>>& dmaWrBufs,
                         std::unordered_map<size_t, std::vector<uint8_t*>>& dmaRdBufs);

  /* Execute same sequence of DMA data moves on either multiple/single device(s) and multiple/single queue(s). */
  bool executeDmaListCmds(bool singleDevice, bool singleQueue, bool isAsync,
                          const std::vector<std::pair<DmaType, size_t>>& dmaMoveSequence);
};

} // namespace dev::dl_tests
