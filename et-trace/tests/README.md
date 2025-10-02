To add a device_trace test:

 1. Create a new test TEST_NAME.c file (see decode_test_template.c)
 2. Add the following to [tests/CMakeLists.txt](./CMakeLists.txt)
     `add_dtrace_test( TEST_NAME )`

This will create two new targets:

 - `TEST_NAME_mm` : Test compiled with -DMASTER_MINION
 - `TEST_NAME_cm` : Test compiled normally

Both of these targets will be added as tests as well.

To compile and run all tests (from project root):

    mkdir build && cd build
    cmake -DDTRACE_TEST=ON ..
    make
    ctest
