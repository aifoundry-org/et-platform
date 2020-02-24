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
# FIXME add support for building the crypto library SW-1772

### Crypto API: handle the case where the library comes from either the
### CryptoAPI package or the EsperantoCodesigningTools package

find_package(EsperantoCryptoAPI)
if(EsperantoCryptoAPI_FOUND)

  message("Crypto API Found: ${EsperantoCryptoAPI_FOUND}")
  get_target_property(CRYPTO_HEADERS_INTERFACE_PATH crypto_api INTERFACE_INCLUDE_DIRECTORIES)
  set(CRYPTO_HEADERS ${CRYPTO_HEADERS_INTERFACE_PATH} PARENT_SCOPE)
  # set the variable in the rest of our scripts
  set(EsperantoCryptoAPI_FOUND ${EsperantoCryptoAPI_FOUND} PARENT_SCOPE)

else()

  find_package(EsperantoCodeSigningTools CONFIG REQUIRED)
  message("Code Signing Tools Found: ${EsperantoCodeSigningTools_FOUND}")
  get_target_property(CRYPTO_HEADERS_INTERFACE_PATH crypto_api INTERFACE_INCLUDE_DIRECTORIES)
  set(CRYPTO_HEADERS ${CRYPTO_HEADERS_INTERFACE_PATH} PARENT_SCOPE)

endif()
