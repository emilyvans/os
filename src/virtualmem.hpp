#include <stdint.h>

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
// 51-12		| Address of next table
// 11-8			| Available? Unused?
// 7			| Reserved(0)
// 6			| Available? Unused?
// 5			| Accessed
// 4			| Cache Disable
// 3			| Write-Through
// 2			| User/Supervisor
// 1			| Read/Write
// 0			| Present

struct Page_table {
	uint64_t PTE[512];
};

struct Page_directory {
	uint64_t PDE[512];
};

struct Page_directory_pointer_table {
	uint64_t PDPTE[512];
};

struct PML4Table {
	uint64_t PML4E[512];
};

namespace Virtual_memory {

enum map_flags {
	Execute_disable_flag = (uint64_t)1 << 63,
	Accessed_flag = 1 << 5,
	Cache_disable_flag = 1 << 4,
	Write_through_flag = 1 << 3,
	user_flag = 1 << 2,
	readwrite_flag = 1 << 1,
	present_flag = 1 << 0,
};
/*
inline uint64_t flag_mask =
    (uint64_t)map_flags::Execute_disable | (uint64_t)map_flags::Accessed |
    (uint64_t)map_flags::Cache_disable | (uint64_t)map_flags::Write_through |
    (uint64_t)map_flags::user | (uint64_t)map_flags::readwrite |
    (uint64_t)map_flags::present | 1 << 6 | 1 << 7;
*/
constexpr uint64_t address_mask = 0xFFFFFFFFFF000;

void initialize();

void map_page(PML4Table *root, uint64_t virtual_address,
              uint64_t physical_address, uint64_t flags);

void set_current_pagemap(PML4Table *pagemap);

void swap_to_kernel_pagemap();

} // namespace Virtual_memory
