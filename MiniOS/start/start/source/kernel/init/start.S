#include "osConfig.h"
    .text
    .extern kernelInit
    .extern init_main
    .global kernelInit
_start:
    push 4(%esp)
    call kernelInit
    jmp $KERNEL_SELECTOR_CS, $gdtReload # 用于设置cs的值
gdtReload:
    mov $KERNEL_SELECTOR_DS, %ax
	mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

    mov $(stack + KERNEL_STACK_SIZE), %esp
    jmp init_main

    .bss
.comm stack, KERNEL_STACK_SIZE

.macro exception_handler name num with_error_code
    .text
    .extern do_handler_\name
    .global exception_handler_\name
exception_handler_\name:
    .if \with_error_code == 0
        push $0
    .endif

    push $\num

    pusha
    push %ds
    push %es
    push %fs
    push %gs

    push %esp

    call do_handler_\name

    pop %esp

    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa

    add $(2*4), %esp

    iret
.endm

exception_handler unknow, -1, 0
exception_handler divider, 0, 0
exception_handler debug, 1, 0
exception_handler NMI, 2, 0
exception_handler breakpoint, 3, 0
exception_handler overflow, 4, 0
exception_handler BOUND_Range, 5, 0
exception_handler invaild_opcode, 6, 0
exception_handler device_unavailbale, 7, 0
exception_handler double_fault, 8, 1
exception_handler invaild_tss, 10, 1
exception_handler segment_not_present, 11, 1
exception_handler stack_segmant_fault, 12, 1
exception_handler general_protection, 13, 1
exception_handler page_fault, 14, 1
exception_handler fpu_error, 16, 0
exception_handler aligment_check, 17, 1
exception_handler machine_check, 18, 0
exception_handler simd_exception, 19, 0
exception_handler virtual_exception, 20, 0
exception_handler control_exception, 21, 1

exception_handler time, 0x20, 0
exception_handler kbd, 0x21, 0
exception_handler ide_primary, 0x2E, 0
    // simple_switch(&from, &to)
    .text
    .global simple_switch
simple_switch:
    mov 4(%esp), %eax
    mov 8(%esp), %edx


    push %ebp
    push %ebx
    push %esi
    push %edi
    mov %esp, (%eax)

    
    mov %edx, %esp
    pop %edi
    pop %esi
    pop %ebx
    pop %ebp
    ret

    .global do_handler_syscall
    .global exeception_handler_syscall
exeception_handler_syscall:
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    pushf

    mov %esp, %eax
    push %eax
    call do_handler_syscall
    add $4, %esp


    popf
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa


    retf $(5*4)





