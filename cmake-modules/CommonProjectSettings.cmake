### Require out-of-source builds (exclude conan package creation from this)
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if (EXISTS "${LOC_PATH}")
    message(STATUS "You should not build in a source directory (or any directory with a CMakeLists.txt file)."
            "Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

###
### Restrict build types and set default build type to RelWithDebInfo if none was specified
###
### USERS of this module are expected to define:
###   - 'option(ENABLE_STRICT_BUILD_TYPES "" TRUE)'
### USERS of this module can override
###   - ET_DEFAULT_BUILD_TYPE (defaults to RelWithDebInfo)
if (ENABLE_STRICT_BUILD_TYPES)
    set(allowedBuildTypes Debug Release RelWithDebInfo MinSizeRel)

    if (NOT DEFINED ET_DEFAULT_BUILD_TYPE)
        message("* ET_DEFAULT_BUILD_TYPE not defined. Setting RelWithDebInfo as default value.")
	set(ET_DEFAULT_BUILD_TYPE RelWithDebInfo)
    endif()
    
    get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG) # CMAKE > 3.9
    if (isMultiConfig)
        if (NOT ET_DEFAULT_BUILD_TYPE IN_LIST CMAKE_CONFIGURATION_TYPES)
            if (ET_DEFAULT_BUILD_TYPE IN_LIST allowedBuildTypes)
                list(APPEND CMAKE_CONFIGURATION_TYPES ${ET_DEFAULT_BUILD_TYPE})
            endif()
        endif()
    else()
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${allowedBuildTypes})

        if (NOT CMAKE_BUILD_TYPE)
		message(STATUS "* No CMAKE_BUILD_TYPE selected. Setting value to default (${ET_DEFAULT_BUILD_TYPE})")
            set(CMAKE_BUILD_TYPE ${ET_DEFAULT_BUILD_TYPE} CACHE STRING "Choose the type of build" FORCE)
        elseif(NOT CMAKE_BUILD_TYPE IN_LIST allowedBuildTypes)
            message(FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE}. Only these are allowed: ${allowedBuildTypes}")
        endif()
    endif()
endif()

# Generate compile_commands.json to make it easier to work with clang based tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (ENABLE_IPO)
    cmake_policy(SET CMP0069 NEW)

    include(CheckIPOSupported)
    check_ipo_supported(RESULT result OUTPUT output)
    if (result)
        message(STATUS "* IPO (Link time optimizations) is enabled.")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(WARNING "IPO is not supported: ${output}.")
    endif()
else()
    message(STATUS "* IPO is disabled.")
endif()

if (ENABLE_UNITY)
    set(CMAKE_UNITY_BUILD ON)
    # Per-target alternaive:
    #set_target_properties(intro PROPERTIES UNITY_BUILD ON)
endif()
