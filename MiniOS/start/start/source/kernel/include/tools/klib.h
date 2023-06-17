#ifndef KLIB_H
#define KLIB_H

#include "common/types.h"
#include "stdarg.h"

#ifndef RELEASE

#define ASSERT(expr)    \
    if (!(expr)) panic(__FILE__, __LINE__, __func__, #expr)
    void panic(const char *file, int line, const char *func, const char *cond);
#else
#define ASSERT(expr)           ((void)0)
#endif

static inline u32 down2(u32 size, u32 bound){
    return size & ~(bound - 1);
}

static inline u32 up2(u32 size, u32 bound){
    return (size + bound - 1) & ~(bound - 1);
}

void panic(const char *file, int line, const char *func, const char *cond);

void kernel_strcpy(char *dest, const char *src);
void kernel_strncpy(char *dest, const char *src, int size);
int kernel_strncmp(const char *s1, const char *s2, int size);
int kernel_strlen(const char *str);

void kernel_memcpy(void *dest, void *src, int size);
int kernel_memset(void *dest, int value, int size);
int kernel_memcmp(void *d1, void *d2, int size);

void kernel_sprintf(char *buffer, const char *fmt, ...);
void kernel_vsprintf(char *buffer, const char *fmt, va_list args);

int string_count(char **start);
char *get_file_name(const char *path);

#endif