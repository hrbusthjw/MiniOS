#ifndef KEY_H
#define KEY_H
#include "cpu/irq.h"

#define KEY_RSHIFT      0X36
#define KEY_LSHIFT      0X2A
#define KEY_CAPSLOCK    0X3A
#define KEY_0XE0        0XE0
#define KEY_0XE1        0XE1
#define KEY_CTRL        0X1D
#define KEY_ALT         0X38

#define KEY_F1          0X3B
#define KEY_F2          0X3C
#define KEY_F3          0X3D
#define KEY_F4          0X3E
#define KEY_F5          0X3F
#define KEY_F6          0X40
#define KEY_F7          0X41
#define KEY_F8          0X42
#define KEY_F9          0X43
#define KEY_F10         0X44
#define KEY_F11         0X57
#define KEY_F12         0X58

#define KBD_PORT_DATA       0X60
#define KBD_PORT_STAT       0X64
#define KBD_PORT_CMD        0X64

#define KBD_STAT_RECV_READY (1 << 0)

typedef struct _key_map_t{
    u8 normal;
    u8 func;
}key_map_t;

typedef struct _kbd_state_t {
    int lshift_press : 1;
    int rshift_press : 1;
    int capslock : 1;
    int lalt : 1;
    int ralt : 1;
    int lctrl : 1;
    int rctrl : 1;
}kbd_state_t;



void kbd_init(void);
void exception_handler_kbd(void);
void do_handler_kbd(exception_frame_t *frame);


#endif