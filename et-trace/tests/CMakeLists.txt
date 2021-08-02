add_library(dtrace_test_mm common/device_trace.c common/mock_atomics.c)
target_compile_definitions(dtrace_test_mm PUBLIC MASTER_MINION)
target_compile_options(dtrace_test_mm PRIVATE "-Wno-address-of-packed-member")
target_link_libraries(dtrace_test_mm PUBLIC device_trace)

add_library(dtrace_test_cm common/device_trace.c)
target_link_libraries(dtrace_test_cm PUBLIC device_trace)

macro(add_dtrace_test name)
  add_executable(${name}_mm ${name}.c)
  target_link_libraries(${name}_mm PRIVATE dtrace_test_mm)
  target_compile_definitions(${name}_mm PRIVATE MASTER_MINION)

  add_executable(${name}_cm ${name}.c)
  target_link_libraries(${name}_cm PRIVATE dtrace_test_cm)

  add_test(NAME ${name}_mm COMMAND ${name}_mm)
  add_test(NAME ${name}_cm COMMAND ${name}_cm)
endmacro()

add_dtrace_test(decode_string_test)
add_dtrace_test(decode_string_fmt_test)
add_dtrace_test(decode_string_ovf_test)
add_dtrace_test(decode_pmc_test)
add_dtrace_test(decode_u8_test)
add_dtrace_test(decode_u16_test)
add_dtrace_test(decode_u32_test)
add_dtrace_test(decode_u64_test)
add_dtrace_test(decode_float_test)
