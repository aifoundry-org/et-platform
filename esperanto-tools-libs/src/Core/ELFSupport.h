//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_ELFSUPPROT_H
#define ET_RUNTIME_ELFSUPPROT_H

#include <cstdint>
#include <elfio/elfio.hpp>
#include <istream>
#include <string>
#include <unordered_map>
#include <vector>

namespace et_runtime {

/// @Brief Base class holding the information of an ELF file
class ELFInfo {
public:
  ELFInfo(const std::string &name);
  virtual ~ELFInfo() = default;

  virtual bool loadELF(const std::string &path);
  virtual bool loadELF(std::istream &stream);
  virtual bool loadELF(std::vector<char> &data);

  const std::string &name() const { return name_; }
  const std::vector<char> &data() const { return data_; }
  size_t elfSize() { return elf_size_; }

  /// @Brief return the address where we are expected to load the ELF
  ///
  /// Currently we are expecting to have a signle segment per ELF file, return
  /// the physical address of that segment
  size_t loadAddr();

protected:
  std::vector<char> data_; ///< ELF raw data
  std::string path_;       ///< Path to the ELF
  size_t elf_size_ =
      0; ///< Size of the ELF as computed by the different sections
  const std::string
      name_;            ///< Kernel device name, exists for any valid hostFun.
  ELFIO::elfio reader_; ///< elfio object that holds the parsed ELF information

private:
  bool checkELFSegments();
};

/// @brief Clas that hold the information of the Kernel ELF file
class KernelELFInfo final : public ELFInfo {
public:
  KernelELFInfo(const std::string &name);

  virtual bool loadELF(const std::string &path) override;
  virtual bool loadELF(std::istream &stream) override;
  virtual bool loadELF(std::vector<char> &data) override;

  bool rawKernelExists(const std::string &name);
  size_t rawKernelOffset(const std::string &name);

private:
  using KernelOffsetMap = std::unordered_map<std::string, size_t>;

  KernelOffsetMap kernel_offset_;     ///< Offset of kernel entrypoints
  KernelOffsetMap raw_kernel_offset_; ///< Offset of Raw Kernel entrypoints
};

} // namespace et_runtime

#endif // ET_RUNTIME_ELFSUPPROT_H
