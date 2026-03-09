#include "cpu/asm.hpp"

void outb(uint16_t address, uint8_t data) {
	asm volatile("outb %0, %w1" : : "a"(data), "Nd"(address) : "memory");
}

void outw(uint16_t address, uint16_t data) {
	asm volatile("outw %0, %w1" : : "a"(data), "Nd"(address) : "memory");
}

void outl(uint16_t address, uint32_t data) {
	asm volatile("outl %0, %w1" : : "a"(data), "Nd"(address) : "memory");
}

uint8_t inb(uint16_t address) {
	uint8_t data;
	asm volatile("inb %1, %0" : "=a"(data) : "Nd"(address));
	return data;
}

uint16_t inw(uint16_t address) {
	uint16_t data;
	asm volatile("inw %1, %0" : "=a"(data) : "Nd"(address));
	return data;
}

uint32_t inl(uint16_t address) {
	uint32_t data;
	asm volatile("inl %1, %0" : "=a"(data) : "Nd"(address));
	return data;
}

void io_wait() {
	outb(0x80, 0);
}
