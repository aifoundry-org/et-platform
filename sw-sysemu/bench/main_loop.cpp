#include <benchmark/benchmark.h>
#include "sys_emu.h"

// Define the list of ELF files to preload
std::vector<std::string> preload_elfs_list = {
     "./build/Release/device-bootloaders/lib/esperanto-fw/BootromTrampolineToBL2/BootromTrampolineToBL2.elf",
     "./build/Release/device-bootloaders/lib/esperanto-fw/ServiceProcessorBL2/fast-boot/ServiceProcessorBL2_fast-boot.elf",
     "./build/Release/device-minion-runtime/lib/esperanto-fw/MachineMinion/MachineMinion.elf",
     "./build/Release/device-minion-runtime/lib/esperanto-fw/MasterMinion/MasterMinion.elf",
     "./build/Release/device-minion-runtime/lib/esperanto-fw/WorkerMinion/WorkerMinion.elf"
};

static void BM_main_internal(benchmark::State& state) {
    sys_emu_cmd_options cmd_options;

    // Push the ELF files to cmd_options.elf_files
    for (const auto& elf_file : preload_elfs_list) {
        cmd_options.elf_files.push_back(elf_file);
    }

    std::unique_ptr<sys_emu> emu(new sys_emu(cmd_options));
    
    // Run the benchmark
    int status = emu.get()->main_internal();

    std::cout << "Status " << status << std::endl; 
};

// Register the function as a benchmark
BENCHMARK(BM_main_internal);

// Run the benchmark
BENCHMARK_MAIN();
