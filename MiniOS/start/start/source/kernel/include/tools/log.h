#ifndef LOG_H
#define LOG_H

#define COM1_PROT 0x3f8 // 串口号

void log_init(void);
void log_print(const char *fmt, ...);

#endif