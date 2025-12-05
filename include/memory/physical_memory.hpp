#ifndef INCLUDE_MEMORY_PHYSICAL_MEMORY_HPP_
#define INCLUDE_MEMORY_PHYSICAL_MEMORY_HPP_
#include <stdint.h>

typedef uint64_t PhysicalAddress;

namespace physicalmemory {
void initialize();
PhysicalAddress kalloc(uint64_t page_count);
void kfree(PhysicalAddress address, uint64_t page_count);
} // namespace physicalmemory
#endif // INCLUDE_MEMORY_PHYSICAL_MEMORY_HPP_
