#include "cpu/GDT.hpp"

// B = Base address
// L = Limit
// A = access byte
// F = flags
// 63-32
// 0bBBBBBBBBFFFFLLLLAAAAAAAABBBBBBBB
// 31-0
// 0bBBBBBBBBBBBBBBBBLLLLLLLLLLLLLLLL

void set_gdt_entry(GDT *table, uint8_t i, uint32_t base, uint32_t limit,
                   uint8_t flags, uint8_t access_byte) {
	GDTEntry *entry = &table->entries[i];

	entry->Limit0 = limit & 0xFFFF;
	entry->Limit1_Flags = (limit >> 16) & 0x0F;
	entry->Limit1_Flags |= (flags << 4) & 0xF0;
	entry->Base0 = base & 0xFFFF;
	entry->Base1 = (base >> 16) & 0xFF;
	entry->Base2 = (base >> 24) & 0xFF;
	entry->Accessbyte = access_byte;
}

static GDT gdt;

void init_GDT() {
	// null segment
	set_gdt_entry(&gdt, 0, 0, 0, 0, 0);
	// kernel code segment
	set_gdt_entry(
		&gdt, 1, 0, 0,
		(uint8_t)(GDTFlags::page_granularity | GDTFlags::long_mode_segemnt),
		(uint8_t)(GDTAccessbyteFlags::read | GDTAccessbyteFlags::executable |
	              GDTAccessbyteFlags::code_data_segemnt |
	              GDTAccessbyteFlags::present));
	// kernel data segment
	set_gdt_entry(
		&gdt, 2, 0, 0,
		(uint8_t)(GDTFlags::page_granularity | GDTFlags::long_mode_segemnt),
		(uint8_t)(GDTAccessbyteFlags::read |
	              GDTAccessbyteFlags::code_data_segemnt |
	              GDTAccessbyteFlags::present));

	// user code segment
	set_gdt_entry(
		&gdt, 3, 0, 0,
		(uint8_t)(GDTFlags::page_granularity | GDTFlags::long_mode_segemnt),
		(uint8_t)(GDTAccessbyteFlags::read | GDTAccessbyteFlags::executable |
	              GDTAccessbyteFlags::code_data_segemnt |
	              GDTAccessbyteFlags::ring_3_access |
	              GDTAccessbyteFlags::present));
	// user data segment
	set_gdt_entry(
		&gdt, 4, 0, 0,
		(uint8_t)(GDTFlags::page_granularity | GDTFlags::long_mode_segemnt),
		(uint8_t)(GDTAccessbyteFlags::read |
	              GDTAccessbyteFlags::code_data_segemnt |
	              GDTAccessbyteFlags::ring_3_access |
	              GDTAccessbyteFlags::present));

	GDTDescriptor gdt_descriptor = {.Size = sizeof(GDT) - 1,
	                                .Offset = (uint64_t)&gdt.entries};

	load_gdt(&gdt_descriptor);
}
