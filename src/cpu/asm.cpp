#include "cpu/asm.hpp"

void outb(uint16_t address, uint8_t data) {
	asm volatile("outb %b0, %w1" : : "a"(data), "Nd"(address) : "memory");
}

uint8_t inb(uint16_t address) {
	uint8_t data;
	asm volatile("inb %w1, %b0" : "=a"(data) : "Nd"(address) : "memory");
	return data;
}

void io_wait() {
	outb(0x80, 0);
}
