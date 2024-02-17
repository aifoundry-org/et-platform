#include "macros.h"

int main() {
/* RV64I */
	asm volatile("add x13, x3, x18");
	asm volatile("ld x31, x28, 8(x8)");
	asm volatile("or x5, x19, x19");
	asm volatile("xor x21, x16, x9");
	asm volatile("addi x29, x30, 1238");
/* RV64M */
	asm volatile("remu x21, x27, x24");
	asm volatile("rem x25, x19, x16");
	asm volatile("div x31, x23, x27");
	asm volatile("divw x11, x27, x12");
	asm volatile("rem x9, x9, x23");
}