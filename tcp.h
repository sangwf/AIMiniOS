#ifndef TCP_H
#define TCP_H

#include "types.h"
#include "ipv4.h"

// TCP 标志位
#define TCP_FLAG_FIN      0x01
#define TCP_FLAG_SYN      0x02
#define TCP_FLAG_RST      0x04
#define TCP_FLAG_PSH      0x08
#define TCP_FLAG_ACK      0x10
#define TCP_FLAG_URG      0x20

// TCP 连接状态
#define TCP_CLOSED       0
#define TCP_LISTEN       1
#define TCP_SYN_SENT     2
#define TCP_SYN_RECEIVED 3
#define TCP_ESTABLISHED  4
#define TCP_FIN_WAIT_1   5
#define TCP_FIN_WAIT_2   6
#define TCP_TIME_WAIT    7
#define TCP_CLOSING      8
#define TCP_CLOSE_WAIT   9
#define TCP_LAST_ACK     10

#define MAX_TCP_CONNECTIONS 8
#define TCP_WINDOW_SIZE 8192

// TCP头结构体
struct tcp_header {
    uint16_t src_port;        // 源端口
    uint16_t dest_port;       // 目标端口
    uint32_t seq_num;         // 序列号
    uint32_t ack_num;         // 确认号
    uint8_t data_offset;      // 数据偏移 (4位) + 保留位 (4位)
    uint8_t flags;            // 控制位
    uint16_t window;          // 窗口大小
    uint16_t checksum;        // 校验和
    uint16_t urgent_ptr;      // 紧急指针
    // 选项 (可选)
} __attribute__((packed));

// TCP伪头部，用于计算校验和
struct tcp_pseudo_header {
    uint32_t src_ip;         // 源IP
    uint32_t dest_ip;        // 目标IP
    uint8_t zero;            // 保留位(必须为0)
    uint8_t protocol;        // 协议(TCP为6)
    uint16_t tcp_length;     // TCP长度(头部+数据)
} __attribute__((packed));

// TCP 连接结构
struct tcp_connection {
    uint8_t state;          // 连接状态
    uint32_t local_ip;      // 本地IP
    uint16_t local_port;    // 本地端口
    uint32_t remote_ip;     // 远程IP
    uint16_t remote_port;   // 远程端口
    uint32_t seq_num;       // 当前序列号
    uint32_t ack_num;       // 当前确认号
    uint16_t window;        // 当前窗口大小
    uint8_t retries;        // 重试次数
};

// 数据接收回调函数类型
typedef void (*tcp_data_callback)(struct tcp_connection* conn, uint8_t* data, uint16_t length);

// TCP初始化
void tcp_init(void);

// TCP数据包处理
void handle_tcp_packet(uint8_t* packet, uint16_t length);

// TCP连接管理
struct tcp_connection* tcp_create_connection(void);
void tcp_free_connection(struct tcp_connection* conn);
int tcp_connect_ip(uint32_t dest_ip, uint16_t dest_port);
int tcp_send_data(struct tcp_connection* conn, uint8_t* data, uint16_t length);
void tcp_close_conn(struct tcp_connection* conn);
void tcp_set_data_callback(struct tcp_connection* conn, tcp_data_callback callback);

// TCP监听端口
int tcp_listen(uint16_t port);

// TCP Socket style API
typedef struct sockaddr_in {
    uint32_t sin_addr;
    uint16_t sin_port;
} sockaddr_in_t;

int tcp_socket(void);
int tcp_connect(int sockfd, struct sockaddr_in* server);
int tcp_send(int sockfd, char* data, int length);
int tcp_close(int sockfd);

#endif // TCP_H 