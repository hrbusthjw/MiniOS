#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "tools/list.h"

typedef struct _sem_t
{
    int count;
    list_t wait_list;
}sem_t;

void sem_init(sem_t *sem, int count);

void sem_wait(sem_t *sem);
void sem_post(sem_t *sem);

int get_sem_count(sem_t *sem);

#endif