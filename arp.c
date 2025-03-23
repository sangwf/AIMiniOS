#include "arp.h"
#include "network.h"
#include "rtl8139.h"
#include "terminal.h"
#include "memory.h"
#include "byteorder.h"
#include "types.h"
#include "serial.h"

// 引用外部变量
extern bool disable_rtl_debug;
extern struct net_device net_dev;
extern const uint8_t broadcast_mac[6];

// ARP缓存
static struct arp_cache_entry arp_cache[ARP_CACHE_SIZE];

// 初始化ARP缓存
void arp_init(void) {
    clear_arp_cache();
    serial_write_string("ARP initialized\r\n");
}

// 清除ARP缓存
void clear_arp_cache(void) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        arp_cache[i].valid = false;
    }
    serial_write_string("ARP cache cleared\r\n");
}

// 发送ARP请求包
bool send_arp_request(uint32_t target_ip) {
    serial_write_string("Sending ARP request for IP: ");
    serial_print_ip(target_ip);
    serial_write_string("\r\n");
    
    // 创建包缓冲区
    uint8_t buffer[42]; // 以太网帧大小 + ARP包大小
    memset(buffer, 0, sizeof(buffer));
    
    // 设置以太网帧头
    struct eth_header *eth = (struct eth_header *)buffer;
    // 设置广播MAC地址
    memset(eth->dest_mac, 0xFF, 6);
    // 设置发送者MAC地址
    memcpy(eth->src_mac, net_dev.mac_addr, 6);
    // 设置帧类型为ARP (0x0806)
    eth->type = htons(ETH_TYPE_ARP);
    
    // 设置ARP包内容
    struct arp_packet *arp = (struct arp_packet *)(buffer + sizeof(struct eth_header));
    arp->hardware_type = htons(1);        // 以太网
    arp->protocol_type = htons(0x0800);   // IPv4
    arp->hardware_size = 6;               // MAC地址长度
    arp->protocol_size = 4;               // IP地址长度
    arp->opcode = htons(ARP_REQUEST);     // ARP请求
    
    // 设置发送者MAC和IP
    memcpy(arp->sender_mac, net_dev.mac_addr, 6);
    arp->sender_ip = htonl(net_dev.ip_addr);
    
    // 设置目标MAC (全0) 和目标IP
    memset(arp->target_mac, 0, 6);
    arp->target_ip = htonl(target_ip);
    
    // 发送ARP请求
    serial_write_string("ARP packet prepared, sending...\r\n");
    return network_send_packet(buffer, sizeof(buffer));
}

// 处理接收到的ARP包
void handle_arp_packet(uint8_t *packet, uint16_t length) {
    if (length < sizeof(struct eth_header) + sizeof(struct arp_packet)) {
        terminal_writestring("ARP packet too short\n");
        return;
    }
    
    struct eth_header *eth = (struct eth_header *)packet;
    struct arp_packet *arp = (struct arp_packet *)(packet + sizeof(struct eth_header));
    
    // 检查ARP包类型
    if (ntohs(arp->hardware_type) != 1 || ntohs(arp->protocol_type) != 0x0800 ||
        arp->hardware_size != 6 || arp->protocol_size != 4) {
        terminal_writestring("Invalid ARP packet\n");
        return;
    }
    
    // Convert IP addresses from network to host byte order for display and comparison
    uint32_t sender_ip = ntohl(arp->sender_ip);
    uint32_t target_ip = ntohl(arp->target_ip);
    
    serial_write_string("Received ARP packet: ");
    if (ntohs(arp->opcode) == ARP_REQUEST) {
        serial_write_string("REQUEST");
    } else if (ntohs(arp->opcode) == ARP_REPLY) {
        serial_write_string("REPLY");
    } else {
        serial_write_string("UNKNOWN TYPE");
    }
    
    serial_write_string("\r\nFrom IP: ");
    serial_print_ip(sender_ip); // Use the host byte order IP for display
    serial_write_string(" MAC: ");
    for (int i = 0; i < 6; i++) {
        serial_write_hex_byte(arp->sender_mac[i]);
        if (i < 5) serial_write_string(":");
    }
    serial_write_string("\r\n");
    
    // 更新ARP缓存 (使用主机字节序)
    update_arp_cache(sender_ip, arp->sender_mac);
    
    // 如果收到ARP请求并且目标IP是我们的IP, 则发送ARP应答
    if (ntohs(arp->opcode) == ARP_REQUEST && target_ip == net_dev.ip_addr) {
        serial_write_string("Received ARP request for our IP, responding...\r\n");
        
        // 交换MAC和IP地址
        memcpy(eth->dest_mac, eth->src_mac, 6);
        memcpy(eth->src_mac, net_dev.mac_addr, 6);
        
        // 修改ARP包为应答
        arp->opcode = htons(ARP_REPLY);
        
        // 设置目标MAC和IP (原请求发送者的MAC和IP)
        memcpy(arp->target_mac, arp->sender_mac, 6);
        arp->target_ip = arp->sender_ip; // Keep in network byte order
        
        // 设置发送者MAC和IP (我们的MAC和IP)
        memcpy(arp->sender_mac, net_dev.mac_addr, 6);
        arp->sender_ip = htonl(net_dev.ip_addr); // Convert to network byte order
        
        // 发送ARP应答
        network_send_packet(packet, length);
        serial_write_string("ARP reply sent\r\n");
    }
}

// 从缓存中查找MAC地址
bool get_mac_from_cache(uint32_t ip_addr, uint8_t *mac_out) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip_addr == ip_addr) {
            memcpy(mac_out, arp_cache[i].mac_addr, 6);
            serial_write_string("MAC found in cache for IP: ");
            serial_print_ip(ip_addr);
            serial_write_string("\r\n");
            return true;
        }
    }
    
    serial_write_string("MAC not found in cache for IP: ");
    serial_print_ip(ip_addr);
    serial_write_string("\r\n");
    return false;
}

// 更新ARP缓存
void update_arp_cache(uint32_t ip_addr, uint8_t *mac_addr) {
    // 首先查找现有条目
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip_addr == ip_addr) {
            // 更新现有条目
            memcpy(arp_cache[i].mac_addr, mac_addr, 6);
            serial_write_string("Updated existing ARP cache entry for IP: ");
            serial_print_ip(ip_addr);
            serial_write_string("\r\n");
            return;
        }
    }
    
    // 查找空闲条目
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (!arp_cache[i].valid) {
            // 添加新条目
            arp_cache[i].ip_addr = ip_addr;
            memcpy(arp_cache[i].mac_addr, mac_addr, 6);
            arp_cache[i].valid = true;
            serial_write_string("Added new ARP cache entry for IP: ");
            serial_print_ip(ip_addr);
            serial_write_string("\r\n");
            return;
        }
    }
    
    // 缓存满了，替换第一个条目
    arp_cache[0].ip_addr = ip_addr;
    memcpy(arp_cache[0].mac_addr, mac_addr, 6);
    serial_write_string("ARP cache full, replaced first entry with IP: ");
    serial_print_ip(ip_addr);
    serial_write_string("\r\n");
}

// 解析IP到MAC地址 (先查缓存, 如果没有则发送ARP请求)
bool arp_resolve(uint32_t ip_addr, uint8_t *mac_out) {
    serial_write_string("Resolving IP: ");
    serial_print_ip(ip_addr);
    serial_write_string("\r\n");
    
    // 检查是否是本机IP
    if (ip_addr == net_dev.ip_addr) {
        memcpy(mac_out, net_dev.mac_addr, 6);
        serial_write_string("IP is our own IP, using our MAC\r\n");
        return true;
    }
    
    // 从缓存中查找
    if (get_mac_from_cache(ip_addr, mac_out)) {
        return true;
    }
    
    // 缓存中没有，可以尝试发送ARP请求，但这个实现中
    // 我们只检查缓存，因为发送ARP请求后需要等待处理ARP回复，
    // 这是异步的操作，不适合在这个函数中完成。
    serial_write_string("IP not in cache and not our IP, resolution failed\r\n");
    return false;
} 