/*
 *  linux/lib/dup.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>

int dup(int fd)
{
    long __res;
    __asm__ volatile("int $0x80"
                     : "=a"(__res)
                     : "0"(__NR_dup), "b"((long)(fd)));
    if (__res >= 0)
        return (int)__res;
    errno = (int)-__res;
    return -1;
}

