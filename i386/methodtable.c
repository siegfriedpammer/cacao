#include "methodtable.h"

static mtentry *mtroot = NULL;

void addmethod(u1 *start, u1 *end)
{
    mtentry *mte = GCNEW(mtentry, 1);

/*      printf("start=%lx end=%lx\n", start, end); */

    if (mtroot == NULL) {
	mtentry *tmp = GCNEW(mtentry, 1);
	tmp->start = (u1 *) asm_calljavamethod;
	tmp->end = (u1 *) asm_calljavafunction;    /* little hack, but should work */
	tmp->next = mtroot;
	mtroot = tmp;

	tmp = GCNEW(mtentry, 1);
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

/*      printf("findentry: start\n"); */

    while (mte != NULL) {
/*  	printf("%lx <= %lx <= %lx, %lx\n", mte->start, pos, mte->end, mte->next); */

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
    printf("val=%lx\n", x);
    fflush(stdout);
}
