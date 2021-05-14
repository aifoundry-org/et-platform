if (NOT TARGET etsoc_hal::etsoc_hal)
    add_library(etsoc_hal INTERFACE)
    add_library(etsoc_hal::etsoc_hal ALIAS etsoc_hal)
    target_include_directories(etsoc_hal
        INTERFACE 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/etsoc_hal/inc>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/etsoc_hal>
    )
  
    install(DIRECTORY etsoc_hal DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw)
    install(TARGETS etsoc_hal
        EXPORT EsperantoDeviceMinionRuntimeTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto-fw/etsoc_hal
        COMPONENT etsoc_hal
    )
endif()

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