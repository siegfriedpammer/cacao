/* jit/loop/tracing.h - trace functions header

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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

   Authors: Christian Thalinger

   $Id: tracing.h 557 2003-11-02 22:51:59Z twisti $

*/


#ifndef _TRACING_H
#define _TRACING_H

#include "jit.h"

/*	
   This struct is needed to record the source of operands of intermediate code
   instructions. The instructions are scanned backwards and the stack is 
   analyzed in order to determine the type of operand.
*/
struct Trace {
	int type;                  /* the type of the operand						*/
	int neg;                   /* set if negated								*/
	int var;                   /* variable reference	for IVAR				*/
			                   /* array reference		for AVAR/ARRAY			*/
	int nr;                    /* instruction number in the basic block, where */
			                   /* the trace is defined							*/
	int constant;              /* constant value		for ICONST				*/
				               /* modifiers			for IVAR				*/
};



/* function protoypes */
struct Trace* create_trace(int type, int var, int constant, int nr);
struct Trace* add(struct Trace* a, struct Trace* b);
struct Trace* negate(struct Trace* a);
struct Trace* sub(struct Trace* a, struct Trace* b);
struct Trace* array_length(struct Trace* a);
struct Trace* tracing(basicblock *block, int index, int temp);

#endif /* _TRACING_H */


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
