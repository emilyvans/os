#include "virtual_memory.hpp"
#include "console.hpp"
#include "limine.h"
#include "limine_requests.hpp"
#include "physical_memory.hpp"
#include <stdint.h>

void print(uint64_t original_number, uint64_t base);
PML4Table *kernel_map = 0;
PML4Table *current_map = 0;

void clear_page(PhysicalAddress physical_address) {
	uint64_t address = physical_address + hhdm_request.response->offset;
	for (uint64_t i = address; i < address + 4096; i += 8) {
		uint64_t *addr = (uint64_t *)i;
		*addr = 0;
		(void)addr;
	}
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

	kernel_map = (PML4Table *)physicalmemory::kalloc(sizeof(PML4Table) / 4096);
	clear_page((PhysicalAddress)kernel_map);

	for (uint64_t i = 0; i < executable_file_response->executable_file->size;
	     i += 4096) {
		map_page(kernel_map, executable_address_response->virtual_base + i,
		         executable_address_response->physical_base + i,
		         present_flag | readwrite_flag);
	}

	for (uint64_t i = 0; i < 0x300000000; i += 4096) {
		map_page(kernel_map, hhdm_response->offset + i, i,
		         present_flag | readwrite_flag);
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

	for (uint64_t i = 0; i < framebuffer_length; i += 4096) {
		map_page(kernel_map,
		         (uint64_t)framebuffer_response->framebuffers[0]->address + i,
		         framebuffer_physical_address + i,
		         present_flag | readwrite_flag);
	}

	swap_to_kernel_pagemap();
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
// bits(-bits)	| meaning
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

void virtualmemory::map_page(PML4Table *root, uint64_t virtual_address,
                             uint64_t physical_address, uint64_t flags) {
	limine_hhdm_response *hhdm_response = hhdm_request.response;
	root = (PML4Table *)((uint64_t)root + hhdm_response->offset);

	uint64_t pml4_index = (virtual_address >> 39) & 0x1FF;
	uint64_t pdpt_index = (virtual_address >> 30) & 0x1FF;
	uint64_t pd_index = (virtual_address >> 21) & 0x1FF;
	uint64_t pt_index = (virtual_address >> 12) & 0x1FF;

	if (!(root->PML4E[pml4_index] & present_flag)) {
		PhysicalAddress table =
			physicalmemory::kalloc(sizeof(PageDirectoryPointerTable) / 4096);
		clear_page(table);
		uint64_t entry = (uint64_t)table & address_mask;
		root->PML4E[pml4_index] =
			entry | present_flag | readwrite_flag | user_flag;
	}
	PageDirectoryPointerTable *pdpt =
		(PageDirectoryPointerTable *)((root->PML4E[pml4_index] & address_mask) +
	                                  hhdm_response->offset);

	if (!(pdpt->PDPTE[pdpt_index] & present_flag)) {
		PhysicalAddress directory =
			physicalmemory::kalloc(sizeof(PageDirectory) / 4096);
		clear_page(directory);
		uint64_t entry = (uint64_t)directory & address_mask;
		pdpt->PDPTE[pdpt_index] =
			entry | present_flag | readwrite_flag | user_flag;
	}

	PageDirectory *page_directory =
		(PageDirectory *)((pdpt->PDPTE[pdpt_index] & address_mask) +
	                      hhdm_response->offset);

	if (!(page_directory->PDE[pd_index] & present_flag)) {
		PhysicalAddress table =
			physicalmemory::kalloc(sizeof(PageTable) / 4096);
		clear_page(table);
		uint64_t entry = (uint64_t)table & address_mask;
		page_directory->PDE[pd_index] =
			entry | present_flag | readwrite_flag | user_flag;
	}

	PageTable *page_table =
		(PageTable *)((page_directory->PDE[pd_index] & address_mask) +
	                  hhdm_response->offset);

	page_table->PTE[pt_index] = (physical_address & address_mask) | flags;
}

void virtualmemory::set_current_pagemap(PML4Table *pagemap) {
	current_map = pagemap;
	asm volatile("mov %0, %%cr3" ::"r"((uint64_t)pagemap) : "memory");
}

void virtualmemory::swap_to_kernel_pagemap() {
	set_current_pagemap(kernel_map);
}
