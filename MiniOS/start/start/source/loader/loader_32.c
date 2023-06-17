#include "loader.h"
#include "common/elf.h"

#define CR4_PSE     (1 << 4)
#define CR0_PG      (1 << 31)
#define PDE_P       (1 << 0)
#define PDE_W       (1 << 1)
#define PDE_PS      (1 << 7)

// 将磁盘加载到内存
static void readDisk(int sector, int sectorNumber, u8 *buffer)
{
    outb(0x1F6, (u8)(0xE0));

    outb(0x1F2, (u8)(sectorNumber >> 8));
    outb(0x1F3, (u8)(sector >> 24));
    outb(0x1F4, (u8)(0));
    outb(0x1F5, (u8)(0));

    outb(0x1F2, (u8)(sectorNumber));
    outb(0x1F3, (u8)(sector));
    outb(0x1F4, (u8)(sector >> 8));
    outb(0x1F5, (u8)(sector >> 16));

    outb(0x1F7, (u8)(0x24));

    u16 *dataBuf = (u16 *)buffer;
    while (sectorNumber-- > 0)
    {
        while ((inb(0x1F7) & 0x88) != 0x8)
        {
        }
        for (int i = 0; i < SECTOR_SIZE / 2; i++)
        {
            *dataBuf++ = inw(0x1F0);
        }
    }
}

// 解析Kernel的elf文件，加载到内存指定位置
static u32 reload_elf_file(u8 *file_buffer)
{
    Elf32_Ehdr *elf_hdr = (Elf32_Ehdr *)file_buffer;
    if ((elf_hdr->e_ident[0] != 0x7f) || (elf_hdr->e_ident[1] != 'E') ||
        (elf_hdr->e_ident[2] != 'L') || (elf_hdr->e_ident[3] != 'F'))
    {
        return 0;
    }

    for (int i = 0; i < elf_hdr->e_phnum; i++)
    {
        Elf32_Phdr *phdr = (Elf32_Phdr *)(file_buffer + elf_hdr->e_phoff) + i;
        if (phdr->p_type != PT_LOAD)
        {
            continue;
        }

        u8 *src = file_buffer + phdr->p_offset;
        u8 *dest = (u8 *)phdr->p_paddr;
        for (int j = 0; j < phdr->p_filesz; j++)
        {
            *dest++ = *src++;
        }
        dest = (u8 *)phdr->p_paddr + phdr->p_filesz;
        for (int j = 0; j < phdr->p_memsz - phdr->p_filesz; j++)
        {
            *dest++ = 0;
        }
    }

    return elf_hdr->e_entry;
}

void enable_page_mode(){
    static u32 page_dir[1024] __attribute__((aligned(4096))) = {
        [0] = PDE_P | PDE_PS | PDE_W | 0,
    };

    u32 cr4 = readCR4();
    writeCR4(cr4 | CR4_PSE);
    cr4 = readCR4();

    writeCR3((u32)page_dir);
    u32 cr3 = readCR3();

    writeCR0(readCR0() | CR0_PG);
    u32 cr0 = readCR0();
}

static void die(int code)
{
    while (1)
        ;
}

// 跳转到Kernel
void loaderKernel(void)
{

    readDisk(100, 500, (u8 *)SYS_KERNEL_LOAD_ADDR);

    u32 kernelEntry = reload_elf_file((u8 *)SYS_KERNEL_LOAD_ADDR);

    if (kernelEntry == 0)
    {
        die(-1);
    }

    enable_page_mode();

    ((void (*)(boot_info_t *))kernelEntry)(&boot_info);
}