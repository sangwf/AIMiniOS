#include "interrupt.h"

// Add this if you don't already have it
void irq8_handler(void) {
    // Basic RTC interrupt handler
    outb(0x70, 0x0C);  // Select register C
    inb(0x71);         // Read and discard contents (required to receive further interrupts)
    
    // Send EOI (End of Interrupt)
    outb(0x20, 0x20);
    outb(0xA0, 0x20);
} 