/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

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

/* TODO: All trace packet information should be in common files
         for both Host and Device usage. */
constexpr uint32_t TRACE_STRING_MAX_SIZE = 64;
constexpr uint32_t TRACE_MAGIC_HEADER = 0x76543210;

extern "C" {
enum trace_type_e {
  TRACE_TYPE_STRING,
  TRACE_TYPE_PMC_COUNTER,
  TRACE_TYPE_PMC_ALL_COUNTERS,
  TRACE_TYPE_VALUE_U64,
  TRACE_TYPE_VALUE_U32,
  TRACE_TYPE_VALUE_U16,
  TRACE_TYPE_VALUE_U8,
  TRACE_TYPE_VALUE_FLOAT
};

enum trace_buffer_type_e {
  TRACE_MM_BUFFER,
  TRACE_CM_BUFFER,
  TRACE_SP_BUFFER
};

struct trace_buffer_size_header_t {
  uint32_t data_size;
} __attribute__((packed));

struct trace_buffer_std_header_t {
  uint32_t magic_header;
  uint32_t data_size;
  uint16_t type;
  uint8_t  pad[6];
} __attribute__((packed));

struct trace_entry_header_t {
  uint64_t cycle;   // Current cycle
  uint16_t type;    // One of enum trace_type_e
} __attribute__((packed));

struct trace_string_t {
  struct trace_entry_header_t header;
  char dataString[64];
} __attribute__((packed));
}
