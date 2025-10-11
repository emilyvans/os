#include <stddef.h>
#include <stdint.h>

struct Idt {
	uint16_t address_low = 0;
	uint16_t selector = 0;
	uint8_t ist = 0;
	uint8_t flags = 0;
	uint16_t address_mid = 0;
	uint32_t address_high = 0;
	uint32_t reserved = 0;
} __attribute__((packed));

struct Idtr {
	Idt idts[256];
};
struct Idtrr {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed, aligned(0x1000)));

extern "C" uint64_t isr_table[256];

struct interrupt_frame {
	size_t r15;
	size_t r14;
	size_t r13;
	size_t r12;
	size_t r11;
	size_t r10;
	size_t r9;
	size_t r8;
	size_t rbp;
	size_t rdi;
	size_t rsi;
	size_t rdx;
	size_t rcx;
	size_t rbx;
	size_t rax;
	size_t interupt_nr;
	size_t error_code;
	size_t rip;
	size_t cs;
	size_t flags;
	size_t sp;
	size_t ss;
};

void set_IDT_entry(Idtr *idtr, uint64_t i, uint64_t entry, uint8_t dpl);
void load_idt(Idtrr *idt_reg);

// RAX RBX RCX RDX RSI RDI (rsp rbp) R8 R9 R10 R11 R12 R13 R14 R15

extern "C" void interrupt_handler(struct interrupt_frame *frame);
