find_package(Benchmark REQUIRED)

configure_file(elfs.h.in ${CMAKE_CURRENT_BINARY_DIR}/elfs.h)

add_executable(bench main_loop.cpp)
target_include_directories(bench
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/sys_emu>
)
target_link_libraries(bench
    PRIVATE
        benchmark::benchmark
        sw-sysemu::sw-sysemu
)
