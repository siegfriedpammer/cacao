/* vm/jit/inline/sets.c -

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
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

   Authors: Carolyn Oates

   $Id: sets.c 2107 2005-03-28 22:44:28Z twisti $

*/


#include <stdio.h>

#include "types.h"
#include "mm/memory.h"
#include "vm/global.h"
#include "vm/linker.h"
#include "vm/loader.h"
#include "vm/tables.h"
#include "vm/jit/inline/sets.h"


/*
 * set.c - functions to manipulate ptr sets.
 */

 
/*------------------------------------------------------------*/
/*-- fieldinfo call set fns */
/*------------------------------------------------------------*/
fldSetNode *inFldSet(fldSetNode *s, fieldinfo *f)
{
	fldSetNode* i;
	for (i=s; i != NULL; i = i->nextfldRef) {
		if (i->fldRef == f) {
			return i; /* true = found */
		}
	}
	return NULL;
}


/*------------------------------------------------------------*/
/* */
fldSetNode *addFldRef(fldSetNode *s, fieldinfo *f)
{
	fldSetNode *s1 = s;
	if (!inFldSet(s,f)) {
		s1 = NEW(fldSetNode);
		s1->nextfldRef  = s;
		s1->fldRef      = f;
		s1->writePUT     = false;
		s1->readGET    = false;
		s1->lastptrPUT = NULL;
		s1->lastptrGET = NULL;

		if (s == NULL)
			s1->index = 1;
		else
			s1->index = s->index+1; 
	}
	return s1;
}


/*------------------------------------------------------------*/
fldSet *add2FldSet(fldSet *sf,  fieldinfo *f, bool wput, bool rget)
{
	fldSetNode *s1;
	fldSetNode *s;
 
	if (sf == NULL) {
		sf = createFldSet();
	}
	s = sf->head;
	s1 = inFldSet(s,f);
	if (s1 == NULL) {
		s1 = NEW(fldSetNode);
		if (sf->head == NULL) {
			sf->head  = s1;
			sf->pos   = s1;
			s1->index = 1;
		}        
		else {
			sf->tail->nextfldRef  = s1;
			sf->length++;
			s1->index = sf->length;
    	} 
		s1->nextfldRef  = NULL;
		s1->fldRef      = f;
		s1->writePUT    = wput;
		s1->readGET     = rget;
		s1->lastptrPUT = NULL;
		s1->lastptrGET = NULL;
		sf->tail = s1;
	}
	else 	{
		if ((s1->writePUT == false) && (wput)) 
			s1->writePUT = wput;
		if ((s1->readGET == false)  && (rget)) 
			s1->readGET  = rget;
	}
	return sf;
}


/*------------------------------------------------------------*/
fldSet *createFldSet( )
{
	fldSet *s;
	s = NEW(fldSet);
	s->head = NULL;
	s->tail = NULL;
	s->pos  = NULL;
	s->length = 0;
	return s;
}


/*------------------------------------------------------------*/
/*-- methodinfo call set fns */
/*------------------------------------------------------------*/
int inMethSet(methSetNode *s, methodinfo *m)
{
	methSetNode* i;
	for (i = s; i != NULL; i = i->nextmethRef) {
		if (i->methRef == m) {
			return (int)1; /* true = found */
		}
	}
	return (int)0;
}


/*------------------------------------------------------------*/
methSetNode *addMethRef(methSetNode *s,  methodinfo *m)
{
	methSetNode *s1 = s;
	if (!inMethSet(s,m)) {
		s1 = NEW(methSetNode);
		s1->nextmethRef= s;
		s1->methRef = m;
		s1->lastptrIntoClassSet2 = NULL;
		if (s == NULL)
			s1->index = 1;
		else
			s1->index = s->index+1; 
		s1->monoPoly = MONO; 
	}
  
	return s1;
}


/*x------------------------------------------------------------*/
methSet *add2MethSet(methSet *sm,  methodinfo *m)
{
	methSetNode *snew;
	methSetNode *s;
 
	if (sm == NULL) {
		sm = createMethSet();
	}
	s = sm->head;
	if (!inMethSet(s,m)) {
		snew = NEW(methSetNode);
		if (sm->head == NULL) {
			sm->head  = snew;
			sm->pos   = snew;
			snew->index = 1;
		}        
		else {
			sm->tail->nextmethRef  = snew;
			sm->length++;
			snew->index = sm->length;
    	}
		snew->monoPoly = MONO; 
		snew->nextmethRef= NULL;
		snew->methRef = m;
		snew->lastptrIntoClassSet2 = NULL; /* ????? */
		sm->tail = snew;
	}
	return sm;
}

 
/*------------------------------------------------------------*/
methSet *createMethSet( )
{
	methSet *s;
	s = NEW(methSet);
	s->head = NULL;
	s->tail = NULL;
	s->pos  = NULL;
	s->length = 0;
	return s;
}


/*------------------------------------------------------------*/
/*-- classinfo XTA set fns  */
/*------------------------------------------------------------*/
int inSet(classSetNode *s, classinfo *c)
{
	classSetNode* i;
	for (i = s; i != NULL; i = i->nextClass) {
		if (i->classType == c) {
			return  ((i->index) + 1); /* true = found */
		}
	}
	return (int)0;
}


/*------------------------------------------------------------*/
classSetNode *addElement(classSetNode *s,  classinfo *c)
{
	classSetNode *s1 = s;
	if (!inSet(s,c)) {
		s1 = NEW(classSetNode);
		s1->nextClass= s;
		s1->classType = c;
		if (s == NULL)
			s1->index = 1;
		else
			s1->index = s->index+1; 
	}
	return s1;
}


/*------------------------------------------------------------*/
classSet *add2ClassSet(classSet *sc,  classinfo *c)
{
	classSetNode *s1;
	classSetNode *s;
 
	if (sc == NULL) {
		sc = createClassSet();
	}
	s = sc->head;
	
	if (!inSet(s,c)) {
		s1 = NEW(classSetNode);
		if (sc->head == NULL) {
			sc->head  = s1;
			sc->pos   = s1;
			s1->index = 1;
		}        
		else {
			sc->tail->nextClass  = s1;
			sc->length++;
			s1->index = sc->length;
    	} 
		s1->classType = c;
		s1->nextClass= NULL;
		sc->tail  = s1;
	}
	return sc;
}


/*------------------------------------------------------------*/
classSet *createClassSet()
{
	classSet *s;
	s = NEW(classSet);
	s->head = NULL;
	s->tail = NULL;
	s->pos  = NULL;
	s->length = 0;
	return s;
}


/*------------------------------------------------------------*/
/* Returns:                                                   */
/*    -1  c is a subclass   of an existing set element        */
/*     0  c class type cone does not overlap any set element  */
/*     1  c is a superclass of an existing set element        */

int inRange(classSetNode *s, classinfo *c)
{
	classSetNode* i;
	int rc = 0;

	for (i = s; i != NULL; i = i->nextClass) {
		classinfo *cs = i->classType;
		LAZYLOADING(c)  /* ??? is c reallz needed ???*/
		if (cs->vftbl->baseval <= c->vftbl->baseval) {
			if (c->vftbl->baseval <= (cs->vftbl->baseval+cs->vftbl->diffval)) {
				rc = -1;  /* subtype */
			}
		}
		else {
			if (cs->vftbl->baseval < (c->vftbl->baseval+c->vftbl->diffval)) {
				i->classType = c;   /* replace element with its new super */
				rc  = 1; /* super */
			}
		}
    }
	return rc;
}


/*------------------------------------------------------------*/
/* adds class if not subtype of an existing set element       */
/* if "new" class is super class of an existing element       */
/* then replace the existing element with the "new" class     */

classSetNode *addClassCone(classSetNode *s,  classinfo *c)
{
	classSetNode *s1 = s;
 
	if (inRange(s,c) == 0) {
		/* not in set nor cone of an existing element so add */
		s1 = NEW(classSetNode);
		s1->nextClass= s;
		s1->classType = c;
		if (s == NULL)
			s1->index = 1;
		else
			s1->index = s->index+1; 
	}
	return s1;
}


/*------------------------------------------------------------*/
/* intersect the subtypes of class t with class set s         */
classSetNode * intersectSubtypesWithSet(classinfo *t, classSetNode *s) {
	classSetNode *s1 = NULL;
	classSetNode *c;

	/* for each s class */
	for (c=s; c != NULL; c = c->nextClass) {
		vftbl_t *t_cl_vt;
		vftbl_t *c_cl_vt;
		LAZYLOADING(c->classType)
		LAZYLOADING(t)
		t_cl_vt = t->vftbl;
		c_cl_vt = c->classType->vftbl;
		/* if s class is in the t Class range */
		if (  (t_cl_vt->baseval <=  c_cl_vt->baseval)
			  && (c_cl_vt->baseval <= (t_cl_vt->baseval+t_cl_vt->diffval)) ) {

			/*    add s class to return class set */
			s1 = addElement(s1,c->classType);
		}
	}
	return s1;
}


/*------------------------------------------------------------*/
int sizeOfSet(classSetNode *s) {
	/*** need to update */
	int cnt=0;
	classSetNode * i;
	for (i=s; i != NULL; i = i->nextClass) cnt++;
	return cnt;
}

  
/*------------------------------------------------------------*/
int printSet(classSetNode *s)
{
	classSetNode* i;
	int cnt=0;

	if (s == NULL) {
		printf("Set of types: <");
		printf("\t\tEmpty Set\n");

	} else {
		printf("<%i>Set of types: ",s->index);
		for (i = s; i != NULL; i = i->nextClass) {
        	printf("\t#%i: ",cnt);
			if (i->classType == NULL)  {
				printf("NULL CLASS");
				fflush(stdout);

			} else {
				utf_display(i->classType->name);
				fflush(stdout); 
				printf("<b%i/d%i> ",i->classType->vftbl->baseval,i->classType->vftbl->diffval); 
				fflush(stdout);
			}
			cnt++;
		}
		printf(">\n");
	}
	return cnt;
}


/*------------------------------------------------------------*/
int printClassSet(classSet *sc)
{
	if (sc == NULL) {
		printf("Class Set not yet created\n");
		return 0;

	} else 
		return (printSet(sc->head));
}


/*------------------------------------------------------------*/
int printMethSet(methSetNode *s)
{
	methSetNode* i;
	int cnt=0;
	if (s == NULL) {
		printf("Set of Methods: "); fflush(stdout);
	     	printf("\t\tEmpty Set\n"); fflush(stdout);
		}
	else 	{
		printf("<%i>Set of Methods: ",s->index);fflush(stdout); 
		for (i=s; i != NULL; i = i->nextmethRef) {
        	printf("\t#%i: ",cnt);

			/* class.method */
			utf_display(i->methRef->class->name);
			printf(".");
			method_display(i->methRef);

			/* lastptr <class> */
			printf("\t<");
			if (i->lastptrIntoClassSet2 != NULL)
				utf_display(i->lastptrIntoClassSet2->classType->name);
			printf(">\n");

			cnt++;
		}
		printf("\n");
	}
	return cnt;
}


/*------------------------------------------------------------*/
int printMethodSet(methSet *sm) {
	if (sm == NULL) {
		printf("Method Set not yet created\n");fflush(stdout);
		return 0;
	}
	else	{
		if (sm->head == NULL) {
			printf("Set of Methods: "); fflush(stdout);
		     	printf("\t\tEmpty Set\n"); fflush(stdout);
			return 0;
			}
		else
			return (printMethSet(sm->head));
		}
}


/*------------------------------------------------------------*/
int printFldSet(fldSetNode *s)
{
	fldSetNode* i;
	int cnt=0;

	if (s == NULL) {
		printf("Set of Fields: ");
		printf("\tEmpty Set\n");
	}
	else 	{
		printf("<%i>Set of Fields: ",s->index);
		for (i=s; i != NULL; i = i->nextfldRef) {
        	printf("\t#%i: ",cnt);
			printf("(%ir/%iw)",i->writePUT,i->readGET);
			field_display(i->fldRef);
			cnt++;
		}
		printf("\n");
	}
	return cnt;
}


/*------------------------------------------------------------*/
int printFieldSet(fldSet *sf) {
	if (sf == NULL) {
		printf("Field Set not yet created\n");
		return 0;
	}
	else
		return (printFldSet(sf->head));
}
/*------------------------------------------------------------*/
/*void destroy_set */


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
