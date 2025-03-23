; boot.asm
; Multiboot header
MULTIBOOT_PAGE_ALIGN    equ 1<<0
MULTIBOOT_MEMORY_INFO   equ 1<<1
MULTIBOOT_HEADER_MAGIC  equ 0x1BADB002
MULTIBOOT_HEADER_FLAGS  equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
MULTIBOOT_CHECKSUM      equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

section .text
align 4

; Multiboot header
dd MULTIBOOT_HEADER_MAGIC
dd MULTIBOOT_HEADER_FLAGS
dd MULTIBOOT_CHECKSUM

; Kernel entry point
global _start
global start
extern kernel_main

_start:
start:
    ; Set up the stack
    mov esp, stack_top

    ; Push multiboot info struct pointer
    push ebx

    ; Call the kernel
    call kernel_main

    ; Halt if we return from the kernel
    cli
.hang:
    hlt
    jmp .hang

; Stack setup
section .bss
align 16
stack_bottom:
    resb 16384 ; 16 KiB
stack_top:
