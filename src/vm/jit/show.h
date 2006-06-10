/* vm/jit/show.h - showing the intermediate representation

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

   Authors: Andreas Krall

   Changes: Edwin Steiner
            Christian Thalinger
            Christian Ullrich

   $Id$

*/


#ifndef _SHOW_H
#define _SHOW_H

#include "config.h"
#include "vm/types.h"

#include "vm/jit/jit.h"

#define SHOW_INSTRUCTIONS  0
#define SHOW_PARSE         1
#define SHOW_STACK         2
#define SHOW_REGS          3
#define SHOW_CODE          4

#if !defined(NDEBUG)
bool show_init(void);

void new_show_method(jitdata *jd, int stage);
void new_show_basicblock(jitdata *jd, basicblock *bptr, int stage);
void new_show_icmd(jitdata *jd, new_instruction *iptr, bool deadcode, int stage);

void show_method(jitdata *jd);
void show_basicblock(jitdata *jd, basicblock *bptr);
void show_icmd(instruction *iptr, bool deadcode);
#endif /* !defined(NDEBUG) */

#endif /* _SHOW_H */

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
