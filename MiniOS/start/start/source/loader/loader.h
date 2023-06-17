#ifndef LOADER_H
#define LOADER_H

#include "common/boot_info.h"
#include "common/types.h"
#include "common/CPUInlineFun.h"

void protectedModeEntry(void);

//用于检测内存容量
typedef struct SMAP_entry{
    u32 baseL;//64位 base
    u32 baseH;
    u32 lengthL;//64位length
    u32 lengthH;
    u32 type;//entry Type， 值为1表面为我们可用的空间
    u32 ACPI;//extended， bit0=1表明此条目应当被忽略
}__attribute__((packed)) SMAP_entry_t;

extern boot_info_t boot_info;

#endif