#ifndef CONSOLE_H
#define CONSOLE_H
#include "common/types.h"
#include "dev/tty.h"
#include "ipc/mutex.h"

#define CONSOLE_DISP_ADDR   0XB8000
#define CONSOLE_DISP_END    (0XB8000 + 32*1024)
#define CONSOLE_ROW_MAN     25
#define CONSOLE_COL_MAX     80

#define ASCII_ESC           0X1B

#define ESC_PARMA_MAX       10

typedef enum {
    COLOR_BLACK = 0,
    COLOR_BLUE,
    COLOR_GREEN,
    COLOR_CYAN,
    COLOR_RED,
    COLOR_MAGENTA,
    COLOR_BROWN,
    COLOR_GRAY,
    COLOR_DARKGRAY,
    COLOR_LIGHT_BLUE,
    COLOR_LIGHT_GREEN,
    COLOR_LIGHT_GYAN,
    COLOR_LIGHT_RED,
    COLOR_LIGHT_MAGENTA,
    COLOR_YELLOW,
    COLOR_WHITE,
}color_t;

typedef union {
	struct {
		char c;						// 显示的字符
		char foreground : 4;		// 前景色
		char background : 3;		// 背景色
	};

	u16 v;
}disp_char_t;

typedef struct _console_t
{
    enum{
        CONSOLE_WRITE_NORMAL,
        CONSOLE_WRITE_ESC,
        CONSOLE_WRITE_SQUARE,
    }write_state;

    disp_char_t *disp_base;
    int cursor_row,cursor_col;
    int display_rows, display_cols;
    color_t foreground, background;

    int old_cursor_col, old_cursor_row;

    int esc_param[ESC_PARMA_MAX];
    int curr_param_index;

    mutex_t mutex;
}console_t;

int console_init(int index);
int console_write(tty_t *tty);
void console_close(int console);
void console_select(int index);


#endif