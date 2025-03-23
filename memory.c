#include "kernel.h"
#include "memory.h"

// 内核堆的起始和结束地址
extern uint32_t kernel_end;
static uint32_t heap_end = (uint32_t)&kernel_end;

// 简单的内存分配器
void* kmalloc(size_t size) {
    // 对齐到4字节边界
    size = (size + 3) & ~3;
    
    // 分配内存
    uint32_t addr = heap_end;
    heap_end += size;
    
    return (void*)addr;
}

// 简单的内存释放函数
void kfree(void* ptr) {
    // 这个简单的实现不做任何事情
    (void)ptr;
}

void* memset(void* dest, int val, size_t len) {
    uint8_t* ptr = (uint8_t*)dest;
    while(len-- > 0) {
        *ptr++ = val;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, size_t len) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while(len-- > 0) {
        *d++ = *s++;
    }
    return dest;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* p1 = s1;
    const unsigned char* p2 = s2;
    while(n--) {
        if(*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
} 