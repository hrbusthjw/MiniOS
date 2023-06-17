#include "dev/time.h"

static u32 sys_tick;

// 定时器中断处理函数
void do_handler_time(exception_frame_t *frame)
{
    sys_tick++;
    irq_send_eoi(IRQ0_TIMER);
    task_time_tick();
}

// 初始化8253，安装中断向量表，使能中断
static void init_pit(void)
{
    u32 reload_count = PIT_OSC_FQER * OS_TICKS_MS / 1000;
    outb(PIT_COMMAND_MODE_PORT, PIT_CHANNEL | PIT_LOAD_LOHI | PIT_MODE3);
    outb(PIT_CHANNEL0_DATA_PORT, reload_count & 0xff);
    outb(PIT_CHANNEL0_DATA_PORT, (reload_count >> 8) & 0xff);

    irq_install(IRQ0_TIMER, (irq_handler_t)exception_handler_time);
    irq_enable(IRQ0_TIMER);
}

// 初始化定时器
void time_init(void)
{
    sys_tick = 0;
    init_pit();
}