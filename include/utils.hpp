#ifndef INCLUDE_UTILS_HPP_
#define INCLUDE_UTILS_HPP_
#include <stddef.h>
#include <stdint.h>

#define DIV_ROUNDUP(A, B) ((A + (B - 1)) / B)

#define ALIGN_UP(A, B) (DIV_ROUNDUP(A, B) * B)

#define ALIGN_DOWN(A, B) ((A / B) * B)

extern "C" void *memcpy(void *__restrict dest, const void *__restrict src,
                        size_t count);

extern "C" void *memset(void *address, int number, size_t count);

extern "C" void *memmove(void *dest, const void *src, size_t count);

extern "C" int memcmp(const void *address1, const void *address2, size_t count);

#endif // INCLUDE_UTILS_HPP_
