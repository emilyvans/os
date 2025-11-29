#ifndef INCLUDE_ASM_HPP_
#define INCLUDE_ASM_HPP_
#include <stdint.h>

void outb(uint16_t address, uint8_t data);
uint8_t inb(uint16_t address);
void io_wait();

#endif // INCLUDE_ASM_HPP_
