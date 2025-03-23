#include "terminal.h"
#include "types.h"

// VGA文本模式的显存地址
#define VGA_MEMORY 0xB8000
// VGA文本模式的宽度和高度
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// 当前光标位置
static uint16_t cursor_x = 0;
static uint16_t cursor_y = 0;

// 字符颜色
static uint8_t make_color(uint8_t fg, uint8_t bg) {
    return fg | bg << 4;
}

// VGA字符项
static uint16_t make_vgaentry(char c, uint8_t color) {
    uint16_t c16 = c;
    uint16_t color16 = color;
    return c16 | color16 << 8;
}

// 清屏
void terminal_clear(void) {
    uint8_t color = make_color(15, 0); // 白字黑底
    uint16_t* vga = (uint16_t*)VGA_MEMORY;
    
    for(int y = 0; y < VGA_HEIGHT; y++) {
        for(int x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga[index] = make_vgaentry(' ', color);
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

// 输出字符
void terminal_putchar(char c) {
    uint8_t color = make_color(15, 0); // 白字黑底
    uint16_t* vga = (uint16_t*)VGA_MEMORY;
    
    if(c == '\n') {
        cursor_x = 0;
        cursor_y++;
        if(cursor_y >= VGA_HEIGHT) {
            // 滚动屏幕
            for(int y = 0; y < VGA_HEIGHT - 1; y++) {
                for(int x = 0; x < VGA_WIDTH; x++) {
                    vga[y * VGA_WIDTH + x] = vga[(y + 1) * VGA_WIDTH + x];
                }
            }
            // 清除最后一行
            for(int x = 0; x < VGA_WIDTH; x++) {
                vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = make_vgaentry(' ', color);
            }
            cursor_y = VGA_HEIGHT - 1;
        }
        return;
    }
    
    const size_t index = cursor_y * VGA_WIDTH + cursor_x;
    vga[index] = make_vgaentry(c, color);
    
    if(++cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
        if(cursor_y >= VGA_HEIGHT) {
            // 滚动屏幕
            for(int y = 0; y < VGA_HEIGHT - 1; y++) {
                for(int x = 0; x < VGA_WIDTH; x++) {
                    vga[y * VGA_WIDTH + x] = vga[(y + 1) * VGA_WIDTH + x];
                }
            }
            // 清除最后一行
            for(int x = 0; x < VGA_WIDTH; x++) {
                vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = make_vgaentry(' ', color);
            }
            cursor_y = VGA_HEIGHT - 1;
        }
    }
}

// 输出字符串
void terminal_writestring(const char* str) {
    for(size_t i = 0; str[i] != '\0'; i++) {
        terminal_putchar(str[i]);
    }
}

// 打印十六进制数
void print_hex(uint32_t num) {
    static const char hex_chars[] = "0123456789ABCDEF";
    char buffer[11];  // "0x" + 8 digits + null terminator
    buffer[0] = '0';
    buffer[1] = 'x';
    buffer[9] = '\0';
    
    for(int i = 7; i >= 0; i--) {
        buffer[i + 2] = hex_chars[num & 0xF];
        num >>= 4;
    }
    
    terminal_writestring(buffer);
}

// 添加新的调试输出函数
void debug_log(const char* message) {
    terminal_writestring("[DEBUG] ");
    terminal_writestring(message);
}

void debug_log_hex(const char* prefix, uint32_t value) {
    terminal_writestring("[DEBUG] ");
    terminal_writestring(prefix);
    terminal_writestring(": ");
    print_hex(value);
    terminal_writestring("\n");
}

void terminal_writedec(uint32_t n) {
    if (n == 0) {
        terminal_putchar('0');
        return;
    }

    char buf[32];
    int i = 0;
    
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n = n / 10;
    }
    
    while (--i >= 0) {
        terminal_putchar(buf[i]);
    }
}

void terminal_writehex(uint32_t n) {
    int i;
    int leading_zero = 1;
    
    terminal_writestring("0x");
    
    for (i = 28; i >= 0; i -= 4) {
        int digit = (n >> i) & 0xF;
        if (digit == 0 && leading_zero && i != 0) {
            continue;
        }
        leading_zero = 0;
        if (digit < 10) {
            terminal_putchar('0' + digit);
        } else {
            terminal_putchar('a' + (digit - 10));
        }
    }
    
    if (leading_zero) {
        terminal_putchar('0');
    }
}

void terminal_writehex16(uint16_t num) {
    terminal_writestring("0x");
    for (int i = 12; i >= 0; i -= 4) {
        uint8_t digit = (num >> i) & 0xF;
        terminal_putchar(digit < 10 ? '0' + digit : 'A' + digit - 10);
    }
}

void terminal_writehex8(uint8_t num) {
    terminal_writestring("0x");
    for (int i = 4; i >= 0; i -= 4) {
        uint8_t digit = (num >> i) & 0xF;
        terminal_putchar(digit < 10 ? '0' + digit : 'A' + digit - 10);
    }
}

// 写入32位十六进制数
void terminal_writehex32(uint32_t num) {
    terminal_writestring("0x");
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t digit = (num >> i) & 0xF;
        if (digit < 10) {
            terminal_putchar('0' + digit);
        } else {
            terminal_putchar('A' + digit - 10);
        }
    }
}

// 以十进制形式打印整数
void terminal_writeint(int value, int base) {
    if (value < 0) {
        terminal_writestring("-");
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
        terminal_writestring(buf);
    } else {
        terminal_writehex32(value);
    }
}

// 初始化终端
void terminal_initialize(void) {
    uint8_t color = make_color(15, 0); // 白字黑底
    uint16_t* vga = (uint16_t*)VGA_MEMORY;
    
    for(int y = 0; y < VGA_HEIGHT; y++) {
        for(int x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga[index] = make_vgaentry(' ', color);
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}
 