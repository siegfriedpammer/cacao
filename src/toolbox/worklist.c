/* src/toolbox/worklist.c - worklist implementation

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

   $Id: worklist.c$

*/

#include "mm/memory.h"
#include "toolbox/worklist.h"


/******************************************************************************

Worklist Implementation


******************************************************************************/

worklist *wl_new(int size) {
	worklist *w;

	w = DNEW(worklist);
	w->W_stack = DMNEW(int, size);
	w->W_top = 0;
	w->W_bv = bv_new(size);
#ifdef WL_DEBUG_CHECK
	w->size = size;
#endif
	return w;
}

void wl_add(worklist *w, int element) {
	_WL_CHECK_BOUNDS(element, 0, w->size);
	if (!bv_get_bit(w->W_bv, element)) {
		_BV_ASSERT((w->W_top < w->size));
		w->W_stack[(w->W_top)++] = element;
		bv_set_bit(w->W_bv, element);
	}
}

int wl_get(worklist *w) {
	int element;

	_WL_ASSERT((w->W_top > 0));
	element = w->W_stack[--(w->W_top)];
	bv_reset_bit(w->W_bv, element);
	return element;
}

bool wl_is_empty(worklist *w) {
	return (w->W_top == 0);
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
