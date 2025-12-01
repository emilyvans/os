#ifndef INCLUDE_CONSOLE_HPP_
#define INCLUDE_CONSOLE_HPP_
#include <stdarg.h>
#include <stdint.h>

void init_console(uint64_t new_cols, uint64_t new_rows, uint64_t new_scale);
void clear_console();
uint64_t strlen(const char *str);
void number(char *buffer, uint64_t num, uint64_t base = 10);
void put_char(char c);
void vprintf(const char *fmt, va_list arg_list);
void printf(const char *fmt, ...);
void console_set_uart_enabled();

#endif // INCLUDE_CONSOLE_HPP_
