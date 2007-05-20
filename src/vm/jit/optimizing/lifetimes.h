/* src/vm/jit/lsra/graph.h - lifetimes header

   Copyright (C) 2005, 2006 R. Grafl, A. Krall, C. Kruegel, C. Oates,
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

   Authors: Christian Ullrich

   $Id: lifetime.h$

*/


#ifndef _LIFETIMES_H
#define _LIFETIMES_H

#include "vm/jit/optimizing/dominators.h"

#if !defined(NDEBUG)
# include <assert.h>
# define LT_DEBUG_CHECK
/* # define LT_DEBUG_VERBOSE */
#endif

#ifdef LT_DEBUG_CHECK
# define _LT_CHECK_BOUNDS(i,l,h) assert( ((i) >= (l)) && ((i) < (h)));
# define _LT_ASSERT(a) assert((a));
#else
# define _LT_CHECK_BOUNDS(i,l,h)
# define _LT_ASSERT(a)
#endif

#define LT_BB_IN 3
#define LT_BB_OUT 2
#define LT_DEF 1
#define LT_USE 0

typedef struct site *lt_iterator;
void lt_scanlifetimes(jitdata *, graphdata *, dominatordata *);
void lt_add_ss(struct lifetime *, stackptr );
void lt_remove_use_site(struct lifetime *lt, int block, int iindex);
void lt_move_use_sites(struct lifetime *from, struct lifetime *to);
void lt_lifeness_analysis(jitdata *, graphdata *);
#endif /* _LIFETIMES_H */

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
