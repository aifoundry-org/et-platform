add_library(test_framework)

target_include_directories(test_framework
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
)
    
target_sources(test_framework
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tf/tf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_commands.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_generic_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_mem_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_minion_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_noc_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_pcie_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_pu_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_spio_tests.c
)

target_link_libraries(test_framework
  PRIVATE
    EsperantoDeviceMinionRuntime::sp_firmware_helpers
    device_serial
)
