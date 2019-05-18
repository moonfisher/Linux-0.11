#ifndef _HEAD_H
#define _HEAD_H

#if DEBUG
    #define ASM_NO_64    0
#else
    #define ASM_NO_64    1
#endif

typedef struct desc_struct
{
    unsigned long a;
    unsigned long b;
} desc_table[256];

#if ASM_NO_64
    extern unsigned long pg_dir[1024];
    extern desc_table idt, gdt;
#else
    unsigned long pg_dir[1024];
    desc_table idt, gdt;
#endif

#define GDT_NUL     0
#define GDT_CODE    1
#define GDT_DATA    2
#define GDT_TMP     3

#define LDT_NUL     0
#define LDT_CODE    1
#define LDT_DATA    2

#endif
