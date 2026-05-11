#include "memory/virtual_memory.hpp"
#include "driver/console.hpp"
#include "limine/limine_requests.hpp"
#include "list/container_of.hpp"
#include "list/klist.hpp"
#include "memory/physical_memory.hpp"
#include "utils.hpp"
#include <limine.h>
#include <stdint.h>

uint64_t kernel_map = 0;
uint64_t current_map = 0;

void clear_page(PhysicalAddress physical_address) {
	uint64_t address = physical_address + hhdm_request.response->offset;
	for (uint64_t i = address; i < address + 4096; i += 8) {
		uint64_t *addr = (uint64_t *)i;
		*addr = 0x0;
		(void)addr;
	}
}

void *page_alloc(uint64_t pages) {
	uint64_t addr = physicalmemory::kalloc(pages);
	if (addr == NULL) {
		return NULL;
	}
	return (void *)(addr + hhdm_request.response->offset);
}

void free_pages(void *start, uint64_t pages) {
	physicalmemory::kfree(
		(PhysicalAddress)start - hhdm_request.response->offset, pages);
}

typedef struct FreeList {
	FreeList *next;
} FreeList;

typedef struct Slab {
	uintptr_t first_object_address;
	uint64_t total_count;
	uint64_t free_count;
	uint64_t object_size;
	FreeList *free_list;
	KListHead list;
	struct Cache *parent;
} Slab;

typedef struct Cache {
	uint64_t object_size;
	KListHead full_slabs;
	KListHead partial_slabs;
	KListHead free_slabs;
} Cache;

typedef struct Allocation {
	uint8_t type; // 0 for page, 1 for slab
	uint64_t size;
} Allocation;

Allocation *items;
uint64_t capacity;
uint64_t size;

#define SLAB_PAGES 4
uint64_t slab_availible_space = ((SLAB_PAGES * 4096) - sizeof(Slab));

Slab *alloc_slab(uint64_t obj_size) {
	void *pages = page_alloc(SLAB_PAGES);
	uint64_t object_offset = slab_availible_space % obj_size;
	uint64_t object_count = slab_availible_space / obj_size;
	Slab *slab = (Slab *)pages;

	slab->object_size = obj_size;
	slab->free_count = slab->total_count = object_count;
	klist_init(&slab->list);

	slab->first_object_address =
		(uintptr_t)pages + sizeof(Slab) + object_offset;
	FreeList *prev_free_list = (FreeList *)slab->first_object_address;

	slab->free_list = prev_free_list;
	for (uint64_t i = 1; i < object_count; i++) {
		FreeList *free_list =
			(FreeList *)((uintptr_t)prev_free_list + obj_size);
		prev_free_list->next = free_list;
		prev_free_list = free_list;
	}

	return slab;
}

void free_slab(Slab *slab) {
	free_pages(slab, SLAB_PAGES);
}

// 8 16 32 64 128 256 512 1024 2048
#define KMEM_CACHE_COUNT 9
Cache kmem_cache[KMEM_CACHE_COUNT];
uint64_t kmem_cache_sizes[KMEM_CACHE_COUNT] = {8,   16,  32,   64,  128,
                                               256, 512, 1024, 2048};

void kalloc_init() {
	for (uint64_t i = 0; i < KMEM_CACHE_COUNT; i++) {
		Cache *cache = &kmem_cache[i];
		cache->object_size = kmem_cache_sizes[i];
		klist_init(&cache->free_slabs);
		klist_init(&cache->partial_slabs);
		klist_init(&cache->full_slabs);
	}
}

void *kalloc(uint64_t bytes) {
	Cache *selected_cache = nullptr;
	for (uint64_t i = 0; i < KMEM_CACHE_COUNT; i++) {
		Cache *cache = &kmem_cache[i];
		if (cache->object_size < bytes)
			continue;
		selected_cache = cache;
		break;
	}
	if (selected_cache == nullptr)
		return nullptr;

	if (!klist_is_empty(&selected_cache->partial_slabs)) {
		Slab *slab =
			container_of(selected_cache->partial_slabs.next, Slab, list);
		FreeList *free_list = slab->free_list;
		slab->free_list = free_list->next;
		slab->free_count -= 1;
		if (slab->free_count == 0) {
			KListHead *list_head =
				klist_pop_head(&selected_cache->partial_slabs);
			klist_add_tail(&selected_cache->full_slabs, list_head);
		}
		return free_list;
	} else if (!klist_is_empty(&selected_cache->free_slabs)) {
		Slab *slab = container_of(selected_cache->free_slabs.next, Slab, list);
		FreeList *free_list = slab->free_list;
		slab->free_list = free_list->next;
		slab->free_count -= 1;
		KListHead *list_head = klist_pop_head(&selected_cache->free_slabs);
		klist_add_tail(&selected_cache->partial_slabs, list_head);
		return free_list;
	} else {
		Slab *slab = alloc_slab(selected_cache->object_size);
		slab->parent = selected_cache;
		FreeList *free_list = slab->free_list;
		slab->free_list = free_list->next;
		slab->free_count -= 1;
		klist_add_tail(&selected_cache->partial_slabs, &slab->list);
		return free_list;
	}

	return nullptr;
}

void kfree(void *ptr) {
	Slab *slab = (Slab *)((uintptr_t)ptr & ~((4096 * SLAB_PAGES) - 1));
}

void *kzalloc(uint64_t bytes) {
	void *start = kalloc(bytes);
	if (start == nullptr) {
		return nullptr;
	}

	memset(start, 0, bytes);

	return start;
}

void virtualmemory::initialize() {
	limine_executable_file_response *executable_file_response =
		executable_file_request.response;
	limine_executable_address_response *executable_address_response =
		executable_address_request.response;
	limine_framebuffer_response *framebuffer_response =
		framebuffer_request.response;
	limine_hhdm_response *hhdm_response = hhdm_request.response;
	limine_memmap_response *memmap_response = memmap_request.response;

	kernel_map = physicalmemory::kalloc(sizeof(PML4Table) / 4096);
	clear_page((PhysicalAddress)kernel_map);

	for (uint64_t i = 0; i < executable_file_response->executable_file->size;
	     i += 4096) {
		map_page(kernel_map, executable_address_response->virtual_base + i,
		         executable_address_response->physical_base + i,
		         present_flag | readwrite_flag);
	}
	for (uint64_t i = 0; i < memmap_response->entry_count; i++) {
		limine_memmap_entry *entry = memmap_response->entries[i];
		/*if (entry->type == LIMINE_MEMMAP_ACPI_TABLES ||
		    entry->type == LIMINE_MEMMAP_ACPI_RECLAIMABLE ||
		    entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE ||
		    entry->type == LIMINE_MEMMAP_RESERVED ||
		    entry->type == LIMINE_MEMMAP_USABLE) {*/
		if (entry->type != LIMINE_MEMMAP_BAD_MEMORY) {
			/*if (entry->type == LIMINE_MEMMAP_ACPI_TABLES ||
			    entry->type == LIMINE_MEMMAP_ACPI_RECLAIMABLE ||
			    entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE ||
			    entry->type == LIMINE_MEMMAP_USABLE) {*/
			for (uint64_t offset = 0; offset < entry->length; offset += 4096) {
				map_page(kernel_map,
				         hhdm_response->offset + entry->base + offset,
				         entry->base + offset, present_flag | readwrite_flag);
			}
		}
	}

	uint64_t framebuffer_physical_address = 0;
	uint64_t framebuffer_length = 0;

	for (uint64_t i = 0; i < memmap_response->entry_count; i++) {
		limine_memmap_entry *entry = memmap_response->entries[i];
		if (entry->type != LIMINE_MEMMAP_FRAMEBUFFER)
			continue;
		framebuffer_physical_address = entry->base;
		framebuffer_length = entry->length;
	}
	/*
	    for (uint64_t i = get_bitmap_base();
	         i < (get_bitmap_base() + get_bitmap_byte_length()); i += 4096) {
	        map_page(kernel_map, i, i - hhdm_response->offset,
	                 present_flag | readwrite_flag);
	    }*/

	for (uint64_t i = 0; i < framebuffer_length; i += 4096) {
		map_page(kernel_map,
		         (uint64_t)framebuffer_response->framebuffers[0]->address + i,
		         framebuffer_physical_address + i,
		         present_flag | readwrite_flag);
	}

	swap_to_kernel_pagemap();
	kalloc_init();
}

// P = address in the page
// T = index in page table
// D = index in page directory
// p = index in page directory pointer table
// M = index in page map level 4 table
// X = unused part of virtual address
// 0bXXXXXXXXXXXXXXXXMMMMMMMMMpppppppppDDDDDDDDDTTTTTTTTTPPPPPPPPPPPP
//
// entries format
// bit(-bit)	| meaning
// 63			| Execute Disable
// 62-52		| Available? Unused?
// 51-12		| bits 51-12 of the Address of next table
// 11-8			| Available? Unused?
// 7			| Reserved(0)
// 6			| Available? Unused?
// 5			| Accessed
// 4			| Cache Disable
// 3			| Write-Through
// 2			| User/Supervisor
// 1			| Read/Write
// 0			| Present

void virtualmemory::map_page(uint64_t root_physical, uint64_t virtual_address,
                             uint64_t physical_address, uint64_t flags) {
	limine_hhdm_response *hhdm_response = hhdm_request.response;
	PML4Table *root =
		(PML4Table *)((uint64_t)root_physical + hhdm_response->offset);

	uint64_t upper_flags = present_flag | readwrite_flag;
	if (flags & Cache_disable_flag)
		upper_flags |= Cache_disable_flag;

	uint64_t pml4_index = (virtual_address >> 39) & 0x1FF;
	uint64_t pdpt_index = (virtual_address >> 30) & 0x1FF;
	uint64_t pd_index = (virtual_address >> 21) & 0x1FF;
	uint64_t pt_index = (virtual_address >> 12) & 0x1FF;

	if (!(root->PML4E[pml4_index] & present_flag)) {
		PhysicalAddress table =
			physicalmemory::kalloc(sizeof(PageDirectoryPointerTable) / 4096);
		clear_page(table);
		uint64_t entry = (uint64_t)table & address_mask;
		root->PML4E[pml4_index] = entry | upper_flags;
	}
	PageDirectoryPointerTable *pdpt =
		(PageDirectoryPointerTable *)((root->PML4E[pml4_index] & address_mask) +
	                                  hhdm_response->offset);

	if (!(pdpt->PDPTE[pdpt_index] & present_flag)) {
		PhysicalAddress directory =
			physicalmemory::kalloc(sizeof(PageDirectory) / 4096);
		clear_page(directory);
		uint64_t entry = (uint64_t)directory & address_mask;
		pdpt->PDPTE[pdpt_index] = entry | upper_flags;
	}

	PageDirectory *page_directory =
		(PageDirectory *)((pdpt->PDPTE[pdpt_index] & address_mask) +
	                      hhdm_response->offset);

	if (!(page_directory->PDE[pd_index] & present_flag)) {
		PhysicalAddress table =
			physicalmemory::kalloc(sizeof(PageTable) / 4096);
		clear_page(table);
		uint64_t entry = (uint64_t)table & address_mask;
		page_directory->PDE[pd_index] = entry | upper_flags;
	}

	PageTable *page_table =
		(PageTable *)((page_directory->PDE[pd_index] & address_mask) +
	                  hhdm_response->offset);

	page_table->PTE[pt_index] = (physical_address & address_mask) | flags;
}

void virtualmemory::map_kernel_page(uint64_t virtual_address,
                                    uint64_t physical_address, uint64_t flags) {
	map_page(kernel_map, virtual_address, physical_address, flags);
}

void virtualmemory::set_current_pagemap(uint64_t pagemap) {
	current_map = pagemap;
	asm volatile("mov %0, %%cr3" ::"r"((uint64_t)pagemap) : "memory");
}

void virtualmemory::swap_to_kernel_pagemap() {
	set_current_pagemap(kernel_map);
}
