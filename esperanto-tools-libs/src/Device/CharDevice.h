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

#include <experimental/filesystem>
#include <fstream>

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

  bool write(uintptr_t addr, const void *data, ssize_t size);
  bool read(uintptr_t addr, void *data, ssize_t size);

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
