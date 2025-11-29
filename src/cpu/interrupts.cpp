#include "cpu/interrupts.hpp"
#include <stdint.h>

void set_IDT_entry(InterruptDescriptorTable *idtr, uint64_t i, uint64_t entry,
                   uint8_t dpl) {

	IDTEntry *idt = &idtr->entries[i];

	idt->address_low = entry & 0xFFFF;
	idt->address_mid = (entry >> 16) & 0xFFFF;
	idt->address_high = (entry >> 32) & 0xFFFFFFFF;

	idt->selector = 0x8;

	idt->flags = 0b1110 | ((dpl & 0b11) << 5) | (1 << 7);

	idt->ist = 0;
	idt->reserved = 0;
}

void load_IDT(IDTRegister *idt_reg) {
	asm volatile("lidt %0" ::"m"(*idt_reg));
}

static InterruptDescriptorTable idt;
static IDTRegister idt_reg;

void init_IDT() {
	// sets all entry to asm handlers
	for (uint64_t i = 0; i < 256; i++) {
		set_IDT_entry(&idt, i, isr_table[i], 0);
	}
	idt_reg.limit = sizeof(InterruptDescriptorTable) - 1;
	idt_reg.base = (uint64_t)&idt.entries;
	// clear/disable interupts
	asm("cli");

	load_IDT(&idt_reg);
	// enable interupts
	asm("sti");
}
