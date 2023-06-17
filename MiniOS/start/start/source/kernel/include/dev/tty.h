#ifndef TTY_H
#define TTY_H
#include "ipc/semaphore.h"

#define TTY_OCRLF           (1 << 0)
#define TTY_INCLR           (1 << 0)
#define TTY_IECHO           (1 << 1)

#define TTY_OBUF_SIZE       512
#define TTY_IBUF_SIZE       512

#define TTY_NR              8

#define TTY_CMD_ECHO        0x1

typedef struct _tty_fifo_t {
    char *buf;
    int size;
    int read, write;
    int count;
}tty_fifo_t;


typedef struct _tty_t{
    char obuf[TTY_OBUF_SIZE];
    char ibuf[TTY_IBUF_SIZE];
    tty_fifo_t ififo;
    tty_fifo_t ofifo;
    sem_t o_semaphore;
    sem_t i_semaphore;

    int iflags;

    int oflags;

    int console_index;
}tty_t;

int tty_fifo_put(tty_fifo_t *fifo, char c);
int tty_fifo_get(tty_fifo_t *fifo, char *c);
void tty_in(char ch);
void tty_select(int tty);



#endif