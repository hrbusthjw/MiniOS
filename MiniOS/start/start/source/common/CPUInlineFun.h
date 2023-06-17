#ifndef CPUINLINEFUN_H
#define CPUINLINEFUN_H

#include "types.h"

// 关中断
static inline void cli(void)
{
    __asm__ __volatile__("cli");
}
// 开中断
static inline void sti(void)
{
    __asm__ __volatile__("sti");
}

// inb, 往指定端口读一个字节
static inline u8 inb(u16 port)
{
    u8 returnValue;
    // inb 用法：inb al, dx
    __asm__ __volatile__("inb %[p], %[v]"
                         : [v] "=a"(returnValue)
                         : [p] "d"(port));
    return returnValue;
}

// inw, 往指定端口读两个字节
static inline u16 inw(u16 port)
{
    u16 returnValue;
    // in 用法：in ax, dx
    __asm__ __volatile__("in %1, %0"
                         : "=a"(returnValue)
                         : "dN"(port));
    return returnValue;
}

static inline void outw(u16 port, u16 data)
{
    // outw 用法：in ax, dx
    __asm__ __volatile__("out %[v], %[p]"::[p]"d"(port), [v]"a"(data));
}

// outb, 往指定端口写一个字节
static inline void outb(u16 port, u8 data)
{
    // outb 用法：outb al, dx
    __asm__ __volatile("outb %[v], %[p]" ::[p] "d"(port), [v] "a"(data));
}

// lgdt加载gdt表
static inline void lgdt(u32 start, u32 size)
{
    struct
    {
        u16 limit;
        u16 start15_0;
        u16 start31_16;
    } gdt;

    gdt.start31_16 = start >> 16;
    gdt.start15_0 = start & 0xffff;
    gdt.limit = size - 1;

    __asm __volatile__("lgdt %[g]" ::[g] "m"(gdt));
}

// 加载中断向量表
static inline void lidt(u32 start, u32 size)
{
    struct
    {
        u16 limit;
        u16 start15_0;
        u16 start31_16;
    } idt;

    idt.start31_16 = start >> 16;
    idt.start15_0 = start & 0xffff;
    idt.limit = size - 1;

    __asm __volatile__("lidt %[g]" ::[g] "m"(idt));
}

// 读cr0
static inline u32 readCR0()
{
    u32 cr0;
    __asm__ __volatile__("mov %%cr0, %[v]":[v] "=r"(cr0));
    return cr0;
}

// 写cr0
static inline void writeCR0(u32 value)
{
    __asm__ __volatile__("mov %[v], %%cr0" ::[v] "r"(value));
}

static inline u32 readCR2()
{
    u32 cr2;
    __asm__ __volatile__("mov %%cr2, %[v]":[v]"=r"(cr2));
    return cr2;
}

static inline u32 readCR3()
{
    u32 cr3;
    __asm__ __volatile__("mov %%cr3, %[v]":[v]"=r"(cr3));
    return cr3;
}

static inline void writeCR3(u32 value)
{
    __asm__ __volatile__("mov %[v], %%cr3"::[v] "r"(value));
}

static inline u32 readCR4()
{
    u32 cr4;
    __asm__ __volatile__("mov %%cr4, %[v]":[v]"=r"(cr4));
    return cr4;
}

static inline void writeCR4(u32 value)
{
    __asm__ __volatile__("mov %[v], %%cr4"::[v] "r"(value));
}

// 远跳转
static inline void farJump(u32 selector, u32 offset)
{
    u32 addr[] = {offset, selector};
    __asm__ __volatile__("ljmpl *(%[a])" ::[a] "r"(addr));
}
// cpu停止
static inline u16 hlt()
{
    __asm__ __volatile__("hlt");
}

static inline void write_tr (u16 tss_sel){
    __asm__ __volatile__("ltr %%ax"::"a"(tss_sel));
}

static inline u32 read_eflags(void) {
    u32 eflags;
    __asm__ __volatile__("pushf\n\tpop %%eax":"=a"(eflags));
    return eflags;
}

static inline void write_eflags(u32 eflags) {
    __asm__ __volatile__("push %%eax\n\tpopf"::"a"(eflags));
}

#endif