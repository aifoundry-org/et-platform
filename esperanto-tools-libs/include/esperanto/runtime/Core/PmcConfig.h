//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//-----------------------------------------------------------------------------

#ifndef ET_RUNTIME_PMC_CONFIG_H
#define ET_RUNTIME_PMC_CONFIG_H

#include <cstdint>

namespace et_runtime {

class Device;

/// @class PmcConfig PmcConfig.h esperanto/runtime/Core/PmcConfig.h
///
/// @brief Class that implements basic support to configure performance
/// monitoring counters (PMCs) with DeviceAPI
///
class PmcConfig {

public:
  /// @brierf PmcConfig constructor
  ///
  /// @param[in] dev Reference to the associate device
  PmcConfig(Device &dev) : dev_(dev){};

  ///
  /// @brief Send PmcConfig command to device
  ///
  /// @param[in] conf_buffer_addr: Address of the PMC configuration buffer
  /// @returns true on success, false on failure
  bool set_pmc_config(uint64_t conf_buffer_addr);

  ///
  /// @brief Clear PMC config / disable PMC collection
  ///
  /// @returns true on success, false on failure
  bool clear_pmc_config();

private:
  Device &dev_; ///< Device object, this class interacts with
};

} // namespace et_runtime

#endif
