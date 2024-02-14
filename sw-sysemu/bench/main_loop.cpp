#include <benchmark/benchmark.h>
#include <cstdlib>

#include "sys_emu.h"
#include "elfs.h"

// Define the list of ELF files to preload
std::vector<std::string> preload_elfs_list = {
    std::string{PROJECT_BINARY_DIR} + std::string{"/device-bootloaders/lib/esperanto-fw/BootromTrampolineToBL2/BootromTrampolineToBL2.elf"},
    std::string{PROJECT_BINARY_DIR} + std::string{"/device-bootloaders/lib/esperanto-fw/ServiceProcessorBL2/fast-boot/ServiceProcessorBL2_fast-boot.elf"},
    std::string{PROJECT_BINARY_DIR} + std::string{"/device-minion-runtime/lib/esperanto-fw/MachineMinion/MachineMinion.elf"},
    std::string{PROJECT_BINARY_DIR} + std::string{"/device-minion-runtime/lib/esperanto-fw/MasterMinion/MasterMinion.elf"},
    std::string{PROJECT_BINARY_DIR} + std::string{"/device-minion-runtime/lib/esperanto-fw/WorkerMinion/WorkerMinion.elf"}
};

static void BM_main_internal(benchmark::State& state) {
    sys_emu_cmd_options cmd_options;

    // Push the ELF files to cmd_options.elf_files
    for (const auto& elf_file : preload_elfs_list) {
        cmd_options.elf_files.push_back(elf_file);
    }

    auto emu = std::make_unique<sys_emu>(cmd_options);

    int status;
    for (auto _ : state) {
        // Run the benchmark
        status = emu->main_internal();
        assert(status == EXIT_SUCCESS);
    }

    std::cout << "Status " << status << std::endl;
};

// Register the function as a benchmark
BENCHMARK(BM_main_internal);

// Run the benchmark
BENCHMARK_MAIN();
