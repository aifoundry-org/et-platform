#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

configure_file(check-breakpad.sh.in check-breakpad.sh @ONLY)

build_test_binary(
  TARGET test_sentry
  SOURCES test_sentry.cc
  )


set(SENTRY_TEST_LIST
  segfault
  assert_failure
  )

if(SANITIZE_ADDRESS)

  list(APPEND SENTRY_TEST_LIST
    address_sanitizer_failure
    )

  set(TEST_PREFIX  ASAN_OPTIONS=abort_on_error=1)

endif(SANITIZE_ADDRESS)

foreach(SENTRY_TEST ${SENTRY_TEST_LIST})
  add_test(
    NAME Sentry::${SENTRY_TEST}
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/check-breakpad.sh --gtest-cmd "${TEST_PREFIX} ${CMAKE_CURRENT_BINARY_DIR}/test_sentry --gtest_filter=SentryTest.${SENTRY_TEST}" --test-name ${SENTRY_TEST}
    )
  set_tests_properties( Sentry::${SENTRY_TEST}
    PROPERTIES
      LABELS Generic Tools Sentry
      TIMEOUT 120
    )
endforeach()
