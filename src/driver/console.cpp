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

void number_to_str(char *buffer, int64_t num, uint64_t base, bool is_signed) {
	char rev_num[66] = {0};
	uint64_t start_number;
	bool is_negative = false;
	if (is_signed && num < 0) {
		start_number = -num;
		is_negative = true;
	} else {
		start_number = num;
	}

	uint64_t number = start_number % base;
	uint64_t current_running_number = start_number - number;
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
	if (is_negative) {
		rev_num[count++] = '-';
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

enum LengthModifier {
	NORMAL_LENGTH = 0,
	CHAR_LENGTH,
	SHORT_LENGTH,
	LONG_LENGTH,
	LONG_LONG_LENGTH,
	MAX_LENGTH,
	SIZE_LENGTH,
	PTRDIFF_LENGTH,
};

enum FormatFlags {
	ALTERNATE_FLAG = (1ul << 0),
	ZERO_PADDING_FLAG = (1ul << 1),
	LEFT_ALIGNED_FLAG = (1ul << 2),
	BLANK_POSITIVE_SIGN_FLAG = (1ul << 3),
	SIGN_FLAG = (1ul << 4),
};

struct FormatState {
	uint64_t width;
	uint64_t precision;
	enum LengthModifier length;
	uint8_t /*enum FormatFlags*/ flags;
	bool is_number;
	bool has_precision;
	bool is_signed;
	uint8_t radix;
};

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
		} else if (((i + 1) < length) && fmt[i + 1] == '%') {
			put_char('%');
			i += 1;
			continue;
		}
		i += 1;
		FormatState state = {.width = 0,
		                     .precision = 0,
		                     .length = NORMAL_LENGTH,
		                     .flags = 0,
		                     .is_number = 0,
		                     .has_precision = 0,
		                     .is_signed = 0,
		                     .radix = 0};

		// flags
		bool flag_found = false;
		do {
			flag_found = false;
			if (i < length && fmt[i] == '#') {
				state.flags |= ALTERNATE_FLAG;
				flag_found = true;
				i++;
			} else if (!(i < length)) {
				continue;
			}

			if (i < length && fmt[i] == '0') {
				state.flags |= ZERO_PADDING_FLAG;
				flag_found = true;
				i++;
			} else if (!(i < length)) {
				continue;
			}

			if (i < length && fmt[i] == '-') {
				state.flags |= LEFT_ALIGNED_FLAG;
				flag_found = true;
				i++;
			} else if (!(i < length)) {
				continue;
			}

			if (i < length && fmt[i] == ' ') {
				state.flags |= BLANK_POSITIVE_SIGN_FLAG;
				flag_found = true;
				i++;
			} else if (!(i < length)) {
				continue;
			}

			if (i < length && fmt[i] == '+') {
				state.flags |= SIGN_FLAG;
				flag_found = true;
				i++;
			} else if (!(i < length)) {
				continue;
			}
		} while (flag_found);
		// width
		if (i < length && fmt[i] >= '0' && fmt[i] <= '9') {
			while (fmt[i] && fmt[i] >= '0' && fmt[i] <= '9' && i < length) {
				state.width = state.width * 10 + (fmt[i] - '0');
				i++;
			}
		} else if (!(i < length)) {
			continue;
		}
		//.percision
		if (i < length && fmt[i] == '.') {
			i++;
			if (i < length && fmt[i] != '-') {
				state.has_precision = true;
				while (fmt[i] && fmt[i] >= '0' && fmt[i] <= '9' && i < length) {
					state.precision = state.precision * 10 + (fmt[i] - '0');
					i++;
				}
			} else {
				i++;
				while (fmt[i] && fmt[i] >= '0' && fmt[i] <= '9' && i < length) {
					i++;
				}
			}
		} else if (!(i < length)) {
			continue;
		}

		// length
		if ((i + 1 < length) && fmt[i] == 'h' && fmt[i + 1] == 'h') {
			state.length = CHAR_LENGTH;
			i += 2;
		} else if ((i + 1 < length) && fmt[i] == 'l' && fmt[i + 1] == 'l') {
			state.length = LONG_LONG_LENGTH;
			i += 2;
		} else if ((i + 1 < length) && fmt[i] == 'h') {
			state.length = SHORT_LENGTH;
			i += 1;
		} else if ((i + 1 < length) && fmt[i] == 'l') {
			state.length = LONG_LENGTH;
			i += 1;
		} else if ((i + 1 < length) && fmt[i] == 'j') {
			state.length = MAX_LENGTH;
			i += 1;
		} else if ((i + 1 < length) && fmt[i] == 'z') {
			state.length = SIZE_LENGTH;
			i += 1;
		} else if ((i + 1 < length) && fmt[i] == 't') {
			state.length = PTRDIFF_LENGTH;
			i += 1;
		}

		switch (fmt[i]) {
		case 's': {
			const char *str = va_arg(arg_list, char *);
			if (str == nullptr &&
			    ((state.has_precision && state.precision >= 6) ||
			     !state.has_precision)) {
				str = "(null)";
			} else if (str == nullptr) {
				str = "";
			}
			uint64_t length = strlen(str);
			if (state.has_precision && length > state.precision) {
				length = state.precision;
			}
			uint64_t padding =
				int64_t(state.width - length) > 0 ? (state.width - length) : 0;
			for (uint64_t j = 0;
			     j < padding && !(state.flags & LEFT_ALIGNED_FLAG); j++) {
				put_char(' ');
			}
			for (uint64_t j = 0; j < length; j++) {
				put_char(str[j]);
			}
			for (uint64_t j = 0;
			     j < padding && (state.flags & LEFT_ALIGNED_FLAG); j++) {
				put_char(' ');
			}
		} break;
		case 'c': {
			uint64_t padding =
				(int64_t(state.width) - 1) > 0 ? (state.width - 1) : 0;
			for (uint64_t j = 0;
			     j < padding && !(state.flags & LEFT_ALIGNED_FLAG); j++) {
				put_char(' ');
			}
			put_char(va_arg(arg_list, int));
			for (uint64_t j = 0;
			     j < padding && (state.flags & LEFT_ALIGNED_FLAG); j++) {
				put_char(' ');
			}
		} break;
		case 'p': {
			char buffer[66];
			number_to_str(buffer, (uint64_t)va_arg(arg_list, void *), 16,
			              false);
			put_char('0');
			put_char('x');
			uint64_t length = strlen(buffer);
			for (uint64_t j = 0; j < length; j++) {
				put_char(buffer[j]);
			}
		} break;
		case 'b': {
			state.is_number = true;
			state.radix = 2;
			if (state.flags & ALTERNATE_FLAG) {
				put_char('0');
				put_char('b');
			}
		} break;
		case 'x': {
			state.is_number = true;
			state.radix = 16;
			if (state.flags & ALTERNATE_FLAG) {
				put_char('0');
				put_char('x');
			}
		} break;
		case 'u': {
			state.is_number = true;
			state.radix = 10;
		} break;
		case 'd': {
			state.is_number = true;
			state.radix = 10;
			state.is_signed = true;
		} break;
		case 'i': {
			state.is_number = true;
			state.radix = 10;
			state.is_signed = true;
		} break;
		}
		if (state.is_number) {
			char buffer[66];
			if (state.is_signed) {
				int64_t number;
				if (state.length == CHAR_LENGTH) {
					number =
						(int64_t)(signed char)(va_arg(arg_list, signed int));
				} else if (state.length == SHORT_LENGTH) {
					number =
						(int64_t)(signed short)(va_arg(arg_list, signed int));
				} else if (state.length == LONG_LONG_LENGTH) {
					number = (int64_t)(signed long long)(va_arg(
						arg_list, signed long long int));
				} else if (state.length == LONG_LENGTH) {
					number = (int64_t)(signed long)(va_arg(arg_list,
					                                       signed long int));
				} else if (state.length == MAX_LENGTH) {
					number = (int64_t)(intmax_t)(va_arg(arg_list, intmax_t));
				} else if (state.length == SIZE_LENGTH) {
					number = (int64_t)(va_arg(arg_list, signed long int));
				} else if (state.length == PTRDIFF_LENGTH) {
					number = (int64_t)(va_arg(arg_list, ptrdiff_t));
				} else {
					number =
						(int64_t)(signed int)(va_arg(arg_list, signed int));
				}
				number_to_str(buffer, number, state.radix, true);
			} else {
				uint64_t number;
				if (state.length == CHAR_LENGTH) {
					number = (uint64_t)(unsigned char)(va_arg(arg_list,
					                                          unsigned int));
				} else if (state.length == SHORT_LENGTH) {
					number = (uint64_t)(unsigned short)(va_arg(arg_list,
					                                           unsigned int));
				} else if (state.length == LONG_LONG_LENGTH) {
					number = (uint64_t)(unsigned long long)(va_arg(
						arg_list, unsigned long long int));
				} else if (state.length == LONG_LENGTH) {
					number = (uint64_t)(unsigned long)(va_arg(
						arg_list, unsigned long int));
				} else if (state.length == MAX_LENGTH) {
					number = (uint64_t)(intmax_t)(va_arg(arg_list, uintmax_t));
				} else if (state.length == SIZE_LENGTH) {
					number = (uint64_t)(va_arg(arg_list, size_t));
				} else if (state.length == PTRDIFF_LENGTH) {
					number = (uint64_t)(va_arg(arg_list, ptrdiff_t));
				} else {
					number = (uint64_t)(unsigned int)(va_arg(arg_list,
					                                         unsigned int));
				}
				number_to_str(buffer, number, state.radix, false);
			}
			char padding_char = ' ';
			if ((state.flags & ZERO_PADDING_FLAG) &&
			    !(state.flags & LEFT_ALIGNED_FLAG)) {
				padding_char = '0';
			}
			uint64_t length = strlen(buffer);
			bool is_negative = buffer[0] == '-';
			uint64_t digit_count = is_negative ? length - 1 : length;
			uint64_t width =
				state.has_precision && digit_count < state.precision
					? state.precision + (is_negative ? 1 : 0)
					: length;
			if ((state.flags & BLANK_POSITIVE_SIGN_FLAG) &&
			    !(state.flags & LEFT_ALIGNED_FLAG) && !is_negative) {
				put_char(' ');
				width += 1;
			}
			if ((state.flags & SIGN_FLAG) && !is_negative) {
				put_char('+');
				width += 1;
			}
			if (!(state.flags & LEFT_ALIGNED_FLAG) &&
			    (padding_char == ' ' || state.has_precision)) {
				while (width < state.width) {
					put_char(' ');
					width++;
				}
			}

			if (buffer[0] == '-') {
				put_char('-');
				while ((!state.has_precision && width < state.width) ||
				       digit_count < state.precision) {
					put_char('0');
					width++;
					digit_count++;
				}
				for (uint64_t j = 1; j < length; j++) {
					put_char(buffer[j]);
				}
			} else {
				while ((!state.has_precision && width < state.width) ||
				       digit_count < state.precision) {
					put_char('0');
					width++;
					digit_count++;
				}
				for (uint64_t j = 0; j < length; j++) {
					put_char(buffer[j]);
				}
			}

			if ((state.flags & LEFT_ALIGNED_FLAG) && padding_char == ' ') {
				while (width < state.width) {
					put_char(padding_char);
					width++;
				}
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
