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
#include "Tracing/Tracing.h"
#include "esperanto/runtime/CodeManagement/CodeRegistry.h"
#include "esperanto/runtime/Core/Device.h"
#include "esperanto/runtime/Support/Logging.h"

namespace et_runtime {

UberKernel::UberKernelLaunch::UberKernelLaunch(
    const UberKernel &kernel, std::vector<std::vector<Kernel::LaunchArg>> &args)
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

etrtError UberKernel::UberKernelLaunch::launchBlocking(Device *dev,
                                                       Stream *stream) {

  // Make sure that the ELF is on the device, if not load it
  auto module_id = kernel_.moduleID();
  auto module = CodeRegistry::registry().getModule(module_id);
  assert(module != nullptr);

  if (!module->onDevice()) {
    return etrtErrorModuleNotOnDevice;
  }

  uintptr_t kernel_entry_point;
  assert(module->rawKernelExists(kernel_.name()));

  auto entry_point_res = module->onDeviceKernelEntryPoint(kernel_.name());
  assert(entry_point_res);

  // Find the entrypoint function to call on the ELF
  kernel_entry_point = entry_point_res.get();

  // Allocate tensor on the device that will hold the arguments of the uber
  // kernel
  void *uber_kernel_args = nullptr;
  // Allocate the buffer on the device and get a pointer to that buffer
  auto alloc_status =
      dev->mem_manager().malloc(&uber_kernel_args, arg_vals_.size());

  if (alloc_status != etrtSuccess) {
    return alloc_status;
  }

  // Copy the UberKernel arguments to the device
  auto memcpy_res = dev->memcpy(uber_kernel_args, arg_vals_.data(),
                                arg_vals_.size(), etrtMemcpyHostToDevice);
  if (memcpy_res != etrtSuccess) {
    return memcpy_res;
  }

  Kernel::layer_dynamic_info_t layer_info = {0};
  layer_info.tensor_a = reinterpret_cast<uint64_t>(uber_kernel_args);

  auto args_size = sizeof(layer_info);
  std::vector<uint8_t> args_buff(args_size);
  ::memcpy(&args_buff[0], &layer_info, sizeof(layer_info));

  dev->addCommand(
      stream,
      std::shared_ptr<device_api::CommandBase>(new device_api::LaunchCommand(
          kernel_entry_point, args_buff, kernel_.name())));

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
