/* i386/methodtable.c **********************************************************

    Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

    See file COPYRIGHT for information on usage and disclaimer of warranties

    Contains the codegenerator for an i386 processor.
    This module generates i386 machine code for a sequence of
    pseudo commands (ICMDs).

    Authors: Christian Thalinger EMAIL: cacao@complang.tuwien.ac.at

    Last Change: $Id: methodtable.c 525 2003-10-22 21:14:23Z twisti $

*******************************************************************************/

#include "methodtable.h"

static mtentry *mtroot = NULL;



void addmethod(u1 *start, u1 *end)
{
  /* boehm makes problems with jvm98 db */
#if 0
    mtentry *mte = GCNEW(mtentry, 1);
#else
    mtentry *mte = NEW(mtentry);
#endif

/*      fprintf(stderr, "start=%lx end=%lx\n", start, end); */

    if (mtroot == NULL) {
#if 0
        mtentry *tmp = GCNEW(mtentry, 1);
#else
        mtentry *tmp = NEW(mtentry);
#endif
	tmp->start = (u1 *) asm_calljavamethod;
	tmp->end = (u1 *) asm_calljavafunction;    /* little hack, but should work */
	tmp->next = mtroot;
	mtroot = tmp;

#if 0
        tmp = GCNEW(mtentry, 1);
#else
        tmp = NEW(mtentry);
#endif
	tmp->start = (u1 *) asm_calljavafunction;
	tmp->end = (u1 *) asm_call_jit_compiler;    /* little hack, but should work */
	tmp->next = mtroot;
	mtroot = tmp;
    }

    mte->start = start;
    mte->end = end;
    mte->next = mtroot;
    mtroot = mte;
}



u1 *findmethod(u1 *pos)
{
    mtentry *mte = mtroot;

/*      printf("findmethod: start\n"); */

    while (mte != NULL) {
/*          printf("%p <= %p <= %p\n", mte->start, pos, mte->end); */
          
	if (mte->start <= pos && pos <= mte->end) {
	    return mte->start;

	} else {
	    mte = mte->next;
	}
    }
	
    return NULL;
}



void asmprintf(int x)
{
    printf("val=%x\n", x);
    fflush(stdout);
}
