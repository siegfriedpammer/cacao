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

   $Id: parseRT.c 1508 2004-11-15 08:34:10Z carolyn $

Changes:
opcode put into functions
changed class_findmethod class_fetchmethod

*/

/***************

 USAGE:
 Methods called by NATIVE methods and classes loaded dynamically
 cannot be found by parsing. The following files supply missing methods:

 rtMissedIn0 - (provided) has the methods missed by every java program
 rtMissed||mainClassName - is program specific.

 A file rtMissed will be written by RT analysis of any methods missed.

 This file can be renamed to rtMissed concatenated with the main class name.

 Example:
 ./cacao -rt hello

 inlining with virtuals should fail if the returned rtMissed is not empty.
 so...
 mv rtMissed rtMissedhello
 ./cacao hello

Results: (currently) with -stat see # methods marked used
 
TODO: end analysis if mono- or polymorphic call (in parseRTstats)
****************/

#include <stdio.h>
#include <string.h>
#include "tables.h"

#include "statistics.h"
#include "loader.h"
#include "main.h"
#include "options.h"
#include "jit/jit.h"
#include "jit/parse.h"
#include "toolbox/list.h"
#include "toolbox/memory.h"   
#include "parseRT.h"
#include "parseRTstats.h"

static bool firstCall= true;
static list *rtaWorkList;
FILE *rtMissed;   /* Methods missed during RTA parse of Main  */
 
#define LAZYLOADING(class) { \
	if (!class->loaded) \
            if (!class_load(class)) \
                return 0; \
	if (!class->linked) \
            if (!class_link(class)) \
                return 0; }

bool DEBUGinf = false;
bool DEBUGr = false;
bool DEBUGopcodes = false;

#define METHINFOx(mm) \
    { \
	printf("<c%i/m%i/p%i>\t", \
		mm->class->classUsed,mm->methodUsed, mm->monoPoly); \
	utf_display(mm->class->name); printf("."); fflush(stdout); \
  	method_display(mm); fflush(stdout); }

#define METHINFO(mm) \
if (DEBUGr == true) { \
	printf("<c%i/m%i/p%i>\t", \
		mm->class->classUsed,mm->methodUsed, mm->monoPoly); \
	utf_display(mm->class->name); printf("."); fflush(stdout); \
  	method_display(mm); fflush(stdout); }

#define METHINFOxt(mm,TXT) \
		 { \
                printf(TXT); \
		METHINFOx(mm) \
		}

#define METHINFOt(mm,TXT) \
if (DEBUGr == true) { \
                printf(TXT); \
		METHINFO(mm) \
		}

#define CLASSNAME1(cls,TXT) \
if (DEBUGr == true) {printf(TXT); \
	printf("<c%i>\t",cls->classUsed); \
	utf_display(cls->name); fflush(stdout);}

#define CLASSNAMEop(cls) \
if (DEBUGr == true) {printf("\t%s: ",opcode_names[opcode]);\
	printf("<c%i>\t",cls->classUsed); \
  	utf_display(cls->name); printf("\n");fflush(stdout);}

#define CLASSNAME(cls,TXT) \
if (DEBUGr == true) { printf(TXT); \
		printf("<c%i>\t",cls->classUsed); \
  		utf_display(cls->name); printf("\n");fflush(stdout);} 

#define SHOWOPCODE \
if (DEBUGopcodes == true) {printf("Parse p=%i<%i<   opcode=<%i> %s\n", \
	                   p, m->jcodelength,opcode,opcode_names[opcode]);}

/*********************************************************************/

void addToRtaWorkList(methodinfo *meth, char *info) {
    rtaNode    *rta;

if (meth->methodUsed == USED) return;

if (!(meth->flags & ACC_ABSTRACT))  {
    count_methods_marked_used++;
    METHINFOt(meth,info)
	if (meth->class->super != NULL) {
		CLASSNAME(meth->class->super,"\tsuper=")
		}
	else {
		if (DEBUGr) printf("\tsuper=NULL\n");}
	fflush(stdout);
    meth ->methodUsed = USED;
    rta = NEW(rtaNode);
    rta->method = meth ;
    list_addlast(rtaWorkList,rta);
if (meth->class->classUsed == NOTUSED) {
	printf("\nADDED method in class not used at all!\n");
	fflush(stdout);
	}
    }
/***
else {
     printf("Method not added to work list!!!<%i> : ",
     meth->methodUsed); fflush(stdout);
     METHINFO(meth)
     }
***/
}

/**************************************************************************/
void rtaMarkSubs(classinfo *class, methodinfo *topmethod); 

/*------------------------------------------------------------------------*/
void rtaAddUsedInterfaceMethods(classinfo *ci) {
	int jj,mm;

	/* add used interfaces methods to callgraph */
	for (jj=0; jj < ci -> interfacescount; jj++) {
		classinfo *ici = ci -> interfaces [jj];
	
		if (DEBUGinf) { 
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
				if (DEBUGinf) { 
					if  (imi->methodUsed != USED) {
						if (imi->methodUsed == NOTUSED) printf("Interface Method notused: "); 
						if (imi->methodUsed == MARKED) printf("Interface Method marked: "); 
						utf_display(ici->name);printf(".");method_display(imi);fflush(stdout);
					}
				} 
				if  (imi->methodUsed == USED) {
					if (DEBUGinf) { 
						printf("Interface Method used: "); utf_display(ici->name);printf(".");method_display(imi);fflush(stdout);

						/* Mark this method used in the (used) implementing class and its subclasses */
						printf("rMAY ADD methods that was used by an interface\n");
					}
					if ((utf_new_char("<clinit>") != imi->name) &&
					    (utf_new_char("<init>") != imi->name))
					    rtaMarkSubs(ci,imi);
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
void rtaAddMarkedMethods(classinfo *ci) {
int ii,jj,mm;

/* add marked methods to callgraph */ 
for (ii=0; ii<ci->methodscount; ii++) { 
	methodinfo *mi = &(ci->methods[ii]);

	if (mi->methodUsed == MARKED) { 
       		addToRtaWorkList(mi,
				"addTo was MARKED:");
		}
	else	{
		for (jj=0; jj < ci -> interfacescount; jj++) {
			classinfo *ici = ci -> interfaces [jj];
			/*  use resolve method....!!!! */
			if (ici -> classUsed != NOTUSED) {
				for (mm=0; mm< ici->methodscount; mm++) {
					methodinfo *imi = &(ici->methods[mm]);
					METHINFOt(imi,"NEW IMPD INTERFACE:")
				      /*if interface method=method is used*/
					if  (  	   (imi->methodUsed == USED)
			   &&	 ( (imi->name == mi->name) 
			   && 	   (imi->descriptor == mi->descriptor))) {
       					  addToRtaWorkList(mi,
				     "addTo was interfaced used/MARKED:");
					  }
					} /*end for */	
				}
			}
		}
	}
}    


/*********************************************************************/
void addClassInit(classinfo *ci, bool clinits, bool finalizes, bool addmark)
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
        addToRtaWorkList(mi,"addTo CLINIT added:");
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
        addToRtaWorkList(mi,"addTo FINALIZE added:");
      }     
    }        

  if (addmark) {
    rtaAddMarkedMethods(ci);
    rtaAddUsedInterfaceMethods(ci);
    }

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

void rtaMarkMethod(classinfo *class, methodinfo *topmethod) {

utf *name 	= topmethod->name; 
utf *descriptor = topmethod->descriptor;
methodinfo *submeth;

/* See if method defined in class heirarchy */
submeth = class_resolvemethod(class, name, descriptor); 
METHINFOt(submeth,"rtaMarkMethod submeth:");
if (submeth == NULL) {
	utf_display(class->name); printf(".");
	METHINFOx(topmethod);
	printf("parse RT: Method not found in class hierarchy");fflush(stdout);
	panic("parse RT: Method not found in class hierarchy");
	}
if (submeth->methodUsed == USED) return;
  
#undef CTA 
#ifdef CTA
  /* Class Type Analysis if class.method in virt cone marks it used */
  /*   very inexact, too many extra methods */
  addClassInit(	submeth->class,
		true,true,true);
  submeth->monoPoly = POLY;
  addToRtaWorkList(submeth,
		   "addTo RTA VIRT CONE:");
  return;
#endif

  if (submeth->class == class) { 

	/*--- Method defined in class -----------------------------*/
    	if (  submeth->class->classUsed == USED) { 
		/* method defined in this class -> */
		/* Class IS  marked USED           */ 
		/*    -> mark method as USED       */
  		submeth->monoPoly = POLY;
  		addToRtaWorkList(submeth,
	   		"addTo VIRT CONE 1:");
		}
	else 	{
		/* method defined in this class -> */
		/* Class IS NOT  marked USED (PART or NOTUSED) */ 
		/* -> if Method NOTUSED mark method as  MARKED */
		METHINFOt(submeth,
	 		"\tmarked VIRT CONE 2:");
		submeth->monoPoly = POLY;
		submeth->methodUsed = MARKED;
		/* Note: if class NOTUSED and subclass is used handled  */
		/*       by subsequent calls to rtaMarkMethods for cone */
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
			METHINFOt(submeth,"JUST MARKED :");
			}
		}
        /* add method to rta work list if conditions met */
	/* if ( (submeth->class->classUsed == USED) || */
	if (class->classUsed == USED) {
  		submeth->monoPoly = POLY;
  		addToRtaWorkList(submeth,
	  			"addTo VIRT CONE 3:");
		}
  	} /* end NOT defined in class */

} 

/*----------------------------------------------------------------------*/
/* Mark the method with the same name and descriptor as topmethod       */
/*   and any subclass where the method is defined and/or class is used  */
/*                                                                      */
/*----------------------------------------------------------------------*/
void rtaMarkSubs(classinfo *class, methodinfo *topmethod) {

  /* Mark method in class  */
  CLASSNAME1(class," MARKSUBS ");
  METHINFOt(topmethod," TOP ");
  rtaMarkMethod(class, topmethod);  

  /* Mark method in subclasses */
  if (class->sub != NULL) {
     classinfo *subs;
	
     if (!(topmethod->flags & ACC_FINAL )) {
	for (subs = class->sub;subs != NULL;subs = subs->nextsub) {
	  CLASSNAME1(subs," SUBS ");
  	  rtaMarkSubs(subs, topmethod); 
	  }
	}
     }
  return;
}


/*********************************************************************/

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
            /*RTAPRINT08invokeInterface1*/
		   if (DEBUGinf) {
			METHINFO(mi)
                        printf("Implemented By classes :\n");fflush(stdout);
                        if (subs == NULL) printf("\tNOT IMPLEMENTED !!!\n");
                        fflush(stdout);
			}
	while (subs != NULL) { 			
		classinfo * isubs = subs->classType;
	   /*RTAPRINT09invokeInterface2*/
		   if (DEBUGinf) {
		       printf("\t");utf_display(isubs->name);fflush(stdout);
			printf(" <%i>\n",isubs->classUsed);fflush(stdout);
			}
		/* Mark method (mark/used) in classes that implement the method */
		/*if (isubs->classUsed != NOTUSED) */ 
			{
			methodinfo *submeth;
						
			submeth = class_findmethod(isubs,mi->name, mi->descriptor); 
			if (submeth != NULL)
				submeth->monoPoly = POLY; /*  poly even if nosubs */
			rtaMarkSubs(isubs, mi);  
			}
				
		subs = subs->nextClass;
		} /* end while */
} 


/*********************************************************************/

int parseRT(methodinfo *m)
{
        int  p;                     /* java instruction counter */ 
        int  nextp;                 /* start of next java instruction */
        int  opcode;                /* java opcode */
        int  i;                     /* temp for different uses (counters)*/
        bool iswide = false;        /* true if last instruction was a wide*/
        int rc = 1;

METHINFOt(m,"\n----RT PARSING:"); 
if (DEBUGr) printf("\n");

/* scan all java instructions */
	for (p = 0; p < m->jcodelength; p = nextp) {

		opcode = code_get_u1(p,m);            /* fetch op code  */
		SHOWOPCODE

		nextp = p + jcommandsize[opcode];   /* compute next instrtart */
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

				CLASSNAME(fi->class,"\tPUT/GETSTATIC: ");
				if (!fi->class->initialized) {
					m->isleafmethod = false;
					}	
				addClassInit(	fi->class,
						true,true,false);
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
				mi = class_resolveclassmethod(mr->class,
                                                mr->name,
                                                mr->descriptor,
              					m->class,
              					false);


				if (mi) 
				   {
				   METHINFOt(mi,"INVOKESTAT/SPEC:: ")
  				   mi->monoPoly = MONO;
			   	   if ((opcode == JAVA_INVOKESTATIC)	   
				     || (mi->flags & ACC_STATIC)  
				     || (mi->flags & ACC_PRIVATE)  
				     || (mi->flags & ACC_FINAL) )  
			             {
				     if (mi->class->classUsed == PARTUSED){
					   addClassInit(mi->class,
					  		true,true,true);
					}
				     else {
				        if (mi->class->classUsed == NOTUSED){
					   addClassInit(mi->class,
					  		true,true,false);
				     	   if (mi->class->classUsed == NOTUSED){
				     	       mi->class->classUsed = PARTUSED;						   } 
					   }
					}
				     if (mi->class->classUsed == NOTUSED) {
    					METHINFOt(mi,"WAS / WARUM 1")
					panic("WAS /WARUM 1 ????");
					}
			   	     if (opcode == JAVA_INVOKESTATIC)	   
	    		               addToRtaWorkList(mi,
				    	 	     "addTo INVOKESTATIC ");
				     else
	    		               addToRtaWorkList(mi,
				    	 	    "addTo INVOKESPECIAL ");
				     } 
				   else {
				     /* Handle special <init> calls */
					
				     /* for now same as rest */
				     if (mi->class->classUsed != USED){
				       /* RTA special case:
				          call of super's <init> then
				          methods of super class not all used */
				       if (utf_new_char("<init>")==mi->name) {
					    if (m->class->super == mi->class) {
						METHINFOt(mi,"SUPER INIT:");
						/* super init */
					     	addClassInit(mi->class,
						        true,true,true);
						if (mi->class->classUsed == NOTUSED) mi->class->classUsed = PARTUSED;
						}
					    else {
						METHINFOt(mi,"NORMAL INIT:");
					        addClassInit(mi->class,
							true,true,true);
					        }
					 } 
				       if (utf_new_char("<clinit>")==mi->name)
					  addClassInit(	mi->class,
							true,true,false);

				       if (!((utf_new_char("<init>")==mi->name))
				       ||   (utf_new_char("<clinit>")==mi->name)) {
					  METHINFOt(mi,"SPECIAL not init:")
					  addClassInit(	mi->class,
							true,true,true);
					  } 
				       } 
					   
				     if (utf_new_char("<init>")==mi->name) {
				       if (mi->class->classUsed == NOTUSED) {
    					  METHINFOt(mi,"WAS / WARUM 2")
					  panic("WAS /WARUM 2 ????");
					  }
					addToRtaWorkList(mi,
                                                    "addTo INVOKESPECIAL INIT ");
				        } 
				     } 
				   } 
/***  assume if method can't be resolved won't actually be called or
      there is a real error in classpath and in normal parse an exception
      will be thrown. Following debug print can verify this
else  from if (mi) {
CLASSNAME1(mr->class,"CouldNOT Resolve method:");printf(".");fflush(stdout);
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
				   METHINFOt(mi,"INVOKEVIRTUAL ::");
			   	   if ((mi->flags & ACC_STATIC) 
				   ||  (mi->flags & ACC_PRIVATE)  
				   ||  (mi->flags & ACC_FINAL) )  
			             {
				     if (mi->class->classUsed == NOTUSED){
				       addClassInit(mi->class,
				   		    true,true,true);
				       }
  				      mi->monoPoly = MONO;
	    		              addToRtaWorkList(mi,
				  	            "addTo INVOKEVIRTUAL ");
				      } 
				   else {
				     mi->monoPoly = POLY;
				     rtaMarkSubs(mi->class,mi);
				     }
				   } 
				else {
CLASSNAME1(mr->class,"CouldNOT Resolve virt meth:");printf(".");fflush(stdout);
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
					   METHINFOt(mi,"\tINVOKEINTERFACE: ")
					   rtaMarkInterfaceSubs(mi);
					   }
				/* see INVOKESTATIC for explanation about */
			        /*   case when Interface is not resolved  */
                                /* descriptor2types(mi); ?? do need paramcnt? */
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
                        /* s_count++; look for s_counts for VTA */
						/* ci->classUsed=USED; */
			/* add marked methods */
			CLASSNAME(ci,"NEW : do nothing");
			addClassInit(ci, true, true, true);   
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
                       	CLASSNAMEop(cls);
                        if (cls->classUsed == NOTUSED){
                        	addClassInit(cls,
                                             true,true,true);
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

int getline(char *line, int max, FILE *inFP) {
if (fgets(line, max, inFP) == NULL) 
  return 0;
else
  return strlen((const char *) line);
}

/* Initialize RTA Work list ***********************************************/

/*-- Get meth ptr for class.meth desc and add to RTA worklist --*/
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
 	      addClassInit(callmeth->class, \
 			   true,true,true);\
	      } \
	callmeth->monoPoly = POLY; \
	addToRtaWorkList(callmeth,txt);


/*--  
    Initialize RTA work list with methods/classes from:  
      System calls 
	and 
      rtMissedIn list (missed becaused called from NATIVE &/or dynamic calls
--*/
methodinfo *initializeRTAworklist(methodinfo *m) {
  	classinfo  *c;
        methodinfo* callmeth;
	char systxt[]    = "System     Call :";
	char missedtxt[] = "rtMissedIn Call :";

	FILE *rtMissedIn; /* Methods missed during previous RTA parse */
	char line[256];
	char* class, *meth, *desc;
	/*char filename[256] = "rtMissed"*/
	char filenameIn[256] = "rtIn/";
	methodinfo *rm =NULL;  /* return methodinfo ptr to main method */


        /* Create RTA call work list */
	rtaWorkList = NEW(list);
        list_init(rtaWorkList, OFFSET(rtaNode,linkage) );

	/* Add first method to call list */
       	m->class->classUsed = USED; 
	addToRtaWorkList(m,systxt);

	/* Add system called methods */
 	SYSADD(mainstring, "main","([Ljava/lang/String;)V",systxt)
	rm = callmeth;  
 	SYSADD("java/lang/Runtime","getRuntime","()Ljava/lang/Runtime;",systxt)
 	SYSADD("java/lang/Runtime","exit","(I)V",systxt)

	/*----- rtMissedIn 0 */
        if ( (rtMissedIn = fopen("rtMissedIn0", "r")) == NULL) {
			/* if (verbose) */
		    {printf("No rtMissedIn0 file\n");fflush(stdout);} 
		return  rm;
		}
	while (getline(line,256,rtMissedIn)) {
	    class = strtok(line, " \n");
	    meth  = strtok(NULL, " \n");
	    desc  = strtok(NULL, " \n");
 		SYSADD(class,meth,desc,missedtxt)
		}
	fclose(rtMissedIn);

	/*----- rtMissedIn pgm specific */
        strcat(filenameIn, (const char *)mainstring);  
        if ( (rtMissedIn = fopen(filenameIn, "r")) == NULL) {
		/*if (verbose)*/ 
		    {printf("No rtMissedIn=%s file\n",filenameIn);fflush(stdout);} 
		return rm;
		}
	while (getline(line,256,rtMissedIn)) {
	    class = strtok(line, " \n");
	    meth  = strtok(NULL, " \n");
	    desc  = strtok(NULL, " \n");
	    if ((class == NULL) || (meth == NULL) || (desc == NULL)) { 
		fprintf(stderr,"Error in rtMissedIn file for: %s.%s %s\n",class, meth, desc); 
		fflush(stderr);
		panic ("Error in rtMissedIn file for: class.meth, desc\n"); 
		}
 	    SYSADD(class,meth,desc,missedtxt)
	    }
	fclose(rtMissedIn);
	return rm;
}

/*- end initializeRTAworklist-------- */



/*-- RTA -- *******************************************************/
int RT_jit_parse(methodinfo *m)
{
  methodinfo *rt_method;
  rtaNode    *rta;
  methodinfo *mainmeth;

  /* Should only be called once */
  if (firstCall) {
        firstCall = false; /* turn flag off */

        /*----- RTA initializations --------*/
  	if (verbose) 
      	    log_text("RTA static analysis started.\n");

	mainmeth = initializeRTAworklist(m);

    if ( (rtMissed = fopen("rtMissed", "w")) == NULL) {
        printf("CACAO - rtMissed file: cant open file to write\n");
        }
    /* Note: rtMissed must be renamed to rtMissedIn to be used as input */
	
    /*------ process RTA call work list --------*/
    for (rta =list_first(rtaWorkList); 
	 rta != NULL; 
	 rta =list_next(rtaWorkList,rta)) 
        { 
        rt_method = rta->method;
	if (! (  (rt_method->flags & ACC_NATIVE  )
            ||   (rt_method->flags & ACC_ABSTRACT) ) )	
	    {
	    /* RTA parse to approxmate....
		what classes/methods will really be used during execution */
	    parseRT(rt_method);  
	    }
	else {
	    if (rt_method->flags & ACC_NATIVE  )
	        {
	       METHINFOt(rt_method,"TO BE NATIVE RTA PARSED :");
		/* parseRTpseudo(rt_method); */
	        }   
	    else {
	       printf("Abstract method in RTA Work List: ");
	       METHINFO(rt_method);
	       panic("Abstract method in RTA Work List.");
               }
            }            	
    	}	
    fclose(rtMissed);
    if (verbose) {
        if (opt_stat) {
          printRThierarchyInfo(m); 
	  }
      printCallgraph(rtaWorkList); 
      }

    if (verbose) {
      log_text("RTA static analysis done.\n");
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
