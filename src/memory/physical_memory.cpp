#include "memory/physical_memory.hpp"
#include "driver/console.hpp"
#include "limine/limine_requests.hpp"
#include "panic.hpp"
#include "utils.hpp"
#include <limine.h>
#include <stdint.h>

struct Bitmap {
	bool operator[](uint64_t index) { return get(index); }

	bool get(uint64_t index) {
		uint64_t byte_index = index / 8;
		uint8_t bit_index = index % 8;
		return buffer[byte_index] & (0x1 << bit_index);
	}

	void set(uint64_t index, bool value) {
		if (value) {
			set(index);
		} else {
			reset(index);
		}
	}

	void set(uint64_t index) {
		uint64_t byte_index = index / 8;
		uint8_t bit_index = index % 8;
		buffer[byte_index] |= 0x1 << bit_index;
	}

	void reset(uint64_t index) {
		uint64_t byte_index = index / 8;
		uint8_t bit_index = index % 8;
		uint8_t byte = 255;
		byte ^= 0x1 << bit_index;
		buffer[byte_index] &= byte;
	}

	uint64_t get_length() { return length * 8; }

private:
	uint8_t *buffer;
	uint64_t length;
	friend void physicalmemory::initialize();
} bitmap;

uint64_t highest_address = 0;
uint64_t free_mem = 0;
uint64_t lowest_usable_page = UINT64_MAX;

void physicalmemory::initialize() {
	limine_hhdm_response *hhdm_response = hhdm_request.response;
	limine_memmap_response *memmap_response = memmap_request.response;
	for (uint64_t i = 0; i < memmap_response->entry_count; i++) {
		limine_memmap_entry *entry = memmap_response->entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE) {
			if (highest_address < entry->base + entry->length) {
				highest_address = entry->base + entry->length;
			}
			free_mem += entry->length;
		}
	}

	uint64_t bitmap_minimum_size = highest_address / 4096 / 8;

	uint64_t bitmap_size = ALIGN_UP(bitmap_minimum_size, 4096);

	for (uint64_t i = 0; i < memmap_response->entry_count; i++) {
		limine_memmap_entry *entry = memmap_response->entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE) {
			if (entry->length >= bitmap_size) {
				bitmap.buffer = (uint8_t *)entry->base + entry->length -
				                bitmap_size + hhdm_response->offset;
				bitmap.length = bitmap_size;
				memset(bitmap.buffer, 0xFF, bitmap_size);
				entry->length -= bitmap_size;
				free_mem -= bitmap_size;
				break;
			}
		}
	}

	for (uint64_t i = 0; i < memmap_response->entry_count; i++) {
		limine_memmap_entry *entry = memmap_response->entries[i];
		if (entry->type != LIMINE_MEMMAP_USABLE)
			continue;

		if ((entry->base / 4096) < lowest_usable_page) {
			lowest_usable_page = entry->base / 4096;
		}

		for (uint64_t addr = 0; addr < entry->length; addr += 4096) {
			bitmap.reset((entry->base + addr) / 4096);
		}
	}
}

PhysicalAddress physicalmemory::kalloc(uint64_t pages) {
	if (pages == 0) {
		printf("zero page alloc");
		hcf();
		return 0;
	}

	uint64_t count = 0;

	for (uint64_t i = lowest_usable_page; i <= bitmap.get_length(); i++) {
		if (!bitmap[i]) {
			count++;
			if (count == pages) {
				bitmap.set(i);
				return i * 4096;
			}
		} else {
			count = 0;
		}
	}

	return 0;
}

void physicalmemory::kfree(PhysicalAddress address, uint64_t page_count) {
	for (uint64_t addr = (uint64_t)address; addr < (page_count * 4096);
	     addr += 4096) {
		bitmap.reset(addr / 4096);
	}
}
