/* jit/parseXTA.c - parser and print functions for Rapid Type Analyis

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

   $Id: parseXTA.c 1959 2005-02-19 11:46:27Z carolyn $

*/

/*--------------------------------------
XTA so far
static
fields
virtual 
interfaces - started
clinit / final
general initialization 
----
interfaces
+ hand coded calls for system
2nd interation (want to try it in inlining)

Testing

?? PARTUSED???
----------------------------------*/

/***************
 XTA Type Static Analysis of Java program
   used -xta option is turned on either explicitly 
 or automatically with inlining of virtuals.

 XTA is called for reachable methods and keeps sets for each Method and Field used as follows:
1. Work list of reachable methods is initialized to main + JVM called methods 
						+ missed methods
2. For virtual method call from M of e.m then
  a. Add all static lookup of m in the cone of e = ce.mi to reachable worklist
	JAVA_INVOKESTATIC/ JAVA_INVOKESPECIAL - addXTAcallededges
	JAVA_INVOKEVIRTUAL - xtaMarkSubs
	JAVA_INVOKEINTERFACES
  b. Add mi's parameters class + subtypes that are used by M to mi used classes
	When XTA parsed follow the calls list (beg. parseXTA)
  c. Add mi's return type + subtypes used by mi to M's used classes
	When XTA parsed follow the calledBy list (end parseXTA)
  d. Add ce of mi to mi's used classes
	static/special  - addXTAcalledges

3. new C (new, <init>, & similiar) then add  C to M's used classes
	JAVA_NEW, INVOKE_SPECIAL <init>, JAVA_CHECKCAST / JAVA_INSTANCEOF - classinit 
4. read Field X.f: add X to M's used classes
	JAVA_GETSTATIC
5. write Field X.f: add X+subtypes that are used by M to X's classes
	JAVA_PUTSTATIC

 
 If M calls 
 USAGE:
 Methods called by NATIVE methods and classes loaded dynamically
 cannot be found by parsing. The following files supply missing methods:

 xtaMissedIn0 - (provided) has the methods missed by every java program
 xtaMissed||mainClassName - is program specific.

 A file xtaMissed will be written by XTA analysis of any methods missed.

 This file can be renamed to xtaMissed concatenated with the main class name.

 Example:
 ./cacao -xta hello

 inlining with virtuals should fail if the returned xtaMissed is not empty.
 so...
 mv xtaMissed xtaMissedhello
 ./cacao hello

Results: (currently) with -stat see # methods marked used
 
****************/

#include <stdio.h>
#include <string.h>

#include "cacao/cacao.h"
#include "mm/memory.h"   
#include "toolbox/list.h"
#include "vm/class.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "vm/tables.h"
#include "vm/jit/jit.h"
#include "vm/jit/parse.h"
#include "vm/jit/inline/parseXTA.h"
#include "vm/jit/inline/parseRTstats.h"
#include "vm/jit/inline/parseRTprint.h"



static bool firstCall= true;
static list *xtaWorkList;
FILE *xtaMissed;   /* Methods missed during XTA parse of Main  */

bool XTA_DEBUGinf = false;
bool XTA_DEBUGr = false;
bool XTA_DEBUGopcodes = false;
 
/*********************************************************************/
/*********************************************************************/

/*-------------------------------------------------------------------------------*/

xtafldinfo * xtafldinfoInit (fieldinfo *f)
{
        if (f->xta != NULL)
                return f->xta;

        f->xta = NEW(xtafldinfo);

        f->xta->fieldChecked = false;
        f->xta->fldClassType = NULL;
        f->xta->XTAclassSet = NULL;

        return f->xta;
}

/*-------------------------------------------------------------------------------*/

xtainfo *xtainfoInit(methodinfo *m)
{
        if (m->xta != NULL)
                return m->xta; /* already initialized */

    	count_methods_marked_used++;

        m ->xta = (xtainfo *) NEW(xtainfo);
        m ->xta-> XTAmethodUsed = NOTUSED;

	/* xta sets for a method */
        m->xta->fldsUsed        = NULL;
        m ->xta-> XTAclassSet   = NULL;
				/* Methods's have access to the class they are a part of */
        m ->xta-> XTAclassSet   = add2ClassSet ( m ->xta-> XTAclassSet, m->class);  
	/* cone set of methods parameters */ 
       		 /* what if no param?? is it NULL then, too? */ 
        m->xta->paramClassSet = descriptor2typesL(m); 

	/* Edges */
        m->xta->calls         = NULL;
        m->xta->calledBy      = NULL;

        m ->xta->markedBy     = NULL;
        m->xta->chgdSinceLastParse = false;
        return m->xta;

	/* thought needed at some earlier point */
        /*m->xta->marked        = NULL; * comment out*/
        /*m ->xta->interfaceCalls    = NULL*/
}

/*-------------------------------------------------------------------------------*/
void xtaAddCallEdges(methodinfo *mCalls, methodinfo *mCalled, s4 monoPoly, char *info) {
    xtaNode    *xta;

/* First call to this method initializations */
if (mCalled->methodUsed != USED) {
  if (!(mCalled->flags & ACC_ABSTRACT))  {
    mCalled->xta = xtainfoInit(mCalled);
    mCalled ->methodUsed = USED; /* used to see if method in the work list of methods */ 
    xta = NEW(xtaNode);
    xta->method = mCalled ;
    list_addlast(xtaWorkList,xta);
    }
  else return; /* abstract */
  }

if ((mCalls == NULL) && (monoPoly == SYSCALL)) return;
/* Add call edges */
mCalls->xta->calls = add2MethSet(mCalls->xta->calls, mCalled);
			/* mono if static, private, final else virtual so poly */
mCalls->xta->calls->tail->monoPoly = monoPoly;  

mCalled->xta->calledBy = add2MethSet(mCalled->xta->calledBy, mCalls);

/* IS THIS REALLY NEEDED???? */
if (mCalled->xta->calledBy == NULL) panic("mCalled->xta->calledBy is NULL!!!");
if (mCalls->xta->calls == NULL) panic("mCalls->xta->calls is NULL!!!");

}


/*-------------------------------------------------------------------------------*/
bool xtaPassParams (methodinfo *Called, methodinfo *Calls, methSetNode *lastptrInto)
{
        classSetNode *p;
        classSetNode *c;
        classSetNode *c1;
        classSetNode *cprev;
        bool          chgd = false;

        if (lastptrInto->lastptrIntoClassSet2 == NULL) {
                if (Calls->xta->XTAclassSet != NULL)
                        c1 = Calls->xta->XTAclassSet->head;
                 /*else already c1 = NULL; */
        }
        else    {
                /* start with type where left off */
                c1 = lastptrInto->lastptrIntoClassSet2;
                c1 = c1 -> nextClass;  /* even if NULL */
        }
	if (c1 == NULL) return false;

        cprev = NULL;
       /* for each Param Class */
        for (   p=Called->xta->paramClassSet; p != NULL; p = p->nextClass) {

                /* for each SmCalls class */
                for (c=c1; c != NULL; c = c->nextClass) {
                        vftbl_t *p_cl_vt = p->classType->vftbl;
                        vftbl_t *c_cl_vt = c->classType->vftbl;

                        /* if SmCalls class is in the Params Class range */
                        if (  (p_cl_vt->baseval <=  c_cl_vt->baseval)
                                  && (c_cl_vt->baseval <= (p_cl_vt->baseval+p_cl_vt->diffval)) ) {

                                /*    add Calls class to CalledBy Class set */
                                Called->xta->XTAclassSet = Called->xta->XTAclassSet = add2ClassSet(Called->xta->XTAclassSet, c->classType);
                              chgd = true;
                        }
                        cprev = c;
                }
        }
        lastptrInto->lastptrIntoClassSet2 = cprev;
       	return chgd;
}


/*-------------------------------------------------------------------------------*/
void xtaPassAllCalledByParams (methodinfo *m) {
        methSetNode *SmCalled;
        if (m->xta->calledBy == NULL)
		return;
        for (SmCalled  = m->xta->calledBy->head; 
	     SmCalled != NULL; 
	     SmCalled = SmCalled->nextmethRef) {
                m->xta->chgdSinceLastParse = false;             /* re'init flag */
                xtaPassParams(m, SmCalled->methRef,SmCalled);   /* chg flag output ignored for 1st regular parse */
        }
}


/*-------------------------------------------------------------------------------*/
void xtaPassFldPUT(methodinfo *m, fldSetNode *fN)
{
	/* Field type is a class */
	classSetNode *c;
	classSetNode *c1 = NULL;
	classSetNode *cp = NULL;
	classSetNode *cprev= NULL;

	fieldinfo *fi;
	if (fN != NULL)
		fi = fN->fldRef;
	else
		return;

/* Use lastptr  so don't check whole XTA class set each time */
	cp = fN->lastptrPUT;
	if (cp != NULL) {
		if (cp->nextClass != NULL)
			c1 = cp -> nextClass;
	} 
	else	{
		if (m->xta->XTAclassSet != NULL)
			c1  = m->xta->XTAclassSet->head;
	}

	/*--- PUTSTATIC specific ---*/
	/* Sx = intersection of type+subtypes(field x)   */
	/*   and Sm (where putstatic code is)            */
	for (c=c1; c != NULL; c=c->nextClass) {
		vftbl_t *f_cl_vt = fi->xta->fldClassType->vftbl;
		vftbl_t *c_cl_vt =  c->   classType->vftbl;

		if ((f_cl_vt->baseval <= c_cl_vt->baseval)
			&& (c_cl_vt->baseval <= (f_cl_vt->baseval+f_cl_vt->diffval)) ) {
			fi->xta->XTAclassSet = add2ClassSet(fi->xta->XTAclassSet,c->classType);
		}
		cprev = c;
	}
	fN->lastptrPUT = cprev;
}
/*-------------------------------------------------------------------------------*/
void xtaPassFldGET(methodinfo *m, fldSetNode *fN)
{
	/* Field type is a class */
	classSetNode *c;
	classSetNode *c1 = NULL;
	classSetNode *cp = NULL;
	classSetNode *cprev= NULL;

	fieldinfo *fi;
	if (fN != NULL)
		fi = fN->fldRef;
	else
		return;

/* Use lastptr  so don't check whole XTA class set each time */
	cp = fN->lastptrGET;
	if (cp != NULL) {
		if (cp->nextClass != NULL)
			c1 = cp -> nextClass;
	} 
	else	{
		if (fi->xta->XTAclassSet != NULL)
			c1  = fi->xta->XTAclassSet->head;
	}

	/*--- GETSTATIC specific ---*/
	/* Sm = union of Sm and Sx */
	for (c=c1; c != NULL; c=c->nextClass) {
		bool addFlg = false;
		if (m->xta->XTAclassSet ==NULL) 
			addFlg = true;
		else 	{
			if (!(inSet (m->xta->XTAclassSet->head, c->classType) )) 
				addFlg = true;
			}
		if (addFlg) {
			m->xta->XTAclassSet 
				= add2ClassSet(m->xta->XTAclassSet,c->classType);
		}
		cprev = c;
	}

	fN->lastptrGET = cprev;
}


/*-------------------------------------------------------------------------------*/
void xtaAllFldsUsed (methodinfo *m) {
        fldSetNode  *f;
        fldSetNode *f1=NULL;
        /*      bool chgd = false */

        if (m->xta->fldsUsed == NULL) return;

        /* for each field that this method uses */
        f1 = m->xta->fldsUsed->head;

        for (f=f1; f != NULL; f = f->nextfldRef) {

                if (f->writePUT)
                        xtaPassFldPUT(m,f);
                if (f->readGET)
                        xtaPassFldGET(m,f);
        }
}

/*-------------------------------------------------------------------------------*/
bool xtaPassReturnType(methodinfo *Called, methodinfo *Calls) {

	classSetNode* cs;
	classSetNode* cs1;
	bool          fchgd = false;

	/* Get Called return class is null */
	if ((Called->returnclass == NULL) && (Called->xta->paramClassSet == NULL)) {
		Called->xta->paramClassSet = descriptor2typesL(Called); /* old comment - in new xta struc init */ 
	}

	if (Called->returnclass == NULL) {
		return fchgd;
	}
	
	if (Called->xta->XTAclassSet == NULL) 
		cs1 = NULL;
	else
		cs1 =  Called->xta->XTAclassSet->head;

	for (cs =cs1; cs != NULL; cs = cs->nextClass) {
		classinfo *c = cs->classType;
		vftbl_t *r_cl_vt = Called->returnclass->vftbl; 
		vftbl_t *c_cl_vt = c->vftbl; 

		/* if class is a subtype of the return type, then add to Calls class set (ie.interscection)*/
		if (  (r_cl_vt->baseval <=  r_cl_vt->baseval)
			  && (c_cl_vt->baseval <= (r_cl_vt->baseval+r_cl_vt->diffval)) ) {
			Calls->xta->XTAclassSet = add2ClassSet(Calls->xta->XTAclassSet, c);  
			fchgd = true;
		}
	} 

	return fchgd; 
}


/*-------------------------------------------------------------------------------*/
void  xtaMethodCalls_and_sendReturnType(methodinfo *m)
{
        methSetNode *SmCalled;  /* for return type       */
        methSetNode *SmCalls;   /* for calls param types */
        methSetNode *s1=NULL;
        bool chgd = false;
        xtaAllFldsUsed (m);

        /* for each method that this method calls */
        if (m->xta->calls == NULL)
                s1 = NULL;
        else
                s1 = SmCalls=m->xta->calls->head;

        for (SmCalls=s1; SmCalls != NULL; SmCalls = SmCalls->nextmethRef) {
                /*    pass param types  */
                bool chgd = false;
                chgd = xtaPassParams (SmCalls->methRef, m, SmCalls);
                /* if true chgd after its own parse */
                if (!(SmCalls->methRef->xta->chgdSinceLastParse)) {
                        SmCalls->methRef->xta->chgdSinceLastParse = true;
                }
        }

        /* for each calledBy method */
        /*    send return type */
        if (m->xta->calledBy == NULL)
                s1 = NULL;
        else
                s1 = m->xta->calledBy->head;
        for (SmCalled=s1; SmCalled != NULL; SmCalled = SmCalled->nextmethRef) {

                chgd = xtaPassReturnType(m, SmCalled->methRef);
                if (!(SmCalled->methRef->xta->chgdSinceLastParse)) {
                        SmCalled->methRef->xta->chgdSinceLastParse = chgd;
                }
        }
}

/*-------------------------------------------------------------------------------*/
bool xtaAddFldClassTypeInfo(fieldinfo *fi) {

	bool is_classtype = false; /* return value */

	if (fi->xta->fieldChecked) {
		if (fi->xta->fldClassType != NULL)
			return true;  /* field has a class type */
		else
			return false;
	}
	fi->xta->fieldChecked = true;

	if (fi->type == TYPE_ADDRESS) {
		char *utf_ptr = fi->descriptor->text;  /* current position in utf text */

		if (*utf_ptr != 'L') {
			while (*utf_ptr++ =='[') ;
		}

		if (*utf_ptr =='L') {
			is_classtype = true;
			if  (fi->xta->fldClassType== NULL) {
				char *desc;
				char *cname;
				classinfo * class;

				desc = MNEW(char, 256);
				strcpy(desc,++utf_ptr);
				cname = strtok(desc,";");
				class = class_get(utf_new_char(cname));
				fi->xta->fldClassType= class;    /* save field's type class ptr */	
			} 
		}
	}
	return is_classtype;
}

/*--------------------------------------------------------------*/
/* Mark the method with same name /descriptor in topmethod      */
/* in class                                                     */
/*                                                              */
/* Class marked USED and method defined in this class ->        */
/*    -> add call edges = USED                                  */
/* Class not marked USED and method defined in this class ->    */
/*    -> if Method NOTUSED mark method as MARKED                */
/*                                                              */
/* Class USED, but method not defined in this class ->          */
/* -> 1) search up the heirarchy and mark method where defined  */
/*    2) if class where method is defined is not USED ->        */
/*       -> ????mark class with defined method as PARTUSED          */
/*--------------------------------------------------------------*/


void xtaMarkMethod(classinfo *class, methodinfo *mCalls, methodinfo *topmethod, classSetNode *subtypesUsedSet) {

utf *name 	= topmethod->name; 
utf *descriptor = topmethod->descriptor;
methodinfo *submeth;

/* See if method defined in class heirarchy */
submeth = class_resolvemethod(class, name, descriptor); 
METHINFOt(submeth,"xtaMarkMethod submeth:",XTA_DEBUGr);
if (submeth == NULL) {
	utf_display(class->name); printf(".");
	METHINFOx(topmethod);
	printf("parse XTA: Method not found in class hierarchy");fflush(stdout);
	panic("parse XTA: Method not found in class hierarchy");
	}

/* if submeth called previously from this method then return */
if (mCalls->xta->calls != NULL) {
	if (inMethSet(mCalls->xta->calls->head,submeth)) return;
	}

#undef CTA 
#ifdef CTA
  /* Class Type Analysis if class.method in virt cone marks it used */
  /*   very inexact, too many extra methods */
  XTAaddClassInit(submeth,	submeth->class,
		CLINITS_T,FINALIZE_T,ADDMARKED_T);
  if (inSet(subtypesUsedSet,submeth->class)) {
      submeth->monoPoly = POLY;
      xtaAddCallEdges(mCalls, submeth, submeth->monoPoly, 
 		      "addTo XTA VIRT CONE:");
      }
  return;
#endif

  if (submeth->class == class) { 

	/*--- Method defined in class -----------------------------*/
  	if (inSet(subtypesUsedSet,submeth->class)) {
		/* method defined in this class -> */
		/* Class IS  marked USED           */ 
		/*    -> mark method as USED       */
  		submeth->monoPoly = POLY;
      		xtaAddCallEdges(mCalls, submeth, submeth->monoPoly, 
	   		"addTo VIRT CONE 1:");
		}
	else 	{
		/* method defined in this class -> */
		/* Class IS NOT  marked USED (PART or NOTUSED) */ 
		/* -> if Method NOTUSED mark method as  MARKED */
		METHINFOt(submeth,
	 		"\tmarked VIRT CONE 2:",XTA_DEBUGr);
		submeth->monoPoly = POLY;
		submeth->xta->markedBy = add2MethSet(submeth->xta->markedBy,mCalls);
		/* Note: if class NOTUSED and subclass is used handled  */
		/*       by subsequent calls to xtaMarkMethods for cone */
		}
	} /* end defined in class */

  else {
	/*--- Method NOT defined in class ---------------*/
        /* first mark classes if needed */
  	if (!(inSet(subtypesUsedSet,submeth->class))) {
		submeth->class->classUsed = PARTUSED; /**** ???? equivalant for xta ???? */
  		if (!(inSet(subtypesUsedSet,class))) {
			submeth->monoPoly = POLY;
			submeth->xta->markedBy = add2MethSet(submeth->xta->markedBy,mCalls);
			METHINFOt(submeth,"JUST MARKED :",XTA_DEBUGr);
			}
		}
        /* add method to xta work list if conditions met */
		/*??if ( (submeth->class->classUsed == USED) ||  */
  	if (inSet(subtypesUsedSet,class)) {
  		submeth->monoPoly = POLY;
  		xtaAddCallEdges(mCalls, submeth, submeth->monoPoly, 
	  			"addTo VIRT CONE 3:");
		}
  	} /* end NOT defined in class */

} 

/*----------------------------------------------------------------------*/
/* Mark the method with the same name and descriptor as topmethod       */
/*   and any subclass where the method is defined and/or class is used  */
/*                                                                      */
/*----------------------------------------------------------------------*/

void xtaMarkSubs(methodinfo *mCalls, classinfo *class, methodinfo *topmethod, classSetNode *subtypesUsedSet) {
        /* xtaPRINTmarkSubs1*/
        xtaMarkMethod(class, mCalls, topmethod, subtypesUsedSet);   /* Mark method in class where it was found */
        if (class->sub != NULL) {
                classinfo *subs;

                if (!(topmethod->flags & ACC_FINAL )) {
                        for (subs = class->sub; subs != NULL; subs = subs->nextsub) {
                                /* xtaPRINTmarkSubs1 */
                                xtaMarkSubs(mCalls, subs, topmethod, subtypesUsedSet);
                                }
                        }
                }
}


/**************************************************************************/
/* Add Marked methods for input class ci                                  */
/* Add methods with the same name and descriptor as implemented interfaces*/
/*   with the same method name                                            */
/*  ??? not XTA checked                                                                      */
/*------------------------------------------------------------------------*/
void xtaAddMarkedMethods(methodinfo *mCalls, classinfo *ci) {
int ii,jj,mm;

/* add marked methods to callgraph */ 
for (ii=0; ii<ci->methodscount; ii++) { 
	methodinfo *mi = &(ci->methods[ii]);

	if (mi->methodUsed == MARKED) { 
		xtaAddCallEdges(mCalls, mi, MONO,  /* SHOULD this really be MONO ?????? */
				"addTo was MARKED:");
		}
	else	{
		for (jj=0; jj < ci -> interfacescount; jj++) {
			classinfo *ici = ci -> interfaces [jj];
			/*  use resolve method....!!!! */
			if (ici -> classUsed != NOTUSED) {
				for (mm=0; mm< ici->methodscount; mm++) {
					methodinfo *imi = &(ici->methods[mm]);
					METHINFOt(imi,"NEW IMPD INTERFACE:",XTA_DEBUGinf)
				      /*if interface method=method is used*/
					if  (  	   (imi->methodUsed == USED)
			   &&	 ( (imi->name == mi->name) 
			   && 	   (imi->descriptor == mi->descriptor))) {
				xtaAddCallEdges(mCalls, mi, mi->monoPoly, 
				     "addTo was interfaced used/MARKED:");
					  }
					} /*end for */	
				}
			}
		}
	}
}    
void xtaAddUsedInterfaceMethods(methodinfo *m, classinfo *ci); /* prototype until check code for xta */

/*----------------------------------------------------------------------*/

#define CLINITS_T   true
#define FINALIZE_T  true
#define ADDMARKED_T true

#define CLINITS_F   false 
#define FINALIZE_F  false 
#define ADDMARKED_F false
/*-----------------------*/

void XTAaddClassInit(methodinfo *mCalls, classinfo *ci, bool clinits, bool finalizes, bool addmark)
{
  methodinfo *mi;

  if (addmark)
  	ci->classUsed = USED;
	
  if (clinits) { /* No <clinit>  available - ignore */
    mi = class_findmethod(ci, utf_clinit, utf_void__void); 
    if (mi) { 
	if (ci->classUsed != USED)
	    ci->classUsed = PARTUSED;
        mi->monoPoly = MONO;
	xtaAddCallEdges(mCalls, mi, mi->monoPoly, 
        		"addTo CLINIT added:");
      }     
    }        

  /*Special Case for System class init:
    add java/lang/initializeSystemClass to callgraph */
  if (ci->name == utf_new_char("initializeSystemClass")) {
    /* ?? what is name of method ?? */ 
    } 

  if (finalizes) {
    mi = class_findmethod(ci, utf_finalize, utf_void__void);
    if (mi) { 
	if (ci->classUsed != USED)
	    ci->classUsed = PARTUSED;
	mi->monoPoly = MONO;
	xtaAddCallEdges(mCalls, mi, mi->monoPoly, 
        		"addTo FINALIZE added:");
      }     
    }        

  if (addmark) {
    xtaAddMarkedMethods(mCalls, ci);
    }
  xtaAddUsedInterfaceMethods(mi,ci);

}

/*-------------------------------------------------------------------------------*/

/*********************************************************************/
/*********************************************************************/

/**************************************************************************/

/*------------------------------------------------------------------------*/
void xtaAddUsedInterfaceMethods(methodinfo *m, classinfo *ci) {
	int jj,mm;

	/* add used interfaces methods to callgraph */
	for (jj=0; jj < ci -> interfacescount; jj++) {
		classinfo *ici = ci -> interfaces [jj];
	
		if (XTA_DEBUGinf) { 
			printf("BInterface used: ");fflush(stdout); 
			utf_display(ici->name);
			printf("<%i>\t",ici -> classUsed ); fflush(stdout); 
			if (ici -> classUsed == NOTUSED) printf("\t classUsed=NOTUSED\n" );
			if (ici -> classUsed == USED) printf("\t classUsed=USED\n");
			if (ici -> classUsed == PARTUSED) printf("\t classUsed=PARTUSED\n");
			fflush(stdout);
		}
		/* add class to interfaces list of classes that implement it */
		ici -> impldBy =  addElement(ici -> impldBy,  ci);

		/* if interface class is used */
        if (ici -> classUsed != NOTUSED) {

			/* for each interface method implementation that has already been used */
			for (mm=0; mm< ici->methodscount; mm++) {
				methodinfo *imi = &(ici->methods[mm]);
				if (XTA_DEBUGinf) { 
					if  (imi->methodUsed != USED) {
						if (imi->methodUsed == NOTUSED) printf("Interface Method notused: "); 
						if (imi->methodUsed == MARKED) printf("Interface Method marked: "); 
						utf_display(ici->name);printf(".");method_display(imi);fflush(stdout);
					}
				} 
				if  (imi->methodUsed == USED) {
					if (XTA_DEBUGinf) { 
						printf("Interface Method used: "); utf_display(ici->name);printf(".");method_display(imi);fflush(stdout);

						/* Mark this method used in the (used) implementing class and its subclasses */
						printf("rMAY ADD methods that was used by an interface\n");
					}
					if ((utf_clinit != imi->name) &&
					    (utf_init != imi->name))
						{
						classSetNode *subtypesUsedSet = NULL;
					    	xtaMarkSubs(m, ci, imi, subtypesUsedSet); /*** CODE not finished for set */
						}
				}
			}
		}
	}

}


/*********************************************************************/

void xtaMarkInterfaceSubs(methodinfo *m, methodinfo *mi) {				
	classSetNode *subs;
	if (mi->class->classUsed == NOTUSED) {
		mi->class->classUsed = USED; 
		class_java_lang_Object->impldBy =  addElement(class_java_lang_Object -> impldBy,  mi->class);
		}

	/* add interface class to list kept in Object */
	mi->methodUsed = USED;
	mi->monoPoly   = POLY;

	subs =  mi->class->impldBy; 
            /*XTAPRINT08invokeInterface1*/
		   if (XTA_DEBUGinf) {
			METHINFO(mi,XTA_DEBUGinf)
                        printf("Implemented By classes :\n");fflush(stdout);
                        if (subs == NULL) printf("\tNOT IMPLEMENTED !!!\n");
                        fflush(stdout);
			}
	while (subs != NULL) { 			
		methodinfo *submeth;
		   if (XTA_DEBUGinf) {
		       printf("\t");utf_display(subs->classType->name);fflush(stdout);
			printf(" <%i>\n",subs->classType->classUsed);fflush(stdout);
			}
		/*Mark method (mark/used) in classes that implement method*/
						
		submeth = class_findmethod(subs->classType, mi->name, mi->descriptor); 
		if (submeth != NULL) {
			classSetNode *subtypesUsedSet = NULL;
			submeth->monoPoly = POLY; /*  poly even if nosubs */
			mi->xta->XTAmethodUsed = USED;
			if (m->xta->XTAclassSet != NULL) {
				subtypesUsedSet =
				intersectSubtypesWithSet
					(subs->classType, m->xta->XTAclassSet->head);

                                 subtypesUsedSet =
                                 intersectSubtypesWithSet(mi->class, m->xta->XTAclassSet->head);
				}
                         else
                                 subtypesUsedSet = addElement(subtypesUsedSet, m->class);
                         xtaMarkSubs(m, subs->classType, mi, subtypesUsedSet);
                         }
				
		subs = subs->nextClass;
		} /* end while */
} 





				

/*********************************************************************/

int parseXTA(methodinfo *m)
{
        int  p;                     /* java instruction counter */ 
        int  nextp;                 /* start of next java instruction */
        int  opcode;                /* java opcode */
        int  i;                     /* temp for different uses (counters)*/
        bool iswide = false;        /* true if last instruction was a wide*/
        int rc = 1;

METHINFOt(m,"\n----XTA PARSING:",XTA_DEBUGopcodes); 
if ((XTA_DEBUGr)||(XTA_DEBUGopcodes)) printf("\n");
        if (m->xta == NULL) {
		xtainfoInit (m);
		}
	else    {
		xtaPassAllCalledByParams (m); 
		}

/* scan all java instructions */
	for (p = 0; p < m->jcodelength; p = nextp) {

		opcode = code_get_u1(p,m);            /* fetch op code  */
		SHOWOPCODE(XTA_DEBUGopcodes)

		nextp = p + jcommandsize[opcode];   /* compute next instr start */
		if (nextp > m->jcodelength)
			panic("Unexpected end of bytecode");

		switch (opcode) {

		case JAVA_ILOAD:
		case JAVA_LLOAD:
		case JAVA_FLOAD:
		case JAVA_DLOAD:
		case JAVA_ALOAD:

                case JAVA_ISTORE:
                case JAVA_LSTORE:
                case JAVA_FSTORE:
                case JAVA_DSTORE:
                case JAVA_ASTORE:

			if (iswide) {
				nextp = p + 3;
				iswide = false;
			}
			break;

		case JAVA_IINC: 
			{
				
				if (iswide) {
					iswide = false;
					nextp = p + 5;
				}
			}
			break;

			/* wider index for loading, storing and incrementing */

		case JAVA_WIDE:
			iswide = true;
			nextp = p + 1;
			break;

		case JAVA_RET:
			if (iswide) {
				nextp = p + 3;
				iswide = false;
			}
			break;

		case JAVA_LOOKUPSWITCH:
			{
			s4 num;
                        nextp = ALIGN((p + 1), 4) + 4;
			num = code_get_u4(nextp,m);
                        nextp += (code_get_u4(nextp,m)) * 8 + 4;
			break;
			}


		case JAVA_TABLESWITCH:
                       {
                                s4 num;
                                nextp = ALIGN ((p + 1),4);
                                num = code_get_s4(nextp + 4, m);
                                num = code_get_s4(nextp + 8, m) - num;
                                nextp = nextp + 16 + 4 * num;
                                break;
                        }
 /*********************/

		case JAVA_PUTSTATIC: /* write */

			i = code_get_u2(p + 1,m);
			{
				constant_FMIref *fr;
				fieldinfo *fi;

				fr = class_getconstant(m->class, i, CONSTANT_Fieldref);
				LAZYLOADING(fr->class)

				fi = class_resolvefield(fr->class,
							fr->name,
							fr->descriptor,
							m->class,
							true);

				if (!fi)
					return 0; /* was NULL */

				fi->xta = xtafldinfoInit(fi);
				XTAaddClassInit(m,	fi->class,
						CLINITS_T,FINALIZE_T,ADDMARKED_F);
				if (xtaAddFldClassTypeInfo(fi)) {
					m->xta->fldsUsed = add2FldSet(m->xta->fldsUsed, fi, true,false);
					}
			}
			break;


		case JAVA_GETSTATIC: /* read */

			i = code_get_u2(p + 1,m);
			{
				constant_FMIref *fr;
				fieldinfo *fi;

				fr = class_getconstant(m->class, i, CONSTANT_Fieldref);
				LAZYLOADING(fr->class)

				fi = class_resolvefield(fr->class,
							fr->name,
							fr->descriptor,
							m->class,
							true);

				if (!fi)
					return 0; /* was NULL */

				fi->xta = xtafldinfoInit(fi);
				XTAaddClassInit(m,	fi->class,
						CLINITS_T,FINALIZE_T,ADDMARKED_F);
				if (xtaAddFldClassTypeInfo(fi)) {
					m->xta->fldsUsed = add2FldSet(m->xta->fldsUsed, fi, false, true);
					}

			}
			break;


				    	 	 		
			case JAVA_INVOKESTATIC:
			case JAVA_INVOKESPECIAL:
				i = code_get_u2(p + 1,m);
				{
				constant_FMIref *mr;
				methodinfo *mi;

				mr = class_getconstant(m->class, i, CONSTANT_Methodref);
				LAZYLOADING(mr->class) 
				mi = class_resolveclassmethod(	mr->class,
												mr->name,
												mr->descriptor,
												m->class,
												false);

				if (mi) 
				   {
				   METHINFOt(mi,"INVOKESTAT/SPEC:: ",XTA_DEBUGopcodes)
  				   mi->monoPoly = MONO;
  				   
  				   /*---- Handle "leaf" = static, private, final calls-------------*/
			   	   if ((opcode == JAVA_INVOKESTATIC)	   
				     || (mi->flags & ACC_STATIC)  
				     || (mi->flags & ACC_PRIVATE)  
				     || (mi->flags & ACC_FINAL) )  
			         	{
				       	if (mi->class->classUsed != USED) { /* = NOTUSED or PARTUSED */ 
				 		XTAaddClassInit(m, mi->class, 
					  			CLINITS_T,FINALIZE_T,ADDMARKED_T); 
								/* Leaf methods are used whether class is or not */
								/*   so mark class as PARTlyUSED                 */
				     	       	mi->class->classUsed = PARTUSED; 
						} 
					/* Add to XTA working list/set of reachable methods	*/
					if (opcode == JAVA_INVOKESTATIC)  /* if stmt just for debug tracing */	   
						             /* calls , called */
						xtaAddCallEdges(m, mi, MONO, 
							"addTo INVOKESTATIC "); 
					else 
						xtaAddCallEdges(m, mi, MONO, 
							"addTo INVOKESPECIAL ");	
					} /* end STATIC, PRIVATE, FINAL */ 
				     	
				   else {
					/*---- Handle special <init> calls ---------------------------------------------*/
				   
					if (mi->class->classUsed != USED) {
			       		/* XTA special case:
			          		call of super's <init> then
			          		methods of super class not all used */
				          		
			          		/*--- <init>  ()V  is equivalent to "new" 
			          			indicating a class is used = instaniated ---- */	
			       			if (utf_init==mi->name) {
				    			if ((m->class->super == mi->class) 
				    			&&  (m->descriptor == utf_void__void) ) 
								{
								METHINFOt(mi,"SUPER INIT:",XTA_DEBUGopcodes);
								/* super init so class may be only used because of its sub-class */
				     				XTAaddClassInit(m,mi->class,
						        		CLINITS_T,FINALIZE_T,ADDMARKED_F);
								if (mi->class->classUsed == NOTUSED) mi->class->classUsed = PARTUSED;
								}
					    		else {
						    		/* since <init> indicates classes is used, then add marked methods, too */
									METHINFOt(mi,"NORMAL INIT:",XTA_DEBUGopcodes);
					        		XTAaddClassInit(m, mi->class,
										CLINITS_T,FINALIZE_T,ADDMARKED_T);
					        		}
							xtaAddCallEdges(m, mi, MONO, 
									"addTo INIT ");
							} /* end just for <init> ()V */
					 		
						/* <clinit> for class inits do not add marked methods; 
								class not yet instaniated */ 
						if (utf_clinit==mi->name)
							XTAaddClassInit(m,	mi->class,
									CLINITS_T,FINALIZE_T,ADDMARKED_F);

						if (!((utf_init==mi->name))
						||   (utf_clinit==mi->name)) {
							METHINFOt(mi,"SPECIAL not init:",XTA_DEBUGopcodes)
							if (mi->class->classUsed !=USED)
								mi->class->classUsed = PARTUSED;
							xtaAddCallEdges(m, mi, MONO, 
								"addTo SPEC notINIT ");
							} 
					  			
						} /* end init'd class not used = class init process was needed */ 
							
					/* add method to XTA list = set of reachable methods */	
					xtaAddCallEdges(m, mi, MONO, 
						"addTo SPEC whymissed ");
					} /* end inits */
				} 
/***  assume if method can't be resolved won't actually be called or
      there is a real error in classpath and in normal parse an exception
      will be thrown. Following debug print can verify this
else  from if (mi) {
CLASSNAME1(mr->class,"CouldNOT Resolve method:",,XTA_DEBUGr);printf(".");fflush(stdout);
utf_display(mr->name); printf(" "); fflush(stdout);
utf_display(mr->descriptor); printf("\n");fflush(stdout);
***/
			}
			break;

                case JAVA_INVOKEVIRTUAL:
			i = code_get_u2(p + 1,m);
			{
				constant_FMIref *mr;
                                methodinfo *mi;

			       	mr = m->class->cpinfos[i];
                                /*mr = class_getconstant(m->class, i, CONSTANT_Methodref)*/
			       	LAZYLOADING(mr->class) 
				mi = class_resolveclassmethod(mr->class,
                                                mr->name,
                                                mr->descriptor,
              					m->class,
              					false);


				if (mi) 
				   {
				   METHINFOt(mi,"INVOKEVIRTUAL ::",XTA_DEBUGopcodes);
			   	   if ((mi->flags & ACC_STATIC) 
				   ||  (mi->flags & ACC_PRIVATE)  
				   ||  (mi->flags & ACC_FINAL) )  
			             { /*** DOES THIS EVER OCCUR ??? */
				     if (mi->class->classUsed == NOTUSED){
				       XTAaddClassInit(m, mi->class,
				   		    CLINITS_T,FINALIZE_T,ADDMARKED_T);
				       }
  				      mi->monoPoly = MONO;
				      xtaAddCallEdges(m, mi, MONO, 
				  	            "addTo INVOKEVIRTUAL ");
				      } 
				   else { /* normal virtual */
        				classSetNode *subtypesUsedSet = NULL;
				        if (m->xta->XTAclassSet != NULL)
					        subtypesUsedSet =
				                intersectSubtypesWithSet(mi->class, m->xta->XTAclassSet->head);
				        else
				                subtypesUsedSet = addElement(subtypesUsedSet, m->class);
				       mi->monoPoly = POLY;
				       xtaMarkSubs(m, mi->class, mi, subtypesUsedSet);
				       }
				   } 
				else {
CLASSNAME1(mr->class,"CouldNOT Resolve virt meth:",XTA_DEBUGr);printf(".");fflush(stdout);
utf_display(mr->name); printf(" "); fflush(stdout);
utf_display(mr->descriptor); printf("\n");fflush(stdout);
				   }
				}
				break;

                case JAVA_INVOKEINTERFACE:
                        i = code_get_u2(p + 1,m);
                        {
                                constant_FMIref *mr;
                                methodinfo *mi;

                                mr = class_getconstant(m->class, i, CONSTANT_InterfaceMethodref);
                                LAZYLOADING(mr->class)

                                mi = class_resolveinterfacemethod(mr->class,
                                                          mr->name,
                                                          mr->descriptor,
                                                          m->class,
                                                          false);
					if (mi)
                                           {
					   METHINFOt(mi,"\tINVOKEINTERFACE: ",XTA_DEBUGopcodes)
					   xtaMarkInterfaceSubs(m,mi);
					   }
				/* see INVOKESTATIC for explanation about */
			        /*   case when Interface is not resolved  */
                                /*descriptor2types(mi); 
				?? do need paramcnt? for XTA (or just XTA)*/
                        }
                        break;

                case JAVA_NEW:
                /* means class is at least passed as a parameter */
                /* class is really instantiated when class.<init> called*/
                        i = code_get_u2(p + 1,m);
			{
			classinfo *ci;
                        ci = class_getconstant(m->class, i, CONSTANT_Class);
                        /*** s_count++; look for s_counts for VTA */
			/* add marked methods */
			CLASSNAME(ci,"NEW : do nothing",XTA_DEBUGr);
			XTAaddClassInit(m, ci, CLINITS_T, FINALIZE_T,ADDMARKED_T); 
			m->xta->XTAclassSet = add2ClassSet(m->xta->XTAclassSet,ci );
			}
                        break;

                case JAVA_CHECKCAST:
                case JAVA_INSTANCEOF:
                /* class used */
                        i = code_get_u2(p + 1,m);
                        {
                        classinfo *cls =
                                (classinfo *)
		 	     class_getconstant(m->class, i, CONSTANT_Class);
                        LAZYLOADING(cls)
                       	CLASSNAMEop(cls,XTA_DEBUGr);
                        if (cls->classUsed == NOTUSED){
                        	XTAaddClassInit(m, cls,
                                            CLINITS_T,FINALIZE_T,ADDMARKED_T);
				m->xta->XTAclassSet = add2ClassSet(m->xta->XTAclassSet,cls );
                                }
			}
                        break;

		default:
			break;
				
		} /* end switch */

		} /* end for */
        xtaMethodCalls_and_sendReturnType(m);

	return rc;
}

/* Helper fn for initialize **********************************************/

int XTAgetline(char *line, int max, FILE *inFP) {
if (fgets(line, max, inFP) == NULL) 
  return 0;
else
  return strlen((const char *) line);
}

/* Initialize XTA Work list ***********************************************/

/*-- Get meth ptr for class.meth desc and add to XTA worklist --*/
#define SYSADD(cls,meth,desc, is_mono_poly, txt) \
  c = class_new(utf_new_char(cls)); \
  LAZYLOADING(c) \
  callmeth = class_resolveclassmethod(c, \
    utf_new_char(meth), \
    utf_new_char(desc), \
    c, \
    false); \
  if (callmeth->class->classUsed != USED) {  \
      c->classUsed = PARTUSED; \
      XTAaddClassInit(callmeth, callmeth->class, \
		   CLINITS_T,FINALIZE_T,ADDMARKED_T);\
      } \
  callmeth->monoPoly = is_mono_poly; \
  xtaAddCallEdges(NULL, callmeth, is_mono_poly, txt); 

/*-- ---------------------------------------------------------------------------- 
    Initialize XTA work list with methods/classes from:  
      System calls 
	and 
      xtaMissedIn list (missed becaused called from NATIVE &/or dynamic calls
-------------------------------------------------------------------------------*/
methodinfo *initializeXTAworklist(methodinfo *m) {
  	classinfo  *c;
        methodinfo* callmeth;
	char systxt[]    = "System     Call :";
	char missedtxt[] = "xtaMissedIn Call :";

	FILE *xtaMissedIn; /* Methods missed during previous XTA parse */
	char line[256];
	char* class, *meth, *desc;
	methodinfo *rm =NULL;  /* return methodinfo ptr to main method */


        /* Create XTA call work list */
	xtaWorkList = NEW(list);
        list_init(xtaWorkList, OFFSET(xtaNode,linkage) );

	/* Add first method to call list */
       	m->class->classUsed = USED; 
        xtaAddCallEdges(NULL, m, SYSCALL, systxt); 
	/* Add system called methods */
/*** 	SYSADD(mainstring, "main","([Ljava/lang/String;)V", SYSCALL, systxt) ***/
 	SYSADD(MAINCLASS, MAINMETH, MAINDESC, SYSCALL, systxt)
	rm = callmeth;  
/*** 	SYSADD("java/lang/System","exit","(I)V",SYSCALL, systxt) ***/
 	SYSADD(EXITCLASS, EXITMETH, EXITDESC, SYSCALL, systxt)
	/*----- xtaMissedIn 0 */
        if ( (xtaMissedIn = fopen("xtaMissedIn0", "r")) == NULL) {
		/*if (opt_verbose) */
		    {printf("No xtaMissedIn0 file\n");fflush(stdout);} 
		return  rm;
		}
	while (XTAgetline(line,256,xtaMissedIn)) {
	    class = strtok(line, " \n");
	    meth  = strtok(NULL, " \n");
	    desc  = strtok(NULL, " \n");
 		SYSADD(class,meth,desc, POLY, missedtxt)
	        /* ??? Need to hand / hard code who calls it ??? */
		}
	fclose(xtaMissedIn);




	return rm;

}

/*- end initializeXTAworklist-------- */


/*-------------------------------------------------------------------------------*/
methodinfo *missedXTAworklist()  
{
	FILE *xtaMissedIn; /* Methods missed during previous XTA parse */
	char filenameIn[256] = "xtaIn/";
	char line[256];
	char* class, *meth, *desc;
	char* calls_class, *calls_meth, *calls_desc;
	char missedtxt[] = "xtaIn/ missed Call :";
  	classinfo  *c;
        methodinfo* callmeth;

	methodinfo *rm =NULL;  /* return methodinfo ptr to main method */


#if defined(USE_THREADS)
	SYSADD(THREADCLASS, THREADMETH, THREADDESC, SYSCALL, "systxt2")
	SYSADD(THREADGROUPCLASS, THREADGROUPMETH, THREADGROUPDESC, SYSCALL, "systxt2")
#endif
	/*----- xtaMissedIn pgm specific */
        strcat(filenameIn, (const char *)mainstring);  
        if ( (xtaMissedIn = fopen(filenameIn, "r")) == NULL) {
		/*if (opt_verbose)*/ 
		    {printf("No xtaIn/=%s file\n",filenameIn);fflush(stdout);} 
		return rm;
		}
	while (XTAgetline(line,256,xtaMissedIn)) {
	    calls_class = strtok(line, " \n");
	    calls_meth  = strtok(NULL, " \n");
	    calls_desc  = strtok(NULL, " \n");
	    
	    class = strtok(NULL, " \n");
	    meth  = strtok(NULL, " \n");
	    desc  = strtok(NULL, " \n");
	    
	    if ((calls_class == NULL) || (calls_meth == NULL) || (calls_desc == NULL) 
	    ||        (class == NULL) ||       (meth == NULL) ||       (desc == NULL))  
		panic (
		"Error in xtaMissedIn file: Missing a part of calls_class.calls_meth calls calls_desc class.meth desc \n"); 
 	    SYSADD(class,meth,desc, POLY, missedtxt)
	    }
	fclose(xtaMissedIn);

	return rm;
}



/*--------------------------------------------------------*/
/* parseXTAmethod                                          */
/* input: method to be XTA static parsed                  */
/*--------------------------------------------------------*/
void parseXTAmethod(methodinfo *xta_method) {
	if (! (  (xta_method->flags & ACC_NATIVE  )
            ||   (xta_method->flags & ACC_ABSTRACT) ) )	
	    {
	    /* XTA parse to approxmate....
		what classes/methods will really be used during execution */
	    parseXTA(xta_method);  
	    }
	else {
	    if (xta_method->flags & ACC_NATIVE  )
	        {
	       METHINFOt(xta_method,"TO BE NATIVE XTA PARSED :",XTA_DEBUGopcodes);
		/* parseXTApseudo(xta_method); */
	        }   
	    else {
	       printf("Abstract method in XTA Work List: ");
	       METHINFOx(xta_method);
	       panic("Abstract method in XTA Work List.");
               }
            }            	
}


/*-- XTA -- *******************************************************/
int XTA_jit_parse(methodinfo *m)
{
  xtaNode    *xta;
  methodinfo *mainmeth;

  /* Should only be called once */
  if (firstCall) {

    /*----- XTA initializations --------*/
    if (opt_verbose) 
      	    log_text("XTA static analysis started.\n");

    mainmeth = initializeXTAworklist(m);
    firstCall = false; /* turn flag off */

    if ( (xtaMissed = fopen("xtaMissed", "w")) == NULL) {
        printf("CACAO - xtaMissed file: cant open file to write\n");
        }
    /* Note: xtaMissed must be renamed to xtaMissedIn to be used as input */
	
    /*------ process XTA call work list --------*/
    for (xta =list_first(xtaWorkList); 
	 xta != NULL; 
	 xta =list_next(xtaWorkList,xta)) 
        { 
	parseXTAmethod(xta->method);
    	}	
    missedXTAworklist();  
    for (xta =list_first(xtaWorkList); 
	 xta != NULL; 
	 xta =list_next(xtaWorkList,xta)) 
        { 
	parseXTAmethod(xta->method);
    	}	

    fclose(xtaMissed);
    if (opt_verbose) {
        if (opt_stat) {
          printf("printXTAhierarchyInfo(m); not yet there\n");
	  }
      printf("printCallgraph(xtaWorkList); should be called here\n");
      }

    if (opt_verbose) {
      log_text("XTA static analysis done.\n");
      }
  }
return 0;
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

