#include "GDT.hpp"

// B = Base address
// L = Limit
// A = access byte
// F = flags
// 63-32
// 0bBBBBBBBBFFFFLLLLAAAAAAAABBBBBBBB
// 31-0
// 0bBBBBBBBBBBBBBBBBLLLLLLLLLLLLLLLL

void set_gdt_entry(GDT_table *table, uint8_t i, uint32_t base, uint32_t limit,
                   uint8_t flags, uint8_t access_byte) {
	GDT_entry *entry = &table->entries[i];

	entry->Limit0 = limit & 0xFFFF;
	entry->Limit1_Flags = (limit >> 16) & 0x0F;
	entry->Limit1_Flags |= (flags << 4) & 0xF0;
	entry->Base0 = base & 0xFFFF;
	entry->Base1 = (base >> 16) & 0xFF;
	entry->Base2 = (base >> 24) & 0xFF;
	entry->Accessbyte = access_byte;
}
