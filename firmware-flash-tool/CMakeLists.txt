cmake_minimum_required (VERSION 2.6)
project (crypto_api)
#set (EXTRA_LIBS ${EXTRA_LIBS} json)

set(CMAKE_BUILD_TYPE Debug)

find_library(JSON_LIB json-c HINTS OPENSSL_DIR)
if(NOT JSON_LIB)
  message(FATAL_ERROR "libjson-c library not found")
endif()

file(MAKE_DIRECTORY build)

# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_BINARY_DIR}/include")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g \
-Wall -Wextra -Werror -Wnull-dereference \
-Wduplicated-branches -Wduplicated-cond -Wshadow -Wpointer-arith -Wundef \
-Wbad-function-cast -Wcast-qual -Wcast-align -Wconversion -Wlogical-op \
-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations")

ADD_EXECUTABLE( esperanto_flash_tool src/esperanto_flash_tool.c src/parse_template_file.c )
target_link_libraries (esperanto_flash_tool json-c)
#set_target_properties (esperanto_flash_tool PROPERTIES OUTPUT_NAME tools/esperanto_generate_certificate_request)
