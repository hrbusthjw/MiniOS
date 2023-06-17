#include "core/syscall.h"
#include "core/task.h"
#include "tools/log.h"
#include "fs/fs.h"
#include "core/memory.h"

typedef int (*syscall_handler_t)(u32 arg0, u32 arg1, u32 arg2, u32 arg3);

void sys_print_msg(char *fmt, int arg){
    log_print(fmt, arg);
}

static const syscall_handler_t sys_table[] = {
    [SYS_sleep] = (syscall_handler_t)sys_sleep,
    [SYS_getpid] = (syscall_handler_t)sys_getpid,
    [SYS_fork] = (syscall_handler_t)sys_fork,
    [SYS_execve] = (syscall_handler_t)sys_execve,
    [SYS_yield] = (syscall_handler_t)sys_yield,
    [SYS_open] = (syscall_handler_t)sys_open,
    [SYS_read] = (syscall_handler_t)sys_read,
    [SYS_write] = (syscall_handler_t)sys_write,
    [SYS_close] = (syscall_handler_t)sys_close,
    [SYS_lseek] = (syscall_handler_t)sys_lseek,
    [SYS_isatty] = (syscall_handler_t)sys_isatty,
    [SYS_fstat] = (syscall_handler_t)sys_fstat,
    [SYS_sbrk] = (syscall_handler_t)sys_sbrk,
    [SYS_dup] = (syscall_handler_t)sys_dup,
    [SYS_exit] = (syscall_handler_t)sys_exit,
    [SYS_wait] = (syscall_handler_t)sys_wait,
    [SYS_opendir] = (syscall_handler_t)sys_opendir,
	[SYS_readdir] = (syscall_handler_t)sys_readdir,
	[SYS_closedir] = (syscall_handler_t)sys_closedir,
    [SYS_ioctl] = (syscall_handler_t)sys_ioctl,
    [SYS_unlink] = (syscall_handler_t)sys_unlink,
    [SYS_printmsg] = (syscall_handler_t)sys_print_msg,

};

void do_handler_syscall(syscall_frame_t *frame){
    if (frame->func_id < sizeof(sys_table) / sizeof(sys_table[0])){
        syscall_handler_t handler = sys_table[frame->func_id];
        if (handler){
            int ret = handler(frame->arg0, frame->arg1, frame->arg2, frame->arg3);
            frame->eax = ret;
            return;
        }
    }

    task_t *task = task_get_current_task();
    log_print("task: %s, Unknown syscall : %d", task->name, frame->func_id);
    frame->eax = -1;
}