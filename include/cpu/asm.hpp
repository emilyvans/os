#ifndef INCLUDE_CPU_ASM_HPP_
#define INCLUDE_CPU_ASM_HPP_
#include <stdint.h>

void outb(uint16_t address, uint8_t data);
void outw(uint16_t address, uint16_t data);
void outl(uint16_t address, uint32_t data);
uint8_t inb(uint16_t address);
uint16_t inw(uint16_t address);
uint32_t inl(uint16_t address);
void io_wait();

#endif // INCLUDE_CPU_ASM_HPP_
