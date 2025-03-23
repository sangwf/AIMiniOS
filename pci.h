#ifndef PCI_H
#define PCI_H

#include "types.h"

// PCI配置空间寄存器
#define PCI_VENDOR_ID      0x00
#define PCI_DEVICE_ID      0x02
#define PCI_COMMAND        0x04
#define PCI_STATUS         0x06
#define PCI_REVISION_ID    0x08
#define PCI_PROG_IF        0x09
#define PCI_SUBCLASS       0x0A
#define PCI_CLASS          0x0B
#define PCI_CACHE_LINE     0x0C
#define PCI_LATENCY        0x0D
#define PCI_HEADER_TYPE    0x0E
#define PCI_BIST           0x0F
#define PCI_BAR0           0x10
#define PCI_BAR1           0x14
#define PCI_BAR2           0x18
#define PCI_BAR3           0x1C
#define PCI_BAR4           0x20
#define PCI_BAR5           0x24
#define PCI_CARDBUS_CIS    0x28
#define PCI_SUBSYS_VENDOR  0x2C
#define PCI_SUBSYS_ID      0x2E
#define PCI_ROM_ADDRESS    0x30
#define PCI_CAPABILITIES   0x34
#define PCI_INTERRUPT_LINE 0x3C
#define PCI_INTERRUPT_PIN  0x3D
#define PCI_MIN_GNT        0x3E
#define PCI_MAX_LAT        0x3F

// PCI命令寄存器位
#define PCI_COMMAND_IO     0x0001  // I/O空间使能
#define PCI_COMMAND_MEMORY  0x0002     // 内存空间使能
#define PCI_COMMAND_MASTER  0x0004     // 总线主控使能
#define PCI_COMMAND_SPECIAL 0x0008     // 特殊周期使能
#define PCI_COMMAND_INVALIDATE 0x0010 // 内存写和使能
#define PCI_COMMAND_VGA_SNOOP 0x0020  // VGA调色板窥探
#define PCI_COMMAND_PARITY  0x0040    // 奇偶校验错误响应
#define PCI_COMMAND_WAIT    0x0080    // SERR#使能
#define PCI_COMMAND_PERR    0x0100   // 奇偶校验错误响应

// PCI状态寄存器位
#define PCI_STATUS_CAP_LIST  0x0010    // 支持能力列表
#define PCI_STATUS_66MHZ     0x0020    // 支持66MHz
#define PCI_STATUS_UDF       0x0040    // UDF支持
#define PCI_STATUS_FAST_BACK 0x0080    // 快速后退支持
#define PCI_STATUS_PARITY    0x0100   // 检测到奇偶校验错误
#define PCI_STATUS_DEVSEL    0x0600   // DEVSEL定时
#define PCI_STATUS_SIG_TARGET_ABORT 0x0800  // 发出目标中止
#define PCI_STATUS_REC_TARGET_ABORT 0x1000 // 接收到目标中止
#define PCI_STATUS_REC_MASTER_ABORT 0x2000 // 接收到主控中止
#define PCI_STATUS_SIG_SYSTEM_ERROR 0x4000 // 发出系统错误
#define PCI_STATUS_DETECTED_PARITY  0x8000 // 检测到奇偶校验错误

// 函数声明
void pci_init(void);
uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_config_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value);
uint32_t pci_config_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_config_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);
int pci_find_device(uint16_t vendor_id, uint16_t device_id, uint16_t* bus, uint16_t* slot);
uint16_t pci_get_iobase(uint16_t bus, uint16_t slot);
void pci_configure_interrupt(uint16_t bus, uint16_t slot, uint8_t interrupt_line);

#endif // PCI_H 