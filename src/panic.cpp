#include "panic.hpp"

// Halt and catch fire function.
void hcf(void) {
	for (;;) {
		asm("hlt");
	}
}
