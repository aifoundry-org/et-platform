//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "common/Constants.h"

#include "RuntimeImp.h"
#include "TestUtils.h"
#include "runtime/IProfileEvent.h"
#include "runtime/IProfiler.h"
#include "runtime/IRuntime.h"
#include <device-layer/IDeviceLayer.h>
#include <hostUtils/logging/Logging.h>

#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <gtest/gtest.h>

#include <experimental/filesystem>
#include <fstream>
#include <ios>
#include <sstream>
#include <thread>
#include <random>

namespace fs = std::experimental::filesystem;

namespace {
constexpr uint64_t kSysEmuMaxCycles = std::numeric_limits<uint64_t>::max();
constexpr uint64_t kSysEmuMinionShiresMask = 0x1FFFFFFFFu;
} // namespace

namespace rt::tests {

TEST(Profiler, add_2_vectors_profiling) {
  auto kernel_file = std::ifstream((fs::path(KERNELS_DIR) / fs::path("add_vector.elf")).string(), std::ios::binary);
  ASSERT_TRUE(kernel_file.is_open());

  auto iniF = kernel_file.tellg();
  kernel_file.seekg(0, std::ios::end);
  auto endF = kernel_file.tellg();
  auto size = endF - iniF;
  kernel_file.seekg(0, std::ios::beg);

  std::vector<std::byte> kernelContent(static_cast<unsigned long>(size));
  kernel_file.read(reinterpret_cast<char*>(kernelContent.data()), size);

  auto deviceLayer = dev::IDeviceLayer::createSysEmuDeviceLayer(getDefaultOptions());
  auto runtime = rt::IRuntime::create(deviceLayer.get());
  

  // setup the profiler
  auto profiler = runtime->getProfiler();
  std::stringstream ss;
  profiler->start(ss, rt::IProfiler::OutputType::Json);

  auto devices = runtime->getDevices();
  auto dev = devices[0];
  auto imp = static_cast<rt::RuntimeImp*>(runtime.get());
  imp->setMemoryManagerDebugMode(dev, true);

  std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution dis;

  std::vector<int> vA, vB, vResult;
  auto numElems = 10496;
  for (int i = 0; i < numElems; ++i) {
    vA.emplace_back(dis(gen));
    vB.emplace_back(dis(gen));
    vResult.emplace_back(vA.back() + vB.back());
  }
  auto bufferSize = static_cast<unsigned long>(numElems) * sizeof(int);
  auto bufA = runtime->mallocDevice(devices[0], bufferSize);
  auto bufB = runtime->mallocDevice(devices[0], bufferSize);
  auto bufResult = runtime->mallocDevice(devices[0], bufferSize);

  auto stream = runtime->createStream(dev);
  auto kernelId = runtime->loadCode(stream, kernelContent.data(), kernelContent.size()).kernel_;
  runtime->memcpyHostToDevice(stream, reinterpret_cast<std::byte*>(vA.data()), bufA, bufferSize);
  runtime->memcpyHostToDevice(stream, reinterpret_cast<std::byte*>(vB.data()), bufB, bufferSize);
  runtime->setOnStreamErrorsCallback([](auto, const auto&) { FAIL(); });
  struct Parameters {
    void* vA;
    void* vB;
    void* vResult;
    int numElements;
  } parameters{bufA, bufB, bufResult, numElems};

  std::vector<int> resultFromDevice(static_cast<unsigned long>(numElems));
  for (auto i = 0u; i < static_cast<unsigned long>(numElems); ++i) {
    resultFromDevice[i] = 0xF;
  }
  runtime->memcpyHostToDevice(stream, reinterpret_cast<std::byte*>(resultFromDevice.data()), bufResult, bufferSize);
  runtime->kernelLaunch(stream, kernelId, reinterpret_cast<std::byte*>(&parameters), sizeof(parameters), 0x1);
  for (auto i = 0u; i < static_cast<unsigned long>(numElems); ++i) {
    resultFromDevice[i] = 0x0;
  }
  runtime->memcpyDeviceToHost(stream, bufResult, reinterpret_cast<std::byte*>(resultFromDevice.data()), bufferSize);
  runtime->waitForStream(stream);
  LOG(INFO) << "After wait for stream";
  profiler->stop();
  LOG(INFO) << "After stopping profiler";
  runtime.reset();
  LOG(INFO) << "After resetting runtime";
  EXPECT_EQ(resultFromDevice, vResult);

  // take the string from the serialization and deserialize it to check everything is in place
  auto str = ss.str();
  LOG(INFO) << "Trace: " << str;
}

class ProfileEventDeserializationTest : public ::testing::Test {
protected:
  void SetUp() override {
    CreateProfileEvent();

    std::ostringstream oss_json;
    SerializeToJson(oss_json);
    trace_contents_json = oss_json.str();

    std::ostringstream oss_binary(std::ios_base::binary);
    SerializeToBinary(oss_binary);
    trace_contents_binary = oss_binary.str();
  }
private:
  void CreateProfileEvent() {
      rt::profiling::ProfileEvent evt(rt::profiling::Type::Start, rt::profiling::Class::GetDevices);
      evt.setPairId(33);
      // etc..
      reference_evt = evt;
  }
  void SerializeToJson(std::ostringstream& oss_json) {
      cereal::JSONOutputArchive archive_json(oss_json);
      archive_json(reference_evt);
  }
  void SerializeToBinary(std::ostringstream& oss_binary) {
    cereal::BinaryOutputArchive archive_binary(oss_binary);
    archive_binary(reference_evt);
  }
protected:
  rt::profiling::ProfileEvent reference_evt;

  std::string trace_contents_json;
  std::string trace_contents_binary;
};

TEST_F(ProfileEventDeserializationTest, DeserializeJson) {
  std::istringstream iss(trace_contents_json);
  cereal::JSONInputArchive archive(iss);

  rt::profiling::ProfileEvent deserialized_evt;
  archive(deserialized_evt);

  EXPECT_EQ(deserialized_evt.getType(), reference_evt.getType());
  EXPECT_EQ(deserialized_evt.getClass(), reference_evt.getClass());
  EXPECT_EQ(deserialized_evt.getTimeStamp(), reference_evt.getTimeStamp());
  EXPECT_EQ(deserialized_evt.getThreadId(), reference_evt.getThreadId());

  EXPECT_TRUE(deserialized_evt.getPairId().has_value());
  EXPECT_EQ(deserialized_evt.getPairId().value(), 33);
}

TEST_F(ProfileEventDeserializationTest, DeserializeBinary) {
  std::istringstream iss(trace_contents_binary, std::ios_base::binary);
  cereal::BinaryInputArchive archive(iss);

  rt::profiling::ProfileEvent deserialized_evt;
  archive(deserialized_evt);

  EXPECT_EQ(deserialized_evt.getType(), reference_evt.getType());
  EXPECT_EQ(deserialized_evt.getClass(), reference_evt.getClass());
  EXPECT_EQ(deserialized_evt.getTimeStamp(), reference_evt.getTimeStamp());
  EXPECT_EQ(deserialized_evt.getThreadId(), reference_evt.getThreadId());

  EXPECT_TRUE(deserialized_evt.getPairId().has_value());
  EXPECT_EQ(deserialized_evt.getPairId().value(), 33);
}

} // namespace rt::tests

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
