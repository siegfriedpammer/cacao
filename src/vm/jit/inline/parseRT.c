/* src/vm/jit/inline/parseRT.c - parser and print functions for Rapid Type
                                 Analyis

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

   $Id: parseRT.c 2193 2005-04-02 19:33:43Z edwin $

*/

/***************
 Rapid Type Static Analysis of Java program
   used -rt option is turned on either explicitly 
 or automatically with inlining of virtuals.

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
 
****************/

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "cacao/cacao.h"
#include "mm/memory.h"   
#include "toolbox/list.h"
#include "vm/class.h"
#include "vm/linker.h"
#include "vm/loader.h"
#include "vm/resolve.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "vm/tables.h"
#include "vm/jit/jit.h"
#include "vm/jit/parse.h"
#include "vm/jit/inline/parseRT.h"
#include "vm/jit/inline/parseRTstats.h"
#include "vm/jit/inline/parseRTprint.h"



static bool firstCall= true;
static list *rtaWorkList;
FILE *rtMissed;   /* Methods missed during RTA parse of Main  */

bool RTA_DEBUGinf = false;
bool RTA_DEBUGr = false;
bool RTA_DEBUGopcodes = false;
 


/*********************************************************************/

void addToRtaWorkList(methodinfo *meth, char *info) {
    rtaNode    *rta;

if (meth->methodUsed == USED) return;

if (!(meth->flags & ACC_ABSTRACT))  {
#if defined(STATISTICS)
    count_methods_marked_used++;
#endif
    METHINFOt(meth,info,RTA_DEBUGopcodes)
	if (meth->class->super.cls != NULL) {
		CLASSNAME(meth->class->super.cls,"\tsuper=",RTA_DEBUGr)
		}
	else {
		if (RTA_DEBUGr) printf("\tsuper=NULL\n");}
	fflush(stdout);
    meth ->methodUsed = USED;
    rta = NEW(rtaNode);
    rta->method = meth ;
    list_addlast(rtaWorkList,rta);
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
void rtaMarkSubs(classinfo *class, methodinfo *topmethod); 

/*------------------------------------------------------------------------*/
void rtaAddUsedInterfaceMethods(classinfo *ci) {
	int jj,mm;

	/* add used interfaces methods to callgraph */
	for (jj=0; jj < ci -> interfacescount; jj++) {
		classinfo *ici = ci -> interfaces [jj].cls;
	
		if (RTA_DEBUGinf) { 
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
				if (RTA_DEBUGinf) { 
					if  (imi->methodUsed != USED) {
						if (imi->methodUsed == NOTUSED) printf("Interface Method notused: "); 
						if (imi->methodUsed == MARKED) printf("Interface Method marked: "); 
						utf_display(ici->name);printf(".");method_display(imi);fflush(stdout);
					}
				} 
				if  (imi->methodUsed == USED) {
					if (RTA_DEBUGinf) { 
						printf("Interface Method used: "); utf_display(ici->name);printf(".");method_display(imi);fflush(stdout);

						/* Mark this method used in the (used) implementing class and its subclasses */
						printf("rMAY ADD methods that was used by an interface\n");
					}
					if ((utf_clinit != imi->name) &&
					    (utf_init != imi->name))
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
	else	{ /*** ??? Should this be an else or ??? */
		for (jj=0; jj < ci -> interfacescount; jj++) {
			classinfo *ici = ci -> interfaces [jj].cls;
			/*  use resolve method....!!!! */
			if (ici -> classUsed != NOTUSED) {
				for (mm=0; mm< ici->methodscount; mm++) {
					methodinfo *imi = &(ici->methods[mm]);
					METHINFOt(imi,"NEW IMPD INTERFACE:",RTA_DEBUGinf)
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

#define CLINITS_T   true
#define FINALIZE_T  true
#define ADDMARKED_T true

#define CLINITS_F   false 
#define FINALIZE_F  false 
#define ADDMARKED_F false
/*********************************************************************/
void RTAaddClassInit(classinfo *ci, bool clinits, bool finalizes, bool addmark)
{
  methodinfo *mi;

  if (addmark)
  	ci->classUsed = USED;
	
  if (clinits) { /* No <clinit>  available - ignore */
    mi = class_findmethod(ci, 
			utf_clinit, 
			utf_void__void);
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
			utf_void__void);
    if (mi) { 
	if (ci->classUsed != USED)
	    ci->classUsed = PARTUSED;
	mi->monoPoly = MONO;
        addToRtaWorkList(mi,"addTo FINALIZE added:");
      }     
    }        

  if (addmark) {
    rtaAddMarkedMethods(ci);
    }
  rtaAddUsedInterfaceMethods(ci);

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
METHINFOt(submeth,"rtaMarkMethod submeth:",RTA_DEBUGr);
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
  RTAaddClassInit(	submeth->class,
		CLINITS_T,FINALIZE_T,ADDMARKED_T);
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
	 		"\tmarked VIRT CONE 2:",RTA_DEBUGr);
		submeth->monoPoly = POLY;
		submeth->methodUsed = MARKED;
		/* Note: if class NOTUSED and subclass is used handled  */
		/*       by subsequent calls to rtaMarkMethods for cone */
		}
	} /* end defined in class */

  else {
	/*--- Method NOT defined in class ---------------*/
	/* then check class the method could be called with */

        /* first mark classes if needed */
	if (submeth->class->classUsed == NOTUSED) {
		submeth->class->classUsed = PARTUSED;
		if (class->classUsed != USED) {
			submeth->monoPoly = POLY;
			submeth->methodUsed = MARKED;
			METHINFOt(submeth,"JUST MARKED :",RTA_DEBUGr);
			}
		}
        /* add method to rta work list if conditions met */
		/*??if ( (submeth->class->classUsed == USED) ||  */
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
  CLASSNAME1(class," MARKSUBS ",RTA_DEBUGr);
  METHINFOt(topmethod," TOP ",RTA_DEBUGr);
  rtaMarkMethod(class, topmethod);  

  /* Mark method in subclasses */
  if (class->sub != NULL) {
     classinfo *subs;
	
     if (!(topmethod->flags & ACC_FINAL )) {
	for (subs = class->sub;subs != NULL;subs = subs->nextsub) {
	  CLASSNAME1(subs," SUBS ",RTA_DEBUGr);
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
		   if (RTA_DEBUGinf) {
			METHINFO(mi,RTA_DEBUGinf)
                        printf("Implemented By classes :\n");fflush(stdout);
                        if (subs == NULL) printf("\tNOT IMPLEMENTED !!!\n");
                        fflush(stdout);
			}
	while (subs != NULL) { 			
		classinfo * isubs = subs->classType;
		methodinfo *submeth;
		   if (RTA_DEBUGinf) {
		       printf("\t");utf_display(isubs->name);fflush(stdout);
			printf(" <%i>\n",isubs->classUsed);fflush(stdout);
			}
		/*Mark method (mark/used) in classes that implement method*/
						
		submeth = class_findmethod(isubs,mi->name, mi->descriptor); 
		if (submeth != NULL)
			submeth->monoPoly = POLY; /*  poly even if nosubs */
		rtaMarkSubs(isubs, mi);  
				
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

METHINFOt(m,"\n----RT PARSING:",RTA_DEBUGopcodes); 
if ((RTA_DEBUGr)||(RTA_DEBUGopcodes)) printf("\n");

/* scan all java instructions */
	for (p = 0; p < m->jcodelength; p = nextp) {

		opcode = code_get_u1(p,m);            /* fetch op code  */
		SHOWOPCODE(RTA_DEBUGopcodes)

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
				classinfo *frclass;

				fr = class_getconstant(m->class, i, CONSTANT_Fieldref);
				if (!resolve_classref(m,fr->classref,resolveEager,true,&frclass))
					panic("Could not resolve class reference");
				LAZYLOADING(frclass);

				fi = class_resolvefield(frclass,
							fr->name,
							fr->descriptor,
							m->class,
							true);

				if (!fi)
					return 0; /* was NULL */

				CLASSNAME(fi->class,"\tPUT/GETSTATIC: ",RTA_DEBUGr);
				RTAaddClassInit(	fi->class,
						CLINITS_T,FINALIZE_T,ADDMARKED_F);
			}
			break;


				    	 	 		
			case JAVA_INVOKESTATIC:
			case JAVA_INVOKESPECIAL:
				i = code_get_u2(p + 1,m);
				{
				constant_FMIref *mr;
				methodinfo *mi;
				classinfo *mrclass;

				mr = class_getconstant(m->class, i, CONSTANT_Methodref);
				if (!resolve_classref(m,mr->classref,resolveEager,true,&mrclass))
					panic("Could not resolve class reference");
				LAZYLOADING(mrclass) 
				mi = class_resolveclassmethod(	mrclass,
												mr->name,
												mr->descriptor,
												m->class,
												false);

				if (mi) 
				   {
				   METHINFOt(mi,"INVOKESTAT/SPEC:: ",RTA_DEBUGopcodes)
  				   mi->monoPoly = MONO;
  				   
  				   /*---- Handle "leaf" = static, private, final calls----------------------------*/
			   	   if ((opcode == JAVA_INVOKESTATIC)	   
				     || (mi->flags & ACC_STATIC)  
				     || (mi->flags & ACC_PRIVATE)  
				     || (mi->flags & ACC_FINAL) )  
			         	{
				        if (mi->class->classUsed != USED){ /* = NOTUSED or PARTUSED */
					   	RTAaddClassInit(mi->class, 
					  			CLINITS_T,FINALIZE_T,ADDMARKED_T); 
								/* Leaf methods are used whether class is or not */
								/*   so mark class as PARTlyUSED                 */
				     	       	mi->class->classUsed = PARTUSED; 
						} 
					/* Add to RTA working list/set of reachable methods	*/
					if (opcode == JAVA_INVOKESTATIC)  /* if stmt just for debug tracing */	   
						addToRtaWorkList(mi, 
								"addTo INVOKESTATIC "); 
					else 
						addToRtaWorkList(mi, 
								"addTo INVOKESPECIAL ");	
					} 
				     	
				   else {
					/*---- Handle special <init> calls ---------------------------------------------*/
				   
					if (mi->class->classUsed != USED) {
			       		/* RTA special case:
			          		call of super's <init> then
			          		methods of super class not all used */
				          		
			          		/*--- <init>  ()V  is equivalent to "new" 
			          		indicating a class is used = instaniated ---- */	
			       			if (utf_init==mi->name) {
				    			if ((m->class->super.cls == mi->class) 
				    			&&  (m->descriptor == utf_void__void) ) 
								{
								METHINFOt(mi,"SUPER INIT:",RTA_DEBUGopcodes);
								/* super init so class may be only used because of its sub-class */
				     				RTAaddClassInit(mi->class,
						        		CLINITS_T,FINALIZE_T,ADDMARKED_F);
								if (mi->class->classUsed == NOTUSED) mi->class->classUsed = PARTUSED;
								}
				    			else {
					    			/* since <init> indicates classes is used, then add marked methods, too */
								METHINFOt(mi,"NORMAL INIT:",RTA_DEBUGopcodes);
				        			RTAaddClassInit(mi->class,
									CLINITS_T,FINALIZE_T,ADDMARKED_T);
				        			}
							addToRtaWorkList(mi,
								"addTo INIT ");
							} /* end just for <init> ()V */
					 		
						/* <clinit> for class inits do not add marked methods; class not yet instaniated */	 
						if (utf_clinit==mi->name)
							RTAaddClassInit(	mi->class,
										CLINITS_T,FINALIZE_T,ADDMARKED_F);

						if (!((utf_init==mi->name))
						||   (utf_clinit==mi->name)) {
							METHINFOt(mi,"SPECIAL not init:",RTA_DEBUGopcodes)
							if (mi->class->classUsed !=USED)
								mi->class->classUsed = PARTUSED;
							addToRtaWorkList(mi,
								"addTo SPEC notINIT ");
							} 
					 	 			
						} /* end init'd class not used = class init process was needed */ 
							
					/* add method to RTA list = set of reachable methods */	
					addToRtaWorkList(mi,
							"addTo SPEC whymissed ");
					} /* end inits */
				   } 
/***  assume if method can't be resolved won't actually be called or
      there is a real error in classpath and in normal parse an exception
      will be thrown. Following debug print can verify this
else  from if (mi) {
CLASSNAME1(mr->class,"CouldNOT Resolve method:",,RTA_DEBUGr);printf(".");fflush(stdout);
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
								classinfo *mrclass;

			       	mr = m->class->cpinfos[i];
                                /*mr = class_getconstant(m->class, i, CONSTANT_Methodref)*/
					if (!resolve_classref(m,mr->classref,resolveEager,true,&mrclass))
						panic("Could not resolve class reference");
			       	LAZYLOADING(mrclass) 
				mi = class_resolveclassmethod(mrclass,
                                                mr->name,
                                                mr->descriptor,
              					m->class,
              					false);


				if (mi) 
				   {
				   METHINFOt(mi,"INVOKEVIRTUAL ::",RTA_DEBUGopcodes);
			   	   if ((mi->flags & ACC_STATIC) 
				   ||  (mi->flags & ACC_PRIVATE)  
				   ||  (mi->flags & ACC_FINAL) )  
			             {
				     if (mi->class->classUsed == NOTUSED){
				       RTAaddClassInit(mi->class,
				   		    CLINITS_T,FINALIZE_T,ADDMARKED_T);
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
CLASSNAME1(mrclass,"CouldNOT Resolve virt meth:",RTA_DEBUGr);printf(".");fflush(stdout);
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
								classinfo *mrclass;

                                mr = class_getconstant(m->class, i, CONSTANT_InterfaceMethodref);
								if (!resolve_classref(m,mr->classref,resolveEager,true,&mrclass))
									panic("Could not resolve class reference");
                                LAZYLOADING(mrclass)

                                mi = class_resolveinterfacemethod(mrclass,
                                                          mr->name,
                                                          mr->descriptor,
                                                          m->class,
                                                          false);
					if (mi)
                                           {
					   METHINFOt(mi,"\tINVOKEINTERFACE: ",RTA_DEBUGopcodes)
					   rtaMarkInterfaceSubs(mi);
					   }
				/* see INVOKESTATIC for explanation about */
			        /*   case when Interface is not resolved  */
                                /*method_descriptor2types(mi); 
				?? do need paramcnt? for RTA (or just XTA)*/
                        }
                        break;

                case JAVA_NEW:
                /* means class is at least passed as a parameter */
                /* class is really instantiated when class.<init> called*/
                        i = code_get_u2(p + 1,m);
			{
				constant_classref *cr;
				classinfo *ci;
                cr = (constant_classref *)class_getconstant(m->class, i, CONSTANT_Class);
				resolve_classref(NULL,cr,resolveEager,false,&ci);
                        /*** s_count++; look for s_counts for VTA */
			/* add marked methods */
			CLASSNAME(ci,"NEW : do nothing",RTA_DEBUGr);
			RTAaddClassInit(ci, CLINITS_T, FINALIZE_T,ADDMARKED_T);   
			}
                        break;

                case JAVA_CHECKCAST:
                case JAVA_INSTANCEOF:
                /* class used */
                        i = code_get_u2(p + 1,m);
                        {
							constant_classref *cr;
                        	classinfo *cls;

							cr = (constant_classref*) class_getconstant(m->class, i, CONSTANT_Class);
							resolve_classref(NULL,cr,resolveEager,false,&cls);

                        LAZYLOADING(cls)
                       	CLASSNAMEop(cls,RTA_DEBUGr);
                        if (cls->classUsed == NOTUSED){
                        	RTAaddClassInit(cls,
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

int RTAgetline(char *line, int max, FILE *inFP) {
if (fgets(line, max, inFP) == NULL) 
  return 0;
else
  return strlen((const char *) line);
}

/* Initialize RTA Work list ***********************************************/

/*-- Get meth ptr for class.meth desc and add to RTA worklist --*/
#define SYSADD(cls,meth,desc,txt) \
        load_class_bootstrap(utf_new_char(cls),&c); \
        LAZYLOADING(c) \
        callmeth = class_resolveclassmethod(c, \
              utf_new_char(meth), \
              utf_new_char(desc), \
              c, \
              false); \
        if (callmeth->class->classUsed != USED) {  \
	      c->classUsed = PARTUSED; \
 	      RTAaddClassInit(callmeth->class, \
 			   CLINITS_T,FINALIZE_T,ADDMARKED_T);\
	      } \
	callmeth->monoPoly = POLY; \
	addToRtaWorkList(callmeth,txt);


/*-- ----------------------------------------------------------------------------- 
      System calls 
	and 
      rtMissedIn list (missed becaused called from NATIVE &/or dynamic calls
 *-- -----------------------------------------------------------------------------*/
methodinfo *initializeRTAworklist(methodinfo *m) {
  	classinfo  *c;
        methodinfo* callmeth;
		char systxt[]    = "System     Call :";
	char missedtxt[] = "rtMissedIn Call :";

	FILE *rtMissedIn; /* Methods missed during previous RTA parse */
	char line[256];
	char* class, *meth, *desc;
	methodinfo *rm =NULL;  /* return methodinfo ptr to main method */


        /* Create RTA call work list */
	rtaWorkList = NEW(list);
        list_init(rtaWorkList, OFFSET(rtaNode,linkage) );

	/* Add first method to call list */
       	m->class->classUsed = USED; 
	addToRtaWorkList(m,systxt);
	/* Add system called methods */
/*** 	SYSADD(mainstring, "main","([Ljava/lang/String;)V",systxt) ***/
 	SYSADD(MAINCLASS, MAINMETH, MAINDESC,systxt)
	rm = callmeth;  
/*** 	SYSADD("java/lang/System","exit","(I)V",systxt) ***/
 	SYSADD(EXITCLASS, EXITMETH, EXITDESC, systxt)
	/*----- rtMissedIn 0 */
        if ( (rtMissedIn = fopen("rtMissedIn0", "r")) == NULL) {
		/*if (opt_verbose) */
		    {printf("No rtMissedIn0 file\n");fflush(stdout);} 
		return  rm;
		}
	while (RTAgetline(line,256,rtMissedIn)) {
	    class = strtok(line, " \n");
	    meth  = strtok(NULL, " \n");
	    desc  = strtok(NULL, " \n");
 		SYSADD(class,meth,desc,missedtxt)
		}
	fclose(rtMissedIn);




	return rm;

}

/*- end initializeRTAworklist-------- */



/*-------------------------------------------------------------------------------*/
methodinfo *missedRTAworklist()  
{
	FILE *rtMissedIn; /* Methods missed during previous RTA parse */
	char filenameIn[256] = "rtIn/";
	char line[256];
	char* class, *meth, *desc;
	char missedtxt[] = "rtIn/ missed Call :";
  	classinfo  *c;
        methodinfo* callmeth;

	methodinfo *rm =NULL;  /* return methodinfo ptr to main method */


#if defined(USE_THREADS)
	SYSADD(THREADCLASS, THREADMETH, THREADDESC, "systxt2")
	SYSADD(THREADGROUPCLASS, THREADGROUPMETH, THREADGROUPDESC, "systxt2")
#endif
	/*----- rtMissedIn pgm specific */
        strcat(filenameIn, (const char *)mainstring);  
        if ( (rtMissedIn = fopen(filenameIn, "r")) == NULL) {
		/*if (opt_verbose)*/ 
		    {printf("No rtIn/=%s file\n",filenameIn);fflush(stdout);} 
		return rm;
		}
	while (RTAgetline(line,256,rtMissedIn)) {
	    class = strtok(line, " \n");
	    meth  = strtok(NULL, " \n");
	    desc  = strtok(NULL, " \n");
	    if ((class == NULL) || (meth == NULL) || (desc == NULL))  
		panic ("Error in rtMissedIn file for: class.meth, desc\n"); 
 	    SYSADD(class,meth,desc,missedtxt)
	    }
	fclose(rtMissedIn);

	return rm;
}



/*--------------------------------------------------------*/
/* parseRTmethod                                          */
/* input: method to be RTA static parsed                  */
/*--------------------------------------------------------*/
void parseRTmethod(methodinfo *rt_method) {
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
	       METHINFOt(rt_method,"TO BE NATIVE RTA PARSED :",RTA_DEBUGopcodes);
		/* parseRTpseudo(rt_method); */
	        }   
	    else {
	       printf("Abstract method in RTA Work List: ");
	       METHINFOx(rt_method);
	       panic("Abstract method in RTA Work List.");
               }
            }            	
}


/*-- RTA -- *******************************************************/
int RT_jit_parse(methodinfo *m)
{
  rtaNode    *rta;
  methodinfo *mainmeth;

  /* Should only be called once */
  if (firstCall) {

        /*----- RTA initializations --------*/
  	if (opt_verbose) 
      	    log_text("RTA static analysis started.\n");

	mainmeth = initializeRTAworklist(m);
        firstCall = false; /* turn flag off */

    if ( (rtMissed = fopen("rtMissed", "w")) == NULL) {
        printf("CACAO - rtMissed file: cant open file to write\n");
        }
    /* Note: rtMissed must be renamed to rtMissedIn to be used as input */
	
    /*------ process RTA call work list --------*/
    for (rta =list_first(rtaWorkList); 
	 rta != NULL; 
	 rta =list_next(rtaWorkList,rta)) 
        { 
	parseRTmethod(rta->method);
    	}	
    missedRTAworklist();  
    for (rta =list_first(rtaWorkList); 
	 rta != NULL; 
	 rta =list_next(rtaWorkList,rta)) 
        { 
	parseRTmethod(rta->method);
    	}	

    fclose(rtMissed);
    if (opt_verbose) {
        if (opt_stat) {
          printRThierarchyInfo(m); 
	  }
      printCallgraph(rtaWorkList); 
      }

    if (opt_verbose) {
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

