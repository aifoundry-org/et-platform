/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include <esperanto/et-trace/layout.h>

#define ET_TRACE_DECODER_IMPL
#include <esperanto/et-trace/decoder.h>

// A namespace containing template for `bit_cast`. To be removed when `bit_cast` will be available
namespace templ {
template <class To, class From>
typename std::enable_if_t<
  sizeof(To) == sizeof(From) && std::is_trivially_copyable_v<From> && std::is_trivially_copyable_v<To>, To>
// constexpr support needs compiler magic
bit_cast(const From& src) noexcept {
  static_assert(std::is_trivially_constructible_v<To>,
                "This implementation additionally requires destination type to be trivially constructible");

  To dst;
  memcpy(&dst, &src, sizeof(To));
  return dst;
}
} // namespace templ

struct asic_frequencies_t {
  uint32_t minion_shire_mhz;
  uint32_t noc_mhz;
  uint32_t mem_shire_mhz;
  uint32_t ddr_mhz;
  uint32_t pcie_shire_mhz;
  uint32_t io_shire_mhz;
} __attribute__((packed));

struct dram_bw_t {
  uint32_t read_req_sec;
  uint32_t write_req_sec;
} __attribute__((packed));

struct max_dram_bw_t {
  uint8_t max_bw_rd_req_sec;
  uint8_t max_bw_wr_req_sec;
  uint8_t pad[6];
} __attribute__((packed));

struct module_uptime_t {
  uint16_t day;
  uint8_t hours;
  uint8_t mins;
  uint8_t pad[4];
} __attribute__((packed));

struct module_voltage_t {
  uint8_t ddr;
  uint8_t l2_cache;
  uint8_t maxion;
  uint8_t minion;
  uint8_t pcie;
  uint8_t noc;
  uint8_t pcie_logic;
  uint8_t vddqlp;
  uint8_t vddq;
  uint8_t pad[7];
} __attribute__((packed));

typedef uint8_t power_state_e;

#define SP_NUM_REGISTERS (32)
#define SP_EXCEPTION_STACK_FRAME_SIZE (sizeof(uint64_t) * 28)
#define SP_EXCEPTION_CSRS_FRAME_SIZE (sizeof(uint64_t) * 4)
#define SP_EXCEPTION_FRAME_SIZE (SP_EXCEPTION_STACK_FRAME_SIZE + SP_EXCEPTION_CSRS_FRAME_SIZE)
#define SP_PERF_GLOBALS_SIZE                                                                                           \
  (sizeof(struct asic_frequencies_t) + sizeof(struct dram_bw_t) + sizeof(struct max_dram_bw_t) + sizeof(uint32_t) +    \
   sizeof(uint64_t))
#define SP_POWER_GLOBALS_SIZE                                                                                          \
  (sizeof(power_state_e) + sizeof(uint8_t) + (sizeof(uint8_t) * 3) + sizeof(struct module_uptime_t) +                  \
   sizeof(struct module_voltage_t) + sizeof(uint64_t) + sizeof(uint64_t))
#define SP_GLOBALS_SIZE (SP_PERF_GLOBALS_SIZE + SP_POWER_GLOBALS_SIZE)
