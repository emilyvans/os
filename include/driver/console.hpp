#ifndef INCLUDE_DRIVER_CONSOLE_HPP_
#define INCLUDE_DRIVER_CONSOLE_HPP_
#include <stdarg.h>
#include <stdint.h>

void init_console(uint64_t new_cols, uint64_t new_rows, uint64_t new_scale);
void clear_console();
uint64_t strlen(const char *str);
void number_to_str(char *buffer, uint64_t num, uint64_t base = 10);
void put_char(char c);
void vprintf(const char *fmt, va_list arg_list)
	__attribute__((format(__printf__, 1, 0)));
void printf(const char *fmt, ...) __attribute__((format(__printf__, 1, 2)));
void console_set_uart_enabled();
void blink_cursor_off();
void blink_cursor_on();
void cursor_dec();
void cursor_inc();

#endif // INCLUDE_DRIVER_CONSOLE_HPP_
