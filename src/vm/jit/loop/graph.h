/* jit/loop/graph.h - control flow graph header

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

   $Id: graph.h 1203 2004-06-22 23:14:55Z twisti $

*/


#ifndef _GRAPH_H
#define _GRAPH_H

#include "loop.h"

void LoopContainerInit(methodinfo *m, struct LoopContainer *lc, int i);
void depthFirst(methodinfo *m);
void dF(methodinfo *m, int from, int blockIndex);
void dF_Exception(methodinfo *m, int from, int blockIndex);

void resultPass1();

#endif /* _GRAPH_H */

