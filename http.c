#include "http.h"
#include "tcp.h"
#include "network.h"
#include "terminal.h"

// 定义Hello World服务器端口
#define HELLO_SERVER_PORT 80

// 启动HTTP服务器
void start_http_server(void) {
    terminal_writestring("Starting HTTP server on port 80...\n");
    
    // 开始监听
    if(tcp_listen(HELLO_SERVER_PORT) < 0) {
        terminal_writestring("Failed to start listening\n");
        return;
    }
    
    terminal_writestring("HTTP server started successfully!\n");
}

// 字符串长度计算函数
static int my_strlen(const char* str) {
    int len = 0;
    while(str[len]) len++;
    return len;
}

// 网络Hello World测试函数
void test_hello_world_network(void) {
    terminal_writestring("[HTTP] Starting HTTP client test\n");

    // 创建套接字
    int sockfd = tcp_socket();
    if (sockfd < 0) {
        terminal_writestring("[HTTP] Failed to create socket\n");
        return;
    }
    terminal_writestring("[HTTP] Socket created\n");

    // 连接到服务器10.0.2.2:80
    struct sockaddr_in server;
    server.sin_addr = 0x0A000202; // 10.0.2.2
    server.sin_port = 80;
    
    terminal_writestring("[HTTP] Connecting to 10.0.2.2:80\n");
    if (tcp_connect(sockfd, &server) < 0) {
        terminal_writestring("[HTTP] Failed to connect\n");
        return;
    }
    terminal_writestring("[HTTP] Connection established\n");

    // 发送消息
    char *message = "GET / HTTP/1.1\r\nHost: 10.0.2.2\r\n\r\n";
    terminal_writestring("[HTTP] Sending message: GET / HTTP/1.1\r\n");
    if (tcp_send(sockfd, message, my_strlen(message)) < 0) {
        terminal_writestring("[HTTP] Failed to send message\n");
        return;
    }
    terminal_writestring("[HTTP] Message sent successfully\n");

    // 关闭套接字
    tcp_close(sockfd);
    terminal_writestring("[HTTP] Socket closed\n");
} 