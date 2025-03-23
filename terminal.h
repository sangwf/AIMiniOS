#ifndef TERMINAL_H
#define TERMINAL_H

#include "types.h"

void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);
void terminal_writedec(uint32_t n);
void terminal_writehex(uint32_t n);
void terminal_writehex8(uint8_t value);
void terminal_writehex16(uint16_t value);
void terminal_writehex32(uint32_t value);
void terminal_writeint(int value, int base);
void terminal_clear(void);
void debug_log(const char* message);
void debug_log_hex(const char* prefix, uint32_t value);
void print_hex(uint32_t num);

#endif // TERMINAL_H 