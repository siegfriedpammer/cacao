/* toolbox/tree.h - binary tree management

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   Institut f. Computersprachen, TU Wien
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser, M. Probst,
   S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich,
   J. Wenninger

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

   Authors: Reinhard Grafl

   $Id: tree.h 684 2003-12-02 16:50:17Z twisti $

*/


#ifndef _TREE_H
#define _TREE_H

typedef int (*treeelementcomperator) (void *key, void * element);


typedef struct treenode {
	struct treenode *left,*right;
	struct treenode *parent;
	
	void *element;
} treenode;

typedef struct {
	int usedump;
	treeelementcomperator comperator;	

	treenode *top;
	treenode *active;
} tree;


/* function prototypes */

tree *tree_new(treeelementcomperator comperator);
tree *tree_dnew(treeelementcomperator comperator);
void tree_free(tree *t);

void tree_add(tree *t, void *element, void *key);
void *tree_find(tree *t, void *key);

void *tree_this(tree *t);
void *tree_first(tree *t);
void *tree_next(tree *t);

#endif /* _TREE_H */


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
