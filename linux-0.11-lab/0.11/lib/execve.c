/*
 *  linux/lib/execve.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>

int execve(const char * a, char ** b, char ** c)
{
    long __res;
    __asm__ volatile("int $0x80"
                     : "=a"(__res)
                     : "0"(__NR_execve), "b"((long)(a)), "c"((long)(b)), "d"((long)(c)));
    if (__res >= 0)
        return (int)__res;
    errno = (int)-__res;
    return -1;                                                                            
}

