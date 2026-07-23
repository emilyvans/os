#ifndef INCLUDE_MEMORY_PHYSICAL_MEMORY_HPP_
#define INCLUDE_MEMORY_PHYSICAL_MEMORY_HPP_
#include "list/klist.hpp"
#include <stdint.h>

typedef uint64_t PhysicalAddress;

enum PageType : uint8_t {
	PageTypeNone = 0,
	PageTypeSlab,
};

typedef struct Page_S {
	uint64_t address;
	PageType type;
	uint32_t data;
} Page;

typedef struct PageArray_s {
	uint64_t count;
	Page *data;
} PageArray;

namespace physicalmemory {
void initialize();
PhysicalAddress kalloc(uint64_t page_count, uint64_t alignment = 1);
void kfree(PhysicalAddress address, uint64_t page_count);
uint64_t get_free_ram();
uint64_t get_total_ram();
} // namespace physicalmemory

typedef struct Bitmap_s {
	bool get(uint64_t index);

	void set(uint64_t index, bool value);

	void set(uint64_t index);

	void reset(uint64_t index);

	bool operator[](uint64_t index) { return get(index); }

	uint64_t get_length() { return bit_length; }

private:
	uint8_t *buffer;
	uint64_t byte_length;
	uint64_t bit_length;
	friend void physicalmemory::initialize();
	friend uint64_t get_bitmap_base();
} Bitmap;

typedef struct MemoryRegion {
	uint64_t start;
	PageArray pages;
	Bitmap bitmap;
} MemoryRegion;

#endif // INCLUDE_MEMORY_PHYSICAL_MEMORY_HPP_
