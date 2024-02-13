#include <benchmark/benchmark.h>

#include "sys_emu.h"

static void BM_main_internal(benchmark::State& state) {
    // Perform setup here
    sys_emu_cmd_options options;
    //FILLME: OBS this is incomplete, current struct defaults end up
    //        generating SIGSEGV on main loop start
    sys_emu emulator(options);

    for (auto _ : state) {
        // This code gets timed
        emulator.main_internal();
    }
}
// Register the function as a benchmark
BENCHMARK(BM_main_internal);
// Run the benchmark
BENCHMARK_MAIN();
