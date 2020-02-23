#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

# Rules for building external cmake dependencies
# FIXME add support for building the the crypto library SW-1772

## Firmware Code signing tools
# Find the package or else build it from source

find_package(EsperantoCodeSigningTools CONFIG REQUIRED)
message("Code Signing Tools Found: ${EsperantoCodeSigningTools_FOUND}")

get_target_property(CRYPTO_HEADERS_INTERFACE_PATH crypto_api INTERFACE_INCLUDE_DIRECTORIES)
set(CRYPTO_HEADERS ${CRYPTO_HEADERS_INTERFACE_PATH} PARENT_SCOPE)
