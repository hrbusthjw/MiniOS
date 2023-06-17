#include "tools/klib.h"
#include "tools/log.h"
#include "common/CPUInlineFun.h"

void kernel_strcpy(char *dest, const char *src)
{
    int i = 0;
    while (*(src + i) != '\0')
    {
        *(dest + i) = *(src + i);
        i++;
    }
    *(dest + i) = '\0';
}

void kernel_strncpy(char *dest, const char *src, int size)
{
    int i = 0;
    while (*(src + i) != '\0' && i < size)
    {
        *(dest + i) = *(src + i);
        i++;
    }
    while (i < size)
    {
        *(dest + i) = '\0';
        i++;
    }
}

int kernel_strncmp(const char *s1, const char *s2, int size)
{
    for (int i = 0; i < size; ++i)
    {
        if (*(s1 + i) != *(s2 + i))
        {
            return (*(s1 + i) < *(s2 + i)) ? -1 : 1;
        }
        else if (*(s1 + i) == '\0')
        {
            return 0;
        }
    }
    return 0;
}

int kernel_strlen(const char *str)
{
    int i;
    for (i = 0; *(str + i) != '\0'; ++i)
        ;
    return i;
}

void kernel_memcpy(void *dest, void *src, int size)
{
    char *p_dest = (char *)dest;
    char *p_src = (char *)src;
    for (int i = 0; i < size; ++i)
    {
        *(p_dest + i) = *(p_src + i);
    }
}

int kernel_memset(void *dest, int value, int size)
{
    u8 *p_dest = (u8 *)dest;
    for (int i = 0; i < size; ++i)
    {
        *(p_dest + i) = (u8)value;
    }
    return 0;
}

int kernel_memcmp(void *d1, void *d2, int size)
{
    u8 *p1 = (u8 *)d1;
    u8 *p2 = (u8 *)d2;
    for (int i = 0; i < size; i++)
    {
        if (*(p1 + i) != *(p2 + i))
        {
            return (*(p1 + i) < *(p2 + i)) ? -1 : 1;
        }
    }
    return 0;
}

void kernel_itoa(u32 num, char *str, int base)
{
    char *p = str;
    char *p1, *p2;
    do
    {
        int remainder = num % base;
        if (remainder < 10)
        {
            *p++ = '0' + remainder;
        }
        else
        {
            *p++ = 'A' + remainder - 10;
        }
        num /= base;
    } while (num != 0);
    *p = '\0';

    // 反转字符串
    p1 = str;
    p2 = p - 1;
    while (p1 < p2)
    {
        char tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
        p1++;
        p2--;
    }
}

void kernel_vsprintf(char *buffer, const char *fmt, va_list args)
{
    char *p = buffer; // 缓冲区指针
    int i;

    for (i = 0; fmt[i] != '\0'; i++)
    {
        if (fmt[i] == '%')
        {
            char *s;
            int n;
            u32 x;

            switch (fmt[++i])
            {
            case 's':
                s = va_arg(args, char *);
                while (*s != '\0')
                {
                    *p++ = *s++;
                }
                break;
            case 'd':
                n = va_arg(args, int);
                if (n < 0)
                {
                    *p++ = '-';
                    n = -n;
                }
                kernel_itoa(n, p, 10);
                while (*p != '\0')
                {
                    p++;
                }
                break;
            case 'X':
            case 'x': 
                x = va_arg(args, long int);
                kernel_itoa(x, p, 16);
                while (*p != '\0')
                {
                    p++;
                }
                break;
            case 'O':
            case 'o': 
                n = va_arg(args, int);
                kernel_itoa(n, p, 8);
                while (*p != '\0')
                {
                    p++;
                }
                break;
            case 'b': 
                n = va_arg(args, int);
                kernel_itoa(n, p, 2);
                while (*p != '\0')
                {
                    p++;
                }
                break;
            case 'c':
                *p++ = va_arg(args, int);
                break;
            default: 
                *p++ = fmt[i];
                break;
            }
        }
        else
        {
            *p++ = fmt[i];
        }
    }
    *p = '\0';
}

void kernel_sprintf(char *buffer, const char *fmt, ...){
    va_list args;

    va_start(args, fmt);
    kernel_vsprintf(buffer, fmt, args);
    va_end(args);
}

void panic(const char *file, int line, const char *func, const char *cond){
    log_print("assert failed! %s", cond);
    log_print("file: %s\nline %d\nfunc:%s\n", file, line, func);
    hlt();
}

int string_count(char **start){
    int count = 0;
    if (start){
        while(*start++){
            count++;
        }
    }
    return count;
}

char *get_file_name(const char *path){
    char *s = (char *)path;
    while (*s != '\0'){
        s++;
    }

    while((*s != '/') && (*s != '\\') && (s >= path)){
        s--;
    }

    return s + 1;
}