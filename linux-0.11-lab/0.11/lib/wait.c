/*
 *  linux/lib/wait.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>
#include <sys/wait.h>

pid_t waitpid(pid_t pid, int *wait_stat, int options)
{
    long __res;
    __asm__ volatile("int $0x80"
                     : "=a"(__res)
                     : "0"(__NR_waitpid), "b"((long)(pid)), "c"((long)(wait_stat)), "d"((long)(options)));
    if (__res >= 0)
        return (pid_t)__res;
    errno = (int) - __res;
    return -1;
}

pid_t wait(int *wait_stat)
{
    return waitpid(-1, wait_stat, 0);
}




