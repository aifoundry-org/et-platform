/**
 * Copyright (c) 2023, Esperanto Technologies Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <vector>

#include <linux/elf.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>

namespace rt {
static uint32_t constexpr PF_DUMPCORE = 0x200;

// The following are some definitions adapted from elfcore.h from linux to make
// them match the ETSOC-1 target

static int32_t constexpr ETSOC_EI_NINDENT = 16;
static int32_t constexpr ETSOC_ELF_PRARGSZ = 80;

// Signal information
struct etsoc_elf_siginfo {
  int32_t si_signo;
  int32_t si_code;
  int32_t si_errno;
};

struct etsoc_timeval {
  uint64_t tv_sec = 0;
  uint64_t tv_usec = 0;
};

// Set of General Purpose Registers
struct etsoc_gregset_t {
  uint64_t pc;
  uint64_t xregs[31];
};

// Processor status
struct etsoc_elf_prstatus {
  etsoc_elf_siginfo pr_info;
  uint16_t pr_cursig = 0;
  uint64_t pr_sigpend = 0;
  uint64_t pr_sighold = 0;
  uint32_t pr_pid = 0;
  uint32_t pr_ppid = 0;
  uint32_t pr_pgrp = 0;
  uint32_t pr_sid = 0;
  etsoc_timeval pr_utime;
  etsoc_timeval pr_stime;
  etsoc_timeval pr_cutime;
  etsoc_timeval pr_cstime;
  etsoc_gregset_t pr_reg;
  uint32_t pr_fpvalid = 0;
};

// Process information
struct etsoc_elf_prpsinfo {
  int8_t pr_state;
  char pr_sname;
  int8_t pr_zombie;
  int8_t pr_nice;
  uint64_t pr_flag;
  uint32_t pr_uid;
  uint32_t pr_gid;
  int32_t pr_pid;
  int32_t pr_ppid;
  int32_t pr_pgrp;
  int32_t pr_sid;
  char pr_fname[16];
  char pr_psargs[ETSOC_ELF_PRARGSZ];
};

class ETSOCElf {
public:
  // Header of a segment (AKA program header)
  struct SegmentHeader {
    uint32_t p_type = 0;
    uint32_t p_flags = 0;
    uint64_t p_offset = 0;
    uint64_t p_vaddr = 0;
    uint64_t p_paddr = 0;
    uint64_t p_filesz = 0;
    uint64_t p_memsz = 0;
    uint64_t p_align = 0;
  };

  // ELF file header
  struct Header {
    // ELF magic number
    unsigned char e_ident[EI_NIDENT] = {
      ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS64, ELFDATA2LSB, EV_CURRENT, ELFOSABI_NONE, 0, 0, 0, 0, 0, 0, 0, 0};
    // Type of ELF file
    uint16_t e_type = 0;
    // Architecture
    uint16_t e_machine = 0;
    // ELF version
    uint32_t e_version = EV_CURRENT;
    // Entry point
    uint64_t e_entry = 0;
    // First SegmentHeader
    uint64_t e_phoff = sizeof(Header); // Right after this header
    // First Section Header Offset
    uint64_t e_shoff = 0;
    // Flags
    uint32_t e_flags = 0;
    // Size of the ELF header
    uint16_t e_ehsize = sizeof(Header);
    // Size of a segment header
    uint16_t e_phentsize = sizeof(SegmentHeader);
    // Number of segments
    uint16_t e_phnum = 0;
    // Size of a section header
    uint16_t e_shentsize = 0;
    // Number of sections
    uint16_t e_shnum = 0;
    // Index of the section that contains the section header string table
    uint16_t e_shstrndx = SHN_UNDEF;
  };

  // Header of an entry of the note segment
  struct NoteHeader {
    // Name size including the null
    uint32_t n_namesz = 5;
    // Actual size of the note (that follows this header)
    uint32_t n_descsz;
    // Type of note
    uint32_t n_type;
    // Name padded to a multiple of 4 bytes
    char noteName[8] = {'C', 'O', 'R', 'E', 0, 0, 0, 0};

    NoteHeader(uint32_t size, uint32_t type)
      : n_descsz(size)
      , n_type(type) {
    }
  };

  struct PRStatusNote {
    NoteHeader header_{sizeof(etsoc_elf_prstatus), NT_PRSTATUS};
    etsoc_elf_prstatus status_;

    static constexpr size_t size_ = sizeof(NoteHeader) + sizeof(etsoc_elf_prstatus);

    // Apply a function to each member
    template <typename FUNC, typename... Args> void apply(FUNC& f, Args... args) const {
      f(header_, args...);
      f(status_, args...);
    }
  };

  struct PRPSInfoNote {
    NoteHeader header_{sizeof(etsoc_elf_prpsinfo), NT_PRPSINFO};
    etsoc_elf_prpsinfo psInfo_;

    static constexpr size_t size_ = sizeof(NoteHeader) + sizeof(etsoc_elf_prpsinfo);

    // Apply a function to each member
    template <typename FUNC, typename... Args> void apply(FUNC& f, Args... args) const {
      f(header_, args...);
      f(psInfo_, args...);
    }
  };

  struct PRSigInfoNote {
    NoteHeader header_{sizeof(siginfo_t), NT_SIGINFO};
    siginfo_t sigInfo_;

    static constexpr size_t size_ = sizeof(NoteHeader) + sizeof(siginfo_t);

    // Apply a function to each member
    template <typename FUNC, typename... Args> void apply(FUNC& f, Args... args) const {
      f(header_, args...);
      f(sigInfo_, args...);
    }
  };

  struct MainThreadNote {
    PRStatusNote prStatus_;
    PRPSInfoNote psInfo_;
    PRSigInfoNote sigInfo_;

    static constexpr size_t size_ = PRStatusNote::size_ + PRPSInfoNote::size_ + PRSigInfoNote::size_;

    // Apply a function to each member
    template <typename FUNC, typename... Args> void apply(FUNC& f, Args... args) const {
      prStatus_.apply(f, args...);
      psInfo_.apply(f, args...);
      sigInfo_.apply(f, args...);
    }
  };

private:
  // ELF file header
  Header header_;
  // Headers of the segments
  std::vector<SegmentHeader> segmentHeaders_;
  // Device address and size of data (not note) segments
  std::vector<std::tuple<std::byte*, size_t>> segmentData_;
};

} // namespace rt