/************************* toolbox/tree.h **************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Binary tree management

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/10/03

*******************************************************************************/

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



tree *tree_new (treeelementcomperator comperator);
tree *tree_dnew (treeelementcomperator comperator);
void tree_free (tree *t);

void tree_add (tree *t, void *element, void *key);
void *tree_find (tree *t, void *key);

void *tree_this (tree *t);
void *tree_first (tree *t);
void *tree_next (tree *t);

