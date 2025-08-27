#include <stdint.h>

enum class GDT_accessbyte_flags {
	accessed = 1 << 0,
	// read only applies to code segments
	read = 1 << 1,
	// write only applies to data segments
	write = 1 << 1,
	// grows down ony applies to data segment
	grows_down = 1 << 2,
	// allows lower or equel access to execute this segment. only applies to
	// code segemnts
	allow_lower = 1 << 2,
	executable = 1 << 3,
	code_data_segemnt = 1 << 4,
	ring_1_access = 1 << 5,
	ring_2_access = 1 << 6,
	ring_3_access = 1 << 6 | 1 << 5,
	present = 1 << 7,
};

inline GDT_accessbyte_flags operator|(GDT_accessbyte_flags a,
                                      GDT_accessbyte_flags b) {
	return (GDT_accessbyte_flags)((uint8_t)a | (uint8_t)b);
}

enum class GDT_flags {
	long_mode_segemnt = 1 << 1,
	protected_mode_segment = 1 << 2,
	page_granularity = 1 << 3,
};

inline GDT_flags operator|(GDT_flags a, GDT_flags b) {
	return (GDT_flags)((uint8_t)a | (uint8_t)b);
}

struct GDT_descriptor {
	uint16_t Size;
	uint64_t Offset;
} __attribute__((packed, aligned(0x1000)));

struct GDT_entry {
	uint16_t Limit0;
	uint16_t Base0;
	uint8_t Base1;
	uint8_t Accessbyte;
	uint8_t Limit1_Flags;
	uint8_t Base2;
} __attribute__((packed));

struct GDT_table {
	GDT_entry entries[256];
};

void set_gdt_entry(GDT_table *table, uint8_t i, uint32_t base, uint32_t limit,
                   uint8_t flags, uint8_t access_byte);

// i don't know how this function works. but the function is defined in gdt.asm
extern "C" void load_gdt(GDT_descriptor *gdt_descriptor);
