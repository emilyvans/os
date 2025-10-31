#include "utils.hpp"

extern "C" void *memcpy(void *__restrict dest, const void *__restrict src,
                        size_t count) {
	uint8_t *__restrict pdest = (uint8_t *__restrict)dest;
	const uint8_t *__restrict psrc = (const uint8_t *__restrict)src;

	for (size_t i = 0; i < count; i++) {
		pdest[i] = psrc[i];
	}

	return dest;
}

extern "C" void *memset(void *address, int number, size_t count) {
	uint8_t *p = (uint8_t *)address;

	for (size_t i = 0; i < count; i++) {
		p[i] = (uint8_t)number;
	}

	return address;
}

extern "C" void *memmove(void *dest, const void *src, size_t count) {
	uint8_t *pdest = (uint8_t *)dest;
	const uint8_t *psrc = (const uint8_t *)src;

	if (src > dest) {
		for (size_t i = 0; i < count; i++) {
			pdest[i] = psrc[i];
		}
	} else if (src < dest) {
		for (size_t i = count; i > 0; i--) {
			pdest[i - 1] = psrc[i - 1];
		}
	}

	return dest;
}

extern "C" int memcmp(const void *address1, const void *address2,
                      size_t count) {
	const uint8_t *p1 = (const uint8_t *)address1;
	const uint8_t *p2 = (const uint8_t *)address2;

	for (size_t i = 0; i < count; i++) {
		if (p1[i] != p2[i]) {
			return p1[i] < p2[i] ? -1 : 1;
		}
	}

	return 0;
}
