
set(DEVICE_KERNELS_DIR ${PROJECT_SOURCE_DIR}/../device/tests)


# 
# compile generic_launcher lib
#

add_library(etsoc_gpsdk STATIC 
    src/lib/GenericLauncher.cpp 
)
target_include_directories(etsoc_gpsdk PUBLIC include ${CMAKE_CURRENT_LIST_DIR}/../../common/include sdk/include)
target_link_libraries(etsoc_gpsdk 
    PUBLIC 
        deviceLayer::deviceLayer
        runtime::etrt
    PRIVATE
        esperantoTrace::et_trace
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0.0>>:stdc++fs>
)

# 
# compile specific launcher apps.
#

if (BUILD_TESTS)

add_executable(basic_launcher src/basic_launcher/Launcher.cpp)

target_link_libraries(basic_launcher
    PRIVATE
        etsoc_gpsdk
)

add_executable(hello_world_launcher src/hello_world/hello_world.cpp)
target_link_libraries(hello_world_launcher
    PRIVATE
        etsoc_gpsdk
)

add_executable(saxpy_launcher src/saxpy/saxpy.cpp)
target_include_directories(saxpy_launcher PRIVATE ${DEVICE_KERNELS_DIR}/saxpy)
target_link_libraries(saxpy_launcher
    PRIVATE
        etsoc_gpsdk
)

add_executable(sdot_launcher src/sdot/sdot.cpp)
target_include_directories(sdot_launcher PRIVATE ${DEVICE_KERNELS_DIR}/sdot)
target_link_libraries(sdot_launcher
    PRIVATE
        etsoc_gpsdk
)

add_executable(barrier_launcher src/barrier/barrier.cpp)
target_include_directories(barrier_launcher PRIVATE ${DEVICE_KERNELS_DIR}/barrier) 
target_link_libraries(barrier_launcher
    PRIVATE
        etsoc_gpsdk
)

add_executable(fft_launcher src/fft_launcher/FFTLauncher.cpp)
target_include_directories(fft_launcher PRIVATE ${DEVICE_KERNELS_DIR}/fftKernel/include)
target_compile_definitions(fft_launcher PRIVATE FFT_VERIFICATION)
target_link_libraries(fft_launcher 
    PRIVATE
        etsoc_gpsdk
	${FFTW3F_LIBRARIES}
)

add_executable(multiKernel_launcher src/multi_kernel/multiKernel.cpp)
target_include_directories(multiKernel_launcher PRIVATE ${DEVICE_KERNELS_DIR}/saxpy)
target_link_libraries(multiKernel_launcher
    PRIVATE
        etsoc_gpsdk
)

add_executable(txfma_launcher src/txfma_launcher/txfma_launcher.cpp)
target_include_directories(txfma_launcher PRIVATE ${DEVICE_KERNELS_DIR}/txfma)
target_link_libraries(txfma_launcher
    PRIVATE
        etsoc_gpsdk
)

add_executable(multiDevice_launcher src/multi_device/multiDevice.cpp)
target_include_directories(multiDevice_launcher PRIVATE ${DEVICE_KERNELS_DIR}/saxpy)
target_link_libraries(multiDevice_launcher
    PRIVATE
        etsoc_gpsdk
)

add_executable(multiDeviceHang_launcher src/multi_device/multiDeviceHang.cpp)
target_link_libraries(multiDeviceHang_launcher
    PRIVATE
        etsoc_gpsdk
)

add_executable(trapping_launcher src/trapping/trapping.cpp)
target_link_libraries(trapping_launcher
    PRIVATE
        etsoc_gpsdk
)

add_executable(matmul_launcher src/autogen_matmul/autogen_matmul.cpp)
target_include_directories(matmul_launcher PRIVATE ${DEVICE_KERNELS_DIR}/autogenMatmul)
target_link_libraries(matmul_launcher
    PRIVATE
        etsoc_gpsdk
)


add_executable(p2p_launcher src/p2p_launcher/p2p_launcher.cpp)
target_link_libraries(p2p_launcher
    PRIVATE
        etsoc_gpsdk
)


add_executable(exhaustive_cast_launcher src/exhaustive_cast/exhaustive_cast.cpp)
target_include_directories(exhaustive_cast_launcher PRIVATE ${DEVICE_KERNELS_DIR}/exhaustive_cast)
target_link_libraries(exhaustive_cast_launcher
    PRIVATE
        etsoc_gpsdk
)

add_executable(stack_launcher src/custom_stack/user_defined_stack_launcher.cpp)
target_include_directories(stack_launcher PRIVATE ${DEVICE_KERNELS_DIR}/custom_stack)
target_link_libraries(stack_launcher
    PRIVATE
        etsoc_gpsdk
)

add_executable(star_scratchpad_proof src/star_scratchpad_proof/star_scratchpad_proof.cpp)
target_link_libraries(star_scratchpad_proof
    PRIVATE
        etsoc_gpsdk
)

add_executable(shire_latency_probe src/topology_probe/shire_latency_probe.cpp)
target_link_libraries(shire_latency_probe
    PRIVATE
        etsoc_gpsdk
)

add_executable(nested_scratchpad_demo src/nested_scratchpad_demo/nested_scratchpad_demo.cpp)
target_link_libraries(nested_scratchpad_demo
    PRIVATE
        etsoc_gpsdk
)

add_executable(nested_scratchpad_stencil_demo src/nested_scratchpad_stencil_demo/nested_scratchpad_stencil_demo.cpp)
target_link_libraries(nested_scratchpad_stencil_demo
    PRIVATE
        etsoc_gpsdk
)

add_executable(nested_scratchpad_atomic_reads_demo src/nested_scratchpad_atomic_reads_demo/nested_scratchpad_atomic_reads_demo.cpp)
target_link_libraries(nested_scratchpad_atomic_reads_demo
    PRIVATE
        etsoc_gpsdk
)

add_executable(nested_scratchpad_chimera_gemv_demo src/nested_scratchpad_chimera_gemv_demo/nested_scratchpad_chimera_gemv_demo.cpp)
target_link_libraries(nested_scratchpad_chimera_gemv_demo
    PRIVATE
        etsoc_gpsdk
)

add_executable(erbium_sim_access_violation_demo src/erbium_sim_access_violation_demo/erbium_sim_access_violation_demo.cpp)
target_link_libraries(erbium_sim_access_violation_demo
    PRIVATE
        etsoc_gpsdk
)

add_executable(star_scratchpad_boundary_violation_demo src/star_scratchpad_boundary_violation_demo/star_scratchpad_boundary_violation_demo.cpp)
target_link_libraries(star_scratchpad_boundary_violation_demo
    PRIVATE
        etsoc_gpsdk
)

add_executable(scp_exec_probe_demo src/scp_exec_probe_demo/scp_exec_probe_demo.cpp)
target_include_directories(scp_exec_probe_demo PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../../esperanto-tools-libs/include)
target_link_libraries(scp_exec_probe_demo
    PRIVATE
        etsoc_gpsdk
        ${CMAKE_CURRENT_LIST_DIR}/../../../build/esperanto-tools-libs-prefix/src/esperanto-tools-libs-build/libetrt.so
)

add_executable(fixed_lpddr_exec_probe_demo src/fixed_lpddr_exec_probe_demo/fixed_lpddr_exec_probe_demo.cpp)
target_include_directories(fixed_lpddr_exec_probe_demo PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../../esperanto-tools-libs/include)
target_link_libraries(fixed_lpddr_exec_probe_demo
    PRIVATE
        etsoc_gpsdk
        ${CMAKE_CURRENT_LIST_DIR}/../../../build/esperanto-tools-libs-prefix/src/esperanto-tools-libs-build/libetrt.so
)

install(TARGETS basic_launcher etsoc_gpsdk saxpy_launcher hello_world_launcher multiKernel_launcher multiDevice_launcher multiDeviceHang_launcher trapping_launcher p2p_launcher exhaustive_cast_launcher stack_launcher star_scratchpad_proof shire_latency_probe nested_scratchpad_demo nested_scratchpad_stencil_demo nested_scratchpad_atomic_reads_demo nested_scratchpad_chimera_gemv_demo erbium_sim_access_violation_demo star_scratchpad_boundary_violation_demo scp_exec_probe_demo fixed_lpddr_exec_probe_demo
    RUNTIME       DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Erbium-flavored launcher (single binary, drives any erbium kernel ELF).
# Self-contained CMakeLists since it has its own dep set and doesn't share
# the etsoc_gpsdk / GenericLauncher scaffolding.
add_subdirectory(src/erbium_launcher)

endif()
