/* src/vm/jit/intrp/md.c - machine dependent Interpreter functions

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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

   Authors: Andreas Krall
            Reinhard Grafl

   Changes: Joseph Wenninger
            Christian Thalinger

   $Id: md.c 3895 2005-12-07 16:03:37Z anton $

*/


#include <assert.h>
#include <stdlib.h>
#include <signal.h>

#include "config.h"
#include "vm/types.h"

#include "vm/jit/intrp/intrp.h"


Inst *vm_prim = NULL; /* initialized by md_init() */
FILE *vm_out = NULL;  /* debugging output for vmgenerated stuff */

/* md_init *********************************************************************

   Do some machine dependent initialization.

*******************************************************************************/

void md_init(void)
{
	vm_out = stdout;
    if (setvbuf(stdout,NULL, _IOLBF,0) != 0) {
		perror("setvbuf error");
		exit(1);
	}
	if ( vm_prim == NULL ) {
		vm_prim = (Inst *)engine(NULL, NULL, NULL);
	}
	if (peeptable == 0) {
		init_peeptable();
	}
	check_prims(vm_prim);
}


#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void thread_restartcriticalsection(ucontext_t *uc)
{
	assert(false);
}
#endif


/* md_stacktrace_get_returnaddress *********************************************

   Returns the return address of the current stackframe, specified by
   the passed stack pointer and the stack frame size.

*******************************************************************************/

u1 *md_stacktrace_get_returnaddress(u1 *sp, u4 framesize)
{
	u1 *ra;

	/* ATTENTION: the passed sp is actually the fp! (see java.vmg for stack 
	   layout) */

	ra = *((u1 **) (sp - framesize - sizeof(void *)));

	return ra;
}


/* XXX remove me!!! */
void md_param_alloc(methoddesc *md)
{
}

void md_return_alloc(methodinfo *m, registerdata *rd, s4 return_type,
					 stackptr stackslot)
{
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
