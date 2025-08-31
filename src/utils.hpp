#include <stddef.h>
#include <stdint.h>

#define DIV_ROUNDUP(A, B) (A + (B - 1)) / B

#define ALIGN_UP(A, B) DIV_ROUNDUP(A, B) * B

#define ALIGN_DOWN(A, B) (A / B) * B

extern "C" void *memcpy(void *__restrict dest, const void *__restrict src,
                        size_t n);
extern "C" void *memset(void *s, int c, size_t n);
extern "C" void *memmove(void *dest, const void *src, size_t n);
extern "C" int memcmp(const void *s1, const void *s2, size_t n);

void print(const char *fmt);

void print(const char *fmt, uint64_t count);

void print(uint64_t original_number, uint64_t base);
