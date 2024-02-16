#include <benchmark/benchmark.h>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "sys_emu.h"
#include "elfs.h"

namespace {

// Define the list of ELF files to preload
std::vector<std::string> fw_elfs = {
    std::string{PROJECT_BINARY_DIR} + std::string{"/device-bootloaders/lib/esperanto-fw/BootromTrampolineToBL2/BootromTrampolineToBL2.elf"},
    std::string{PROJECT_BINARY_DIR} + std::string{"/device-bootloaders/lib/esperanto-fw/ServiceProcessorBL2/fast-boot/ServiceProcessorBL2_fast-boot.elf"},
    std::string{PROJECT_BINARY_DIR} + std::string{"/device-minion-runtime/lib/esperanto-fw/MachineMinion/MachineMinion.elf"},
    std::string{PROJECT_BINARY_DIR} + std::string{"/device-minion-runtime/lib/esperanto-fw/MasterMinion/MasterMinion.elf"},
    std::string{PROJECT_BINARY_DIR} + std::string{"/device-minion-runtime/lib/esperanto-fw/WorkerMinion/WorkerMinion.elf"}
};

class SysEmuBenchmark : public benchmark::Fixture {
public:
    SysEmuBenchmark(std::vector<std::string> elfs_to_preload)
        : elfs_to_preload(std::move(elfs_to_preload))
    {}

    void SetUp(benchmark::State& state) override {
        sys_emu_cmd_options cmd_options;
        cmd_options.mem_check = state.range(0);
        cmd_options.l1_scp_check = state.range(0);
        cmd_options.l2_scp_check = state.range(0);
        cmd_options.flb_check = state.range(0);
        cmd_options.tstore_check = state.range(1);
        // Push the ELF files to cmd_options.elf_files
        for (const auto& elf_file : elfs_to_preload) {
            cmd_options.elf_files.push_back(elf_file);
        }
        emu = std::make_unique<sys_emu>(cmd_options);
    }

    void TearDown(benchmark::State& state) override {
        auto num_inst = emu->get_emu_cycle();
        auto total_exe_time = emu->get_total_exe_time();
        auto cycles_p_s = (1e3 * num_inst) / total_exe_time;
        state.counters.insert({{"cycles", num_inst}, {"cycles/s", cycles_p_s}, {"total_time", total_exe_time}});
        // ips (instructions per second), same as 'cycles/s' but with google benchmark average time
        state.counters["ips"] = benchmark::Counter(num_inst, benchmark::Counter::kIsRate);
    }

protected:
    std::vector<std::string> elfs_to_preload;
    std::unique_ptr<sys_emu> emu;
};

class FWBenchmark : public SysEmuBenchmark {
public:
    FWBenchmark() : SysEmuBenchmark(fw_elfs) {}
};

BENCHMARK_DEFINE_F(FWBenchmark, BM_main_internal_fw_boot)(benchmark::State& state) {
    int status = EXIT_SUCCESS;
    for (auto _ : state) {
        // Run the benchmark
        benchmark::DoNotOptimize(status = emu->main_internal());
        benchmark::ClobberMemory();
        if (status != EXIT_SUCCESS) {
            state.SkipWithError("Failed to run emulator!");
            break; // Needed to skip the rest of the iteration.
        }
    }
};
BENCHMARK_REGISTER_F(FWBenchmark, BM_main_internal_fw_boot)
    ->ArgsProduct({{false, true}, {false, true}})
    ->ArgNames({"mem_check+l1_scp_check+l2_scp_check+flb_check", "tstore_check"});


class InstBenchmark : public SysEmuBenchmark {
public:
    InstBenchmark()
        : SysEmuBenchmark({std::string{DEVICE_KERNELS_DIR} + std::string{"inst_seq.elf"}})
    {}
};

BENCHMARK_DEFINE_F(InstBenchmark, BM_main_internal_inst_seq)(benchmark::State& state) {
    int status = EXIT_SUCCESS;
    for (auto _ : state) {
        // Run the benchmark
        benchmark::DoNotOptimize(status = emu->main_internal());
        benchmark::ClobberMemory();
        if (status != EXIT_SUCCESS) {
            state.SkipWithError("Failed to run emulator!");
            break; // Needed to skip the rest of the iteration.
        }
    }
};
BENCHMARK_REGISTER_F(InstBenchmark, BM_main_internal_inst_seq)
    ->ArgsProduct({{false, true}, {false, true}})
    ->ArgNames({"mem_check+l1_scp_check+l2_scp_check+flb_check", "tstore_check"});

} // end anonymous namespace

// Run the benchmark
BENCHMARK_MAIN();
