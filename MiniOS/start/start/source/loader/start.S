	# 16位代码，务必加上
  	.code16
 	.text
	.global _start
	.extern loaderEntry
_start:
	jmp loaderEntry

	.code32
	.text
	.global protectedModeEntry
	.extern loaderKernel
protectedModeEntry:
	mov $16, %ax
	mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	jmp $8, $loaderKernel