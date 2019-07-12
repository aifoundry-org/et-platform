//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_TARGET_H
#define ET_RUNTIME_DEVICE_TARGET_H

#include "Core/DeviceInformation.h"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace et_runtime {
namespace device {

// Types
typedef struct {
    uint64_t tensor_a;     // Pointer to tensor A
    uint64_t tensor_b;     // Pointer to tensor B
    uint64_t tensor_c;     // Pointer to tensor C
    uint64_t tensor_d;     // Pointer to tensor D
    uint64_t tensor_e;     // Pointer to tensor E
    uint64_t tensor_f;     // Pointer to tensor F
    uint64_t tensor_g;     // Pointer to tensor G
    uint64_t tensor_h;     // Pointer to tensor H
    uint64_t kernel_id;    // Id for this Kernel
} layer_dynamic_info;

/// @brief Struct holding the information for configuring a memory region in the
/// target device
struct MemoryRegionConf {
  uintptr_t start_addr = 0;
  size_t size = 0;
  bool is_exec = false;
};

/// @brief Abtract class describing the interface to talk to different targets
///
/// The runtime can talk to a number of underlying device targers beyond the
/// PCIe device driver that will be in production. This class provides a generic
/// abstract interface to talk to the different targets.
///
/// # RIIA "Device Schemantics"
///
/// Interacting with PCIe drivers or any other simulator does not provide clean
/// RIIA semantics: e.g. closing files or connections can generate errors that
/// we need to be able to handle and recover from if necessary. As such we want
/// to avoid adding this logic inside the destructor where we cannot return an
/// error from and we should not be throwing exceptions either. As such we
/// provide init and deInit functions that will explicitly handle

class DeviceTarget {
public:
  /// @brief Type of different target devices we can have.
  enum class TargetType : uint8_t {
    None = 0,
    PCIe,
    SysEmuCardProxy,
    SysEmuGRPC,
    DeviceGRPC,
  };

  static const std::map<std::string, TargetType> Str2TargetType;

  /// @brief Callback function type for device responses
  using ResponseCallback = std::function<bool()>;
  /// @brief Callback function for device events
  using EventCallback = std::function<bool()>;

  DeviceTarget() = default;
  /// @brief DeviceTargert contructor
  ///
  /// @params[in] path  Path to the device target. This can be either
  ///    the path to the pcie kernel device or that of a local socket.
  DeviceTarget(const std::string &path);
  virtual ~DeviceTarget() = default;

  ///
  /// @brief static function that should enumerate all devices and return
  /// related information
  ///
  /// @return Vector with the DeviceInfomrmation of all devices we can detect on
  /// the system
  /// @todo Having it be a static function is not the best interface decision
  /// this should be revisited
  static std::vector<DeviceInformation> enumerateDevices() {
    abort();
    return {};
  }

  /// @brief Initialize the target device
  virtual bool init() = 0;
  /// @brief De-Initialize the target device
  virtual bool deinit() = 0;
  /// @brief Get status information from the device
  virtual bool getStatus() = 0;
  ///
  /// @brief Get static configuration information from the device
  ///
  /// Extract static configuration information from the device. The static
  /// information should be exposed at the level of the device driver and should
  /// not require exchanging dynamic commands with the device.
  virtual DeviceInformation getStaticConfiguration() = 0;
  ///
  /// @brief Submit a command to the device.
  ///
  /// This is an asynchronous call in respect with the response of the device.
  /// This call wil block only if the command queue to the device is full from
  /// the driver's perspective.
  virtual bool submitCommand() = 0;
  ///
  /// @brief Register callback for when a response arrives from the device.
  virtual bool registerResponseCallback() = 0;
  ///
  /// @brief Register callback for when an event arrives from the device
  virtual bool registerDeviceEventCallback() = 0;

  virtual bool alive() { return device_alive_; }

  /// @todo the following interface mimics the functionality of the origial card
  /// proxy this is transitional code that probably could be removed in the
  /// future.

  /// @brief Define the properties of a specific device region
  virtual bool defineDevMem(uintptr_t dev_addr, size_t size, bool is_exec) = 0;

  /// @brief Read device memory
  virtual bool readDevMem(uintptr_t dev_addr, size_t size, void *buf) = 0;

  /// @brief Write device memory
  virtual bool writeDevMem(uintptr_t dev_addr, size_t size,
                           const void *buf) = 0;

  /// @brief Launch a specific PC on the device.
  virtual bool launch(uintptr_t launch_pc, const layer_dynamic_info *params) = 0;

  /// @brief Boot the device
  virtual bool boot(uintptr_t init_pc, uintptr_t trap_pc) = 0;

  /// @brief Factory function that will generate the appropriate target device
  static std::unique_ptr<DeviceTarget> deviceFactory(TargetType target,
                                                     const std::string &path);
  /// @brief Return the type of device to allocate as specified by the command
  /// line arguments
  /// FIXME this functions should not be released in production
  static TargetType deviceToCreate();

  /// @brief Force the type of device to create
  ///
  /// @param[in] String with the device type, allowed values are: pcie,
  /// sysemu_card_proxy, sysemu_grpc, device_grpc
  /// @returns True if valid value was passed
  static bool setDeviceType(const std::string &device_type);

protected:
  std::string path_; ///< Path of the device to initialize
  bool device_alive_ =
      false; ///< Flag to guard that the device has been initlize correctly and
             ///< gate any incorrect re-initialization or de-initialization
};
} // namespace device
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_TARGET_H
