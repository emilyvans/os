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

struct interrupt_frame {
	size_t ip;
	size_t cs;
	size_t flags;
	size_t sp;
	size_t ss;
};

void set_IDT_entry(Idtr *idtr, uint64_t i, uint64_t entry, uint8_t dpl);
void load_idt(Idtrr *idt_reg);

__attribute__((interrupt)) void
interrupt_handler(struct interrupt_frame *frame);
