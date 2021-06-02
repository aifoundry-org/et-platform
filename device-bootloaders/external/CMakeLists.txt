#FreeRTOS Config
add_library(FreeRTOSConfig INTERFACE)
add_library(FreeRTOS::FreeRTOSConfig ALIAS FreeRTOSConfig)
target_include_directories(FreeRTOSConfig INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOSConfig)
target_link_libraries(FreeRTOSConfig INTERFACE etsoc_hal::etsoc_hal)

#FreeRTOS
file(GLOB FREERTOS_SRC_FILES "FreeRTOS/Source/*.c"
                             "FreeRTOS/Source/portable/GCC/RISC-V/*.c"
                             "FreeRTOS/Source/portable/GCC/RISC-V/*.S")
                            #"FreeRTOS/Source/mem_mang/heap_3.c" not using FreeRTOS heap. Heap is only for libc.

add_library(FreeRTOS STATIC ${FREERTOS_SRC_FILES})
add_library(FreeRTOS::FreeRTOS ALIAS FreeRTOS)
target_include_directories(FreeRTOS PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/Source/include
    ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/Source/portable/GCC/RISC-V
    ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/Source/portable/GCC/RISC-V/chip_specific_extensions/etsoc_RV64IMF
)
target_compile_definitions(FreeRTOS PRIVATE -DportasmHANDLE_INTERRUPT=external_interrupt_handler)
target_link_libraries(FreeRTOS PUBLIC FreeRTOS::FreeRTOSConfig)
