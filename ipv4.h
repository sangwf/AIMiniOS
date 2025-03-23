#ifndef IPV4_H
#define IPV4_H

#include "types.h"

// IP协议类型
#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP  6
#define IP_PROTO_UDP  17

// IPv4头部结构
struct ipv4_header {
    uint8_t version_ihl;         // 版本 (4 bits) + 头部长度 (4 bits)
    uint8_t dscp_ecn;           // 服务类型
    uint16_t total_length;      // 总长度
    uint16_t identification;    // 标识
    uint16_t flags_fragment_offset; // 标志位(3 bits) + 分片偏移(13 bits)
    uint8_t ttl;              // 生存时间
    uint8_t protocol;         // 协议
    uint16_t checksum;        // 头部校验和
    uint32_t src_ip;          // 源地址
    uint32_t dst_ip;          // 目标地址
    // 选项...
} __attribute__((packed));

// ICMP类型
#define ICMP_TYPE_ECHO_REPLY    0
#define ICMP_TYPE_ECHO_REQUEST  8

// ICMP头部结构
struct icmp_header {
    uint8_t type;        // 类型
    uint8_t code;        // 代码
    uint16_t checksum;   // 校验和
    uint16_t identifier; // 标识符
    uint16_t sequence;   // 序列号
    // 数据...
} __attribute__((packed));

// 函数声明 - 我们不再声明这些函数，因为它们将在network.c中声明和实现
// void handle_ipv4_packet(uint8_t* packet, uint16_t length);
// void handle_icmp_packet(uint8_t* packet, uint16_t length, struct ipv4_header* ip_header);
// void send_icmp_echo_reply(struct icmp_header* request, uint8_t* packet, uint16_t length, struct ipv4_header* ip_header);

uint16_t calculate_checksum(void *addr, int count);
uint32_t ip_str_to_int(const char *ip_str);
void ipv4_init(void);

#define ETH_MTU 1500  // 最大传输单元

#endif // IPV4_H 