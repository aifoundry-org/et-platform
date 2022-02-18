cmake_minimum_required(VERSION 3.8)
project(test_package C)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(EsperantoBootLoader REQUIRED)

get_property(BOOTROM_TRAMPOLINE_TO_BL2_ELF TARGET EsperantoBootLoader::ServiceProcessorBL1.elf PROPERTY LOCATION)
get_property(BL1_ELF TARGET EsperantoBootLoader::ServiceProcessorBL1.elf PROPERTY LOCATION)
get_property(BL2_ELF TARGET EsperantoBootLoader::ServiceProcessorBL2_fast-boot.elf PROPERTY LOCATION)

message("BOOTROM_TRAMPOLINE_TO_BL2_ELF   location: ${BOOTROM_TRAMPOLINE_TO_BL2_ELF}")
message("BL1_ELF   location: ${BL1_ELF}")
message("BL2_ELF   location: ${BL2_ELF}")

