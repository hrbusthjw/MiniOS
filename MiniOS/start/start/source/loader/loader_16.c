__asm__(".code16gcc");

#include "loader.h"

boot_info_t boot_info;

//gdt表
u16 gdtTable[][4] = {
    {0, 0, 0, 0},
    {0xFFFF, 0x0000, 0x9A00, 0x00CF},
    {0xFFFF, 0x0000, 0x9200, 0x00CF},
};

//在屏幕上显示信息
static void showMessage(const char *msg){
    char c;
    while((c = *msg++) != '\0'){
        __asm__ __volatile__(
            "mov $0xe, %%ah;mov %[ch], %%al;int $0x10"::[ch]"r"(c)
        );
    }
}

//检测内存容量
static void detectMemory(void){
    u32 contID = 0;
    int entries = 0, signature, bytes;
    SMAP_entry_t smap_buffer;
    showMessage("\r\nTrying to detect memory......\n");
    boot_info.ram_region_count = 0;
    for (int i = 0; i < BOOT_RAM_REGION_MAX; i++){
        SMAP_entry_t *buffer = &smap_buffer;

        __asm__ __volatile__("int $0x15"
        : "=a"(signature), "=c"(bytes), "=b"(contID)
        : "a"(0xE820), "b"(contID), "c"(24), "d"(0x534D4150), "D"(buffer));

        if (signature != 0x534D4150){
            showMessage("\r\nFailed to detect memory!\r\n");
            return;
        }

        if (bytes > 20 && (buffer->ACPI & 0x0001) == 0){
            continue;
        }

        if (buffer->type == 1){
            boot_info.ram_region_cfg[boot_info.ram_region_count].start = buffer->baseL;
            boot_info.ram_region_cfg[boot_info.ram_region_count++].sizes = buffer->lengthL;
        }

        if (contID == 0) break;
    }
    showMessage("\r\nSuccess to detect memory!\r\n");
}

//进入保护模式
static void enterProtectedMode(){
    showMessage("\r\nEntrying to protected mode......\r\n");
    //关中断
    cli();

    //打开A20地址线
    u8 value = inb(0x92);
    outb(0x92, value | 0x2);

    //加载GDT表
    lgdt((u32)gdtTable, sizeof(gdtTable));

    //将cr0寄存器最低位置1
    u32 cr0 = readCR0();
    writeCR0(cr0 | (1 << 0));

    //远跳转,清空流水线
    farJump(8, (u32)protectedModeEntry);
}

void loaderEntry(void){
    showMessage("\n           ......loading......\n");
    detectMemory();
    enterProtectedMode();
    for(;;){}
}