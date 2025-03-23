#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"

// 串口端口
#define SERIAL_COM1_PORT 0x3F8

// 串口初始化 115200 8N1
void serial_init();

// 重定向到串口
void serial_putc(char c);
void serial_write_string(const char* str);
void serial_write_hex(uint32_t value);
void serial_write_hex8(uint8_t value);
void serial_write_hex_byte(uint8_t value);
void serial_write_hex16(uint16_t value);
void serial_write_dec(uint32_t value);
void serial_write_int(int value, int base);

// 调试输出
void print_byte_as_bits(uint8_t val);
void print_bytes_as_hex(uint8_t* data, uint16_t length);
void dump_registers();

// 输出IP地址
void serial_print_ip(uint32_t ip);

#endif /* SERIAL_H */
