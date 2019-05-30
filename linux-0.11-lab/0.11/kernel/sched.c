/*
 *  linux/kernel/sched.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * 'sched.c' is the main kernel file. It contains scheduling primitives
 * (sleep_on, wakeup, schedule etc) as well as a number of simple system
 * call functions (type getpid(), which just extracts a field from
 * current-task
 */
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/sys.h>
#include <linux/fdreg.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>
#include <signal.h>
#include <string.h>

#define _S(nr) (1 << ((nr)-1))
#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))

void show_task(int nr, struct task_struct *p)
{
	int i, j = 4096 - sizeof(struct task_struct);

	printk("%d: pid=%d, state=%d, ", nr, p->pid, p->state);
	i = 0;
	while (i < j && !((char *)(p + 1))[i])
		i++;
	printk("%d (of %d) chars free in kernel stack\n\r", i, j);
}

void show_stat(void)
{
	int i;

	for (i = 0; i < NR_TASKS; i++)
		if (task[i])
			show_task(i, task[i]);
}

#define LATCH (1193180 / HZ)

#if ASM_NO_64
    extern void mem_use(void);
    extern int timer_interrupt(void);
    extern int system_call(void);
#else
    void mem_use(void){};
    int timer_interrupt(void){return 0;};
    int system_call(void){return 0;};
#endif

union task_union {
	struct task_struct task;
	char stack[PAGE_SIZE];
};

static union task_union init_task;

void init_task_f(struct task_struct *init_task_t)
{
    init_task_t->state = 0;
    init_task_t->counter = 15;
    init_task_t->priority = 15;
    init_task_t->signal = 0;
    memset(init_task_t->sigaction, 0, 32 * sizeof(struct sigaction));
    init_task_t->blocked = 0;

    init_task_t->exit_code = 0;
    init_task_t->start_code = 0;
    init_task_t->end_code = 0;
    init_task_t->end_data = 0;
    init_task_t->brk = 0;
    init_task_t->start_stack = 0;
    
    init_task_t->pid = 0;
    init_task_t->father = -1;
    init_task_t->pgrp = 0;
    init_task_t->session = 0;
    init_task_t->leader = 0;
    
    init_task_t->uid = 0;
    init_task_t->euid = 0;
    init_task_t->suid = 0;
    init_task_t->gid = 0;
    init_task_t->egid = 0;
    init_task_t->sgid = 0;
    
    init_task_t->alarm = 0;
    init_task_t->utime = 0;
    init_task_t->stime = 0;
    init_task_t->cutime = 0;
    init_task_t->cstime = 0;
    init_task_t->start_time = 0;
    init_task_t->used_math = 0;

    init_task_t->tty = -1;
    init_task_t->umask = 0022;
    
    init_task_t->pwd = NULL;
    init_task_t->root = NULL;
    init_task_t->executable = NULL;
    init_task_t->close_on_exec = 0;
    memset(init_task_t->filp, 0, NR_OPEN * sizeof(struct file *));

    init_task_t->ldt[0].a = 0;
    init_task_t->ldt[0].b = 0;
    init_task_t->ldt[1].a = 0x9f;
    init_task_t->ldt[1].b = 0xc0fa00;
    init_task_t->ldt[2].a = 0x9f;
    init_task_t->ldt[2].b = 0xc0f200;

    init_task_t->tss.back_link = 0;
    init_task_t->tss.esp0 = PAGE_SIZE + (long)&init_task;
    init_task_t->tss.ss0 = 0x10;
    init_task_t->tss.esp1 = 0;
    init_task_t->tss.ss1 = 0;
    init_task_t->tss.esp2 = 0;
    init_task_t->tss.ss2 = 0;
    init_task_t->tss.cr3 = (long)&pg_dir;
    init_task_t->tss.eip = 0;
    init_task_t->tss.eflags = 0;
    init_task_t->tss.eax = 0;
    init_task_t->tss.ecx = 0;
    init_task_t->tss.edx = 0;
    init_task_t->tss.ebx = 0;
    init_task_t->tss.esp = 0;
    init_task_t->tss.ebp = 0;
    init_task_t->tss.esi = 0;
    init_task_t->tss.edi = 0;
    init_task_t->tss.es = 0x17;
    init_task_t->tss.cs = 0x17;
    init_task_t->tss.ss = 0x17;
    init_task_t->tss.ds = 0x17;
    init_task_t->tss.fs = 0x17;
    init_task_t->tss.gs = 0x17;
    init_task_t->tss.ldt = _LDT(0);
    init_task_t->tss.trace_bitmap = 0x80000000;
    init_task_t->tss.i387.cwd = 0;
    init_task_t->tss.i387.swd = 0;
    init_task_t->tss.i387.twd = 0;
    init_task_t->tss.i387.fip = 0;
    init_task_t->tss.i387.fcs = 0;
    init_task_t->tss.i387.foo = 0;
    init_task_t->tss.i387.fos = 0;
    memset(init_task_t->tss.i387.st_space, 0, 20);
}

long volatile jiffies = 0;
long startup_time = 0;
struct task_struct *current = &(init_task.task);
struct task_struct *last_task_used_math = NULL;

struct task_struct *task[NR_TASKS] = {
	&(init_task.task),
};

long user_stack[PAGE_SIZE >> 2];

struct
{
	long *a;
	short b;
} stack_start = {&user_stack[PAGE_SIZE >> 2], 0x10};
/*
 *  'math_state_restore()' saves the current math information in the
 * old math state array, and gets the new ones from the current task
 */
void math_state_restore()
{
	if (last_task_used_math == current)
		return;
	__asm__("fwait");
	if (last_task_used_math)
	{
		__asm__("fnsave %0" ::"m"(last_task_used_math->tss.i387));
	}
	last_task_used_math = current;
	if (current->used_math)
	{
		__asm__("frstor %0" ::"m"(current->tss.i387));
	}
	else
	{
		__asm__("fninit" ::);
		current->used_math = 1;
	}
}

/*
 *  'schedule()' is the scheduler function. This is GOOD CODE! There
 * probably won't be any reason to change this, as it should work well
 * in all circumstances (ie gives IO-bound processes good response etc).
 * The one thing you might take a look at is the signal-handler code here.
 *
 *   NOTE!!  Task 0 is the 'idle' task, which gets called when no other
 * tasks can run. It can not be killed, and it cannot sleep. The 'state'
 * information in task[0] is never used.
 */
void schedule(void)
{
	int i, next, c;
	struct task_struct **p;

	/* check alarm, wake up any interruptible tasks that have got a signal */

	for (p = &LAST_TASK; p > &FIRST_TASK; --p)
    {
		if (*p)
		{
			if ((*p)->alarm && (*p)->alarm < jiffies)
			{
				(*p)->signal |= (1 << (SIGALRM - 1));
				(*p)->alarm = 0;
			}
			if (((*p)->signal & (_BLOCKABLE & ~(*p)->blocked))
                && (*p)->state == TASK_INTERRUPTIBLE)
            {
				(*p)->state = TASK_RUNNING;
            }
		}
    }

	/* this is the scheduler proper: */

	while (1)
	{
		c = -1;
		next = 0;
		i = NR_TASKS;
		p = &task[NR_TASKS];
		while (--i)
		{
			if (!*--p)
				continue;
			if ((*p)->state == TASK_RUNNING && (*p)->counter > c)
            {
                c = (int)((*p)->counter);
                next = i;
            }
		}
		if (c)
			break;
		for (p = &LAST_TASK; p > &FIRST_TASK; --p)
        {
			if (*p)
            {
				(*p)->counter = ((*p)->counter >> 1) + (*p)->priority;
            }
        }
	}
	switch_to(next);
}

int sys_pause(void)
{
	current->state = TASK_INTERRUPTIBLE;
	schedule();
	return 0;
}

void sleep_on(struct task_struct **p)
{
	struct task_struct *tmp;

	if (!p)
		return;
	if (current == &(init_task.task))
		panic("task[0] trying to sleep");
	tmp = *p;
	*p = current;
	current->state = TASK_UNINTERRUPTIBLE;
	schedule();
	*p = tmp;
	if (tmp)
		tmp->state = TASK_RUNNING;
}

void interruptible_sleep_on(struct task_struct **p)
{
	struct task_struct *tmp;

	if (!p)
		return;
	if (current == &(init_task.task))
		panic("task[0] trying to sleep");
	tmp = *p;
	*p = current;
repeat:
	current->state = TASK_INTERRUPTIBLE;
	schedule();
	if (*p && *p != current)
	{
		(*p)->state = TASK_RUNNING;
		goto repeat;
	}
	*p = tmp;
	if (tmp)
		tmp->state = TASK_RUNNING;
}

void wake_up(struct task_struct **p)
{
	if (p && *p)
	{
		(*p)->state = TASK_RUNNING;
		*p = NULL;
	}
}

/*
 * OK, here are some floppy things that shouldn't be in the kernel
 * proper. They are here because the floppy needs a timer, and this
 * was the easiest way of doing it.
 */
static struct task_struct *wait_motor[4] = {NULL, NULL, NULL, NULL};
static int mon_timer[4] = {0, 0, 0, 0};
static int moff_timer[4] = {0, 0, 0, 0};
unsigned char current_DOR = 0x0C;

int ticks_to_floppy_on(unsigned int nr)
{
	extern unsigned char selected;
	unsigned char mask = 0x10 << nr;

	if (nr > 3)
		panic("floppy_on: nr>3");
	moff_timer[nr] = 10000; /* 100 s = very big :-) */
	cli();					/* use floppy_off to turn it off */
	mask |= current_DOR;
	if (!selected)
	{
		mask &= 0xFC;
		mask |= nr;
	}
	if (mask != current_DOR)
	{
		outb(mask, FD_DOR);
		if ((mask ^ current_DOR) & 0xf0)
			mon_timer[nr] = HZ / 2;
		else if (mon_timer[nr] < 2)
			mon_timer[nr] = 2;
		current_DOR = mask;
	}
	sti();
	return mon_timer[nr];
}

void floppy_on(unsigned int nr)
{
	cli();
	while (ticks_to_floppy_on(nr))
		sleep_on(nr + wait_motor);
	sti();
}

void floppy_off(unsigned int nr)
{
	moff_timer[nr] = 3 * HZ;
}

void do_floppy_timer(void)
{
	int i;
	unsigned char mask = 0x10;

	for (i = 0; i < 4; i++, mask <<= 1)
	{
		if (!(mask & current_DOR))
			continue;
		if (mon_timer[i])
		{
			if (!--mon_timer[i])
				wake_up(i + wait_motor);
		}
		else if (!moff_timer[i])
		{
			current_DOR &= ~mask;
			outb(current_DOR, FD_DOR);
		}
		else
			moff_timer[i]--;
	}
}

#define TIME_REQUESTS 64

static struct timer_list
{
	long jiffies;
	void (*fn)();
	struct timer_list *next;
} timer_list[TIME_REQUESTS], *next_timer = NULL;

void add_timer(long jiffies, void (*fn)(void))
{
	struct timer_list *p;

	if (!fn)
		return;
	cli();
	if (jiffies <= 0)
		(fn)();
	else
	{
		for (p = timer_list; p < timer_list + TIME_REQUESTS; p++)
			if (!p->fn)
				break;
		if (p >= timer_list + TIME_REQUESTS)
			panic("No more time requests free");
		p->fn = fn;
		p->jiffies = jiffies;
		p->next = next_timer;
		next_timer = p;
		while (p->next && p->next->jiffies < p->jiffies)
		{
			p->jiffies -= p->next->jiffies;
			fn = p->fn;
			p->fn = p->next->fn;
			p->next->fn = fn;
			jiffies = p->jiffies;
			p->jiffies = p->next->jiffies;
			p->next->jiffies = jiffies;
			p = p->next;
		}
	}
	sti();
}

void do_timer(long cpl)
{
	extern int beepcount;
	extern void sysbeepstop(void);

	if (beepcount)
		if (!--beepcount)
			sysbeepstop();

	if (cpl)
		current->utime++;
	else
		current->stime++;

	if (next_timer)
	{
		next_timer->jiffies--;
		while (next_timer && next_timer->jiffies <= 0)
		{
			void (*fn)(void);

			fn = next_timer->fn;
			next_timer->fn = NULL;
			next_timer = next_timer->next;
			(fn)();
		}
	}
	if (current_DOR & 0xf0)
		do_floppy_timer();
	if ((--current->counter) > 0)
		return;
	current->counter = 0;
	if (!cpl)
		return;
	schedule();
}

int sys_alarm(long seconds)
{
	long old = current->alarm;

	if (old)
		old = (old - jiffies) / HZ;
	current->alarm = (seconds > 0) ? (jiffies + HZ * seconds) : 0;
	return (int)(old);
}

int sys_getpid(void)
{
	return (int)(current->pid);
}

int sys_getppid(void)
{
	return (int)(current->father);
}

int sys_getuid(void)
{
	return current->uid;
}

int sys_geteuid(void)
{
	return current->euid;
}

int sys_getgid(void)
{
	return current->gid;
}

int sys_getegid(void)
{
	return current->egid;
}

int sys_nice(long increment)
{
	if (current->priority - increment > 0)
		current->priority -= increment;
	return 0;
}

void sched_init(void)
{
	int i;
	struct desc_struct *p;

	if (sizeof(struct sigaction) != 16)
		panic("Struct sigaction MUST be 16 bytes");
    
    init_task_f(&(init_task.task));
	set_tss_desc(gdt + FIRST_TSS_ENTRY, &(init_task.task.tss));
	set_ldt_desc(gdt + FIRST_LDT_ENTRY, &(init_task.task.ldt));
	p = gdt + 2 + FIRST_TSS_ENTRY;
	for (i = 1; i < NR_TASKS; i++)
	{
		task[i] = NULL;
		p->a = p->b = 0;
		p++;
		p->a = p->b = 0;
		p++;
	}
	/* Clear NT, so that we won't have troubles with that later on */
    /* 清除标志寄存器中的位NT，这样以后就不会有麻烦 */
    // NT 标志用于控制程序的递归调用(Nested Task)。当 NT 置位时，那么当前中断任务执行
    // iret 指令时就会引起任务切换。NT 指出 TSS 中的 back_link 字段是否有效。
#if ASM_NO_64
	__asm__("pushfl ; andl $0xffffbfff,(%esp) ; popfl");
#endif
    // 注意！！是将 GDT 中相应 LDT 描述符的选择符加载到 ldtr。只明确加载这一次，以后新任务
    // LDT 的加载，是 CPU 根据 TSS 中的 LDT 项自动加载。
	ltr(0);
	lldt(0);
	outb_p(0x36, 0x43);			/* binary, mode 3, LSB/MSB, ch 0 */
	outb_p(LATCH & 0xff, 0x40); /* LSB */
	outb(LATCH >> 8, 0x40);		/* MSB */
	set_intr_gate(0x20, &timer_interrupt);
	outb(inb_p(0x21) & ~0x01, 0x21);
	set_system_gate(0x80, &system_call);
}
