#include <stdint.h>
#include "macros.h"

int main() {
	uint64_t x31, x8;

/* RV64I */
	asm volatile("and x25, x20, x10");
	asm volatile("sll x18, x5, x28");
	asm volatile("sub x17, x3, x9");
	asm volatile("sub %0, x4, x1" : "=r"(x8));
	asm volatile("xor x15, x22, x26");
	asm volatile("srl x25, x8, x20");
	asm volatile("xori x30, x24, 280");
	asm volatile("addiw x2, x24, 755");
	asm volatile("or x16, x8, x7");
	asm volatile("srl x27, x15, x24");
	asm volatile("add x13, x3, x18");
        asm volatile("ld %0, 8(%1)" : "=r"(x31) : "r"(x8));
	asm volatile("or x5, x19, x19");
	asm volatile("xor x21, x16, x9");
	asm volatile("addi x29, x30, 1238");
/* RV64M */
	asm volatile("divuw x20, x1, x23");
	asm volatile("div x30, x21, x14");
	asm volatile("divuw x9, x20, x1");
	asm volatile("divu x3, x0, x21");
	asm volatile("divuw x21, x31, x4");
	asm volatile("mul x8, x26, x8");
	asm volatile("rem x24, x16, x10");
	asm volatile("mul x17, x14, x5");
	asm volatile("mul x20, x31, x8");
	asm volatile("rem x21, x29, x29");
	asm volatile("rem x17, x28, x16");
	asm volatile("divw x28, x26, x27");
	asm volatile("divu x2, x4, x2");
	asm volatile("divu x2, x8, x19");
	asm volatile("divw x17, x19, x2");

}
