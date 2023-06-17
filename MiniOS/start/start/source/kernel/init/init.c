#include "init.h"
#include "core/memory.h"
#include "tools/klib.h"
#include "dev/console.h"
#include "dev/keyboard.h"
#include "fs/fs.h"

static sem_t sem;

void kernelInit(boot_info_t *boot_info)
{
    cpu_init();
    irq_init();
    log_init();

    memory_init(boot_info);
    fs_init();
    time_init();


    task_manager_init();
}

void move_to_first_task(void){
    task_t *curr = task_get_current_task();
    ASSERT(curr != 0);

    tss_t *tss = &(curr->tss);
    __asm__ __volatile__(
        "push %[ss]\n\t"
        "push %[esp]\n\t"
        "push %[eflags]\n\t"
        "push %[cs]\n\t"
        "push %[eip]\n\t"
        "iret\n\t"
        ::[ss]"r"(tss->ss), [esp]"r"(tss->esp), [eflags]"r"(tss->eflags),
        [cs]"r"(tss->cs), [eip]"r"(tss->eip)
    );
}

void init_main(void)
{
    log_print("=========================================\n");
    log_print("Kernel is running......\n");
    log_print("Version: %s, %s\n", OS_VISION, "MiniOS");
    log_print("Created by HRBUST HJW\n");
    log_print("=========================================\n");

    task_first_init();

    move_to_first_task();
}