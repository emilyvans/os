#include "init.hpp"
#include "console.hpp"
#include "limine.h"
#include "screen.hpp"
#include <stdint.h>

void init_display(limine_framebuffer *framebuffer, uint32_t background_color,
                  uint64_t scale) {
	uint32_t cols = framebuffer->width / (8 * scale);
	uint32_t rows = framebuffer->height / (8 * scale);

	init_console(cols, rows, scale);
	init_screen(framebuffer, background_color);
}
