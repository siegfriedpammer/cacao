/* src/vm/jit/sparc64/md-asm.h - assembler defines for Sparc ABI

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

   Authors: Christian Thalinger
            Alexander Jordan

   Changes:

   $Id: md-asm.h 4498 2006-02-12 23:43:09Z twisti $

*/

#ifndef _MD_ASM_H
#define _MD_ASM_H


/* register defines
 * ***********************************************************/

#define zero	%g0
#define itmp1	%g1
#define itmp2	%g2
#define itmp3	%g3

#define mptr_itmp2 itmp2

#define xptr_itmp2 itmp2
#define xpc_itmp3  itmp3

#define ra_caller  %o7
#define ra_callee  %i7

#define pv_caller  %o5
#define pv_callee  %i5

#endif /* _MD_ASM_H */


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 *
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */

