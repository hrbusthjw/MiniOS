#ifndef MAIN_H
#define MAIN_H

#define CLI_INPUT_SIZE      1024

#define CLI_MAX_ARG_NUM     10

#define ESC_CMD_2(Pn, cmd)  "\x1b["#Pn#cmd
#define ESC_CMD_3(f, b, m) "\x1b["#f";"#b#m
#define ESC_CLEAR_SCREEN    ESC_CMD_2(2, J)
#define ESC_COLOR_ERROR     ESC_CMD_3(31, 42, m)
#define ESC_COLOR_DEFAULT   ESC_CMD_3(39, 49, m)
#define ESC_MOVE_CURSOR(row, col)   "\x1b["#row";"#col"H"

typedef struct _cli_cmd_t 
{
    const char *name;
    const char *usage;
    int (*do_func)(int argc, char **argv);
}cli_cmd_t;

typedef struct _cli_t{
    char curr_input[CLI_INPUT_SIZE];
    const cli_cmd_t *cmd_start;
    const cli_cmd_t *cmd_end;
    const char *promot;
}cli_t;

#endif