/* jit/x86_64/methodtable.c - builds a table of all methods

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   Institut f. Computersprachen, TU Wien
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser, M. Probst,
   S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich,
   J. Wenninger

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authores: Christian Thalinger

   $Id: methodtable.c 714 2003-12-07 20:41:24Z twisti $

*/


#include <stdio.h>
#include "methodtable.h"
#include "types.h"
#include "asmpart.h"
#include "toolbox/memory.h"


static mtentry *mtroot = NULL;


void addmethod(u1 *start, u1 *end)
{
  /* boehm makes problems with jvm98 db */
#if 0
    mtentry *mte = GCNEW(mtentry, 1);
#else
    mtentry *mte = NEW(mtentry);
#endif

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

    while (mte != NULL) {
		if (mte->start <= pos && pos <= mte->end) {
			return mte->start;

		} else {
			mte = mte->next;
		}
    }
	
    printf("can't find method with rip=%p\n", pos);
    exit(-1);
}


void asmprintf(long x)
{
	printf("%lx\n", x);
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
