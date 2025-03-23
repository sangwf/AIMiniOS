#ifndef IDT_H
#define IDT_H

#include "types.h"

// IDT 入口结构
struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

// IDTR 指针结构
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// IDT 表项数量
#define IDT_ENTRIES 256

// 声明IDT安装函数
void idt_install(void);

// 声明默认中断处理函数
extern void isr_default(void);

#endif 