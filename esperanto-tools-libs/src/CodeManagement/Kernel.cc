//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/CodeManagement/Kernel.h"
#include "esperanto/runtime/CodeManagement/CodeRegistry.h"
#include "CodeManagement/CodeModule.h"

#include "DeviceAPI/Commands.h"
#include "Tracing/Tracing.h"
#include "Tracing/TracingHelpers.h"
#include "esperanto/runtime/Common/CommonTypes.h"
#include "esperanto/runtime/Core/Device.h"
#include "esperanto/runtime/Support/ErrorOr.h"
#include "esperanto/runtime/Support/Logging.h"

namespace et_runtime {

KernelCodeID Kernel::global_kernel_id_ = 1;
Kernel::KernelMap Kernel::kernel_registry_;

static const char *arg_type_names[(int)Kernel::ArgType::Num] = {
    [(int)Kernel::ArgType::None] = "Unknwon",
    [(int)Kernel::ArgType::T_int8] = "int8",
    [(int)Kernel::ArgType::T_uint8] = "uint8",
    [(int)Kernel::ArgType::T_int16] = "int16",
    [(int)Kernel::ArgType::T_uint16] = "uint16",
    [(int)Kernel::ArgType::T_int32] = "int32",
    [(int)Kernel::ArgType::T_uint32] = "uint32",
    [(int)Kernel::ArgType::T_float] = "float",
    [(int)Kernel::ArgType::T_double] = "double",
    [(int)Kernel::ArgType::T_tensor] = "tensor",
    [(int)Kernel::ArgType::T_layer_dynamic_info] = "layer_dynamic_info",
};

Kernel::KernelLaunch::KernelLaunch(const Kernel &kernel,
                                   std::vector<LaunchArg> &args)
    : kernel_(kernel), args_(args) {
  const auto &arg_types = kernel_.kernelArgs();
  if (arg_types.size() != args_.size()) {
    RTERROR << "Kernel Launch, argument number " << args_.size()
            << " does not match expected number " << arg_types.size();
    std::terminate();
  }
  for (unsigned i = 0; i < args_.size(); i++) {
    if (arg_types[i] != args_[i].type) {
      assert(arg_types[i] < ArgType::Num && args_[i].type < ArgType::Num);
      RTERROR << "Kernel parameter type mispatch,  expected: "
              << arg_type_names[(int)arg_types[i]]
              << " received: " << arg_type_names[(int)args_[i].type];
      std::terminate();
    }
  }

  TRACE_CodeManager_kernel_launch(kernel_.id(), kernel_.name(), args);
}

etrtError Kernel::KernelLaunch::launchBlocking(Device *dev, Stream *stream) {
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

  kernel_entry_point = entry_point_res.get();

  auto args_size = sizeof(Kernel::layer_dynamic_info_t);
  std::vector<uint8_t> args_buff(args_size);
  ::memcpy(&args_buff[0], &args_[0].value.layer_dynamic_info, args_size);

  dev->addCommand(
      dev->getStream(stream),
      std::shared_ptr<device_api::CommandBase>(new device_api::LaunchCommand(
          kernel_entry_point, args_buff, kernel_.name())));

  return etrtSuccess;
}

/// @brief Non blocking kernel launch
/// FIXME SW-1382
etrtError Kernel::KernelLaunch::launchNonBlocking(Stream *stream) {
  std::terminate();
  return etrtSuccess;
}

Kernel::Kernel(CodeModuleID mid) : id_(global_kernel_id_++), mid_(mid) {}

Kernel::Kernel(const ::std::string &name, const std::vector<ArgType> &arg_list,
               CodeModuleID mid)
    : Kernel(mid) {
  name_ = name;
  kernel_args_ = arg_list;
  for (auto &i : arg_list) {
    if (i != ArgType::T_layer_dynamic_info) {
      // FIXME SW-1383
      RTERROR << "Currently only the layer_dynamic_info data struct is "
                 "supported as a kernel argument";
      std::terminate();
    }
  }
  auto success = registerKernel(name, this);
  if (!success) {
    RTERROR << "Kernel by name: " << name << " already exists \n";
    std::terminate();
  }
  TRACE_CodeManager_create_kernel(id_, name_, kernel_args_);
}

bool Kernel::registerKernel(const std::string &name, Kernel *kernel) {
  auto it = kernel_registry_.find(name);
  if (it != kernel_registry_.end()) {
    return false;
  }
  kernel_registry_[name] = {kernel->id(), std::unique_ptr<Kernel>(kernel)};
  return true;
}

ErrorOr<Kernel &> Kernel::findKernel(KernelCodeID id) {
  auto elem = std::find_if(kernel_registry_.begin(), kernel_registry_.end(),
                           [&id](decltype(kernel_registry_)::value_type &e) {
                             return std::get<0>(std::get<1>(e)) == id;
                           });
  if (elem == kernel_registry_.end()) {
    return etrtErrorInvalidKernelImage;
  }
  return *std::get<1>(elem->second).get();
}

std::unique_ptr<Kernel::KernelLaunch>
Kernel::createKernelLaunch(std::vector<LaunchArg> &args) {
  return std::make_unique<Kernel::KernelLaunch>(*this, args);
}

} // namespace et_runtime

std::ostream &operator<<(std::ostream &os,
                         const et_runtime::Kernel::LaunchArg &arg) {
  os << " .type=" << et_runtime::arg_type_names[(int)arg.type];
  switch (arg.type) {
  case et_runtime::Kernel::ArgType::T_int8:
    os << " .value " << arg.value.int8;
    break;
  case et_runtime::Kernel::ArgType::T_uint8:
    os << " .value " << arg.value.uint8;
    break;
  case et_runtime::Kernel::ArgType::T_int16:
    os << " .value " << arg.value.int16;
    break;
  case et_runtime::Kernel::ArgType::T_uint16:
    os << " .value " << arg.value.uint16;
    break;
  case et_runtime::Kernel::ArgType::T_int32:
    os << " .value " << arg.value.int32;
    break;
  case et_runtime::Kernel::ArgType::T_uint32:
    os << " .value " << arg.value.uint32;
    break;
  case et_runtime::Kernel::ArgType::T_float:
    os << " .value " << arg.value.vfloat;
    break;
  case et_runtime::Kernel::ArgType::T_double:
    os << " .value " << arg.value.vdouble;
    break;
  case et_runtime::Kernel::ArgType::T_tensor:
    os << " .value Fix Tensor";
    break;
  case et_runtime::Kernel::ArgType::T_layer_dynamic_info:
    os << std::hex << " tensor_a = 0x" << arg.value.layer_dynamic_info.tensor_a
       << " tensor_b = 0x" << arg.value.layer_dynamic_info.tensor_b
       << " tensor_c = 0x" << arg.value.layer_dynamic_info.tensor_c
       << " tensor_d = 0x" << arg.value.layer_dynamic_info.tensor_d
       << " tensor_e = 0x" << arg.value.layer_dynamic_info.tensor_e
       << " tensor_f = 0x" << arg.value.layer_dynamic_info.tensor_f
       << " tensor_g = 0x" << arg.value.layer_dynamic_info.tensor_g
       << " tensor_h = 0x" << arg.value.layer_dynamic_info.tensor_h
       << " pc/id    = 0x" << arg.value.layer_dynamic_info.kernel_id;
    break;
  default:
    os << "Unknown Type";
    break;
  }

  return os;
}

/// @brief Helper overload of operator<< that will allow us to print
/// a vector of kenrel laucnh arguments
std::ostream &
operator<<(std::ostream &os,
           const std::vector<et_runtime::Kernel::LaunchArg> &vec) {
  for (unsigned int i = 0; i < vec.size(); i++) {
    os << "[" << i << "] = " << vec[i] << " ,";
  }
  return os;
}
