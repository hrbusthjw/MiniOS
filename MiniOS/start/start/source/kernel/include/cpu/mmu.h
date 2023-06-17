#ifndef MMU_H
#define MMU_H
#include "common/types.h"
#include "common/CPUInlineFun.h"

#define PDE_CNT     1024
#define PTE_CNT     1024

#define PTE_P               (1 << 0)
#define PDE_P               (1 << 0)
#define PDE_W               (1 << 1)
#define PTE_W               (1 << 1)
#define PDE_U               (1 << 2)
#define PTE_U               (1 << 2)

//  页目录表
typedef union _page_directory_t{
    u32 v;
    struct 
    {
        u32 present : 1;
        u32 write_enable : 1;
        u32 user_mode_acc : 1;
        u32 write_through : 1;
        u32 cache_disable : 1;
        u32 accessed : 1;
        u32 :1;
        u32 ps : 1;
        u32 : 4;
        u32 phy_pt_addr : 20;       //pagetable addr
    };
    
}page_directory_t;


//  页表
typedef union _page_table_t{
    u32 v;
    struct 
    {
        u32 present : 1;
        u32 write_enable : 1;
        u32 user_mode_acc : 1;
        u32 write_through : 1;
        u32 cache_disable : 1;
        u32 accessed : 1;
        u32 dirty:1;
        u32 pat : 1;
        u32 global : 1;
        u32 : 3;
        u32 phy_pt_addr : 20;       //pagetable addr
    };
    
}page_table_t;

static inline void mmu_set_page_dir(u32 paddr){
    writeCR3(paddr);
}

//  得到pagediretcory中的地址
static inline u32 pde_index(u32 vaddr){
    return (vaddr >> 22);
}
//  得到pagetable中的地址
static inline u32 pte_index(u32 vaddr){
    return (vaddr >> 12) & 0x3ff;
}

static inline u32 pde_paddr(page_directory_t *page_dir){
    return (page_dir->phy_pt_addr << 12);
}
static inline u32 pte_paddr(page_table_t *page_table){
    return (page_table->phy_pt_addr << 12);
}

static inline u32 get_pte_perm(page_table_t *pte){
    return (pte->v & 0x1FF);
}

#endif