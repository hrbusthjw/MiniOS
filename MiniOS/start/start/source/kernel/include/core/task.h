#ifndef TASK_H
#define TASK_H

#include "common/types.h"
#include "tools/list.h"
#include "cpu/cpu.h"
#include "fs/file.h"

#define TASK_NAME_SIZE      32
#define TASK_TIME_SILCE     10

#define TASK_OFILE_NR       128

#define TASK_FLAGS_SYSTEM   (1 << 0)

typedef struct  _task_t
{
    int pid;

    struct _task_t *parent;

    u32 heap_start;
    u32 heap_end;

    file_t *file_table[TASK_OFILE_NR];
    
    // u32 *stack;
    int sleep_ticks;
    int slice_ticks;
    int time_ticks;

    int exit_status;


    // 进程运行状态
    enum {
        CREATE,
        RUNNIG,
        SLEEP,
        READY,
        WATTING,
        ZOMBIE,
    }state;

    char name[TASK_NAME_SIZE];

    // 此处等待重构，不太需要wait_node，徒增难度
    list_node_t run_node;
    list_node_t wait_node;
    list_node_t all_node;

    tss_t tss;
    int tss_sel;// tss选择子
}task_t;

typedef struct _task_manager_t
{
    task_t *curr_task;
    
    list_t ready_list;
    list_t task_list;
    list_t sleep_list;

    task_t first_task;

    task_t idle_task;

    int app_code_sel;
    int app_data_sel;
}task_manager_t;


typedef struct _task_args_t{
    u32 ret_addr;
    u32 argc;
    char **argv;
}task_args_t;

int task_init(task_t *task, const char *name, int flag, u32 entry, u32 esp);

void task_switch_from_to(task_t *from, task_t *to);

// 这个函数不再使用
void simple_switch(u32 **from, u32 *to);

void task_manager_init(void);
void task_first_init(void);
task_t *task_get_first_task(void);

void task_set_ready(task_t *task);
void task_set_block (task_t *task);

int sys_yield(void);
void task_dispatch(void);

task_t *task_get_current_task(void);

void task_time_tick(void);

void task_instrt_to_sleep_list(task_t *task, u32 ticks);
void task_remove_from_sleep_list(task_t *task);

int task_alloc_fd(file_t *file);
void task_remove_fd(int fd);
file_t *task_file(int fd);


void sys_sleep(u32 ms);
int sys_getpid(void);
int sys_fork(void);
int sys_execve(char *name, char **argv, char **env);
void sys_exit(int status);
int sys_wait(int *status);

#endif

