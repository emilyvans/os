#ifndef INCLUDE_DRIVER_SCREEN_HPP_
#define INCLUDE_DRIVER_SCREEN_HPP_
#include <limine.h>
#include <stdint.h>

void init_screen(limine_framebuffer *new_framebuffer,
                 uint32_t new_background_color);
void draw_char(char character, uint64_t start_y, uint64_t start_x,
               uint32_t color = 0xFFFFFF, uint32_t scale = 2);

void clear_screen();
void clear_screen(uint32_t color);

void image_blit(uint32_t *buffer, uint32_t start_x, uint32_t start_y,
                uint32_t width, uint32_t height);

#endif // INCLUDE_DRIVER_SCREEN_HPP_
