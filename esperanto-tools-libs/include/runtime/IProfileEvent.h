

/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/
#pragma once

#include "runtime/IRuntime.h"
#include <runtime/IRuntimeExport.h>

#include <cereal/cereal.hpp>
#include <cereal/types/chrono.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/variant.hpp>

#include <chrono>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>

/// \defgroup runtime_profiler_api Runtime Profiler API
///
/// The runtime profiler API allows to gather profiling data from runtime execution.
/// This API expects to receive an output stream where all profiling events will be dumped, the user can explicitely
/// indicate when to start pofiling and when to stop profiling.
/// @{
namespace rt {
class RuntimeImp;
class CommandSender;

namespace tests {
class ProfileEventDeserializationTest;
}

namespace profiling {

enum class Version : uint16_t {};

constexpr auto kCurrentVersion = Version{3};

enum class Type { Start, End, Complete, Instant, Counter };
enum class Class {
  GetDevices,
  LoadCode,
  UnloadCode,
  MallocDevice,
  FreeDevice,
  CreateStream,
  DestroyStream,
  KernelLaunch,
  MemcpyHostToDevice,
  MemcpyDeviceToHost,
  WaitForEvent,
  WaitForStream,
  DeviceCommand,
  Pmc,
  CommandSent,
  ResponseReceived,
  DispatchEvent,
  GetDeviceProperties,
  StartProfiling,
  EndProfiling,
  CmaCopy,
  CmaWait,
  MemcpyDeviceToDevice,
  SyncTime,
  IdentifyThread,
  MemoryStats,
  COUNT
};

enum class ResponseType { DMARead, DMAWrite, Kernel, DMAP2P, COUNT };

Class class_from_string(const std::string& str);
Type type_from_string(const std::string& str);
ResponseType response_type_from_string(const std::string& str);

class ScopedProfileEvent;

/// \brief ProfileEvent exposes serialization internals for runtime users.
///
class ETRT_API ProfileEvent {
public:
  using Id = uint64_t;
  using Cycles = uint64_t;
  using Clock = std::chrono::steady_clock;
  using TimePoint = std::chrono::time_point<Clock>;
  using SystemClock = std::chrono::system_clock;
  using SystemTimePoint = std::chrono::time_point<SystemClock>;
  using Duration = TimePoint::duration;
  using ExtraValues = std::variant<uint64_t, EventId, StreamId, DeviceId, KernelId, ResponseType, Duration,
                                   DeviceProperties, Version, SystemTimePoint, std::string, bool, std::uint32_t>;
  using ExtraMetadata = std::unordered_map<std::string, ExtraValues>;

  ProfileEvent() = default;
  explicit ProfileEvent(Type type, Class cls);
  explicit ProfileEvent(Type type, Class cls, StreamId stream, EventId event, ExtraMetadata extra = {});

  static constexpr char const* kType = "type";
  static constexpr char const* kClass = "class";
  static constexpr char const* kTimeStamp = "timeStamp";
  static constexpr char const* kThreadId = "thread_id";
  static constexpr char const* kExtraMetadata = "extra";

  // returns the trace protocol version
  Type getType() const;
  Class getClass() const;
  TimePoint getTimeStamp() const;
  std::string getThreadId() const;
  ExtraMetadata getExtras() const;

  std::thread::id getNumericThreadId() const;

  static constexpr std::string_view kVersion = "version";
  static constexpr std::string_view kDuration = "duration";
  static constexpr std::string_view kEventId = "event";
  static constexpr std::string_view kParentId = "parent_id";
  static constexpr std::string_view kStreamId = "stream";
  static constexpr std::string_view kDeviceId = "device_id";
  static constexpr std::string_view kKernelId = "kernel_id";
  static constexpr std::string_view kResponseType = "rsp_type";
  static constexpr std::string_view kLoadAddr = "load_address";
  static constexpr std::string_view kDeviceCmdStartTs = "device_cmd_start_ts";
  static constexpr std::string_view kDeviceCmdWaitDur = "device_cmd_wait_dur";
  static constexpr std::string_view kDeviceCmdExecDur = "device_cmd_exec_dur";
  static constexpr std::string_view kDeviceProps = "device_properties";
  static constexpr std::string_view kTimePointSystem = "system_timepoint";
  static constexpr std::string_view kThreadName = "thread_name";
  static constexpr std::string_view kServerPid = "server_pid";
  static constexpr std::string_view kBarrier = "barrier";
  static constexpr std::string_view kAddress = "ptr";
  static constexpr std::string_view kAddressSrc = "src_ptr";
  static constexpr std::string_view kAddressDst = "dst_ptr";
  static constexpr std::string_view kSize = "size";
  static constexpr std::string_view kAlignment = "alignment";
  static constexpr std::string_view kMemoryStatsAllocatedMem = "mem.allocated_memory";
  static constexpr std::string_view kMemoryStatsFreeMem = "mem.free_memory";
  static constexpr std::string_view kMemoryStatsMaxContiguousFreeMem = "mem.max_contiguous_free_mem";

  std::optional<Version> getVersion() const;
  std::optional<Duration> getDuration() const;
  std::optional<EventId> getEvent() const;
  std::optional<EventId> getParentId() const;
  std::optional<StreamId> getStream() const;
  std::optional<DeviceId> getDeviceId() const;
  std::optional<KernelId> getKernelId() const;
  std::optional<ResponseType> getResponseType() const;
  std::optional<uint64_t> getLoadAddress() const;
  std::optional<Cycles> getDeviceCmdStartTs() const;
  std::optional<Cycles> getDeviceCmdWaitDur() const;
  std::optional<Cycles> getDeviceCmdExecDur() const;
  std::optional<DeviceProperties> getDeviceProperties() const;
  std::optional<SystemTimePoint> getSystemTimeStamp() const;
  std::optional<std::string> getThreadName() const;
  std::optional<uint64_t> getServerPID() const;
  std::optional<bool> getBarrier() const;
  std::optional<uint64_t> getAddress() const;
  std::optional<uint64_t> getAddressSrc() const;
  std::optional<uint64_t> getAddressDst() const;
  std::optional<uint64_t> getSize() const;
  std::optional<uint32_t> getAlignment() const;
  std::optional<uint64_t> getAllocatedMemory() const;
  std::optional<uint64_t> getFreeMemory() const;
  std::optional<uint64_t> getMaxContiguousFreeMemory() const;

  void setType(Type t);
  void setClass(Class c);
  void setTimeStamp(TimePoint t = Clock::now());
  void setThreadId(std::thread::id id = std::this_thread::get_id());
  void setExtras(ExtraMetadata extras);

  void setDuration(Duration d);
  void setEvent(EventId event);
  void setParentId(EventId parent);
  void setStream(StreamId stream);
  void setDeviceId(DeviceId deviceId);
  void setKernelId(KernelId kernelId);
  void setResponseType(ResponseType rspType);
  void setLoadAddress(uint64_t loadAddress);
  void setDeviceCmdStartTs(uint64_t start_ts);
  void setDeviceCmdWaitDur(uint64_t wait_dur);
  void setDeviceCmdExecDur(uint64_t exec_dur);
  void setDeviceProperties(DeviceProperties props);
  void setSystemTimeStamp(SystemTimePoint systemTimeStamp = SystemClock::now());
  void setThreadName(std::string const& threadName);
  void setServerPID(uint64_t serverPID);
  void setBarrier(bool barrier);
  void setAddress(uint64_t ptr);
  void setAddressSrc(uint64_t ptr);
  void setAddressDst(uint64_t ptr);
  void setSize(uint64_t size);
  void setAlignment(uint32_t alignment);
  void setAllocatedMemory(uint64_t size);
  void setFreeMemory(uint64_t size);
  void setMaxContiguousFreeMemory(uint64_t size);

  template <class Archive> friend void load(Archive& ar, ProfileEvent& evt);

private:
  template <typename... Args> void addExtra(std::string_view name, Args&&... args);
  template <typename T> std::optional<T> getExtra(const std::string& name) const;
  template <typename T> std::optional<T> getExtra(std::string_view name) const;

  ExtraMetadata extra_;
  std::string threadId_;
  TimePoint timeStamp_;
  Type type_;
  Class class_;

  std::thread::id numericThreadId_;
};
} // end namespace profiling
} // end namespace rt

namespace rt::profiling {
std::string getString(rt::profiling::Class cls);
std::string getString(rt::profiling::Type type);
std::string getString(rt::profiling::ResponseType rspType);

template <typename Archive> void serialize(Archive& ar, rt::EventId& id) {
  ar(cereal::make_nvp("event_id", id));
}

template <typename Archive> void serialize(Archive& ar, rt::StreamId& id) {
  ar(cereal::make_nvp("stream_id", id));
}
template <typename Archive> void serialize(Archive& ar, rt::DeviceId& id) {
  ar(cereal::make_nvp("device_id", id));
}
template <typename Archive> void serialize(Archive& ar, rt::KernelId& id) {
  ar(cereal::make_nvp("kernel_id", id));
}

template <class Archive> void load(Archive& ar, ProfileEvent& evt) {
  std::string type;
  ar(cereal::make_nvp(ProfileEvent::kType, type));
  evt.setType(type_from_string(type));

  std::string cls;
  ar(cereal::make_nvp(ProfileEvent::kClass, cls));
  evt.setClass(class_from_string(cls));

  ProfileEvent::TimePoint timeStamp;
  ar(cereal::make_nvp(ProfileEvent::kTimeStamp, timeStamp));
  evt.setTimeStamp(timeStamp);

  std::string threadId;
  ar(cereal::make_nvp(ProfileEvent::kThreadId, threadId));
  auto tid = std::thread::id(std::stoull(threadId));
  evt.setThreadId(tid);

  ProfileEvent::ExtraMetadata extra;
  ar(cereal::make_nvp(ProfileEvent::kExtraMetadata, extra));
  evt.setExtras(std::move(extra));
}

template <class Archive> void save(Archive& ar, const ProfileEvent& evt) {
  ar(cereal::make_nvp(ProfileEvent::kType, getString(evt.getType())));
  ar(cereal::make_nvp(ProfileEvent::kClass, getString(evt.getClass())));
  ar(cereal::make_nvp(ProfileEvent::kTimeStamp, evt.getTimeStamp()));
  ar(cereal::make_nvp(ProfileEvent::kThreadId, evt.getThreadId()));
  ar(cereal::make_nvp(ProfileEvent::kExtraMetadata, evt.getExtras()));
}

} // end namespace rt::profiling

namespace cereal {

template <typename Archive> void serialize(Archive& ar, rt::DeviceProperties& props) {
  ar(cereal::make_nvp("frequency", props.frequency_));
  ar(cereal::make_nvp("available_shires", props.availableShires_));
  ar(cereal::make_nvp("memory_bw", props.memoryBandwidth_));
  ar(cereal::make_nvp("memory_size", props.memorySize_));
  ar(cereal::make_nvp("l3_size", props.l3Size_));
  ar(cereal::make_nvp("l2_shire_size", props.l2shireSize_));
  ar(cereal::make_nvp("l2_scp_size", props.l2scratchpadSize_));
  ar(cereal::make_nvp("cache_line_size", props.cacheLineSize_));
  ar(cereal::make_nvp("l2_cache_banks", props.l2CacheBanks_));
  ar(cereal::make_nvp("compute_minion_shire_mask", props.computeMinionShireMask_));
  ar(cereal::make_nvp("spare_compute_minion_shire_id", props.spareComputeMinionShireId_));
  ar(cereal::make_nvp("device_arch_rev", props.deviceArch_));
  ar(cereal::make_nvp("form_factor", props.formFactor_));
  ar(cereal::make_nvp("tdp", props.tdp_));

  try {
    ar(cereal::make_nvp("local_scp_format0_base_address", props.localScpFormat0BaseAddress_));
    ar(cereal::make_nvp("local_scp_format1_base_address", props.localScpFormat1BaseAddress_));
    ar(cereal::make_nvp("local_dram_base_address", props.localDRAMBaseAddress_));
    ar(cereal::make_nvp("onpkg_scp_format2_base_address", props.onPkgScpFormat2BaseAddress_));
    ar(cereal::make_nvp("onpkg_dram_base_address", props.onPkgDRAMBaseAddress_));
    ar(cereal::make_nvp("onpkg_dram_interleaved_base_address", props.onPkgDRAMInterleavedBaseAddress_));

    ar(cereal::make_nvp("local_dram_size", props.localDRAMSize_));
    ar(cereal::make_nvp("minimum_address_alignment_bits", props.minimumAddressAlignmentBits_));
    ar(cereal::make_nvp("num_chiplets", props.numChiplets_));

    ar(cereal::make_nvp("local_scp_format0_shire_least_significant_bit", props.localScpFormat0ShireLSb_));
    ar(cereal::make_nvp("local_scp_format0_shire_bits", props.localScpFormat0ShireBits_));
    ar(cereal::make_nvp("local_scp_format0_local_shire", props.localScpFormat0LocalShire_));

    ar(cereal::make_nvp("local_scp_format1_shire_least_significant_bit", props.localScpFormat1ShireLSb_));
    ar(cereal::make_nvp("local_scp_format1_shire_bits", props.localScpFormat1ShireBits_));

    ar(cereal::make_nvp("local_scp_format2_shire_least_significant_bit", props.onPkgScpFormat2ShireLSb_));
    ar(cereal::make_nvp("local_scp_format2_shire_bits", props.onPkgScpFormat2ShireBits_));
    ar(cereal::make_nvp("local_scp_format2_chiplet_least_significant_bit", props.onPkgScpFormat2ChipletLSb_));
    ar(cereal::make_nvp("local_scp_format2_chiplet_bits", props.onPkgScpFormat2ChipletBits_));

    ar(cereal::make_nvp("onpkg_dram_chiplet_least_significant_bit", props.onPkgDRAMChipletLSb_));
    ar(cereal::make_nvp("onpkg_dram_chiplet_bits", props.onPkgDRAMChipletBits_));
    ar(cereal::make_nvp("onpkg_dram_chiplet_interleaved_least_significant_bit", props.onPkgDRAMInterleavedChipletLSb_));
    ar(cereal::make_nvp("onpkg_dram_chiplet_interleaved_bits", props.onPkgDRAMInterleavedChipletBits_));
  } catch (cereal::Exception const&) {
    // Old traces may not contain these members
  }
}

} // end namespace cereal

/// @}
// End of runtime_profile_event_api
