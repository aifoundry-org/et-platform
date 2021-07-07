add_library(test_framework_core OBJECT)

target_include_directories(test_framework_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
)

target_sources(test_framework_core
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tf/tf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_commands.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_generic_tests.c
)

target_link_libraries(test_framework_core
  PUBLIC
    EsperantoDeviceMinionRuntime::sp_firmware_helpers
    deviceFirmware::device_serial
)

target_compile_definitions(test_framework_core
  PRIVATE
    -DTF_CORE=1
)

####
add_library(test_framework OBJECT)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_maxion_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_noc_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_pcie_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_pu_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_spio_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_asset_tracking_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_pmic_tests.c
)

target_link_libraries(test_framework
  PUBLIC
    EsperantoDeviceMinionRuntime::sp_firmware_helpers
    deviceFirmware::device_serial
    BL2_driver
)

target_compile_definitions(test_framework
  PRIVATE
    -DTF_CORE=0
)
