find_package(Benchmark REQUIRED)

add_executable(bench main_loop.cpp)
target_include_directories(bench
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/sys_emu>
)
target_link_libraries(bench
    PRIVATE
        benchmark::benchmark
        sw-sysemu::sw-sysemu
)
