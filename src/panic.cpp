#include "panic.hpp"
#include "driver/console.hpp"
#include <stdarg.h>

// Halt and catch fire function.
void hcf(void) {
	for (;;) {
		asm("hlt");
	}
}

void panic(const char *fmt, ...) {
	printf("\033[1;31mPANIC: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	printf("\033[0m");
	hcf();
}
