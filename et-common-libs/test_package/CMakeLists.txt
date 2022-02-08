cmake_minimum_required(VERSION 3.8)
project(test_package C)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(WITH_SP_BL "" OFF)
option(WITH_CM_UMODE "" OFF)
option(WITH_MINION_BL "" ON)
option(WITH_MM_RT_SVCS "" ON)
option(WITH_CM_RT_SVCS "" ON)

set(REQUIRED_COMPONENTS)

if (WITH_SP_BL)
   list(APPEND REQUIRED_COMPONENTS "sp_bl1:sp_bl2")
endif()

if (WITH_CM_UMODE)
    list(APPEND REQUIRED_COMPONENTS "cm-umode")
endif()

if (WITH_MINION_BL)
    list(APPEND REQUIRED_COMPONENTS "minion-bl")
endif()

if (WITH_MM_RT_SVCS)
    list(APPEND REQUIRED_COMPONENTS "mm-rt-svcs")
endif()

if (WITH_CM_RT_SVCS)
    list(APPEND REQUIRED_COMPONENTS "cm-rt-svcs")
endif()


find_package(et-common-libs REQUIRED COMPONENTS ${REQUIRED_COMPONENTS})

if (WITH_SP_BL)
    add_executable(${PROJECT_NAME}_sp_bl test_package.c)
    target_compile_definitions(${PROJECT_NAME}_sp_bl PRIVATE TEST_PACKAGE_SP_BL)
    target_link_libraries(${PROJECT_NAME}_sp_bl PRIVATE et-common-libs::sp_bl1 et-common-libs::sp_bl2)
endif()


if (WITH_CM_UMODE)
    add_executable(${PROJECT_NAME}_cm_umode test_package.c)
    target_compile_definitions(${PROJECT_NAME}_cm_umode PRIVATE TEST_PACKAGE_CM_UMODE)
    target_link_libraries(${PROJECT_NAME}_cm_umode PRIVATE et-common-libs::cm-umode)
endif()

if (WITH_MINION_BL)
    add_executable(${PROJECT_NAME}_minion_bl test_package.c)
    target_compile_definitions(${PROJECT_NAME}_minion_bl PRIVATE TEST_PACKAGE_MINION_BL)
    target_link_libraries(${PROJECT_NAME}_minion_bl PRIVATE et-common-libs::minion-bl)
endif()

if (WITH_MM_RT_SVCS)
    add_executable(${PROJECT_NAME}_mm_rt_svcs test_package.c)
    target_compile_definitions(${PROJECT_NAME}_mm_rt_svcs PRIVATE TEST_PACKAGE_MM_RT_SVCS)
    target_link_libraries(${PROJECT_NAME}_mm_rt_svcs PRIVATE et-common-libs::mm-rt-svcs)
endif()

if (WITH_CM_RT_SVCS)
    add_executable(${PROJECT_NAME}_cm_rt_svcs test_package.c)
    target_compile_definitions(${PROJECT_NAME}_cm_rt_svcs PRIVATE TEST_PACKAGE_CM_RT_SVCS)
    target_link_libraries(${PROJECT_NAME}_cm_rt_svcs PRIVATE et-common-libs::cm-rt-svcs)
endif()
