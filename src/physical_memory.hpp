#ifndef INCLUDE_PHYSICALMEMORY_HPP_
#define INCLUDE_PHYSICALMEMORY_HPP_
#include "limine.h"
#include <stdint.h>

namespace physicalmemory {
void initialize();
void *kalloc(uint64_t page_count);
void kfree(void *address, uint64_t page_count);
} // namespace physicalmemory
#endif // INCLUDE_PHYSICALMEMORY_HPP_
