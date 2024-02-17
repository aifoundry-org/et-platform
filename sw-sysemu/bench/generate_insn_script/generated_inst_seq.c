#include "macros.h"

int main() {
/* RV64I */
	asm volatile("xor x29, x2, x20");
	asm volatile("add x14, x2, x15");
	asm volatile("or x10, x10, x20");
	asm volatile("addi x9, x31, 821");
	asm volatile("and x16, x10, x16");
/* RV64M */
	asm volatile("remu x18, x13, x22");
	asm volatile("div x30, x25, x3");
	asm volatile("mul x4, x1, x9");
	asm volatile("divu x6, x12, x2");
	asm volatile("rem x7, x0, x23");
}