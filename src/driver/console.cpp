#include "driver/console.hpp"
#include "driver/screen.hpp"
#include "driver/uart.hpp"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

struct Point {
	uint64_t x;
	uint64_t y;
};

uint64_t cols = 0;
uint64_t rows = 0;
Point cursor = {0, 0};
uint64_t scale = 2;
bool uart_enabled = false;
uint8_t history[80 * 50] = {};

void init_console(uint64_t new_cols, uint64_t new_rows, uint64_t new_scale) {
	cols = new_cols;
	rows = new_rows;
	scale = new_scale;
}

void clear_console() {
	cursor = {0, 0};
	clear_screen();
}

void blink_cursor_on() {
	uint64_t start_y = cursor.y * 8 * scale;
	uint64_t start_x = cursor.x * 8 * scale;
	fill_rect(0xFFFFFFFF, start_x, start_y, 8 * scale, 8 * scale);
};

void blink_cursor_off() {
	uint64_t start_y = cursor.y * 8 * scale;
	uint64_t start_x = cursor.x * 8 * scale;
	clear_rect(start_x, start_y, 8 * scale, 8 * scale);
};

void cursor_inc() {
	cursor.x++;
	if (cursor.x == cols) {
		cursor.x = 0;
		if (cursor.y == rows - 1) {
			cursor.y = 0;
			clear_console();
		} else {
			cursor.y++;
		}
	}
}
void cursor_dec() {
	cursor.x--;
}

void put_char(char character) {
	uint64_t start_y = cursor.y * 8 * scale;
	uint64_t start_x = cursor.x * 8 * scale;
	if (character == '\n') {
		for (uint32_t i = cursor.x; i < cols; i++) {
			uint64_t start_x = i * 8 * scale;
			draw_char(' ', start_y, start_x, 0xFFFFFF, scale);
		}
		cursor.x = 0;
		if (cursor.y == rows - 1) {
			cursor.y = 0;
		} else {
			cursor.y++;
		}
		if (uart_enabled) {
			uart_send('\r');
		}
	} else if (character == '\r') {
		cursor.x = 0;
	} else {
		draw_char(character, start_y, start_x, 0xFFFFFF, scale);
	}

	if (uart_enabled) {
		uart_send(character);
	}
	if (character != '\n' && character != '\r') {
		cursor.x++;
		if (cursor.x == cols) {
			cursor.x = 0;
			if (cursor.y == rows - 1) {
				cursor.y = 0;
				clear_console();
			} else {
				cursor.y++;
			}
		}
	}
}

char digits[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                   '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void number_to_str(char *buffer, uint64_t num, uint64_t base) {
	char rev_num[65] = {0};
	uint64_t number = num % base;
	uint64_t current_running_number = num - number;
	current_running_number = current_running_number / base;
	uint64_t count = 1;
	rev_num[0] = digits[number];
	// put_char(char(number + '0'));
	while (current_running_number != 0) {
		count++;
		number = current_running_number % base;
		current_running_number -= number;
		current_running_number = current_running_number / base;
		rev_num[count - 1] = digits[number];
	}
	if (count > 64) {
		count = 64;
	}
	uint64_t index = 0;
	for (uint64_t i = count + 1; i >= 1; i--) {
		if (rev_num[i - 1] != 0) {
			buffer[index] = rev_num[i - 1];
			index += 1;
		}
	}
	buffer[index] = 0;
}

uint64_t strlen(const char *str) {
	uint64_t i = 0;
	while (str[i] != 0 && i != INT64_MAX) {
		i += 1;
	}
	return i;
}

void vprintf(const char *fmt, va_list arg_list) {
	uint64_t length = strlen(fmt);
	for (uint64_t i = 0; i < length; i++) {
		if (fmt[i] == '\033' && fmt[i + 1] == '[') {
			uart_send('\033');
			uart_send('[');
			i += 2;
			if (i < length) {
				while (i < length && !(0x40 <= fmt[i] && fmt[i] <= 0x7E)) {
					uart_send(fmt[i]);
					i++;
				}
			}
			uart_send(fmt[i]);
			continue;
		} else if (fmt[i] != '%') {
			put_char(fmt[i]);
			continue;
		}
		i += 1;
		bool is_number = false;
		uint64_t number = 0;
		int radix = 0;
		char length[2] = {0, 0};
		if (fmt[i] == 'h' && fmt[i + 1] == 'h') {
			length[0] = 'h';
			length[1] = 'h';
			i += 2;
		} else if (fmt[i] == 'l' && fmt[i + 1] == 'l') {
			length[0] = 'l';
			length[1] = 'l';
			i += 2;
		} else if (fmt[i] == 'h') {
			length[0] = 'h';
			i += 1;
		} else if (fmt[i] == 'l') {
			length[0] = 'l';
			i += 1;
		} else if (fmt[i] == 'j') {
			length[0] = 'j';
			i += 1;
		} else if (fmt[i] == 'z') {
			length[0] = 'z';
			i += 1;
		} else if (fmt[i] == 't') {
			length[0] = 't';
			i += 1;
		}

		switch (fmt[i]) {
		case 's': {
			char *str = va_arg(arg_list, char *);
			if (str == nullptr) {
				str = "(null)";
			}
			uint64_t length = strlen(str);
			for (uint64_t j = 0; j < length; j++) {
				put_char(str[j]);
			}
		} break;
		case 'c': {
			put_char(va_arg(arg_list, int));
		} break;
		case '%': {
			put_char('%');
		} break;
		case 'p': {
			char buffer[64];
			number_to_str(buffer, (uint64_t)va_arg(arg_list, void *), 16);
			put_char('0');
			put_char('x');
			for (uint64_t j = 0; j < strlen(buffer); j++) {
				put_char(buffer[j]);
			}
		} break;
		case 'b': {
			is_number = true;
			radix = 2;
			if (length[0] == 'h') {
				if (length[1] == 'h') {
					number = (uint64_t)(unsigned char)(va_arg(arg_list, int));
				} else {
					number =
						(uint64_t)(unsigned short int)(va_arg(arg_list, int));
				}
			} else if (length[0] == 'l') {
				if (length[1] == 'l') {
					number =
						(uint64_t)(va_arg(arg_list, unsigned long long int));
				} else {
					number = (uint64_t)(va_arg(arg_list, unsigned long int));
				}
			} else if (length[0] == 'j') {
				number = (uint64_t)(va_arg(arg_list, uintmax_t));
			} else if (length[0] == 'z') {
				number = (uint64_t)(va_arg(arg_list, size_t));
			} else if (length[0] == 't') {
				number = (uint64_t)(va_arg(arg_list, ptrdiff_t));
			} else {
				number = (uint64_t)(va_arg(arg_list, unsigned int));
			}
		} break;
		case 'x': {
			is_number = true;
			radix = 16;
			if (length[0] == 'h') {
				if (length[1] == 'h') {
					number = (uint64_t)(unsigned char)(va_arg(arg_list, int));
				} else {
					number =
						(uint64_t)(unsigned short int)(va_arg(arg_list, int));
				}
			} else if (length[0] == 'l') {
				if (length[1] == 'l') {
					number =
						(uint64_t)(va_arg(arg_list, unsigned long long int));
				} else {
					number = (uint64_t)(va_arg(arg_list, unsigned long int));
				}
			} else if (length[0] == 'j') {
				number = (uint64_t)(va_arg(arg_list, uintmax_t));
			} else if (length[0] == 'z') {
				number = (uint64_t)(va_arg(arg_list, size_t));
			} else if (length[0] == 't') {
				number = (uint64_t)(va_arg(arg_list, ptrdiff_t));
			} else {
				number = (uint64_t)(va_arg(arg_list, unsigned int));
			}
		} break;
		case 'u': {
			is_number = true;
			radix = 10;
			if (length[0] == 'h') {
				if (length[1] == 'h') {
					number = (uint64_t)(unsigned char)(va_arg(arg_list, int));
				} else {
					number =
						(uint64_t)(unsigned short int)(va_arg(arg_list, int));
				}
			} else if (length[0] == 'l') {
				if (length[1] == 'l') {
					number =
						(uint64_t)(va_arg(arg_list, unsigned long long int));
				} else {
					number = (uint64_t)(va_arg(arg_list, unsigned long int));
				}
			} else if (length[0] == 'j') {
				number = (uint64_t)(va_arg(arg_list, uintmax_t));
			} else if (length[0] == 'z') {
				number = (uint64_t)(va_arg(arg_list, size_t));
			} else if (length[0] == 't') {
				number = (uint64_t)(va_arg(arg_list, ptrdiff_t));
			} else {
				number = (uint64_t)(va_arg(arg_list, unsigned int));
			}
		} break;
		}
		if (is_number) {
			char buffer[66];
			number_to_str(buffer, number, radix);
			for (uint64_t j = 0; j < strlen(buffer); j++) {
				put_char(buffer[j]);
			}
		}
	}
}

void printf(const char *fmt, ...) {
	va_list arg_list;
	va_start(arg_list, fmt);
	vprintf(fmt, arg_list);
	va_end(arg_list);
}

void console_set_uart_enabled() {
	uart_enabled = true;
	// printf("cols: %u, rows: %u", cols, rows);
}
