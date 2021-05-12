#FreeRTOS

#FreeRTOS source
file(GLOB FREERTOS_SRC_FILES "FreeRTOS/Source/*.c"
                               "FreeRTOS/Source/portable/GCC/RISC-V/*.c"
                               "FreeRTOS/Source/portable/GCC/RISC-V/*.S")
                               #"FreeRTOS/Source/mem_mang/heap_3.c" not using FreeRTOS heap. Heap is only for libc.
  
#FreeRTOS includes
get_filename_component(FREERTOS_INC_DIR "FreeRTOS/Source/include/" ABSOLUTE)
get_filename_component(FREERTOS_PORT_INC_DIR "FreeRTOS/Source/portable/GCC/RISC-V/" ABSOLUTE)
get_filename_component(FREERTOS_CHIP_SPECIFIC_INC_DIR "FreeRTOS/Source/portable/GCC/RISC-V/chip_specific_extensions/etsoc_RV64IMF" ABSOLUTE)
set(FREERTOS_INC_DIRS ${FREERTOS_INC_DIR} ${FREERTOS_PORT_INC_DIR} ${FREERTOS_CHIP_SPECIFIC_INC_DIR})


set(FREERTOS_SRC_FILES ${FREERTOS_SRC_FILES} PARENT_SCOPE)
set(FREERTOS_INC_DIRS ${FREERTOS_INC_DIRS} PARENT_SCOPE)


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