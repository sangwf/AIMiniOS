#include "gdt.h"

// GDT表
static struct gdt_entry gdt[GDT_ENTRIES];
// GDT指针
static struct gdt_ptr gp;

// GDT设置函数
static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    // 设置基地址
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    // 设置段界限
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);

    // 设置粒度位和其他标志
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}

// 加载GDT
extern void gdt_flush(uint32_t);

// 安装GDT
void gdt_install(void) {
    // 设置GDT指针和限长
    gp.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gp.base = (uint32_t)&gdt;

    // NULL描述符
    gdt_set_gate(0, 0, 0, 0, 0);

    // 代码段: base=0, limit=4GB, 4KB blocks, 32-bit opcodes
    gdt_set_gate(1, 0, 0xFFFFFFFF,
        0x9A,    // Present=1, Ring=0, Code
        0xCF);   // 4KB blocks, 32-bit

    // 数据段: base=0, limit=4GB, 4KB blocks, read/write
    gdt_set_gate(2, 0, 0xFFFFFFFF,
        0x92,    // Present=1, Ring=0, Data
        0xCF);   // 4KB blocks, 32-bit

    // 用户模式代码段
    gdt_set_gate(3, 0, 0xFFFFFFFF,
        0xFA,    // Present=1, Ring=3, Code
        0xCF);   // 4KB blocks, 32-bit

    // 用户模式数据段
    gdt_set_gate(4, 0, 0xFFFFFFFF,
        0xF2,    // Present=1, Ring=3, Data
        0xCF);   // 4KB blocks, 32-bit

    // 刷新GDT
    gdt_flush((uint32_t)&gp);
} 