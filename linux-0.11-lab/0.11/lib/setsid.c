/*
 *  linux/lib/setsid.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>

int setsid(void)
{
    long __res;
    __asm__ volatile("int $0x80"
                     : "=a"(__res)
                     : "0"(__NR_setsid));
    if (__res >= 0)
        return (pid_t)__res;
    errno = (int) - __res;
    return -1;
}

