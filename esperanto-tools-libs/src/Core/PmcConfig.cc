//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/Core/PmcConfig.h"
#include "DeviceAPI/CommandsGen.h"
#include "DeviceAPI/ResponsesGen.h"
#include "esperanto/runtime/Core/Device.h"

namespace et_runtime {

bool
PmcConfig::set_pmc_config(uint64_t conf_buffer_addr) {

  auto pmc_config_cmd =
      std::make_shared<device_api::devfw_commands::ConfigurePmcsCmd>(0, conf_buffer_addr);

  dev_.defaultStream().addCommand(pmc_config_cmd);

  auto response_future = pmc_config_cmd->getFuture();
  auto response = response_future.get().response();

  assert(response.response_info.message_id ==
             ::device_api::MBOX_DEVAPI_NON_PRIVILEGED_MID_CONFIGURE_PMCS_RSP);

  return static_cast<bool>(response.status);
}


// Clearing / disabling PMC config is accomplished by sending a 0
// as configuration buffer address.
bool
PmcConfig::clear_pmc_config() {

  return set_pmc_config(0);
}

}  // namespace et-runtime
