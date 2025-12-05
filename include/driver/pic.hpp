#ifndef INCLUDE_DRIVER_PIC_HPP_
#define INCLUDE_DRIVER_PIC_HPP_
#include <stdint.h>

void init_PIC();
void PIC_send_EOI(uint8_t interrupt_number);
void PIC_send_master_EOI();
uint8_t PIC_get_master_isr();
uint8_t PIC_get_slave_isr();

#endif // INCLUDE_DRIVER_PIC_HPP_
