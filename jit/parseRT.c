/* jit/parseRT.c - parser and print functions for Rapid Type Analyis

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

   Authors: Carolyn Oates

   $Id: parseRT.c 1203 2004-06-22 23:14:55Z twisti $

Changes:
opcode put into functions
changed class_findmethod class_fetchmethod

*/


#include "parseRT.h"

 
/*------------ global variables -----------------------------------------*/
#define MAXCALLGRAPH 5000

#include "parseRTflags.h"


bool NATIVE = false;  // Dez version chasing Native problems

int callgraphAddedBy[MAXCALLGRAPH];   // add comment

int methRT = 0;
int methRTlast = -1;
int methRTmax = MAXCALLGRAPH;
methodinfo **callgraph;
/*methodinfo *callgraph[MAXCALLGRAPH];*/ 
	
 
int methXTA = 0;            
int methXTAlast = -1;
int methXTAmax = MAXCALLGRAPH;        
methodinfo **XTAcallgraph;
/*methodinfo *XTAcallgraph[MAXCALLGRAPH];*/

static bool nativecallcompdone=0 ;

static bool firstCall= true;
static bool parse1 = true;
static bool AfterMain = false;
static FILE *rtMissed;   /* Methods missed during RTA parse of Main  */
/*   so easier to build dynmanic calls file */
static FILE *appldynm;  /* Methods dynamically loaded by application */

static utf *utf_OBJECT;   /*  utf_new_char("Object"); */
static utf *utf_MAIN;   /*  utf_new_char("main"); */
static utf *INIT    ;   /*  utf_new_char("<init>"); */
static utf *CLINIT  ;   /*  utf_new_char("<clinit>"); */
static utf *FINALIZE;   /*  utf_new_char("finalize"); */
static utf *EMPTY_DESC; /*  utf_new_char("V()");  */
static int missedCnt = 0;


/*--------------------------------------------------------------*/
/* addToCallgraph - adds to RTA callgraph and                   */ 
/*                  sets  meth->methodUsed  to USED             */
/*--------------------------------------------------------------*/  
//  if ((meth->methodUsed != USED) && (!(meth->flags & ACC_ABSTRACT)) ) { 
#define ADDTOCALLGRAPH(meth)  if ((meth->methodUsed != USED) && (!(meth->flags & ACC_ABSTRACT)) ) { \
	if (opt_rt) {  \
	callgraph[++methRTlast] = meth ; \
	meth->methodUsed = USED; \
			if(pWhenMarked>=1) \
			        {printf("\n Added to Call Graph #%i:",  \
			                 methRTlast); \
			        printf("\t <used flags c/m> <%i/%i> %i\t",  \
			          meth->class->classUsed, \
			          meth->methodUsed, \
			          USED);  \
			        printf(" method name =");   \
			        utf_display(meth->class->name);printf("."); \
			        method_display(meth);fflush(stdout);} \
	} } 

/*--------------------------------------------------------------*/
bool rtaSubUsed(classinfo *class, methodinfo *meth) {
	classinfo *subs;

	for (subs=class->sub; subs != NULL; subs = subs->nextsub) {
		if (subs->classUsed == USED) {
			if (class_findmethod_w(class, meth->name, meth->descriptor,"rtaSubUsed") == NULL)
				return false;
			else 	
				return true;
		}
		if (rtaSubUsed(subs, meth)) 
			return false;
	}
	return false;
}


/*--------------------------------------------------------------*/
/* Mark the method with same name /descriptor in topmethod      */
/* in class                                                     */
/*                                                              */
/* Class not marked USED and method defined in this class ->    */
/*    -> if Method NOTUSED mark method as MARKED                */
/* Class marked USED and method defined in this class ->        */
/*    -> mark method as USED                                    */
/*                                                              */
/* Class USED, but method not defined in this class ->          */
/*    -> search up the heirarchy and mark method where defined  */
/*       if class where method is defined is not USED ->        */
/*	 -> mark class with defined method as PARTUSED          */
/*--------------------------------------------------------------*/

void rtaMarkMethod(classinfo *class, methodinfo *topmethod) {

	utf *name = topmethod -> name; 
	utf *descriptor = topmethod -> descriptor;
	methodinfo *submeth;

	submeth = class_resolvemethod(class, name, descriptor); 
	if (submeth == NULL)
		panic("parse RT: Method not found in class hierarchy");
	if (submeth->methodUsed == USED) return;
  
	if (submeth->class == class) { 

		/*--- Method defined in class -----------------------------*/
    	if (submeth->class->classUsed != USED) { 
			if (submeth->methodUsed == NOTUSED) { 

                /* Class NOT marked USED and method defined in this class -> */
				/*    -> if Method NOTUSED mark method as  MARKED            */
				if (pWhenMarked >= 1) {
					printf("MARKED class.method\t"); 
					utf_display(submeth->class->name);printf(".");method_display(submeth);
				}
				if (rtaSubUsed(submeth->class,submeth)) {
					submeth->class->classUsed = PARTUSED;
					ADDTOCALLGRAPH(submeth) 
						}
				else	{
					submeth->methodUsed = MARKED;
					RTAPRINTmarkMethod1
						}
    		} }
    	else 	{
			/* Class IS  marked USED and method defined in this class -> */
			/*    -> mark method as USED  */
			ADDTOCALLGRAPH(submeth) 
				}
	} /* end defined in class */

	else {
		/*--- Method NOT defined in class -----------------------------*/
		if (submeth->class->classUsed == NOTUSED) {
			submeth->class->classUsed = PARTUSED;
			if (class->classUsed != USED) {
				submeth->methodUsed = MARKED;
			}
		}
		if ( (submeth->class->classUsed == USED) 
			 || (class->classUsed == USED)) {
			ADDTOCALLGRAPH(submeth)
				}
	} /* end NOT defined in class */
} 

/*-------------------------------------------------------------------------------*/
/* Mark the method with the same name and descriptor as topmethod                */
/*   and any subclass where the method is defined and/or class is used           */
/*                                                                               */
/*-------------------------------------------------------------------------------*/
void rtaMarkSubs(classinfo *class, methodinfo *topmethod) {
		RTAPRINTmarkSubs1
	rtaMarkMethod(class, topmethod);   /* Mark method in class where it was found */
	if (class->sub != NULL) {
		classinfo *subs;
	
		if (!(topmethod->flags & ACC_FINAL )) {
			for (subs = class->sub;subs != NULL;subs = subs->nextsub) {
					RTAPRINTmarkSubs1
				rtaMarkSubs(subs, topmethod); 
				}
			}
		}
	return;
}

/*-------------------------------------------------------------------------------*/
/* Add Marked methods for input class ci                                         */
/* Add methods with the same name and descriptor as implemented interfaces       */
/*   with the same method name                                                   */
/*                                                                               */
/*-------------------------------------------------------------------------------*/
void rtaAddMarkedMethods(classinfo *ci) {
	int ii,jj,mm;

	/* add marked methods to callgraph */ 
	for (ii=0; ii<ci->methodscount; ii++) { 
		methodinfo *mi = &(ci->methods[ii]);
		if (mi->methodUsed == MARKED) { 
			if (pWhenMarked >= 1) {
				printf("ADDED a method that was MARKED\n");
				}
			ADDTOCALLGRAPH(mi)  
			}
		else {
// from Dez                         if (NATIVE == true) { printf("NOT MARKED: "); method_display(mi);}

			for (jj=0; jj < ci -> interfacescount; jj++) {
				classinfo *ici = ci -> interfaces [jj];
				/*  use resolve method....!!!! */
				if (ici -> classUsed != NOTUSED) {
					for (mm=0; mm< ici->methodscount; mm++) {
						methodinfo *imi = &(ici->methods[mm]);

						if  (  	   (imi->methodUsed == USED) 
								   &&	 ( (imi->name == mi->name) 
										   && 	   (imi->descriptor == mi->descriptor))) {
							if (pWhenMarked >= 1) 
								printf("ADDED a method that was used by an interface\n");
							ADDTOCALLGRAPH(mi)  
								}
					}
				}
			}
		}
	}
}    
/*-------------------------------------------------------------------------------*/
/*  XTA Functions                                                                */
/*-------------------------------------------------------------------------------*/

xtainfo *xtainfoInit(methodinfo *m)
{
	if (m->xta != NULL)
		return m->xta;
	m ->xta = (xtainfo *) NEW(xtainfo); 
	m ->xta-> XTAmethodUsed = NOTUSED;
	m ->xta-> XTAclassSet   = NULL;
	m ->xta-> XTAclassSet   = add2ClassSet ( m ->xta-> XTAclassSet, m->class);

	/* PartClassSet */
	m->xta->paramClassSet = NULL;
	m->xta->calls         = NULL;
	m->xta->calledBy      = NULL;

	m->xta->marked        = NULL; /* comment out*/
	m ->xta->markedBy     = NULL; 
	m->xta->fldsUsed      = NULL;
	/*m ->xta->interfaceCalls    = NULL*/
	m->xta->chgdSinceLastParse = false;
	return m->xta;
}


xtafldinfo * xtafldinfoInit (fieldinfo *f)
{
	if (f->xta != NULL)
		return f->xta;

	f->xta = NEW(xtafldinfo);

	f->xta->fieldChecked = false;   /*XTA*/
	f->xta->fldClassType = NULL;    /*XTA*/
	f->xta->XTAclassSet = NULL;     /*XTA*/

	return f->xta;
}


bool xtaPassParams (methodinfo *SmCalled, methodinfo *SmCalls, methSetNode *lastptrInto)
{
	classSetNode *p;
	classSetNode *c;
	classSetNode *c1;
	classSetNode *cprev;
	bool          rc = false;

	if (XTAdebug >= 1) {
		printf("\n>>>>>>>>>>>>>>>>><<<xtaPassParams \n");fflush(stdout);

		printf("\tIN SmCalled set : "); 
		utf_display(SmCalled->class->name);printf("."); method_display(SmCalled);
		printClassSet(SmCalled->xta->XTAclassSet); printf("\n"); 

		printf("\tIN SmCalls set: "); 
		utf_display(SmCalls->class->name);printf("."); method_display(SmCalls);
		printClassSet(SmCalls->xta->XTAclassSet); printf("\n"); 
		
		printf("\tIN lastptrInto : (");
		if (lastptrInto->lastptrIntoClassSet2 != NULL) {
			utf_display(lastptrInto->lastptrIntoClassSet2->classType->name); printf(") ");
		}
		else {printf("NULL) ");}
		fflush(stdout);
		utf_display(lastptrInto->methRef->class->name);printf("."); fflush(stdout);
		method_display(lastptrInto->methRef); fflush(stdout);
		printf("\n");fflush(stdout);
	}

	/* Get SmCalled ParamType set if null */
	if (SmCalled->xta->paramClassSet == NULL) {
		SmCalled->xta->paramClassSet = descriptor2typesL(SmCalled); 
	}
	if (XTAdebug >= 1) {
		printf("\tParamPassed\n"); fflush(stdout);
		printSet(SmCalled->xta->paramClassSet);fflush(stdout);
		printf("\n"); fflush(stdout);
	}

	if (lastptrInto->lastptrIntoClassSet2 == NULL) {
		if (SmCalls->xta->XTAclassSet != NULL) 
			c1 = SmCalls->xta->XTAclassSet->head;
		else
			c1 = NULL;
	}
	else	{
		/* start with type where left off */
		c1 = lastptrInto->lastptrIntoClassSet2;  
		c1 = c1 -> nextClass;  /* even if NULL */
	}
	cprev = NULL;
	if (XTAdebug >= 1) {
		if (c1 == NULL){
   			printf("\tIN SmCalls ... start with NULL\n"); fflush(stdout);
		}
		else  	{
   			printf("\tIN SmCalls ... start with :");fflush(stdout);
			utf_display(c1->classType->name); printf("\n");
		}
	}

	/* for each Param Class */
	for (	p=SmCalled->xta->paramClassSet; p != NULL; p = p->nextClass) {

		/* for each SmCalls class */
		for (c=c1; c != NULL; c = c->nextClass) {
			vftbl *p_cl_vt = p->classType->vftbl; 
			vftbl *c_cl_vt = c->classType->vftbl; 

			/* if SmCalls class is in the Params Class range */
			if (  (p_cl_vt->baseval <=  c_cl_vt->baseval)
				  && (c_cl_vt->baseval <= (p_cl_vt->baseval+p_cl_vt->diffval)) ) {

				/*    add SmCalls class to SmCalledBy Class set */
				SmCalled->xta->XTAclassSet = SmCalled->xta->XTAclassSet = add2ClassSet(SmCalled->xta->XTAclassSet, c->classType); 
				rc = true;
			}
			cprev = c;
		}	
	}
	lastptrInto->lastptrIntoClassSet2 = cprev;
	if (XTAdebug >= 1) {
		printf("\tOUT SmCalled set: ");fflush(stdout);
		printClassSet(SmCalled->xta->XTAclassSet);fflush(stdout);

		printf("\tOUT SmCalls set: ");fflush(stdout);
		printClassSet(SmCalls->xta->XTAclassSet);fflush(stdout);

		printf("\tOUT  lastptrInto="); fflush(stdout);
		if (lastptrInto->lastptrIntoClassSet2 != NULL)
			utf_display(lastptrInto->lastptrIntoClassSet2->classType->name);

		printf("<rc=%i>\n",rc);fflush(stdout);
	}
	return rc;
}

/*-------------------------------------------------------------------------------*/
bool xtaPassReturnType(methodinfo *SmCalled, methodinfo *SmCalls) {

	classSetNode* cs;
	classSetNode* cs1;
	bool          rc = false;

	if (XTAdebug >= 1)
		printf("xtaPassReturnType \n");

	/* Get SmCalled return class is null */
	if ((SmCalled->returnclass == NULL) && (SmCalled->xta->paramClassSet == NULL)) {
		SmCalled->xta->paramClassSet = descriptor2typesL(SmCalled); 
	}

	if (SmCalled->returnclass == NULL) {
		if (XTAdebug >= 1)
			printf("\tReturn type is NULL\n");
		return rc;
	}
	
	if (XTAdebug >= 1) {
		printf("\tReturn type is: ");
		utf_display(SmCalled->returnclass->name);
		printf("\n");

		printf("\tIN SmCalls set: ");
		utf_display(SmCalls->class->name); printf("."); method_display(SmCalls);
		printClassSet(SmCalls->xta->XTAclassSet);

		printf("\tIN SmCalled set: ");
		utf_display(SmCalled->class->name); printf("."); method_display(SmCalled);
		printClassSet(SmCalled->xta->XTAclassSet);
	}


	if (SmCalled->xta->XTAclassSet == NULL) 
		cs1 = NULL;
	else
		cs1 =  SmCalled->xta->XTAclassSet->head;
	for (cs =cs1; cs != NULL; cs = cs->nextClass) {
		classinfo *c = cs->classType;
		vftbl *r_cl_vt = SmCalled->returnclass->vftbl; 
		vftbl *c_cl_vt = c->vftbl; 

		/* if class is a subtype of the return type, then add to SmCalls class set (ie.interscection)*/
		if (  (r_cl_vt->baseval <=  r_cl_vt->baseval)
			  && (c_cl_vt->baseval <= (r_cl_vt->baseval+r_cl_vt->diffval)) ) {
			SmCalls->xta->XTAclassSet = add2ClassSet(SmCalls->xta->XTAclassSet, c);  
			rc = true;
		}
	} 

	if (XTAdebug >= 1) {
		printf("\tOUT SmCalls set: ");
		printClassSet(SmCalls->xta->XTAclassSet);
	}
	return rc;
}

/*-------------------------------------------------------------------------------*/
void xtaAddCallEdges(methodinfo *mi, s4 monoPoly) {

	if (mi->xta == NULL)
		mi->xta = xtainfoInit(mi);
	if (mi->xta->XTAmethodUsed  != USED) {  /* if static method not in callgraph */
		mi->xta->XTAmethodUsed = USED;
		if (!(mi->flags & ACC_ABSTRACT)) { 
			if (mi->methodUsed != USED) {
				XTAcallgraph[++methXTAlast] = mi;
	                        mi->methodUsed = USED;
                                        /*RTprint*/ if (pClassHeir >= 1) {
                                                XTAPRINTcallgraph2
                                                }
                                }
			}

		}
			/*RTAprint*/ if(pWhenMarked>=1) {  
				/*RTAprint*/ printf("\nxxxxxxxxxxxxxxxxx XTA set Used or Added to Call Graph #%i:", 
				/*RTAprint*/ 	   methXTAlast); 
				/*RTAprint*/ printf(" method name ="); fflush(stdout);
				/*RTAprint*/ utf_display(mi->class->name);fflush(stdout); printf(".");fflush(stdout); 
				/*RTAprint*/ method_display(mi);fflush(stdout); 
				/*RTAprint*/ printf("\t\t\t\tcalledBy:");
				/*RTAprint*/ utf_display(rt_method->class->name);fflush(stdout); printf(".");fflush(stdout); 
				/*RTAprint*/ method_display(rt_method);fflush(stdout); 
        			/*RTAprint*/ }
	/* add call edges */
	rt_method->xta->calls = add2MethSet(rt_method->xta->calls, mi);
	rt_method->xta->calls->tail->monoPoly = monoPoly;
	mi->xta->calledBy     = add2MethSet(mi->xta->calledBy,     rt_method); 
	if (mi->xta->calledBy     == NULL) panic("mi->xta->calledBy is NULL!!!");
	if (rt_method->xta->calls == NULL) panic("rt_method->xta->calls is NULL!!!");
}


/*--------------------------------------------------------------*/
bool xtaSubUsed(classinfo *class, methodinfo *meth, classSetNode *subtypesUsedSet) {
	classinfo *subs;

	for (subs=class->sub; subs != NULL; subs = subs->nextsub) {
		/* if class used */
		if (inSet(subtypesUsedSet,subs)) {
			if (class_findmethod_w(class, meth->name, meth->descriptor, "xtaSubUsed") == NULL) 
				return false;
			else 	{
				if (class_findmethod_w(subs, meth->name, meth->descriptor, "xtaSubUsed") == NULL) 
					return true;
				}
		}
		if (xtaSubUsed(subs, meth,  subtypesUsedSet)) 
			return false;
	}
	return false;
}


/*-------------------------------------------------------------------------------*/
void xtaMarkMethod(classinfo *class, methodinfo *topmethod, classSetNode *subtypesUsedSet)
{
	methodinfo *submeth;

	utf *name = topmethod -> name;
	utf *descriptor = topmethod -> descriptor;
	/****/
		 printf("xtaMarkMethod for:"); utf_display(class->name);fflush(stdout); 
		 method_display(topmethod);
	/**/

	submeth = class_resolvemethod(class, name, descriptor);

	/***/
                printf(" def: "); utf_display(submeth->class->name); printf("\n");fflush(stdout);
	/****/

	/* Basic checks */
	if (submeth == NULL)
	        panic("parse XTA: Method not found in class hierarchy");
	if (submeth->xta == NULL) 
		submeth->xta = xtainfoInit(submeth);

	if (rt_method->xta->calls != NULL) {
		if (inMethSet(rt_method->xta->calls->head,submeth)) return;
	}
	/*----*/
	if (submeth->class == class) {

        /*--- Method defined in class -----------------------------*/
		if (inSet(subtypesUsedSet,submeth->class)) {
			xtaAddCallEdges(submeth,POLY);	
		}
		else	{
			if (subtypesUsedSet != NULL) {	
				if (xtaSubUsed (class,submeth,subtypesUsedSet)) {
					xtaAddCallEdges(submeth,POLY);
					return; //Dez
				}
                        submeth->xta->markedBy = add2MethSet(submeth->xta->markedBy,rt_method);
                                /****
                                        printf("\t(defd) Added to MarkBy Set\n"); fflush(stdout);
                                        utf_display(submeth->class->name); printf("."); fflush(stdout);
                                        method_display(submeth);
                                        printMethodSet(submeth->xta->markedBy);
                                ****/

			}
		}
	}
	else  {
        /*--- Method NOT defined in class -----------------------------*/
		if (!(inSet(subtypesUsedSet,submeth->class) )){  /* class with method def     is not used */
			if (!(inSet(subtypesUsedSet,class) )) { /* class currently resolving is not used */ 
                                submeth->xta->markedBy = add2MethSet(submeth->xta->markedBy,rt_method);
                                submeth->xta->markedBy = add2MethSet(submeth->xta->markedBy,rt_method);
                                /****
                                        printf("\t(^^defd) Added to MarkBy Set\n"); fflush(stdout);
                                        utf_display(submeth->class->name); printf("."); fflush(stdout);
                                        method_display(submeth);
                                        printMethodSet(submeth->xta->markedBy);
                                ****/
			}
		}
		if ( (inSet(subtypesUsedSet,submeth->class))  /* class with method def     is used */
			 || (inSet(subtypesUsedSet,class)) ) {       /* class currently resolving is used */ 
			xtaAddCallEdges(submeth,POLY);
		}

	} /* end defined in class */

}
/*-------------------------------------------------------------------------------*/
void xtaMarkSubs(classinfo *class, methodinfo *topmethod, classSetNode *subtypesUsedSet) {
	/* xtaPRINTmarkSubs1*/
	xtaMarkMethod(class, topmethod,subtypesUsedSet);   /* Mark method in class where it was found */
	if (class->sub != NULL) {
		classinfo *subs;

		if (!(topmethod->flags & ACC_FINAL )) {
			for (subs = class->sub; subs != NULL; subs = subs->nextsub) {
				/* xtaPRINTmarkSubs1 */
				xtaMarkSubs(subs, topmethod, subtypesUsedSet);
				}
			}
    		}
}

/*-------------------------------------------------------------------------------*/
/* Add Marked methods for input class ci                                         */
/* Add methods with the same name and descriptor as implemented interfaces       */
/*   with the same method name                                                   */
/*                                                                               */
/*-------------------------------------------------------------------------------*/
void xtaAddMarkedMethods(classinfo *ci) {
        int ii,jj,mm;

        /* add marked methods to callgraph */
        for (ii=0; ii<ci->methodscount; ii++) {
                methodinfo *mi = &(ci->methods[ii]);
          if (mi->xta != NULL) {
                if (mi->xta->markedBy != NULL) {
                        methSetNode *mcnode;
                        for (mcnode = mi->xta->markedBy->head; mcnode  != NULL; mcnode  = mcnode ->nextmethRef) {
                                methodinfo *mc = mcnode->methRef;
                                if (pWhenMarked >= 1) {
                                        printf("ADDED a method that was MARKED\n");
                                        }
                                xtaAddCallEdges(mi,POLY);
                                }
                        }
                {
                        for (jj=0; jj < ci -> interfacescount; jj++) {
                                classinfo *ici = ci -> interfaces [jj];
                                /*  use resolve method....!!!! */
                                if (ici -> classUsed != NOTUSED) {
                                        for (mm=0; mm< ici->methodscount; mm++) {
                                                methodinfo *imi = &(ici->methods[mm]);
                                                if (imi->xta != NULL) {
                                                if      ((imi->xta->XTAmethodUsed == USED)
                                                           &&    ( (imi->name == mi->name)
                                                           &&      (imi->descriptor == mi->descriptor))) {
                                                        if (pWhenMarked >= 1)
                                                                printf("ADDED a method that was used by an interface\n");
                                                        xtaAddCallEdges(mi,POLY);
                                                        }
                                                        }
                                                }
                                        }
                                }
                        }
                  }
                }
}

/*-------------------------------------------------------------------------------*/
/* Both RTA and XTA */
/*-------------------------------------------------------------------------------*/

int addClassInit(classinfo *ci, bool clinits, bool finalizes, bool addmark) {
	/* CHANGE to a kind of table look-up for a list of class/methods (currently 3)
	 */

	utf* utf_java_lang_system = utf_new_char("java/lang/System"); 
	utf* utf_initializeSystemClass = utf_new_char("initializeSystemClass"); 
	utf* utf_java_lang_Object = utf_new_char("java/lang/Object"); 

	int m, m1=-1, m2=-1, mf=-1;
	methodinfo *mi;

	if (clinits) { /* No <clinit>  available - ignore */  

		/* Get clinit methodinfo ptr */
		if ((mi = class_resolvemethod(ci,CLINIT, EMPTY_DESC)) !=NULL) {
			if (mi->class != ci) {
				printf("WARNING:::::<clinit> method not found in class requested: ");
				utf_display(ci->name); printf(" found in:"); utf_display(mi->class->name);
				printf("\n");
				}
			/*--- RTA ---*/
			if ( mi->methodUsed != USED) {
				mi->class->classUsed = PARTUSED;  
				ADDTOCALLGRAPH(mi)  
				}

			/*--- XTA ---*/
			if ((XTAOPTbypass) || (opt_xta)) {
				xtaAddCallEdges(mi,MONO); 
				}
			}
		else	{
			printf("class=");utf_display(ci->name);
			printf(" super="); 
			if (ci->super != NULL)
				utf_display(ci->super->name);
			else
				printf("NULL");
			if ( (ci->super->name != utf_OBJECT) && (ci->name != utf_OBJECT)  )
				panic("<clinit> method not found");
			}
		}

	if (finalizes) {   

		/* Get finalize methodinfo ptr */
		if ( (mi = class_findmethod (ci,FINALIZE, EMPTY_DESC)) != NULL) { 
			/*--- RTA ---*/
			if ( mi->methodUsed != USED) {
				mi->class->classUsed = PARTUSED;  
				ADDTOCALLGRAPH(mi)  
				}

			/*--- XTA ---*/
			if ((XTAOPTbypass) || (opt_xta)) {
				xtaAddCallEdges(mi,MONO); 
				}
			}
		}

	/*Special Case for System class init:  
	add java/lang/initializeSystemClass to callgraph */
	/* XXX TWISTI */
/*  	if (class->name == utf_initializeSystemClass) { */
	if (mi->class->name == utf_initializeSystemClass) {
		/* Get clinit methodinfo ptr */
		if ((mi = class_findmethod (ci,utf_initializeSystemClass, EMPTY_DESC)) != NULL) {
			/*--- RTA ---*/
			if ( mi->methodUsed != USED) {
				mi->class->classUsed = PARTUSED;
				ADDTOCALLGRAPH(mi)  
				}

			/*--- XTA ---*/
			if ((XTAOPTbypass) || (opt_xta)) {
				xtaAddCallEdges(mi,MONO);
				}
			}
		}
	if (addmark) {
		/* add marked methods to callgraph */ 
	       	if ((XTAOPTbypass) || (opt_xta))
        	        xtaAddMarkedMethods(ci);
        	else
                	rtaAddMarkedMethods(ci);
		}
		
	return m;
} 


#define rt_code_get_u1(p)  rt_jcode[p]
#define rt_code_get_s1(p)  ((s1)rt_jcode[p])
#define rt_code_get_u2(p)  ((((u2)rt_jcode[p])<<8)+rt_jcode[p+1])
#define rt_code_get_s2(p)  ((s2)((((u2)rt_jcode[p])<<8)+rt_jcode[p+1]))
#define rt_code_get_u4(p)  ((((u4)rt_jcode[p])<<24)+(((u4)rt_jcode[p+1])<<16)\
                           +(((u4)rt_jcode[p+2])<<8)+rt_jcode[p+3])
#define rt_code_get_s4(p)  ((s4)((((u4)rt_jcode[p])<<24)+(((u4)rt_jcode[p+1])<<16)\
                           +(((u4)rt_jcode[p+2])<<8)+rt_jcode[p+3]))



/*-------------------------------------------------------------------------------*/
void rtaAddUsedInterfaceMethods(classinfo *ci) {
	int jj,mm;

	/* add used interfaces methods to callgraph */
	for (jj=0; jj < ci -> interfacescount; jj++) {
		classinfo *ici = ci -> interfaces [jj];
	
		if (pWhenMarked >= 1) { 
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
				if (pWhenMarked >= 1) { 
					if  (imi->methodUsed != USED) {
						if (imi->methodUsed == NOTUSED) printf("Interface Method notused: "); 
						if (imi->methodUsed == MARKED) printf("Interface Method marked: "); 
						utf_display(ici->name);printf(".");method_display(imi);fflush(stdout);
					}
				} 
				if  (imi->methodUsed == USED) {
					if (pWhenMarked >= 1) { 
						printf("Interface Method used: "); utf_display(ici->name);printf(".");method_display(imi);fflush(stdout);

						/* Mark this method used in the (used) implementing class and its subclasses */
						printf("rMAY ADD methods that was used by an interface\n");
					}
					rtaMarkSubs(ci,imi);
				}
			}
		}
	}

}
/*-------------------------------------------------------------------------------*/
void rtaMarkInterfaceSubs(methodinfo *mi) {				
	classSetNode *subs;
	if (mi->class->classUsed == NOTUSED) {
		mi->class->classUsed = USED; 
		class_java_lang_Object->impldBy =  addElement(class_java_lang_Object -> impldBy,  mi->class);
		}

	/* add interface class to list kept in Object */
	mi->methodUsed = USED;
	mi->monoPoly   = POLY;

	subs =  mi->class->impldBy; 
						RTAPRINT08invokeInterface1
	while (subs != NULL) { 			
		classinfo * isubs = subs->classType;
							RTAPRINT09invokeInterface2
		/* Mark method (mark/used) in classes that implement the method */
		if (isubs->classUsed != NOTUSED) {
			methodinfo *submeth;
						
			submeth = class_findmethod(isubs,mi->name, mi->descriptor); 
			if (submeth != NULL)
				submeth->monoPoly = POLY; /*  poly even if nosubs */
			rtaMarkSubs(isubs, mi);  
			}
				
		subs = subs->nextClass;
		} /* end while */
} 

/*-------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------*/



/*-------------------------------------------------------------------------------*/
void xtaAddUsedInterfaceMethods(classinfo *ci) {
	int jj,mm;

/* add used interfaces methods to callgraph */
for (jj=0; jj < ci -> interfacescount; jj++) {
	classinfo *ici = ci -> interfaces [jj];
	
	if (pWhenMarked >= 1) { 
			printf("XInterface used: ");fflush(stdout); 
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
			methodinfo *imi = &(ici->methods[mm]);  /*interface method */
printf("==%i==%i\n",ici->methodscount,mm);
			if (imi->xta == NULL)
				xtainfoInit (imi); 
					/*RTAprint*/if (pWhenMarked >= 1) { 
					/*RTAprint*/  if  (imi->xta->XTAmethodUsed != USED) {
					/*RTAprint*/    if (imi->xta->XTAmethodUsed==NOTUSED) 
								printf("Interface Method notused: "); 
					/*RTAprint*/    if (imi->xta->XTAmethodUsed==MARKED) 
								printf("Interface Method marked: "); 
					/*RTAprint*/    utf_display(ici->name);printf(".");
					/*RTAprint*/    method_display(imi);fflush(stdout);
					/*RTAprint*/    }
					/*RTAprint*/  } 
			if  (imi->xta->XTAmethodUsed == USED) {
				methSetNode *mCalledBy = NULL;
					if (pWhenMarked >= 1) { 
						printf("Interface Method used: "); utf_display(ici->name);printf(".");
						method_display(imi);fflush(stdout);

						/* Mark this method used in the (used) implementing class &its subclasses */
						printf("xMAY ADD methods that was used by an interface\n"); fflush(stdout);
						}
                                        if (pWhenMarked >= 1) {
                                                printf("calledBy set ="); fflush(stdout);
                                                printMethodSet(imi->xta->calledBy);
                                                }
				if (imi->xta->calledBy != NULL) { 
					/* for each calledBy method */
					for (	mCalledBy = imi->xta->calledBy->head; 
						mCalledBy != NULL; 
						mCalledBy = mCalledBy->nextmethRef) {
						if (pWhenMarked >= 1) {
								printf("xtaMarkSubs(");
								utf_display(ci->name); printf("."); fflush(stdout);
								method_display(imi);
								printf("mCalledBy method class set BEFORE\n"); fflush(stdout);
								printSet(mCalledBy->methRef->xta->XTAclassSet->head);
						}
						xtaMarkSubs(ci,imi,mCalledBy->methRef->xta->XTAclassSet->head);
							if (pWhenMarked >= 1) {
								printf("mCalledBy method class set AFTER \n"); fflush(stdout);
								printSet(mCalledBy->methRef->xta->XTAclassSet->head);
							}
						}
					}
				}
			} 
		}
	} /* end for */
}

/*-------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------*/
void xtaMarkInterfaceSubs(methodinfo *mi) {
	classSetNode *subs;
	classSetNode * Si;

	if (mi->xta == NULL)
		xtainfoInit (mi); 
					
	if (mi->class->classUsed != USED) {
		mi->class->classUsed = USED; 
		class_java_lang_Object->impldBy =  addElement(class_java_lang_Object -> impldBy,  mi->class);
		}

	/* add interface class to list kept in Object */
				if (pWhenMarked >= 1) {
					printf("Marking Interface Method: "); fflush(stdout);
					}
	xtaAddCallEdges(mi,POLY);	

	subs =  mi->class->impldBy; 
						RTAPRINT08invokeInterface1
	while (subs != NULL) { 
		classinfo * isubs = subs->classType;
							RTAPRINT09invokeInterface2
		/* Mark method (mark/used) in classes that implement the method */
		if (isubs->classUsed != NOTUSED) {
			methodinfo *submeth;
						
			submeth = class_resolvemethod(isubs,mi->name, mi->descriptor); 
			if (submeth != NULL)    ///+1
				{
		               	classSetNode *subtypesUsedSet = NULL;
				submeth->monoPoly = POLY; /*  poly even if nosubs */
							
				mi->xta->XTAmethodUsed = USED;
                		if (rt_method->xta->XTAclassSet != NULL)
                			subtypesUsedSet =
                    				intersectSubtypesWithSet
                        				(subs->classType, rt_method->xta->XTAclassSet->head);

							if (pWhenMarked >= 1) {
                                   				/*RTAprint*/ printf(" \nXTA subtypesUsedSet: ");
                                                		/*RTAprint*/ fflush(stdout);
                                                		/*RTAprint*/ printSet(subtypesUsedSet);
							}
		                xtaMarkSubs(subs->classType, mi, subtypesUsedSet);
                		}
            		}
		subs = subs->nextClass;
		}
}
/*-------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------*/
bool xtaAddFldClassTypeInfo(fieldinfo *fi) {

	bool rc = false;

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
			rc = true;
			if  (fi->xta->fldClassType== NULL) {
				char *desc;
				char *cname;
				classinfo * class;

				desc = MNEW(char, 256);
				strcpy(desc,++utf_ptr);
				cname = strtok(desc,";");
				if (XTAdebug >= 1) {
					printf("STATIC fields type is: %s\n",cname);
					fflush(stdout);
				}
				class = class_get(utf_new_char(cname));
				fi->xta->fldClassType= class;    /* save field's type class ptr */	
			} 
		}
	}
	return rc;
}

/*-------------------------------------------------------------------------------*/
void xtaPassFldPUT(fldSetNode *fN)
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
		if (rt_method->xta->XTAclassSet != NULL)
			c1  = rt_method->xta->XTAclassSet->head;

		if (XTAfld >=1 ) {
			printf("rt XTA class set =");fflush(stdout);
			printClassSet(rt_method->xta->XTAclassSet);
			printf("\t\tField class type = ");fflush(stdout);
			utf_display(fi->xta->fldClassType->name); printf("\n");
		}
	}

	/*--- PUTSTATIC specific ---*/
	/* Sx = intersection of type+subtypes(field x)   */
	/*   and Sm (where putstatic code is)            */
	for (c=c1; c != NULL; c=c->nextClass) {
		vftbl *f_cl_vt = fi->xta->fldClassType->vftbl;
		vftbl *c_cl_vt =  c->   classType->vftbl;
		if (XTAfld >=2 ) {
			printf("\tXTA class = ");fflush(stdout);
			utf_display(c->classType->name);
			printf("<b=%i> ",c_cl_vt->baseval); fflush(stdout);
			if (c->nextClass == NULL) {
				printf("next=NULL ");fflush(stdout);
			}
			else	{
				printf("next="); fflush(stdout);
				utf_display(c->nextClass->classType->name);
				printf("\n"); fflush(stdout);
			}

			printf("\t\tField class type = ");fflush(stdout);
			utf_display(fi->xta->fldClassType->name);
			printf("<b=%i/+d=%i> \n",f_cl_vt->baseval,(f_cl_vt->baseval+f_cl_vt->diffval)); fflush(stdout);
		}

		if ((f_cl_vt->baseval <= c_cl_vt->baseval)
			&& (c_cl_vt->baseval <= (f_cl_vt->baseval+f_cl_vt->diffval)) ) {
			fi->xta->XTAclassSet = add2ClassSet(fi->xta->XTAclassSet,c->classType);
		}
		cprev = c;
	}
	fN->lastptrPUT = cprev;
}
/*-------------------------------------------------------------------------------*/
void xtaPassFldGET(fldSetNode *fN)
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

		if (XTAfld >=1 ) {
			printf("fld XTA class set =");fflush(stdout);
			printClassSet(fi->xta->XTAclassSet);
			printf("\t\tField class type = ");fflush(stdout);
			utf_display(fi->xta->fldClassType->name); printf("\n");
		}
	}

	/*--- GETSTATIC specific ---*/
	/* Sm = union of Sm and Sx */
	for (c=c1; c != NULL; c=c->nextClass) {
		bool addFlg = false;
		if (rt_method->xta->XTAclassSet ==NULL) 
			addFlg = true;
		else 	{
			if (!(inSet (rt_method->xta->XTAclassSet->head, c->classType) )) 
				addFlg = true;
			}
		if (addFlg) {
			rt_method->xta->XTAclassSet 
				= add2ClassSet(rt_method->xta->XTAclassSet,c->classType);
		}
		cprev = c;
	}

	fN->lastptrGET = cprev;

}

/*-------------------------------------------------------------------------------*/
void xtaPassAllCalledByParams () {
	methSetNode *SmCalled;
	methSetNode *s1;
	if (XTAdebug >= 1) {
		printf("xta->calledBy method set: "); fflush(stdout);
		
                if (rt_method->xta == NULL) panic ("rt_method->xta == NULL!!!!\n");
		printMethodSet(rt_method->xta->calledBy); fflush(stdout);
		}
	if (rt_method->xta->calledBy == NULL)
		s1 = NULL;
	else
		s1 = rt_method->xta->calledBy->head;
	for (SmCalled=s1; SmCalled != NULL; SmCalled = SmCalled->nextmethRef) {
		if (XTAdebug >= 1) {
			printf("SmCalled = "); fflush(stdout);
			utf_display(SmCalled->methRef->class->name); fflush(stdout);
			printf(".");fflush(stdout); method_display(SmCalled->methRef);
		}
				
		rt_method->xta->chgdSinceLastParse = false;		
		xtaPassParams(rt_method, SmCalled->methRef,SmCalled);  	/* chg flag output ignored for 1st regular parse */
	}
}

/*-------------------------------------------------------------------------------*/
void xtaAllFldsUsed ( ){
	fldSetNode  *f;
	fldSetNode *f1=NULL; 
	/*	bool chgd = false */

	if (rt_method->xta->fldsUsed == NULL) return;

	/* for each field that this method uses */
	f1 = rt_method->xta->fldsUsed->head;

	for (f=f1; f != NULL; f = f->nextfldRef) {

		if (f->writePUT)
			xtaPassFldPUT(f);
		if (f->readGET)
			xtaPassFldGET(f);
	}
}
/*-------------------------------------------------------------------------------*/
void  xtaMethodCalls_and_sendReturnType() 
{
	methSetNode *SmCalled;  /* for return type       */
	methSetNode *SmCalls;   /* for calls param types */
	methSetNode *s1=NULL; 
	bool chgd = false;
	if (XTAdebug >= 1) {
		printf("calls method set Return type: ");
		printMethodSet(rt_method->xta->calls);
	}
	xtaAllFldsUsed ( );

	/* for each method that this method calls */
	if (rt_method->xta->calls == NULL)
		s1 = NULL;
	else
		s1 = SmCalls=rt_method->xta->calls->head;

	for (SmCalls=s1; SmCalls != NULL; SmCalls = SmCalls->nextmethRef) {
		/*    pass param types  */
		bool chgd = false;
		chgd = xtaPassParams (SmCalls->methRef, rt_method, SmCalls);  
		/* if true chgd after its own parse */
		if (!(SmCalls->methRef->xta->chgdSinceLastParse)) {
			SmCalls->methRef->xta->chgdSinceLastParse = true;
		}
	}

	/* for each calledBy method */
	/*    send return type */
	if (rt_method->xta->calledBy == NULL)
		s1 = NULL;
	else
		s1 = rt_method->xta->calledBy->head;
	for (SmCalled=s1; SmCalled != NULL; SmCalled = SmCalled->nextmethRef) {

		if (XTAdebug >= 1) {
			printf("\tSmCalled = ");fflush(stdout); utf_display(SmCalled->methRef->class->name);
			printf("."); method_display(SmCalled->methRef);
		}
				
		chgd = xtaPassReturnType(rt_method, SmCalled->methRef); 
		if (!(SmCalled->methRef->xta->chgdSinceLastParse)) {
			SmCalled->methRef->xta->chgdSinceLastParse = chgd;		
		}
	}
}

/*-------------------------------------------------------------------------------*/
/* -- Processes STATIC  & PRIVATE methods

 * -- called for INVOKESTATIC, INVOKESPECIAL - PRIVATE and
 *    STATIC / PRIVATE methods used by NATIVE methods
 *-------------------------------------------------------------------------------*/

// Dez Version but gives too many Missed
void invokestatic2( methodinfo *mi) {

mi->class->classUsed = PARTUSED;
/*-- RTA --*/
addClassInit(mi->class, true, true, true);
                RTAPRINT04invokestatic1

if (opt_rt) {
        ADDTOCALLGRAPH(mi)
        } /* end RTA */

/*-- XTA --*/
if ((XTAOPTbypass) || (opt_xta)) {
        mi->class->classUsed = PARTUSED;
        rt_method->xta->XTAclassSet = add2ClassSet(rt_method->xta->XTAclassSet,mi->class);
        xtaAddCallEdges(mi,MONO);
        } /* end XTA */
}



//// from just RTA version before Dez changes
void invokestatic(methodinfo *mi){
		RTAPRINT04invokestatic1
if (mi->class->classUsed == NOTUSED) {
	mi->class->classUsed = USED;
			RTAPRINT05invokestatic2
	}
	addClassInit(mi->class,true, true, true);

	if (opt_rt) {
		ADDTOCALLGRAPH(mi)  
		} /* end RTA */
	/*-- XTA --*/
	if ((XTAOPTbypass) || (opt_xta)) {
		xtaAddCallEdges(mi,MONO); 
	} /* end XTA */
}


/*-------------------------------------------------------------------------------*/
/* -- Processes <INIT> methods

 * -- called for INVOKESPECIAL - <init> and
 *    <init> methods used by NATIVE methods
 *-------------------------------------------------------------------------------*/

void initMethods(methodinfo *mi) {

classinfo  *ci = mi->class;

/* new class so add marked methods */
if (opt_rt) {
	if (( mi->methodUsed != USED) || (mi->class->classUsed == PARTUSED))  {
		/*--- process NORMAL <init> method ---------------------------------------------*/
		if ( mi->methodUsed != USED) {
			/* Normal <init> 
			- mark class as USED and <init> to callgraph */

			/*-- RTA --*/
//	addClassInit(mi->class,true, true, false);
			ci->classUsed = USED;
			rtaAddMarkedMethods(ci);  /* add to callgraph marked methods */
					RTAPRINT06Binvoke_spec_init
			rtaAddUsedInterfaceMethods(ci); 
			ADDTOCALLGRAPH(mi)  
			}
		}	
	}

/*-- XTA --*/
if ((XTAOPTbypass) || (opt_xta)) { 
	if (mi->xta == NULL) {
		mi->xta = xtainfoInit(mi);
		}
	if ((mi->xta->XTAmethodUsed != USED) || (mi->class->classUsed == PARTUSED)) {
		ci->classUsed = USED;
		rt_method->xta->XTAclassSet = add2ClassSet(rt_method->xta->XTAclassSet,ci ); 
		xtaAddMarkedMethods(ci);  /* add to callgraph marked methods */
		xtaAddUsedInterfaceMethods(ci); 
		xtaAddCallEdges(mi,MONO);
			RTAPRINT06CXTAinvoke_spec_init1
		} /* end XTA */
	}
}

/*-------------------------------------------------------------------------------*/
/* -- Processes VIRTUAL methods

 * -- called for INVOKEVIRTUAL and 
 *    virtual methods used by NATIVE methods 
 *-------------------------------------------------------------------------------*/

void invokevirtual(methodinfo *mi) {

/*--- RTA ---*/
			RTAPRINT07invoke_spec_virt2
mi->class->classUsed = USED;  // Should this be later?

if ((mi->flags & ACC_STATIC)  || (mi->flags & ACC_PRIVATE)  || (mi->flags & ACC_FINAL) ) {
        invokestatic(mi);
        return;
        }


mi->monoPoly = POLY;

if (opt_rt) { 
	rtaMarkSubs(mi->class,mi); 
	}

/*--- XTA ---*/
if ((XTAOPTbypass) || (opt_xta)) { 
	classSetNode *subtypesUsedSet = NULL;
	if (rt_method->xta->XTAclassSet != NULL)
	subtypesUsedSet = 
		intersectSubtypesWithSet(mi->class, rt_method->xta->XTAclassSet->head);
	else
		subtypesUsedSet = addElement(subtypesUsedSet, rt_method->class);
	/*****/	
	printf(" \nXTA subtypesUsedSet: "); fflush(stdout);
	printSet(subtypesUsedSet);
	/*****/
	xtaMarkSubs(mi->class, mi, subtypesUsedSet);   
	} /* end XTA */
}


/*-------------------------------------------------------------------------------*/
void invokeinterface( methodinfo *mi ) {	
						/*RTAprint*/ if (pWhenMarked >= 1) {
						/*RTAprint*/ printf("\t");fflush(stdout);
						/*RTAprint*/ utf_display(mi->class->name); printf(".");fflush(stdout);
						/*RTAprint*/ method_display(mi); fflush(stdout); 
						/*RTAprint*/ }

if (mi->flags & ACC_STATIC)
	panic ("Static/Nonstatic mismatch calling static method");
						RTAPRINT08AinvokeInterface0
/*--- RTA ---*/
if (opt_rt) {
	rtaMarkInterfaceSubs(mi);
	}
/*--- XTA ---*/
if ((XTAOPTbypass2) || (opt_xta)) {
	xtaMarkInterfaceSubs(mi);
	}
}

/*-------------------------------------------------------------------------------*/
void newClasses(classinfo *ci) {
					if (pWhenMarked >= 1) {
						printf("\tclass=");fflush(stdout);
						utf_display(ci->name); fflush(stdout);
						printf("=\n");fflush(stdout);
						}
/*--- RTA ---*/
if (ci->classUsed != USED) {
					RTAPRINT10new
	ci->classUsed = USED;    /* add to heirarchy    */
	/* Add this class to the implemented by list of the abstract interface */
	rtaAddUsedInterfaceMethods(ci);
	addClassInit(ci, true, true, false);
	} 
/*--- XTA ---*/
if ((XTAOPTbypass) || (opt_xta))
	{
	xtaAddUsedInterfaceMethods(ci);
	rt_method->xta->XTAclassSet = add2ClassSet(rt_method->xta->XTAclassSet,ci ); /*XTA*/
						RTAPRINT10newXTA
	}
}

/*-------------------------------------------------------------------------------*/
static void parseRT(methodinfo *m)
{
	int  p;                     /* java instruction counter                   */
	int  nextp;                 /* start of next java instruction             */
	int  opcode;                /* java opcode                                */
	int  i;                     /* temporary for different uses (counters)    */
	bool iswide = false;        /* true if last instruction was a wide        */

	RTAPRINT01method

		if ( ((XTAOPTbypass) || (opt_xta)) && (rt_method->name != utf_MAIN)) {
  		              		printf("XTA parseRT():"); fflush(stdout);
		                	method_display(rt_method);
                	if (rt_method->xta == NULL)
                        	xtainfoInit (rt_method);
	                xtaPassAllCalledByParams ();
        			        printf("XTA parseRT() after xtaPassAll...\n");fflush(stdout);
   
		}

	/* scan all java instructions */

	for (p = 0; p < rt_jcodelength; p = nextp) {
		opcode = rt_code_get_u1 (p);           /* fetch op code                  */
		RTAPRINT02opcode
			fflush(stdout);	
		nextp = p + jcommandsize[opcode];   /* compute next instruction start */
		switch (opcode) {

			/*--------------------------------*/
			/* Code just to get the correct  next instruction */
			/* 21- 25 */
		case JAVA_ILOAD:
		case JAVA_LLOAD:
		case JAVA_FLOAD:
		case JAVA_DLOAD:

		case JAVA_ALOAD:
			if (iswide)
				{
					nextp = p+3;
					iswide = false;
				}
			break;

			/* 54 -58 */
		case JAVA_ISTORE:
		case JAVA_LSTORE:
		case JAVA_FSTORE:
		case JAVA_DSTORE:

		case JAVA_ASTORE:
			if (iswide)
				{
					iswide=false;
					nextp = p+3;
				}
			break;

			/* 132 */
		case JAVA_IINC:
			{
				if (iswide) {
					iswide = false;
					nextp = p+5;
				}
			}
			break;

			/* wider index for loading, storing and incrementing */
			/* 196 */
		case JAVA_WIDE:
			iswide = true;
			nextp = p + 1;
			break;
			/* 169 */
		case JAVA_RET:
			if (iswide) {
				nextp = p+3;
				iswide = false;
			}
			break;

			/* table jumps ********************************/

		case JAVA_LOOKUPSWITCH:
			{
				s4 num;
				nextp = ALIGN((p + 1), 4);
				num = rt_code_get_u4(nextp + 4);
				nextp = nextp + 8 + 8 * num;
				break;
			}


		case JAVA_TABLESWITCH:
			{
				s4 num;
				nextp = ALIGN ((p + 1),4);
				num = rt_code_get_s4(nextp + 4);
				num = rt_code_get_s4(nextp + 8) - num;
				nextp = nextp + 16 + 4 * num;
				break;
			}

			/*-------------------------------*/
		case JAVA_PUTSTATIC:
			i = rt_code_get_u2(p + 1);
			{
				constant_FMIref *fr;
				fieldinfo *fi;

				fr = class_getconstant (rt_class, i, CONSTANT_Fieldref);
				/* descr has type of field ref'd  */
				fi = class_findfield (fr->class,fr->name, fr->descriptor);
				RTAPRINT03putstatic1

				/*--- RTA ---*/
                                /* class with field - marked in addClassinit */
				addClassInit(fr->class, true, true, false);

				/*--- XTA ---*/
				if   ((XTAOPTbypass) || (opt_xta))
					{
						if (fi->xta == NULL)
							fi->xta = xtafldinfoInit(fi);
						if (xtaAddFldClassTypeInfo(fi)) {  
							rt_method->xta->fldsUsed = add2FldSet(rt_method->xta->fldsUsed, fi, true,false);
						}
					}
			}
			break;

		case JAVA_GETSTATIC:
			i = rt_code_get_u2(p + 1);
			{
				constant_FMIref *fr;
				fieldinfo *fi;

				fr = class_getconstant (rt_class, i, CONSTANT_Fieldref);
				/* descr has type of field ref'd  */
				fi = class_findfield (fr->class,fr->name, fr->descriptor);
				RTAPRINT03putstatic1

				/*--- RTA ---*/
                               	/* class with field - marked in addClassinit */
				addClassInit(fr->class,true, true, true);

				/*--- XTA ---*/
				if  ((XTAOPTbypass) || (opt_xta) ) 
					{
						if (fi->xta == NULL)
							fi->xta = xtafldinfoInit(fi);
						if (xtaAddFldClassTypeInfo(fi)) {
							rt_method->xta->fldsUsed = add2FldSet(rt_method->xta->fldsUsed, fi, false, true);
						}
					}

			}
			break;


			/*--------------------  method invocation ---------------------*/

		case JAVA_INVOKESTATIC:
			i = rt_code_get_u2(p + 1);
			{
				constant_FMIref *mr;
				methodinfo *mi;

				mr = class_getconstant (rt_class, i, CONSTANT_Methodref);
				mi = class_resolveclassmethod (mr->class, mr->name, mr->descriptor, m->class, true);
				if (!mi)
					panic("Exception thrown while parsing bytecode"); /* XXX should be passed on */
				/*-- RTA --*/
				invokestatic(mi);
			}
			break;

		case JAVA_INVOKESPECIAL:
			i = rt_code_get_u2(p + 1);
			{
			constant_FMIref *mr;
			methodinfo *mi;
				
			mr = class_getconstant (rt_class, i, CONSTANT_Methodref);
			mi = class_resolveclassmethod (mr->class, mr->name, mr->descriptor, m->class, true);
			if (!mi)
				panic("Exception thrown while parsing bytecode"); /* XXX should be passed on */
						RTAPRINT06invoke_spec_virt1
			/*--- PRIVATE Method -----------------------------------------------------*/ 
			if (mi->name        != INIT) {     /* if method called is PRIVATE */ 
							RTAPRINT07invoke_spec_virt2
							RTAPRINT04invokestatic1
				invokestatic(mi);
				}

			else 	{
				/*--- Test for super <init> which is: <init> calling its super class <init> -*/
				initMethods(mi);
				}
			}	                                     	 
			break;


		case JAVA_INVOKEVIRTUAL:
			i = rt_code_get_u2(p + 1);
			{
			constant_FMIref *mr;
			methodinfo *mi;
				
			mr = class_getconstant (rt_class, i, CONSTANT_Methodref);
			mi = class_resolveclassmethod (mr->class, mr->name, mr->descriptor, m->class, true);
			if (!mi)
				panic("Exception thrown while parsing bytecode"); /* XXX should be passed on */

			invokevirtual(mi);
			}
			break;

		case JAVA_INVOKEINTERFACE:
			i = rt_code_get_u2(p + 1);
			{
			constant_FMIref *mr;
			methodinfo *mi;

			mr = class_getconstant (rt_class, i, CONSTANT_InterfaceMethodref);
			mi = class_resolveinterfacemethod (mr->class, mr->name, mr->descriptor, m->class, true);
			if (!mi)
				panic("Exception thrown while parsing bytecode"); /* XXX should be passed on */
			invokeinterface(mi);
			}
			break;

			/* miscellaneous object operations *******/

		case JAVA_NEW:
			i = rt_code_get_u2 (p+1);
			{
				classinfo *ci;

				ci = class_getconstant (rt_class, i, CONSTANT_Class); 
				newClasses(ci);
			}
			break;

		default:
			break;

		} /* end switch */


	} /* end for */

	if (p != rt_jcodelength)
		panic("Command-sequence crosses code-boundary");

	if ((XTAOPTbypass) || (opt_xta))
		xtaMethodCalls_and_sendReturnType();


}

/*-------------------------------------------------------------------------------*/
/* RTA add Native Methods/ Class functions  */
/*-------------------------------------------------------------------------------*/
void   findMarkNativeUsedMeth (utf * c1, utf* m1, utf* d1) {

	classinfo  *class;
	methodinfo *meth;

	class = class_get(c1);
	if (class == NULL)  {
		utf_display(c1);
		printf("  WARNING: CLASS used by NATIVE method is NULL so loaded\n");
/*  		loader_load_sysclass(NULL, c1);	 */
/*  		class = class_get(c1); */
		class = class_new(c1);
		if (class == NULL)  {
			panic("CLASS used by NATIVE method is NULL and loading didn't help\n");
			return;    /*Note: Since NativeCalls is for mult programs some may not be loaded - that's ok */
			}
	}

	if (class->classUsed == NOTUSED) {
printf("NATIVE_MARKED CLASS USED "); utf_display(class->name); printf("\n");
		class->classUsed = USED; /* MARK CLASS USED */
		/* add marked methods to callgraph */ 
		rtaAddMarkedMethods(class);
	}

	meth = class_findmethod_w (class, m1, d1,"findMarkNativeUsedMeth");
	if (meth == NULL) {
		utf_display(class->name);printf(".");utf_display(m1);printf(" ");utf_display(d1);
		panic("WARNING from parseRT:  Method given is used by Native method call, but NOT FOUND\n");
	}
	else
		rtaMarkSubs(class,meth);
}

/*-------------------------------------------------------------------------------*/

void   findMarkNativeUsedClass (utf * c) {
	classinfo  *class;

	class = class_get(c);
	if (class == NULL)  panic("parseRT: Class used by Native method called not loaded!!!");
	class->classUsed = USED;

	/* add marked methods to callgraph */
	rtaAddMarkedMethods(class);
}


/*-------------------------------------------------------------------------------*/

void markNativeMethodsRT(utf *rt_class, utf* rt_method, utf* rt_descriptor) {
	int i,j,k;
	bool found = false;

	nativecallcompdone = natcall2utf(nativecallcompdone); 

	for (i=0; i<NATIVECALLSSIZE; i++) {
		if (rt_class  == nativeCompCalls[i].classname) {
	
			/* find native class.method invoked */
			for (j=0; (!(found) && (j<nativeCompCalls[i].methCnt)); j++) {

				if ( (rt_method     == nativeCompCalls[i].methods[j].methodname)
					 && (rt_descriptor == nativeCompCalls[i].methods[j].descriptor)) {

					found=true;
printf("#%i#\n",nativeCompCalls[i].callCnt[j]);
					/* mark methods and classes used by this native class.method */
					for (k=0; k < nativeCompCalls[i].callCnt[j]; k++) {
						if (nativeCompCalls[i].methods[j].methodCalls[k].methodname != NULL) {
							/* mark method used */
							findMarkNativeUsedMeth(
												   nativeCompCalls[i].methods[j].methodCalls[k].classname,
												   nativeCompCalls[i].methods[j].methodCalls[k].methodname,
												   nativeCompCalls[i].methods[j].methodCalls[k].descriptor); 

							/*RTprint 
							  printf("\nmark method used: "); fflush(stdout);
							  utf_display(nativeCompCalls[i].methods[j].methodCalls[k].classname); printf(".");fflush(stdout);
							  utf_display(nativeCompCalls[i].methods[j].methodCalls[k].methodname); printf("=="); fflush(stdout);
							  utf_display(nativeCompCalls[i].methods[j].methodCalls[k].descriptor); printf("==\n"); fflush(stdout);
							*/
						}
						else {
							/* mark class used */
							findMarkNativeUsedClass( nativeCompCalls[i].methods[j].methodCalls[k].classname);
						} /* if-else k  */ 

					}  /* for k */ 

				}  /* if j */
			}  /* for j */

		}  /* if i */  
	}  /* for i */

}



/*-------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------*/
/* still need to look at field sets in 2nd pass and clinit .....  */
void XTA_jit_parse2(methodinfo *m)
{
int methRT; /* local */
	if (XTAdebug >= 1) 
		printf("\n\nStarting Round 2 XTA !!!!!!!!!!!!!!\n");

	/* for each method in XTA worklist = callgraph (use RTA for now) */
	methRT=0;
	//while (methRT <= methRTlast) {
	while (methRT <= methXTAlast) {
		rt_method      = XTAcallgraph[methRT];
		rt_class       = rt_method->class;
		rt_descriptor  = rt_method->descriptor;
		rt_jcodelength = rt_method->jcodelength;
		rt_jcode       = rt_method->jcode;

		if (! (  (rt_method->flags & ACC_NATIVE  )
				 ||   (rt_method->flags & ACC_ABSTRACT) ) ) {
			if (XTAdebug >= 1) {
				printf("\n!!!!! XTA Round 2 Parse of #%i:",methRT);fflush(stdout);
				utf_display(rt_class->name); printf("."); fflush(stdout);
				method_display(rt_method);
			}
			/*   if XTA type set changed since last parse */
			if (rt_method->xta->chgdSinceLastParse) {

				/*     get types from methods it is calledBy */
				xtaPassAllCalledByParams ();

				/* Pass parameter types to methods it calls and  send the return type those called by  */
				xtaMethodCalls_and_sendReturnType();
			}
		}
		methRT++;
	}
	if (XTAdebug >= 1) {

		printf("\n\nEND_OF Round 2 XTA !!!!!!!!!!!!!!\n");
		printXTACallgraph ();
	}
	
	RTAPRINT14CallgraphLast  /*was >=2 */
		RTAPRINT15HeirarchyiLast /*was >= 2 */
		}


/*-------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------*/
void mainRTAparseInit (methodinfo *m )
{
	/*printf("MAIN_NOT_STARTED \n");*/ 
	if (class_java_lang_Object->sub != NULL) { 
		RTAPRINT16stats1;
	}

	if (firstCall) {
		firstCall=false;

		/* Frequently used utf strings */
		utf_OBJECT = utf_new_char("java/lang/Object");
		utf_MAIN  = utf_new_char("main");
		INIT      = utf_new_char("<init>");
		CLINIT    = utf_new_char("<clinit>");
		FINALIZE  = utf_new_char("finalize");
		EMPTY_DESC= utf_new_char("()V");

		/* open file for list of methods parsed before main method or missed and parsed after main */
		if ( (rtMissed = fopen("rtMissed", "w")) == NULL) {
    			printf("CACAO - rtMissed file: cant open file to write\n");
		}
		else {
			fprintf(rtMissed,"To Help User create a dymLoad file \n");
			fprintf(rtMissed,
		"Not parsed in the static analysis parse of Main: #rt parse / #missed class.method (descriptor) \n");
			fprintf(rtMissed,"\n\tBEFORE MAIN RT PARSE\n");
			fflush(rtMissed);
			fclose(rtMissed);
		}
		/* Allocate callgraph */
		if (opt_rt) {
			callgraph = MNEW (methodinfo*, MAXCALLGRAPH);   /****/
			}
		if ((XTAOPTbypass) || (opt_xta)) {
			printf("XTAXTA  CALLGRAPHS allocated\n");
			XTAcallgraph = MNEW (methodinfo*, MAXCALLGRAPH);
			}
		}

	if (m->name == utf_MAIN) {
		rtMissed = fopen("rtMissed","a");
		fprintf(rtMissed,"\n\n\tAFTER MAIN RT PARSE\n");
		fclose(rtMissed);
		AfterMain = true;
	}
	else {  
		if ( (rtMissed = fopen("rtMissed", "a")) == NULL) {
			printf("CACAO - rtMissed file: cant open file to write\n");
		}
		else {
			fprintf(rtMissed,"#%i/#%i ",methRTlast+1,missedCnt++ );
			utf_fprint(rtMissed,m->class->name);
			fprintf(rtMissed," ");
			fprintflags(rtMissed,m->flags);
			fprintf(rtMissed," ");
			utf_fprint(rtMissed,m->name);
			fprintf(rtMissed," ");
			utf_fprint(rtMissed,m->descriptor);
			fprintf(rtMissed,"\n");
			fflush(rtMissed);
			fclose(rtMissed);
		}
		if (AfterMain) {
			printf("#%i : ",methRT);
			printf("Method missed by static analysis Main parse. See ./rtMissed file\n");
			/***	panic ("Method missed by static analysis Main parse. See rtMissed file");**/
		}
	}

	/* At moment start RTA before main when parsed                      */
	/* Will definitely use flag with to know if ok to apply in-lining.  */
}


/*-------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------*/

void RTparseCGWorklist (methodinfo *m) {  
printf("IIIIIIIIIIIIIIIIIIIIn RTparseCGWorklist\n"); fflush(stdout);
	if (m->methodUsed == USED) return;
	if ((firstCall) || (parse1) || (m->name == utf_MAIN))
		mainRTAparseInit (m);
	parse1= false;
	m->methodUsed = USED;

	/* initialise parameter type descriptor */
	callgraph[++methRTlast] = m;          /*-- RTA --*/
		RTAPRINT11addedtoCallgraph 
	/* <init> then like a new class so add marked methods to callgraph */
	if (m->name == INIT)  {  /* need for <init>s parsed efore Main */
		classinfo *ci;
		ci = m->class;
		ci->classUsed = USED;
			if (pWhenMarked >= 1) {
				printf("Class=");utf_display(ci->name);
			}
			/* add marked methods to callgraph */
			RTAPRINT11addedtoCallgraph2
		rtaAddMarkedMethods(ci);
		} /* if */

	/*---- RTA call graph worklist -----***/
	while (methRT <= methRTlast) {
		rt_method      = callgraph[methRT];
	    	rt_class       = rt_method->class;
	    	rt_descriptor  = rt_method->descriptor;
	    	rt_jcodelength = rt_method->jcodelength;
	    	rt_jcode       = rt_method->jcode;

		if (! (  (rt_method->flags & ACC_NATIVE  )
				 ||   (rt_method->flags & ACC_ABSTRACT) ) ) {
			parseRT(m);
			}
	    	//if (true == false) {   // At moment nativecalls.h is not current and neither helps nor hinders
	    	else 	{
				RTAPRINT12bAbstractNative
            		if (rt_method->flags & ACC_NATIVE ) {
					RTAPRINT12aNative
						/* mark used and add to callgraph methods and classes used by NATIVE method */
						markNativeMethodsRT(rt_class->name,rt_method->name,rt_descriptor);		  
				}
			if (rt_method->flags & ACC_ABSTRACT) {
				panic("ABSTRACT_SHOULD not ever get into the callgraph!!!!!****!!!****!!!!****!!!!\n"); 
				}
			}
		methRT++;
				RTAPRINT12Callgraph 
				RTAPRINT13Heirarchy 
		} /* while */

	if (m->class->classUsed == NOTUSED)
		m->class->classUsed = USED; /* say Main's class has a method used ??*/ 
}


/*-------------------------------------------------------------------------------*/
/*--------------------------------------------------------*/
int getdymline(char *line, int max, FILE *inFP) {
if (fgets(line, max, inFP) == NULL)
  return 0;
else {
  return strlen((const char *)line);
  }
}

/*-------------------------------------------------------------------------------*/

methodinfo *getApplicationDynamics ( ) {
char line[512]="";
char *classname;
char *methname ;
char *desc;
int rc;

classinfo  *class;
methodinfo *mi;

if ( (appldynm = fopen("appldynm", "r")) == NULL) {
	printf("parseRT - appldynm file: no appldynm file - ok if nothing in rtMissed after Main\n");
	return NULL;
}

if ( getdymline(line,512,appldynm) != 0)
	{
	printf("line=%s\n",line); fflush(stdout);

	classname = strtok(line," \n");
	methname  = strtok(NULL," \n");
	if (methname == NULL) {  /* if no meth */
		panic("parseRT - dynamic loaded class/method/desc in appldynm has no method");
		}
	desc = strtok(NULL," \n");
	if (desc == NULL) {  /* if no desc */
		panic("parseRT - dynamic loaded class/method/desc in appldynm has no descriptor");
		}
	printf("appldynm class=%s meth=%s desc=%s\n",classname, methname,desc); fflush(stdout);

        class = class_get(utf_new_char(classname));
	if (class == NULL) {
/*  		class = loader_load_sysclass(NULL,  utf_new_char(classname));	 */
		class = class_new(utf_new_char(classname));
		}
	mi = class_fetchmethod(class,utf_new_char(methname), utf_new_char(desc)) ;
	printf("+++++--3--+++++\t");
	RTparseCGWorklist (mi) ;  

	}

fclose(appldynm);
return NULL;
}

/*-------------------------------------------------------------------------------*/

void RT_jit_parse(methodinfo *m)
{
classinfo  *class;
methodinfo * mi;
classinfo *topclass;

/*-- RTA -- *******************************************************/
    if (opt_rt) 
       {
	/*---- java/lang/Object.<clinit> ----*/
 if ((firstCall == false) || (true == false))  
	{
	parse1 = true;
	if (m->methodUsed == USED) return;
	printf("MMMMMMMMMMMMMMMMMMMMMMMMMMMMissed\n"); fflush(stdout);
	RTparseCGWorklist (m) ;  
                                        /*RTAprint*/ if (pCallgraph >= 1) {
                                        /*RTAprint*/    printCallgraph ();}
                                        /*RTprint*/ //if (pClassHeir >= 1) {
                                        /*RTprint*/ //    printRThierarchyInfo(m);
                                        /*RTprint*/ //    }
                                        /*RTprint*/     /**printObjectClassHeirarchyAll( );**/
						fflush(stdout);
	printf("END MMMMMMMMMMMMMMMMMMMMMMMMMMMMissed\n"); fflush(stdout);
	return;
	}
 else
	{
	parse1 = true;
	printf("SSSSSSSSSSSSSSSSSSSSSStatic Analysis \n"); fflush(stdout);
	printf("+++++--1--+++++\t");
	RTparseCGWorklist (m) ;  

        class = class_get(utf_new_char("java/lang/Thread"));
	mi = class_fetchmethod(class,clinit_name(),clinit_desc()) ;
	printf("+++++--2--+++++\t");
	RTparseCGWorklist (mi) ;  

        class = class_get(utf_new_char("java/lang/ThreadGroup"));
	mi = class_fetchmethod(class,clinit_name(),clinit_desc()) ;
	printf("+++++--3--+++++\t");
	RTparseCGWorklist (mi) ;  


        class = class_get(utf_new_char("java/lang/Throwable"));
	mi = class_fetchmethod(class,clinit_name(),clinit_desc()) ;
	printf("+++++--4--+++++\t");
	RTparseCGWorklist (mi) ;  

	printf("mainstring=%s=\n",mainstring);fflush(stdout);
/*  	class = loader_load_sysclass(NULL,  utf_new_char(mainString));	 */
	class = class_new(utf_new_char(mainstring));
	mi = class_fetchmethod(	class,
				utf_new_char("main"),
				utf_new_char("([Ljava/lang/String;)V")
                               );
	printf("+++++--5--+++++\t");
	RTparseCGWorklist (mi) ;  

	printf("+++++--6--+++++\t");
	getApplicationDynamics();

printf("-+-+-+-+-+-+-+-+-+-+-33"); fflush(stdout);

                                        /*RTAprint*/ if (pCallgraph >= 1) {
                                        /*RTAprint*/    printCallgraph ();}
                                        /*RTprint*/ //if (pClassHeir >= 1) {
                                        /*RTprint*/ //    printRThierarchyInfo(m);
                                        /*RTprint*/ //    }
                                        /*RTprint*/     /**printObjectClassHeirarchyAll( );**/
						fflush(stdout);
printf("-+-+-+-+-+-+-+-+-+-+-44"); fflush(stdout);
	/** MFREE(callgraph,methodinfo*,MAXCALLGRAPH);  causes stack overflow **/
	}
     } /*  end opt_rt */

/*-- XTA -- *******************************************************/
   	if ((XTAOPTbypass) || (opt_xta)) {
	  	if (m->xta != NULL) {
			if (m->xta->XTAmethodUsed == USED) return;
			}
	  	mainRTAparseInit (m);

		XTAcallgraph[++methXTAlast] = m;
		if (m->xta == NULL) {
			m->xta = xtainfoInit(m);
			}
		m->xta->XTAmethodUsed = USED;
			{methodinfo *mi = m;
			printf("<");fflush(stdout);
			XTAPRINTcallgraph2
			}
		}

	/*-- Call graph work list loop -----------------*/
	/*---- XTA call graph worklist -----***/
							useXTAcallgraph = true;
   	if ((useXTAcallgraph) && (opt_xta)) {
							printf("USING XTA call graph>>>>>>>>>><<\n");

          while (methXTA <= methXTAlast) {
                rt_method      = XTAcallgraph[methXTA];
							printf("xTA CALLGRAPH #%i:",methXTA); fflush(stdout);
							method_display(rt_method);
                rt_class       = rt_method->class;
                rt_descriptor  = rt_method->descriptor;
                rt_jcodelength = rt_method->jcodelength;
                rt_jcode       = rt_method->jcode;

                if (! (  (rt_method->flags & ACC_NATIVE  )
                                 ||   (rt_method->flags & ACC_ABSTRACT) ) ) {
                        parseRT(m);
                        }
                else    {
                                RTAPRINT12bAbstractNative
                        if (rt_method->flags & ACC_NATIVE ) {
                                        RTAPRINT12aNative
                                /* mark used and add to callgraph methods and classes used by NATIVE method */
                                markNativeMethodsRT(rt_class->name,rt_method->name,rt_descriptor);
                                }
                        if (rt_method->flags & ACC_ABSTRACT) {
                                panic("ABSTRACT_SHOULD not ever get into the callgraph!!!!!****!!!****!!!!****!!!!\n");
                                }
                        }
                methXTA++;
                                RTAPRINT12Callgraph
                                RTAPRINT13Heirarchy
                } /* while */
	if (m->class->classUsed == NOTUSED)
		m->class->classUsed = USED; /* say Main's class has a method used ??*/ 
	printXTACallgraph ();

	if (m->name == utf_MAIN) { /*-- MAIN specific -- */
                                        /*RTAprint*/ if (pCallgraph >= 1) {
                                        /*RTAprint*/    printXTACallgraph (); }
                                        /*RTprint*/ if (pClassHeir >= 1) {
                                        /*RTprint*/     printf("Last RTA Info -+-+-");
                                        /*RTprint*/     printRThierarchyInfo(m);
                                        /*RTprint*/     }
                                        /*RTprint*/     /**printObjectClassHeirarchyAll( );**/
						fflush(stdout);
		/*--- XTA round 2+ "parse" - use info structures only so not a real parse */
		XTA_jit_parse2(m);
		printf("XTAXTA  CALLGRAPHS -SHOULD BE BUT ISNT- returned \n");
		//MFREE(XTAcallgraph,methodinfo*,MAXCALLGRAPH);
		}
        } /*  end opt_xta */

	RTAPRINT14CallgraphLast  /*  was >=2*/
		/***RTAPRINT15HeirarchyiLast **/ /*was >= 2 */

	return;
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

