message(${CMAKE_CURRENT_SOURCE_DIR})
message(${CMAKE_CURRENT_BINARY_DIR})

set(tf conftest.py tf_specification.json tf_specification.py tf_target.py test_sp.py)
foreach(file ${tf})
    message("filename: ${file}")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${file} ${CMAKE_CURRENT_BINARY_DIR}/${file} COPYONLY)
endforeach()

find_package(GTest REQUIRED)
find_package(gflags REQUIRED)
find_package(esperanto-device-api REQUIRED)
find_package(EsperantoBootLoader REQUIRED)
find_package(EsperantoDeviceMinionRuntime REQUIRED)

include_directories(${CMAKE_INSTALL_INCLUDEDIR}/device-layer)

#helper to create tests
function(create_test)
  cmake_parse_arguments(TEST
    ""
    "TARGET"
    "SOURCES;ARGS;PROPERTIES"
    ${ARGN}
  )

  add_executable(${TEST_TARGET} ${TEST_SOURCES})
  target_compile_features(${TEST_TARGET} PRIVATE cxx_std_17)
  target_link_libraries(${TEST_TARGET} deviceLayer::deviceLayer GTest::GTest GTest::Main gflags)
  target_include_directories(${TEST_TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${ESPERANTO_DEVICE_API_INCLUDE_DIR} ..)
  gtest_discover_tests(${TEST_TARGET}
    EXTRA_ARGS ${TEST_ARGS}
    TEST_PREFIX device-layer:
    PROPERTIES ${TEST_PROPERTIES}
    TEST_LIST DISCOVERED_TESTS
  )
endfunction()

#all tests should be created using the helper
create_test(
  TARGET     launch_sysemu
  SOURCES    launch_sysemu.cpp
  PROPERTIES "LABELS;Generic;LABELS;SYSEMU;LABELS;Bootloaders;TIMEOUT;20"
)