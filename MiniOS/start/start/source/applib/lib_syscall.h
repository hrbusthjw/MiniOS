#ifndef LIB_SYSCALL_H
#define LIB_SYSCALL_H

#include "osConfig.h"
#include "core/syscall.h"
#include <sys/stat.h>

typedef struct _syscall_args_t
{
    int id;
    int arg0;
    int arg1;
    int arg2;
    int arg3;
}syscall_args_t;

void msleep(int ms);
int getpid(void);
void print_msg(const char *fmt, int arg);
int fork(void);
int execve(const char *name, char * const *argv, char * const *env);
int yield(void);

int open(const char *name, int flags, ...);
int read(int file, char *ptr, int len);
int write(int file, char *ptr, int len);
int close(int file);
int lseek(int file, int ptr, int dir);

int isatty(int file);
int fstat(int file, struct stat *st);
void *sbrk(ptrdiff_t incr);

int dup(int file);

void _exit(int status);
int wait(int *status);

int unlink(const char *path);

int ioctl(int fd, int cmd, int arg0, int arg1);

struct dirent {
   int index;         // 在目录中的偏移
   int type;            // 文件或目录的类型
   char name [255];       // 目录或目录的名称
   int size;            // 文件大小
};


typedef struct _DIR
{
    int index;          // 当前遍历的索引
    struct dirent dirent;
}DIR;


DIR *opendir(const char *path);
struct dirent *readdir(DIR *dir);
int closedir (DIR *dir);

#endif