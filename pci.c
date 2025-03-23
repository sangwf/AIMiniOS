#include "pci.h"
#include "io.h"
#include "terminal.h"
#include "rtl8139.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

// 生成PCI配置地址
static uint32_t pci_get_address(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    return (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
}

// 读取PCI配置空间的16位值
uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = pci_get_address(bus, slot, func, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    return (uint16_t)((inl(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
}

// 写入PCI配置空间的16位值
void pci_config_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value) {
    uint32_t address = pci_get_address(bus, slot, func, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value << ((offset & 2) * 8));
}

// 读取PCI配置空间的32位值
uint32_t pci_config_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = pci_get_address(bus, slot, func, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

// 写入PCI配置空间的32位值
void pci_config_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address = pci_get_address(bus, slot, func, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

// 检查设备是否存在
static bool pci_device_exists(uint8_t bus, uint8_t slot, uint8_t func) {
    uint16_t vendor = pci_config_read_word(bus, slot, func, PCI_VENDOR_ID);
    return vendor != 0xFFFF;
}

// 获取设备类型信息
static void pci_get_device_info(uint8_t bus, uint8_t slot, uint8_t func) {
    uint16_t vendor = pci_config_read_word(bus, slot, func, PCI_VENDOR_ID);
    uint16_t device = pci_config_read_word(bus, slot, func, PCI_DEVICE_ID);
    uint8_t class_code = pci_config_read_word(bus, slot, func, PCI_CLASS) & 0xFF;
    uint8_t subclass = pci_config_read_word(bus, slot, func, PCI_SUBCLASS) & 0xFF;
    
    terminal_writestring("Found device at ");
    terminal_writestring("Bus: ");
    print_hex(bus);
    terminal_writestring(" Slot: ");
    print_hex(slot);
    terminal_writestring(" Function: ");
    print_hex(func);
    terminal_writestring("\n");
    
    terminal_writestring("Vendor ID: ");
    print_hex(vendor);
    terminal_writestring(" Device ID: ");
    print_hex(device);
    terminal_writestring("\n");
    
    terminal_writestring("Class: ");
    print_hex(class_code);
    terminal_writestring(" Subclass: ");
    print_hex(subclass);
    terminal_writestring("\n");
    
    if(vendor == RTL8139_VENDOR_ID && device == RTL8139_DEVICE_ID) {
        terminal_writestring("*** RTL8139 Network Card Found! ***\n");
    }
}

// 初始化PCI总线
void pci_init(void) {
    terminal_writestring("\n=== PCI Bus Initialization ===\n");
    
    // 检查PCI总线是否存在
    uint32_t tmp = pci_config_read_dword(0, 0, 0, 0);
    if(tmp == 0xFFFFFFFF) {
        terminal_writestring("ERROR: No PCI bus found!\n");
        return;
    }
    
    terminal_writestring("PCI bus detected\n");
    terminal_writestring("Scanning PCI devices...\n");
    
    // 扫描所有PCI设备
    for(uint8_t bus = 0; bus < 8; bus++) {
        for(uint8_t slot = 0; slot < 32; slot++) {
            for(uint8_t func = 0; func < 8; func++) {
                if(pci_device_exists(bus, slot, func)) {
                    pci_get_device_info(bus, slot, func);
                }
            }
        }
    }
    
    terminal_writestring("=== PCI Bus Initialization Complete ===\n\n");
}

// 查找指定的PCI设备
int pci_find_device(uint16_t vendor_id, uint16_t device_id, uint16_t* bus, uint16_t* slot) {
    for(uint8_t b = 0; b < 8; b++) {
        for(uint8_t s = 0; s < 32; s++) {
            uint16_t v = pci_config_read_word(b, s, 0, PCI_VENDOR_ID);
            if(v == vendor_id) {
                uint16_t d = pci_config_read_word(b, s, 0, PCI_DEVICE_ID);
                if(d == device_id) {
                    *bus = b;
                    *slot = s;
                    return 1;
                }
            }
        }
    }
    return 0;
}

// 获取设备的I/O基地址
uint16_t pci_get_iobase(uint16_t bus, uint16_t slot) {
    uint32_t bar0 = pci_config_read_dword(bus, slot, 0, PCI_BAR0);
    if(bar0 & 1) {  // 检查是否是I/O空间
        // 启用设备的总线主控和I/O空间访问
        uint16_t command = pci_config_read_word(bus, slot, 0, PCI_COMMAND);
        command |= (PCI_COMMAND_IO | PCI_COMMAND_MASTER | PCI_COMMAND_PERR);
        pci_config_write_word(bus, slot, 0, PCI_COMMAND, command);
        
        // 获取并显示中断线路
        uint8_t interrupt_line = pci_config_read_word(bus, slot, 0, PCI_INTERRUPT_LINE) & 0xFF;
        terminal_writestring("Device interrupt line: ");
        print_hex(interrupt_line);
        terminal_writestring("\n");
        
        // 验证命令寄存器设置
        command = pci_config_read_word(bus, slot, 0, PCI_COMMAND);
        terminal_writestring("PCI Command register: ");
        print_hex(command);
        terminal_writestring("\n");
        
        return (uint16_t)(bar0 & ~3);
    }
    return 0;
}

// 配置设备的中断
void pci_configure_interrupt(uint16_t bus, uint16_t slot, uint8_t interrupt_line) {
    // 设置中断线路
    pci_config_write_word(bus, slot, 0, PCI_INTERRUPT_LINE, interrupt_line);
    
    terminal_writestring("Configured interrupt line: ");
    print_hex(interrupt_line);
    terminal_writestring("\n");
} 