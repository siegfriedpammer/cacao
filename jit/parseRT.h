/********************** parseRT.h ******************************************
  Parser and print functions for Rapid Type Analyis
  used to only compile methods that may actually be used.
***************************************************************************/
#include "natcalls.h"

#include "parseRTprint.h"    /* RTAPRINT trace/info/debug prints  */
#include "sets.h"
 
/*------------ global variables -----------------------------------------*/
#define MAXCALLGRAPH 5000

bool XTAOPTbypass = false;
bool XTAOPTbypass2 = false;   /* for now  invokeinterface     */
bool XTAOPTbypass3 = false;   /* print XTA classsets in stats */
int  XTAdebug = 0; 
int  XTAfld = 0; 

int I;  	/* ASTORE /ALOAD index */

int methRT = 0;            
int methRTlast = -1;;      
int methRTmax=MAXCALLGRAPH;        
methodinfo *callgraph   [MAXCALLGRAPH];         

 
int methXTA = 0;            
int methXTAlast = -1;;      
int methXTAmax=MAXCALLGRAPH;        
methodinfo *XTAcallgraph[MAXCALLGRAPH];          

static bool nativecallcompdone=0 ;

static bool firstCall= true;
static FILE *rtMissed;   /* Methods missed during RTA parse of Main  */
		         /*   so easier to build dynmanic calls file */

static utf *utf_MAIN;   /*  utf_new_char("main"); */
static utf *INIT    ;   /*  utf_new_char("<init>"); */
static utf *CLINIT  ;   /*  utf_new_char("<clinit>"); */
static utf *FINALIZE;   /*  utf_new_char("finalize"); */
static utf *EMPTY_DESC; /*  utf_new_char("V()");  */
static int missedCnt = 0;

static bool useArrayOpcodes = false;
static bool useFieldOpcodes = false;
static bool useObjectrefOpcodes = false;
static bool useOtherOpcodes = false;

static s4 currentXTAround = 0;
static s4 prevXTAround    = -1;

#include "jit/parseRTstats.h"

/*--------------------------------------------------------------*/
/* addToCallgraph - adds to RTA callgraph and                   */ 
/*                  sets  meth->methodUsed  to USED             */
/*--------------------------------------------------------------*/  
#define ADDTOCALLGRAPH(meth)  if ((meth->methodUsed != USED) && (!(meth->flags & ACC_ABSTRACT)) ) { \
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
	}


/*--------------------------------------------------------------*/
bool rtaSubUsed(classinfo *class, methodinfo *meth) {
	classinfo *subs;

	for (subs=class->sub; subs != NULL; subs = subs->nextsub) {
		if (subs->classUsed == USED) {
			if (class_findmethod(class, meth->name, meth->descriptor) == NULL) 
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
/* Mark the method with same name /descriptor in topmethod
/* in class
/*
/* Class not marked USED and method defined in this class -> 
/*    -> if Method NOTUSED mark method as MARKED 
/* Class marked USED and method defined in this class ->
/*    -> mark method as USED

/* Class USED, but method not defined in this class ->
/*    -> search up the heirarchy and mark method where defined
/*       if class where method is defined is not USED ->
/*	 -> mark class with defined method as PARTUSED 

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

		/* Class NOT marked USED and method defined in this class -> 
		/*    -> if Method NOTUSED mark method as  MARKED  */
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
		/* Class IS  marked USED and method defined in this class ->
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
/* Mark the method with the same name and descriptor as topmethod
/*   and any subclass where the method is defined and/or class is used
/*
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
/* Add Marked methods for input class ci 
/* Add methods with the same name and descriptor as implemented interfaces
/*   with the same method name
/*
/*-------------------------------------------------------------------------------*/
void addMarkedMethods(classinfo *ci) {
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
/*  XTA Functions */
/*-------------------------------------------------------------------------------*/
bool xtaPassParams (methodinfo *SmCalled, methodinfo *SmCalls, methSetNode *lastptrInto) {

classSetNode *p;
classSetNode *c;
classSetNode *c1;
classSetNode *cprev;
bool          rc = false;

	if (XTAdebug >= 1) {
		printf("\n>>>>>>>>>>>>>>>>><<<xtaPassParams \n");fflush(stdout);

		printf("\tIN SmCalled set : "); 
		utf_display(SmCalled->class->name);printf("."); method_display(SmCalled);
		printClassSet(SmCalled->XTAclassSet); printf("\n"); 

		printf("\tIN SmCalls set: "); 
		utf_display(SmCalls->class->name);printf("."); method_display(SmCalls);
		printClassSet(SmCalls->XTAclassSet); printf("\n"); 
		
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
if (SmCalled->paramClassSet == NULL) {
	SmCalled->paramClassSet = descriptor2typesL(SmCalled); 
	}
	if (XTAdebug >= 1) {
		printf("\tParamPassed\n"); fflush(stdout);
		printSet(SmCalled->paramClassSet);fflush(stdout);
		printf("\n"); fflush(stdout);
		}

if (lastptrInto->lastptrIntoClassSet2 == NULL) {
	if (SmCalls->XTAclassSet != NULL) 
		c1 = SmCalls->XTAclassSet->head;
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
for (	p=SmCalled->paramClassSet; p != NULL; p = p->nextClass) {

	/* for each SmCalls class */
	for (c=c1; c != NULL; c = c->nextClass) {
		vftbl *p_cl_vt = p->classType->vftbl; 
		vftbl *c_cl_vt = c->classType->vftbl; 

		/* if SmCalls class is in the Params Class range */
		if (  (p_cl_vt->baseval <=  c_cl_vt->baseval)
		   && (c_cl_vt->baseval <= (p_cl_vt->baseval+p_cl_vt->diffval)) ) {

			/*    add SmCalls class to SmCalledBy Class set */
			SmCalled->XTAclassSet = SmCalled->XTAclassSet = add2ClassSet(SmCalled->XTAclassSet, c->classType); 
			rc = true;
			}
		cprev = c;
		}	
	}
lastptrInto->lastptrIntoClassSet2 = cprev;
			if (XTAdebug >= 1) {
				printf("\tOUT SmCalled set: ");fflush(stdout);
				printClassSet(SmCalled->XTAclassSet);fflush(stdout);

				printf("\tOUT SmCalls set: ");fflush(stdout);
				printClassSet(SmCalls->XTAclassSet);fflush(stdout);

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
if ((SmCalled->returnclass == NULL) && (SmCalled->paramClassSet == NULL)) {
	SmCalled->paramClassSet = descriptor2typesL(SmCalled); 
	}

if (SmCalled->returnclass == NULL) {
		if (XTAdebug >= 1)
			printf("\tReturn type is NULL\n");
	return;
	}
	
	if (XTAdebug >= 1) {
		printf("\tReturn type is: ");
		utf_display(SmCalled->returnclass->name);
		printf("\n");

		printf("\tIN SmCalls set: ");
		utf_display(SmCalls->class->name); printf("."); method_display(SmCalls);
		printClassSet(SmCalls->XTAclassSet);

		printf("\tIN SmCalled set: ");
		utf_display(SmCalled->class->name); printf("."); method_display(SmCalled);
		printClassSet(SmCalled->XTAclassSet);
		}


if (SmCalled->XTAclassSet == NULL) 
	cs1 = NULL;
else
	cs1 =  SmCalled->XTAclassSet->head;
for (cs =cs1; cs != NULL; cs = cs->nextClass) {
	classinfo *c = cs->classType;
	vftbl *r_cl_vt = SmCalled->returnclass->vftbl; 
	vftbl *c_cl_vt = c->vftbl; 

	/* if class is a subtype of the return type, then add to SmCalls class set (ie.interscection)*/
	if (  (r_cl_vt->baseval <=  r_cl_vt->baseval)
	   && (c_cl_vt->baseval <= (r_cl_vt->baseval+r_cl_vt->diffval)) ) {
		SmCalls->XTAclassSet = add2ClassSet(SmCalls->XTAclassSet, c);  
		rc = true;
		}
	} 

	if (XTAdebug >= 1) {
		printf("\tOUT SmCalls set: ");
		printClassSet(SmCalls->XTAclassSet);
		}
return rc;
}

/*-------------------------------------------------------------------------------*/
void xtaAddCallEdges(methodinfo *mi, s4 monoPoly) {

	if (mi->XTAmethodUsed  != USED) {  /* if static method not in callgraph */
		XTAcallgraph[++methXTAlast] = mi;
		mi->XTAmethodUsed = USED;
				XTAPRINTcallgraph2
		}
	/* add call edges */
	rt_method->calls = add2MethSet(rt_method->calls, mi);
	rt_method->calls->tail->monoPoly = monoPoly;
	mi->calledBy     = add2MethSet(mi->calledBy,     rt_method); 
if (mi->calledBy     == NULL) panic("mi->calledBy is NULL!!!");
if (rt_method->calls == NULL) panic("rt_method->calls is NULL!!!");
}


/*--------------------------------------------------------------*/
bool xtaSubUsed(classinfo *class, methodinfo *meth, classSetNode *subtypesUsedSet) {
	classinfo *subs;

	for (subs=class->sub; subs != NULL; subs = subs->nextsub) {
		/* if class used */
		if (inSet(subtypesUsedSet,subs)) {
			if (class_findmethod(class, meth->name, meth->descriptor) == NULL) 
				return false;
			else 	
				return true;
			}
		if (xtaSubUsed(subs, meth,  subtypesUsedSet)) 
			return false;
		}
	return false;
}


/*-------------------------------------------------------------------------------*/
void xtaMarkMethod(classinfo *class, methodinfo *topmethod, classSetNode *subtypesUsedSet)
{
  utf *name = topmethod -> name;
  utf *descriptor = topmethod -> descriptor;
  methodinfo *submeth;

/****
printf("xtaMarkMethod for:"); utf_display(class->name);fflush(stdout);
  method_display(topmethod);
  submeth = class_resolvemethod(class, name, descriptor);
printf(" def: "); utf_display(submeth->class->name);fflush(stdout);
  method_display(submeth);
****/

  /* Basic checks */
  if (submeth == NULL)
        panic("parse XTA: Method not found in class hierarchy");

  if (rt_method->calls != NULL) {
	if (inMethSet(rt_method->calls->head,submeth)) return;
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
				}
			}
		else	{
			rt_method->marked = add2MethSet(rt_method->marked, submeth);
			}
		}
	}
  else  {
        /*--- Method NOT defined in class -----------------------------*/
	if (!(inSet(subtypesUsedSet,submeth->class) )){  /* class with method def     is not used */
		if (!(inSet(subtypesUsedSet,class) )) { /* class currently resolving is not used */ 
			rt_method->marked = add2MethSet(rt_method->marked, submeth);
			/*printf("Added to marked Set: "); fflush(stdout);printMethodSet(rt_method->marked);*/
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
return;
}

/*-------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------*/

int addClassInit(classinfo *ci) {
/* CHANGE to a kind of table look-up for a list of class/methods (currently 3)
*/

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

if (mf >= 0) {   

	/* Get finalize methodinfo ptr */
	mi = class_findmethod (ci,ci->methods[mf].name , NULL); 

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

/*Special Case for System class init:  
	add java/lang/initializeSystemClass to callgraph */
if (m2 >= 0) {
	/* Get clinit methodinfo ptr */
	mi = class_findmethod (ci,ci->methods[m2].name , NULL); 

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

/* add marked methods to callgraph */ 
addMarkedMethods(ci); 
		
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
/*xx*/ void addUsedInterfaceMethods(classinfo *ci) {
int ii,jj,mm;

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
	printf("MAY ADD methods that was used by an interface\n");
	}
				rtaMarkSubs(ci,imi);
                                }
                        }
                }
	}

}
/*-------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------*/
void xtaMarkInterfaceSubs(methodinfo *mCalled) {
	classSetNode * Si;
	classinfo    * Smi;
	
	/* for every class that implements the interface of the method called */
	for (Si = mCalled->class->impldBy; Si != NULL; Si = Si->nextClass) {
		/* add all definitions of this method for this interface */
		methodinfo *submeth;

		submeth = class_findmethod(Si->classType, mCalled->name, mCalled->descriptor); 
		if (submeth == NULL) ; /* search up the heir - ignore for now!!! */
		else	{
			classSetNode *subtypesUsedSet = NULL;
					
			if (rt_method->XTAclassSet != NULL)
				subtypesUsedSet = intersectSubtypesWithSet(submeth->class, rt_method->XTAclassSet->head);
				
						printf(" \nXTA subtypesUsedSet: "); fflush(stdout);
						printSet(subtypesUsedSet);
			xtaMarkSubs(submeth->class, submeth, subtypesUsedSet);   
			}
		}
}

/*-------------------------------------------------------------------------------*/
bool xtaAddFldClassTypeInfo(fieldinfo *fi) {

bool rc = false;

if (fi->fieldChecked) {
	if (fi->fldClassType != NULL)
		return true;  /* field has a class type */
	else
		return false;
	}
fi->fieldChecked = true;

if (fi->type == TYPE_ADDRESS) {
	char *utf_ptr = fi->descriptor->text;  /* current position in utf text */

	if (*utf_ptr != 'L') {
		while (*utf_ptr++ =='[') ;
			}

	if (*utf_ptr =='L') {
		rc = true;
		if  (fi->fldClassType== NULL) {
			char *desc;
			char *cname;
			classinfo * class;

			desc =       MNEW (char, 256);
			strcpy (desc,++utf_ptr);
			cname = strtok(desc,";");
					if (XTAdebug >= 1) {
						printf("STATIC field's type is: %s\n",cname);
						fflush(stdout);
						}
			class = class_get(utf_new_char(cname));
			fi->fldClassType= class;    /* save field's type class ptr */	
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
	if (rt_method->XTAclassSet != NULL)
		c1  = rt_method->XTAclassSet->head;

			if (XTAfld >=1 ) {
				printf("rt XTA class set =");fflush(stdout);
				printClassSet(rt_method->XTAclassSet);
				printf("\t\tField class type = ");fflush(stdout);
				utf_display(fi->fldClassType->name); printf("\n");
				}
		}

/*--- PUTSTATIC specific ---*/
/* Sx = intersection of type+subtypes(field x)   */
/*   and Sm (where putstatic code is)            */
for (c=c1; c != NULL; c=c->nextClass) {
	vftbl *f_cl_vt = fi->fldClassType->vftbl;
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
			utf_display(fi->fldClassType->name);
			printf("<b=%i/+d=%i> \n",f_cl_vt->baseval,(f_cl_vt->baseval+f_cl_vt->diffval)); fflush(stdout);
			}

	if ((f_cl_vt->baseval <= c_cl_vt->baseval)
	&& (c_cl_vt->baseval <= (f_cl_vt->baseval+f_cl_vt->diffval)) ) {
		fi->XTAclassSet = add2ClassSet(fi->XTAclassSet,c->classType);
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
	if (fi->XTAclassSet != NULL)
		c1  = fi->XTAclassSet->head;

			if (XTAfld >=1 ) {
				printf("fld XTA class set =");fflush(stdout);
				printClassSet(fi->XTAclassSet);
				printf("\t\tField class type = ");fflush(stdout);
				utf_display(fi->fldClassType->name); printf("\n");
				}
	}

/*--- GETSTATIC specific ---*/
/* Sm = union of Sm and Sx */
for (c=c1; c != NULL; c=c->nextClass) {
	bool addFlg = false;
	if (rt_method->XTAclassSet ==NULL) 
		addFlg = true;
	else 	{
		if (!(inSet (rt_method->XTAclassSet->head, c->classType) )) 
			addFlg = true;
		}
	if (addFlg) {
		rt_method->XTAclassSet 
			= add2ClassSet(rt_method->XTAclassSet,c->classType);
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
			printf("calledBy method set: "); fflush(stdout);
			printMethodSet(rt_method->calledBy); fflush(stdout);
			}
if (rt_method->calledBy == NULL)
	s1 = NULL;
else
	s1 = rt_method->calledBy->head;
for (SmCalled=s1; SmCalled != NULL; SmCalled = SmCalled->nextmethRef) {
		if (XTAdebug >= 1) {
			printf("SmCalled = "); fflush(stdout);
			utf_display(SmCalled->methRef->class->name); fflush(stdout);
			printf(".");fflush(stdout); method_display(SmCalled->methRef);
			}
				
	rt_method->chgdSinceLastParse = false;		
	xtaPassParams(rt_method, SmCalled->methRef,SmCalled);  	/* chg flag output ignored for 1st regular parse */
	}
}

/*-------------------------------------------------------------------------------*/
void xtaAllFldsUsed ( ){
	fldSetNode  *f;
	fldSetNode *f1=NULL; 
	bool chgd = false;

if (rt_method->fldsUsed == NULL) return;

/* for each field that this method uses */
f1 = rt_method->fldsUsed->head;

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
			printMethodSet(rt_method->calls);
			printf("AAAAAAAAAAAAAAFTER printMethSett(rt_method->calls)\n");fflush(stdout);
			}
xtaAllFldsUsed ( );

/* for each method that this method calls */
if (rt_method->calls == NULL)
	s1 = NULL;
else
	s1 = SmCalls=rt_method->calls->head;

for (SmCalls=s1; SmCalls != NULL; SmCalls = SmCalls->nextmethRef) {
	/*    pass param types  */
	bool chgd = false;
	chgd = xtaPassParams (SmCalls->methRef, rt_method, SmCalls);  
	/* if true chgd after its own parse */
	if (!(SmCalls->methRef->chgdSinceLastParse)) {
		SmCalls->methRef->chgdSinceLastParse = true;
		}
	}

/* for each calledBy method */
/*    send return type */
if (rt_method->calledBy == NULL)
	s1 = NULL;
else
	s1 = rt_method->calledBy->head;
for (SmCalled=s1; SmCalled != NULL; SmCalled = SmCalled->nextmethRef) {

		if (XTAdebug >= 1) {
			printf("\tSmCalled = ");fflush(stdout); utf_display(SmCalled->methRef->class->name);
			printf("."); method_display(SmCalled->methRef);
			}
				
	chgd = xtaPassReturnType(rt_method, SmCalled->methRef); 
	if (!(SmCalled->methRef->chgdSinceLastParse)) {
		SmCalled->methRef->chgdSinceLastParse = chgd;		
		}
	}
}


/*-------------------------------------------------------------------------------*/
static void parseRT()
{
	int  p;                     /* java instruction counter                   */
	int  nextp;                 /* start of next java instruction             */
	int  opcode;                /* java opcode                                */
	int  i;                     /* temporary for different uses (counters)    */
        bool iswide = false;        /* true if last instruction was a wide        */

		RTAPRINT01method

	if ( ((XTAOPTbypass) || (opt_xta)) && (rt_method->name != utf_MAIN)) {

		xtaPassAllCalledByParams (); 
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
                                if (iswide)
                                  {
                                  nextp = p+3;
                                  iswide = false;
                                  }
                                break;

                        case JAVA_ALOAD_0:
			/* ---->>>> */ 	if (useFieldOpcodes == false)  break;  /* <<<<<<******* */
			 printf("ALOAD0 %s i=%i\n", opcode_names[opcode],0);
	               	 class_showconstanti(rt_class, 0); 
                                break;

                        case JAVA_ALOAD_1:
			/* ---->>>> */ 	if (useFieldOpcodes == false)  break;  /* <<<<<<******* */
			 printf("ALOAD1 %s i=%i\n", opcode_names[opcode],1);
			 class_showconstanti(rt_class, 1); 
                                break;

                        case JAVA_ALOAD_2:
			/* ---->>>> */ 	if (useFieldOpcodes == false)  break;  /* <<<<<<******* */
			 printf("ALOAD2 %s i=%i\n", opcode_names[opcode],2);
			 class_showconstanti(rt_class, 2); 
                                break;

                        case JAVA_ALOAD_3:
			/* ---->>>> */ 	if (useFieldOpcodes == false)  break;  /* <<<<<<******* */
			 printf("ALOAD3 %s i=%i\n", opcode_names[opcode],3);
			 class_showconstanti(rt_class, 3); 
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
			/* ---->>>> */ 	if (useFieldOpcodes == false)  break;  /* <<<<<<******* */
			 printf("ALOADs %s i=%i <%x>\n", opcode_names[opcode],i,rt_jcode[p+1]);
			 class_showconstanti(rt_class, i); 
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


                        case JAVA_ASTORE_0:
			/* ---->>>> */ 	if (useFieldOpcodes == false)  break;  /* <<<<<<******* */
					printf("ASTOREs %s i=%i\n", opcode_names[opcode],0);
					class_showconstanti(rt_class,0);
I=0;
if (CONSTANT_Class == rt_class->cptags [I] ) {  
	printf("ASTORE CONSTANT #%i_Class found =",I); 
	utf_display(((classinfo*)rt_class->cpinfos [I])->name); 
	printf("\n"); 
	((classinfo*)rt_class->cpinfos [I])->classUsed = PARTUSED; 
	}

                                break;

                        case JAVA_ASTORE_1:
			/* ---->>>> */ 	if (useFieldOpcodes == false)  break;  /* <<<<<<******* */
					printf("ASTOREs %s i=%i\n", opcode_names[opcode],1);
					class_showconstanti(rt_class, 1);
I=1;
if (CONSTANT_Class == rt_class->cptags [I] ) {  
	printf("ASTORE CONSTANT #%i_Class found =",I); 
	utf_display(((classinfo*)rt_class->cpinfos [I])->name); 
	printf("\n"); 
	((classinfo*)rt_class->cpinfos [I])->classUsed = PARTUSED; 
	}

                                break;

                        case JAVA_ASTORE_2:
			/* ---->>>> */ 	if (useFieldOpcodes == false)  break;  /* <<<<<<******* */
					printf("ASTOREs %s i=%i\n", opcode_names[opcode],2);
					class_showconstanti(rt_class, 2);
I=2;
if (CONSTANT_Class == rt_class->cptags [I] ) {  
	printf("ASTORE CONSTANT #%i_Class found =",I); 
	utf_display(((classinfo*)rt_class->cpinfos [I])->name); 
	printf("\n"); 
	((classinfo*)rt_class->cpinfos [I])->classUsed = PARTUSED; 
	}

                                break;

                        case JAVA_ASTORE_3:
			/* ---->>>> */ 	if (useFieldOpcodes == false)  break;  /* <<<<<<******* */
					printf("ASTOREs %s i=%i\n", opcode_names[opcode],3);
					class_showconstanti(rt_class, 3);
I=3;
if (CONSTANT_Class == rt_class->cptags [I] ) {  
	printf("ASTORE CONSTANT #%i_Class found =",I); 
	utf_display(((classinfo*)rt_class->cpinfos [I])->name); 
	printf("\n"); 
	((classinfo*)rt_class->cpinfos [I])->classUsed = PARTUSED; 
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

			/* ---->>>> */ 	if (useFieldOpcodes == false)  break;  /* <<<<<<******* */
					printf("ASTOREs %s i=%i <%x>\n", opcode_names[opcode],i,rt_jcode[p+1]);
					class_showconstanti(rt_class, rt_jcode[p+1]);

	/* old			if (CONSTANT_Class == rt_class->cptags [rt_jcode[p+1]] ) { */
I=rt_jcode[p+1];
if (CONSTANT_Class == rt_class->cptags [I] ) {  
	printf("ASTORE CONSTANT #%i_Class found =",I); 
	utf_display(((classinfo*)rt_class->cpinfos [I])->name); 
	printf("\n"); 
	((classinfo*)rt_class->cpinfos [I])->classUsed = PARTUSED; 
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

                        /* managing arrays ************************************************/

                        case JAVA_ANEWARRAY:
			/* ---->>>> */ 	if (useArrayOpcodes == false)  break;  /* <<<<<<******* */
                                i = rt_code_get_u2(p+1);
                                {
                                constant_FMIref *ar;
                                voidptr e;
				classinfo *c;
                                /* array or class type ? */
                                if (class_constanttype (rt_class, i) != CONSTANT_Arraydescriptor) {
                                        e = class_getconstant(rt_class, i, CONSTANT_Class);
					c = (classinfo *)e;
					if (c->classUsed == NOTUSED)
						c->classUsed = PARTUSED;
/*COtest*/ printf("ANEWARRAY Mark class=");utf_display ( c-> name );printf("=>PARTUSED\n");
                                        }
                                }
                                break;

                        case JAVA_MULTIANEWARRAY:
			/* ---->>>> */ 	if (useArrayOpcodes == false)  break;  /* <<<<<<******* */
                                i = rt_code_get_u2(p+1);
                                {
                                constant_arraydescriptor *ar;
				int t;
                         arraydesc:       ar = class_getconstant(rt_class, i, CONSTANT_Arraydescriptor);
				/*ar = rt_class-> cpinfos [i];  */
				t = ar->arraytype;
				while (ARRAYTYPE_ARRAY== t) {
					ar = ar->elementdescriptor;
					t = ar->arraytype;
					}
				if (ARRAYTYPE_OBJECT == t) {
					printf("MULTINEWARRAY 1Marking class=");utf_display(ar->objectclass->name);printf("\n");
					if (ar->objectclass->classUsed == NOTUSED) {
     						ar->objectclass->classUsed == PARTUSED;
						}
  					}
                                }
                                break;


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
                                addClassInit(fr->class);

				/*--- XTA ---*/
				if   ((XTAOPTbypass) || (opt_xta))
				{
				if (xtaAddFldClassTypeInfo(fi)) {  
					rt_method->fldsUsed = add2FldSet(rt_method->fldsUsed, fi, true,false);
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
                                addClassInit(fr->class);

				/*--- XTA ---*/
				if  ((XTAOPTbypass) || (opt_xta) ) 
				{
				if (xtaAddFldClassTypeInfo(fi)) {
					rt_method->fldsUsed = add2FldSet(rt_method->fldsUsed, fi, false, true);
					}
				}

                                }
                                break;

                        case JAVA_PUTFIELD:
                        case JAVA_GETFIELD:
			/* ---->>>> */ 	if (useFieldOpcodes == false)  break;  /* <<<<<<******* */
                                i = rt_code_get_u2(p + 1);
                                {
                                constant_FMIref *fr;
                                fieldinfo *fi;
                                classinfo *ci;
                                methodinfo *mi;
				int m;

class_showconstanti(rt_class,i); fflush(stdout);
                                fr = class_getconstant (rt_class, i, CONSTANT_Fieldref);
                                fi = class_findfield (fr->class, fr->name, fr->descriptor);
				ci = fr->class;  /* class with the local field         */
						 /*   either current class or inherited (a super) */
						 /* descriptor has type of field ref'd */
					RTAPRINT03putstatic1
                                /*** OP2A(opcode, fi->type, fi); ***/
                                }
                                break;
 

                        /*--------------------  method invocation ---------------------*/

                        case JAVA_INVOKESTATIC:
                                i = rt_code_get_u2(p + 1);
                                {
                                constant_FMIref *mr;
                                methodinfo *mi;

                                mr = class_getconstant (rt_class, i, CONSTANT_Methodref);
                                mi = class_findmethod (mr->class, mr->name, mr->descriptor);
				/*-- RTA --*/
					RTAPRINT04invokestatic1
                                if (mi->class->classUsed == NOTUSED) {
                                    mi->class->classUsed = USED;
					RTAPRINT05invokestatic2
                                    }
				addClassInit(mi->class);
	
				ADDTOCALLGRAPH(mi)  
fflush(stdout);
				/*-- XTA --*/
				if ((XTAOPTbypass) || (opt_xta)) {
				xtaAddCallEdges(mi,MONO); 
				} /* end XTA */
                                }
                                break;

                        case JAVA_INVOKESPECIAL:
               			i = rt_code_get_u2(p + 1);
                                {
                                constant_FMIref *mr;
                                methodinfo *mi;
				classinfo  *ci;
				
               			mr = class_getconstant (rt_class, i, CONSTANT_Methodref);
                                mi = class_findmethod (mr->class, mr->name, mr->descriptor);
				ci = mi->class;
								RTAPRINT06invoke_spec_virt1
				/*--- PRIVATE Method -----------------------------------------------------*/ 
                               	if (mi->name        != INIT) {     /* if method called is PRIVATE */ 
								RTAPRINT07invoke_spec_virt2
								RTAPRINT04invokestatic1
					/*-- RTA --*/   /* was just markSubs(mi); */
				    	ADDTOCALLGRAPH(mi)  

					/*--- XTA ---*/
					if ((XTAOPTbypass) || (opt_xta)) {
					xtaAddCallEdges(mi,MONO);
					} /* end XTA */
					}

				else 	{
				/*--- Test for super <init> which is: <init> calling its super class <init> -*/

					/* new class so add marked methods */
					if (( mi->methodUsed != USED) || (mi->class->classUsed == PARTUSED))  {
				/*--- process NORMAL <init> method ---------------------------------------------*/
						if ( mi->methodUsed != USED) {
							/* Normal <init> 
								- mark class as USED and <init> to callgraph */
				
							/*-- RTA --*/
							ci->classUsed = USED;
							addMarkedMethods(ci);    /* add to callgraph marked methods */
									RTAPRINT06Binvoke_spec_init
							addUsedInterfaceMethods(ci); 
			    				ADDTOCALLGRAPH(mi)  

							/*-- XTA --*/
							if ((XTAOPTbypass) || (opt_xta)) { 
							rt_method->XTAclassSet = add2ClassSet(rt_method->XTAclassSet,ci ); 
							xtaAddCallEdges(mi,MONO);
										RTAPRINT06CXTAinvoke_spec_init1
							} /* end XTA */
							}
						}
					}

				}	                                     	 
                                break;


                        case JAVA_INVOKEVIRTUAL:
                		i = rt_code_get_u2(p + 1);
                                {
                                constant_FMIref *mr;
                                methodinfo *mi;
				
				mr = class_getconstant (rt_class, i, CONSTANT_Methodref);
                        	mi = class_findmethod (mr->class, mr->name, mr->descriptor);

				/*--- RTA ---*/
						RTAPRINT07invoke_spec_virt2
				mi->monoPoly = POLY;
				rtaMarkSubs(mi->class,mi); 

				/*--- XTA ---*/
				if ((XTAOPTbypass) || (opt_xta)) { 
				classSetNode *subtypesUsedSet = NULL;
				if (rt_method->XTAclassSet != NULL)
					subtypesUsedSet = intersectSubtypesWithSet(mi->class, rt_method->XTAclassSet->head);
						/*****	
						printf(" \nXTA subtypesUsedSet: "); fflush(stdout);
						printSet(subtypesUsedSet);
						*****/
				xtaMarkSubs(mi->class, mi, subtypesUsedSet);   
				} /* end XTA */
				}
                                break;

                        case JAVA_INVOKEINTERFACE:
                                i = rt_code_get_u2(p + 1);
                                {
                                constant_FMIref *mr;
                                methodinfo *mi;
				classSetNode *subs;
				
                                mr = class_getconstant (rt_class, i, CONSTANT_InterfaceMethodref);
                                mi = class_findmethod (mr->class, mr->name, mr->descriptor);

                                if (mi->flags & ACC_STATIC)
                                        panic ("Static/Nonstatic mismatch calling static method");

				/*--- RTA ---*/
						RTAPRINT08AinvokeInterface0
				if (mi->class->classUsed == NOTUSED) {
					mi->class->classUsed = USED; /*??PARTUSED;*/
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
					}

				/*--- XTA ---*/
				if ((XTAOPTbypass2) || (opt_xta))
				{
				xtaMarkInterfaceSubs(mi);
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
if (pWhenMarked >= 1) {
	printf("\tclass=");fflush(stdout);
	utf_display(ci->name); fflush(stdout);
	printf("=\n");fflush(stdout);
	}
				/*--- RTA ---*/
				if (ci->classUsed != USED) {
                                	int ii;
						RTAPRINT10new
                                        ci->classUsed = USED;    /* add to heirarchy    */
					/* Add this class to the implemented by list of the abstract interface */
					addUsedInterfaceMethods(ci);
				    	addClassInit(ci);
                                    	} 
				/*--- XTA ---*/
				if ((XTAOPTbypass) || (opt_xta))
				{
				rt_method->XTAclassSet = add2ClassSet(rt_method->XTAclassSet,ci ); /*XTA*/
						RTAPRINT10newXTA
				}
                                }
                                break;
	/* ---- Reference Opcodes --------------------------------------------*/

		      case JAVA_CHECKCAST:
			/* ---->>>> */ 	if (useObjectrefOpcodes == false)  break;  /* <<<<<<******* */

                                i = rt_code_get_u2(p+1);
				{
				classinfo *ci;

                                /* array type cast-check */
                                if (class_constanttype (rt_class, i) == CONSTANT_Arraydescriptor) {
class_showconstanti(rt_class,i);
goto arraydesc;   /* better to make a fn later ?? */
panic("arraydescriptor in checkcast - panic so find it");
					/***
                                        LOADCONST_A(class_getconstant(rt_class, i, CONSTANT_Arraydescriptor));  
                                        s_count++;	
                                        BUILTIN2((functionptr) asm_builtin_checkarraycast, TYPE_ADR); 
					****/
                                        }
                                else { /* object type cast-check */
					ci = class_getconstant(rt_class, i, CONSTANT_Class);
					ci->classUsed =PARTUSED;
/*RTtest p1*/ if (pWhenMarked >= 2) {
/*RTtest*/   printf("checkcast class=");utf_display(ci->name);printf(" marked PARTUSED\n");
/*RTtest*/ }
                                        /*****
                                        LOADCONST_A(class_getconstant(irt_class, i, CONSTANT_Class));
                                        s_count++;
                                        BUILTIN2((functionptr) asm_builtin_checkcast, TYPE_ADR);
                                        OP2A(opcode, 1, (class_getconstant(rt_class, i, CONSTANT_Class)));
                                        ****/
                                        }
				}
                                break;

                        case JAVA_INSTANCEOF:
			/* ---->>>> */ 	if (useObjectrefOpcodes == false)  break;  /* <<<<<<******* */
                                i = rt_code_get_u2(p+1);
				{
				classinfo *ci;

                                /* array type cast-check */
                                if (class_constanttype (rt_class, i) == CONSTANT_Arraydescriptor) {
class_showconstanti(rt_class,i);
goto arraydesc;   /* better to make a fn later */
panic("arraydescriptor in instanceof- panic so find it");
					/***
                                        LOADCONST_A(class_getconstant(rt_class, i, CONSTANT_Arraydescriptor));
					***/
                                        }
                                else { /* object type cast-check */
					ci = class_getconstant(rt_class, i, CONSTANT_Class);
					ci->classUsed =PARTUSED;
/*RTtest p1*/ if (pWhenMarked >= 2) {
/*RTtest*/   printf("checkcast class=");utf_display(ci->name);printf(" marked PARTUSED\n");
/*RTtest*/ }
                                        }
				}
                                break;

                        case JAVA_MONITORENTER:
			/* ---->>>> */ 	if (useObjectrefOpcodes == false)  break;  /* <<<<<<******* */
/* comes from stack - how put on stack???? */
#ifdef USE_THREADS
                                if (checksync) {
#ifdef SOFTNULLPTRCHECK
                                        if (checknull) {
						/****
                                                BUILTIN1((functionptr) asm_builtin_monitorenter, TYPE_VOID);
						***/
                                                }
                                        else {
						/****
                                                BUILTIN1((functionptr) builtin_monitorenter, TYPE_VOID);
                                                BUILTIN1((functionptr) asm_builtin_monitorenter, TYPE_VOID);
						***/
                                                }
#else
                                        /***
					BUILTIN1((functionptr) builtin_monitorenter, TYPE_VOID);
					***/
#endif
                                        }
                                else
#endif
                                        {
                                        /***
                                        OP(ICMD_NULLCHECKPOP);
					***/
                                        }
                                break;

                        case JAVA_MONITOREXIT:
			/* ---->>>> */ 	if (useObjectrefOpcodes == false)  break;  /* <<<<<<******* */
/* comes from stack - how put on stack???? */
#ifdef USE_THREADS
                                if (checksync) {
                                        /***
                                        BUILTIN1((functionptr) builtin_monitorexit, TYPE_VOID);
                                        ***/
                                        }
                                else
#endif
                                        {
                                        /***
                                        OP(ICMD_POP);
                                        ***/
                                        }
                                break;

                        case JAVA_ARETURN:
			/* ---->>>> */ 	if (useOtherOpcodes == false)  break;  /* <<<<<<******* */
/* set a variable with type from input and track  it to here */
                                /***
                                blockend = true;
                                OP(opcode);
                                ***/
                                break;

                        case JAVA_ATHROW:
			/* ---->>>> */ 	if (useOtherOpcodes == false)  break;  /* <<<<<<******* */
                                /***
                                blockend = true;
                                OP(opcode);
                                ***/
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
	class->classUsed = USED; /* MARK CLASS USED */
	/* add marked methods to callgraph */ 
	addMarkedMethods(class);
	}

meth = class_findmethod (class, m1, d1);
if (meth == NULL) {
	utf_display(class->name);printf(".");utf_display(m1);printf(" ");utf_display(d1);
	printf("WARNING from parseRT:  Method given is used by Native method call, but NOT FOUND\n");
	}
else
	rtaMarkSubs(class,meth);
}

/*-------------------------------------------------------------------------------*/

void   findMarkNativeUsedClass (utf * c) {
classinfo  *class;
int ii;

class = class_get(c);
if (class == NULL)  panic("parseRT: Class used by Native method called not loaded!!!");
class->classUsed = USED;

/* add marked methods to callgraph */
addMarkedMethods(class);
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
/*-------------------------------------------------------------------------------*/
void mainRTAparseInit (methodinfo *m )
{
/*printf("MAIN_NOT_STARTED \n");*/ 
if (class_java_lang_Object->sub != NULL) { 
	RTAPRINT16stats1
	}

if (firstCall) {
	firstCall=false;

	utf_MAIN  = utf_new_char("main");
	INIT      = utf_new_char("<init>");
	CLINIT    = utf_new_char("<clinit>");
	FINALIZE  = utf_new_char("finalize");
	EMPTY_DESC= utf_new_char("()V");

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

	/* At moment start RTA before main when parsed                      */
	/* Will definitely use flag with to know if ok to apply in-lining.  */
}


/*-------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------*/
/* still need to look at field sets in 2nd pass and clinit .....  */
void XTA_jit_parse2(methodinfo *m)
{
			if (XTAdebug >= 1) 
				printf("\n\nStarting Round 2 XTA !!!!!!!!!!!!!!\n");

/* for each method in XTA worklist = callgraph (use RTA for now) */
methRT=0;
while (methRT <= methRTlast) {
	rt_method      = callgraph[methRT];
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
		if (rt_method->chgdSinceLastParse) {

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

void RT_jit_parse(methodinfo *m)
{
	/*-- RTA --*/
	if (m->methodUsed == USED) return;
	mainRTAparseInit (m);
		
	/* initialise parameter type descriptor */
        callgraph[++methRTlast] = m;          /*-- RTA --*/
	m->methodUsed = USED;
			RTAPRINT11addedtoCallgraph 

	/* <init> then like a new class so add marked methods to callgraph */
	if (m->name == INIT)  {  /* need for <init>s parsed efore Main */
  	  classinfo *ci;
	  int ii;
		ci = m->class;
		ci->classUsed = USED;
		if (pWhenMarked >= 1) {
			printf("Class=");utf_display(ci->name);
			}
		/* add marked methods to callgraph */
			RTAPRINT11addedtoCallgraph2
		addMarkedMethods(ci);
	  } /* if */

	/*-- XTA --*/
   	if ((XTAOPTbypass) || (opt_xta)) {
                XTAcallgraph[++methXTAlast] = m;
                m->XTAmethodUsed = USED;
			{methodinfo *mi = m;
			XTAPRINTcallgraph2
			}
	}

	/*-- Call graph work list loop -----------------*/

        while (methRT <= methRTlast) {
            rt_method      = callgraph[methRT];
	    rt_class       = rt_method->class;
	    rt_descriptor  = rt_method->descriptor;
	    rt_jcodelength = rt_method->jcodelength;
	    rt_jcode       = rt_method->jcode;

            if (! (  (rt_method->flags & ACC_NATIVE  )
		||   (rt_method->flags & ACC_ABSTRACT) ) ) {
	      parseRT();
		}
	    else {
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
			printXTACallgraph ();
			RTAPRINT14CallgraphLast  /*  was >=2*/
			RTAPRINT15HeirarchyiLast /*was >= 2 */

   	if ((XTAOPTbypass) || (opt_xta)) {
		/*--- XTA round 2+ "parse" - use info structures only so not a real parse */
		XTA_jit_parse2(m);
		}

return;
}
