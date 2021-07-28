add_executable(decode_trace_test
  decode_trace.c
  device_trace.c
)
target_link_libraries(decode_trace_test PRIVATE device_trace)
add_test(NAME decode_trace_test COMMAND decode_trace_test)
