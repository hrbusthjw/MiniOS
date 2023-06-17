#include "applib/lib_syscall.h"
#include <stdlib.h>
#include <string.h>


static inline int syscall(syscall_args_t *args){
    u32 addr[] = {0, SELECTOR_SYSCALL | 0};
    int ret;

    __asm__ __volatile__(
        "push %[arg3]\n\t"
        "push %[arg2]\n\t"
        "push %[arg1]\n\t"
        "push %[arg0]\n\t"
        "push %[id]\n\t"
        "lcall *(%[a])"
        :"=a"(ret)
        : 
        [arg3]"r"(args->arg3),
        [arg2]"r"(args->arg2),
        [arg1]"r"(args->arg1),
        [arg0]"r"(args->arg0),
        [id]"r"(args->id),
        [a] "r"(addr)
        );
        return ret;
}

void msleep(int ms){
    if (ms <= 0){
        return;
    }

    syscall_args_t args;
    args.id = SYS_sleep;
    args.arg0 = ms;

    syscall(&args);
}

int getpid(void){
    syscall_args_t args;
    args.id = SYS_getpid;

    return syscall(&args);
}

void print_msg(const char *fmt, int arg){
    syscall_args_t args;
    args.id = SYS_printmsg;
    args.arg0 = (int)fmt;
    args.arg1 = arg;

    syscall(&args);
}

int fork(void){
    syscall_args_t args;
    args.id = SYS_fork;

    return syscall(&args);
}

int execve(const char *name, char * const *argv, char * const *env){
    syscall_args_t args;
    args.id = SYS_execve;
    args.arg0 = (int)name;
    args.arg1 = (int)argv;
    args.arg2 = (int)env;

    return syscall(&args);
}

int yield(void){
    syscall_args_t args;
    args.id = SYS_yield;

    return syscall(&args);
}

int open(const char *name, int flags, ...){
    syscall_args_t args;
    args.id = SYS_open;
    args.arg0 = (int)name;
    args.arg1 = (int)flags;

    return syscall(&args);
}

int read(int file, char *ptr, int len){
    syscall_args_t args;
    args.id = SYS_read;
    args.arg0 = file;
    args.arg1 = (int)ptr;
    args.arg2 = len;

    return syscall(&args);
}

int write(int file, char *ptr, int len){
    syscall_args_t args;
    args.id = SYS_write;
    args.arg0 = file;
    args.arg1 = (int)ptr;
    args.arg2 = len;

    return syscall(&args);
}

int close(int file){
    syscall_args_t args;
    args.id = SYS_close;
    args.arg0 = file;

    return syscall(&args);
}

int lseek(int file, int ptr, int dir){
    syscall_args_t args;
    args.id = SYS_lseek;
    args.arg0 = file;
    args.arg1 = ptr;
    args.arg2 = dir;

    return syscall(&args);
}

int isatty(int file){
    syscall_args_t args;
    args.id = SYS_isatty;
    args.arg0 = file;

    return syscall(&args);
}

int fstat(int file, struct stat *st){
    syscall_args_t args;
    args.id = SYS_fstat;
    args.arg0 = file;
    args.arg1 = (int)st;

    return syscall(&args);
}

void *sbrk(ptrdiff_t incr){
    syscall_args_t args;
    args.id = SYS_sbrk;
    args.arg0 = (int)incr;

    return (void *)syscall(&args);
}

int dup(int file){
    syscall_args_t args;
    args.id = SYS_dup;
    args.arg0 = file;

    return syscall(&args);
}

void _exit(int status){
    syscall_args_t args;
    args.id = SYS_exit;
    args.arg0 = status;

    syscall(&args);
    while(1);
}

int wait(int *status){
    syscall_args_t args;
    args.id = SYS_wait;
    args.arg0 = (int)status;

    return syscall(&args);
}