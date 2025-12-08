#include "driver/uart.hpp"
#include "cpu/asm.hpp"
#include "driver/console.hpp"
#include <stdint.h>

#define PORT 0x3F8

bool uart_set_baud_rate(uint16_t baud_rate) {
	uint16_t divisor = 115200 / baud_rate;
	uint8_t least = divisor & 0xff;
	uint8_t most = (divisor >> 8) & 0xff;

	outb(PORT + 1, 0x0);
	io_wait();

	outb(PORT + 3, 0x80);
	io_wait();

	outb(PORT + 0, least);
	io_wait();
	outb(PORT + 1, most);
	io_wait();

	outb(PORT + 3, 0x3);
	io_wait();

	outb(PORT + 4, 0b10000);
	io_wait();

	outb(PORT + 0, 0x8D);
	io_wait();

	bool found = false;

	for (uint64_t i = 0; (i < 999999) && !found; i++) {
		if (inb(PORT) == 0x8D) {
			found = true;
		}
		io_wait();
	}
	if (!found) {
		return false;
	}

	outb(PORT + 4, 0x0F);
	io_wait();
	outb(PORT + 1, 0x0);
	io_wait();
	outb(PORT + 2, 0xC7);
	io_wait();
	return true;
}

void uart_init() {
	if (uart_set_baud_rate(9600)) {
		console_set_uart_enabled();
	}
}

void uart_send(char c) {
	while (!(inb(PORT + 5) & 0b100000)) {
		io_wait();
	}

	outb(PORT, c);
}

bool uart_data_recieved() {
	return (inb(PORT + 5) & 0b1);
}

char uart_recieve() {
	while (!uart_data_recieved()) {
		io_wait();
	}
	return inb(PORT);
}
