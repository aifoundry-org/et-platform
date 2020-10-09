#include "esperanto/runtime/Core/Profiler.h"
#include "esperanto/runtime/Core/Device.h"
#include "Core/TraceHelper.h"
#include "Tracing/DeviceFWTrace.h"


namespace et_runtime {

Profiler::Profiler(Device &dev)
 : dev_(dev)
{
  constexpr uint32_t k_initial_trace_buffer_size = 8192;
  //TODO: Runtime should be able to resize trace buffer acording
  // to the current requirements. (larger buffers for long executions)

  TraceHelper trace_helper(dev_);

  trace_helper.prepare_trace_buffers();
  trace_helper.configure_trace_buffer_size_knob(k_initial_trace_buffer_size);
  trace_helper.reset_trace_buffers();
}

etrtError Profiler::start()
{
  TraceHelper trace_helper(dev_);

  auto success = trace_helper.configure_trace_state_knob(1);
  return success ? etrtError::etrtSuccess :  etrtError::etrtErrorDeviceConfig;
}

etrtError Profiler::stop() {
  TraceHelper trace_helper(dev_);

  auto success = trace_helper.configure_trace_state_knob(0);
  return success ? etrtError::etrtSuccess :  etrtError::etrtErrorDeviceConfig;
}

etrtError Profiler::flush() {
  TraceHelper trace_helper(dev_);

  ::device_api::non_privileged::discover_trace_buffer_rsp_t rsp = {0};
  rsp = trace_helper.discover_trace_buffer();
  if (!rsp.status) {
    return etrtError::etrtErrorDeviceConfig;
  }

  auto success = trace_helper.prepare_trace_buffers();
  if (!success) {
    return etrtError::etrtErrorDeviceConfig;
  }

  std::vector<uint8_t> data(rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS);
  {
    auto&& dst_ptr = data.data();
    auto&& src_ptr = reinterpret_cast<const void *>(rsp.trace_base + ALIGN(sizeof(::device_api::non_privileged::trace_control_t), TRACE_BUFFER_REGION_ALIGNEMNT));
    const size_t size = rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS;

    auto res = dev_.memcpy(dst_ptr, src_ptr, size, etrtMemcpyKind::etrtMemcpyDeviceToHost);
    if (res != etrtError::etrtSuccess)
    {
      return res;
    }    
  }
  
  auto result = tracing::DeviceAPI_DeviceFW_process_device_traces(&data[0], rsp.trace_buffer_size, NUMBER_OF_TRACE_BUFFERS);
  if (result != ::device_api::non_privileged::TRACE_STATUS_SUCCESS) {
    return etrtError::etrtErrorDeviceConfig;
  }

  return etrtError::etrtSuccess;
}

} // end et_runtime namespace