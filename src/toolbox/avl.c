/* src/toolbox/avl.c - AVL tree implementation

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

   Changes:

   $Id: avl.c 4908 2006-05-12 16:49:50Z edwin $

*/


#include "config.h"

#include <assert.h>

#include "vm/types.h"

#include "mm/memory.h"
#include "toolbox/avl.h"

#if defined(USE_THREADS)
# if defined(NATIVE_THREADS)
#  include "threads/native/threads.h"
# else
#  include "threads/green/threads.h"
# endif
#endif

#include "vm/builtin.h"


/* avl_create ******************************************************************

   Creates a AVL tree structure.

*******************************************************************************/

avl_tree *avl_create(avl_comparator *compar)
{
	avl_tree *t;

	t = NEW(avl_tree);

	t->root       = NULL;
	t->comparator = compar;
	t->entries    = 0;

#if defined(USE_THREADS)
	/* create lock object for this tree */

	t->lock       = NEW(java_objectheader);

# if defined(NATIVE_THREADS)
	lock_init_object_lock(t->lock);
# endif
#endif

	return t;
}


/* avl_newnode *****************************************************************

   Creates a new AVL node and sets the pointers correctly.

*******************************************************************************/

static avl_node *avl_newnode(void *data)
{
	avl_node *n;

	n = NEW(avl_node);

	n->data      = data;

	/* ATTENTION: NEW allocates memory zeroed out */

/* 	n->balance   = 0; */
/* 	n->childs[0] = NULL; */
/* 	n->childs[1] = NULL; */

	return n;
}


/* avl_rotate_left *************************************************************

   Does a left rotation on an AVL node.

   A (node)         B
    \              / \
     B       -->  A   C
      \
       C

*******************************************************************************/

static void avl_rotate_left(avl_node **node)
{
	avl_node *tmp;
	avl_node *tmpnode;

	/* rotate the node */

	tmp                       = *node;
	tmpnode                   = tmp->childs[AVL_RIGHT];
	tmp->childs[AVL_RIGHT]    = tmpnode->childs[AVL_LEFT];
	tmpnode->childs[AVL_LEFT] = tmp;

	/* set new parent node */

	*node                     = tmpnode;
}


/* avl_rotate_right ************************************************************

   Does a right rotation on an AVL node.

       C (node)         B
      /                / \
     B           -->  A   C
    /
   A

*******************************************************************************/

static void avl_rotate_right(avl_node **node)
{
	avl_node *tmp;
	avl_node *tmpnode;

	/* rotate the node */

	tmp                        = *node;
	tmpnode                    = tmp->childs[AVL_LEFT];
	tmp->childs[AVL_LEFT]      = tmpnode->childs[AVL_RIGHT];
	tmpnode->childs[AVL_RIGHT] = tmp;

	/* set new parent node */

	*node                      = tmpnode;
}


/* avl_adjust_balance **********************************************************

   Does a balance adjustment after a double rotation.

*******************************************************************************/

static void avl_adjust_balance(avl_node *node)
{
	avl_node *left;
	avl_node *right;

	left  = node->childs[AVL_LEFT];
	right = node->childs[AVL_RIGHT];

	switch (node->balance) {
	case -1:
		left->balance  = 0;
		right->balance = 1;
		break;

	case 0:
		left->balance  = 0;
		right->balance = 0;
		break;

	case 1:
		left->balance  = -1;
		right->balance = 0;
		break;
	}

	node->balance = 0;
}


/* avl_insert_intern ***********************************************************

   Inserts a AVL node into a AVL tree.

*******************************************************************************/

static s4 avl_insert_intern(avl_tree *tree, avl_node **node, void *data)
{
	avl_node *tmpnode;
	s4        res;
	s4        direction;
	s4        insert;
	s4        balance;

	/* set temporary variable */

	tmpnode = *node;

	/* per default no node was inserted */

	insert = 0;

	/* compare the current node */

	res = tree->comparator(data, tmpnode->data);

	/* is this node already in the tree? */

	if (res == 0)
		assert(0);

	/* goto left or right child */

	direction = (res < 0) ? AVL_LEFT : AVL_RIGHT;

	/* there is a child, go recursive */

	if (tmpnode->childs[direction]) {
		balance = avl_insert_intern(tree, &tmpnode->childs[direction], data);

	} else {
		avl_node *newnode;

		/* no child, create this node */

		newnode = avl_newnode(data);

		/* insert into parent node */

		tmpnode->childs[direction] = newnode;

		/* node was inserted, but don't set insert to 1, since this
		   insertion is handled in this recursion depth */

		balance = 1;
	}

	/* add insertion value to node balance, value depends on the
	   direction */

	tmpnode->balance += (direction == AVL_LEFT) ? -balance : balance;

	if ((balance != 0) && (tmpnode->balance != 0)) {
		if (tmpnode->balance < -1) {
			/* left subtree too tall: right rotation needed */

			if (tmpnode->childs[AVL_LEFT]->balance < 0) {
				avl_rotate_right(&tmpnode);

				/* simple balance adjustments */

				tmpnode->balance = 0;
				tmpnode->childs[AVL_RIGHT]->balance = 0;

			} else {
				avl_rotate_left(&tmpnode->childs[AVL_LEFT]);
				avl_rotate_right(&tmpnode);
				avl_adjust_balance(tmpnode);
			}

		} else if (tmpnode->balance > 1) {
			/* right subtree too tall: left rotation needed */

			if (tmpnode->childs[AVL_RIGHT]->balance > 0) {
				avl_rotate_left(&tmpnode);

				/* simple balance adjustments */

				tmpnode->balance = 0;
				tmpnode->childs[AVL_LEFT]->balance = 0;

			} else {
				avl_rotate_right(&tmpnode->childs[AVL_RIGHT]);
				avl_rotate_left(&tmpnode);
				avl_adjust_balance(tmpnode);
			}

		} else {
			insert = 1;
		}
	}

	/* set back node */

	*node = tmpnode;

	/* insertion was ok */

	return insert;
}


/* avl_insert ******************************************************************

   Inserts a AVL node into a AVL tree.

*******************************************************************************/

bool avl_insert(avl_tree *tree, void *data)
{
	assert(tree);
	assert(data);

#if defined(USE_THREADS)
	builtin_monitorenter(tree->lock);
#endif

	/* if we don't have a root node, create one */

	if (tree->root == NULL) {
		tree->root = avl_newnode(data);

	} else {
		avl_insert_intern(tree, &tree->root, data);
	}

	/* increase entries count */

	tree->entries++;

#if 0
	printf("tree=%p entries=%d\n", tree, tree->entries);

	printf("-------------------\n");
	avl_dump(tree->root, 0);
	printf("-------------------\n");
#endif

#if defined(USE_THREADS)
	builtin_monitorexit(tree->lock);
#endif

	/* insertion was ok */

	return true;
}


/* avl_find ********************************************************************

   Find a given data structure in the AVL tree, with the comparision
   function of the tree.

*******************************************************************************/

void *avl_find(avl_tree *tree, void *data)
{
	avl_node *node;
	s4        res;

	assert(tree);
	assert(data);

#if defined(USE_THREADS)
	builtin_monitorenter(tree->lock);
#endif

	/* search the tree for the given node */

	for (node = tree->root; node != NULL; ) {
		/* compare the current node */

		res = tree->comparator(data, node->data);

		/* was the entry found? return it */

		if (res == 0) {
#if defined(USE_THREADS)
			builtin_monitorexit(tree->lock);
#endif

			return node->data;
		}

		/* goto left or right child */

		node = node->childs[(res < 0) ? AVL_LEFT : AVL_RIGHT];
	}

#if defined(USE_THREADS)
	builtin_monitorexit(tree->lock);
#endif

	/* entry was not found, returning NULL */

	return NULL;
}


/* avl_dump ********************************************************************

   Dumps the AVL tree starting with node.

*******************************************************************************/

#if !defined(NDEBUG)
void avl_dump(avl_node* node, s4 indent)
{
	s4 tmp;

	tmp = indent;

	if (node == NULL)
			return;

	if (node->childs[AVL_RIGHT])
		avl_dump(node->childs[AVL_RIGHT], tmp + 1);

	while(indent--)
		printf("   ");

	printf("%p (%d)\n", node->data, node->balance);

	if (node->childs[AVL_LEFT])
		avl_dump(node->childs[AVL_LEFT], tmp + 1);
}
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
 */
