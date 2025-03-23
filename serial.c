#include "serial.h"
#include "terminal.h"
#include "io.h"

// COM1串行端口地址
#define COM1 0x3F8

// 初始化串行端口
void serial_init(void) {
   // 禁用所有中断
   outb(COM1 + 1, 0x00);
   
   // 设置波特率
   outb(COM1 + 3, 0x80);    // 设置DLAB位
   outb(COM1 + 0, 0x03);    // 设置除数的低位: 38400 baud
   outb(COM1 + 1, 0x00);    // 设置除数的高位
   
   // 8位数据位, 无奇偶校验, 1位停止位
   outb(COM1 + 3, 0x03);
   
   // 启用FIFO，清除接收和发送FIFO，设置14字节阈值
   outb(COM1 + 2, 0xC7);
   
   // 启用IRQ，设置回环模式以进行测试
   outb(COM1 + 4, 0x0B);
   
   // 测试串行芯片（设置回环模式）
   outb(COM1 + 4, 0x1E);
   outb(COM1 + 0, 0xAE);   // 发送测试字节
   
   // 检查是否收到了测试字节
   if(inb(COM1 + 0) != 0xAE) {
      // 串行端口测试失败
      return;
   }
   
   // 实际使用时，关闭回环模式，启用中断
   outb(COM1 + 4, 0x0F);
}

// 检查发送缓冲区是否为空
int serial_is_transmit_empty(void) {
   return inb(COM1 + 5) & 0x20;
}

// 写一个字符到串行端口
void serial_putc(char c) {
   // 等待传输缓冲区为空
   while (!(inb(COM1 + 5) & 0x20));
   
   // 发送字符
   outb(COM1, c);
   
   // 如果是换行，自动添加回车符
   if (c == '\n') {
      serial_putc('\r');
   }
}

// 写字符串到串行端口
void serial_write_string(const char* str) {
    while (*str) {
        serial_putc(*str);
        str++;
    }
}

// 简化的十六进制转换
static const char* hex_digits = "0123456789ABCDEF";

// 写8位十六进制数
void serial_write_hex8(uint8_t value) {
   serial_putc('0');
   serial_putc('x');
   serial_putc(hex_digits[(value >> 4) & 0xF]);
   serial_putc(hex_digits[value & 0xF]);
}

// Alias for serial_write_hex8 for compatibility
void serial_write_hex_byte(uint8_t value) {
   serial_write_hex8(value);
}

// 写16位十六进制数
void serial_write_hex16(uint16_t value) {
    serial_write_string("0x");
    serial_putc(hex_digits[(value >> 12) & 0xF]);
    serial_putc(hex_digits[(value >> 8) & 0xF]);
    serial_putc(hex_digits[(value >> 4) & 0xF]);
    serial_putc(hex_digits[value & 0xF]);
}

// 写32位十六进制数
void serial_write_hex32(uint32_t value) {
   serial_putc('0');
   serial_putc('x');
   serial_putc(hex_digits[(value >> 28) & 0xF]);
   serial_putc(hex_digits[(value >> 24) & 0xF]);
   serial_putc(hex_digits[(value >> 20) & 0xF]);
   serial_putc(hex_digits[(value >> 16) & 0xF]);
   serial_putc(hex_digits[(value >> 12) & 0xF]);
   serial_putc(hex_digits[(value >> 8) & 0xF]);
   serial_putc(hex_digits[(value >> 4) & 0xF]);
   serial_putc(hex_digits[value & 0xF]);
}

// 写十进制数字
void serial_write_dec(uint32_t value) {
   char buffer[12]; // 32位整数最大11位（包括负号）
   char *p = buffer + 11;
   *p = '\0';
   
   do {
      *--p = '0' + (value % 10);
      value /= 10;
   } while (value);
   
   serial_write_string(p);
}

// 输出IP地址
void serial_print_ip(uint32_t ip) {
   char buffer[4];
   
   // Print first octet
   buffer[0] = '0' + ((ip >> 24) & 0xFF) / 100;
   buffer[1] = '0' + (((ip >> 24) & 0xFF) / 10) % 10;
   buffer[2] = '0' + ((ip >> 24) & 0xFF) % 10;
   buffer[3] = 0;
   if (buffer[0] == '0') {
       if (buffer[1] == '0') {
           serial_putc(buffer[2]);
       } else {
           serial_putc(buffer[1]);
           serial_putc(buffer[2]);
       }
   } else {
       serial_putc(buffer[0]);
       serial_putc(buffer[1]);
       serial_putc(buffer[2]);
   }
   
   serial_putc('.');
   
   // Print second octet
   buffer[0] = '0' + ((ip >> 16) & 0xFF) / 100;
   buffer[1] = '0' + (((ip >> 16) & 0xFF) / 10) % 10;
   buffer[2] = '0' + ((ip >> 16) & 0xFF) % 10;
   if (buffer[0] == '0') {
       if (buffer[1] == '0') {
           serial_putc(buffer[2]);
       } else {
           serial_putc(buffer[1]);
           serial_putc(buffer[2]);
       }
   } else {
       serial_putc(buffer[0]);
       serial_putc(buffer[1]);
       serial_putc(buffer[2]);
   }
   
   serial_putc('.');
   
   // Print third octet
   buffer[0] = '0' + ((ip >> 8) & 0xFF) / 100;
   buffer[1] = '0' + (((ip >> 8) & 0xFF) / 10) % 10;
   buffer[2] = '0' + ((ip >> 8) & 0xFF) % 10;
   if (buffer[0] == '0') {
       if (buffer[1] == '0') {
           serial_putc(buffer[2]);
       } else {
           serial_putc(buffer[1]);
           serial_putc(buffer[2]);
       }
   } else {
       serial_putc(buffer[0]);
       serial_putc(buffer[1]);
       serial_putc(buffer[2]);
   }
   
   serial_putc('.');
   
   // Print fourth octet
   buffer[0] = '0' + (ip & 0xFF) / 100;
   buffer[1] = '0' + ((ip & 0xFF) / 10) % 10;
   buffer[2] = '0' + (ip & 0xFF) % 10;
   if (buffer[0] == '0') {
       if (buffer[1] == '0') {
           serial_putc(buffer[2]);
       } else {
           serial_putc(buffer[1]);
           serial_putc(buffer[2]);
       }
   } else {
       serial_putc(buffer[0]);
       serial_putc(buffer[1]);
       serial_putc(buffer[2]);
   }
}

// 以十进制形式打印整数到串口
void serial_write_int(int value, int base) {
    if (value < 0) {
        serial_write_string("-");
        value = -value;
    }
    
    if (base == 10) {
        char buf[32];
        int i = 0;
        if (value == 0) {
            buf[i++] = '0';
        } else {
            while (value > 0) {
                buf[i++] = (value % 10) + '0';
                value /= 10;
            }
        }
        
        // 反转缓冲区
        for (int j = 0; j < i / 2; j++) {
            char temp = buf[j];
            buf[j] = buf[i - j - 1];
            buf[i - j - 1] = temp;
        }
        
        buf[i] = '\0';
        serial_write_string(buf);
    } else {
        char hex_digits[] = "0123456789ABCDEF";
        char buf[32];
        int i = 0;
        
        if (value == 0) {
            buf[i++] = '0';
        } else {
            while (value > 0) {
                buf[i++] = hex_digits[value % base];
                value /= base;
            }
        }
        
        // 反转缓冲区
        for (int j = 0; j < i / 2; j++) {
            char temp = buf[j];
            buf[j] = buf[i - j - 1];
            buf[i - j - 1] = temp;
        }
        
        buf[i] = '\0';
        serial_write_string(buf);
    }
}
