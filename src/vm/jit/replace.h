/* vm/jit/replace.h - on-stack replacement of methods

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


#ifndef _REPLACE_H
#define _REPLACE_H

#include "config.h"
#include "vm/types.h"
#include "vm/method.h"

/* An `rplpoint` represents a replacement point in a compiled method  */

struct rplpoint {
	u1       *pc;           /* machine code PC of this point  */
	u1       *outcode;      /* pointer to replacement-out code*/
	rplpoint *hashlink;     /* chain to next rplpoint in hash */ /* XXX needed? */
	codeinfo *code;         /* codeinfo this point belongs to */
	rplpoint *target;       /* target of the replacement      */
	s2       *regalloc;     /* pointer to register index table*/
	s2        regalloccount;/* number of local allocations    */
};

/* An `executionsstate` represents the state of a thread as it reached */
/* an replacement point or is about to enter one.                      */

#define MD_EXCSTATE_NREGS  32
#define MD_EXCSTATE_NCALLEESAVED  8

struct executionstate {
	u1           *pc;
	u8            regs[MD_EXCSTATE_NREGS];
	u8            savedregs[MD_EXCSTATE_NCALLEESAVED]; /* or read from frame */

	u1           *frame;
	
    java_objectheader *locked; /* XXX maybe just leave it in frame? */
};

/* `sourcestate` will probably only be used for debugging              */

struct sourcestate {
	u8           *javastack;
	s4            javastackdepth;

	u8            javalocals;
	s4            javalocalscount;
};

/*** prototypes ********************************************************/

bool replace_create_replacement_points(codeinfo *code,registerdata *rd);
void replace_free_replacement_points(codeinfo *code);

void replace_activate_replacement_point(rplpoint *rp,rplpoint *target);
void replace_activate(codeinfo *code,codeinfo *target);

void replace_me(rplpoint *rp,executionstate *es);

#ifndef NDEBUG
void replace_show_replacement_points(codeinfo *code);
void replace_replacement_point_println(rplpoint *rp);
void replace_executionstate_println(executionstate *es);
#endif

/* machine dependent functions (code in ARCH_DIR/md.c) */

#if defined(ENABLE_JIT)
void md_patch_replacement_point(rplpoint *rp);
#endif

#endif

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
