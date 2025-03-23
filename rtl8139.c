#include "rtl8139.h"
#include "io.h"
#include "pci.h"
#include "terminal.h"
#include "memory.h"
#include "network.h"
#include "serial.h"

// Global variables
uint16_t rtl8139_bus = 0;
uint16_t rtl8139_slot = 0;
static uint16_t iobase = 0;
static uint8_t *rx_buffer;
static uint8_t tx_buffer[4][TX_BUFFER_SIZE] __attribute__((aligned(16)));
static uint8_t current_tx_buffer = 0;
static uint32_t current_rx_ptr = 0;

// 从network.c引入全局变量，控制调试输出
extern bool disable_rtl_debug;

// 等待函数
static void rtl8139_delay(void) {
    for(volatile int i = 0; i < 10000; i++) {}
}

// 初始化RTL8139网卡
void rtl8139_init(uint16_t bus, uint16_t slot) {
    // Store bus and slot numbers
    rtl8139_bus = bus;
    rtl8139_slot = slot;
    
    terminal_writestring("\n=== RTL8139 Initialization ===\n");
    serial_write_string("\r\n=== RTL8139 Initialization ===\r\n");
    serial_write_string("RTL8139: Debug output enabled status: ");
    serial_write_string(disable_rtl_debug ? "OFF (disable_rtl_debug=true)\r\n" : "ON (disable_rtl_debug=false)\r\n");
    
    // 获取RTL8139的I/O基地址
    iobase = get_rtl8139_iobase(bus, slot);
    
    terminal_writestring("RTL8139 I/O Base: 0x");
    terminal_writehex16(iobase);
    terminal_writestring("\n");
    
    serial_write_string("RTL8139 I/O Base: 0x");
    serial_write_hex16(iobase);
    serial_write_string("\r\n");
    
    // 配置中断
    pci_configure_interrupt(bus, slot, 11);  // IRQ 11 是常用的网卡中断

    // 分配接收缓冲区
    rx_buffer = (uint8_t *)kmalloc(RX_BUFFER_SIZE + 16);
    if (!rx_buffer) {
        terminal_writestring("Failed to allocate RX buffer\n");
        return;
    }

    terminal_writestring("RX buffer allocated at: ");
    terminal_writehex((uint32_t)rx_buffer);
    terminal_writestring("\n");

    // 电源管理唤醒
    terminal_writestring("Power management wake up...\n");
    outb(iobase + 0x52, 0x00);  // 修正为写入0x00
    rtl8139_delay();

    // 软件复位
    terminal_writestring("Performing software reset...\n");
    outb(iobase + RTL8139_REG_CMD, RTL8139_CMD_RESET);
    rtl8139_delay();

    // 等待复位完成
    terminal_writestring("Waiting for reset completion...\n");
    int timeout = 1000;
    while ((inb(iobase + RTL8139_REG_CMD) & RTL8139_CMD_RESET) != 0) {
        rtl8139_delay();
        timeout--;
        if (timeout <= 0) {
            terminal_writestring("RTL8139 reset timeout!\n");
            return;
        }
    }

    terminal_writestring("RTL8139 reset completed successfully\n");

    // 初始化接收缓冲区 - 确保16字节对齐
    terminal_writestring("Initializing RX buffer...\n");
    rx_buffer = (uint8_t *)kmalloc(RX_BUFFER_SIZE + 16);
    if (!rx_buffer) {
        terminal_writestring("Failed to allocate RX buffer\n");
        return;
    }
    
    // 确保16字节对齐
    uint32_t rx_buffer_phys = (uint32_t)rx_buffer;
    if (rx_buffer_phys & 0xF) {
        rx_buffer_phys = (rx_buffer_phys + 16) & ~0xF;
    }
    
    outl(iobase + RTL8139_REG_RBSTART, rx_buffer_phys);
    rtl8139_delay();

    // 设置IMR和ISR
    terminal_writestring("Setting up interrupts...\n");
    terminal_writestring("Initial ISR value: ");
    terminal_writehex16(inw(iobase + RTL8139_REG_ISR));
    terminal_writestring("\n");
    
    // 启用所有中断
    outw(iobase + RTL8139_REG_IMR, RTL8139_ISR_ROK | RTL8139_ISR_TOK | 
                                   RTL8139_ISR_RER | RTL8139_ISR_TER);
    outw(iobase + RTL8139_REG_ISR, 0xFFFF);  // 清除所有中断
    rtl8139_delay();

    terminal_writestring("IMR set to: ");
    terminal_writehex16(inw(iobase + RTL8139_REG_IMR));
    terminal_writestring("\n");

    // 配置接收和发送
    terminal_writestring("Configuring RX/TX...\n");
    
    // 配置接收
    uint32_t rcr_config = RTL8139_RCR_AAP |  // 接收所有物理地址包
                         RTL8139_RCR_APM |  // 接收物理匹配包
                         RTL8139_RCR_AM |   // 接收多播包
                         RTL8139_RCR_AB |   // 接收广播包
                         RTL8139_RCR_WRAP | // 接收缓冲区回环
                         RTL8139_RCR_RBLEN_32K |  // 32K 接收缓冲区
                         RTL8139_RCR_MXDMA_UNLIMITED;  // 无限制 DMA 突发
    
    outl(iobase + RTL8139_REG_RCR, rcr_config);
    rtl8139_delay();
    
    // 验证RCR配置
    uint32_t rcr_verify = inl(iobase + RTL8139_REG_RCR);
    terminal_writestring("RCR configured: ");
    terminal_writehex32(rcr_verify);
    terminal_writestring("\n");
    
    // 配置发送
    uint32_t tcr_config = RTL8139_TCR_IFG_STANDARD |  // 标准帧间隔
                         RTL8139_TCR_MXDMA_2048 |    // 2048 字节 DMA 突发
                         RTL8139_TCR_CRC;            // 追加 CRC
    
    outl(iobase + RTL8139_REG_TCR, tcr_config);
    rtl8139_delay();
    
    // 验证TCR配置
    uint32_t tcr_verify = inl(iobase + RTL8139_REG_TCR);
    terminal_writestring("TCR configured: ");
    terminal_writehex32(tcr_verify);
    terminal_writestring("\n");
    
    // 启用发送和接收
    terminal_writestring("Enabling RX/TX...\n");
    outb(iobase + RTL8139_REG_CMD, RTL8139_CMD_RX_ENABLE | RTL8139_CMD_TX_ENABLE);
    rtl8139_delay();

    // 验证网卡状态
    uint8_t cmd = inb(iobase + RTL8139_REG_CMD);
    terminal_writestring("Command register after enable: ");
    terminal_writehex8(cmd);
    terminal_writestring("\n");
    
    if ((cmd & (RTL8139_CMD_RX_ENABLE | RTL8139_CMD_TX_ENABLE)) != 
        (RTL8139_CMD_RX_ENABLE | RTL8139_CMD_TX_ENABLE)) {
        terminal_writestring("RTL8139 failed to enable RX/TX!\n");
        return;
    }

    terminal_writestring("RTL8139 initialized successfully\n");
    terminal_writestring("=== Final Register State ===\n");
    rtl8139_dump_registers();
    terminal_writestring("===========================\n\n");

    // 发送测试数据包
    terminal_writestring("Sending test packet...\n");
    uint8_t test_data[] = {
        // 以太网头
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // 目标MAC（广播）
        0x52, 0x54, 0x00, 0x12, 0x34, 0x56,  // 源MAC
        0x08, 0x00,                          // 类型（IPv4）
        
        // IP头
        0x45, 0x00,                          // 版本和头部长度
        0x00, 0x1C,                          // 总长度（28字节）
        0x00, 0x00,                          // 标识
        0x00, 0x00,                          // 标志和片偏移
        0x40, 0x01,                          // TTL和协议（ICMP）
        0x00, 0x00,                          // 校验和
        0x0A, 0x00, 0x02, 0x0F,             // 源IP（10.0.2.15）
        0x0A, 0x00, 0x02, 0x02,             // 目标IP（10.0.2.2）
        
        // ICMP头
        0x08, 0x00,                          // 类型和代码（Echo请求）
        0x00, 0x00,                          // 校验和
        0x00, 0x01,                          // 标识符
        0x00, 0x01                           // 序列号
    };

    // 计算IP校验和
    uint16_t ip_checksum = network_checksum(test_data + 14, 20);  // 跳过以太网头
    test_data[24] = ip_checksum & 0xFF;         // 校验和低字节
    test_data[25] = (ip_checksum >> 8) & 0xFF;  // 校验和高字节

    // 计算ICMP校验和
    uint16_t icmp_checksum = network_checksum(test_data + 34, 8);  // ICMP部分
    test_data[36] = icmp_checksum & 0xFF;         // 校验和低字节
    test_data[37] = (icmp_checksum >> 8) & 0xFF;  // 校验和高字节

    rtl8139_send_packet(test_data, sizeof(test_data));
    terminal_writestring("Test packet sent\n");
}

// 获取RTL8139的I/O基地址
uint16_t get_rtl8139_iobase(uint16_t bus, uint16_t slot) {
    return pci_get_iobase(bus, slot);
}

// 发送数据包
void rtl8139_send_packet(const void* data, uint16_t length) {
    if (!disable_rtl_debug) {
        terminal_writestring("RTL8139: Sending packet, length = ");
        terminal_writedec(length);
        terminal_writestring("\n");
        
        serial_write_string("RTL8139: Sending packet, length = ");
        serial_write_dec(length);
        serial_write_string("\r\n");
    }

    // 检查是否禁用调试输出
    if (!disable_rtl_debug) {
        terminal_writestring("\n=== Sending Packet [DEBUG] ===\n");
        terminal_writestring("Length: ");
        terminal_writehex16(length);
        terminal_writestring("\nPacket Data (First 64 bytes): ");
        
        // 打印数据包内容
        const uint8_t* packet = (const uint8_t*)data;
        for(int i = 0; i < length && i < 64; i++) {
            if(i % 16 == 0) terminal_writestring("\n");
            terminal_writehex8(packet[i]);
            terminal_writestring(" ");
        }
        terminal_writestring("\n");

        // 解析以太网头部
        if (length >= 14) {
            terminal_writestring("\nEthernet Header:\n");
            terminal_writestring("  Dest MAC: ");
            for (int i = 0; i < 6; i++) {
                terminal_writehex8(packet[i]);
                if (i < 5) terminal_writestring(":");
            }
            terminal_writestring("\n  Src MAC: ");
            for (int i = 0; i < 6; i++) {
                terminal_writehex8(packet[i+6]);
                if (i < 5) terminal_writestring(":");
            }
            terminal_writestring("\n  Type: ");
            terminal_writehex8(packet[12]);
            terminal_writehex8(packet[13]);
            terminal_writestring("\n");
        }

        // 解析IP头部
        if (length >= 34 && packet[12] == 0x08 && packet[13] == 0x00) {
            terminal_writestring("\nIP Header:\n");
            terminal_writestring("  Version/IHL: ");
            terminal_writehex8(packet[14]);
            terminal_writestring("\n  Total Length: ");
            terminal_writehex8(packet[16]);
            terminal_writehex8(packet[17]);
            terminal_writestring("\n  Protocol: ");
            terminal_writehex8(packet[23]);
            terminal_writestring("\n  Checksum: ");
            terminal_writehex8(packet[24]);
            terminal_writehex8(packet[25]);
            terminal_writestring("\n  Src IP: ");
            for (int i = 0; i < 4; i++) {
                terminal_writehex8(packet[26+i]);
                if (i < 3) terminal_writestring(".");
            }
            terminal_writestring("\n  Dest IP: ");
            for (int i = 0; i < 4; i++) {
                terminal_writehex8(packet[30+i]);
                if (i < 3) terminal_writestring(".");
            }
            terminal_writestring("\n");
        }
    }

    // 处理发送包太大的情况
    if (length > TX_BUFFER_SIZE) {
        if (!disable_rtl_debug) {
            terminal_writestring("ERROR: Packet too large!\n");
        }
        return;
    }

    // 复制数据到发送缓冲区
    if (!disable_rtl_debug) {
        terminal_writestring("Copying data to TX buffer ");
        terminal_writehex8(current_tx_buffer);
        terminal_writestring("...\n");
    }
    
    const uint8_t* packet = (const uint8_t*)data;
    for (int i = 0; i < length; i++) {
        tx_buffer[current_tx_buffer][i] = packet[i];
    }

    // 设置发送描述符 - 使用缓冲区的物理地址
    if (!disable_rtl_debug) {
        terminal_writestring("Setting TSAD: ");
        terminal_writehex8(current_tx_buffer);
        terminal_writestring(" to address: ");
        terminal_writehex32((uint32_t)tx_buffer[current_tx_buffer]);
        terminal_writestring("\n");
    }
    
    // 设置发送描述符地址寄存器
    outl(iobase + RTL8139_REG_TSAD0 + (current_tx_buffer * 4), (uint32_t)tx_buffer[current_tx_buffer]);
    
    // 验证TSAD设置
    if (!disable_rtl_debug) {
        uint32_t tsad_verify = inl(iobase + RTL8139_REG_TSAD0 + (current_tx_buffer * 4));
        terminal_writestring("Verified TSAD");
        terminal_writehex8(current_tx_buffer);
        terminal_writestring(": ");
        terminal_writehex32(tsad_verify);
        terminal_writestring("\n");
        
        terminal_writestring("Setting TSD: ");
        terminal_writehex8(current_tx_buffer);
        terminal_writestring(" with length: ");
        terminal_writehex16(length);
        terminal_writestring("\n");
    }
    
    // 设置发送状态描述符寄存器，启动传输
    // 确保使用正确的配置位：长度字段应该是前16位
    uint32_t tsd_value = length;  // 长度在低16位，其他配置位
    
    if (!disable_rtl_debug) {
        terminal_writestring("TSD raw value to set: ");
        terminal_writehex32(tsd_value);
        terminal_writestring("\n");
    }
    
    outl(iobase + RTL8139_REG_TSD0 + (current_tx_buffer * 4), tsd_value);

    // 立即验证TSD设置
    if (!disable_rtl_debug) {
        uint32_t tsd_verify = inl(iobase + RTL8139_REG_TSD0 + (current_tx_buffer * 4));
        terminal_writestring("Verified TSD raw value: ");
        terminal_writehex32(tsd_verify);
        terminal_writestring("\n");
        
        terminal_writestring("Packet queued for transmission\n");
        terminal_writestring("TSD");
        terminal_writehex8(current_tx_buffer);
        terminal_writestring(" after setting: ");
        terminal_writehex32(inl(iobase + RTL8139_REG_TSD0 + (current_tx_buffer * 4)));
        terminal_writestring("\n");
    }

    // 更新发送缓冲区索引
    current_tx_buffer = (current_tx_buffer + 1) % 4;

    // 等待发送完成
    if (!disable_rtl_debug) {
        terminal_writestring("Waiting for transmission to complete...\n");
    }
    
    uint32_t tsd = inl(iobase + RTL8139_REG_TSD0 + ((current_tx_buffer - 1) % 4) * 4);
    int timeout = 1000;
    while (!(tsd & RTL8139_TSD_TOK) && timeout > 0) {
        rtl8139_delay();
        tsd = inl(iobase + RTL8139_REG_TSD0 + ((current_tx_buffer - 1) % 4) * 4);
        
        if (!disable_rtl_debug && timeout % 100 == 0) {
            terminal_writestring("TSD during wait: ");
            terminal_writehex32(tsd);
            terminal_writestring(" [");
            if (tsd & RTL8139_TSD_TOK) terminal_writestring(" TOK");
            if (tsd & RTL8139_TSD_TUN) terminal_writestring(" TUN");
            if (tsd & RTL8139_TSD_TABT) terminal_writestring(" TABT");
            terminal_writestring(" ]\n");
        }
        
        timeout--;
        
        // 检查是否有发送错误
        if (tsd & RTL8139_TSD_TABT) {
            if (!disable_rtl_debug) {
                terminal_writestring("ERROR: Transmission aborted!\n");
            }
            break;
        }
        if (tsd & RTL8139_TSD_TUN) {
            if (!disable_rtl_debug) {
                terminal_writestring("ERROR: Transmit underrun!\n");
            }
            break;
        }
    }

    if (!disable_rtl_debug) {
        if (timeout == 0) {
            terminal_writestring("ERROR: Transmission timeout!\n");
        } else {
            terminal_writestring("Transmission completed\n");
            terminal_writestring("Final TSD value: ");
            terminal_writehex32(tsd);
            terminal_writestring(" [");
            if (tsd & RTL8139_TSD_TOK) terminal_writestring(" TOK");
            if (tsd & RTL8139_TSD_TUN) terminal_writestring(" TUN");
            if (tsd & RTL8139_TSD_TABT) terminal_writestring(" TABT");
            terminal_writestring(" ]\n");
        }

        // 打印最终的寄存器状态
        rtl8139_dump_registers();
        terminal_writestring("=== Sending Packet End ===\n");
    }
}

// 处理中断
void rtl8139_handle_interrupt(void) {
    terminal_writestring("\n=== RTL8139 Interrupt [DEBUG] ===\n");
    
    // 读取中断状态寄存器
    uint16_t status = inw(iobase + RTL8139_REG_ISR);
    
    if (!disable_rtl_debug) {
        terminal_writestring("RTL8139: Interrupt, status = 0x");
        terminal_writehex16(status);
        terminal_writestring("\n");
        
        serial_write_string("RTL8139: Interrupt, status = 0x");
        serial_write_hex16(status);
        serial_write_string("\r\n");
    }

    terminal_writestring("ISR Status: ");
    terminal_writehex16(status);
    terminal_writestring(" [");
    if (status & RTL8139_ISR_ROK) terminal_writestring(" ROK");
    if (status & RTL8139_ISR_TOK) terminal_writestring(" TOK");
    if (status & RTL8139_ISR_RER) terminal_writestring(" RER");
    if (status & RTL8139_ISR_TER) terminal_writestring(" TER");
    terminal_writestring(" ]\n");

    // 读取命令寄存器状态
    uint8_t cmd = inb(iobase + RTL8139_REG_CMD);
    terminal_writestring("CMD Status: ");
    terminal_writehex8(cmd);
    terminal_writestring(" [");
    if (cmd & RTL8139_CMD_RX_ENABLE) terminal_writestring(" RX_EN");
    if (cmd & RTL8139_CMD_TX_ENABLE) terminal_writestring(" TX_EN");
    terminal_writestring(" ]\n");

    if (status & RTL8139_ISR_ROK) {
        if (!disable_rtl_debug) {
            terminal_writestring("RTL8139: Packet received\n");
            serial_write_string("RTL8139: Packet received\r\n");
        }
        check_rx_buffer();
    } else {
        terminal_writestring("No packet received (ROK not set)\n");
    }

    if (status & RTL8139_ISR_TOK) {
        if (!disable_rtl_debug) {
            terminal_writestring("RTL8139: Packet transmitted\n");
            serial_write_string("RTL8139: Packet transmitted\r\n");
        }
        
        // 检查所有发送描述符的状态
        for (int i = 0; i < 4; i++) {
            uint32_t tsd = inl(iobase + RTL8139_REG_TSD0 + (i * 4));
            terminal_writestring("TSD");
            terminal_writehex8(i);
            terminal_writestring(": ");
            terminal_writehex32(tsd);
            terminal_writestring(" [");
            if (tsd & RTL8139_TSD_TOK) terminal_writestring(" TOK");
            if (tsd & RTL8139_TSD_TUN) terminal_writestring(" TUN");
            if (tsd & RTL8139_TSD_TABT) terminal_writestring(" TABT");
            terminal_writestring(" ]\n");
        }
    } else {
        terminal_writestring("No packet transmitted (TOK not set)\n");
    }

    if (status & RTL8139_ISR_RER) {
        terminal_writestring("Receive error detected (RER set)\n");
    }

    if (status & RTL8139_ISR_TER) {
        terminal_writestring("Transmit error detected (TER set)\n");
    }

    // 清除中断状态
    terminal_writestring("Clearing interrupt status: ");
    terminal_writehex16(status);
    terminal_writestring("\n");
    outw(iobase + RTL8139_REG_ISR, status);
    
    // 验证中断已清除
    uint16_t new_status = inw(iobase + RTL8139_REG_ISR);
    terminal_writestring("New ISR Status: ");
    terminal_writehex16(new_status);
    terminal_writestring("\n");
    
    terminal_writestring("Interrupt handled\n");
    terminal_writestring("=== RTL8139 Interrupt End ===\n");
}

// 检查接收缓冲区
void check_rx_buffer(void) {
    if (disable_rtl_debug) {
        // 仍然检查接收缓冲区，但不输出调试信息
        uint16_t capr = inw(iobase + RTL8139_REG_CAPR);
        uint16_t cbr = inw(iobase + RTL8139_REG_CBR);
        
        // 计算可用数据量
        uint16_t bytes_avail = 0;
        if (cbr >= capr) {
            bytes_avail = cbr - capr;
        } else {
            bytes_avail = RX_BUFFER_SIZE + cbr - capr;
        }
        
        // 只有当有数据可用时继续
        if (bytes_avail > 0) {
            uint16_t rx_offset = capr + 16;  // 当前CAPR加上帧头大小
            if (rx_offset >= RX_BUFFER_SIZE) rx_offset -= RX_BUFFER_SIZE;
            
            // 读取帧状态和大小
            uint16_t rx_status = *(uint16_t *)(rx_buffer + rx_offset);
            uint16_t rx_size = *(uint16_t *)(rx_buffer + rx_offset + 2);
            
            // 如果有数据 (ROK 位被设置)，处理数据
            if ((rx_status & 0x1) == 0x1) {
                uint8_t *packet = rx_buffer + rx_offset + 4;  // 指向数据部分
                
                // 简单调试信息输出到串口
                serial_write_string("RX packet detected, processing...\r\n");
                
                // 处理数据包
                handle_network_packet(packet, rx_size - 4);
                
                // 更新CAPR
                rx_offset = (rx_offset + rx_size + 4 + 3) & ~3;  // 对齐到4字节边界
                if (rx_offset >= RX_BUFFER_SIZE) rx_offset = rx_offset - RX_BUFFER_SIZE + 16;
                outw(iobase + RTL8139_REG_CAPR, rx_offset - 16);  // 更新CAPR
            }
        }
        return;
    }
    
    // 显示调试信息
    terminal_writestring("\n=== Checking RX Buffer [DEBUG] ===\n");
    
    // 读取CAPR和CBR寄存器
    uint16_t capr = inw(iobase + RTL8139_REG_CAPR);
    uint16_t cbr = inw(iobase + RTL8139_REG_CBR);
    
    terminal_writestring("CAPR (Read Pointer): ");
    terminal_writehex16(capr);
    terminal_writestring("\nCBR (Write Pointer): ");
    terminal_writehex16(cbr);
    terminal_writestring("\nRX Buffer Physical Address: ");
    terminal_writehex32((uint32_t)rx_buffer);
    
    // 计算可用数据量
    uint16_t bytes_avail = 0;
    if (cbr >= capr) {
        bytes_avail = cbr - capr;
    } else {
        bytes_avail = RX_BUFFER_SIZE + cbr - capr;
    }
    
    terminal_writestring("\nAvailable data in buffer: ");
    terminal_writehex16(bytes_avail);
    terminal_writestring(" bytes\n");
    
    // 检查是否有数据
    if (bytes_avail > 0) {
        // 有数据，处理它
        uint16_t rx_offset = capr + 16;  // 当前CAPR加上帧头大小
        if (rx_offset >= RX_BUFFER_SIZE) rx_offset -= RX_BUFFER_SIZE;
        
        // 读取帧状态和大小
        uint16_t rx_status = *(uint16_t *)(rx_buffer + rx_offset);
        uint16_t rx_size = *(uint16_t *)(rx_buffer + rx_offset + 2);
        
        terminal_writestring("Received packet: Status=0x");
        terminal_writehex16(rx_status);
        terminal_writestring(", Size=");
        terminal_writehex16(rx_size);
        terminal_writestring("\n");
        
        // 检查状态
        if ((rx_status & 0x1) == 0x1) {  // ROK 位被设置
            terminal_writestring("Valid packet received\n");
            uint8_t *packet = rx_buffer + rx_offset + 4;  // 指向数据部分
            
            // 显示前几个字节
            terminal_writestring("Packet data (first 16 bytes): ");
            for (int i = 0; i < 16 && i < rx_size; i++) {
                terminal_writehex8(packet[i]);
                terminal_writestring(" ");
            }
            terminal_writestring("\n");
            
            // 处理数据包
            handle_network_packet(packet, rx_size - 4);
            
            // 更新CAPR
            rx_offset = (rx_offset + rx_size + 4 + 3) & ~3;  // 对齐到4字节边界
            if (rx_offset >= RX_BUFFER_SIZE) rx_offset = rx_offset - RX_BUFFER_SIZE + 16;
            outw(iobase + RTL8139_REG_CAPR, rx_offset - 16);  // 更新CAPR
        } else {
            terminal_writestring("Invalid packet received\n");
        }
    } else {
        terminal_writestring("No new data in RX buffer\n");
    }
    
    terminal_writestring("=== Checking RX Buffer End ===\n");
}

// 打印RTL8139寄存器状态
void rtl8139_dump_registers(void) {
    if (disable_rtl_debug) return;  // 如果禁用调试，直接返回
    
    terminal_writestring("\n=== RTL8139 Register Dump ===\n");
    terminal_writestring("CMD: 0x");
    terminal_writehex8(inb(iobase + RTL8139_REG_CMD));
    terminal_writestring(" [ ");
    if (inb(iobase + RTL8139_REG_CMD) & 0x08) terminal_writestring("TX_EN ");
    if (inb(iobase + RTL8139_REG_CMD) & 0x04) terminal_writestring("RX_EN ");
    terminal_writestring("]\n");
    
    // 显示ISR和IMR状态
    uint16_t isr = inw(iobase + RTL8139_REG_ISR);
    terminal_writestring("ISR: 0x");
    terminal_writehex16(isr);
    terminal_writestring(" [ ");
    if (isr & RTL8139_ISR_ROK) terminal_writestring("ROK ");
    if (isr & RTL8139_ISR_TOK) terminal_writestring("TOK ");
    terminal_writestring("]\n");
    
    uint16_t imr = inw(iobase + RTL8139_REG_IMR);
    terminal_writestring("IMR: 0x");
    terminal_writehex16(imr);
    terminal_writestring(" [ ");
    if (imr & RTL8139_ISR_ROK) terminal_writestring("ROK ");
    if (imr & RTL8139_ISR_TOK) terminal_writestring("TOK ");
    terminal_writestring("]\n");
    
    // 显示RCR状态
    uint32_t rcr = inl(iobase + RTL8139_REG_RCR);
    terminal_writestring("RCR: 0x");
    terminal_writehex32(rcr);
    terminal_writestring(" [ ");
    if (rcr & RTL8139_RCR_AAP) terminal_writestring("AAP ");
    if (rcr & RTL8139_RCR_APM) terminal_writestring("APM ");
    if (rcr & RTL8139_RCR_AM) terminal_writestring("AM ");
    if (rcr & RTL8139_RCR_AB) terminal_writestring("AB ");
    if (rcr & RTL8139_RCR_WRAP) terminal_writestring("WRAP ");
    terminal_writestring("]\n");
    
    // 显示传输状态寄存器
    for (int i = 0; i < 4; i++) {
        uint32_t tsd = inl(iobase + RTL8139_REG_TSD0 + i * 4);
        terminal_writestring("TSD0x0");
        terminal_writehex8(i);
        terminal_writestring(": 0x");
        terminal_writehex32(tsd);
        terminal_writestring(" [ ");
        if (tsd & RTL8139_TSD_TOK) terminal_writestring("TOK ");
        if (tsd & RTL8139_TSD_TABT) terminal_writestring("TABT ");
        terminal_writestring("] Size: 0x");
        terminal_writehex16((tsd >> 16) & 0x1FFF);
        terminal_writestring("\n");
    }
    
    // 显示CAPR和CBR值
    terminal_writestring("CAPR: 0x");
    terminal_writehex16(inw(iobase + RTL8139_REG_CAPR));
    terminal_writestring(" CBR: 0x");
    terminal_writehex16(inw(iobase + RTL8139_REG_CBR));
    terminal_writestring("\n");
    
    // 显示RBSTART值
    terminal_writestring("RBSTART: 0x");
    terminal_writehex32(inl(iobase + RTL8139_REG_RBSTART));
    terminal_writestring("\n");
    
    // 显示MAC地址
    terminal_writestring("MAC Address: ");
    for (int i = 0; i < 6; i++) {
        terminal_writestring("0x");
        terminal_writehex8(inb(iobase + RTL8139_REG_IDR0 + i));
        if (i < 5) terminal_writestring(":");
    }
    terminal_writestring("\n");
    
    terminal_writestring("============================\n");
}