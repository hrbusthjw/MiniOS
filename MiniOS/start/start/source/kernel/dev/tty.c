#include "dev/tty.h"
#include "dev/dev.h"
#include "tools/log.h"
#include "dev/console.h"
#include "dev/keyboard.h"
#include "cpu/irq.h"

static tty_t tty_devs[TTY_NR];

static int curr_tty = 0;

static tty_t *get_tty(device_t *dev){
    int index = dev->minor;
    if (index < 0 || index >= TTY_NR || (!dev->open_count)){
        log_print("tty is not opened, tty = %d", index);
        return (tty_t *)0;
    }
    return tty_devs + index;
}

void tty_fifo_init(tty_fifo_t *fifo, char *buf, int size){
    fifo->buf = buf;
    fifo->count = 0;
    fifo->size = size;
    fifo->read = 0;
    fifo->write = 0;
}

int tty_fifo_put(tty_fifo_t *fifo, char c){
    irq_state_t st = irq_enter_critical_section();
    if (fifo->count >= fifo->size){
        irq_leave_critical_section(st);
        return -1;
    }

    fifo->buf[fifo->write++] = c;

    if (fifo->write >= fifo->size){
        fifo->write = 0;
    }

    fifo->count++;
    irq_leave_critical_section(st);
    return 0;
}

int tty_fifo_get(tty_fifo_t *fifo, char *c){
    irq_state_t st = irq_enter_critical_section();
    if (fifo->count <= 0){
        irq_leave_critical_section(st);
        return -1;
    }

    *c = fifo->buf[fifo->read++];

    if (fifo->read >= fifo->size){
        fifo->read = 0;
    }

    fifo->count--;
    irq_leave_critical_section(st);
    return 0;
}

int tty_open(device_t *dev){
    int index = dev->minor;
    if (index < 0 || index >= TTY_NR){
        log_print("open tty failed, err tty num = %d", index);
        return -1;
    }

    tty_t *tty = tty_devs + index;
    tty_fifo_init(&tty->ofifo, tty->obuf, TTY_OBUF_SIZE);
    sem_init(&tty->o_semaphore, TTY_OBUF_SIZE);
    sem_init(&tty->i_semaphore, 0);
    tty_fifo_init(&tty->ififo, tty->ibuf, TTY_IBUF_SIZE);
    tty->oflags = TTY_OCRLF;
    tty->iflags = TTY_INCLR | TTY_IECHO;

    tty->console_index = index;

    kbd_init();

    console_init(index);

    return 0;
}

int tty_write(device_t *dev, int addr, char *buf, int size);

int tty_read(device_t *dev, int addr, char *buf, int size){
    if (size < 0){
        return -1;
    }

    tty_t *tty = get_tty(dev);

    char *p_buf = buf;
    int len = 0;
    while (len < size){
        sem_wait(&tty->i_semaphore);

        char ch;
        tty_fifo_get(&tty->ififo, &ch);
        switch (ch)
        {
        case 0x7f:
            if (len == 0){
                continue;
            } else {
                len--;
                p_buf--;
            }
            break;
        case '\n':
            if (tty->iflags & TTY_INCLR && len < size - 1){
                *p_buf++ = '\r';
                len++;
            }
            *p_buf++ = '\n';
            len++;
            break;
        
        default:
            *p_buf++ = ch;
            len++;
            break;
        }

        if (tty->iflags & TTY_IECHO){
            tty_write(dev, 0, &ch, 1);
        }

        if (ch == '\n' || ch == '\r'){
            break;
        }
    }
    return len;
}

int tty_write(device_t *dev, int addr, char *buf, int size){
    if (size < 0){
        return -1;
    }

    tty_t *tty = get_tty(dev);
    if (!tty){
        return -1;
    }

    int len = 0;

    while (size)
    {
        char c = *buf++;

        if ((c == '\n') && (tty->oflags & TTY_OCRLF)){
            sem_wait(&tty->o_semaphore);
            int err = tty_fifo_put(&tty->ofifo, '\r');
            if (err < 0){
                break;
            }
        }

        sem_wait(&tty->o_semaphore);
        int err = tty_fifo_put(&tty->ofifo, c);
        if (err < 0){
            break;
        }

        len++;
        size--;

        // console_write(tty);
    }
    console_write(tty);
    
    return len;
}

int tty_control(device_t *dev, int cmd, int arg0, int arg1){
    tty_t * tty = get_tty(dev);

	switch (cmd) {
	case TTY_CMD_ECHO:
		if (arg0) {
			tty->iflags |= TTY_IECHO;
		} else {
			tty->iflags &= ~TTY_IECHO;
		}
		break;
	default:
		break;
	}
}

int tty_close(device_t *dev){

}

void tty_in(char ch){
    tty_t *tty = tty_devs + curr_tty;

    if (get_sem_count(&tty->i_semaphore) >= TTY_IBUF_SIZE){
        return;
    }

    tty_fifo_put(&tty->ififo, ch);
    sem_post(&tty->i_semaphore);
}

void tty_select(int tty){
    if (tty != curr_tty){
        console_select(tty);
        curr_tty = tty;
    }
}

dev_desc_t dev_tty_desc = {
    .name = "tty",
    .major = DEV_TTY,
    .open = tty_open,
    .read = tty_read,
    .write = tty_write,
    .control = tty_control,
    .close = tty_close,
};