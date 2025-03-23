global gdt_flush

section .text
gdt_flush:
    ; 保存参数
    mov eax, [esp + 4]
    
    ; 加载GDT
    cli                 ; 禁用中断
    lgdt [eax]         ; 加载GDT表
    
    ; 通过远跳转切换到新的代码段
    jmp 0x08:.reload_CS  ; 0x08 是代码段选择子

.reload_CS:
    ; 重新加载数据段寄存器
    mov ax, 0x10       ; 0x10 是数据段选择子
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; 保持栈指针不变，但确保它在正确的段中
    mov esp, esp
    
    ; 返回，但不启用中断（让内核决定何时启用中断）
    ret