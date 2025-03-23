#ifndef NETWORK_H
#define NETWORK_H

#include "types.h"
#include "ipv4.h"

// 以太网帧头
struct eth_header {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t type;
} __attribute__((packed));

// 以太网帧类型
#define ETH_TYPE_IP    0x0800
#define ETH_TYPE_ARP   0x0806

// 以太网MTU(最大传输单元)
#define ETH_MTU 1500

// 全局网络设备对象
struct net_device {
    uint8_t mac_addr[6];
    uint32_t ip_addr;
    uint32_t netmask;
    uint32_t gateway;
    uint16_t mtu;
};

extern struct net_device net_dev;
extern const uint8_t broadcast_mac[6];

// 函数声明
void network_init(void);
bool network_send_packet(uint8_t *data, uint16_t length);
void handle_network_packet(uint8_t *packet, uint16_t length);
void handle_ip_packet(uint8_t *packet, uint16_t length);
void handle_icmp_packet(uint8_t *packet, uint16_t length);
void send_icmp_echo_reply(struct icmp_header *request, uint8_t *packet, uint16_t length, struct ipv4_header *ip_header);
void send_icmp_echo_request(uint32_t target_ip);
void print_ip(uint32_t ip);
void print_mac(uint8_t *mac);
uint16_t network_checksum(const uint8_t *data, size_t length);
bool get_destination_mac(uint32_t ip_addr, uint8_t *mac_out);

// 网络调试开关
extern bool disable_rtl_debug;

#endif // NETWORK_H
