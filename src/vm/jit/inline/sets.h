/* jit/sets.h -

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

   Authors: Carolyn Oates

   $Id: sets.h 557 2003-11-02 22:51:59Z twisti $

*/


#ifndef _SET_H
#define _SET_H

#include "types.h"

typedef struct methSet      methSet;
typedef struct methSetNode  methSetNode;
typedef struct fldSet       fldSet;
typedef struct fldSetNode   fldSetNode;
typedef struct classSet     classSet;
typedef struct classSetNode classSetNode;


#include "global.h"


/*------------ Method /Class Used Markers -------------------------------*/                 

/* Class flags =
   USED all methods and fields are available; 
   PARTUSED = specific methods (static, <init>, <clinit>, inherited def used, special) used, 
	      but not instanciated
   NOTUSED = nothing used in class - not needed 
*/

/* Method Flags =
   USED = method definition is used
   PARTUSED = method definition will be used if class instanciated
   NOTUSED  = method defintion never used
*/

#define USED      2
#define PARTUSED  1
#define MARKED    1
#define NOTUSED   0

#define MONO      0
#define MONO1	  1 /* potential poly that is really mono */
#define POLY      2


/*------------------------------------------------------------*/
/*-- flds used by a method set fns */
/*------------------------------------------------------------*/
struct fldSet {
	fldSetNode *head;
	fldSetNode *tail;
	fldSetNode *pos;
	s4 length;
};


struct fldSetNode {
	fieldinfo *fldRef;
	fldSetNode *nextfldRef;
	bool writePUT;
	bool readGET;
	classSetNode *lastptrPUT; 
	classSetNode *lastptrGET; 
	s2 index;
};


fldSetNode *inFldSet (fldSetNode *, fieldinfo *);
fldSetNode *addFldRef(fldSetNode *, fieldinfo *);
fldSet *add2FldSet(fldSet *, fieldinfo *, bool, bool);
fldSet *createFldSet();
int printFldSet(fldSetNode *);
int printFieldSet(fldSet *);


/*------------------------------------------------------------*/
/*-- methodinfo call set fns */
/*------------------------------------------------------------*/
struct methSet {
	methSetNode *head;
	methSetNode *tail;
	methSetNode *pos;
	s4 length;
};


struct methSetNode {
	methodinfo   *methRef;
	methSetNode  *nextmethRef;
	classSetNode *lastptrIntoClassSet2;
	s2            index;
	s4            monoPoly;
};


int inMethSet (methSetNode *, methodinfo *);
methSetNode *addMethRef(methSetNode *, methodinfo *);
methSet *add2MethSet(methSet    *, methodinfo *);
methSet *createMethSet();
int printMethSet   (methSetNode *);
int printMethodSet (methSet *);


/*------------------------------------------------------------*/
/*-- classinfo XTA set fns  */
/*------------------------------------------------------------*/

struct classSet {
	classSetNode *head;
	classSetNode *tail;
	classSetNode *pos;
	s4 length;
};


struct classSetNode {
	classinfo *classType;
	classSetNode *nextClass;
	s2 index;
};


int inSet(classSetNode *, classinfo *);
classSetNode *addElement(classSetNode *,  classinfo *);
classSet *add2ClassSet(classSet *,  classinfo *);
classSet *createClassSet();
int inRange(classSetNode *, classinfo *);
classSetNode *addClassCone(classSetNode *,  classinfo *);
classSetNode *intersectSubtypesWithSet(classinfo *, classSetNode *); 
int sizeOfSet(classSetNode *s);
int setSize(classSetNode *);
int printSet(classSetNode *);
int printClassSet(classSet *);

#endif /* _SETS_H */


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
