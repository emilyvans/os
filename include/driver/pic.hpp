#ifndef INCLUDE_PIC_HPP_
#define INCLUDE_PIC_HPP_
#include <stdint.h>

void init_PIC();
void PIC_send_EOI(uint8_t interrupt_number);

#endif // INCLUDE_PIC_HPP_
