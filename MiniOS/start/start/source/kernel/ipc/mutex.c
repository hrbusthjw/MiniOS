#include "ipc/mutex.h"
#include "cpu/irq.h"

void mutex_init(mutex_t *mutex)
{
    mutex->locked_count = 0;
    mutex->owner = (task_t *)0;
    list_init(&mutex->wait_list);
}

void mutex_lock(mutex_t *mutex)
{
    irq_state_t state = irq_enter_critical_section();

    task_t *curr = task_get_current_task();

    if (mutex->locked_count == 0)
    {
        mutex->locked_count++;
        mutex->owner = curr;
    }
    else if (mutex->owner == curr)
    {
        mutex->locked_count++;
    }
    else
    {
        task_set_block(curr);
        list_insert_tail(&mutex->wait_list, &curr->wait_node);
        task_dispatch();
    }

    irq_leave_critical_section(state);
}
void mutex_unlock(mutex_t *mutex)
{
    irq_state_t state = irq_enter_critical_section();

    task_t *curr = task_get_current_task();

    if (mutex->owner == curr)
    {
        if (--mutex->locked_count == 0)
        {
            mutex->owner = (task_t *)0;
            if (list_count(&mutex->wait_list))
            {
                list_node_t *node = list_delete_head(&mutex->wait_list);
                task_t *task = list_node_parent(node, task_t, wait_node);
                task_set_ready(task);

                mutex->locked_count = 1;
                mutex->owner = task;
                // 个人添加，测试使用
                // 打开下面两行代码，当前进程会无条件跳转到下一个等待自旋锁的进程执行
                // 并且直到时间片结束才会切换，此时不会使得进程交替执行，避免无意义的切换
                // 个人认为这种方法的效率更高，不会导致无意义的调用dispatch， 不过实际使用来说似乎差别不大
                // 注释掉下面两行，被互斥锁阻塞的进程将以尽可能快的速度无视时间片调度去执行程序
                // 两种方法其实都可以
                task_set_block(curr);
                task_set_ready(curr);

                task_dispatch();
            }
        }
    }
    irq_leave_critical_section(state);
}