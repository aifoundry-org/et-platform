cmake_minimum_required(VERSION 3.8)
project(test_package C)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(EsperantoDeviceMinionRuntime REQUIRED)

get_property(MACHINE_MINION_ELF TARGET EsperantoDeviceMinionRuntime::MachineMinion.elf PROPERTY LOCATION)
get_property(MASTER_MINION_ELF TARGET EsperantoDeviceMinionRuntime::MasterMinion.elf PROPERTY LOCATION)
get_property(MASTER_MINION_TF_ELF TARGET EsperantoDeviceMinionRuntime::MasterMinionTF.elf PROPERTY LOCATION)
get_property(WORKER_MINION_ELF TARGET EsperantoDeviceMinionRuntime::WorkerMinion.elf PROPERTY LOCATION)

message("MACHINE_MINION_ELF   location: ${MACHINE_MINION_ELF}")
message("MASTER_MINION_ELF    location: ${MASTER_MINION_ELF}")
message("MASTER_MINION_TF_ELF location: ${MASTER_MINION_TF_ELF}")
message("WORKER_MINION_ELF    location: ${WORKER_MINION_ELF}")

get_filename_component(GET_GIT_HASH_ABS_PATH "${ESPERANTO_DEVICE_MINION_RUNTIME_BIN_DIR}/esperanto-fw/get_git_hash.py" ABSOLUTE)
get_filename_component(GET_GIT_VERSION_ABS_PATH "${ESPERANTO_DEVICE_MINION_RUNTIME_BIN_DIR}/esperanto-fw/get_git_version.py" ABSOLUTE)

message("GET_GIT_HASH_ABS_PATH    path: ${GET_GIT_HASH_ABS_PATH}")
message("GET_GIT_VERSION_ABS_PATH path: ${GET_GIT_VERSION_ABS_PATH}")