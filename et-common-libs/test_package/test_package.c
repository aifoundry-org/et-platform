#include <etsoc/common/log_common.h>
#include <etsoc/common/utils.h>
#include <etsoc/drivers/pmu/pmu.h>

#include <etsoc/isa/atomic.h>
#include <etsoc/isa/barriers.h>
#include <etsoc/isa/cacheops-umode.h>
#include <etsoc/isa/esr_defines.h>
#include <etsoc/isa/fcc.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/syscall.h>
#include <etsoc/isa/tensors.h>
#include <etsoc/isa/utils.h>
#include <etsoc/isa/macros.h>

#include <trace/trace_umode.h>

#include <stdio.h>

int main() { 
    printf("SUCCESS\n");
    return 0;
}