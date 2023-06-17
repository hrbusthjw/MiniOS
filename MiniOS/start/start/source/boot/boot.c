__asm__(".code16gcc");

#include "boot.h"

#define LOADE_START_ADDR        0x8000

void boot_entry(void) {
    ((void(*)(void))LOADE_START_ADDR)();
} 

