#ifndef INCLUDE_PHYSICALMEMORY_HPP_
#define INCLUDE_PHYSICALMEMORY_HPP_
#include "limine.h"
#include <stdint.h>

typedef uint64_t PhysicalAddress;

namespace physicalmemory {
void initialize();
PhysicalAddress kalloc(uint64_t page_count);
void kfree(PhysicalAddress address, uint64_t page_count);
} // namespace physicalmemory
#endif // INCLUDE_PHYSICALMEMORY_HPP_
