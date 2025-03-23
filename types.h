#ifndef TYPES_H
#define TYPES_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef unsigned int size_t;

// 布尔类型定义
#ifndef __cplusplus
typedef _Bool bool;
#define true 1
#define false 0
#endif

// NULL定义
#define NULL ((void*)0)

#endif /* TYPES_H */
