/************************** toolbox/tree.c *************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Not documented, see tree.h.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/10/03

*******************************************************************************/

#include <stdio.h>
#include <assert.h>

#include "global.h"
#include "memory.h"
#include "tree.h"


tree *tree_new (treeelementcomperator comperator)
{
	tree *t = NEW (tree);
	
	t -> usedump = 0;
	t -> comperator = comperator;
	t -> top = NULL;
	t -> active = NULL;
		
	return t;
}

tree *tree_dnew (treeelementcomperator comperator)
{
	tree *t = DNEW (tree);
	
	t -> usedump = 1;
	t -> comperator = comperator;
	t -> top = NULL;
	t -> active = NULL;
	
	return t;
}


static void tree_nodefree (treenode *tn)
{
	if (!tn) return;
	tree_nodefree (tn -> left);
	tree_nodefree (tn -> right);
	FREE (tn, treenode);
}

void tree_free (tree *t)
{
	assert (! t -> usedump);
	
	tree_nodefree ( t -> top );
	FREE (t, tree);
}


static treenode *tree_nodeadd 
    (tree *t, treenode *par, treenode *n, void *element, void *key)
{
	if (!n) {
		if ( t-> usedump )  n = DNEW (treenode);
		else                n = NEW (treenode);
		
		n -> left = NULL;
		n -> right = NULL;
		n -> parent = par;
		n -> element = element;
		}
	else {
		if ( t->comperator (key, n -> element) < 0 ) {
			n -> left = tree_nodeadd (t, n, n -> left, element, key);
			}
		else {
			n -> right = tree_nodeadd (t, n, n -> right, element, key);
			}
		}
		
	return n;
}

void tree_add (tree *t, void *element, void *key)
{
	t -> top = tree_nodeadd (t, NULL, t -> top, element, key);
	t -> active = t -> top;
}


static treenode *tree_nodefind (tree *t, treenode *n, void *key)
{
	int way;

	if (!n) return NULL;

	way = t -> comperator (key, n->element);
	if (way==0) return n;
	if (way<0) return tree_nodefind (t, n -> left, key);
	else       return tree_nodefind (t, n -> right, key);
}


void *tree_find (tree *t, void *key)
{
	treenode *tn = tree_nodefind (t, t -> top, key);
	if (!tn) return NULL;

	t -> active = tn;
	return tn -> element;
}



void *tree_this (tree *t)
{	
	if (! t->active) return NULL;
	return t->active->element;
}


static treenode *leftmostnode(treenode *t)
{
	while (t->left) t=t->left;
	return t;
}

void *tree_first (tree *t)
{
	treenode *a = t->top;
	if (!a) return NULL;

	a = leftmostnode (a);
	t->active = a;
	return a->element;
}

void *tree_next (tree *t)
{
	treenode *a = t->active;
	treenode *comefrom = NULL;

	while (a) {
		if (!a) return NULL;

		if (a->left && (a->left == comefrom)) {
			t -> active = a;
			return a->element;
			}
	
		if (a->right && (a->right != comefrom) ) {
			a = leftmostnode(a->right);
			t -> active = a;
			return a->element;
			}
		
		comefrom=a;
		a=a->parent;
		}
	t->active = NULL;
	return NULL;
}
