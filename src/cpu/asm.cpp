#include "cpu/asm.hpp"

void outb(uint16_t address, uint8_t data) {
	asm volatile("outb %b0, %w1" : : "a"(data), "Nd"(address) : "memory");
}

uint8_t inb(uint16_t address) {
	uint8_t data;
	asm volatile("inb %1, %0" : "=a"(data) : "d"(address));
	return data;
}

void io_wait() {
	outb(0x80, 0);
}
