#ifndef ARP_H
#define ARP_H

#include "types.h"

// 以太网帧头的前向声明
struct eth_header;

// ARP操作码
#define ARP_REQUEST     1
#define ARP_REPLY       2

// ARP缓存大小
#define ARP_CACHE_SIZE  16

// ARP缓存条目结构
struct arp_cache_entry {
    uint32_t ip_addr;  // 主机字节序
    uint8_t mac_addr[6];
    bool valid;
};

// ARP数据包结构 (不包含以太网头)
struct arp_packet {
    uint16_t hardware_type;  // 网络字节序
    uint16_t protocol_type;  // 网络字节序
    uint8_t hardware_size;
    uint8_t protocol_size;
    uint16_t opcode;        // 网络字节序
    uint8_t sender_mac[6];
    uint32_t sender_ip;     // 网络字节序
    uint8_t target_mac[6];
    uint32_t target_ip;     // 网络字节序
} __attribute__((packed));

// 函数声明
void arp_init(void);
void clear_arp_cache(void);
bool send_arp_request(uint32_t target_ip);  // 传入主机字节序
void handle_arp_packet(uint8_t *packet, uint16_t length);
bool get_mac_from_cache(uint32_t ip_addr, uint8_t *mac_out);  // 传入主机字节序
void update_arp_cache(uint32_t ip_addr, uint8_t *mac_addr);  // 传入主机字节序
bool arp_resolve(uint32_t ip_addr, uint8_t *mac_out);  // 解析IP到MAC地址

#endif // ARP_H 