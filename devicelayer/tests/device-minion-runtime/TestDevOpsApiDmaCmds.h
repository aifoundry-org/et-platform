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

  /* DMA List Positive Testing Functions */
  void dataRWListCmd_PositiveTesting_3_11();

  /* DMA List Negative Testing Functions */
  void dataRWListCmd_NegativeTesting_3_12();

  /* DMA List Stress Testing Functions */
  void dmaListWrAndRd(bool singleDevice, bool singleQueue, uint32_t numOfDmaEntries);

private:
  enum class DmaType { DMA_WRITE, DMA_READ };

  bool fillDmaStream(int deviceIdx, std::vector<std::unique_ptr<IDevOpsApiCmd>>& stream,
                     const std::vector<DmaType>& cmdSequence, const std::vector<size_t>& sizeSequence,
                     std::vector<std::vector<uint8_t>>& dmaWrBufs, std::vector<std::vector<uint8_t>>& dmaRdBufs);
  bool executeDmaCmds(bool singleDevice, bool singleQueue, bool isAsync, const std::vector<DmaType>& cmdSequence,
                      const std::vector<size_t>& sizeSequence, std::vector<std::vector<uint8_t>>& dmaWrBufs,
                      std::vector<std::vector<uint8_t>>& dmaRdBufs);
};
