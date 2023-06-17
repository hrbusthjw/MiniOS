#ifndef MAIN_H
#define MAIN_H

#define ESC_CMD_2(Pn, cmd)  "\x1b["#Pn#cmd
#define ESC_CMD_3(f, b, m) "\x1b["#f";"#b#m
#define ESC_CLEAR_SCREEN    ESC_CMD_2(2, J)
#define ESC_COLOR_ERROR     ESC_CMD_3(31, 42, m)
#define ESC_COLOR_DEFAULT   ESC_CMD_3(39, 49, m)
#define ESC_MOVE_CURSOR(row, col)   "\x1b["#row";"#col"H"

#endif