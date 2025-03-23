#ifndef GDT_H
#define GDT_H

#include "types.h"

// GDT 入口结构
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

// GDTR 指针结构
struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// GDT 表项数量
#define GDT_ENTRIES 5

// 声明GDT安装函数
void gdt_install(void);

#endif 