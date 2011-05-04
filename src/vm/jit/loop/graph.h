/* src/vm/jit/loop/graph.h - control flow graph header

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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

*/


#ifndef _GRAPH_H
#define _GRAPH_H

#include "config.h"
#include "vm/types.h"

#include "vm/method.hpp"

#include "vm/jit/loop/loop.h"


/* function prototypes ********************************************************/
#ifdef __cplusplus
extern "C" {
#endif

void LoopContainerInit(jitdata *jd, struct LoopContainer *lc, int i);
void depthFirst(jitdata *jd);
void dF(jitdata *jd, loopdata *ld, int from, int blockIndex);
void dF_Exception(jitdata *jd, loopdata *ld, int from, int blockIndex);

void resultPass1(methodinfo *);

#ifdef __cplusplus
}
#endif

#endif /* _GRAPH_H */


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
