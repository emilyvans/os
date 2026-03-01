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
	printf("PANIC: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}
