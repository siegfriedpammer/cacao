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

   $Id: parseXTA.c 1881 2005-01-21 13:46:51Z carolyn $

*/

/***************
 Rapid Type Static Analysis of Java program
   used -xta option is turned on either explicitly 
 or automatically with inlining of virtuals.

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
#include "vm/tables.h"
#include "vm/statistics.h"
#include "vm/loader.h"
#include "vm/options.h"
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

void addToXtaWorkList(methodinfo *meth, char *info) {
    xtaNode    *xta;

if (meth->methodUsed == USED) return;

if (!(meth->flags & ACC_ABSTRACT))  {
    count_methods_marked_used++;
    METHINFOt(meth,info,XTA_DEBUGopcodes)
	if (meth->class->super != NULL) {
		CLASSNAME(meth->class->super,"\tsuper=",XTA_DEBUGr)
		}
	else {
		if (XTA_DEBUGr) printf("\tsuper=NULL\n");}
	fflush(stdout);
    meth ->methodUsed = USED;
    xta = NEW(xtaNode);
    xta->method = meth ;
    list_addlast(xtaWorkList,xta);
if (meth->class->classUsed == NOTUSED) {
    	METHINFOx(meth)
	printf("\nADDED method in class not used at all!\n");
	fflush(stdout);
	}
    }
/***
else {
     printf("Method not added to work list!!!<%i> : ",
     meth->methodUsed); fflush(stdout);
     METHINFO(meth,true)
     }
***/
}

/**************************************************************************/
void xtaMarkSubs(classinfo *class, methodinfo *topmethod); 

/*------------------------------------------------------------------------*/
void xtaAddUsedInterfaceMethods(classinfo *ci) {
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
					if ((utf_new_char("<clinit>") != imi->name) &&
					    (utf_new_char("<init>") != imi->name))
					    xtaMarkSubs(ci,imi);
				}
			}
		}
	}

}


/**************************************************************************/
/* Add Marked methods for input class ci                                  */
/* Add methods with the same name and descriptor as implemented interfaces*/
/*   with the same method name                                            */
/*                                                                        */
/*------------------------------------------------------------------------*/
void xtaAddMarkedMethods(classinfo *ci) {
int ii,jj,mm;

/* add marked methods to callgraph */ 
for (ii=0; ii<ci->methodscount; ii++) { 
	methodinfo *mi = &(ci->methods[ii]);

	if (mi->methodUsed == MARKED) { 
       		addToXtaWorkList(mi,
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
       					  addToXtaWorkList(mi,
				     "addTo was interfaced used/MARKED:");
					  }
					} /*end for */	
				}
			}
		}
	}
}    

#define CLINITS_T   true
#define FINALIZE_T  true
#define ADDMARKED_T true

#define CLINITS_F   false 
#define FINALIZE_F  false 
#define ADDMARKED_F false
/*********************************************************************/
void XTAaddClassInit(classinfo *ci, bool clinits, bool finalizes, bool addmark)
{
  methodinfo *mi;

  if (addmark)
  	ci->classUsed = USED;
	
  if (clinits) { /* No <clinit>  available - ignore */
    mi = class_findmethod(ci, 
			utf_new_char("<clinit>"), 
			utf_new_char("()V"));
    if (mi) { 
	if (ci->classUsed != USED)
	    ci->classUsed = PARTUSED;
	mi->monoPoly = MONO;
        addToXtaWorkList(mi,"addTo CLINIT added:");
      }     
    }        

  /*Special Case for System class init:
    add java/lang/initializeSystemClass to callgraph */
  if (ci->name == utf_new_char("initializeSystemClass")) {
    /* ?? what is name of method ?? */ 
    } 

  if (finalizes) {
    mi = class_findmethod(ci, 
			utf_new_char("finalize"), 
			utf_new_char("()V"));
    if (mi) { 
	if (ci->classUsed != USED)
	    ci->classUsed = PARTUSED;
	mi->monoPoly = MONO;
        addToXtaWorkList(mi,"addTo FINALIZE added:");
      }     
    }        

  if (addmark) {
    xtaAddMarkedMethods(ci);
    }
    xtaAddUsedInterfaceMethods(ci);

}


/*--------------------------------------------------------------*/
/* Mark the method with same name /descriptor in topmethod      */
/* in class                                                     */
/*                                                              */
/* Class marked USED and method defined in this class ->        */
/*    -> mark method as USED                                    */
/* Class not marked USED and method defined in this class ->    */
/*    -> if Method NOTUSED mark method as MARKED                */
/*                                                              */
/* Class USED, but method not defined in this class ->          */
/* -> 1) search up the heirarchy and mark method where defined  */
/*    2) if class where method is defined is not USED ->        */
/*       -> mark class with defined method as PARTUSED          */
/*--------------------------------------------------------------*/

void xtaMarkMethod(classinfo *class, methodinfo *topmethod) {

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
if (submeth->methodUsed == USED) return;
  
#undef CTA 
#ifdef CTA
  /* Class Type Analysis if class.method in virt cone marks it used */
  /*   very inexact, too many extra methods */
  XTAaddClassInit(	submeth->class,
		CLINITS_T,FINALIZE_T,ADDMARKED_T);
  submeth->monoPoly = POLY;
  addToXtaWorkList(submeth,
		   "addTo XTA VIRT CONE:");
  return;
#endif

  if (submeth->class == class) { 

	/*--- Method defined in class -----------------------------*/
    	if (  submeth->class->classUsed == USED) { 
		/* method defined in this class -> */
		/* Class IS  marked USED           */ 
		/*    -> mark method as USED       */
  		submeth->monoPoly = POLY;
  		addToXtaWorkList(submeth,
	   		"addTo VIRT CONE 1:");
		}
	else 	{
		/* method defined in this class -> */
		/* Class IS NOT  marked USED (PART or NOTUSED) */ 
		/* -> if Method NOTUSED mark method as  MARKED */
		METHINFOt(submeth,
	 		"\tmarked VIRT CONE 2:",XTA_DEBUGr);
		submeth->monoPoly = POLY;
		submeth->methodUsed = MARKED;
		/* Note: if class NOTUSED and subclass is used handled  */
		/*       by subsequent calls to xtaMarkMethods for cone */
		}
	} /* end defined in class */

  else {
	/*--- Method NOT defined in class ---------------*/
        /* first mark classes if needed */
	if (submeth->class->classUsed == NOTUSED) {
		submeth->class->classUsed = PARTUSED;
		if (class->classUsed != USED) {
			submeth->monoPoly = POLY;
			submeth->methodUsed = MARKED;
			METHINFOt(submeth,"JUST MARKED :",XTA_DEBUGr);
			}
		}
        /* add method to xta work list if conditions met */
		/*??if ( (submeth->class->classUsed == USED) ||  */
	if (class->classUsed == USED) {
  		submeth->monoPoly = POLY;
  		addToXtaWorkList(submeth,
	  			"addTo VIRT CONE 3:");
		}
  	} /* end NOT defined in class */

} 

/*----------------------------------------------------------------------*/
/* Mark the method with the same name and descriptor as topmethod       */
/*   and any subclass where the method is defined and/or class is used  */
/*                                                                      */
/*----------------------------------------------------------------------*/
void xtaMarkSubs(classinfo *class, methodinfo *topmethod) {

  /* Mark method in class  */
  CLASSNAME1(class," MARKSUBS ",XTA_DEBUGr);
  METHINFOt(topmethod," TOP ",XTA_DEBUGr);
  xtaMarkMethod(class, topmethod);  

  /* Mark method in subclasses */
  if (class->sub != NULL) {
     classinfo *subs;
	
     if (!(topmethod->flags & ACC_FINAL )) {
	for (subs = class->sub;subs != NULL;subs = subs->nextsub) {
	  CLASSNAME1(subs," SUBS ",XTA_DEBUGr);
  	  xtaMarkSubs(subs, topmethod); 
	  }
	}
     }
  return;
}


/*********************************************************************/

void xtaMarkInterfaceSubs(methodinfo *mi) {				
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
		classinfo * isubs = subs->classType;
		methodinfo *submeth;
		   if (XTA_DEBUGinf) {
		       printf("\t");utf_display(isubs->name);fflush(stdout);
			printf(" <%i>\n",isubs->classUsed);fflush(stdout);
			}
		/*Mark method (mark/used) in classes that implement method*/
						
		submeth = class_findmethod(isubs,mi->name, mi->descriptor); 
		if (submeth != NULL)
			submeth->monoPoly = POLY; /*  poly even if nosubs */
		xtaMarkSubs(isubs, mi);  
				
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
		case JAVA_PUTSTATIC:
		case JAVA_GETSTATIC:

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

				CLASSNAME(fi->class,"\tPUT/GETSTATIC: ",XTA_DEBUGr);
				if (!fi->class->initialized) {
					m->isleafmethod = false;
					}	
				XTAaddClassInit(	fi->class,
						CLINITS_T,FINALIZE_T,ADDMARKED_F);
			}
			break;


				    	 	 		
			case JAVA_INVOKESTATIC:
			case JAVA_INVOKESPECIAL:
				i = code_get_u2(p + 1,m);
				{
				constant_FMIref *mr;
				methodinfo *mi;

				m->isleafmethod = false;
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
  				   
  				   /*---- Handle "leaf" = static, private, final calls----------------------------*/
			   	   if ((opcode == JAVA_INVOKESTATIC)	   
				     || (mi->flags & ACC_STATIC)  
				     || (mi->flags & ACC_PRIVATE)  
				     || (mi->flags & ACC_FINAL) )  
			         	{
				     	if (mi->class->classUsed == PARTUSED){ 
					   		XTAaddClassInit(mi->class, 
					  			CLINITS_T,FINALIZE_T,ADDMARKED_T); 
							} 
				     	else { 
				        	if (mi->class->classUsed == NOTUSED){ 
					   			XTAaddClassInit(mi->class, 
					  				CLINITS_T,FINALIZE_T,ADDMARKED_T); 
				     	   		if (mi->class->classUsed == NOTUSED){    
									/* Leaf methods are used whether class is or not */
									/*   so mark class as PARTlyUSED                 */
				     	       		mi->class->classUsed = PARTUSED; 						   
									}  
								} 
							} 
						/* Add to XTA working list/set of reachable methods	*/
						if (opcode == JAVA_INVOKESTATIC)  /* if stmt just for debug tracing */	   
							addToXtaWorkList(mi, 
								"addTo INVOKESTATIC "); 
						else 
							addToXtaWorkList(mi, 
								"addTo INVOKESPECIAL ");	
						} 
				     	
					else {
					/*---- Handle special <init> calls ---------------------------------------------*/
				   
						if (mi->class->classUsed != USED) {
				       		/* XTA special case:
				          		call of super's <init> then
				          		methods of super class not all used */
				          		
				          	/*--- <init>  ()V  is equivalent to "new" 
				          		indicating a class is used = instaniated ---- */	
				       		if (utf_new_char("<init>")==mi->name) {
					    		if ((m->class->super == mi->class) 
					    		&&  (m->descriptor == utf_new_char("()V")) ) 
									{
									METHINFOt(mi,"SUPER INIT:",XTA_DEBUGopcodes);
									/* super init so class may be only used because of its sub-class */
					     			XTAaddClassInit(mi->class,
						        		CLINITS_T,FINALIZE_T,ADDMARKED_F);
									if (mi->class->classUsed == NOTUSED) mi->class->classUsed = PARTUSED;
									}
					    		else {
						    		/* since <init> indicates classes is used, then add marked methods, too */
									METHINFOt(mi,"NORMAL INIT:",XTA_DEBUGopcodes);
					        		XTAaddClassInit(mi->class,
										CLINITS_T,FINALIZE_T,ADDMARKED_T);
					        		}
								addToXtaWorkList(mi,
									"addTo INIT ");
								} /* end just for <init> ()V */
					 		
							/* <clinit> for class inits do not add marked methods; class not yet instaniated */	 
							if (utf_new_char("<clinit>")==mi->name)
								XTAaddClassInit(	mi->class,
									CLINITS_T,FINALIZE_T,ADDMARKED_F);

							if (!((utf_new_char("<init>")==mi->name))
							||   (utf_new_char("<clinit>")==mi->name)) {
								METHINFOt(mi,"SPECIAL not init:",XTA_DEBUGopcodes)
								if (mi->class->classUsed !=USED)
									mi->class->classUsed = PARTUSED;
								addToXtaWorkList(mi,
									"addTo SPEC notINIT ");
								} 
					  			
							} /* end init'd class not used = class init process was needed */ 
							
						/* add method to XTA list = set of reachable methods */	
						addToXtaWorkList(mi,
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

                               	m->isleafmethod = false;
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
			             {
				     if (mi->class->classUsed == NOTUSED){
				       XTAaddClassInit(mi->class,
				   		    CLINITS_T,FINALIZE_T,ADDMARKED_T);
				       }
  				      mi->monoPoly = MONO;
	    		              addToXtaWorkList(mi,
				  	            "addTo INVOKEVIRTUAL ");
				      } 
				   else {
				     mi->monoPoly = POLY;
				     xtaMarkSubs(mi->class,mi);
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

                                m->isleafmethod = false;

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
					   xtaMarkInterfaceSubs(mi);
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
                        m->isleafmethod = false; /* why for new ? */
                        /*** s_count++; look for s_counts for VTA */
			/***ci->classUsed=USED;  */
			/* add marked methods */
			CLASSNAME(ci,"NEW : do nothing",XTA_DEBUGr);
			XTAaddClassInit(ci, CLINITS_T, FINALIZE_T,ADDMARKED_T);   
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
                        	XTAaddClassInit(cls,
                                            CLINITS_T,FINALIZE_T,ADDMARKED_T);
                                }
			}
                        break;

		default:
			break;
				
		} /* end switch */

		} /* end for */

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
#define SYSADD(cls,meth,desc,txt) \
        c = class_new(utf_new_char(cls)); \
        LAZYLOADING(c) \
        callmeth = class_resolveclassmethod(c, \
              utf_new_char(meth), \
              utf_new_char(desc), \
              c, \
              false); \
        if (callmeth->class->classUsed != USED) {  \
	      c->classUsed = PARTUSED; \
 	      XTAaddClassInit(callmeth->class, \
 			   CLINITS_T,FINALIZE_T,ADDMARKED_T);\
	      } \
	callmeth->monoPoly = POLY; \
	addToXtaWorkList(callmeth,txt);


/*--  
    Initialize XTA work list with methods/classes from:  
      System calls 
	and 
      xtaMissedIn list (missed becaused called from NATIVE &/or dynamic calls
--*/
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
	addToXtaWorkList(m,systxt);
	/* Add system called methods */
/*** 	SYSADD(mainstring, "main","([Ljava/lang/String;)V",systxt) ***/
 	SYSADD(MAINCLASS, MAINMETH, MAINDESC,systxt)
	rm = callmeth;  
/*** 	SYSADD("java/lang/System","exit","(I)V",systxt) ***/
 	SYSADD(EXITCLASS, EXITMETH, EXITDESC, systxt)
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
 		SYSADD(class,meth,desc,missedtxt)
		}
	fclose(xtaMissedIn);




	return rm;

}

/*- end initializeXTAworklist-------- */



methodinfo *missedXTAworklist()  
{
	FILE *xtaMissedIn; /* Methods missed during previous XTA parse */
	char filenameIn[256] = "xtaIn/";
	char line[256];
	char* class, *meth, *desc;
	char missedtxt[] = "xtaIn/ missed Call :";
  	classinfo  *c;
        methodinfo* callmeth;

	methodinfo *rm =NULL;  /* return methodinfo ptr to main method */


#if defined(USE_THREADS)
	SYSADD(THREADCLASS, THREADMETH, THREADDESC, "systxt2")
	SYSADD(THREADGROUPCLASS, THREADGROUPMETH, THREADGROUPDESC, "systxt2")
#endif
	/*----- xtaMissedIn pgm specific */
        strcat(filenameIn, (const char *)mainstring);  
        if ( (xtaMissedIn = fopen(filenameIn, "r")) == NULL) {
		/*if (opt_verbose)*/ 
		    {printf("No xtaIn/=%s file\n",filenameIn);fflush(stdout);} 
		return rm;
		}
	while (XTAgetline(line,256,xtaMissedIn)) {
	    class = strtok(line, " \n");
	    meth  = strtok(NULL, " \n");
	    desc  = strtok(NULL, " \n");
	    if ((class == NULL) || (meth == NULL) || (desc == NULL)) { 
		fprintf(stderr,"Error in xtaMissedIn file for: %s.%s %s\n",class, meth, desc); 
		fflush(stderr);
		panic ("Error in xtaMissedIn file for: class.meth, desc\n"); 
		}
 	    SYSADD(class,meth,desc,missedtxt)
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

