#include "types.h"
#include "terminal.h"

__attribute__((interrupt))
void irq8_handler(void* frame) {
    terminal_writestring("IRQ 8 received\n");
    // 发送 EOI (End of Interrupt)
    outb(0x20, 0x20);
    if(irq > 7) {
        outb(0xA0, 0x20);
    }
}
