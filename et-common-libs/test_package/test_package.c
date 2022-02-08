#if defined(TEST_PACKAGE_SP_BL)

// sp_bl1
#include <etsoc/drivers/pmu/pmu.h>
#include <etsoc/isa/io.h>
#include <etsoc/isa/atomic.h>
//#include <etsoc/isa/atomic-impl.h> clashes with atomic.h
#include <etsoc/isa/etsoc_memory.h>
#include <etsoc/isa/esr_defines.h>
#include <etsoc/isa/macros.h>
#include <etsoc/isa/fcc.h>
#include <etsoc/isa/flb.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/sync.h>
#include <etsoc/isa/cacheops.h>
#include <etsoc/isa/riscv_encoding.h>
#include <etsoc/isa/utils.h>
#include <system/layout.h>
#include <system/etsoc_ddr_region_map.h>

#include <etsoc/drivers/serial/serial.h>

// sp_bl2
#include <etsoc/common/common_defs.h>
#include <etsoc/common/log_common.h>
// #include <etsoc/common/utils.h> // includes trace/trace_umode_intern.h but umode trace headers aren't shipped with sp_bl2
#include <etsoc/drivers/pcie/pcie_int.h>
#include <etsoc/drivers/pmu/pmu.h>
#include <etsoc/isa/atomic.h>
//#include <etsoc/isa/atomic-impl.h> clashes with atomic.h
#include <etsoc/isa/etsoc_memory.h>
#include <etsoc/isa/io.h>
#include <etsoc/isa/cacheops.h>
#include <etsoc/isa/esr_defines.h>
#include <etsoc/isa/fcc.h>
#include <etsoc/isa/utils.h>
//#include <transports/vq/vq.h>  // commented out due 'etsoc_rt_memory.h:53:2: error: #error "Definition for device runtime memory access not provided!"'
#include <transports/circbuff/circbuff.h>
#include <transports/sp_mm_iface/sp_mm_comms_spec.h>
#include <transports/mm_cm_iface/message_types.h>
//#include <transports/sp_mm_iface/sp_mm_iface.h> // commented out due 'etsoc_rt_memory.h:53:2: error: #error "Definition for device runtime memory access not provided!"'
#include <transports/sp_mm_iface/sp_mm_shared_config.h>
#include <system/layout.h>
#include <system/etsoc_ddr_region_map.h>

#include <etsoc/drivers/serial/serial.h>
//#include <etsoc/isa/etsoc_rt_memory.h>

#elif defined(TEST_PACKAGE_CM_UMODE)

#include <etsoc/isa/atomic.h>
//#include <etsoc/isa/atomic-impl.h> clashes with atomic.h
#include <etsoc/isa/barriers.h>
//#include <etsoc/isa/cacheops.h> clases with cacheops-umode.h
#include <etsoc/isa/cacheops-umode.h>
#include <etsoc/isa/esr_defines.h>
#include <etsoc/isa/fcc.h>
#include <etsoc/isa/flb.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/syscall.h>
#include <etsoc/isa/tensors.h>
#include <etsoc/isa/utils.h>
#include <trace/trace_umode.h>
#include <trace/trace_umode_cb.h>
#include <trace/trace_umode_intern.h>

#include <etsoc/common/utils.h>
#include <etsoc/drivers/pmu/pmu.h>

#elif defined(TEST_PACKAGE_MINION_BL)

#include <etsoc/drivers/pmu/pmu.h>
#include <etsoc/isa/atomic.h>
//#include <etsoc/isa/atomic-impl.h> clashes with atomic.h
#include <etsoc/isa/etsoc_memory.h>
#include <etsoc/isa/esr_defines.h>
#include <etsoc/isa/macros.h>
#include <etsoc/isa/fcc.h>
#include <etsoc/isa/flb.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/io.h>
#include <etsoc/isa/sync.h>
#include <etsoc/isa/cacheops.h>
#include <etsoc/isa/syscall.h>
#include <etsoc/isa/riscv_encoding.h>
#include <etsoc/isa/utils.h>
#include <system/etsoc_ddr_region_map.h>
#include <system/layout.h>

#include <transports/mm_cm_iface/broadcast.h>

#elif defined(TEST_PACKAGE_MM_RT_SVCS)

#include <etsoc/common/common_defs.h>
#include <etsoc/common/log_common.h>
#include <etsoc/isa/atomic.h>
//#include <etsoc/isa/atomic-impl.h> clashes with atomic.h
#include <etsoc/isa/cacheops.h>
#include <etsoc/isa/esr_defines.h>
#include <etsoc/isa/etsoc_memory.h>
#include <etsoc/isa/fcc.h>
#include <etsoc/isa/flb.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/io.h>
#include <etsoc/isa/macros.h>
#include <etsoc/isa/sync.h>
#include <etsoc/isa/syscall.h>
#include <etsoc/isa/riscv_encoding.h>
#include <etsoc/isa/utils.h>
#include <system/etsoc_ddr_region_map.h>
#include <system/layout.h>

#include <common/printf.h>
#include <etsoc/drivers/serial/serial.h>
#include <etsoc/drivers/pcie/pcie_int.h>
#include <etsoc/drivers/pcie/pcie_device.h>
#include <etsoc/drivers/pmu/pmu.h>
#include <etsoc/isa/etsoc_rt_memory.h>
#include <transports/circbuff/circbuff.h>
#include <transports/vq/vq.h>
#include <transports/mm_cm_iface/broadcast.h>
#include <transports/mm_cm_iface/message_types.h>
#include <transports/sp_mm_iface/sp_mm_comms_spec.h>
#include <transports/sp_mm_iface/sp_mm_iface.h>
#include <transports/sp_mm_iface/sp_mm_shared_config.h>

#elif defined(TEST_PACKAGE_CM_RT_SVCS)

#include <etsoc/common/common_defs.h>
#include <etsoc/common/log_common.h>
#include <etsoc/isa/atomic.h>
//#include <etsoc/isa/atomic-impl.h> clashes with atomic.h
#include <etsoc/isa/esr_defines.h>
#include <etsoc/isa/fcc.h>
#include <etsoc/isa/flb.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/io.h>
#include <etsoc/isa/etsoc_memory.h>
#include <etsoc/isa/macros.h>
#include <etsoc/isa/cacheops.h>
#include <etsoc/isa/sync.h>
#include <etsoc/isa/syscall.h>
#include <etsoc/isa/riscv_encoding.h>
#include <etsoc/isa/utils.h>
#include <system/etsoc_ddr_region_map.h>
#include <system/layout.h>

#include <common/printf.h>
#include <transports/circbuff/circbuff.h>
#include <etsoc/drivers/pmu/pmu.h>
#include <etsoc/drivers/serial/serial.h>
#include <transports/mm_cm_iface/message_types.h>

#else
#error "we should not reach here.."
#endif

int main()
{
    return 0;
}