#include "interupts.hpp"
#include <stdint.h>

void print(const char *);
void print(uint64_t, uint64_t);
void print_entry(Idt entry);

void set_IDT_entry(Idtr *idtr, uint64_t i, uint64_t entry, uint8_t dpl) {

	Idt *idt = &idtr->idts[i];

	idt->address_low = entry & 0xFFFF;
	idt->address_mid = (entry >> 16) & 0xFFFF;
	idt->address_high = (entry >> 32) & 0xFFFFFFFF;

	idt->selector = 0x8;

	idt->flags = 0b1110 | ((dpl & 0b11) << 5) | (1 << 7);

	idt->ist = 0;
	idt->reserved = 0;
	void (*fun)();

	fun = (void (*)())entry;
}

__attribute__((interrupt)) void
interrupt_handler(struct interrupt_frame *frame) {}

#define tt 0

void load_idt(Idtrr *idt_reg) {
	uint64_t t = (uint64_t)idt_reg;
	Idtr *idtr = (Idtr *)idt_reg->base;
	for (uint64_t i = tt; i < tt + 30; i++) {
		print_entry(idtr->idts[i]);
	}
	asm volatile("lidt %0" ::"m"(*idt_reg));
	print("done");
}
