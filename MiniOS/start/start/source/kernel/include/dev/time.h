#ifndef TIME_H
#define TIME_h
#include "common/types.h"
#include "common/CPUInlineFun.h"
#include "osConfig.h"
#include "cpu/irq.h"
#include "core/task.h"

#define PIT_OSC_FQER 1193182
#define PIT_CHANNEL0_DATA_PORT 0X40
#define PIT_COMMAND_MODE_PORT 0X43

#define PIT_CHANNEL (0 << 6)   // 选择计数器0
#define PIT_LOAD_LOHI (3 << 4) // 先读后写
#define PIT_MODE3 (3 << 1)     // 工作在方式3

void time_init(void);

void exception_handler_time(void);

#endif