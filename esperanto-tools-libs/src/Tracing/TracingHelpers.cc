/*------------------------------------------------------------------------------
 * Copyright (C) 2019, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

#include "TracingHelpers.h"

#include "Tracing/Tracing.h"
#include "esperanto/runtime/CodeManagement/Kernel.h"

std::ostream &operator<<(std::ostream &os,
                         const std::vector<unsigned char> &vec) {
  for (auto &i : vec) {
    os << std::hex << (unsigned int) i << ", ";
  }
  return os;
}

template <>
std::vector<et_runtime::tracing::CodeManager_kernel_argument_type>
conv_vec<et_runtime::Kernel::ArgType,
         et_runtime::tracing::CodeManager_kernel_argument_type>(
    const std::vector<et_runtime::Kernel::ArgType> &src) {
  std::vector<et_runtime::tracing::CodeManager_kernel_argument_type> res(
      src.size());
  for (unsigned int i = 0; i < src.size(); i++) {
    res[i] = static_cast<et_runtime::tracing::CodeManager_kernel_argument_type>(
        static_cast<int>(src[i]));
  }
  return res;
}

// @brief converting the argument vector to the tracing type
template <>
std::vector<et_runtime::tracing::CodeManager_kernel_argument>
conv_vec<et_runtime::Kernel::LaunchArg,
         et_runtime::tracing::CodeManager_kernel_argument>(
    const std::vector<et_runtime::Kernel::LaunchArg> &src) {

  std::vector<et_runtime::tracing::CodeManager_kernel_argument> res(src.size());

  for (unsigned int i = 0; i < src.size(); i++) {
    auto &arg = src[i];
    et_runtime::tracing::CodeManager_kernel_argument dst;
    switch (arg.type) {
    case et_runtime::Kernel::ArgType::T_int8:
      dst.set_int8(arg.value.int8);
      break;
    case et_runtime::Kernel::ArgType::T_uint8:
      dst.set_uint8(arg.value.uint8);
      break;
    case et_runtime::Kernel::ArgType::T_int16:
      dst.set_int16(arg.value.int16);
      break;
    case et_runtime::Kernel::ArgType::T_uint16:
      dst.set_uint16(arg.value.uint16);
      break;
    case et_runtime::Kernel::ArgType::T_int32:
      dst.set_int32(arg.value.int32);
      break;
    case et_runtime::Kernel::ArgType::T_uint32:
      dst.set_uint32(arg.value.uint32);
      break;
    case et_runtime::Kernel::ArgType::T_float:
      dst.set_vfloat(arg.value.vfloat);
      break;
    case et_runtime::Kernel::ArgType::T_double:
      dst.set_vdouble(arg.value.vdouble);
      break;
    case et_runtime::Kernel::ArgType::T_tensor:
      abort();
      break;
    case et_runtime::Kernel::ArgType::T_layer_dynamic_info: {
      auto dyn_info =
          new ::et_runtime::tracing::CodeManager_layer_dynamic_info_t();
      dyn_info->set_tensor_a(arg.value.layer_dynamic_info.tensor_a);
      dyn_info->set_tensor_b(arg.value.layer_dynamic_info.tensor_b);
      dyn_info->set_tensor_c(arg.value.layer_dynamic_info.tensor_c);
      dyn_info->set_tensor_d(arg.value.layer_dynamic_info.tensor_d);
      dyn_info->set_tensor_e(arg.value.layer_dynamic_info.tensor_e);
      dyn_info->set_tensor_f(arg.value.layer_dynamic_info.tensor_f);
      dyn_info->set_tensor_g(arg.value.layer_dynamic_info.tensor_g);
      dyn_info->set_tensor_h(arg.value.layer_dynamic_info.tensor_h);
      dyn_info->set_kernel_id(arg.value.layer_dynamic_info.kernel_id);
      dst.set_allocated_layer_dynamic_info(dyn_info);
    } break;
    default:
      break;
    }
    res[i] = dst;
  }
  return res;
}
