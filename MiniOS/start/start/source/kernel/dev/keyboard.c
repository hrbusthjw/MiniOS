#include "dev/keyboard.h"
#include "cpu/irq.h"
#include "common/types.h"
#include "tools/log.h"
#include "common/CPUInlineFun.h"
#include "tools/klib.h"
#include "dev/tty.h"

static kbd_state_t kbd_st;

static const key_map_t map_table[] = {
    [0x2] = {'1', '!'},
    [0x3] = {'2', '@'},
    [0x4] = {'3', '#'},
    [0x5] = {'4', '$'},
    [0x6] = {'5', '%'},
    [0x7] = {'6', '^'},
    [0x08] = {'7', '&'},
    [0x09] = {'8', '*'},
    [0x0A] = {'9', '('},
    [0x0B] = {'0', ')'},
    [0x0C] = {'-', '_'},
    [0x0D] = {'=', '+'},
    [0x0E] = {0x7f, 0x7f},
    [0x0F] = {'\t', '\t'},
    [0x10] = {'q', 'Q'},
    [0x11] = {'w', 'W'},
    [0x12] = {'e', 'E'},
    [0x13] = {'r', 'R'},
    [0x14] = {'t', 'T'},
    [0x15] = {'y', 'Y'},
    [0x16] = {'u', 'U'},
    [0x17] = {'i', 'I'},
    [0x18] = {'o', 'O'},
    [0x19] = {'p', 'P'},
    [0x1A] = {'[', '{'},
    [0x1B] = {']', '}'},
    [0x1C] = {'\n', '\n'},
    [0x1E] = {'a', 'A'},
    [0x1F] = {'s', 'B'},
    [0x20] = {'d', 'D'},
    [0x21] = {'f', 'F'},
    [0x22] = {'g', 'G'},
    [0x23] = {'h', 'H'},
    [0x24] = {'j', 'J'},
    [0x25] = {'k', 'K'},
    [0x26] = {'l', 'L'},
    [0x27] = {';', ':'},
    [0x28] = {'\'', '"'},
    [0x29] = {'`', '~'},
    [0x2B] = {'\\', '|'},
    [0x2C] = {'z', 'Z'},
    [0x2D] = {'x', 'X'},
    [0x2E] = {'c', 'C'},
    [0x2F] = {'v', 'V'},
    [0x30] = {'b', 'B'},
    [0x31] = {'n', 'N'},
    [0x32] = {'m', 'M'},
    [0x33] = {',', '<'},
    [0x34] = {'.', '>'},
    [0x35] = {'/', '?'},
    [0x39] = {' ', ' '},

    [82] = {'0', '0'},
    [79] = {'1', '1'},
    [80] = {'2', '2'},
    [81] = {'3', '3'},
    [75] = {'4', '4'},
    [76] = {'5', '5'},
    [77] = {'6', '6'},
    [71] = {'7', '7'},
    [72] = {'8', '8'},
    [73] = {'9', '9'},
    [53] = {'/', '/'},
    [55] = {'*', '*'},
    [74] = {'-', '-'},
    [78] = {'+', '+'},
    [83] = {'.', '.'},
};

static inline int is_make_code(u8 key_code){
    return !(key_code & 0x80);
}

static inline char get_key(u8 key_code){
    return key_code & 0x7f;
}

static void do_fx_key(char key){
    int index = key - KEY_F1;
    if (kbd_st.lctrl || kbd_st.rctrl){
        tty_select(index);
    }
}

static void do_normal_key(u8 raw_code){
    char key = get_key(raw_code);
    int is_make = is_make_code(raw_code);

    switch (key)
    {
        case KEY_RSHIFT:
            kbd_st.rshift_press = is_make ? 1 : 0;
            break;
        case KEY_LSHIFT:
            kbd_st.lshift_press = is_make ? 1 : 0;
            break;
        case KEY_CAPSLOCK:
            if (is_make){
                kbd_st.capslock = ~kbd_st.capslock;
            }
            break;
        case KEY_ALT:
            kbd_st.lalt = is_make;
            break;
        case KEY_CTRL:
            kbd_st.lctrl = is_make;
            break;
        case KEY_F1:
        case KEY_F2:
        case KEY_F3:
        case KEY_F4:
        case KEY_F5:
        case KEY_F6:
        case KEY_F7:
        case KEY_F8:
            do_fx_key(key);
            break;
        case KEY_F9:
        case KEY_F10:
        case KEY_F11:
        case KEY_F12:
            break;
    
        default:
            if(is_make) {
                if (kbd_st.rshift_press || kbd_st.lshift_press){
                    key = map_table[key].func;
                }else {
                    key = map_table[key].normal;
                }

                if (kbd_st.capslock){
                    if (key >= 'A' && key <= 'Z'){
                        key = key - 'A' + 'a';
                    } else if (key >= 'a' && key <= 'z'){
                        key = key - 'a' + 'A';
                    }
                }
                // log_print("\033[36;44mkey: %c\033[39;49m\n", key);
                tty_in(key);
            }
            break;
    }
}

static void do_e0_key(u8 raw_code){
    char key = get_key(raw_code);
    int is_make = is_make_code(raw_code);

    switch (key)
    {
    case KEY_CTRL:
        kbd_st.rctrl = is_make;
        break;
    case KEY_ALT:
        kbd_st.ralt = is_make;
        break;
    
    default:
        break;
    }
}

void kbd_init(void){
    static int inited = 0;
    if (!inited){
        kernel_memset(&kbd_st, 0, sizeof(kbd_st));
        irq_install(IRQ1_KEYBOARD, (irq_handler_t)exception_handler_kbd);
        irq_enable(IRQ1_KEYBOARD);

        inited = 1;
    }
}

void do_handler_kbd(exception_frame_t *frame){
    static enum {
        NORMAL,
        BEGIN_0XE0,
        BEGIN_0XE1,
    }recv_st = NORMAL;

    u32 status = inb(KBD_PORT_STAT);
    if (!(status & KBD_STAT_RECV_READY)){
        irq_send_eoi(IRQ1_KEYBOARD);
        return;
    }

    u8 raw_code = inb(KBD_PORT_DATA);
    // log_print("\033[31;42mkey: %d\033[39;49m", raw_code);
    irq_send_eoi(IRQ1_KEYBOARD);

    if (raw_code == KEY_0XE0) {
        recv_st = BEGIN_0XE0;
    } else if (raw_code == KEY_0XE1)
    {
        recv_st = BEGIN_0XE1;
    } else {
        switch (recv_st)
        {
        case NORMAL:
            do_normal_key(raw_code);
            break;
        case BEGIN_0XE0:
            do_e0_key(raw_code);
            recv_st = NORMAL;
            break;
        case BEGIN_0XE1:
            recv_st = NORMAL;
            break;
        
        default:
            break;
        }
    }
    
}
