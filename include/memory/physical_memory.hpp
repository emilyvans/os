#ifndef INCLUDE_MEMORY_PHYSICAL_MEMORY_HPP_
#define INCLUDE_MEMORY_PHYSICAL_MEMORY_HPP_
#include <stdint.h>

typedef uint64_t PhysicalAddress;

namespace physicalmemory {
void initialize();
PhysicalAddress kalloc(uint64_t page_count);
void kfree(PhysicalAddress address, uint64_t page_count);
uint64_t get_free_ram();
uint64_t get_total_ram();
} // namespace physicalmemory
#endif // INCLUDE_MEMORY_PHYSICAL_MEMORY_HPP_
