#ifndef CPU_H
#define CPU_H
#include "common/types.h"

#define SEG_G (1 << 15)
#define SEG_D (1 << 14)
#define SEG_P_PARENT (1 << 7)

#define SEG_DPL0 (0 << 5)
#define SEG_DPL3 (3 << 5)

#define SEG_CPL0 (0 << 0)
#define SEG_CPL3 (3 << 0)

#define SEG_S_SYSTEM (0 << 4)
#define SEG_S_NORMAL (1 << 4)

#define SEG_TYPE_CODE (1 << 3)
#define SEG_TYPE_DATA (0 << 3)

#define SEG_TYPE_RW (1 << 1)

#define SEG_TYPE_TSS    (9 << 0)

// 中断门属性宏
#define GATE_P_PRESENT (1 << 15)  // 段存在
#define GATE_DPL0 (0 << 13)      // 最高权限
#define GATE_DPL3 (3 << 13)      // 最低权限
#define GATE_TYPE_INT (0XE << 8) // 32位模式,并且是中断门，而不是陷阱门或其他
#define GATE_TYPE_SYSCALL   (0XC << 8) //系统调用门描述符

#define EFLAGS_DEFAULT      (1 << 1)
#define EFLAGS_IF           (1 << 9)

// GDT表项
#pragma pack(1)
typedef struct _segment_desc_t
{
    u16 limit15_0;
    u16 base15_0;
    u8 base23_16;
    u16 attr;
    u8 base31_24;
} segment_desc_t;

typedef struct _gate_disc_t
{
    u16 offset15_0;
    u16 selector;
    u16 attr;
    u16 offset31_16;
} gate_desc_t;

typedef struct _tss_t {
    u32 pre_link;
    u32 esp0, ss0, esp1, ss1, esp2, ss2;
    u32 cr3;
    u32 eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    u32 es, cs, ss, ds, fs, gs;
    u32 ldt;
    u32 iomap;
}tss_t;


#pragma pack()

void cpu_init(void);
void segment_desc_set(int sector, u32 base, u32 limit, u16 attr);
void gate_desc_set(gate_desc_t *desc, u16 selector, u32 offset, u16 attr);

void gdt_free_sel(int sel);

int gdt_alloc_desc();
void switch_to_tss(int tss_sel);

#endif