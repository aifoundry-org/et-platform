/*-------------------------------------------------------------------------
 * Copyright (C) 2023, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "CoreDumper.h"
#include "ETSOCElf.h"
#include "RuntimeImp.h"
#include "Utils.h"
#include "runtime/Types.h"
#include <fstream>
#include <unistd.h>

using namespace rt;

namespace {
constexpr auto kAlignment = 256;
constexpr auto kMaxValidContextType = 4;

// Standard RISC-V exception mcause values
// Instruction address misaligned
constexpr uint64_t RISCV_INSTR_MISSALIGN_MCAUSE = 0;
// Instruction access fault
constexpr uint64_t RISCV_INSTR_FAULT_MCAUSE = 1;
// Illegal instruction
constexpr uint64_t RISCV_INSTR_ILLEGAL_MCAUSE = 2;
// Load address misaligned
constexpr uint64_t RISCV_LOAD_MISSALIGN_MCAUSE = 4;
// Load access fault
constexpr uint64_t RISCV_LOAD_FAULT_MCAUSE = 5;
// Store/AMO address misaligned
constexpr uint64_t RISCV_STORE_MISSALIGN_MCAUSE = 6;
// Store/AMO access fault
constexpr uint64_t RISCV_STORE_FAULT_MCAUSE = 7;

// Linux process states
enum class ProcessState { R = 0, S, D, T, Z, W };

// ELF machine ID which may not be in the linux headers
#ifndef EM_RISCV
#define EM_RISCV 243
#endif

// Helper class to dump structs piecewise (to skip compiler introduced
// padding)
struct Dumper {
  std::ostream& out_;

  explicit Dumper(std::ostream& out)
    : out_(out) {
  }

  template <typename T> void operator()(T const& object) const {
    out_.write(reinterpret_cast<char const*>(&object), sizeof(object));
  }
};

struct NoteData {
  // Note fragments of the main thread
  ETSOCElf::MainThreadNote mainThread_;
  // Note fragments of all other threads
  std::vector<ETSOCElf::PRStatusNote> threads_;

  // Size in disk
  size_t size() const {
    return ETSOCElf::MainThreadNote::size_ + ETSOCElf::PRStatusNote::size_ * threads_.size();
  }

  // Apply a function to each member/element
  template <typename FUNC, typename... Args> void apply(FUNC& f, Args... args) const {
    mainThread_.apply(f, args...);
    for (auto const& thread : threads_) {
      thread.apply(f, args...);
    }
  }
};

// Returns the positions of the context vector that contain a valid entry
std::vector<size_t> getValidErrorContextIndices(const rt::StreamError& error) {
  std::vector<size_t> result;

  auto& contexts = error.errorContext_.value();
  // it starts with 1 because all older logic was done that way ... but 0 should be valid as well
  for (size_t i = 1; i < contexts.size(); i++) {
    if (contexts[i].type_ <= kMaxValidContextType) {
      result.push_back(i);
    }
  }
  return result;
}

ETSOCElf::PRSigInfoNote getSigInfoNote(const rt::ErrorContext& context) {
  ETSOCElf::PRSigInfoNote note;

  // Since this is a struct containing unions, here we initialize it to 0
  memset(&note.sigInfo_, 0, sizeof(note.sigInfo_));

  bool isInterrupt = context.mcause_ & (1UL << 63);
  uint64_t cleanMCause = context.mcause_ & ~(1UL << 63);

  note.sigInfo_.si_pid = static_cast<int32_t>(context.hartId_);
  if (isInterrupt) {
    note.sigInfo_.si_signo = SIGINT;
  } else {
    switch (cleanMCause) {
    case RISCV_INSTR_MISSALIGN_MCAUSE:
      note.sigInfo_.si_signo = SIGBUS;
      note.sigInfo_.si_code = BUS_ADRALN;
      break;
    case RISCV_INSTR_FAULT_MCAUSE:
      note.sigInfo_.si_signo = SIGSEGV;
      note.sigInfo_.si_code = SEGV_ACCERR;
      break;
    case RISCV_INSTR_ILLEGAL_MCAUSE:
      note.sigInfo_.si_signo = SIGILL;
      note.sigInfo_.si_code = ILL_ILLOPC;
      break;
    case RISCV_LOAD_MISSALIGN_MCAUSE:
      note.sigInfo_.si_signo = SIGBUS;
      note.sigInfo_.si_code = BUS_ADRALN;
      break;
    case RISCV_LOAD_FAULT_MCAUSE:
      note.sigInfo_.si_signo = SIGSEGV;
      note.sigInfo_.si_code = SEGV_ACCERR;
      break;
    case RISCV_STORE_MISSALIGN_MCAUSE:
      note.sigInfo_.si_signo = SIGBUS;
      note.sigInfo_.si_code = BUS_ADRALN;
      break;
    case RISCV_STORE_FAULT_MCAUSE:
      note.sigInfo_.si_signo = SIGSEGV;
      note.sigInfo_.si_code = SEGV_ACCERR;
      break;
    default:
      // NOTE: we assign SIGPIPE for things that make no sense
      note.sigInfo_.si_signo = SIGPIPE;
      break;
    }
    note.sigInfo_.si_addr = reinterpret_cast<void*>(context.mtval_);
  }

  return note;
}

// Generates the process status note
ETSOCElf::PRStatusNote getPRStatusNote(const rt::ErrorContext& context) {
  ETSOCElf::PRStatusNote note;

  // Build the ELF siginfo from what would be a system siginfo note
  auto siginfoNote = getSigInfoNote(context);
  note.status_.pr_info.si_signo = siginfoNote.sigInfo_.si_signo;
  note.status_.pr_info.si_code = siginfoNote.sigInfo_.si_code;
  note.status_.pr_info.si_errno = 0;
  note.status_.pr_cursig = 0;
  note.status_.pr_sigpend = 0;
  note.status_.pr_sighold = 0;
  note.status_.pr_pid = static_cast<uint32_t>(context.hartId_);
  note.status_.pr_ppid = 0;
  note.status_.pr_pgrp = 0;
  note.status_.pr_sid = 0;
  // NOTE: note.pr_utime, note.pr_stime, note.pr_cu_time and note.pr_cstime
  // are set to 0 by default

  note.status_.pr_reg.pc = context.mepc_;
  for (auto i = 0U; i < 31; i++) {
    note.status_.pr_reg.xregs[i] = context.gpr_[i];
  }

  note.status_.pr_fpvalid = 0;

  return note;
}

ETSOCElf::PRPSInfoNote getPSInfoNote(const rt::ErrorContext& context, std::string const& processName) {
  ETSOCElf::PRPSInfoNote note;

  // Zero it to simplify copying strings
  memset(&note.psInfo_, 0, sizeof(note.psInfo_));

  note.psInfo_.pr_state = static_cast<char>(ProcessState::R);
  note.psInfo_.pr_sname = 'R';
  note.psInfo_.pr_zombie = 0;
  note.psInfo_.pr_nice = 0;
  note.psInfo_.pr_flag = PF_DUMPCORE;
  note.psInfo_.pr_uid = getuid();
  note.psInfo_.pr_gid = getgid();
  note.psInfo_.pr_pid = static_cast<int32_t>(context.hartId_);
  note.psInfo_.pr_ppid = 0;
  note.psInfo_.pr_pgrp = 0;
  note.psInfo_.pr_sid = 0;

  // Set up the process name
  strncpy(note.psInfo_.pr_fname, processName.c_str(), sizeof(note.psInfo_.pr_fname));
  strncpy(note.psInfo_.pr_psargs, processName.c_str(), sizeof(note.psInfo_.pr_psargs));
  return note;
}

NoteData createNoteData(const rt::StreamError& error, const std::vector<size_t>& contextIndices,
                        std::string const& processName) {

  // TODO check if this is needed, i think this is guaranteed by the program flow already
  RT_LOG_IF(FATAL, contextIndices.empty()) << "Context indices can not be empty.";

  auto& contexts = error.errorContext_.value();
  auto const& first = contexts[contextIndices[0]];

  NoteData data;
  data.mainThread_.prStatus_ = getPRStatusNote(first);
  data.mainThread_.psInfo_ = getPSInfoNote(first, processName);
  data.mainThread_.sigInfo_ = getSigInfoNote(first);

  for (size_t i = 1; i < contextIndices.size(); i++) {
    auto index = contextIndices[i];
    data.threads_.emplace_back(getPRStatusNote(contexts[index]));
  }

  return data;
}
} // namespace

void CoreDumper::addCodeAddress(DeviceId id, std::byte* address) {
  auto res = codeAddresses_[id].emplace(address);
  LOG_IF(FATAL, !res.second) << "Address already registered. This is likely a bug.";
}

void CoreDumper::removeCodeAddress(DeviceId id, std::byte* address) {
  auto res = codeAddresses_[id].erase(address);
  LOG_IF(FATAL, res == 0) << "Address not registered. This is likely a bug.";
}

void CoreDumper::addKernelExecution(const std::string& coreDumpPath, KernelId kernelId, EventId eventId) {
  if (kernelExecutions_.find(eventId) != kernelExecutions_.end()) {
    throw Exception("EventId already registered. This is likely a bug.");
  }
  kernelExecutions_[eventId] = {kernelId, coreDumpPath};
}

void CoreDumper::removeKernelExecution(EventId eventId) {
  kernelExecutions_.erase(eventId);
}

void CoreDumper::dump(EventId eventId, const std::vector<AllocationInfo>& allocations, const rt::StreamError& error,
                      RuntimeImp& runtime) {

  auto it = kernelExecutions_.find(eventId);
  if (it == end(kernelExecutions_)) {
    return; // nothing to dump
  }
  if (not error.errorContext_.has_value()) {
    RT_LOG(WARNING) << "Device error (no core dump possible). Not dumping a core without any kernel loaded.";
    return;
  }

  auto contextIndices = getValidErrorContextIndices(error);
  if (contextIndices.empty()) {
    LOG(WARNING) << "Device error (no core dump possible). Could not find any initialized device error context.";
    return;
  }

  RT_LOG(WARNING) << "Dumping the stack is not possible yet.";

  auto [kernelId, coreDumpFilePath] = it->second;
  unused(kernelId);

  // try to open a writing stream
  auto os = std::ofstream{coreDumpFilePath, std::ios::binary | std::ios::trunc};

  if (!os.is_open()) {
    RT_LOG(WARNING) << "Could not open file " << coreDumpFilePath << " for writing";
    return;
  }

  auto numSegments = /* notes */ 1 + allocations.size();
  RT_LOG_IF(FATAL, numSegments > std::numeric_limits<uint16_t>::max()) << "Too many segments to dump";

  auto device = error.device_;

  auto fileOffset = 0UL;

  // write the ELF header
  ETSOCElf::Header header;
  header.e_type = ET_CORE;
  header.e_machine = EM_RISCV;
  header.e_phnum = static_cast<uint16_t>(numSegments);

  os.write(reinterpret_cast<const char*>(&header), sizeof(header));
  fileOffset += sizeof(header);

  std::vector<ETSOCElf::SegmentHeader> segmentHeaders(numSegments);

  // Data starts after the segment headers
  auto dataOffset = sizeof(ETSOCElf::Header) + sizeof(ETSOCElf::SegmentHeader) * numSegments;
  dataOffset = align(dataOffset, kAlignment);

  // Initialize the notes segment header
  ETSOCElf::SegmentHeader& notesHeader = segmentHeaders[0];
  std::string processName = "dummyProcessName";
  auto note = createNoteData(error, contextIndices, processName);
  notesHeader.p_type = PT_NOTE;
  notesHeader.p_offset = dataOffset;
  notesHeader.p_filesz = note.size();
  notesHeader.p_align = 4;

  dataOffset = dataOffset + note.size();
  dataOffset = align(dataOffset, kAlignment);

  // Initialize the code and data segment headers
  size_t segmentIndex = 1;
  for (auto [address, size] : allocations) {
    auto& segmentHeader = segmentHeaders[segmentIndex];

    segmentHeader.p_type = PT_LOAD;

    // check if this is code or data section
    if (codeAddresses_[device].find(address) != codeAddresses_[device].end()) {
      segmentHeader.p_flags = (PF_X | PF_R);
    } else {
      segmentHeader.p_flags = (PF_R | PF_W);
    }

    segmentHeader.p_offset = dataOffset;
    segmentHeader.p_vaddr = reinterpret_cast<uint64_t>(address);
    segmentHeader.p_paddr = reinterpret_cast<uint64_t>(address);
    segmentHeader.p_filesz = size;
    segmentHeader.p_memsz = size;
    segmentHeader.p_align = kAlignment;

    dataOffset = dataOffset + size;
    dataOffset = align(dataOffset, kAlignment);
    segmentIndex++;
  }
  // Write the segment headers
  os.write(reinterpret_cast<const char*>(segmentHeaders.data()),
           static_cast<long>(sizeof(*segmentHeaders.data()) * segmentHeaders.size()));
  fileOffset += sizeof(*segmentHeaders.data()) * segmentHeaders.size();

  // Enforce alignment in the output file
  std::fill_n(std::ostreambuf_iterator<char>(os), segmentHeaders[0].p_offset - fileOffset, 0);
  fileOffset = segmentHeaders[0].p_offset;

  // Dump the note segment
  {
    Dumper dumper(os);
    note.apply(dumper);
    fileOffset += note.size();
  }

  auto stream = runtime.doCreateStream(device);

  // Dump allocated device memory regions
  segmentIndex = 1;
  for (auto [address, size] : allocations) {
    // Space for a copy in the host
    std::vector<std::byte> hostAddress(size);

    // Copy from device to host
    auto copyEventId =
      runtime.doMemcpyDeviceToHost(stream, address, hostAddress.data(), size, false, defaultCmaCopyFunction);

    // Enforce alignment in the output file
    std::fill_n(std::ostreambuf_iterator<char>(os), segmentHeaders[segmentIndex].p_offset - fileOffset, 0);
    fileOffset = segmentHeaders[segmentIndex].p_offset;

    // Wait for the copy to finish
    auto success = runtime.doWaitForEvent(copyEventId);
    if (not success) {
      RT_LOG(WARNING) << "Timed out copying core dump data from device.";
      runtime.destroyStream(stream);
      return;
    }

    // Write the segment
    os.write(reinterpret_cast<const char*>(hostAddress.data()), static_cast<long>(size));
    fileOffset += size;

    segmentIndex++;
  }

  runtime.doDestroyStream(stream);
  RT_LOG(INFO) << "Core dump completed.";
}
