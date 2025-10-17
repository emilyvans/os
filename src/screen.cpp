#include "screen.hpp"
#include "8x8font.h"
#include "limine.h"
#include <stddef.h>
#include <stdint.h>

uint32_t background_color = 0;
limine_framebuffer *framebuffer;

void init_screen(limine_framebuffer *new_framebuffer,
                 uint32_t new_background_color) {
	framebuffer = new_framebuffer;
	background_color = new_background_color;
	clear_screen();
}

void draw_char(char c, uint64_t start_y, uint64_t start_x, uint32_t color,
               uint32_t scale) {
	uint8_t *character_glyph = font8x8_basic[(uint8_t)c];
	volatile uint32_t *fb_ptr = (uint32_t *)framebuffer->address;

	for (uint64_t y = start_y; y < start_y + (8 * scale); y++) {
		uint8_t row = character_glyph[int((y - start_y) / scale)];
		uint64_t row_offset = y * framebuffer->width;

		for (uint64_t x = start_x; x < start_x + (8 * scale); x++) {
			bool pixel = (row >> int((x - start_x) / scale)) & 1;

			if (pixel) {
				fb_ptr[row_offset + x] = color;
			} else {
				fb_ptr[row_offset + x] = background_color; // 0x000000;
			}
		}
	}
}

void clear_screen() {
	for (size_t x = 0; x < framebuffer->width; x++) {
		for (size_t y = 0; y < framebuffer->height; y++) {
			volatile uint32_t *fb_ptr = (uint32_t *)framebuffer->address;
			fb_ptr[y * framebuffer->width + x] = background_color;
		}
	}
}

void clear_screen(uint32_t color) {
	for (size_t x = 0; x < framebuffer->width; x++) {
		for (size_t y = 0; y < framebuffer->height; y++) {
			volatile uint32_t *fb_ptr = (uint32_t *)framebuffer->address;
			fb_ptr[y * framebuffer->width + x] = color; // 0x0000FF;
		}
	}
}

void image_blit(uint32_t *buffer, uint32_t start_x, uint32_t start_y,
                uint32_t width, uint32_t height) {
	volatile uint32_t *fb_ptr = (uint32_t *)framebuffer->address;

	for (uint32_t y = 0; y < height; y++) {
		uint32_t image_row_offset = y * width;
		uint32_t screen_row_offset = (y + start_y) * framebuffer->width;
		for (uint32_t x = 0; x < width; x++) {
			fb_ptr[screen_row_offset + (start_x + x)] =
				buffer[image_row_offset + x];
		}
	}
}
