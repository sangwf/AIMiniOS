#include "idt.h"
#include "terminal.h"
#include "rtl8139.h"
#include "io.h"

// IDT表
static struct idt_entry idt[IDT_ENTRIES];
// IDT指针
static struct idt_ptr idtp;

// 中断处理函数声明
extern void rtl8139_handler(void);

// 设置IDT表项
static void idt_set_gate(int num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].flags = flags;
}

// 加载IDT
extern void idt_load(uint32_t);

// 网卡中断处理函数
void handle_rtl8139_interrupt(void) {
    uint16_t isr = inw(get_rtl8139_iobase(rtl8139_bus, rtl8139_slot) + RTL8139_REG_ISR);
    
    terminal_writestring("RTL8139 interrupt: ");
    terminal_writehex16(isr);
    terminal_writestring("\n");

    if(isr & RTL8139_ISR_ROK) {
        terminal_writestring("Receive OK\n");
        
        uint16_t current_rx_ptr = inw(get_rtl8139_iobase(rtl8139_bus, rtl8139_slot) + RTL8139_REG_CAPR);
        terminal_writestring("CAPR: ");
        terminal_writehex16(current_rx_ptr);
        terminal_writestring("\n");
        
        uint16_t rx_buf_ptr = inw(get_rtl8139_iobase(rtl8139_bus, rtl8139_slot) + RTL8139_REG_CBR);
        terminal_writestring("CBR: ");
        terminal_writehex16(rx_buf_ptr);
        terminal_writestring("\n");
        
        check_rx_buffer();
    }
    
    if(isr & RTL8139_ISR_TOK) {
        terminal_writestring("Transmit OK\n");
    }

    // 清除中断标志
    outw(get_rtl8139_iobase(rtl8139_bus, rtl8139_slot) + RTL8139_REG_ISR, isr);
    
    rtl8139_dump_registers();
}

// 安装IDT
void idt_install(void) {
    // 设置IDT指针
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base = (uint32_t)&idt;

    // 清空IDT
    for(int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, (uint32_t)isr_default, 0x08, 0x8E);
    }

    // 注册RTL8139中断处理程序 (IRQ11)
    idt_set_gate(43, (uint32_t)rtl8139_handler, 0x08, 0x8E);

    // 加载IDT
    idt_load((uint32_t)&idtp);
    
    // 初始化PIC
    // ICW1: 初始化命令开始
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    
    // ICW2: 中断向量偏移
    outb(0x21, 0x20);  // Master PIC - IRQ0 映射到 0x20
    outb(0xA1, 0x28);  // Slave PIC - IRQ8 映射到 0x28
    
    // ICW3: 主从PIC连接
    outb(0x21, 0x04);  // Master PIC - IRQ2连接从PIC
    outb(0xA1, 0x02);  // Slave PIC - 连接到主PIC的IRQ2
    
    // ICW4: 设置8086模式
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    
    // 清除IMR，允许所有中断
    outb(0x21, 0x00);
    outb(0xA1, 0x00);
} 