#include "core/memory.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "cpu/mmu.h"
#include "dev/console.h"

static addr_alloc_t paddr_alloc;
static page_directory_t kernel_page_directory[PDE_CNT] __attribute__((aligned(MEM_PAGE_SIZE)));

static void addr_alloc_init(addr_alloc_t *alloc, u8 *bits, u32 start, u32 size, u32 page_size){
    mutex_init(&alloc->mutex);
    alloc->start = start;
    alloc->size = size;
    alloc->page_size = page_size;
    bitmap_init(&alloc->bitmap, bits, alloc->size / page_size, 0);
}

static u32 addr_alloc_page(addr_alloc_t *alloc, int page_count){
    u32 addr = 0;
    mutex_lock(&alloc->mutex);

    int page_index = bitmap_alloc_nbits(&alloc->bitmap, 0, page_count);
    if (page_index >= 0){
        addr = alloc->start + page_index * alloc->page_size;
    }
    mutex_unlock(&alloc->mutex);
    return addr;
}

static void addr_free_page(addr_alloc_t *alloc, u32 addr, int page_count){
    mutex_lock(&alloc->mutex);
    u32 page_index = (addr - alloc->start) / alloc->page_size;
    bitamp_set_bit(&alloc->bitmap, page_index, page_count, 0);
    mutex_unlock(&alloc->mutex);
}

void show_memory_info(boot_info_t *boot_info){
    log_print("Memory region: \n");
    for (int i = 0; i < boot_info->ram_region_count; ++i){
        log_print("[%d]: 0x%x - 0x%x\n", i, boot_info->ram_region_cfg[i].start,
        boot_info->ram_region_cfg[i].sizes);
    }
    log_print("\n");
}

static u32 total_memory_size(boot_info_t *boot_info){
    u32 mem_size = 0;
    for (int i = 0; i < boot_info->ram_region_count; ++i){
        mem_size += boot_info->ram_region_cfg[i].sizes;
    }
    return mem_size;
}

// 根据页目录表和虚拟地址找到pagetable的对应表项，如果没有找到对应的二级页表，即二级页表不存在，则可选择创建二级页表
page_table_t *find_pte(page_directory_t *page_dir, u32 vaddr, int alloc){
    page_table_t *page_table;
    page_directory_t *pde = page_dir + pde_index(vaddr);
    if(pde->present){
        page_table = (page_table_t *)pde_paddr(pde);
    }else{
        if(alloc == 0){
            return (page_table_t *)0;
        }
        u32 page_paddr = addr_alloc_page(&paddr_alloc, 1);
        if(page_paddr == 0){
            return (page_table_t *)0;
        }

        pde->v = page_paddr | PDE_P | PDE_W | PDE_U;
        page_table = (page_table_t *)page_paddr;
        kernel_memset(page_table, 0, MEM_PAGE_SIZE);
    }
    return page_table + pte_index(vaddr);
}

// 建立映射关系 页表-->物理地址
int memory_create_map(page_directory_t *page_dir, u32 vaddr, u32 paddr, int count, u32 perm){
    for (int i = 0; i < count; ++i){
        // log_print("create map: v-0x%x, p-0x%x, perm:0x%x", vaddr, paddr, perm);
        page_table_t *pte = find_pte(page_dir, vaddr, 1);
        if(pte == (page_table_t *)0){
            // log_print("create pte failed, pte==0");
            return -1;
        }
        // log_print("pte addr:0x%x", (u32)pte);

        ASSERT(pte->present == 0);
        pte->v = paddr | perm | PTE_P;

        vaddr += MEM_PAGE_SIZE;
        paddr += MEM_PAGE_SIZE;
    }
    return 0;
}

//  创建内核的页表，映射到物理地址
void create_kernel_table(void){
    extern u8 start_text[], end_text[], start_data[], kernel_base[];
    static memory_map_t kernel_map[] = {
        {kernel_base, start_text, kernel_base, PTE_W},
        {start_text, end_text, start_text,   0},
        {start_data, (void *)(MEM_EBDA_START - 1), start_data, PTE_W},
        {(void *)CONSOLE_DISP_ADDR, (void *)CONSOLE_DISP_END, (void *)CONSOLE_DISP_ADDR, PTE_W},
        {(void *)MEM_EXT_START, (void *)MEM_EXT_END, (void *)MEM_EXT_START, PTE_W},
    };

    for (int i = 0; i < sizeof(kernel_map) / sizeof(memory_map_t); ++i){
        memory_map_t *map = kernel_map + i;
        u32 vstart = down2((u32)map->vstart, MEM_PAGE_SIZE);
        u32 vend = up2((u32)map->vend, MEM_PAGE_SIZE);
        // u32 paddr = down2((u32)map->pstart, MEM_PAGE_SIZE);
        int page_count = (vend - vstart) / MEM_PAGE_SIZE;

        memory_create_map(kernel_page_directory, vstart, (u32)map->pstart, page_count, map->perm);
    }
}

u32 memory_create_uvm(void){
    page_directory_t *page_dir = (page_directory_t *)addr_alloc_page(&paddr_alloc, 1);
    if (page_dir == 0){
        return 0;
    }
    kernel_memset((void *)page_dir, 0, MEM_PAGE_SIZE);
    u32 user_pde_start = pde_index(MEMORY_TASK_BASE);
    for (int i = 0; i < user_pde_start; i++)
    {
        page_dir[i].v = kernel_page_directory[i].v;
    }

    return (u32)page_dir;
    
}

void memory_init(boot_info_t *boot_info){
    extern u8 *mem_free_start;
    u8 *mem_free_temp = (u8 *)&mem_free_start;
    log_print("Memory init......\n");

    show_memory_info(boot_info);

    u32 mem_up1MB_free = total_memory_size(boot_info) - MEM_EXT_START;    mem_up1MB_free = down2(mem_up1MB_free, MEM_PAGE_SIZE);
    log_print("Free memory: 0x%x, size: 0x%x\n", MEM_EXT_START, mem_up1MB_free);

    addr_alloc_init(&paddr_alloc, mem_free_temp, MEM_EXT_START, 
    mem_up1MB_free, MEM_PAGE_SIZE);
    mem_free_temp += bitmap_bytes_count(paddr_alloc.size / MEM_PAGE_SIZE);

    ASSERT(mem_free_temp < (u8 *)MEM_EBDA_START);

    create_kernel_table();
    mmu_set_page_dir((u32)kernel_page_directory);
}

// 这个地方没有完善
int memory_alloc_for_page_dir(u32 page_dir, u32 vaddr, u32 size, u32 perm){
    u32 curr_vaddr = vaddr;
    int page_count = up2(size, MEM_PAGE_SIZE) / MEM_PAGE_SIZE;

    for (int i = 0; i < page_count; ++i){
        u32 paddr = addr_alloc_page(&paddr_alloc, 1);
        if (paddr == 0){
            log_print("mem alloc failed. no memory");
            return 0;
        }

        int err = memory_create_map((page_directory_t *)page_dir, curr_vaddr, paddr, 1, perm);
        // 失败的话应该释放掉之前的内存， 这里简单起见就不做处理
        if (err < 0){
            log_print("create memory failed. err = %d", err);
            return 0;
        }
        curr_vaddr += MEM_PAGE_SIZE;
    }
}

int memory_alloc_page_for(u32 addr, u32 size, u32 perm){
    return memory_alloc_for_page_dir(task_get_current_task()->tss.cr3, addr, size, perm);
}

u32 memory_alloc_page(void){
    u32 addr = addr_alloc_page(&paddr_alloc, 1);
    return addr;
}

static page_directory_t *curr_page_dir(void){
    return (page_directory_t *)(task_get_current_task()->tss.cr3);
}

void memory_free_page(u32 addr){
    if (addr < MEMORY_TASK_BASE) {
        addr_free_page(&paddr_alloc, addr, 1);
    }else{
        page_table_t *pte = find_pte(curr_page_dir(), addr, 0);
        ASSERT(pte != (page_table_t *)0 && pte->present);

        addr_free_page(&paddr_alloc, pte_paddr(pte), 1);
        pte->v = 0;

    }
}

void memory_destory_uvm(u32 page_dir){
    u32 user_pde_start = pde_index(MEMORY_TASK_BASE);
    page_directory_t *pde = (page_directory_t *)page_dir + user_pde_start;

    for (int i = user_pde_start; i < PDE_CNT; i++, pde++){
        if (!pde->present){
            continue;
        }

        page_table_t *pte = (page_table_t *)pde_paddr(pde);

        for (int j = 0; j < PTE_CNT; j++, pte++){
            if (!pte->present){
                continue;
            }

            addr_free_page(&paddr_alloc, pte_paddr(pte), 1);
        }
        addr_free_page(&paddr_alloc, (u32)pde_paddr(pde), 1);
    }
}


u32 memory_copy_uvm(u32 page_dir){
    u32 to_page_dir = memory_create_uvm();

    if (to_page_dir == 0){
        goto copy_uvm_failed;
    }

    u32 user_pde_start = pde_index(MEMORY_TASK_BASE);

    page_directory_t *pde = (page_directory_t *)page_dir + user_pde_start;

    for (int i = user_pde_start; i < PDE_CNT; i++, pde++){
        if (!pde->present){
            continue;
        }

        page_table_t *pte = (page_table_t *)pde_paddr(pde);
        
        for (int j = 0; j < PTE_CNT; j++, pte++){
            if (!pte->present){
                continue;
            }

            u32 page = addr_alloc_page(&paddr_alloc, 1);

            if (page == 0){
                goto copy_uvm_failed;
            }

            u32 vaddr = (i << 22) | (j << 12);

            int err = memory_create_map((page_directory_t *)to_page_dir, vaddr, page, 1, get_pte_perm(pte));
            if (err < 0){
                goto copy_uvm_failed;
            }

            kernel_memcpy((void *)page, (void *)vaddr, MEM_PAGE_SIZE);
        }
    }

    return to_page_dir;


copy_uvm_failed:
    if (to_page_dir){
        memory_destory_uvm(to_page_dir);
    }
    return 0;
}

u32 memory_get_paddr(u32 page_dir, u32 vaddr){
    page_table_t *pte = find_pte((page_directory_t *)page_dir, vaddr, 0);
    if (!pte){
        return 0;
    }

    return pte_paddr(pte) + (vaddr & (MEM_PAGE_SIZE - 1));
}

int memory_copy_uvm_data(u32 to, u32 page_dir, u32 from, u32 size){
    while(size > 0){
        u32 to_paddr = memory_get_paddr(page_dir, to);
        if (to_paddr == 0){
            return -1;
        }
        u32 offset = to_paddr & (MEM_PAGE_SIZE - 1);
        u32 curr_size = MEM_PAGE_SIZE - offset;
        if (curr_size > size){
            curr_size = size;
        }

        kernel_memcpy((void *)to_paddr, (void *)from, curr_size);

        size -= curr_size;
        to += curr_size;
        from += curr_size;
    }
}


void *sys_sbrk(int incr){
    task_t *task = task_get_current_task();
    char *pre_heap_end = (char *)task->heap_end;

    int pre_incr = incr;

    ASSERT(incr >= 0);

    if (incr == 0){
        log_print("sbrk(0), end = 0x%x", task->heap_end);
        return (void *)task->heap_end;
    }

    u32 start = task->heap_end;
    u32 end = start + incr;

    int start_offset = start % MEM_PAGE_SIZE;
    if (start_offset){
        if (start_offset + incr <= MEM_PAGE_SIZE){
            task->heap_end = end;
            return pre_heap_end;
        }else{
            u32 curr_size = MEM_PAGE_SIZE - start_offset;
            start += curr_size;
            incr -= curr_size;
        }
    }

    if (incr){
        u32 curr_size = end - start;
        int err = memory_alloc_page_for(start, curr_size, PTE_P | PTE_U | PTE_W);
        if (err < 0){
            log_print("sbrk: alloc failed");
            return (char *)-1;
        }
    }

    // log_print("sbrk(%d): end = 0x%x\n", pre_incr, end);

    task->heap_end = end;

    return (char *)pre_heap_end;
}