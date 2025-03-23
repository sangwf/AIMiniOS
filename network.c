#include "network.h"
#include "rtl8139.h"
#include "terminal.h"
#include "arp.h"
#include "tcp.h"
#include "memory.h"
#include "byteorder.h"
#include "serial.h"
#include "ipv4.h"

// ICMP类型常量
#define ICMP_TYPE_ECHO_REQUEST  8
#define ICMP_TYPE_ECHO_REPLY    0

// IP协议类型 (与tcp.h保持一致)
#define IP_PROTO_ICMP 1
#define IP_PROTO_UDP  17

// 全局网络设备对象
struct net_device net_dev;
// 广播MAC地址
const uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
// RTL调试输出控制变量，默认禁用调试输出
bool disable_rtl_debug = true;

// 函数声明
void handle_ip_packet(uint8_t *packet, uint16_t length);
void handle_icmp_packet(uint8_t *packet, uint16_t length);
void send_icmp_echo_reply(struct icmp_header *request, uint8_t *packet, uint16_t length, struct ipv4_header *ip_header);
void send_icmp_echo_request(uint32_t target_ip);

// 初始化网络
void network_init(void) {
    // 初始化网络设备
    net_dev.ip_addr = 0x0A00020F;      // 10.0.2.15 (QEMU默认IP)
    net_dev.netmask = 0xFFFFFF00;      // 255.255.255.0
    net_dev.gateway = 0x0A000202;      // 10.0.2.2 (QEMU默认网关)
    net_dev.mtu = 1500;

    // 初始化MAC地址 (这里使用默认值，实际上应该从网卡读取)
    net_dev.mac_addr[0] = 0x52;
    net_dev.mac_addr[1] = 0x54;
    net_dev.mac_addr[2] = 0x00;
    net_dev.mac_addr[3] = 0x12;
    net_dev.mac_addr[4] = 0x34;
    net_dev.mac_addr[5] = 0x56;

    terminal_writestring("Network initialized: IP ");
    print_ip(net_dev.ip_addr);
    terminal_writestring("\n");
    
    serial_write_string("Network initialized: IP ");
    serial_print_ip(net_dev.ip_addr);
    serial_write_string("\r\n");

    // 初始化ARP、TCP等网络协议
    tcp_init();
}

// 发送网络数据包
bool network_send_packet(uint8_t *data, uint16_t length) {
    // 通过RTL8139驱动发送数据包
    rtl8139_send_packet(data, length);
    return true;
}

// 处理接收到的网络数据包
void handle_network_packet(uint8_t *packet, uint16_t length) {
    struct eth_header *eth = (struct eth_header *)packet;

    // 根据以太网帧类型分发到相应的处理函数
    switch (ntohs(eth->type)) {
        case ETH_TYPE_ARP:
            if (!disable_rtl_debug) {
                terminal_writestring("Received ARP packet\n");
                serial_write_string("Received ARP packet\r\n");
            }
            handle_arp_packet(packet, length);
            break;
        case ETH_TYPE_IP:
            // 分发IP数据包
            handle_ip_packet(packet, length);
            break;
        default:
            if (!disable_rtl_debug) {
                terminal_writestring("Unsupported ethernet type: 0x");
                terminal_writehex16(ntohs(eth->type));
                terminal_writestring("\n");
                
                serial_write_string("Unsupported ethernet type: 0x");
                serial_write_hex16(ntohs(eth->type));
                serial_write_string("\r\n");
            }
            break;
    }
}

// 处理IP数据包
void handle_ip_packet(uint8_t *packet, uint16_t length) {
    if (length < sizeof(struct eth_header) + sizeof(struct ipv4_header)) {
        if (!disable_rtl_debug) {
            terminal_writestring("Packet too short for IP\n");
        }
        return;
    }

    struct ipv4_header *ip = (struct ipv4_header *)(packet + sizeof(struct eth_header));
    
    // IP地址字节序检查和调试输出
    serial_write_string("IP packet - SRC: ");
    serial_print_ip(ntohl(ip->src_ip));
    serial_write_string(" DST: ");
    serial_print_ip(ntohl(ip->dst_ip));
    serial_write_string("\r\n");
    
    // 比较前转换字节序 - 将网络字节序的ip->dst_ip转换为主机字节序后再比较
    // 或者将主机字节序的net_dev.ip_addr转换为网络字节序后再比较
    if (ntohl(ip->dst_ip) != net_dev.ip_addr) {
        if (!disable_rtl_debug) {
            terminal_writestring("IP packet not for us: ");
            print_ip(ntohl(ip->dst_ip));
            terminal_writestring(" vs our IP ");
            print_ip(net_dev.ip_addr);
            terminal_writestring("\n");
        }
        serial_write_string("IP packet not for us\r\n");
        return;
    }
    
    // 数据包是发给我们的，输出确认信息
    serial_write_string("IP packet is for us, processing...\r\n");

    uint8_t protocol = ip->protocol;
    
    if (!disable_rtl_debug) {
        terminal_writestring("IP Protocol: ");
        terminal_writehex8(protocol);
        terminal_writestring("\n");
    }

    switch (protocol) {
        case IP_PROTO_ICMP:
            // ICMP消息始终处理，无论debug模式如何
            serial_write_string("Received ICMP packet\r\n");
            handle_icmp_packet(packet, length);
            break;
        case IP_PROTO_TCP:
            if (!disable_rtl_debug) {
                terminal_writestring("Received TCP packet\n");
            }
            handle_tcp_packet(packet, length);
            break;
        case IP_PROTO_UDP:
            if (!disable_rtl_debug) {
                terminal_writestring("Received UDP packet (not handled)\n");
            }
            // 未实现UDP处理
            break;
        default:
            if (!disable_rtl_debug) {
                terminal_writestring("Unsupported IP protocol\n");
            }
            break;
    }
}

// 计算校验和
uint16_t network_checksum(const uint8_t *data, size_t length) {
    uint32_t sum = 0;

    // 按照2字节为单位进行累加
    const uint16_t *ptr = (const uint16_t *)data;
    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }

    // 如果长度为奇数，处理最后一个字节
    if (length > 0) {
        sum += *(const uint8_t *)ptr;
    }

    // 处理进位
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return ~sum;
}

// 获取目标MAC地址
bool get_destination_mac(uint32_t ip_addr, uint8_t *mac_out) {
    // 使用ARP协议解析IP地址对应的MAC地址
    return arp_resolve(ip_addr, mac_out);
}

// 监视网络状态
void network_monitor_status(void) {
    terminal_writestring("Network status: OK\n");
    terminal_writestring("Current IP: ");
    print_ip(net_dev.ip_addr);
    terminal_writestring("\n");
}

// 打印IP地址
void print_ip(uint32_t ip) {
    // 按照正确的网络字节顺序显示IP（大端序，即高位字节先显示）
    terminal_writedec((ip >> 24) & 0xFF);
    terminal_writestring(".");
    terminal_writedec((ip >> 16) & 0xFF);
    terminal_writestring(".");
    terminal_writedec((ip >> 8) & 0xFF);
    terminal_writestring(".");
    terminal_writedec((ip >> 0) & 0xFF);
}

// Handle ICMP packet
void handle_icmp_packet(uint8_t *packet, uint16_t length) {
    struct eth_header *eth = (struct eth_header *)(packet);
    struct ipv4_header *ip_header = (struct ipv4_header *)(packet + sizeof(struct eth_header));
    struct icmp_header *icmp_header = (struct icmp_header *)(packet + sizeof(struct eth_header) + sizeof(struct ipv4_header));

    // Force debug message for all ICMP packets - critical to debug output
    serial_write_string("\r\n!!! ICMP PACKET !!!\r\n");
    serial_write_string("Type: ");
    serial_write_dec(icmp_header->type);
    if (icmp_header->type == ICMP_TYPE_ECHO_REQUEST) {
        serial_write_string(" (ECHO REQUEST)");
    } else if (icmp_header->type == ICMP_TYPE_ECHO_REPLY) {
        serial_write_string(" (ECHO REPLY)");
    }
    
    serial_write_string(" Code: ");
    serial_write_dec(icmp_header->code);
    serial_write_string("\r\nFrom IP: ");
    serial_print_ip(ntohl(ip_header->src_ip));
    serial_write_string("\r\n");
    
    // Dump the ICMP payload
    uint8_t *data = packet + sizeof(struct eth_header) + sizeof(struct ipv4_header) + sizeof(struct icmp_header);
    uint16_t data_length = length - sizeof(struct eth_header) - sizeof(struct ipv4_header) - sizeof(struct icmp_header);
    
    serial_write_string("Data (");
    serial_write_dec(data_length);
    serial_write_string(" bytes): ");
    
    for (uint16_t i = 0; i < data_length && i < 16; i++) {
        serial_write_hex8(data[i]);
        serial_write_string(" ");
    }
    
    serial_write_string("\r\nASCII: ");
    for (uint16_t i = 0; i < data_length && i < 16; i++) {
        if (data[i] >= 32 && data[i] <= 126) {
            char c = data[i];
            serial_write_string(&c);
        } else {
            serial_write_string(".");
        }
    }
    serial_write_string("\r\n");

    // Check for echo request
    if (icmp_header->type == ICMP_TYPE_ECHO_REQUEST && icmp_header->code == 0) {
        terminal_clear();  // Clear screen for better message display
        terminal_writestring("\n\n");
        terminal_writestring("=== PING RECEIVED ===\n");
        terminal_writestring("Received PING request from IP: ");
        print_ip(ntohl(ip_header->src_ip));
        terminal_writestring("\n\n");
        terminal_writestring("Preparing to send ICMP reply with 'hello,world'...\n");

        // Also output to serial port
        serial_write_string("\r\n=== PING RECEIVED ===\r\n");
        serial_write_string("Received PING request from IP: ");
        serial_print_ip(ntohl(ip_header->src_ip));
        serial_write_string("\r\n");
        
        // Send ICMP Echo reply
        send_icmp_echo_reply(icmp_header, packet, length, ip_header);
    } 
    // Check for echo reply - display content of received ping reply
    else if (icmp_header->type == ICMP_TYPE_ECHO_REPLY && icmp_header->code == 0) {
        // Get the data portion of the ICMP packet
        
        // 总是输出这个关键信息，无论debug模式是否开启
        serial_write_string("\r\n!!! ICMP ECHO REPLY RECEIVED !!!\r\n");
        serial_write_string("From IP: ");
        serial_print_ip(ntohl(ip_header->src_ip));
        serial_write_string("\r\n");
        
        // Clear part of the screen but leave the ping history
        terminal_writestring("\n");
        terminal_writestring("=======================================\n");
        terminal_writestring("🔔 PING REPLY RECEIVED FROM HOST 🔔\n");
        terminal_writestring("=======================================\n");
        terminal_writestring("From IP: ");
        print_ip(ntohl(ip_header->src_ip));
        terminal_writestring("\n\n");
        
        // 强调这是收到的回复
        terminal_writestring("✓ Successfully received ICMP echo reply!\n\n");
        
        terminal_writestring("Message content: \"");
        // Display the data content (assuming printable ASCII)
        for (uint16_t i = 0; i < data_length; i++) {
            if (data[i] >= 32 && data[i] <= 126) { // Printable ASCII
                terminal_putchar(data[i]);
                // 发送到串口也显示
                char c = data[i];
                serial_write_string(&c);
            } else if (data[i] == 0) {
                // End of string
                break;
            } else {
                terminal_writestring(".");
                serial_write_string(".");
            }
        }
        terminal_writestring("\"\n\n");
        serial_write_string("\"\r\n");
        
        terminal_writestring("Hex representation: ");
        serial_write_string("Hex data: ");
        for (uint16_t i = 0; i < data_length && i < 32; i++) {
            terminal_writehex8(data[i]);
            terminal_writestring(" ");
            serial_write_hex8(data[i]);
            serial_write_string(" ");
            if (i % 8 == 7) {
                terminal_writestring("\n                    ");
                serial_write_string("\r\n                    ");
            }
        }
        
        terminal_writestring("\n\n");
        terminal_writestring("Waiting for next ping reply...\n");
        terminal_writestring("=======================================\n");

        // Serial logging for ICMP echo reply
        serial_write_string("\r\n=== PING REPLY RECEIVED ===\r\n");
        serial_write_string("From IP: ");
        serial_print_ip(ntohl(ip_header->src_ip));
        serial_write_string("\r\nData: \"");
        for (uint16_t i = 0; i < data_length; i++) {
            if (data[i] >= 32 && data[i] <= 126) { // Printable ASCII
                char c = data[i];
                serial_write_string(&c);
            } else if (data[i] == 0) {
                break;
            } else {
                serial_write_string(".");
            }
        }
        serial_write_string("\"\r\n=========================\r\n");
    }
}

// Send ICMP Echo reply
void send_icmp_echo_reply(struct icmp_header *request, uint8_t *packet, uint16_t length, struct ipv4_header *ip_header) {
    // Allocate send buffer
    uint8_t buffer[ETH_MTU];
    memcpy(buffer, packet, length);

    // Get ethernet header and IP header pointers
    struct eth_header *eth = (struct eth_header *)buffer;
    struct ipv4_header *ip = (struct ipv4_header *)(buffer + sizeof(struct eth_header));
    struct icmp_header *icmp = (struct icmp_header *)(buffer + sizeof(struct eth_header) + sizeof(struct ipv4_header));

    // Log the original IPs for debugging
    serial_write_string("Original IPs - Src: ");
    serial_print_ip(ntohl(ip->src_ip));
    serial_write_string(" Dst: ");
    serial_print_ip(ntohl(ip->dst_ip));
    serial_write_string("\r\n");

    // Swap MAC addresses
    uint8_t temp_mac[6];
    memcpy(temp_mac, eth->dest_mac, 6);
    memcpy(eth->dest_mac, eth->src_mac, 6);
    memcpy(eth->src_mac, temp_mac, 6);

    // Swap IP addresses - they are already in network byte order
    uint32_t temp_ip = ip->dst_ip;
    ip->dst_ip = ip->src_ip;
    ip->src_ip = temp_ip;

    // Log the swapped IPs for debugging
    serial_write_string("Swapped IPs - Src: ");
    serial_print_ip(ntohl(ip->src_ip));
    serial_write_string(" Dst: ");
    serial_print_ip(ntohl(ip->dst_ip));
    serial_write_string("\r\n");

    // Modify ICMP packet to Echo reply
    icmp->type = ICMP_TYPE_ECHO_REPLY;
    icmp->code = 0;
    icmp->checksum = 0;
    
    // Add "hello,world" to ICMP reply data
    char *hello_msg = "hello,world";
    uint16_t hello_len = 12; // Including null terminator
    uint16_t data_offset = sizeof(struct eth_header) + sizeof(struct ipv4_header) + sizeof(struct icmp_header);
    
    memcpy(buffer + data_offset, hello_msg, hello_len);
    
    // Update IP total length
    uint16_t total_length = sizeof(struct ipv4_header) + sizeof(struct icmp_header) + hello_len;
    ip->total_length = htons(total_length);
    
    // Calculate ICMP length and recompute checksum
    uint16_t icmp_len = sizeof(struct icmp_header) + hello_len;
    icmp->checksum = network_checksum((uint8_t *)icmp, icmp_len);

    // Recompute IP header checksum
    ip->checksum = 0;
    ip->checksum = network_checksum((uint8_t *)ip, sizeof(struct ipv4_header));

    uint16_t packet_length = sizeof(struct eth_header) + ntohs(ip->total_length);
    
    // Log to serial
    serial_write_string("\r\n=== SENDING ICMP ECHO REPLY ===\r\n");
    serial_write_string("To IP: ");
    serial_print_ip(ntohl(ip->dst_ip));
    serial_write_string("\r\nWith data: \"hello,world\"\r\n");
    
    // Send Echo reply
    network_send_packet(buffer, packet_length);
    
    // Display success message
    terminal_writestring("\n✓ Successfully sent ICMP reply with 'hello,world'\n");
    terminal_writestring("\nTo IP address: ");
    print_ip(ntohl(ip->dst_ip));
    terminal_writestring("\n\n");
    terminal_writestring("=== Waiting for next PING request ===\n");
    
    serial_write_string("\r\n✓ Successfully sent ICMP reply with 'hello,world'\r\n");
    serial_write_string("\r\nTo IP address: ");
    serial_print_ip(ntohl(ip->dst_ip));
    serial_write_string("\r\n\r\n");
}

// Send ICMP Echo request with "hello,world" data
void send_icmp_echo_request(uint32_t target_ip) {
    // Log to serial port
    serial_write_string("\r\n=== SENDING ICMP ECHO REQUEST ===\r\n");
    serial_write_string("Target IP: ");
    serial_print_ip(target_ip);  // 正确：直接使用主机字节序打印
    serial_write_string("\r\n");

    // First make sure we have the MAC address for the target IP
    uint8_t target_mac[6];
    if (!get_destination_mac(target_ip, target_mac)) {
        terminal_writestring("Failed to get MAC address for target IP\n");
        serial_write_string("Failed to get MAC address for target IP\r\n");
        return;
    }
    
    serial_write_string("Target MAC: ");
    for (int i = 0; i < 6; i++) {
        serial_write_hex8(target_mac[i]);
        if (i < 5) serial_write_string(":");
    }
    serial_write_string("\r\n");
    
    // Create a buffer for the ICMP echo request packet
    uint8_t buffer[ETH_MTU];
    memset(buffer, 0, ETH_MTU);
    
    // Set up ethernet header
    struct eth_header *eth = (struct eth_header *)buffer;
    memcpy(eth->dest_mac, target_mac, 6);
    memcpy(eth->src_mac, net_dev.mac_addr, 6);
    eth->type = htons(ETH_TYPE_IP);
    
    // Set up IP header
    struct ipv4_header *ip = (struct ipv4_header *)(buffer + sizeof(struct eth_header));
    ip->version_ihl = 0x45;  // Version 4, header length 5 DWORDs
    ip->dscp_ecn = 0;
    ip->identification = 0;
    ip->flags_fragment_offset = 0;
    ip->ttl = 64;
    ip->protocol = IP_PROTO_ICMP;
    ip->src_ip = htonl(net_dev.ip_addr);  // 必须转换为网络字节序
    ip->dst_ip = htonl(target_ip);        // 必须转换为网络字节序
    
    // Set up ICMP header
    struct icmp_header *icmp = (struct icmp_header *)(buffer + sizeof(struct eth_header) + sizeof(struct ipv4_header));
    icmp->type = ICMP_TYPE_ECHO_REQUEST;
    icmp->code = 0;
    icmp->identifier = htons(0x1234);  // Some identifier
    icmp->sequence = htons(0x0001);    // Sequence number
    
    // Add "hello,world" data
    char *hello_msg = "hello,world";
    uint16_t hello_len = 12;  // Including null terminator
    uint16_t data_offset = sizeof(struct eth_header) + sizeof(struct ipv4_header) + sizeof(struct icmp_header);
    memcpy(buffer + data_offset, hello_msg, hello_len);
    
    // Calculate total length for IP header
    uint16_t total_length = sizeof(struct ipv4_header) + sizeof(struct icmp_header) + hello_len;
    ip->total_length = htons(total_length);
    
    // Calculate checksums
    ip->checksum = 0;
    ip->checksum = network_checksum((uint8_t *)ip, sizeof(struct ipv4_header));
    
    icmp->checksum = 0;
    icmp->checksum = network_checksum((uint8_t *)icmp, sizeof(struct icmp_header) + hello_len);
    
    // Total packet length
    uint16_t packet_length = sizeof(struct eth_header) + total_length;
    
    // Log packet details to serial
    serial_write_string("Packet length: ");
    serial_write_dec(packet_length);
    serial_write_string(" bytes\r\n");
    serial_write_string("Data: \"hello,world\"\r\n");
    
    // Clear screen and display status
    terminal_clear();
    terminal_writestring("\n\n");
    terminal_writestring("======================================\n");
    terminal_writestring("     MiniOS Hello World Network Demo     \n");
    terminal_writestring("======================================\n\n");
    
    terminal_writestring("Sending PING with 'hello,world' to: ");
    print_ip(target_ip);  // 使用更新后的print_ip函数，显示正确的字节序
    terminal_writestring("\n\n");
    
    // Send the packet
    network_send_packet(buffer, packet_length);
    
    serial_write_string("ICMP Echo Request sent!\r\n");
    
    terminal_writestring("✓ PING sent successfully!\n");
    terminal_writestring("Waiting for replies...\n\n");
    terminal_writestring("Any received replies will be shown below:\n");
    terminal_writestring("======================================\n");
}
