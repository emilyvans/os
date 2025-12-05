#ifndef INCLUDE_DRIVER_INIT_HPP_
#define INCLUDE_DRIVER_INIT_HPP_
#include <limine.h>
#include <stdint.h>

void init_display(limine_framebuffer *framebuffer, uint32_t background_color,
                  uint64_t scale);

#endif // INCLUDE_DRIVER_INIT_HPP_
