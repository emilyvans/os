#include "memory/physical_memory.hpp"
#include "driver/console.hpp"
#include "limine/limine_requests.hpp"
#include "panic.hpp"
#include "utils.hpp"
#include <limine.h>
#include <stdint.h>

typedef struct Bitmap {
	bool operator[](uint64_t index) { return get(index); }

	bool get(uint64_t index) {
		uint64_t byte_index = index >> 3; // /8
		uint8_t bit_index = index & 0x7;  // % 8
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
		uint64_t byte_index = index >> 3; // /8
		uint8_t bit_index = index & 0x7;  // % 8
		buffer[byte_index] |= 0x1 << bit_index;
	}

	void reset(uint64_t index) {
		uint64_t byte_index = index >> 3; // /8
		uint8_t bit_index = index & 0x7;  // % 8
		buffer[byte_index] &= ~(1 << bit_index);
	}

	uint64_t get_length() { return bit_length; }

private:
	uint8_t *buffer;
	uint64_t byte_length;
	uint64_t bit_length;
	friend void physicalmemory::initialize();
	friend uint64_t get_bitmap_base();
} Bitmap;
/*
// maybe use later
typedef struct Bintree {
    uint64_t length;
    Bintree *left;
    Bintree *right;
} Bintree;
*/

typedef struct MemoryRegion {
	uint64_t start;
	Bitmap bitmap;
} MemoryRegion;

uint64_t memory_region_count = 0;
MemoryRegion *memory_regions = NULL;

uint64_t physicalmemory::get_total_ram() {
	uint64_t total_ram = 0;
	for (uint64_t i = 0; i < memory_region_count; i++) {
		MemoryRegion *region = memory_regions + i;
		total_ram += region->bitmap.get_length() * 4096;
	}
	return total_ram;
}

uint64_t physicalmemory::get_free_ram() {
	uint64_t free_ram = 0;
	for (uint64_t i = 0; i < memory_region_count; i++) {
		MemoryRegion *region = memory_regions + i;
		for (uint64_t j = 0; j < region->bitmap.get_length(); j++) {
			if (!region->bitmap[j]) {
				free_ram += 4096;
			}
		}
	}

	return free_ram;
}

void physicalmemory::initialize() {
	limine_hhdm_response *hhdm_response = hhdm_request.response;
	limine_memmap_response *memmap_response = memmap_request.response;
	uint64_t total_size = 0;
	for (uint64_t i = 0; i < memmap_response->entry_count; i++) {
		limine_memmap_entry *entry = memmap_response->entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE) {
			uint64_t page_count = DIV_ROUNDUP(entry->length, 4096);
			total_size += (DIV_ROUNDUP(page_count, 8) + sizeof(MemoryRegion));
			memory_region_count++;
			printf("start: 0x%x, size 0x%x, type: 0x%x\n", entry->base,
			       entry->length, entry->type);
		}
	}

	total_size = ALIGN_UP(total_size, 4096);

	void *bitmaps_start = NULL;
	for (uint64_t i = 0; i < memmap_response->entry_count; i++) {
		limine_memmap_entry *entry = memmap_response->entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE) {
			if (entry->length >= total_size) {
				void *start = (void *)(entry->base + entry->length -
				                       total_size + hhdm_response->offset);
				bitmaps_start =
					(void *)((uint64_t)start +
				             (sizeof(MemoryRegion) * memory_region_count));
				memory_regions = (MemoryRegion *)start;
			}
		}
	}

	memset(memory_regions, 0, total_size);

	for (uint64_t i = 0, j = 0, current_map_ptr = (uint64_t)bitmaps_start;
	     i < memmap_response->entry_count; i++) {
		limine_memmap_entry *entry = memmap_response->entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE) {
			uint64_t page_count = DIV_ROUNDUP(entry->length, 4096);
			uint64_t size = DIV_ROUNDUP(page_count, 8);
			MemoryRegion *region = memory_regions + j;
			region->start = entry->base;
			region->bitmap.byte_length = size;
			region->bitmap.bit_length = page_count;
			region->bitmap.buffer = (uint8_t *)current_map_ptr;
			if (entry->base == 0) {
				region->bitmap.set(0);
			}
			current_map_ptr += size;
			j++;
		}
	}

	for (uint64_t i = 0; i < memory_region_count; i++) {
		MemoryRegion *region = memory_regions + i;
		if (region->start <= (uint64_t)memory_regions &&
		    (region->start + (region->bitmap.get_length() * 4096)) >
		        (uint64_t)memory_regions) {
			uint64_t pages = total_size / 4096;
			uint64_t start = region->bitmap.get_length() - pages;
			for (uint64_t i = start; i < region->bitmap.get_length(); i++) {
				region->bitmap.set(i);
			}
			break;
		}
	}
}

PhysicalAddress physicalmemory::kalloc(uint64_t pages) {
	if (pages == 0) {
		printf("zero page alloc");
		hcf();
		return 0;
	}

	for (uint64_t i = 0; i < memory_region_count; i++) {
		uint64_t count = 0;
		MemoryRegion *region = memory_regions + i;
		for (uint64_t j = 0; j < region->bitmap.get_length(); j++) {
			if (!region->bitmap[j]) {
				count++;
				if (count == pages) {
					for (uint64_t index = count; index > 0; index--) {
						region->bitmap.set(j + index - 1);
					}
					// printf("alloc: 0x%x, size: 0x%x\n",
					//        region->start + ((j - count + 1) * 4096),
					//        pages * 4096);
					return region->start + ((j - count + 1) * 4096);
				}
			} else {
				count = 0;
			}
		}
	}
	/*
	    for (uint64_t i = lowest_usable_page; i < bitmap.get_length(); i++) {
	        if (!bitmap[i]) {
	            count++;
	            if (count == pages) {
	                for (uint64_t j = 0; j < count; j++) {
	                    bitmap.set(i + j);
	                }
	                free_mem -= count * 4096;
	                return i * 4096;
	            }
	        } else {
	            count = 0;
	        }
	    }*/

	return NULL;
}

void physicalmemory::kfree(PhysicalAddress address, uint64_t page_count) {
	if (address % 4096 != 0) {
		printf("none page aligned free");
		hcf();
	}
	for (uint64_t i = 0; i < memory_region_count; i++) {
		MemoryRegion *region = memory_regions + i;
		if (region->start <= address &&
		    (region->start + (region->bitmap.get_length() * 4096)) > address) {
			uint64_t start_offset = address - region->start;
			for (uint64_t j = start_offset / 4096;
			     j < ((start_offset / 4096) + page_count); j++) {
				region->bitmap.reset(j);
			}
			break;
		}
	}
}
