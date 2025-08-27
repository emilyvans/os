#include "limine.h"
#include <stdint.h>

namespace Physical_memory {
void initialize();
void *kalloc(uint64_t page_count);
} // namespace Physical_memory
