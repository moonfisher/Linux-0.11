#ifndef _SCHED_H
#define _SCHED_H

#define NR_TASKS    64
#define HZ          100

#define FIRST_TASK  task[0]
#define LAST_TASK   task[NR_TASKS - 1]

#include <linux/head.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <signal.h>

#if (NR_OPEN > 32)
#error "Currently the close-on-exec-flags are in one word, max 32 files/proc"
#endif

#define TASK_RUNNING            0
#define TASK_INTERRUPTIBLE      1
#define TASK_UNINTERRUPTIBLE    2
#define TASK_ZOMBIE             3
#define TASK_STOPPED            4

#ifndef NULL
#define NULL ((void *)0)
#endif

extern int copy_page_tables(unsigned long from, unsigned long to, long size);
extern int free_page_tables(unsigned long from, unsigned long size);

extern void sched_init(void);
extern void schedule(void);
extern void trap_init(void);
#ifndef PANIC
void panic(const char *str);
#endif
extern int tty_write(unsigned minor, char *buf, int count);

typedef int (*fn_ptr)();

struct i387_struct
{
    long cwd;
    long swd;
    long twd;
    long fip;
    long fcs;
    long foo;
    long fos;
    long st_space[20]; /* 8*10 bytes for each FP-reg = 80 bytes */
};

struct tss_struct
{
    long back_link; /* 16 high bits zero */
    long esp0;
    long ss0; /* 16 high bits zero */
    long esp1;
    long ss1; /* 16 high bits zero */
    long esp2;
    long ss2; /* 16 high bits zero */
    long cr3;
    long eip;
    long eflags;
    long eax, ecx, edx, ebx;
    long esp;
    long ebp;
    long esi;
    long edi;
    long es;		   /* 16 high bits zero */
    long cs;		   /* 16 high bits zero */
    long ss;		   /* 16 high bits zero */
    long ds;		   /* 16 high bits zero */
    long fs;		   /* 16 high bits zero */
    long gs;		   /* 16 high bits zero */
    long ldt;		   /* 16 high bits zero */
    long trace_bitmap; /* bits: trace 0, bitmap 16-31 */
    struct i387_struct i387;
};

/*
 linux 下可以用 ls -l 命令来看到文件的权限。
 用 ls 命令所得到的表示法的格式是类似这样的：-rwxr-xr-x 。这种表示方法一共有十位：

 9  |   8 7 6   |   5 4 3   |   2 1 0
 -  |   r w x   |   r - x   |   r - x

 第 9 位表示文件类型, 可以为 p、d、l、s、c、b 和 -

 p 表示命名管道文件
 d 表示目录文件
 l 表示符号连接文件
 - 表示普通文件
 s 表示 socket 文件
 c 表示字符设备文件
 b 表示块设备文件

 第 8-6 位、5-3 位、2-0 位分别表示文件所有者的权限，同组用户的权限，其他用户的权限，其形式为rwx：

 r 表示可读，可以读出文件的内容
 w 表示可写，可以修改文件的内容
 x 表示可执行，可运行这个程序
 没有权限的位置用 - 表示

 例子：

 ls -l myfile
 显示为：

 -rwxr-x--- 1 foo staff 7734 Apr 05 17:07 myfile

 表示文件 myfile 是普通文件，文件的所有者是 foo 用户，而 foo 用户属于 staff 组，
 文件只有 1 个硬连接，长度是 7734 个字节，最后修改时间 4月5日17:07。

 所有者 foo 对文件有读写执行权限，staff 组的成员对文件有读和执行权限，其他的用户对这个文件没有权限。

 如果一个文件被设置了 suid 或 sgid 位，会分别表现在所有者或同组用户的权限的可执行位上。

 例如：
 1、-rwsr-xr-x 表示 suid 和所有者权限中可执行位被设置
 2、-rwSr--r-- 表示 suid 被设置，但所有者权限中可执行位没有被设置
 3、-rwxr-sr-x 表示 sgid 和同组用户权限中可执行位被设置
 4、-rw-r-Sr-- 表示 sgid 被设置，但同组用户权限中可执行位没有被社
*/
struct task_struct
{
    /* these are hardcoded - don't touch */
    long state; /* -1 unrunnable, 0 runnable, >0 stopped */
    long counter;
    long priority;
    long signal;
    struct sigaction sigaction[32];
    long blocked; /* bitmap of masked signals */
    /* various fields */
    int exit_code;
    unsigned long start_code;
    unsigned long end_code;
    unsigned long end_data;
    unsigned long brk;
    unsigned long start_stack;
    long pid;
    long father;
    long pgrp;
    long session;
    long leader;
    // uid (ruid), 用于在系统中标识一个用户是谁，当用户使用用户名和密码成功登录后一个
    // linux 系统后就唯一确定了他的 uid.
    unsigned short uid;
    // euid, 用于系统决定用户对系统资源的访问权限，通常情况下等于 uid
    unsigned short euid;
    // suid (Set User ID)，用于对外权限的开放。它是跟文件绑定而不是跟用户绑定
    unsigned short suid;
    // 实际组 id
    unsigned short gid;
    // 有效的组 id，它们指定了访问目标的权限
    unsigned short egid;
    // sgid (Set Group ID),
    unsigned short sgid;
    long alarm;
    long utime;
    long stime;
    long cutime;
    long cstime;
    long start_time;
    unsigned short used_math;
    /* file system info */
    int tty; /* -1 if no tty, so it must be signed */
    unsigned short umask;
    struct m_inode *pwd;
    struct m_inode *root;
    struct m_inode *executable;
    unsigned long close_on_exec;
    struct file *filp[NR_OPEN];
    /* ldt for this task 0 - zero 1 - cs 2 - ds&ss */
    struct desc_struct ldt[3];
    /* tss for this task */
    struct tss_struct tss;
};

/*
 *  INIT_TASK is used to set up the first task table, touch at
 * your own risk!. Base=0, limit=0x9ffff (=640kB)
 */
#define INIT_TASK                                                                                                                                                                                                      \
	/* state etc */ {                                                                                                                                                                                                  \
		0, 15, 15,                                                                                                                                                                                                     \
			/* signals */ 0, {                                                                                                                                                                                         \
								 {},                                                                                                                                                                                   \
							 },                                                                                                                                                                                        \
			0, /* ec,brk... */ 0, 0, 0, 0, 0, 0, /* pid etc.. */ 0, -1, 0, 0, 0, /* uid etc */ 0, 0, 0, 0, 0, 0, /* alarm */ 0, 0, 0, 0, 0, 0, /* math */ 0, /* fs info */ -1, 0022, NULL, NULL, NULL, 0, /* filp */ { \
																																																		   NULL,       \
																																																	   },              \
			{                                                                                                                                                                                                          \
				{0, 0},                                                                                                                                                                                                \
				/* ldt */ {0x9f, 0xc0fa00},                                                                                                                                                                            \
				{0x9f, 0xc0f200},                                                                                                                                                                                      \
			},                                                                                                                                                                                                         \
			/*tss*/ {0, PAGE_SIZE + (long)&init_task, 0x10, 0, 0, 0, 0, (long)&pg_dir, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, _LDT(0), 0x80000000, {}},                                     \
	}

extern struct task_struct *task[NR_TASKS];
extern struct task_struct *last_task_used_math;
extern struct task_struct *current;
extern long volatile jiffies;
extern long startup_time;

#define CURRENT_TIME (startup_time + jiffies / HZ)

extern void add_timer(long jiffies, void (*fn)(void));
extern void sleep_on(struct task_struct **p);
extern void interruptible_sleep_on(struct task_struct **p);
extern void wake_up(struct task_struct **p);

/*
 * Entry into gdt where to find first TSS. 0-nul, 1-cs, 2-ds, 3-syscall
 * 4-TSS0, 5-LDT0, 6-TSS1 etc ...
 */
#define FIRST_TSS_ENTRY 4
#define FIRST_LDT_ENTRY (FIRST_TSS_ENTRY + 1)
#define _TSS(n) ((((unsigned long)n) << 4) + (FIRST_TSS_ENTRY << 3))
#define _LDT(n) ((((unsigned long)n) << 4) + (FIRST_LDT_ENTRY << 3))
#define ltr(n) __asm__("ltr %%ax" ::"a"(_TSS(n)))
#define lldt(n) __asm__("lldt %%ax" ::"a"(_LDT(n)))
#define str(n)                  \
	__asm__("str %%ax\n\t"      \
			"subl %2,%%eax\n\t" \
			"shrl $4,%%eax"     \
			: "=a"(n)           \
			: "a"(0), "i"(FIRST_TSS_ENTRY << 3))
/*
 *	switch_to(n) should switch tasks to task nr n, first
 * checking that n isn't the current task, in which case it does nothing.
 * This also clears the TS-flag if the task we switched to has used
 * tha math co-processor latest.
 */
/*
 Linux0.11 中默认使用的是硬件支持的 tss 切换，系统为每个进程分配一个 tss 结构用来存储进程的运行
 信息（上下文环境），然后通过 CPU 的一个长跳转指令 ljmp，跳转到 TSS 段选择符来，来实现进程的切换，
 LDT 的更新，也是 CPU 根据 TSS 中的 LDT 项自动加载。
 这种方式易于实现，但不便于管理多 CPU 进程，效率不佳，耗费 cpu 时钟周期多，现在操作系统多进程都是
 共享同一个全局 TSS，直接在内存里通过切换堆栈来切换进程
*/
#if ASM_NO_64
#define switch_to(n)                                                        \
{                                                                           \
    struct                                                                  \
    {                                                                       \
        long a, b;                                                          \
    } __tmp;                                                                \
    __asm__("cmpl %%ecx,current\n\t" /*比较当前要切换的进程是否是当前运行的进程*/  \
            "je 1f\n\t" /*如果是则不调度直接退出*/                              \
            "movw %%dx,%1\n\t" /*将要调度的 tss 指针存到 tmp.b 中*/             \
            "xchgl %%ecx,current\n\t" /*交换 pcb 值，要调度的存储在 ecx 中*/     \
            "ljmp *%0\n\t" /*进行长跳转，即切换 tss*/                           \
            "cmpl %%ecx,last_task_used_math\n\t" /*进行切换后的处理协处理器*/    \
            "jne 1f\n\t"                                                     \
            "clts\n"                                                         \
            "1:" ::"m"(*&__tmp.a),                                           \
            "m"(*&__tmp.b),                                                  \
            "d"(_TSS(n)), "c"((long)task[n]));                               \
}
#else
#define switch_to(n)
#endif

#define PAGE_ALIGN(n) (((n) + 0xfff) & 0xfffff000)

#if ASM_NO_64
#define _set_base(addr, base)                 \
        __asm__("push %%edx\n\t"                  \
                "movw %%dx,%0\n\t"                \
                "rorl $16,%%edx\n\t"              \
                "movb %%dl,%1\n\t"                \
                "movb %%dh,%2\n\t"                \
                "pop %%edx" ::"m"(*((addr) + 2)), \
                "m"(*((addr) + 4)),               \
                "m"(*((addr) + 7)),               \
                "d"(base))

#define _set_limit(addr, limit)         \
        __asm__("push %%edx\n\t"            \
                "movw %%dx,%0\n\t"          \
                "rorl $16,%%edx\n\t"        \
                "movb %1,%%dh\n\t"          \
                "andb $0xf0,%%dh\n\t"       \
                "orb %%dh,%%dl\n\t"         \
                "movb %%dl,%1\n\t"          \
                "pop %%edx" ::"m"(*(addr)), \
                "m"(*((addr) + 6)),         \
                "d"(limit))
#else
#define _set_base(addr, base)
#define _set_limit(addr, limit)
#endif

#define set_base(ldt, base) _set_base(((char *)&(ldt)), (base))
#define set_limit(ldt, limit) _set_limit(((char *)&(ldt)), (limit - 1) >> 12)

/**
#define _get_base(addr) ({\
unsigned long __base; \
__asm__("movb %3,%%dh\n\t" \
	"movb %2,%%dl\n\t" \
	"shll $16,%%edx\n\t" \
	"movw %1,%%dx" \
	:"=d" (__base) \
	:"m" (*((addr)+2)), \
	 "m" (*((addr)+4)), \
	 "m" (*((addr)+7)) \
        :"memory"); \
__base;})
**/

static inline unsigned long _get_base(char *addr)
{
    unsigned long __base;
    __asm__("movb %3,%%dh\n\t"
            "movb %2,%%dl\n\t"
            "shll $16,%%edx\n\t"
            "movw %1,%%dx"
            : "=&d"(__base)
            : "m"(*((addr) + 2)),
            "m"(*((addr) + 4)),
            "m"(*((addr) + 7)));
    return __base;
}

#define get_base(ldt) _get_base(((char *)&(ldt)))

#if ASM_NO_64
#define get_limit(segment) ({ \
    unsigned long __limit; \
    __asm__("lsll %1,%0\n\tincl %0":"=r" (__limit):"r" (segment)); \
    __limit; })
#else
#define get_limit(segment) (0)
#endif

#endif
