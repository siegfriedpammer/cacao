/* jit/parseRTstats.c -

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

   $Id: parseRTstats.c 620 2003-11-13 13:50:10Z twisti $

*/


#include "parseRT.h"
#include "loader.h"
#include "tables.h"
#include "toolbox/loging.h"


/*--- Statistics ----------------------------------------------------------*/

int unRTclassHeirCnt=0;
int unRTmethodCnt = 0;

/*-----*/
int RTclassHeirNotUsedCnt=0; 
int RTclassHeirUsedCnt=0;    
int RTclassHeirPartUsedCnt=0;
int RTclassHeirSuperCnt=0;

int RTmethodNotUsedCnt = 0;
int RTmethodNotUsedCnt1= 0;
int RTmethodNotUsedCnt2= 0;
int RTmethodUsedCnt = 0;
int RTmethodMarkedCnt= 0;

/* What might be inlined of the Used Methods */
int RTmethodFinal  = 0;
int RTmethodStatic = 0;
int RTmethodFinalStatic = 0;
int RTmethodNoSubs = 0;

int RTmethodMono; 
int RTmethodPossiblePoly;
int RTmethodPolyReallyMono;
int RTmethodPoly;

int RTmethodFinal100  = 0;
int RTmethodStatic100 = 0;
int RTmethodFinalStatic100 = 0;
int RTmethodNoSubs100 = 0;

#define MAXCODLEN 10

int RTmethodNoSubsAbstract = 0;
int RTmethod1Used  = 0;

int RTmethodAbstract = 0;

int subRedefsCnt =0;
int subRedefsCntUsed =0;

/*------------- RTAprint flags ------------------------------------------------------------------*/
int pCallgraph  = 0;    /* 0 - dont print 1 - print at end from main                             */ 
			/* 2 - print at end of RT parse call                                     */
			/* 3- print after each method RT parse                                   */
int pClassHeir  = 1;    /* 0 - dont print 1 - print at end from main                             */
			/* 2 - print at end of RT parse call  3-print after each method RT parse */
int pClassHeirStatsOnly = 1;  /* usually 2 Print only the statistical summary info for class heirarchy     */

int pOpcodes    = 0;    /* 0 - don't print 1- print in parse RT 2- print in parse                */
			/* 3 - print in both                                                     */
int pWhenMarked = 0;    /* 0 - don't print 1 - print when added to callgraph + when native parsed*/
			/* 2 - print when marked+methods called                                  */
			/* 3 - print when class/method looked at                                 */
int pStats = 0;         /* 0 - don't print; 1= analysis only; 2= whole unanalysed class heirarchy*/

/*-----------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/
void printXTACallgraph()
{ 
	int i;

	if (XTAdebug >= 1) {
		printf("----- XTA Callgraph Worklist:<%i>\n",methXTAlast);
		for (i=0;i<=methXTAlast;i++) {
			printf("  (%i): ",i);
			utf_display(XTAcallgraph[i]->class->name);
			printf(":");
			method_display(XTAcallgraph[i]);
		}

		printf("\n\n");
	}
}

/*-----------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/

void printCallgraph ()
{ int i;

 printf("-*-*-*-*- RTA Callgraph Worklist:<%i>\n",methRTlast);
 for (i=0;i<=methRTlast;i++) {
	 printf("  (%i): ",i);
	 utf_display(callgraph[i]->class->name);
	 printf(":");
	 method_display(callgraph[i]);
 }

 printf("\n\n");
}
/*--------------------------------------------------------------*/
void printObjectClassHeirarchy1() {
	if (pStats >= 1) {
        unRTclassHeirCnt=0;
        unRTmethodCnt = 0;
		printObjectClassHeirarchy(class_java_lang_Object);
        printf("\n >>>>>>>>>>>>>>>>>>>>  END of unanalysed Class Heirarchy: #%i classes /  #%i methods\n\n",
			   unRTclassHeirCnt,unRTmethodCnt);
	}

}
/*--------------------------------------------------------------*/
void printObjectClassHeirarchy(classinfo  *class) {
  
	classinfo  *subs;
	methodinfo *meth;
	int t,m,cnt;

	if (class == NULL) {return;}
	unRTclassHeirCnt++; unRTmethodCnt += class->methodscount;
	if (pStats == 2) {
		printf("\n");
		/* Class Name */
		for (t=0;t<class->index;t++) printf("\t"); 
		if (class->flags & ACC_INTERFACE) printf("ABSTRACT ");

		printf("Class: "); 
		utf_display(class->name);    
		printf(" <%i> (depth=%i) \n",class->classUsed,class->index);
		/* Print methods used */
		cnt=0; 
		for (m=0; m < class->methodscount; m++) {
            meth = &class->methods[m];
			if (cnt == 0) {
				for (t=0;t<class->index;t++) printf("\t");
				printf("aMethods used:\n");
			}
			for (t=0;t<class->index;t++) printf("\t");
			printf("\t");
			utf_display(meth->class->name); 
			printf(".");
			method_display(meth);
			cnt++;
	    }
		if (cnt > 0) printf("> %i of %i methods\n",cnt, class->methodscount);
    }

    for (subs = class->sub;subs != NULL;subs = subs->nextsub) {
		printObjectClassHeirarchy(subs);
	}

}
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
int subdefd(methodinfo *meth) {
    classinfo *subs;
    methodinfo *submeth;

	/*printf("subdefd for:");utf_display(meth->class->name);printf(".");method_display(meth); fflush(stdout);*/

    if (  (meth->flags & ACC_STATIC) && (meth->flags & ACC_FINAL ) )  
    	panic("Possible Poly call for FINAL or STATIC\n");

    if ((meth->class->sub == NULL)  && (!(meth->flags & ACC_ABSTRACT )) ) { 
		return 0;
	}
    if (meth->flags & ACC_ABSTRACT ) ; /*printf("AB\n"); fflush(stdout); */

	/*printf("s exist for:");utf_display(meth->class->name);printf(".");method_display(meth);*/

    for (subs = meth->class->sub;subs != NULL;subs = subs->nextsub) {
		submeth = class_findmethod(subs, meth->name, meth->descriptor); 
		if (submeth != NULL) {
			subRedefsCnt++;
			if (submeth->methodUsed == USED) {
				subRedefsCntUsed++;
				/*return 1;*/
			}
			else {
				if (subdefd(submeth) > 0)
					; /*return 1;*/
			}
		}
	}
    if (subRedefsCntUsed > 0) return 1;
    return 0;
}
/*--------------------------------------------------------------*/

void printRTClassHeirarchy(classinfo  *class) {
  
	classinfo  *subs;
	methodinfo *meth;
	int m,cnt;

	if (class == NULL) {return;}
    /* Class Name */
    if (class->classUsed == NOTUSED) {
		RTclassHeirNotUsedCnt++;
		RTmethodNotUsedCnt = RTmethodNotUsedCnt + class->methodscount;
		RTmethodNotUsedCnt1 = RTmethodNotUsedCnt1 + class->methodscount;
		for (m=0; m < class->methodscount; m++) {
			meth = &class->methods[m];
			if (meth->methodUsed == USED) {
				if (pClassHeirStatsOnly >= 2) {
					printf("METHOD marked used in CLASS marked NOTUSED: "); 
					utf_display(class->name);
					printf(".");
					method_display(meth);
					printf("<%i>\n\t",meth->methodUsed);
					fflush(stdout);
					printf("\n\n\n\nMETHOD marked used in CLASS marked NOTUSED\n\n\n\n"); 
				}
			}
		}
	}

    if (class->classUsed != NOTUSED) {
        if (pClassHeirStatsOnly >= 2) {
			printf("\nClass: "); 
			utf_display(class->name);    
			printf(" <%i> (depth=%i) ",class->classUsed,class->index);

			printf("\tbase/diff =%3d/%3d\n",
				   class->vftbl->baseval,
				   class->vftbl->diffval);
		}

        if (class->classUsed == PARTUSED) {
            if (pClassHeirStatsOnly >= 2) {
				printf("\tClass not instanciated - but  methods/fields resolved to this class' code (static,inits,fields,super)\n");
			}
			RTclassHeirPartUsedCnt++;
	    }	
        else {
			if (pClassHeirStatsOnly >= 2) {
                printf("\n");
			}
			RTclassHeirUsedCnt++;
		}


		/* Print methods used */
		cnt=0;
        for (m=0; m < class->methodscount; m++) {

            meth = &class->methods[m];
		
			if (meth->methodUsed == NOTUSED)	RTmethodNotUsedCnt2++; 
			if (meth->methodUsed == MARKED)   RTmethodMarkedCnt++;
			if (meth->methodUsed == USED) {
				RTmethodUsedCnt++;
				if (  (meth->flags & ACC_FINAL ) && (!(meth->flags & ACC_STATIC)) ) { 
					RTmethodFinal++;
					if (meth->jcodelength < MAXCODLEN)  RTmethodFinal100++;
				}

				if (  (meth->flags & ACC_STATIC) && (!(meth->flags & ACC_FINAL )) ) { 
					RTmethodStatic++;
					if (meth->jcodelength < MAXCODLEN)  RTmethodStatic100++;
				}

				if (  (meth->flags & ACC_STATIC) && (meth->flags & ACC_FINAL ) ) { 
					RTmethodFinalStatic++;
					if (meth->jcodelength < MAXCODLEN)  RTmethodFinalStatic100++;
				}

				if ((! ((meth->flags & ACC_FINAL ) && (meth->flags & ACC_STATIC)) ) 
					&& ((meth->class->sub == NULL)  && (!(meth->flags & ACC_ABSTRACT)) ))    {
					RTmethodNoSubs++;
					if (meth->jcodelength < MAXCODLEN)  RTmethodNoSubs100++;
				}

				if ((! ((meth->flags & ACC_FINAL ) && (meth->flags & ACC_STATIC)) ) 
					&& ((meth->class->sub == NULL)  &&   (meth->flags & ACC_ABSTRACT)  ))    RTmethodNoSubsAbstract++;

				if (meth->flags & ACC_ABSTRACT) RTmethodAbstract++;
		     					
				if (meth->monoPoly == MONO) RTmethodMono++;
				if (meth->monoPoly == POLY) {
					RTmethodPossiblePoly++;
					subRedefsCnt = 0;
					subRedefsCntUsed = 0;
					if (meth->flags & ACC_ABSTRACT ) {
						if (pClassHeirStatsOnly >= 2) {
							printf("STATS: abstract_method=");
							utf_display(meth->class->name);printf(".");
							method_display(meth);
						}
					}
					else 	{
						if (subdefd(meth) == 0) {
							meth->monoPoly = MONO1;
							RTmethodPolyReallyMono++;
						}			
						else	{
							RTmethodPoly++;
							meth->subRedefs = subRedefsCnt;
							meth->subRedefsUsed = subRedefsCntUsed;
						}
					}
				}

				if (pClassHeirStatsOnly >= 2) {
					if (cnt == 0) {
						printf("bMethods used:\n");
					}
					cnt++;
					printf("\t");
					utf_display(meth->class->name); 
					printf(".");
					method_display(meth);
					printf("\t\t");
					if (meth->monoPoly != MONO) printf("\t\tRedefs used/total<%i/%i>\t", meth->subRedefsUsed, meth->subRedefs);
					if ( (XTAOPTbypass3) || (opt_xta)) {
						if (meth->xta->XTAclassSet == NULL)
							printf("class set never created\n");
						else
							printSet(meth->xta->XTAclassSet->head);
					}
				}
			}
		}
		if (pClassHeirStatsOnly >= 2) {
			if (cnt > 0) printf("> %i of %i methods used\n",cnt, class->methodscount);
		}
	}

    for (subs = class->sub; subs != NULL; subs = subs->nextsub) {
		printRTClassHeirarchy(subs);
	}
}
/*--------------------------------------------------------------*/
void printRTInterfaceClasses() {
	int mm;
	classinfo *ci = class_java_lang_Object;
	classSetNode *subs;

	int RTmethodInterfaceClassImplementedCnt 	= 0;
	int RTmethodInterfaceClassUsedCnt 		= 0;

	int RTmethodInterfaceMethodTotalCnt 		= 0;
	int RTmethodInterfaceMethodNotUsedCnt 	= 0;
	int RTmethodInterfaceMethodUsedCnt 		= 0;

	int RTmethodClassesImpldByTotalCnt 		= 0;

	int RTmethodInterfaceMonoCnt			= 0;
	int RTmethodInterfacePolyReallyMonoCnt=0;  /* look at every method that implments and see if its poly or mono1*/

	int RTmethodNoSubsAbstractCnt = 0;

	for (subs = ci->impldBy; subs != NULL; subs = subs->nextClass) {
        classinfo * ici = subs->classType;
		classinfo * isubs = subs->classType;
		classSetNode * inBy;
		int impldBycnt;

		if (isubs->sub == NULL) RTmethodNoSubsAbstractCnt++;
		if (pClassHeir >= 2) {
			printf("Interface class: ");fflush(stdout);
       		utf_display(ici->name); printf("\t#Methods=%i",ici->methodscount);
		}
		RTmethodInterfaceClassImplementedCnt++;
		if (ici -> classUsed == USED)  	  {RTmethodInterfaceClassUsedCnt++;}
		if (pClassHeir >= 2) {
			printf("\n\t\t\tImplemented by classes:\n");
		}
		impldBycnt = 0;
		/* get the total impldBy classes Used */
		for (inBy = ici->impldBy; inBy != NULL; inBy = inBy->nextClass) {
			impldBycnt++;
			RTmethodClassesImpldByTotalCnt++;
			if (pClassHeir >= 2) {
				printf("\t\t\t");utf_display(inBy->classType->name);
				printf("\n");
			}
			if (inBy->classType->classUsed == NOTUSED) 
				panic("printRTInterfaceClasses: class in the implemented list without being used!!!??");
		}
		if (pClassHeir >= 2) {
			printf("\t\t\tImpld by: %i\n",impldBycnt);
		}
		if (impldBycnt== 1) RTmethodInterfaceMonoCnt++;

        /* if interface class is used */
        if (ici -> classUsed != NOTUSED) {
			if (pClassHeir >= 2) {
	        	printf("    cMethods used:\n");
			}

			/* for each interface method implementation that has been used */
			for (mm=0; mm< ici->methodscount; mm++) {
				methodinfo *imi = &(ici->methods[mm]);
				RTmethodInterfaceMethodTotalCnt++;
				if  (imi->methodUsed != USED) {
					RTmethodInterfaceMethodNotUsedCnt++;
				}
				if  (imi->methodUsed == USED) {
					RTmethodInterfaceMethodUsedCnt++;
					if (pClassHeirStatsOnly >= 2) {
						printf("\t\t"); 
						utf_display(ici->name);printf(".");method_display(imi);fflush(stdout);
					}
					if (impldBycnt == 1) {
						classinfo  *cii;
						methodinfo *mii;

						/* if only 1 implementing class then possibly really mono call */
				        inBy = ici->impldBy;
						cii = inBy->classType;
						
						mii = class_findmethod(cii, imi->name, imi->descriptor); 
						if (mii == NULL) {
							/* assume its resolved up the heirarchy and just 1 possiblity so MONO1 */
							imi->monoPoly = MONO1;
							RTmethodInterfacePolyReallyMonoCnt++;
						}
						else	{
							/**if (imi->monoPoly != POLY) 
								panic ("interface monopoly not POLY");
							**/
							if (mii->monoPoly != POLY) {
								imi->monoPoly = MONO1;
								RTmethodInterfacePolyReallyMonoCnt++;
							}
							else	{
								imi->monoPoly = POLY;
							}
						}
					}
				}
			}
			if (pClassHeir >= 2) {
				printf("\n");
			}
		}
	}
	if (pClassHeirStatsOnly >= 1) {
		printf("\n\n  >>>>>>>>>>>>>>>>>>>>  Interface Statistics Summary: \n");
		printf("Classes:  Total:   %i \tUSED:      %i \tIMPLD BY:   \t%i \tJUST 1 IMPLD BY:  %i \tNOSUB:     %i \n",
			   RTmethodInterfaceClassImplementedCnt,
			   RTmethodInterfaceClassUsedCnt,RTmethodClassesImpldByTotalCnt, RTmethodInterfaceMonoCnt,
			   RTmethodNoSubsAbstractCnt);
		printf("Methods:  Total:   %i \tNOTUSED:   %i  \tUSED:      \t%i \tPoly that resolves to Mono  %i \n",
			   RTmethodInterfaceMethodTotalCnt,
			   RTmethodInterfaceMethodNotUsedCnt,RTmethodInterfaceMethodUsedCnt, RTmethodInterfacePolyReallyMonoCnt);
	}
}
/*--------------------------------------------------------------*/

void printRThierarchyInfo(methodinfo *m) {

	/*-- init for statistics --*/
	RTclassHeirNotUsedCnt=0; 
	RTclassHeirUsedCnt=0;    
	RTclassHeirPartUsedCnt=0;   
	RTclassHeirSuperCnt=0;   
	RTmethodNotUsedCnt = 0; 
	RTmethodNotUsedCnt1 = 0; 
	RTmethodNotUsedCnt2 = 0;  
	RTmethodUsedCnt = 0;   
	RTmethodMarkedCnt= 0;  


	/*-- --*/
	if (pClassHeirStatsOnly >= 2) {
		printf("\nRT Class Hierarchy for ");
		printf("--- start of RT info --------------- after :\n");
		if (m != NULL) {
			utf_display(m->class->name); 
			printf(".");
			method_display(m);
			printf("\n");
		}
    }
	printRTClassHeirarchy(class_java_lang_Object);
	if (pClassHeirStatsOnly >= 2) {
		fflush(stdout);
		printf("--- end  of RT info ---------------\n");
    }
	if (pClassHeirStatsOnly >= 1) {
		/*--  statistic results --*/
		printRTInterfaceClasses();
	
		printf("\n  >>>>>>>>>>>>>>>>>>>>  Analysed Class Hierarchy Statistics:\n"); 
		printf(" Used            \t%i \tclasses\t/ Used       \t%i methods \t of USED: %i%% \t  of ALL: %i%% \n",
			   RTclassHeirUsedCnt,RTmethodUsedCnt,
			   ((100*RTmethodUsedCnt)/(RTmethodUsedCnt + RTmethodNotUsedCnt2)) ,
			   ((100*RTmethodUsedCnt)/ (RTmethodNotUsedCnt    + RTmethodUsedCnt    + RTmethodMarkedCnt)) );
		printf(" Part Used       \t%i \tclasses\t/\n",RTclassHeirPartUsedCnt); 
		printf(" Not Used        \t%i \tclasses\t/\n\n",RTclassHeirNotUsedCnt); 
		printf("                 \t    \t       \t/ Just Marked \t%i methods\n\n",RTmethodMarkedCnt); 
		printf(" In Not Used     \t    \tclasses\t/ Not Used    \t%i methods\n",RTmethodNotUsedCnt1); 
		printf(" In Used         \t    \tclasses\t/ Not Used    \t%i methods\n",RTmethodNotUsedCnt2);
		printf(" Total           \t%i \tclasses\t/ Total       \t%i methods\n\n",
			   RTclassHeirNotUsedCnt + RTclassHeirUsedCnt + RTclassHeirPartUsedCnt,  
			   RTmethodNotUsedCnt1 + RTmethodNotUsedCnt2    + RTmethodUsedCnt    + RTmethodMarkedCnt ); 

		printf(" Mono vs. Polymorphic calls:\n");
		printf(" Mono calls     \t%i   \tPoly that resolves to Mono \t%i \tPoly calls     \t%i\n\n",
			   RTmethodMono, RTmethodPolyReallyMono, RTmethodPoly);

		printf(" No Subs: Total=\t%i   \tAbstract No Subs=           \t%i \tAbstract methods used =\t%i\n",
			   RTmethodNoSubs, RTmethodNoSubsAbstract, RTmethodAbstract);

		printf(" Inlining possible:  \tFINALs %i \tSTATICs %i \t FINAL & STATIC %i \t Class has No Subs %i \n",
			   RTmethodFinal, RTmethodStatic,RTmethodFinalStatic,  RTmethodNoSubs);
		printf("    Code size < 100  \tFINALs %i \tSTATICs %i \t FINAL & STATIC %i \t Class has No Subs %i \n",
			   RTmethodFinal100, RTmethodStatic100,RTmethodFinalStatic100,  RTmethodNoSubs100);
	}
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
