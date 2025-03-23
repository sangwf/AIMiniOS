global idt_load
global isr_default
global rtl8139_handler

extern handle_rtl8139_interrupt

section .text
idt_load:
    push ebp
    mov ebp, esp
    mov eax, [ebp + 8]  ; 获取IDT指针参数
    lidt [eax]         ; 加载IDT
    pop ebp
    ret

isr_default:
    pushad            ; 保存所有通用寄存器
    
    ; 这里可以添加默认的中断处理代码
    ; 现在我们只是简单地忽略中断
    
    popad             ; 恢复所有通用寄存器
    iret              ; 中断返回

rtl8139_handler:
    pushad           ; 保存所有通用寄存器
    
    call handle_rtl8139_interrupt
    
    popad            ; 恢复所有通用寄存器
    iret             ; 中断返回