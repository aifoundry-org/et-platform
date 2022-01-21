get_filename_component(SHARED_SRC_DIR "shared/common/src" ABSOLUTE)
get_filename_component(SHARED_INC_DIR "shared/common/include" ABSOLUTE)

add_subdirectory(shared)
add_subdirectory(test_framework)
add_subdirectory(BootromTrampolineToBL2)
add_subdirectory(ServiceProcessorBL1)
add_subdirectory(ServiceProcessorBL2)
