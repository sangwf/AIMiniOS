#ifndef BYTEORDER_H
#define BYTEORDER_H

#include "types.h"

// 网络字节序转换函数
static inline uint16_t htons(uint16_t x) {
    return ((x & 0xFF) << 8) | ((x & 0xFF00) >> 8);
}

static inline uint16_t ntohs(uint16_t x) {
    return htons(x);
}

static inline uint32_t htonl(uint32_t x) {
    return ((x & 0xFF) << 24) |
           ((x & 0xFF00) << 8) |
           ((x & 0xFF0000) >> 8) |
           ((x & 0xFF000000) >> 24);
}

static inline uint32_t ntohl(uint32_t x) {
    return htonl(x);
}

#endif // BYTEORDER_H 