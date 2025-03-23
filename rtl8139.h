#ifndef RTL8139_H
#define RTL8139_H

#include "types.h"

// PCI 配置
#define RTL8139_VENDOR_ID 0x10EC
#define RTL8139_DEVICE_ID 0x8139

// 寄存器偏移量
#define RTL8139_REG_CMD     0x37
#define RTL8139_REG_IMR     0x3C
#define RTL8139_REG_ISR     0x3E
#define RTL8139_REG_RCR     0x44
#define RTL8139_REG_TCR     0x40
#define RTL8139_REG_RBSTART 0x30
#define RTL8139_REG_CAPR    0x38
#define RTL8139_REG_CBR     0x3A
#define RTL8139_REG_TSAD0   0x20
#define RTL8139_REG_TSD0    0x10
#define RTL8139_REG_IDR0    0x00

// 命令寄存器位
#define RTL8139_CMD_RESET      0x10
#define RTL8139_CMD_RX_ENABLE  0x08
#define RTL8139_CMD_TX_ENABLE  0x04
#define RTL8139_CMD_BUFE       0x01

// 中断状态寄存器位
#define RTL8139_ISR_ROK  0x0001  // 接收 OK
#define RTL8139_ISR_TOK  0x0004  // 发送 OK
#define RTL8139_ISR_RER  0x0002  // 接收错误
#define RTL8139_ISR_TER  0x0008  // 发送错误

// 接收配置寄存器位
#define RTL8139_RCR_AAP            0x00000001  // 接收所有物理地址包
#define RTL8139_RCR_APM            0x00000002  // 接收物理匹配包
#define RTL8139_RCR_AM             0x00000004  // 接收多播包
#define RTL8139_RCR_AB             0x00000008  // 接收广播包
#define RTL8139_RCR_WRAP           0x00000080  // 接收缓冲区回环
#define RTL8139_RCR_RBLEN_32K      0x00000000  // 32K 接收缓冲区
#define RTL8139_RCR_MXDMA_UNLIMITED 0x00000700  // 无限制 DMA 突发

// 发送配置寄存器位
#define RTL8139_TCR_IFG_STANDARD  0x03000000  // 标准帧间隔
#define RTL8139_TCR_MXDMA_2048    0x00000700  // 2048 字节 DMA 突发
#define RTL8139_TCR_CRC           0x00010000  // 追加 CRC

// 发送状态寄存器位
#define RTL8139_TSD_TOK   0x00008000  // 发送 OK
#define RTL8139_TSD_TUN   0x00004000  // 发送不足
#define RTL8139_TSD_TABT  0x00002000  // 发送中止

// 缓冲区大小
#define RX_BUFFER_SIZE 32768
#define TX_BUFFER_SIZE 1536

// Global variables
extern uint16_t rtl8139_bus;
extern uint16_t rtl8139_slot;

// 禁用调试标志
extern bool disable_rtl_debug;

// Function declarations
void rtl8139_init(uint16_t bus, uint16_t slot);
void rtl8139_send_packet(const void* data, uint16_t length);
void rtl8139_handle_interrupt(void);
void check_rx_buffer(void);
void rtl8139_dump_registers(void);
uint16_t get_rtl8139_iobase(uint16_t bus, uint16_t slot);

#endif 