//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/CodeManagement/UberKernel.h"

#include "CodeManagement/CodeModule.h"
#include "DeviceAPI/Commands.h"
#include "DeviceAPI/CommandsGen.h"
#include "Tracing/Tracing.h"
#include "esperanto/runtime/CodeManagement/CodeRegistry.h"
#include "esperanto/runtime/Core/CommandLineOptions.h"
#include "esperanto/runtime/Core/Device.h"
#include "esperanto/runtime/Support/Logging.h"

namespace et_runtime {

UberKernel::UberKernelLaunch::UberKernelLaunch(const UberKernel &kernel,
                                               const ArgValues &args)
    : kernel_(kernel) {
  std::vector<LaunchArg> flat_args; // flatten out the nested vectors
  unsigned int flat_args_bytes = 0;
  // Check that the argument list matches the registered signature from the
  // UberKernel
  const auto &exp_types = kernel.argList();
  if (exp_types.size() != args.size()) {
    RTERROR << "Incorrect number of UberKernel layers, expected: "
            << exp_types.size() << " received: " << args.size();
    std::terminate();
  }
  for (unsigned int i = 0; i < args.size(); i++) {
    auto &ref_vec = exp_types[i];
    auto &arg_vec = args[i];
    if (ref_vec.size() != arg_vec.size()) {
      RTERROR << "For layer: " << i
              << ", incorrect number of UberKernel layers, expected: "
              << ref_vec.size() << " received: " << arg_vec.size();
      std::terminate();
    }
    for (unsigned j = 0; j < arg_vec.size(); j++) {
      if (ref_vec[j] != arg_vec[j].type) {
        RTERROR << "For layer: " << i << ", incorrect argument " << j
                << " expected: " << argTypeToStr(ref_vec[j])
                << " received: " << argTypeToStr(arg_vec[j].type);
        std::terminate();
      }
      flat_args.push_back(arg_vec[j]);
      flat_args_bytes += argTypeSize(arg_vec[j].type);
    }
  }
  // Pack the Argumennts as necessary and extract them from the union
  for (auto &i : flat_args) {
    auto val_data = argBytes(i);
    arg_vals_.insert(arg_vals_.end(), val_data.begin(), val_data.end());
  }
  assert(arg_vals_.size() == flat_args_bytes);

  TRACE_CodeManager_uber_kernel_launch(kernel.id(), kernel.name(), flat_args,
                                       arg_vals_);
}

ErrorOr<std::shared_ptr<device_api::devfw_commands::KernelLaunchCmd>>
UberKernel::UberKernelLaunch::launchHelper(Stream *stream) {

  auto entry_res = kernel_.kernelEntryPoint(&stream->dev());
  if (entry_res.getError() != etrtSuccess) {
    return entry_res.getError();
  }

  uintptr_t kernel_entry_point = entry_res.get();

  // Allocate tensor on the device that will hold the arguments of the uber
  // kernel
  void *uber_kernel_args = nullptr;
  // Allocate the buffer on the device and get a pointer to that buffer
  auto alloc_status =
      stream->dev().mem_manager().malloc(&uber_kernel_args, arg_vals_.size());

  if (alloc_status != etrtSuccess) {
    return alloc_status;
  }

  // Copy the UberKernel arguments to the device
  auto memcpy_res =
      stream->dev().memcpy(uber_kernel_args, arg_vals_.data(), arg_vals_.size(),
                           etrtMemcpyHostToDevice);
  if (memcpy_res != etrtSuccess) {
    return memcpy_res;
  }

  ::device_api::non_privileged::dev_api_kernel_params_t params = {0};
  params.tensor_a = reinterpret_cast<uint64_t>(uber_kernel_args);

  // FIXME we should be querying the device-fw for that information first
  auto active_shires_opt = absl::GetFlag(FLAGS_shires);
  int active_shires = std::stoi(active_shires_opt);

  ::device_api::non_privileged::dev_api_kernel_info_t info = {
      .compute_pc = kernel_entry_point,
      .uber_kernel_nodes = 0,
      .shire_mask = (1ULL << active_shires) - 1,
  };
  // FIXME the following is a hack that should be eventually be cleaned with
  // proper support in the DeviceAPI When launching an uberkernel set the
  // specific bit of the shire mask
  info.shire_mask |= (1ULL << MASTER_SHIRE_NUM);

  auto launch_cmd =
      std::make_shared<device_api::devfw_commands::KernelLaunchCmd>(
          stream->id(), params, info, true);
  return launch_cmd;
}

etrtError UberKernel::UberKernelLaunch::launchBlocking(Stream *stream) {

  auto create_command_res = launchHelper(stream);
  if (create_command_res.getError() != etrtSuccess) {
    return create_command_res.getError();
  }
  auto launch_cmd = create_command_res.get();
  stream->addCommand(launch_cmd);

  auto response_future = launch_cmd->getFuture();
  auto response = response_future.get().response();
  assert(response.response_info.message_id ==
         ::device_api::MBOX_DEVAPI_NON_PRIVILEGED_MID_KERNEL_LAUNCH_RSP);

  auto &cmd_info = launch_cmd->cmd_info();
  assert(response.kernel_id == cmd_info.kernel_params.kernel_id);

  if (response.error ==
          ::device_api::non_privileged::DEV_API_KERNEL_LAUNCH_ERROR::
              DEV_API_KERNEL_LAUNCH_ERROR_OK ||
      response.error ==
          ::device_api::non_privileged::DEV_API_KERNEL_LAUNCH_ERROR::
              DEV_API_KERNEL_LAUNCH_ERROR_RESULT_OK) {
    RTDEBUG << "Received successfull launch \n";
  } else {
    assert(false);
  }

  return etrtSuccess;
}

etrtError UberKernel::UberKernelLaunch::launchNonBlocking(Stream *stream) {

  auto create_command_res = launchHelper(stream);
  if (create_command_res.getError() != etrtSuccess) {
    return create_command_res.getError();
  }
  auto launch_cmd = create_command_res.get();
  stream->addCommand(launch_cmd);

  return etrtSuccess;
}

UberKernel::UberKernel(const std::string &name,
                       const std::vector<std::vector<ArgType>> &arg_list,
                       CodeModuleID mid)
    : Kernel(mid) {
  name_ = name;
  arg_list_ = arg_list;
  auto success = registerKernel(name, this);
  if (!success) {
    RTERROR << "Kernel by name: " << name << " already exists \n";
    std::terminate();
  }

  TRACE_CodeManager_create_uber_kernel(id_, name);
}

ErrorOr<UberKernel &> UberKernel::findKernel(KernelCodeID id) {
  auto find_res = Kernel::findKernel(id);
  if (!(bool)find_res) {
    return find_res.getError();
  }
  return dynamic_cast<UberKernel &>(find_res.get());
}

} // namespace et_runtime
