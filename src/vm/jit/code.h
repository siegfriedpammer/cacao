/* vm/jit/code.h - codeinfo struct for representing compiled code

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Contact: cacao@cacaojvm.org

   Authors: Edwin Steiner

   Changes:

   $Id$

*/


#ifndef _CODE_H
#define _CODE_H

#include "config.h"
#include "vm/types.h"

#include "vm/method.h"
#include "vm/jit/replace.h"

/* A `codeinfo` represents a particular realization of a method in     */
/* machine code.                                                       */

struct codeinfo {
	methodinfo   *m;                /* method this is a realization of */
	codeinfo     *prev;             /* previous codeinfo of this method*/
	
	/* machine code */
	u1           *mcode;            /* pointer to machine code         */
	u1           *entrypoint;       /* machine code entry point        */
	s4            mcodelength;      /* length of generated machine code*/
	bool          isleafmethod;     /* does method call subroutines    */

	/* replacement */
	rplpoint     *rplpoints;        /* replacement points              */
	s2           *regalloc;         /* register allocation info        */
	s4            rplpointcount;    /* number of replacement points    */
	s4            globalcount;      /* number of global allocations    */
	s4            regalloccount;    /* number of total allocations     */

	/* profiling */
	u4            frequency;        /* number of method invocations    */
	u4           *bbfrequency;
	s8            cycles;           /* number of cpu cycles            */
};

codeinfo *code_codeinfo_new(methodinfo *m);
void code_codeinfo_free(codeinfo *code);

void code_free_code_of_method(methodinfo *m);

#endif /* _CODE_H */

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
 * vim:noexpandtab:sw=4:ts=4:
 */
