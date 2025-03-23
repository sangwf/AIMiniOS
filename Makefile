CC = x86_64-elf-gcc
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nodefaultlibs -I. -g -O0 -fno-omit-frame-pointer -fno-pic -fno-pie -ffreestanding -Wall -Wextra -mno-mmx -mno-sse -mno-sse2 -DDEBUG
LD = x86_64-elf-ld
LDFLAGS = -T link.ld -m elf_i386 --no-warn-rwx-segments
ASM = nasm
ASMFLAGS = -f elf32 -g -F dwarf

OBJS = boot.o kernel.o terminal.o gdt.o gdt_asm.o idt.o idt_asm.o network.o pci.o memory.o tcp.o http.o rtl8139.o arp.o serial.o

.PHONY: all clean run run_debug run_nodebug

all: kernel.bin

kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

clean:
	rm -f *.o kernel.bin *.log *.pcap

# 普通运行模式
run: kernel.bin
	qemu-system-i386 -kernel kernel.bin \
		-netdev user,id=mynet0,net=10.0.2.0/24,host=10.0.2.2,dhcpstart=10.0.2.15 \
		-device rtl8139,netdev=mynet0,mac=52:54:00:12:34:56 \
		-no-reboot -no-shutdown \
		-d int \
		-D qemu.log \
		-monitor stdio \
		-serial file:serial_output.log \
		-object filter-dump,id=f1,netdev=mynet0,file=network_dump.pcap

# 调试模式：RTL调试输出启用
run_debug: kernel.bin
	# 修改kernel.c中的disable_rtl_debug值为false
	sed -i.bak 's/disable_rtl_debug = true;/disable_rtl_debug = false;/g' kernel.c
	make
	qemu-system-i386 -kernel kernel.bin \
		-netdev user,id=mynet0,net=10.0.2.0/24,host=10.0.2.2,dhcpstart=10.0.2.15 \
		-device rtl8139,netdev=mynet0,mac=52:54:00:12:34:56 \
		-no-reboot -no-shutdown \
		-monitor stdio \
		-serial file:serial_debug.log \
		-object filter-dump,id=f1,netdev=mynet0,file=network_dump.pcap
	# 恢复原始文件
	mv kernel.c.bak kernel.c

# 无调试模式：RTL调试输出禁用
run_nodebug: kernel.bin
	# 修改kernel.c中的disable_rtl_debug值为true
	sed -i.bak 's/disable_rtl_debug = false;/disable_rtl_debug = true;/g' kernel.c
	make
	qemu-system-i386 -kernel kernel.bin \
		-netdev user,id=mynet0,net=10.0.2.0/24,host=10.0.2.2,dhcpstart=10.0.2.15 \
		-device rtl8139,netdev=mynet0,mac=52:54:00:12:34:56 \
		-no-reboot -no-shutdown \
		-monitor stdio \
		-serial file:serial_nodebug.log \
		-object filter-dump,id=f1,netdev=mynet0,file=network_dump.pcap
	# 恢复原始文件
	mv kernel.c.bak kernel.c
