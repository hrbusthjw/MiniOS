#include "ipc/semaphore.h"
#include "core/task.h"
#include "cpu/irq.h"

void sem_init(sem_t *sem, int count)
{
    sem->count = count;
    list_init(&sem->wait_list);
}

void sem_wait(sem_t *sem)
{
    irq_state_t state = irq_enter_critical_section();
    if (sem->count > 0)
    {
        sem->count--; // 当前进程获得信号
    }
    else
    {
        task_t *curr = task_get_current_task();
        task_set_block(curr);
        list_insert_tail(&sem->wait_list, &curr->wait_node);
        task_dispatch(); //切换进程
    }
    irq_leave_critical_section(state);
}

void sem_post(sem_t *sem)
{
    irq_state_t state = irq_enter_critical_section();

    if (list_count(&sem->wait_list))
    {
        list_node_t *node = list_delete_head(&sem->wait_list);
        task_t *task = list_node_parent(node, task_t, wait_node);
        task_set_ready(task);

        task_dispatch();
    }
    else
    {
        sem->count++;
    }

    irq_leave_critical_section(state);
}

int get_sem_count(sem_t *sem){
    irq_state_t state = irq_enter_critical_section();
    int count = sem->count;
    irq_leave_critical_section(state);
    return count;
}