/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include "decode.h"
#include "emu_gio.h"
#include "gold.h"
#include "insn.h"
#include "insn_func.h"
#include "log.h"
#include "processor.h"
#include "traps.h"
#include "utility.h"
#include "fpu/fpu.h"
#include "fpu/fpu_casts.h"

// FIXME: Replace with "processor.h"
#include "emu_defines.h"
extern std::array<Processor,EMU_NUM_THREADS> cpu;

//namespace bemu {


// LCOV_EXCL_START
static inline float32_t fexp_vs_gold(float32_t x)
{
    float32_t fpuval = fpu::f32_exp2(x);
    float32_t gldval = gld::f32_exp2(x);
    if (gld::security_ulp_check(gldval.v, fpuval.v))
    {
        LOG(WARN, "FEXP mismatch with input: 0x%08x golden: 0x%08x libfpu: 0x%08x. This might happen, report to jordi.sola@esperantotech.com if needed.",
            x.v, gldval.v, fpuval.v);
    }
    return fpuval;
}

static inline float32_t flog_vs_gold(float32_t x)
{
    float32_t fpuval = fpu::f32_log2(x);
    float32_t gldval = gld::f32_log2(x);
    if (gld::security_ulp_check(gldval.v, fpuval.v))
    {
        LOG(WARN, "FLOG 2ULP mismatch with input: 0x%08x golden: 0x%08x libfpu: 0x%08x", x.v, gldval.v, fpuval.v);
    }
    /*else if (fpuval.v != gldval.v)
    {
        LOG(WARN, "FLOG 1ULP diff with input: 0x%08x golden: 0x%08x libfpu: 0x%08x", x.v, gldval.v, fpuval.v);
    }*/
    return fpuval;
}

static inline float32_t frcp_vs_gold(float32_t x)
{
    float32_t fpuval = fpu::f32_rcp(x);
    float32_t gldval = gld::f32_rcp(x);
    if (gld::security_ulp_check(gldval.v, fpuval.v))
    {
        LOG(WARN, "FRCP mismatch with input: 0x%08x golden: 0x%08x libfpu: 0x%08x. This might happen, report to jordi.sola@esperantotech.com if needed.",
            x.v, gldval.v, fpuval.v);
    }
    return fpuval;
}


static inline float32_t frsq_vs_gold(float32_t x)
{
    float32_t fpuval = fpu::f32_rsqrt(x);
    float32_t gldval = gld::f32_rsqrt(x);
    if (gld::security_ulp_check(gldval.v, fpuval.v))
    {
        LOG(WARN, "FRSQ mismatch with input: 0x%08x golden: 0x%08x libfpu: 0x%08x. This might happen, report to jordi.sola@esperantotech.com if needed.",
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
        LOG(WARN, "FSIN mismatch with input: 0x%08x golden: 0x%08x libfpu: 0x%08x. This might happen, report to jordi.sola@esperantotech.com if needed.",
            x.v, gldval.v, fpuval.v);
    }
    return fpuval;
}
// LCOV_EXCL_STOP

void insn_fexp_ps(insn_t inst)
{
    require_fp_active();
    DISASM_FD_FS1("fexp.ps");
    WRITE_VD( fexp_vs_gold(FS1.f32[e]) );
    set_fp_exceptions();
}


void insn_flog_ps(insn_t inst)
{
    require_fp_active();
    DISASM_FD_FS1("flog.ps");
    WRITE_VD( flog_vs_gold(FS1.f32[e]) );
    set_fp_exceptions();
}


void insn_frcp_ps(insn_t inst)
{
    require_fp_active();
    DISASM_FD_FS1("frcp.ps");
    WRITE_VD( frcp_vs_gold(FS1.f32[e]) );
    set_fp_exceptions();
}


void insn_frsq_ps(insn_t inst)
{
    require_fp_active();
    DISASM_FD_FS1("frsq.ps");
    WRITE_VD( frsq_vs_gold(FS1.f32[e]) );
    set_fp_exceptions();
}


void insn_fsin_ps(insn_t inst)
{
    require_fp_active();
    DISASM_FD_FS1("fsin.ps");
    WRITE_VD( fsin_vs_gold(FS1.f32[e]) );
    set_fp_exceptions();
}


//} // namespace bemu
