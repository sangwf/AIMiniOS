global irq8_handler
global irq9_handler
global irq10_handler
global irq11_handler
global irq12_handler
global irq13_handler
global irq14_handler
global irq15_handler

irq8_handler:
    cli
    push byte 8
    push byte 40
    jmp irq_common_stub

irq9_handler:
    cli
    push byte 9
    push byte 41
    jmp irq_common_stub

irq10_handler:
    cli
    push byte 10
    push byte 42
    jmp irq_common_stub

irq11_handler:
    cli
    push byte 11
    push byte 43
    jmp irq_common_stub

irq12_handler:
    cli
    push byte 12
    push byte 44
    jmp irq_common_stub

irq13_handler:
    cli
    push byte 13
    push byte 45
    jmp irq_common_stub

irq14_handler:
    cli
    push byte 14
    push byte 46
    jmp irq_common_stub

irq15_handler:
    cli
    push byte 15
    push byte 47
    jmp irq_common_stub 