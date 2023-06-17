#include "tools/log.h"
#include "common/CPUInlineFun.h"
#include "stdarg.h"
#include "tools/klib.h"
#include "cpu/irq.h"
#include "ipc/mutex.h"
#include "dev/console.h"
#include "dev/dev.h"

#define LOG_USE_COM     0
static mutex_t mutex;

static int log_dev_id;

//串口初始化
void log_init(void)
{
    mutex_init(&mutex);

    log_dev_id = dev_open(DEV_TTY, 0, (void *)0);

#if LOG_USE_COM
    outb(COM1_PROT + 1, 0x00);
    outb(COM1_PROT + 3, 0x80);
    outb(COM1_PROT + 0, 0x03);
    outb(COM1_PROT + 1, 0x00);
    outb(COM1_PROT + 3, 0x03);
    outb(COM1_PROT + 2, 0xc7);
    outb(COM1_PROT + 4, 0x0f);
#endif

}

void log_print(const char *fmt, ...)
{
    char str_buf[128];

    va_list args;

    kernel_memset(str_buf, '\0', sizeof(str_buf));
    va_start(args, fmt);
    kernel_vsprintf(str_buf, fmt, args);
    va_end(args);

    mutex_lock(&mutex); //上锁

#if LOG_USE_COM
    const char *p = str_buf;
    while (*p != '\0')
    {
        while ((inb(COM1_PROT + 5) & (1 << 6)) == 0)
            ; // 忙检测
        outb(COM1_PROT, *p++);
    }
    outb(COM1_PROT, '\r');
    outb(COM1_PROT, '\n');
#else
    // console_write(0, str_buf, kernel_strlen(str_buf));
    dev_write(log_dev_id, 0, str_buf, kernel_strlen(str_buf));
    // char c = '\n';
    // console_write(0, &c, 1);
    // dev_write(log_dev_id, 0, &c, 1);
#endif


    mutex_unlock(&mutex);

}