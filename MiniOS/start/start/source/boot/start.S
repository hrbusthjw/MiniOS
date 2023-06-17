	#include "boot.h"

  	// 16位代码，务必加上
  	.code16
 	.text
	.global _start
	.extern boot_entry
_start:
	mov $0, %ax
	mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
# 0x7c00压栈 做boot的栈区
	mov $_start, %esp

	mov $0xe, %ah
	mov  $0x0D, %al
	int $0x10

	mov  $0x0a, %al
	int $0x10

	mov $'J', %al
	int $0x10

	mov $'u', %al
	int $0x10

	mov $'m', %al
	int $0x10

	mov $'p', %al
	int $0x10

	mov $' ', %al
	int $0x10

	mov $'t', %al
	int $0x10

	mov $'o', %al
	int $0x10

	mov $' ', %al
	int $0x10

	mov $'l', %al
	int $0x10

	mov $'o', %al
	int $0x10

	mov $'a', %al
	int $0x10

	mov $'d', %al
	int $0x10

	mov $'e', %al
	int $0x10

	mov $'r', %al
	int $0x10

# bios 加载第二个扇区进入0x8000内存，即进入loader代码区域
read_loader:
	mov $0x8000, %bx
	mov $0x2, %ah
	mov $0x2, %cx
	mov $64, %al
	mov $0x80, %dx
	int $0x13
	jc read_loader

	jmp boot_entry

	jmp .
	
	// 引导结束段
	.section boot_end, "ax"
boot_sig: .byte 0x55, 0xaa
