#include "core/task.h"
#include "ipc/mutex.h"
#include "tools/klib.h"
#include "cpu/irq.h"
#include "osConfig.h"
#include "tools/log.h"
#include "common/CPUInlineFun.h"
#include "core/memory.h"
#include "cpu/mmu.h"
#include "core/syscall.h"
#include "common/elf.h"
#include "fs/fs.h"

static task_manager_t task_manager;
static u32 idle_task_stack[IDLE_STACK_SIZE];
static task_t task_table[TASK_NR];
static mutex_t mutex_for_task_table;


static int tss_init(task_t *task, int flag, u32 entry, u32 esp)
{
    int tss_sel = gdt_alloc_desc();
    if (tss_sel <= 0)
    {
        log_print("alloc tss failed\n");
        return -1;
    }

    segment_desc_set(tss_sel, (u32)&task->tss, sizeof(tss_t),
                     SEG_P_PARENT | SEG_DPL0 | SEG_TYPE_TSS);

    kernel_memset(&task->tss, 0, sizeof(tss_t));

    u32 kernel_stack = memory_alloc_page();

    if (kernel_stack == 0){
        goto tss_init_failed;
    }

    int code_sel, data_sel;

    if (flag & TASK_FLAGS_SYSTEM){
        code_sel = KERNEL_SELECTOR_CS;
        data_sel = KERNEL_SELECTOR_DS;
    }else{
        code_sel = task_manager.app_code_sel | SEG_CPL3;
        data_sel = task_manager.app_data_sel | SEG_CPL3;
    }

    task->tss.eip = entry;
    task->tss.esp = esp;
    task->tss.esp0 = kernel_stack + MEM_PAGE_SIZE;
    task->tss.ss = data_sel;
    task->tss.ss0 = KERNEL_SELECTOR_DS;
    task->tss.es = task->tss.ds = task->tss.fs = task->tss.gs = data_sel;
    task->tss.cs = code_sel;
    task->tss.eflags = EFLAGS_IF | EFLAGS_DEFAULT;

    u32 page_dir  = memory_create_uvm();
    if (page_dir == 0){
        goto tss_init_failed;
    }

    task->tss.cr3 = page_dir;

    task->tss_sel = tss_sel;

    return 0;

tss_init_failed:
    gdt_free_sel(tss_sel);
    if (kernel_stack != 0)
    {
        memory_free_page(kernel_stack);
    }
    return -1;
}

int task_init(task_t *task, const char *name, int flag, u32 entry, u32 esp)
{
    ASSERT(task != (task_t *)0);

    int error = tss_init(task, flag, entry, esp);

    if (error < 0){
        log_print("init task failed!\n");
        return error;
    }

    kernel_strncpy(task->name, name, TASK_NAME_SIZE);
    task->time_ticks = TASK_TIME_SILCE;
    task->sleep_ticks = 0;
    task->slice_ticks = task->time_ticks;
    task->heap_start = 0;
    task->heap_end = 0;
    task->parent = (task_t *)0;
    task->state = CREATE;

    task->exit_status = 0;

    list_node_init(&task->all_node);
    list_node_init(&task->run_node);
    list_node_init(&task->wait_node);

    // 初始化task的file结构指针
    kernel_memset(&task->file_table, 0, sizeof(task->file_table));

    irq_state_t state = irq_enter_critical_section();
    task->pid = (u32)task;
    // task_set_ready(task);
    list_insert_tail(&task_manager.task_list, &task->all_node);
    irq_leave_critical_section(state);
    return 0;
}

int task_ready(task_t *task)
{
    irq_state_t state = irq_enter_critical_section();

    task_set_ready(task);

    irq_leave_critical_section(state);
    return 0;
}

void task_uninit(task_t *task){
    if (task->tss_sel){
        gdt_free_sel(task->tss_sel);
    }

    if (task->tss.esp0){
        memory_free_page(task->tss.esp - MEM_PAGE_SIZE);
    }

    if (task->tss.cr3){
        memory_destory_uvm(task->tss.cr3);
    }

    kernel_memset(task, 0, sizeof(task));
}

void task_switch_from_to(task_t *from, task_t *to)
{
    switch_to_tss(to->tss_sel);
    // 软件切换方式：用于理解linux等现代操作系统的任务切换机制
    // simple_switch(&from->stack, to->stack);
}

static void idle_task_entry()
{
    while (1)
        hlt();
}

void task_manager_init()
{
    kernel_memset(task_table, 0, sizeof(task_table));
    mutex_init(&mutex_for_task_table);

    int sel = gdt_alloc_desc();
    segment_desc_set(sel, 0x00000000, 0xffffffff, SEG_P_PARENT | SEG_DPL3 | SEG_S_NORMAL | SEG_TYPE_DATA | SEG_TYPE_RW |
    SEG_D);
    task_manager.app_data_sel = sel;

    sel = gdt_alloc_desc();
    segment_desc_set(sel, 0x00000000, 0xffffffff, SEG_P_PARENT | SEG_DPL3 | SEG_S_NORMAL | SEG_TYPE_CODE | SEG_TYPE_RW |
    SEG_D);
    task_manager.app_code_sel = sel;

    list_init(&task_manager.ready_list);
    list_init(&task_manager.task_list);
    list_init(&task_manager.sleep_list);
    task_manager.curr_task = (task_t *)0;

    task_init(&task_manager.idle_task, "idle task", TASK_FLAGS_SYSTEM, (u32)idle_task_entry, (u32)(idle_task_stack + IDLE_STACK_SIZE));
    task_ready(&task_manager.idle_task);
}

void task_first_init(void)
{
    void first_task_entry(void);
    extern u8 s_first_task[], e_first_task[];

    u32 copy_size = (u32)(e_first_task - s_first_task);
    u32 alloc_size = 10 * MEM_PAGE_SIZE;
    ASSERT(copy_size < alloc_size);


    u32 first_start = (u32)first_task_entry;

    task_init(&task_manager.first_task, "first task", 0, first_start, first_start + alloc_size);
    task_manager.first_task.heap_start = (u32)e_first_task;
    task_manager.first_task.heap_end = (u32)e_first_task;

    write_tr(task_manager.first_task.tss_sel);
    task_manager.curr_task = &task_manager.first_task;

    mmu_set_page_dir(task_manager.first_task.tss.cr3);

    memory_alloc_page_for(first_start, alloc_size, PTE_P | PTE_W | PTE_U);
    kernel_memcpy((void *)first_start, s_first_task, copy_size);

    task_ready(&task_manager.first_task);
}

// 似乎没用到
task_t *task_get_first_task(void)
{
    return &task_manager.first_task;
}

void task_set_ready(task_t *task)
{
    if (task != &task_manager.idle_task)    // 不能将空闲进程也置为就绪态
    {
        list_insert_tail(&task_manager.ready_list, &task->run_node);
        task->state = READY;
    }
}

void task_set_block(task_t *task)
{
    if (task != &task_manager.idle_task)    // 空闲进程不可能出现在就绪队列中，因此不能移出
        list_delete(&task_manager.ready_list, &task->run_node);
}

task_t *task_get_next_run(void)
{
    if (list_count(&task_manager.ready_list) == 0)  // 如果当前就绪队列中已经没有任务了，需要运行空闲任务
    {
        return &task_manager.idle_task;
    }
    // 否则就返回就绪队列中的第一个任务
    list_node_t *task_node = list_get_first(&task_manager.ready_list);
    return list_node_parent(task_node, task_t, run_node);
}

task_t *task_get_current_task(void)
{
    return task_manager.curr_task;
}

// 当在临界区中切换到别的任务时，eflags会转换为下一个任务的eflags， 因此中断的开关也会转换为下一个任务的中断开关
// 除了第一次执行dispatch的时候，切换到下一个任务不会处于dispatch内，其余时刻执行的任务切换前后都是在dispatch内
// 因此当切换后，中断在一定时间内还是关闭状态，但会很快恢复
// 采用临界区的方式在一定程度上影响性能，比如可能会导致一定次数的定时中断不能执行
// 此处可以考虑使用互斥锁替换，临界区影响性能
void task_dispatch(void)
{
    irq_state_t state = irq_enter_critical_section();
    task_t *to = task_get_next_run();
    if (to != task_manager.curr_task)
    {
        task_t *from = task_get_current_task();
        task_manager.curr_task = to;
        to->state = RUNNIG;
        task_switch_from_to(from, to);
    }
    irq_leave_critical_section(state);
}

// 此方法让进程主动放弃cpu
int sys_yield(void)
{
    irq_state_t state = irq_enter_critical_section();
    if (list_count(&task_manager.ready_list) > 1)
    {
        task_t *curr_task = task_get_current_task();

        task_set_block(curr_task);
        task_set_ready(curr_task);

        task_dispatch();
    }
    irq_leave_critical_section(state);
    return 0;
}

void sys_exit(int status){
    task_t *curr_task = task_get_current_task();

    for (int fd = 0; fd < TASK_OFILE_NR; fd++){
        file_t *file = curr_task->file_table[fd];
        if (file) {
            sys_close(fd);
            curr_task->file_table[fd] = (file_t *)0;
        }
    }

    int move_child = 0;

    mutex_lock(&mutex_for_task_table);
    for (int i = 0; i < TASK_OFILE_NR; ++i){
        task_t *task = task_table + i;
        if(task->parent == curr_task){
            task->parent = &task_manager.first_task;
            if (task->state == ZOMBIE){
                move_child = 1;
            }
        }
    }

    mutex_unlock(&mutex_for_task_table);

    irq_state_t st = irq_enter_critical_section();

    task_t *parent = curr_task->parent;

    if (move_child && (parent != &task_manager.first_task)){
        if (task_manager.first_task.state == WATTING){
            task_set_ready(&task_manager.first_task);
        }
    }


    if (parent->state == WATTING){
        task_set_ready(parent);
    }

    curr_task->state = ZOMBIE;
    curr_task->exit_status = status;
    task_set_block(curr_task);
    irq_leave_critical_section(st);
    task_dispatch();
}

int sys_wait(int *status){
    task_t *curr_task = task_get_current_task();
    for (;;){
        mutex_lock(&mutex_for_task_table);

        for (int i = 0; i < TASK_NR; i++){
            task_t *task = task_table + i;
            if (task->parent != curr_task){
                continue;
            }

            if (task->state == ZOMBIE){
                int pid = task->pid;
                *status = task->exit_status;

                memory_destory_uvm(task->tss.cr3);
                memory_free_page(task->tss.esp0 - MEM_PAGE_SIZE);
                kernel_memset(task, 0, sizeof(task_t));

                mutex_unlock(&mutex_for_task_table);
                
                return pid;
            }
        }

        mutex_unlock(&mutex_for_task_table);

        irq_state_t st = irq_enter_critical_section();

        task_set_block(curr_task);
        irq_leave_critical_section(st);
        curr_task->state = WATTING;
        task_dispatch();
    }
    return 0;
}

void task_time_tick(void)
{
    irq_state_t state = irq_enter_critical_section();
    // if (list_count(&task_manager.ready_list) > 1)   // 当此时只有一个进程或者只有空闲任务的时候，不进行任何操作
    {
        // 否则检查任务的时间片，时间片到了就切换进程
        task_t *curr_task = task_get_current_task();

        if (--curr_task->slice_ticks == 0)
        {
            curr_task->slice_ticks = curr_task->time_ticks;

            task_set_block(curr_task);
            task_set_ready(curr_task);

            task_dispatch();
        }
    }

    // 一次时间片中断就检查一次睡眠队列，如果有时间到的就将其唤醒加入到就绪队列
    list_node_t *node = list_get_first(&task_manager.sleep_list);
    while (node)
    {
        list_node_t *next = node->next;
        task_t *task = list_node_parent(node, task_t, run_node);

        if (--task->sleep_ticks == 0)
        {
            task_remove_from_sleep_list(task);
            task_set_ready(task);
        }
        node = next;
    }
    // task_dispatch();    // ？ 立即切换 此处有问题
    irq_leave_critical_section(state);
}

void task_instrt_to_sleep_list(task_t *task, u32 ticks)
{
    if (!ticks)
    {
        return;
    }

    task->sleep_ticks = ticks;
    task->state = SLEEP;
    list_insert_tail(&task_manager.sleep_list, &task->run_node);
}

void task_remove_from_sleep_list(task_t *task)
{
    list_delete(&task_manager.sleep_list, &task->run_node);
}

static task_t* alloc_task(void){
    task_t *task = (task_t *)0;

    mutex_lock(&mutex_for_task_table);
    for (int i = 0; i < TASK_NR; ++i){
        task_t *curr = task_table + i;
        if (curr->name[0] == '\0'){
            task = curr;
            break;
        }
    }
    mutex_unlock(&mutex_for_task_table);

    return task;
}

static void free_task(task_t *task){
    mutex_lock(&mutex_for_task_table);
    task->name[0] = '\0';
    mutex_unlock(&mutex_for_task_table);
}

void sys_sleep(u32 ms)
{
    irq_state_t state = irq_enter_critical_section();

    task_set_block(task_manager.curr_task);

    task_instrt_to_sleep_list(task_manager.curr_task, (ms + (OS_TICKS_MS - 1)) / OS_TICKS_MS);

    task_dispatch();

    irq_leave_critical_section(state);
}

int sys_getpid(void){
    task_t *task = task_get_current_task();
    return task->pid;
}

static void copy_opened_files(task_t *child_task){
    task_t *parent = task_get_current_task();
    for (int i = 0; i < TASK_OFILE_NR; ++i){
        file_t *file = parent->file_table[i];
        if (file){
            file_inc_ref(file);
            child_task->file_table[i] = file;
        }
    }
}

int sys_fork(void){
    task_t *parent_task = task_get_current_task();

    task_t *child_task = alloc_task();
    if (child_task == (task_t *)0){
        goto fork_failed;
    }

    syscall_frame_t *frame = (syscall_frame_t *)(parent_task->tss.esp0 - sizeof(syscall_frame_t));


    int err = task_init(child_task, parent_task->name, 0, frame->eip, frame->esp + sizeof(u32) * SYSCALL_PARAM_COUNT);
    if (err < 0){
        goto fork_failed;
    }

    tss_t *tss = &child_task->tss;
    tss->eax = 0;   //在子进程中返回pid为0
    tss->ebx = frame->ebx;
    tss->ecx = frame->ecx;
    tss->edx = frame->edx;
    tss->esi = frame->esi;
    tss->edi = frame->edi;
    tss->ebp = frame->ebp;

    tss->cs = frame->cs;
    tss->ds = frame->ds;
    tss->es = frame->es;
    tss->fs = frame->fs;
    tss->gs = frame->gs;
    tss->eflags = frame->eflags;

    child_task->parent = parent_task;

    copy_opened_files(child_task);

    if ((tss->cr3 = memory_copy_uvm(parent_task->tss.cr3)) < 0){
        goto fork_failed;
    }

    // tss->cr3 = parent_task->tss.cr3;
    task_ready(child_task);

    return child_task->pid;

fork_failed:
    if (child_task){
        task_uninit(child_task);
        free_task(child_task);
    }
    return -1;
}

static int load_phdr(int file, Elf32_Phdr *phdr, u32 page_dir){
    ASSERT((phdr->p_vaddr & (MEM_PAGE_SIZE - 1)) == 0);

    int err = memory_alloc_for_page_dir(page_dir, phdr->p_vaddr, phdr->p_memsz, PTE_P | PTE_U | PTE_W);
    if (err < 0){
        log_print("no memory\n");
        return -1;
    }

    if (sys_lseek(file, phdr->p_offset, 0) < 0){
        log_print("read failed\n");
        return -1;
    }

    u32 vaddr = phdr->p_vaddr;
    u32 size = phdr->p_filesz;

    while(size > 0){
        int curr_size = (size > MEM_PAGE_SIZE) ? MEM_PAGE_SIZE : size;
        u32 paddr = memory_get_paddr(page_dir, vaddr);

        if (sys_read(file, (char *)paddr, curr_size) < curr_size){
            log_print("read failed\n");
            return -1;
        }

        size -= curr_size;
        vaddr += curr_size;
    }
}

static u32 load_elf_file (task_t *task, const char *name, u32 page_dir) {
    Elf32_Ehdr elf_hdr;
    Elf32_Phdr elf_phdr;

    // 以只读方式打开
    int file = sys_open(name, 0);   // todo: flags暂时用0替代
    if (file < 0) {
        log_print("open file failed.%s\n", name);
        goto load_failed;
    }

    // 先读取文件头
    int cnt = sys_read(file, (char *)&elf_hdr, sizeof(Elf32_Ehdr));
    if (cnt < sizeof(Elf32_Ehdr)) {
        log_print("elf hdr too small. size=%d\n", cnt);
        goto load_failed;
    }

    // 做点必要性的检查。当然可以再做其它检查
    if ((elf_hdr.e_ident[0] != 0x7f) || (elf_hdr.e_ident[1] != 'E')
        || (elf_hdr.e_ident[2] != 'L') || (elf_hdr.e_ident[3] != 'F')) {
        log_print("check elf indent failed.\n");
        goto load_failed;
    }

    // 必须是可执行文件和针对386处理器的类型，且有入口
    if ((elf_hdr.e_type != ET_EXEC) || (elf_hdr.e_machine != ET_386) || (elf_hdr.e_entry == 0)) {
        log_print("check elf type or entry failed.\n");
        goto load_failed;
    }

    // 必须有程序头部
    if ((elf_hdr.e_phentsize == 0) || (elf_hdr.e_phoff == 0)) {
        log_print("none programe header\n");
        goto load_failed;
    }

    // 然后从中加载程序头，将内容拷贝到相应的位置
    u32 e_phoff = elf_hdr.e_phoff;
    for (int i = 0; i < elf_hdr.e_phnum; i++, e_phoff += elf_hdr.e_phentsize) {
        if (sys_lseek(file, e_phoff, 0) < 0) {
            log_print("read file failed\n");
            goto load_failed;
        }

        // 读取程序头后解析，这里不用读取到新进程的页表中，因为只是临时使用下
        cnt = sys_read(file, (char *)&elf_phdr, sizeof(Elf32_Phdr));
        if (cnt < sizeof(Elf32_Phdr)) {
            log_print("read file failed\n");
            goto load_failed;
        }

        // 简单做一些检查，如有必要，可自行加更多
        // 主要判断是否是可加载的类型，并且要求加载的地址必须是用户空间
        if ((elf_phdr.p_type != PT_LOAD) || (elf_phdr.p_vaddr < MEMORY_TASK_BASE)) {
           continue;
        }

        // 加载当前程序头
        int err = load_phdr(file, &elf_phdr, page_dir);
        if (err < 0) {
            log_print("load program hdr failed\n");
            goto load_failed;
        }

        // 简单起见，不检查了，以最后的地址为bss的地址
        task->heap_start = elf_phdr.p_vaddr + elf_phdr.p_memsz;
        task->heap_end = task->heap_start;
   }

    sys_close(file);
    return elf_hdr.e_entry;

load_failed:
    if (file >= 0) {
        sys_close(file);
    }

    return 0;
}

static int copy_args(char *to, u32 page_dir, int argc, char **argv){
    task_args_t task_args;
    task_args.argc = argc;
    task_args.argv = (char **)(to + sizeof(task_args));

    char *dest_arg = to + sizeof(task_args) + sizeof(char *) * (argc + 1);

    char **dest_arg_tb = (char **)memory_get_paddr(page_dir, (u32)(to + sizeof(task_args)));

    for (int i = 0; i < argc; ++i){
        char *from = argv[i];
        int len = kernel_strlen(from) + 1;
        int err = memory_copy_uvm_data((u32)dest_arg, page_dir, (u32)from, len);
        ASSERT(err >= 0);
        dest_arg_tb[i] = dest_arg;
        dest_arg += len;                                                                                                                                                                                                                           
    }
    if (argc) {
        dest_arg_tb[argc] = '\0';
    }
     
    return memory_copy_uvm_data((u32)to, page_dir, (u32)&task_args, sizeof(task_args));
}

int sys_execve(char *name, char **argv, char **env){
    task_t *task = task_get_current_task();

    kernel_strncpy(task->name, get_file_name(name), TASK_NAME_SIZE);

    u32 old_page_dir = task->tss.cr3;

    u32 new_page_dir = memory_create_uvm();
    if (!new_page_dir){
        goto exec_failed;
    }

    u32 entry = load_elf_file(task, name, new_page_dir);
    if (entry == 0){
        goto exec_failed;
    }

    u32 stack_top = MEM_TASK_STACK_TOP - MEM_TASK_ARG_SIZE;
    int err = memory_alloc_for_page_dir(new_page_dir, MEM_TASK_STACK_TOP - MEM_TASK_STACK_SIZE, 
    MEM_TASK_STACK_SIZE, 
    PTE_P | PTE_U | PTE_W);

    if(err < 0) {
        goto exec_failed;
    }

    int argc = string_count(argv);

    err = copy_args((char *)stack_top, new_page_dir, argc, argv);

    if (err < 0){
        goto exec_failed;
    }

    syscall_frame_t *frame = (syscall_frame_t *)(task->tss.esp0 - sizeof(syscall_frame_t));
    frame->eip = entry;
    frame->eax = frame->ebx = frame->ecx = frame->edx = 0;
    frame->esi = frame->edi = frame->ebp = 0;
    frame->eflags = EFLAGS_IF | EFLAGS_DEFAULT;

    frame->esp = stack_top - sizeof(u32) * SYSCALL_PARAM_COUNT;

    task->tss.cr3 = new_page_dir;

    mmu_set_page_dir(new_page_dir);

    memory_destory_uvm(old_page_dir);


    return 0;

exec_failed:
    if (new_page_dir) {
        task->tss.cr3 = old_page_dir;
        mmu_set_page_dir(new_page_dir);
        memory_destory_uvm(new_page_dir);
    } 
    return -1;
}

//fs
// 让task的file结构指向一个文件
int task_alloc_fd(file_t *file){
    task_t *task = task_get_current_task();
    for (int i = 0; i < TASK_OFILE_NR; ++i){
        file_t *find_file = task->file_table[i];
        if (find_file == (file_t *)0){
            task->file_table[i] = file;
            return i;
        }
    }
    return -1;
}

void task_remove_fd(int fd){
     if (fd >= 0 && fd <= TASK_OFILE_NR){
        task_get_current_task()->file_table[fd] = (file_t *)0;
    }
}

file_t *task_file(int fd){
    if (fd >= 0 && fd <= TASK_OFILE_NR){
        file_t *file = task_get_current_task()->file_table[fd];
        return file;
    }

    return (file_t *)0;
}