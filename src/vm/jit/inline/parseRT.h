/********************** parseRT.h ******************************************
  Parser and print functions for Rapid Type Analyis
  used to only compile methods that may actually be used.
***************************************************************************/
#include "natcalls.h"

#include "parseRTprint.h"    /* RTAPRINT trace/info/debug prints  */

/*------------ Method /Class Used Markers -------------------------------*/
#define USED 1
#define NOTUSED 0
#define JUSTMARKED -1

/* class only */ 
#define METH_USED_BY_SUB -1
#define MARKEDSUPER -2
 
/*------------ global variables -----------------------------------------*/
int methRT = 0;            
int methRTlast = -1;;      
int methRTmax=5000;        
methodinfo *callgraph[5000];          

static bool nativecallcompdone=0 ;

static bool mainStarted = false;
static bool firstCall= true;
static FILE *rtMissed;   /* Methods missed during RTA parse of Main  */
		  /*   so easier to build dynmanic calls file */
static FILE *dynClasss;  /* Classes /methods used, but seen by static analysis */

static utf *INIT    ; //= utf_new_char("<init>");
static utf *CLINIT  ; //= utf_new_char("<clinit>");
static utf *FINALIZE;  // = utf_new_char("finalize");
static int missedCnt = 0;

/*--- Statistics ----------------------------------------------------------*/

int unRTclassHeirCnt=0;
int unRTmethodCnt = 0;

/*-----*/
int RTclassHeirNotUsedCnt=0; 
int RTclassHeirUsedCnt=0;    
int RTclassHeirBySubCnt=0;
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

int RTmethodFinal100  = 0;
int RTmethodStatic100 = 0;
int RTmethodFinalStatic100 = 0;
int RTmethodNoSubs100 = 0;

#define MAXCODLEN 10

int RTmethodNoSubsAbstract = 0;
int RTmethod1Used  = 0;

/*------------- RTAprint flags ------------------------------------------------------------------*/
int pCallgraph  = 0;    /* 0 - dont print 1 - print at end from main                             */ 
                        /* 2 - print at end of RT parse call                                     */
                        /* 3- print after each method RT parse                                   */
int pClassHeir  = 1;    /* 0 - dont print 1 - print at end from main                             */
                        /* 2 - print at end of RT parse call  3-print after each method RT parse */
int pClassHeirStatsOnly = 1;  /* Print only the statistical summary info for class heirarchy     */

int pOpcodes    = 0;    /* 0 - don't print 1- print in parse RT 2- print in parse                */
                        /* 3 - print in both                                                     */
int pWhenMarked = 0;    /* 0 - don't print 1 - print when added to callgraph + when native parsed*/
                        /* 2 - print when marked+methods called                                  */
                        /* 3 - print when class/method looked at                                 */
int pStats = 0;         /* 0 - don't print; 1= analysis only; 2= whole unanalysed class heirarchy*/

/*-----------------------------------------------------------------------------------------------*/

void printCallgraph ()
  { int i;

  for (i=0;i<=methRTlast;i++) {
    printf("  (%i): ",i);
    utf_display(callgraph[i]->class->name);
    printf(":");
    method_display(callgraph[i]);
    }

  printf("\n");
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
	         printf("Methods used:\n");
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
		panic("METHOD marked used in CLASS marked NOTUSED\n"); 
		}
	     }
	  }
	}

    if (class->classUsed != NOTUSED) {
        if (pClassHeirStatsOnly >= 2) {
	  printf("\nClass: "); 
          utf_display(class->name);    
	  printf(" <%i> (depth=%i) ",class->classUsed,class->index);
	  }
        if (class->classUsed == METH_USED_BY_SUB) {
            if (pClassHeirStatsOnly >= 2) {
              printf("\tClass not instanciated - but methods resolved to this class' code\n");
	      }
	    RTclassHeirBySubCnt++;
	    }	
        else {
          if (class->classUsed == MARKEDSUPER) {
            if (pClassHeirStatsOnly >= 2) {
              printf("\tClass not instanciated - but used by super init\n");
	      }
	    RTclassHeirSuperCnt++;
            }		
           else {
              if (pClassHeirStatsOnly >= 2) {
                printf("\n");
		}
	      RTclassHeirUsedCnt++;
	      }
          }


	/* Print methods used */
	cnt=0;
        for (m=0; m < class->methodscount; m++) {
            meth = &class->methods[m];
		
	    if (meth->methodUsed == NOTUSED)	RTmethodNotUsedCnt2++; 
	    if (meth->methodUsed == NOTUSED)	RTmethodNotUsedCnt++; 
	    if (meth->methodUsed == JUSTMARKED) RTmethodMarkedCnt++;
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
		     					
		if (pClassHeirStatsOnly >= 2) {
	          if (cnt == 0) {
	             printf("Methods used:\n");
	             }
	          cnt++;
	          printf("\t");
	          utf_display(meth->class->name); 
		  printf(".");
		  method_display(meth);
		  }
	       }
            }
         if (pClassHeirStatsOnly >= 2) {
	   if (cnt > 0) printf("> %i of %i methods used\n",cnt, class->methodscount);
	   }
         }

    for (subs = class->sub;subs != NULL;subs = subs->nextsub) {
	printRTClassHeirarchy(subs);
        }
}
/*--------------------------------------------------------------*/

void printRThierarchyInfo(methodinfo *m) {

  /*-- init for statistics --*/
  RTclassHeirNotUsedCnt=0; 
  RTclassHeirUsedCnt=0;    
  RTclassHeirBySubCnt=0;   
  RTclassHeirSuperCnt=0;   
  RTmethodNotUsedCnt = 0; 
  RTmethodNotUsedCnt1 = 0; 
  RTmethodNotUsedCnt2 = 0;  
  RTmethodUsedCnt = 0;   
  RTmethodMarkedCnt= 0;  


  /*-- --*/
  if (pClassHeirStatsOnly >= 2) {
    printf("\nRT Class Heirarchy for ");
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
    printf("--- end  of RT info ---------------\n");
    }
 if (pClassHeirStatsOnly >= 1) {

  /*--  statistic results --*/
  printf("\n  >>>>>>>>>>>>>>>>>>>>  Analysed Class Heirarchy Statistics:\n"); 
  printf(" Used            \t#%i \tclasses\t/ Used       \t#%i methods \t of USED: %i%% \t  of ALL: %i%% \n",
		RTclassHeirUsedCnt,RTmethodUsedCnt,
		((100*RTmethodUsedCnt)/(RTmethodUsedCnt + RTmethodNotUsedCnt2)) ,
		((100*RTmethodUsedCnt)/ (RTmethodNotUsedCnt    + RTmethodUsedCnt    + RTmethodMarkedCnt)) );
  printf(" Used by Subtype \t#%i \tclasses\t/\n",RTclassHeirBySubCnt); 
  printf(" Used as Super   \t#%i \tclasses\t/\n\n",RTclassHeirSuperCnt); 
  printf(" Not Used        \t#%i \tclasses\t/\n\n",RTclassHeirNotUsedCnt); 
  printf("                 \t    \t       \t/ Just Marked \t#%i methods\n\n",RTmethodMarkedCnt); 
  printf(" In Not Used     \t    \tclasses\t/ Not Used    \t#%i methods\n",RTmethodNotUsedCnt1); 
  printf(" In Used         \t    \tclasses\t/ Not Used    \t#%i methods\n",RTmethodNotUsedCnt2);
  printf(" Total           \t#%i \tclasses\t/ Total       \t#%i methods\n\n",
	RTclassHeirNotUsedCnt + RTclassHeirUsedCnt + RTclassHeirBySubCnt + RTclassHeirSuperCnt,  
	RTmethodNotUsedCnt    + RTmethodUsedCnt    + RTmethodMarkedCnt ); 

  printf(" Inlining possible:  \tFINALs %i \tSTATICs %i \t FINAL & STATIC %i \t Class has No Subs %i \n",
	RTmethodFinal, RTmethodStatic,RTmethodFinalStatic,  RTmethodNoSubs);
  printf("    Code size < 100  \tFINALs %i \tSTATICs %i \t FINAL & STATIC %i \t Class has No Subs %i \n",
	RTmethodFinal100, RTmethodStatic100,RTmethodFinalStatic100,  RTmethodNoSubs100);
  }
}

/*--------------------------------------------------------------*/
/* addToCallgraph - adds to RTA callgraph and                   */ 
/*                  sets  meth->methodUsed  to USED             */
/*                                                              */
/* To avoid unnecessary calls and dup entries in callgraph      */
/*      meth should not be null                                 */
/*      meth->methodUsed should be NOTUSED when called          */
/*      meth's class should be USED                             */
/*                                                              */
/*--------------------------------------------------------------*/

void addToCallgraph (methodinfo * meth) {
  int mfound =0;
  int im;
  int i;
/* -- Pre-condition tests for adding method to call graph --*/
if (meth==NULL) 		{panic("Trying to add a NULL method to callgraph"); return; }
if (meth->methodUsed == USED)  	 return;  /*This should be test before fn call to avoid needless fn call */
				/* invokevirtual can be abstract	*/
				/* need to try to resolve /mark method 	*/
				/* but... need document what should be 	*/
				/* done / how to tell if doesn't resolved*/
if (meth->flags & ACC_ABSTRACT) {   //printf("addToCallGraph returning because Abstract method\n"); 
				return;}

if (meth->class->classUsed == NOTUSED) {
				if (pWhenMarked >= 1) {
 				  printf("AddToCallGraph method's class not used nor marked<%i> SUPER?\n",
					meth->class->classUsed); 
				  utf_display(meth->class->name);printf(".");
  				  utf_display(meth->name);printf("\n");
				  panic("addToCallgraph called when class was NOTUSED\n");
				  }
  return;
  }

  /*-- Add it to callgraph (mark used) --*/
       callgraph[++methRTlast] = meth ;
		RTAPRINTcallgraph1
       meth->methodUsed = USED;    
}

/*--------------------------------------------------------------*/
/* Mark the method with same name /descriptor in topmethod
/* in class
/*
/* Class not marked USED and method defined in this class -> 
/*    -> if Method NOTUSED mark method as JUSTMARKED
/* Class marked USED and method defined in this class ->
/*    -> mark method as USED

/* Class USED, but method not defined in this class ->
/*    -> search up the heirarchy and mark method where defined
/*       if class where method is defined is not USED ->
/*	 -> mark class with defined method as METH_USED_BY_SUB

/*--------------------------------------------------------------*/

void markMethod(classinfo *class, methodinfo *topmethod) {

  utf *name = topmethod -> name; 
  utf *descriptor = topmethod -> descriptor;
  s4  flags = topmethod -> flags;            

  methodinfo *submeth;
  methodinfo *initmeth;
  classinfo  *ci;
  int m;

  submeth = class_findmethod(class, name, descriptor); 

  if (submeth != NULL) {

/* Class not marked USED and method defined in this class -> 
/*    -> if Method NOTUSED mark method as JUSTMARKED
*/
    if (submeth->class->classUsed != USED) { 
	if (submeth->methodUsed == NOTUSED) { 
           submeth->methodUsed = JUSTMARKED;
			RTAPRINTmarkMethod1
    	  } }

    else {

	/* Class marked used in some way and method defined in this class ->
	/*    -> mark method as USED
	*/
      	if ((submeth ->methodUsed != USED) && (submeth->class->classUsed == USED)) {
		addToCallgraph(submeth);   
        	}  }  
      }

  else {
	/* Class USED, but method not defined in this class ->
	/*    -> search up the heirarchy and mark method where defined
	/*       if class where method is defined is not USED ->
	/*	 -> mark class with defined method as METH_USED_BY_SUB
	*/

       	if (class->classUsed == USED) {
          classinfo *s = class->super;
          int found = 0;  
 	  methodinfo *supermeth;

	  while ((s!=NULL) && (found == 0)) {
	    supermeth = class_findmethod(s, name, descriptor);	   
	    if (supermeth != NULL) { 
              found = 1;
	      if ((s->classUsed == NOTUSED) 
		|| (s->classUsed == MARKEDSUPER)) {

	        s->classUsed = METH_USED_BY_SUB; 
			 RTAPRINTmarkMethod2
                }  

	      if (supermeth->methodUsed !=USED) {   
	        addToCallgraph(supermeth);
	        }
              }   /* end if !NULL */
            else {
	      s = s->super;
              } /* end else NULL */ 
            }   /* end while */

	    if ((s == NULL) && (found == 0))
		panic("parse RT: Method not found in class hierarchy");
          }  /* if current class used  */

    } /* end else Null */
} 

/*-------------------------------------------------------------------------------*/
/* Mark the method with the same name and descriptor as topmethod
/*   and any subclass where the method is defined and/or class is used
/*
/*-------------------------------------------------------------------------------*/

void markSubs(classinfo *class, methodinfo *topmethod) {
		RTAPRINTmarkSubs1
  markMethod(class, topmethod);   /* Mark method in class where it was found */
  if (class->sub != NULL) {
     classinfo *subs;
     int    subMcnt= 0;
	
    if (!(topmethod->flags & ACC_FINAL )) {
       for (subs = class->sub;subs != NULL;subs = subs->nextsub) {
 		RTAPRINTmarkSubs1
         markSubs(subs, topmethod); 
         }
       }
    }
return;
}


/*-------------------------------------------------------------------------------*/

int addClassInit(classinfo *ci) {
// CHANGE to a kind of table look-up for a list of class/methods (currently 3)

utf* utf_java_lang_system = utf_new_char("java/lang/System"); 
utf* utf_initializeSystemClass = utf_new_char("initializeSystemClass"); 
utf* utf_java_lang_Object = utf_new_char("java/lang/Object"); 

int m, m1=-1, m2=-1, mf=-1, ii;
methodinfo *mi;

for  (m=0; m < ci->methodscount; m++) {
    /*<clnit> class init method */
    if (ci->methods[m].name == CLINIT) {
	m1=m;
	}
    /* Special case: System class has an extra initializer method */
    if 	  ((utf_java_lang_system == ci->name) 
	&& (utf_initializeSystemClass == ci->methods[m].name)) {
        m2=m;  
        }

    /* Finalize methods */
    if    ((ci->methods[m].name == FINALIZE) 
	&& (ci->name != utf_java_lang_Object)) {
        mf=m;  
        }

    }

if (m1 >= 0) { /* No <clinit>  available - ignore */  

  /* Get clinit methodinfo ptr */
  mi = class_findmethod (ci,ci->methods[m1].name , NULL); 

  if ( mi->methodUsed != USED) {
	mi->class->classUsed = USED;
	addToCallgraph(mi); 	
	}
  }

if (mf >= 0) {   

  /* Get finalize methodinfo ptr */
  mi = class_findmethod (ci,ci->methods[mf].name , NULL); 

  if ( mi->methodUsed != USED) {
	mi->class->classUsed = USED;
	addToCallgraph(mi); 	
	}
  }

/*Special Case for System class init:  
	add java/lang/initializeSystemClass to callgraph */
if (m2 >= 0) {
	/* Get clinit methodinfo ptr */
	mi = class_findmethod (ci,ci->methods[m2].name , NULL); 

	if ( mi->methodUsed != USED) {
	  mi->class->classUsed = USED;
	  addToCallgraph(mi); 	
  	  }
    }

/* add marked methods to callgraph */ 
for (ii=0; ii<ci->methodscount; ii++) { 
	if (ci->methods[ii].methodUsed == JUSTMARKED) { 
		addToCallgraph(&ci->methods[ii]); 
		}
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

static void parseRT()
{
	int  p;                     /* java instruction counter                   */
	int  nextp;                 /* start of next java instruction             */
	int  opcode;                /* java opcode                                */
	int  i;                     /* temporary for different uses (counters)    */
        bool iswide = false;        /* true if last instruction was a wide        */

		RTAPRINT01method

	/* scan all java instructions */


	for (p = 0; p < rt_jcodelength; p = nextp) {
		opcode = rt_code_get_u1 (p);           /* fetch op code                  */
	RTAPRINT02opcode	
		nextp = p + jcommandsize[opcode];   /* compute next instruction start */
   switch (opcode) {

/*--------------------------------*/
/* Code just to get the correct  next instruction */
			/* 21- 25 */
                        case JAVA_ILOAD:
                        case JAVA_LLOAD:
                        case JAVA_FLOAD:
                        case JAVA_DLOAD:
                                if (iswide)
                                  {
                                  nextp = p+3;
                                  iswide = false;
                                  }
                                break;

                        case JAVA_ALOAD:
				{
                                constant_FMIref *mr;
                                methodinfo *mi;

                                if (!iswide)
                                        i = rt_code_get_u1(p+1);
                                else {
                                        i = rt_code_get_u2(p+1);
                                        nextp = p+3;
                                        iswide = false;
                                        }
if (pWhenMarked >= 4) {
 printf("I-ALOAD %s i=%i <%x>\n", opcode_names[opcode],i,rt_jcode[p+1]);
 /*class_showconstanti(rt_class, i); */
 }
				}

                                break;

			/* 54 -58 */
		        case JAVA_ISTORE:
                        case JAVA_LSTORE:
                        case JAVA_FSTORE:
                        case JAVA_DSTORE:
                                if (iswide)
                                  {
                                  iswide=false;
                                  nextp = p+3;
                                  }
                               break;

                        case JAVA_ASTORE:
                                if (!iswide)
                                        i = rt_code_get_u1(p+1);
                                else {
                                        i = rt_code_get_u2(p+1);
                                        iswide=false;
                                        nextp = p+3;
                                        }

				if (pWhenMarked >= 4) {
					printf("I-ASTORE %s i=%i <%x>\n", opcode_names[opcode],i,rt_jcode[p+1]);
					/*class_showconstanti(rt_class, rt_jcode[p+1]);*/
 					}
                                break;
			/* 132 */
		 	case JAVA_IINC:
                                {
                                int v;

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
#include "parseEXTRAopcodes.h"   /* opcodes just for info */

                        /* managing arrays ************************************************/

			INFOP01newarray

			INFOP02anewarray

			INFOP03multianewarray

/*-------------------------------*/
                        case JAVA_PUTSTATIC:
                        case JAVA_GETSTATIC:
                                i = rt_code_get_u2(p + 1);
                                {
                                constant_FMIref *fr;
                                fieldinfo *fi;
                                classinfo *ci;
                                methodinfo *mi;
				int m;

                                fr = class_getconstant (rt_class, i, CONSTANT_Fieldref);
									 /* type of field */
                                fi = class_findfield (fr->class,fr->name, fr->descriptor);
				ci = fr->class;
					RTAPRINT03putstatic1
				addClassInit(ci);

                                }
                                break;

                        /* method invocation *****/

                        case JAVA_INVOKESTATIC:
                                i = rt_code_get_u2(p + 1);
                                {
                                constant_FMIref *mr;
                                methodinfo *mi;

                                mr = class_getconstant (rt_class, i, CONSTANT_Methodref);
                                mi = class_findmethod (mr->class, mr->name, mr->descriptor);
					RTAPRINT04invokestatic1
                                if (mi->class->classUsed == NOTUSED) {
                                    mi->class->classUsed = USED;
					RTAPRINT05invokestatic2
                                    }
				    addClassInit(mi->class);

                                if (mi->methodUsed  != USED) {  /* if static method not in callgraph */
				    addToCallgraph(mi);
                                    }
                                }
                                break;

                        case JAVA_INVOKESPECIAL:
               			i = rt_code_get_u2(p + 1);
                                {
                                constant_FMIref *mr;
                                methodinfo *mi;
				classinfo  *ci;
				int ii;
				
               			mr = class_getconstant (rt_class, i, CONSTANT_Methodref);
                                mi = class_findmethod (mr->class, mr->name, mr->descriptor);
					RTAPRINT06invoke_spec_virt1

                                if (mi->name != INIT) { /* if method called is PRIVATE */ 
						RTAPRINT07invoke_spec_virt2
				  	markSubs(mi->class,mi); 
					break;
					}
				/* new class so add marked methods */
				if ( mi->methodUsed != USED) {

					/* if parsing <init> method and it calls its super <init>
					/*   -> mark the class of super as MARKEDSUPER
					*/
					if ((INIT == mi->name) 
					&&  (INIT == rt_method->name) 
					&&   (rt_class->super == mi->class)        /* <init> calling super ? */
					&&   (mi->class->classUsed == NOTUSED))    /* only if not used at all */
						mi->class->classUsed = MARKEDSUPER;
					else {
						/* Normal <init> - mark class as USED and <init> to callgraph */
						ci = mi->class;
						ci->classUsed = USED;

						/* add marked methods to callgraph */ 
                                       		for (ii=0; ii<ci->methodscount; ii++) { 
                                       			if (ci->methods[ii].methodUsed == JUSTMARKED) { 
				               			addToCallgraph(&ci->methods[ii]); 
                                    				}
                                    			}
						}

				    	addToCallgraph(mi); /* add to call graph after setting classUsed flag */	
				}	}                                     	 
                                break;


                        case JAVA_INVOKEVIRTUAL:
                		i = rt_code_get_u2(p + 1);
                                {
                                constant_FMIref *mr;
                                methodinfo *mi;
				classinfo  *ci;
				int ii;
				
				mr = class_getconstant (rt_class, i, CONSTANT_Methodref);
                                mi = class_findmethod (mr->class, mr->name, mr->descriptor);
					RTAPRINT06invoke_spec_virt1
 
                                if (mi->name == INIT) {  
					panic("An <init> method called from invokevirtual, but invokespecial expected\n");
					return;
					}     
				/*--------------------------------------------------------------*/
						RTAPRINT07invoke_spec_virt2
				  markSubs(mi->class,mi); 

				}
                                break;

                        case JAVA_INVOKEINTERFACE:
                                i = rt_code_get_u2(p + 1);
                                {
                                constant_FMIref *mr;
                                methodinfo *mi;
                                classinfo *ci;
        			classinfo *subs;

                                mr = class_getconstant (rt_class, i, CONSTANT_InterfaceMethodref);
                                mi = class_findmethod (mr->class, mr->name, mr->descriptor);
                                if (mi->flags & ACC_STATIC)
                                        panic ("Static/Nonstatic mismatch calling static method");
				RTAPRINT08AinvokeInterface0
				ci = mi->class;
        			subs = ci->impldBy; 
					RTAPRINT08invokeInterface1
				while (subs != NULL) { 
						RTAPRINT09invokeInterface2

					/* Mark method (mark/used) in classes that implement the method */
					if (subs->classUsed != NOTUSED)
						markSubs(subs, mi);  /* method may not be found so...??? */
                			subs = subs->nextimpldBy;
					}
                                }
                                break;

                       /* miscellaneous object operations *******/

                        case JAVA_NEW:
                                i = rt_code_get_u2 (p+1);
                                {
                                classinfo *ci;
				int ii;
        			classinfo *subs;
                                ci = class_getconstant (rt_class, i, CONSTANT_Class); 
				
				/* Add this class to the implemented by list of the abstract interface */
				for (ii=0; ii < ci -> interfacescount; ii++) {
        	                	subs = ci -> interfaces [ii]->impldBy;
        	                	ci -> interfaces [ii]->impldBy = ci;
        	                	ci -> interfaces [ii]->nextimpldBy = ci;
					}

				if (ci->classUsed == NOTUSED) {
                                	int ii;
						RTAPRINT10new
                                        ci->classUsed = USED;    /* add to heirarchy    */
				    	addClassInit(ci);
					/* add marked methods to callgraph  ?? here or in init??? */ 
                                    	} 
                                }
                                break;

                        default:
                                break;

                        } /* end switch */


		} /* end for */

	if (p != rt_jcodelength)
		panic("Command-sequence crosses code-boundary");

}

/*-------------------------------------------------------------------------------*/
void   findMarkNativeUsedMeth (utf * c1, utf* m1, utf* d1) {

classinfo  *class;
classinfo  *subs;
classinfo  *ci;
methodinfo *meth;
int m;
int ii;

class = class_get(c1);
if (class == NULL)  {
	return;    /*Note: Since NativeCalls is for mult programs some may not be loaded - that's ok */
	}

if (class->classUsed == NOTUSED) {
	class->classUsed = USED; // MARK CLASS USED????
	/* add marked methods to callgraph */ 
	for (ii=0; ii<class->methodscount; ii++) { 
		if (class->methods[ii].methodUsed == JUSTMARKED) { 
			addToCallgraph(&class->methods[ii]); 
			}
		}    
	}

meth = class_findmethod (class, m1, d1);
if (meth == NULL) {
	utf_display(class->name);printf(".");utf_display(m1);printf(" ");utf_display(d1);
	panic("parseRT:  Method given is used by Native method call, but NOT FOUND");
	}
markSubs(class,meth);
}

/*-------------------------------------------------------------------------------*/

void   findMarkNativeUsedClass (utf * c) {
classinfo  *class;
int ii;

class = class_get(c);
if (class == NULL)  panic("parseRT: Class used by Native method called not loaded!!!");
class->classUsed = USED;

/* add marked methods to callgraph */
for (ii=0; ii<class->methodscount; ii++) {
  if (class->methods[ii].methodUsed == JUSTMARKED) {
	if (class->methods[ii].methodUsed == JUSTMARKED) {
               	addToCallgraph(&class->methods[ii]);
               	}
        }
  }

}


/*-------------------------------------------------------------------------------*/

void markNativeMethodsRT(utf *rt_class, utf* rt_method, utf* rt_descriptor) {
int i,j,k;
bool found = false;
classinfo *called;

nativecallcompdone = natcall2utf(nativecallcompdone); 

for (i=0; i<NATIVECALLSSIZE; i++) {
  if (rt_class  == nativeCompCalls[i].classname) {
    /* find native class.method invoked */
    for (j=0; (!(found) && (j<nativeCompCalls[i].methCnt)); j++) {
      if ( (rt_method     == nativeCompCalls[i].methods[j].methodname)
        && (rt_descriptor == nativeCompCalls[i].methods[j].descriptor)) {

        found=true;

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
void mainRTAparseInit ( )
{
//printf("MAIN_NOT_STARTED \n"); 
if (class_java_lang_Object->sub != NULL) { 
	RTAPRINT16stats1
	}

if (firstCall) {
	firstCall=false;

	if ( (rtMissed = fopen("rtMissed", "w")) == NULL) {
    		printf("CACAO - rtMissed file: can't open file to write\n");
   		 }
	else {
		fprintf(rtMissed,"To Help User create a dymLoad file \n");
		fprintf(rtMissed,
		  "Not parsed in the static analysis parse of Main: #rt parse / #missed class.method (descriptor) \n");
		fprintf(rtMissed,"\n\tBEFORE MAIN RT PARSE\n");
		fflush(rtMissed);
		fclose(rtMissed);
		}

	}

	// At moment start RTA before main when parsed
	// Will definitely use flag with to know if ok to apply in-lining.
}



/*-------------------------------------------------------------------------------*/

void RT_jit_parse(methodinfo *m)
{
utf *utf_MAIN = utf_new_char("main");

	if (m->methodUsed == USED) return;

		INIT    = utf_new_char("<init>");
		CLINIT  = utf_new_char("<clinit>");
		FINALIZE = utf_new_char("finalize");

	if (!mainStarted) { 
		mainRTAparseInit ();
		}
	if (m->name == utf_MAIN) {
		rtMissed = fopen("rtMissed","a");
		fprintf(rtMissed,"\n\n\tAFTER MAIN RT PARSE\n");
		fclose(rtMissed);
		}
	else {  
		if ( (rtMissed = fopen("rtMissed", "a")) == NULL) {
    			printf("CACAO - rtMissed file: can't open file to write\n");
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
		}
		


	/* initialise parameter type descriptor */
        callgraph[++methRTlast] = m;
			RTAPRINT11addedtoCallgraph 
	m->methodUsed = USED;

	/* <init> then like a new class so add marked methods to callgraph */
	if (m->name == INIT)  { 
  	  classinfo *ci;
	  int ii;
		ci = m->class;
		ci->classUsed = USED;
		/* add marked methods to callgraph */
			RTAPRINT11addedtoCallgraph2
		for (ii=0; ii<ci->methodscount; ii++) { 
			if (ci->methods[ii].methodUsed == JUSTMARKED) { 
				addToCallgraph(&ci->methods[ii]); 
				}
			} /* for */
	  } /* if */

        while (methRT <= methRTlast) {
            rt_method      = callgraph[methRT];
	    rt_class       = rt_method->class;
	    rt_descriptor  = rt_method->descriptor;
	    rt_jcodelength = rt_method->jcodelength;
	    rt_jcode       = rt_method->jcode;

            if (! (  (rt_method->flags & ACC_NATIVE  )
		||   (rt_method->flags & ACC_ABSTRACT) ) )
	      parseRT();
	    else {
		if (pOpcodes == 1) {
		  printf("\nPROCESS_abstract or native\n");
		  utf_display(rt_method->class->name); printf(".");
	  	  method_display(rt_method); printf("\n"); fflush(stdout);
		  }

            	if (rt_method->flags & ACC_NATIVE ) {
		  /* mark used and add to callgraph methods and classes used by NATIVE method */
			RTAPRINT12aNative
		  markNativeMethodsRT(rt_class->name,rt_method->name,rt_descriptor);		  
		  }
		if (rt_method->flags & ACC_ABSTRACT) {
		  printf("ABSTRACT_SHOULD not ever get into the callgraph!!!!!****!!!****!!!!****!!!!\n"); 
		  }
		}
	      methRT++;
			RTAPRINT12Callgraph 
			RTAPRINT13Heirarchy 
        } /* while */
	if (m->class->classUsed == NOTUSED)
		m->class->classUsed = USED; /* say Main's class has a method used ??*/ 
			RTAPRINT14CallgraphLast
			RTAPRINT15HeirarchyiLast
}
