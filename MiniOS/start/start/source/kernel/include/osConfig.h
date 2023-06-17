#ifndef OSCONFIG_H
#define OSCONFIG_H

#define GDT_TABLE_SIZE      256
#define KERNEL_SELECTOR_CS  8
#define KERNEL_SELECTOR_DS  16

#define SELECTOR_SYSCALL    (3 * 8)

#define KERNEL_STACK_SIZE (8*1024)

#define OS_TICKS_MS     10

#define OS_VISION       "1.0"

#define IDLE_STACK_SIZE     1024

#define TASK_NR             128

#define ROOT_DEV            DEV_DISK, 0XB1

#endif