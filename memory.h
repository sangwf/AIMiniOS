#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

// 内存操作函数声明
void* memset(void* dest, int val, size_t len);
void* memcpy(void* dest, const void* src, size_t len);
int memcmp(const void* s1, const void* s2, size_t n);

// 内存分配函数声明
void* kmalloc(size_t size);
void kfree(void* ptr);

#endif // MEMORY_H 