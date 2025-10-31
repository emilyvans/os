#include "console.hpp"
#include "screen.hpp"
#include <stdint.h>

struct Point {
	uint64_t x;
	uint64_t y;
};

uint64_t cols = 0;
uint64_t rows = 0;
Point cursor = {0, 0};
uint64_t scale = 2;

void init_console(uint64_t new_cols, uint64_t new_rows, uint64_t new_scale) {
	cols = new_cols;
	rows = new_rows;
	scale = new_scale;
}

void clear_console() {
	cursor = {0, 0};
	clear_screen();
}

void put_char(char character) {
	uint64_t start_y = cursor.y * 8 * scale;
	uint64_t start_x = cursor.x * 8 * scale;
	draw_char(character, start_y, start_x, 0xFFFFFF, scale);

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

char digits[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                   '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void number(char *buffer, uint64_t num, uint64_t base) {
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
	for (uint64_t i = 0; i < strlen(fmt); i++) {
		if (fmt[i] != '%') {
			if (fmt[i] == '\n') {
				cursor.x = 0;
				if (cursor.y == rows - 1) {
					cursor.y = 0;
				} else {
					cursor.y++;
				}
			} else {
				put_char(fmt[i]);
			}
			continue;
		}
		i += 1;
		switch (fmt[i]) {
		case 's': {
			char *str = va_arg(arg_list, char *);
			for (uint64_t j = 0; j < strlen(str); j++) {
				put_char(str[j]);
			}
		} break;
		case 'c': {
			put_char(va_arg(arg_list, int));
		} break;
		case '%': {
			put_char('%');
		} break;
		case 'b': {
			char buffer[64];
			number(buffer, va_arg(arg_list, uint64_t), 2);
			for (uint64_t j = 0; j < strlen(buffer); j++) {
				put_char(buffer[j]);
			}
		} break;
		case 'x': {
			char buffer[64];
			number(buffer, va_arg(arg_list, uint64_t), 16);
			for (uint64_t j = 0; j < strlen(buffer); j++) {
				put_char(buffer[j]);
			}
		} break;
		case 'p': {
			char buffer[64];
			number(buffer, va_arg(arg_list, uint64_t), 16);
			for (uint64_t j = 0; j < strlen(buffer); j++) {
				put_char(buffer[j]);
			}
		} break;
		case 'u': {
			char buffer[64];
			number(buffer, va_arg(arg_list, uint64_t), 10);
			for (uint64_t j = 0; j < strlen(buffer); j++) {
				put_char(buffer[j]);
			}
		} break;
		}
	}
};

void printf(const char *fmt, ...) {
	va_list arg_list;
	va_start(arg_list, fmt);
	vprintf(fmt, arg_list);
	va_end(arg_list);
}
