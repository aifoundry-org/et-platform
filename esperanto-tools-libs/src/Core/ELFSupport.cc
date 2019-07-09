//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "ELFSupport.h"

#include "Support/HelperMacros.h"
#include "Support/Logging.h"
#include "demangle.h"

#include <cstdint>
#include <elfio/elfio.hpp>
#include <elfio/elfio_dump.hpp>
#include <string>
#include <tuple>
#include <vector>

namespace et_runtime {
ELFInfo::ELFInfo(const std::string &n) : name_(n), reader_() {}

bool ELFInfo::loadELF(const std::string &path) {
  path_ = path;
  std::ifstream stream;
  stream.open(path.c_str(), std::ios::in | std::ios::binary);
  if (!stream) {
    return false;
  }
  return loadELF(stream);
}

bool ELFInfo::loadELF(std::istream &stream) {

  // Read the data in a file
  auto stream_it = std::istream_iterator<char>(stream);
  data_.insert(data_.begin(), stream_it, std::istream_iterator<char>());

  // Rewind the stream clear the EOF error before calling seekg
  stream.clear();
  stream.seekg(0, stream.beg);

  if (!reader_.load(stream)) {
    THROW("Could not load an ELF file.");
  }

  if (false) {
    ELFIO::dump::header(std::cerr, reader_);
    ELFIO::dump::section_headers(std::cerr, reader_);
    ELFIO::dump::segment_headers(std::cerr, reader_);
    ELFIO::dump::symbol_tables(std::cerr, reader_);
    ELFIO::dump::notes(std::cerr, reader_);
    ELFIO::dump::dynamic_tags(std::cerr, reader_);
    ELFIO::dump::section_datas(std::cerr, reader_);
    ELFIO::dump::segment_datas(std::cerr, reader_);
  }

  // Sanity checks.
  THROW_IF(reader_.get_machine() != EM_RISCV,
           "Kernels ELF machine is not EM_RISCV.");
  THROW_IF(reader_.get_class() != ELFCLASS64,
           "Kernels ELF class is not ELFCLASS64.");

  THROW_IF(!checkELFSegments(),
           "Currently we support elf files that have a single segment with non-zero size");

  /*
   * Compute Elf File Size by formula: e_shoff + ( e_shentsize * e_shnum )
   * This assumes that the section header table (SHT) is the last part of the
   * ELF. This is usually the case but it could also be that the last section is
   * the last part of the ELF
   */
  size_t elf_size_ = reader_.get_sections_offset() +
                     reader_.get_section_entry_size() * reader_.sections.size();
  RTINFO << "Esperanto ELF file size = " << elf_size_ << "\n";

  return true;
}

// Helper template to wrap a vector and expose an istream interface

class vectorwrapbuf : public std::streambuf {
public:
  vectorwrapbuf(std::vector<char> &vec) {
    this->setg(vec.data(), vec.data(), vec.data() + vec.size());
  }
  virtual pos_type seekoff(off_type __off, std::ios_base::seekdir __way,
                           std::ios_base::openmode) {
    if (__way == std::ios_base::beg) {
      setg(eback(), eback() + __off, egptr());
    } else if (__way == std::ios_base::cur) {
      setg(eback(), gptr() + __off, egptr());
    } else {
      setg(eback(), egptr() + __off, egptr());
    }
    return gptr() - eback();
  }

  virtual pos_type seekpos(pos_type __pos, std::ios_base::openmode __mode) {
    return seekoff(__pos, std::ios_base::beg, __mode);
  }
};

bool ELFInfo::loadELF(std::vector<char> &data) {
  data_ = data;
  vectorwrapbuf databuf(data);
  std::istream is(&databuf);
  return loadELF(is);
}

size_t ELFInfo::loadAddr() {
  return reader_.segments[0]->get_physical_address();
}

bool ELFInfo::checkELFSegments() {
  bool seg_ok = true;
  size_t num = reader_.segments.size();
  for (size_t i = 1; i < num; ++i) {
    if ((reader_.segments[i]->get_file_size() > 0) ||
        (reader_.segments[i]->get_memory_size() > 0))
    {
      seg_ok = false;
    }
  }
  return seg_ok;
}
//------------------------------------------------------------------------------

KernelELFInfo::KernelELFInfo(const std::string &name)
    : ELFInfo(name), kernel_offset_(), raw_kernel_offset_() {}

bool KernelELFInfo::loadELF(const std::string &path) {
  std::ifstream stream;
  stream.open(path.c_str(), std::ios::in | std::ios::binary);
  if (!stream) {
    return false;
  }
  return loadELF(stream);
}

bool KernelELFInfo::loadELF(std::istream &stream) {

  if (!ELFInfo::loadELF(stream)) {
    THROW("Could not load an ELF file.");
  }

  /*
   * Check out all kernel entry functions in .dynsym section.
   */
  using ELFIO::Elf64_Addr;
  using ELFIO::Elf_Half;
  using ELFIO::Elf_Xword;

  for (Elf_Half i = 0; i < reader_.sections.size(); ++i) {
    ELFIO::section *sec = reader_.sections[i];

    if (!(SHT_DYNSYM == sec->get_type() || SHT_SYMTAB == sec->get_type())) {
      continue;
    }
    ELFIO::symbol_section_accessor symbols(reader_, sec);

    for (Elf_Xword sym_idx = 0; sym_idx < symbols.get_symbols_num();
         ++sym_idx) {
      std::string name;
      Elf64_Addr value = 0;
      Elf_Xword size = 0;
      unsigned char bind = 0;
      unsigned char type = 0;
      Elf_Half section = 0;
      unsigned char other = 0;
      symbols.get_symbol(sym_idx, name, value, size, bind, type, section,
                         other);

      if (!(type == STT_FUNC && bind == STB_GLOBAL)) {
        continue;
      }
      // List of function name suffixes to search for
      std::vector<std::tuple<std::string, KernelOffsetMap *>> suffixes = {
          {"_ETKERNEL_entry_point", &kernel_offset_},
          {"_RAWKERNEL_entry_point", &raw_kernel_offset_}};

      for (auto &[suffix, kernel_map] : suffixes) {
        // if not suffix found continue
        if (!(name.size() >= suffix.size() &&
              name.compare(name.size() - suffix.size(), suffix.size(),
                           suffix) == 0)) {
          continue;
        }
        // remove the following suffix
        auto kernel_name = name.substr(0, name.size() - suffix.size());

        if (!kernel_name.empty()) {
          RTINFO << "Esperanto kernel: offset = 0x" << std::hex << value
                 << ", sym name = " << name << ", kernel name = " << kernel_name
                 << " [" << demangle(kernel_name) << "]\n";
          assert(value);
          (*kernel_map)[kernel_name] = value;
        }
      }
    }
  }
  return true;
}

bool KernelELFInfo::loadELF(std::vector<char> &data) {
  vectorwrapbuf databuf(data);
  std::istream is(&databuf);
  return loadELF(is);
}

bool KernelELFInfo::rawKernelExists(const std::string &name) {
  return raw_kernel_offset_.count(name) != 0;
}

size_t KernelELFInfo::rawKernelOffset(const std::string &name) {
  return raw_kernel_offset_[name];
}

} // namespace et_runtime
