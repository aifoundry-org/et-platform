/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "emu_defines.h"
#include "emu_gio.h"
#include "fpu/fpu.h"
#include "fpu/fpu_casts.h"
#include "gold.h"
#include "insn.h"
#include "insn_func.h"
#include "insn_util.h"
#include "log.h"
#include "processor.h"
#include "traps.h"
#include "utility.h"

namespace bemu {


// LCOV_EXCL_START
static inline float32_t fexp_vs_gold(const Hart& cpu, float32_t x)
{
    float32_t fpuval = fpu::f32_exp2(x);
    float32_t gldval = gld::f32_exp2(x);
    if (gld::security_ulp_check(gldval.v, fpuval.v)) {
        WARN_HART(trans, cpu, "FEXP mismatch with input: 0x%08x golden: 0x%08x libfpu: 0x%08x."
                 " This might happen, report to jordi.sola@esperantotech.com if needed.",
                 x.v, gldval.v, fpuval.v);
    }
    return fpuval;
}

static inline float32_t flog_vs_gold(const Hart& cpu, float32_t x)
{
    float32_t fpuval = fpu::f32_log2(x);
    float32_t gldval = gld::f32_log2(x);
    if (gld::security_ulp_check(gldval.v, fpuval.v)) {
        WARN_HART(trans, cpu, "FLOG 2ULP mismatch with input: 0x%08x golden: 0x%08x libfpu: 0x%08x",
                 x.v, gldval.v, fpuval.v);
    }
    /*else if (fpuval.v != gldval.v) {
        WARN_HART(trans, cpu, "FLOG 1ULP diff with input: 0x%08x golden: 0x%08x libfpu: 0x%08x",
                 x.v, gldval.v, fpuval.v);
    }*/
    return fpuval;
}

static inline float32_t frcp_vs_gold(const Hart& cpu, float32_t x)
{
    float32_t fpuval = fpu::f32_rcp(x);
    float32_t gldval = gld::f32_rcp(x);
    if (gld::security_ulp_check(gldval.v, fpuval.v)) {
        WARN_HART(trans, cpu, "FRCP mismatch with input: 0x%08x golden: 0x%08x libfpu: 0x%08x."
                 " This might happen, report to jordi.sola@esperantotech.com if needed.",
                 x.v, gldval.v, fpuval.v);
    }
    return fpuval;
}


#if 0
static inline float32_t frsq_vs_gold(float32_t x)
{
    float32_t fpuval = fpu::f32_rsqrt(x);
    float32_t gldval = gld::f32_rsqrt(x);
    if (gld::security_ulp_check(gldval.v, fpuval.v))
    {
        WARN_HART(trans, cpu, "FRSQ mismatch with input: 0x%08x golden: 0x%08x libfpu: 0x%08x. This might happen, report to jordi.sola@esperantotech.com if needed.",
            x.v, gldval.v, fpuval.v);
    }
    return fpuval;
}


static inline float32_t fsin_vs_gold(float32_t x)
{
    float32_t fpuval = fpu::f32_sin2pi(x);
    float32_t gldval = gld::f32_sin2pi(x);
    if (gld::security_ulp_check(gldval.v, fpuval.v))
    {
        WARN_HART(trans, cpu, "FSIN mismatch with input: 0x%08x golden: 0x%08x libfpu: 0x%08x. This might happen, report to jordi.sola@esperantotech.com if needed.",
            x.v, gldval.v, fpuval.v);
    }
    return fpuval;
}
#endif


void insn_fexp_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1("fexp.ps");
    WRITE_VD( fexp_vs_gold(cpu, FS1.f32[e]) );
    set_fp_exceptions(cpu);
}


void insn_flog_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1("flog.ps");
    WRITE_VD( flog_vs_gold(cpu, FS1.f32[e]) );
    set_fp_exceptions(cpu);
}


void insn_frcp_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1("frcp.ps");
    WRITE_VD( frcp_vs_gold(cpu, FS1.f32[e]) );
    set_fp_exceptions(cpu);
}


void insn_frsq_ps(Hart& cpu)
{
#if 0
    require_fp_active();
    DISASM_FD_FS1("frsq.ps");
    WRITE_VD( frsq_vs_gold(FS1.f32[e]) );
    set_fp_exceptions(cpu);
#else
    DISASM_FD_FS1("frsq.ps");
    throw trap_mcode_instruction(cpu.inst.bits);
#endif
}


void insn_fsin_ps(Hart& cpu)
{
#if 0
    require_fp_active();
    DISASM_FD_FS1("fsin.ps");
    WRITE_VD( fsin_vs_gold(FS1.f32[e]) );
    set_fp_exceptions(cpu);
#else
    DISASM_FD_FS1("fsin.ps");
    throw trap_mcode_instruction(cpu.inst.bits);
#endif
}


} // namespace bemu
