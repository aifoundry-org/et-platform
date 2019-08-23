//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_CHAR_DEVICE_H
#define ET_RUNTIME_DEVICE_CHAR_DEVICE_H

#include "Support/Logging.h"
#include "Device/LinuxETIOCTL.h"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <experimental/filesystem>
#include <fstream>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <tuple>

namespace et_runtime {
namespace device {

///
/// @brief Helper class with RIAA scemantics for handling one of the character
///  devices that the driver opens
///
///
class CharacterDevice {
public:
  /// @brief Constuct a guard passing the path to the character device
  CharacterDevice(const std::experimental::filesystem::path &char_dev,
                  uintptr_t base_addr = 0);
  /// @brief  Disable copy constructor for the guard, object not copyable
  CharacterDevice(const CharacterDevice &rhs) = delete;
  /// @brief Move constructor
  CharacterDevice(CharacterDevice &&other);

  virtual ~CharacterDevice();

  /// @brief Write data[size] to address at the device
  bool write(uintptr_t addr, const void *data, ssize_t size);

  /// @brief Read data[size] at address from the device
  bool read(uintptr_t addr, void *data, ssize_t size);

  /// @brief Execute an IOCTL command on the character device that does not return a result
  ///
  /// @param[in] request The IOCTL command number
  /// @param[in] argp ARG_TYPE data to pass to the device command (usually a pointer)
  ///
  /// @return Tuple of bool and int. If the ioctl was succesfull the tuple
  /// value is <true, return_value>, where return_value holds the return value
  /// of the ioctl. Otherwise the first element of the tuple is false.
  template <typename ARG_TYPE>
  bool ioctl_set(unsigned int request, const ARG_TYPE argp) {
    auto rc = :4:ioctl(fd_, request, argp);
    if (rc < 0) {
      auto error = errno;
      RTERROR << "Failed to execute IOCTL: " << std::strerror(error) << "\n";
      return false;
    }
    assert(rc == 0);
    return true;
  }

  /// @brief Execute an IOCTL command on the character device
  ///
  /// @param[in] request The IOCTL command number
  /// @param[in] argp ARG_TYPE pointer data to pass to the device command (usually a pointer)
  ///
  /// @return Tuple of bool and ARG_TYPE. If the ioctl was succesfull the tuple
  /// value is <true, return_value>, where return_value holds the return value
  /// of the ioctl. Otherwise the first element of the tuple is false.
  template <typename ARG_TYPE>
  std::tuple<bool, ARG_TYPE> ioctl(unsigned int request, const ARG_TYPE* argp) {
    auto rc = ::ioctl(fd_, request, argp);
    if (rc < 0) {
      auto error = errno;
      RTERROR << "Failed to execute IOCTL: " << std::strerror(error) << "\n";
      return {false, 0};
    }
    assert(rc == 0);
    return {true, *argp};
  }

protected:
  /// Using a fstream for now, we need to move to another implementation in the
  /// future
  std::experimental::filesystem::path path_; ///< Path fo the device
  int fd_ = -1;                              ///< File descriptor to the device
  uintptr_t
      base_addr_; ///< Base address of the device to be used to translate
                  ///< absolute adresses FIXME: We should pull this information
                  ///< form an IOCTL to the character device along with the size
};

} // namespace device
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_CHAR_DEV_GUARD_H
