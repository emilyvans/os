#ifndef INCLUDE_DRIVER_UART_HPP_
#define INCLUDE_DRIVER_UART_HPP_
#include <stdint.h>

void uart_set_baud_rate(uint16_t baud_rate);
void uart_init();
bool uart_data_recieved();
void uart_send(char c);
char uart_recieve();

#endif // INCLUDE_DRIVER_UART_HPP_
