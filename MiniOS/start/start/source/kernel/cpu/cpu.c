#include "cpu/cpu.h"
#include "ipc/mutex.h"
#include "osConfig.h"
#include "common/CPUInlineFun.h"
#include "core/syscall.h"

static segment_desc_t gdt_table[GDT_TABLE_SIZE];
static mutex_t mutex;

void segment_desc_set(int sector, u32 base, u32 limit, u16 attr)
{
    segment_desc_t *desc = gdt_table + sector / 8;
    if (limit > 0xfffff)
    {
        attr |= 0x8000;
        limit /= 0x1000;
    }
    desc->limit15_0 = limit & 0xffff;
    desc->base15_0 = base & 0xffff;
    desc->base23_16 = (base >> 16) & 0xff;
    desc->attr = attr | (((limit >> 16) & 0xf) << 8);
    desc->base31_24 = (base >> 24) & 0xff;
}

void gate_desc_set(gate_desc_t *desc, u16 selector, u32 offset, u16 attr)
{
    desc->offset15_0 = offset & 0xffff;
    desc->selector = selector;
    desc->attr = attr;
    desc->offset31_16 = (offset >> 16) & 0xffff;
}

void initGDT(void)
{
    for (int i = 0; i < GDT_TABLE_SIZE; i++)
    {
        segment_desc_set(i << 3, 0, 0, 0);
    }

    segment_desc_set(KERNEL_SELECTOR_CS, 0, 0xffffffff,
                   SEG_P_PARENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_CODE | SEG_D);

    segment_desc_set(KERNEL_SELECTOR_DS, 0, 0xffffffff,
                   SEG_P_PARENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_DATA | SEG_TYPE_RW | SEG_D);

    gate_desc_set((gate_desc_t *)(gdt_table + (SELECTOR_SYSCALL >> 3)), KERNEL_SELECTOR_CS, 
    (u32)exeception_handler_syscall, GATE_P_PRESENT | GATE_DPL3 | GATE_TYPE_SYSCALL | SYSCALL_PARAM_COUNT);

    lgdt((u32)gdt_table, sizeof(gdt_table));
}

// 此处临界区/互斥锁设置存疑
int gdt_alloc_desc(){
    mutex_lock(&mutex);
    for (int i = 1; i < GDT_TABLE_SIZE; ++i){
        segment_desc_t *desc = gdt_table + i;
        if (desc->attr == 0) {
            desc->attr++;       // 此处为了方便实现互斥
            mutex_unlock(&mutex);
            // desc->attr--;       // 恢复属性, 此处似乎不太需要因为返回去的gdt表会被重新初始化
            return i * sizeof(segment_desc_t);
        }
    }
    mutex_unlock(&mutex);
    return -1;
}

void gdt_free_sel(int sel){
    mutex_lock(&mutex);

    gdt_table[sel / sizeof(segment_desc_t)].attr = 0;

    mutex_unlock(&mutex);
}

void switch_to_tss(int tss_sel){
    farJump(tss_sel, 0);
}


void cpu_init(void)
{
    mutex_init(&mutex);
    initGDT();
}