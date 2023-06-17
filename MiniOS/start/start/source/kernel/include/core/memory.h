#ifndef MEMORY_H
#define MEMROY_H
#include "common/types.h"
#include "tools/bitmap.h"
#include "ipc/mutex.h"
#include "common/boot_info.h"

#define MEM_EXT_START       (1024*1024)
#define MEM_PAGE_SIZE       (4*1024)
#define MEM_EBDA_START      0x80000
#define MEMORY_TASK_BASE    0X80000000
#define MEM_EXT_END         (127 * 1024 * 1024)
#define MEM_TASK_STACK_TOP  0XE0000000
#define MEM_TASK_STACK_SIZE (MEM_PAGE_SIZE * 500)
#define MEM_TASK_ARG_SIZE   (MEM_PAGE_SIZE * 4)

// 地址分配结构 记录了一块内存的开始地址，大小，页大小和位图
typedef struct _addr_alloc_t{
    mutex_t mutex;
    u32 start;
    u32 size;
    u32 page_size;
    bitmap_t bitmap;
}addr_alloc_t;

// 内存分配结构
typedef struct _memory_map_t{
    void *vstart;
    void *vend;
    void *pstart;
    u32 perm;   //特权相关属性
}memory_map_t;

void memory_init(boot_info_t *boot_info);
u32 memory_create_uvm(void);
int memory_alloc_page_for(u32 addr, u32 size, u32 perm);

u32 memory_alloc_page(void);
void memory_free_page(u32 addr);
int memory_alloc_for_page_dir(u32 page_dir, u32 vaddr, u32 size, u32 perm);

void memory_destory_uvm(u32 page_dir);
u32 memory_copy_uvm(u32 page_dir);
u32 memory_get_paddr(u32 page_dir, u32 vaddr);

int memory_copy_uvm_data(u32 to, u32 page_dir, u32 from, u32 size);

// newlib库需要
void *sys_sbrk(int incr);

#endif