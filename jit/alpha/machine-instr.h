#ifndef _MACHINE_INSTR_H
#define _MACHINE_INSTR_H

static inline long
__attribute__ ((unused))
atomic_swap (volatile long *p, long val)
{
  long ret, t1;

  __asm__ __volatile__ (
    "/* Inline atomic swap */\n"
	"1:\t"
	"ldq_l	%2,%4\n\t"
	"mov	%3,%0\n\t"
	"stq_c	%0,%1\n\t"
	"beq	%0,2f\n\t"
    ".subsection 1\n"
    "2:\t"
    "br 1b\n"
    ".previous\n\t"
    "3:\t"
    "mb\n\t"
    "/* End atomic swap */"
	: "=&r"(t1), "=m"(*p), "=&r"(ret)
	: "r"(val), "m"(*p));

  return ret;
}

static inline long
__attribute__ ((unused))
compare_and_swap (volatile long *p, long oldval, long newval)
{
  long ret;

  __asm__ __volatile__ (
    "/* Inline compare & swap */\n"
    "1:\t"
    "ldq_l  %0,%4\n\t"
    "cmpeq  %0,%2,%0\n\t"
    "beq    %0,3f\n\t"
    "mov    %3,%0\n\t"
    "stq_c  %0,%1\n\t"
    "beq    %0,2f\n\t"
    ".subsection 1\n"
    "2:\t"
    "br 1b\n"
    ".previous\n\t"
    "3:\t"
    "mb\n\t"
    "/* End compare & swap */"
	: "=&r"(ret), "=m"(*p)
	: "r"(oldval), "r"(newval), "m"(*p));

  return ret;
}

#endif
