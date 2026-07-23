#ifndef INCLUDE_PANIC_HPP_
#define INCLUDE_PANIC_HPP_

void hcf();
void panic(const char *fmt, ...) __attribute__((format(__printf__, 1, 2)));

#endif // INCLUDE_PANIC_HPP_
