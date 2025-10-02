#include "macros.h"
#include "etsoc/isa/tensors.h"
#include "etsoc/isa/cacheops.h"
#include "etsoc/isa/hart.h"
#include <stdint.h>


int main() {
	float f1 = 45.851161435216056, f2 = 21.266342051550836, f3 = 12.248774915438798, output;
	(void)f1;
	(void)f2;
	(void)f3;
	(void)output;
/* RV64A */
	uint32_t result;
	float fresult;

	int32_t i = 1;
	uint32_t u = 1;

    __asm__ volatile ("fcvt.wu.s %0, %1" : "=r"(result) : "f"(f1));
    __asm__ volatile ("fcvt.s.w %0, %1" : "=f"(fresult) : "r"(i));
	__asm__ volatile ("fcvt.s.wu %0, %1" : "=f"(fresult) : "r"(u));

	// fcvt.w.s
	// fsgnj.s
	// fsgnjn.s
	// fsgnjx.s
	__asm__ volatile ("fcvt.w.s %0, %1" : "=r"(result) : "f"(f1));
	__asm__ volatile ("fsgnj.s %0, %1, %2" : "=f"(output) : "f"(f1), "f"(f2));
	__asm__ volatile ("fsgnjn.s %0, %1, %2" : "=f"(output) : "f"(f1), "f"(f2));
	__asm__ volatile ("fsgnjx.s %0, %1, %2" : "=f"(output) : "f"(f1), "f"(f2));
	__asm__ volatile ("fmv.w.x %0, %1" : "=f"(output) : "r"(i));
	
	// fmv.w.x
	// fmv.x.w
	// feq.s
	// fle.s
	// flt.s
	// fclass.s
	__asm__ volatile ("fmv.w.x %0, %1" : "=f"(output) : "r"(u));
	__asm__ volatile ("fmv.x.w %0, %1" : "=r"(result) : "f"(f1));
	__asm__ volatile ("feq.s %0, %1, %2" : "=r"(result) : "f"(f1), "f"(f2));
	__asm__ volatile ("fle.s %0, %1, %2" : "=r"(result) : "f"(f1), "f"(f2));
	__asm__ volatile ("flt.s %0, %1, %2" : "=r"(result) : "f"(f1), "f"(f2));
	__asm__ volatile ("fclass.s %0, %1" : "=r"(result) : "f"(f1));
}