add_library(test_framework_core OBJECT)

target_include_directories(test_framework_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
)

target_sources(test_framework_core
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tf/tf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_command_definitions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_command_handlers_generic.c
)

target_link_libraries(test_framework_core
  PUBLIC
    tf-protocol::tf-protocol
    deviceFirmware::device_common
    et-common-libs::sp-bl1
    et-common-libs::sp-bl2
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
    ${CMAKE_CURRENT_SOURCE_DIR}/test_command_definitions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_command_handlers_generic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_mm_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_mem_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_minion_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_maxion_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_noc_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_pcie_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_pu_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_spio_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_asset_tracking_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_pmic_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_sp_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_trace_tests.c
)

target_link_libraries(test_framework
  PUBLIC
    tf-protocol::tf-protocol
    deviceFirmware::device_common
    et-common-libs::sp-bl2
    FreeRTOS::FreeRTOS
    BL2_driver
)

target_compile_definitions(test_framework
  PRIVATE
    -DTF_CORE=0
)
