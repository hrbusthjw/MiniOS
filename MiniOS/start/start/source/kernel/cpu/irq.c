#include "cpu/irq.h"
#include "tools/log.h"
#include "common/CPUInlineFun.h"
#include "cpu/cpu.h"
#include "core/task.h"
#define IDT_TABLE_NR 128

void exception_handler_unknow(void);

static gate_desc_t idt_table[IDT_TABLE_NR];

// 初始化8259
static void init_pic(void)
{
    outb(PIC0_ICW1, PIC_ICW1_ALWAYS_1 | PIC_ICW1_ICW4);
    outb(PIC0_ICW2, IRQ_PIC_START);
    outb(PIC0_ICW3, 1 << 2);
    outb(PIC0_ICW4, PIC_ICW4_8086);

    outb(PIC1_ICW1, PIC_ICW1_ALWAYS_1 | PIC_ICW1_ICW4);
    outb(PIC1_ICW2, IRQ_PIC_START + 8);
    outb(PIC1_ICW3, 2);
    outb(PIC1_ICW4, PIC_ICW4_8086);

    outb(PIC0_IMR, 0XFF & ~(1 << 2));
    outb(PIC1_IMR, 0XFF);
};

// 中断初始化
void irq_init(void)
{
    for (int i = 0; i < IDT_TABLE_NR; ++i)
    {
        gate_desc_set(idt_table + i, KERNEL_SELECTOR_CS, (u32)exception_handler_unknow,
                      GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_INT);
    }

    irq_install(IRQ0_DE, (irq_handler_t)exception_handler_divider);
    irq_install(IRQ1_DB, (irq_handler_t)exception_handler_debug);
    irq_install(IRQ2_NMI, (irq_handler_t)exception_handler_NMI);
    irq_install(IRQ3_BP, (irq_handler_t)exception_handler_breakpoint);
    irq_install(IRQ4_OF, (irq_handler_t)exception_handler_overflow);
    irq_install(IRQ5_BR, (irq_handler_t)exception_handler_BOUND_Range);
    irq_install(IRQ6_UD, (irq_handler_t)exception_handler_invaild_opcode);
    irq_install(IRQ7_NM, (irq_handler_t)exception_handler_device_unavailbale);
    irq_install(IRQ8_DF, (irq_handler_t)exception_handler_double_fault);
    irq_install(IRQ10_TS, (irq_handler_t)exception_handler_invaild_tss);
    irq_install(IRQ11_NP, (irq_handler_t)exception_handler_segment_not_present);
    irq_install(IRQ12_SS, (irq_handler_t)exception_handler_stack_segmant_fault);
    irq_install(IRQ13_GP, (irq_handler_t)exception_handler_general_protection);
    irq_install(IRQ14_PF, (irq_handler_t)exception_handler_page_fault);
    irq_install(IRQ16_MF, (irq_handler_t)exception_handler_fpu_error);
    irq_install(IRQ17_AC, (irq_handler_t)exception_handler_aligment_check);
    irq_install(IRQ18_MC, (irq_handler_t)exception_handler_machine_check);
    irq_install(IRQ19_XM, (irq_handler_t)exception_handler_simd_exception);
    irq_install(IRQ20_VE, (irq_handler_t)exception_handler_virtual_exception);
    irq_install(IRQ21_CP, (irq_handler_t)exception_handler_control_exception);

    lidt((u32)idt_table, sizeof(idt_table));

    init_pic();
}

static void dump_core_regs(exception_frame_t *frame){
    u32 ss, esp;
    if (frame->cs & 0x3){
        ss = frame->ss3;
        esp = frame->esp3;
    }else{
        ss = frame->ds;
        esp = frame->esp;
    }
    log_print("IRQ: %d, error code: %d", frame->num, frame->error_code);
    log_print("CS: %d\nDS: %d\nES: %d\nSS: %d\nFS: %d\nGS: %d\n",
    frame->cs, frame->ds, frame->es, frame->es, ss, frame->gs);
    log_print("EAX: 0x%X\nEBX: 0x%X\nECX: 0x%X\nEDX: 0x%X\nEDI: 0x%X\nESI: 0x%X\nESP: 0x%X\nESP: 0x%X\n",
    frame->eax, frame->ebx, frame->ecx, frame->edx, frame->edi, frame->esi, esp);
    log_print("EIP: 0x%X\nEFLAGS: 0x%X\n", frame->eip, frame->eflags);
}

static void do_default_handler(exception_frame_t *frame, const char *message)
{
    log_print("------------------------------------------\n");
    log_print("IRQ/Exception happend: %s\n", message);
    dump_core_regs(frame);
    log_print("------------------------------------------\n");
    if (frame->cs & 0x3) {
        sys_exit(frame->error_code);
    } else {
        while(1) {
            hlt();
        }
    }
}

void do_handler_unknow(exception_frame_t *frame)
{
    do_default_handler(frame, "Unknow exception");
}

void do_handler_divider(exception_frame_t *frame)
{
    do_default_handler(frame, "Divider exception");
}

void do_handler_debug(exception_frame_t *frame)
{
    do_default_handler(frame, "Debug exception");
}

void do_handler_NMI(exception_frame_t *frame)
{
    do_default_handler(frame, "NMI exception");
}

void do_handler_breakpoint(exception_frame_t *frame)
{
    do_default_handler(frame, "Breakpoint exception");
}

void do_handler_overflow(exception_frame_t *frame)
{
    do_default_handler(frame, "Overflow exception");
}

void do_handler_BOUND_Range(exception_frame_t *frame)
{
    do_default_handler(frame, "BOUND Range exception");
}

void do_handler_invaild_opcode(exception_frame_t *frame)
{
    do_default_handler(frame, "Invalid opcode exception");
}

void do_handler_device_unavailbale(exception_frame_t *frame)
{
    do_default_handler(frame, "Device unavailable exception");
}

void do_handler_double_fault(exception_frame_t *frame)
{
    do_default_handler(frame, "Double fault exception");
}

void do_handler_invaild_tss(exception_frame_t *frame)
{
    do_default_handler(frame, "Invalid TSS exception");
}

void do_handler_segment_not_present(exception_frame_t *frame)
{
    do_default_handler(frame, "Segment not present exception");
}

void do_handler_stack_segmant_fault(exception_frame_t *frame)
{
    do_default_handler(frame, "Stack segment fault exception");
}

void do_handler_general_protection(exception_frame_t *frame)
{
    log_print("----------------------------------------------\n");
    log_print("IRQ/Exception happend: General protection fault\n");

    if (frame->error_code & ERR_EXT){
        log_print("The exception occurred during delivery of an "
                "event external to the program, such as an interrupt"
                "or an earlier exception\n");
    }else{
        log_print("The exception occurred during delivery of a"
                    "software interrupt (INT n, INT3, or INTO)\n");
    }

    if (frame->error_code & ERR_IDT){
        log_print("The index portion of the error code refers "
                    "to a gate descriptor in the IDT\n");
    }else{
        log_print("The index refers to a descriptor in the GDT\n");
    }

    log_print("segment index: %d\n", frame->error_code & 0xFFF8);

    dump_core_regs(frame);
    log_print("------------------------------------------\n");
    if (frame->cs & 0x3) {
        sys_exit(frame->error_code);
    } else {
        while(1) {
            hlt();
        }
    }
}

void do_handler_page_fault(exception_frame_t *frame)
{
    log_print("----------------------------------------------\n");
    log_print("IRQ/Exception happend: Page fault\n");

    if (frame->error_code & ERR_PAGE_P){
        log_print("The fault was caused by a page-level protection violation : 0x%x\n", readCR2());
    }else{
        log_print("The fault was caused by a none-present page : 0x%x\n", readCR2());
    }

    if (frame->error_code & ERR_PAGE_WR){
        log_print("The access causing the fault was a write : 0x%x\n", readCR2());
    }else{
        log_print("The access causing the fault was a read : 0x%x\n", readCR2());
    }

    if (frame->error_code & ERR_PAGE_US){
        log_print("A user-mode access caused the fault : 0x%x\n", readCR2());
    }else{
        log_print("A supervisor-mode access caused the fault : 0x%x\n", readCR2());
    }


    dump_core_regs(frame);
    log_print("------------------------------------------\n");
    if (frame->cs & 0x3) {
        sys_exit(frame->error_code);
    } else {
        while(1) {
            hlt();
        }
    }
}

void do_handler_fpu_error(exception_frame_t *frame)
{
    do_default_handler(frame, "FPU error exception");
}

void do_handler_aligment_check(exception_frame_t *frame)
{
    do_default_handler(frame, "Alignment check exception");
}

void do_handler_machine_check(exception_frame_t *frame)
{
    do_default_handler(frame, "Machine check exception");
}

void do_handler_simd_exception(exception_frame_t *frame)
{
    do_default_handler(frame, "SIMD exception");
}

void do_handler_virtual_exception(exception_frame_t *frame)
{
    do_default_handler(frame, "Virtualization exception");
}

void do_handler_control_exception(exception_frame_t *frame)
{
    do_default_handler(frame, "Control exception");
}

int irq_install(int irq_num, irq_handler_t handler)
{
    if (irq_num >= IDT_TABLE_NR)
    {
        return -1;
    }

    gate_desc_set(idt_table + irq_num, KERNEL_SELECTOR_CS,
                  (u32)handler, GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_INT);
    return 0;
}

// 使能中断号对应的中断
void irq_enable(int irq_num)
{
    if (irq_num < IRQ_PIC_START)
    {
        return;
    }
    irq_num -= IRQ_PIC_START;
    if (irq_num < 8)
    {
        u8 mask = inb(PIC0_IMR) & ~(1 << irq_num);
        outb(PIC0_IMR, mask);
    }
    else
    {
        irq_num -= 8;
        u8 mask = inb(PIC1_IMR) & ~(1 << irq_num);
        outb(PIC1_IMR, mask);
    }
}

void irq_disable(int irq_num)
{
    if (irq_num < IRQ_PIC_START)
    {
        return;
    }
    irq_num -= IRQ_PIC_START;
    if (irq_num < 8)
    {
        u8 mask = inb(PIC0_IMR) | (1 << irq_num);
        outb(PIC0_IMR, mask);
    }
    else
    {
        irq_num -= 8;
        u8 mask = inb(PIC1_IMR) | (1 << irq_num);
        outb(PIC1_IMR, mask);
    }
}

void irq_disable_global(void)
{
    cli();
}

void irq_enable_global(void)
{
    sti();
}
// 向8259发送中断结束信号
void irq_send_eoi(int irq_num)
{
    irq_num -= IRQ_PIC_START;
    if (irq_num >= 8)
    {
        outb(PIC1_OCW2, PIC_OCW2_EOI);
    }
    outb(PIC0_OCW2, PIC_OCW2_EOI);
}

irq_state_t irq_enter_critical_section(void){
    irq_state_t state = read_eflags();
    irq_disable_global();
    return state;
}

void irq_leave_critical_section(irq_state_t state){
    write_eflags(state);
}