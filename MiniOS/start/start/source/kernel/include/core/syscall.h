#ifndef SYSCALL_H
#define SYSCALL_H

#include "common/types.h"


#define SYS_sleep               0
#define SYS_getpid              1
#define SYS_fork                2
#define SYS_execve              3
#define SYS_yield               4
#define SYS_open                5
#define SYS_read                6
#define SYS_write               7
#define SYS_close               8
#define SYS_lseek               9

// newlib库需要
#define SYS_isatty              10
#define SYS_fstat               11
#define SYS_sbrk                12

#define SYS_dup                 13

#define SYS_exit                14
#define SYS_wait                15

#define SYS_opendir             16
#define SYS_readdir             17
#define SYS_closedir            18

#define SYS_ioctl               19
#define SYS_unlink              20





#define SYS_printmsg            100

#define SYSCALL_PARAM_COUNT     5

typedef struct _syscall_frame_t{
    int eflags;
    int gs, fs, es, ds;
    u32 edi, esi, ebp, dummy, ebx, edx, ecx, eax;
    int eip, cs;
    int func_id, arg0, arg1, arg2, arg3;
    int esp, ss;
}syscall_frame_t;

void exeception_handler_syscall (void);

#endif