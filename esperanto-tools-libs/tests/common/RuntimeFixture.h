//******************************************************************************
// Copyright (C) 2022, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------
#pragma once
#include "MpOrchestrator.h"
#include "TestUtils.h"
#include "server/Client.h"
#include <device-layer/IDeviceLayer.h>

class RuntimeFixture : public testing::Test {
public:
  enum class DeviceLayerImp { PCIE, SYSEMU, FAKE };
  enum class RtType { SP, MP };

  void SetUp() override {
    auto options = rt::getDefaultOptions();
    auto dlCreator = [] {
      switch (sDlType) {
      case DeviceLayerImp::PCIE:
        RT_LOG(INFO) << "Running tests with PCIE deviceLayer";
        return dev::IDeviceLayer::createPcieDeviceLayer();
      case DeviceLayerImp::SYSEMU: {
        RT_LOG(INFO) << "Running tests with SYSEMU deviceLayer";
        auto opts = getSysemuDefaultOptions();
        std::vector<decltype(opts)> vopts;
        for (auto i = 0; i < sNumDevices; ++i) {
          vopts.emplace_back(opts);
          vopts.back().logFile += std::to_string(i);
        }
        return dev::IDeviceLayer::createSysEmuDeviceLayer(vopts);
      }
      case DeviceLayerImp::FAKE:
        RT_LOG(INFO) << "Running tests with FAKE deviceLayer";
        return std::unique_ptr<dev::IDeviceLayer>{std::make_unique<dev::DeviceLayerFake>()};
      }
      throw dev::Exception("Invalid devicelayer type");
    };
    if (sDlType == DeviceLayerImp::FAKE) {
      options.checkDeviceApiVersion_ = false;
    }
    if (sRtType == RtType::SP) {
      deviceLayer_ = dlCreator();
      runtime_ = rt::IRuntime::create(deviceLayer_.get(), options);
      auto imp = static_cast<rt::RuntimeImp*>(runtime_.get());
      for (auto i = 0U; i < static_cast<uint32_t>(deviceLayer_->getDevicesCount()); ++i) {
        imp->setMemoryManagerDebugMode(devices_[i], true);
      }
      loggerDefault_ = std::make_unique<logging::LoggerDefault>();
    } else {
      mpOrchestrator_ = std::make_unique<MpOrchestrator>();
      mpOrchestrator_->createServer(dlCreator, options);
      runtime_ = std::make_unique<rt::Client>(mpOrchestrator_->getSocketPath());
    }
    SetupTrace();
    devices_ = runtime_->getDevices();

    for (auto i = 0U; i < static_cast<uint32_t>(deviceLayer_->getDevicesCount()); ++i) {
      defaultStreams_.emplace_back(runtime_->createStream(devices_[i]));
    }

    runtime_->setOnStreamErrorsCallback([](auto, const auto&) { FAIL(); });
  }

  void TearDown() override {
    for (auto s : defaultStreams_) {
      runtime_->waitForStream(s);
      runtime_->destroyStream(s);
    }
    mpOrchestrator_.reset();
    runtime_.reset();
    traceOut_.close();
    defaultStreams_.clear();
    devices_.clear();
    deviceLayer_.reset();
  }

  rt::KernelId loadKernel(const std::string& kernel_name, uint32_t deviceIdx = 0) {
    auto kernelContent = readFile(std::string{KERNELS_DIR} + "/" + kernel_name);
    EXPECT_FALSE(kernelContent.empty());
    EXPECT_TRUE(devices_.size() > deviceIdx);
    auto st = defaultStreams_[deviceIdx];
    auto res = runtime_->loadCode(st, kernelContent.data(), kernelContent.size());
    runtime_->waitForEvent(res.event_);
    return res.kernel_;
  }

  static bool IsTraceEnabled(int argc, char* argv[]) {
    for (auto i = 1; i < argc; ++i) {
      if (std::string{argv[i]} == "--trace") {
        return true;
      }
    }
    return false;
  }

  static auto getRtType(int argc, char* argv[]) {
    for (auto i = 1; i < argc; ++i) {
      if (std::string{argv[i]} == "--mp") {
        return RtType::MP;
      }
    }
    return RtType::SP;
  }

  static auto getDeviceLayerImp(int argc, char* argv[]) {
    for (auto i = 1; i < argc; ++i) {
      if (std::string{argv[i]} == "--mode=pcie") {
        return DeviceLayerImp::PCIE;
      }
      if (std::string{argv[i]} == "--mode=fake") {
        return DeviceLayerImp::FAKE;
      }
      if (std::string{argv[i]} == "--mode=sysemu") {
        return DeviceLayerImp::SYSEMU;
      }
    }
    // defaults to sysemu
    return DeviceLayerImp::SYSEMU;
  }

  static void ParseArguments(int argc, char* argv[]) {
    sDlType = getDeviceLayerImp(argc, argv);
    sTraceEnabled = IsTraceEnabled(argc, argv);
    sRtType = getRtType(argc, argv);
  }

  inline static DeviceLayerImp sDlType = DeviceLayerImp::SYSEMU;
  inline static RtType sRtType = RtType::SP;
  inline static uint8_t sNumDevices = 1;
  inline static bool sTraceEnabled = false;

private:
  void SetupTrace() {
    if (sTraceEnabled) {
      auto traceFile = ::testing::UnitTest::GetInstance()->current_test_info()->name() + std::string{".trace.json"};
      auto profiler = runtime_->getProfiler();
      RT_LOG(INFO) << "Traces enables for this test. Storing trace at file: " << traceFile;
      traceOut_ = std::ofstream(traceFile);
      ASSERT_TRUE(traceOut_.is_open());
      profiler->start(traceOut_, rt::IProfiler::OutputType::Json);
    }
  }

protected:
  std::ofstream traceOut_;
  std::unique_ptr<logging::LoggerDefault>
    loggerDefault_; // will be set always for SP mode, must be set manually for MP mode (if needed)
  std::unique_ptr<dev::IDeviceLayer> deviceLayer_; // only set for SP mode
  std::unique_ptr<MpOrchestrator> mpOrchestrator_; // only set for MP mode
  rt::RuntimePtr runtime_;
  std::vector<rt::DeviceId> devices_;
  std::vector<rt::StreamId> defaultStreams_;
};
