/* jit/inline.c - code inliner

globals moved to structure and passed as parameter

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

   Authors: Dieter Thuernbeck

   $Id: inline.c 1959 2005-02-19 11:46:27Z carolyn $

*/

/*---
Inlining initializes an inline structure with values of the method called
with. Then it recursively to MAXDEPTH analyzes by parsing the bytecode 
looking for methods that meet the critera to be inlined.

Critera for inlining currently is:
Method to be inlined must:
- be less than MAXCODESIZE 
- only MAXMETHODS can be inlined in 1 method

-in only STATIC, FINAL, PRIVATE methods can be inlined from method's class
-ino (include outsiders) all STATIC, FINAL, PRIVATE methods can be inlined
	note: PRIVATE is always only in the same class
-inv include virtual methods which static analysis (currently only RTA)
     to only have 1 definition used (INVOKEVIRTUAL/INVOKEINTERFACE)
     Currently dynamic loading is handled by rerunning with info 
	(see parseRT). Guards need to be added.
-inp  inline parameters - Parameters are analysed if they are
	readonly, which is used during parsing to generate ICMD_CLEAR_ARGREN
        and ICMD_CLEAR_ARGREN is in turn used during stack analysis to
        replace the ISTORE with a NOP so the same local variable is used.
	Parameters are pushed on the stack, same as normal method 
	invocation when popped the local variable of calling program is used.
-ine  JOWENN <- please add
---*/

#include <stdio.h>
#include <string.h>

#include "mm/memory.h"
#include "toolbox/logging.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/tables.h"
#include "vm/options.h"
#include "vm/jit/jit.h"
#include "vm/jit/parse.h"
#include "vm/jit/inline/inline.h"

#undef  INVIRTDEBUG  /* prints if a method was found to be 
			 a unique virt/interface method  definition */
#undef DEBUGi 

#define METHINFOj(mm) \
    { \
        printf("<j%i/l%i/s%i/(p)%i>\t", \
                (mm)->jcodelength,(mm)->maxlocals, \
		(mm)->maxstack, (mm)->paramcount);  \
        method_display_w_class(mm); }

#define METHINFOx(mm) \
    { \
        printf("<c%i/m%i/p%i>\t", \
                (mm)->class->classUsed,(mm)->methodUsed, (mm)->monoPoly); \
        method_display_w_class(mm); }

#define METHINFO(m) \
  method_display_w_class(m); 

#define IMETHINFO(m) \
  utf_display(m->class->name); printf("."); fflush(stdout); \
  method_display(m); fflush(stdout); \
  printf("\tm->jcodelength=%i; ",m->jcodelength); fflush(stdout); \
  printf("m->jcode=%p;\n",m->jcode); fflush(stdout); \
  printf("\tm->maxlocals=%i; ",m->maxlocals); fflush(stdout); \
  printf("m->maxstack=%i;\n",m->maxstack); fflush(stdout);

/* checked functions and macros: LOADCONST code_get OP1 BUILTIN block_insert bound_check ALIGN */

/* replace jcodelength loops with correct number after main for loop in parse()! */

#define CLASSINFO(cls) \
        {       printf("<c%i>\t",cls->classUsed); \
                utf_display(cls->name); printf("\n");fflush(stdout);}

/*-----------------------------------------------------------*/
/* just initialize global structure for non-inlining         */
/*-----------------------------------------------------------*/

void inlining_init0(methodinfo *m, t_inlining_globals *inline_env)
{
	/* initialization for normal use in parse */
	inlining_set_compiler_variables_fun(m, inline_env);
	inline_env->isinlinedmethod = 0;
	inline_env->cumjcodelength = m->jcodelength; /* for not inlining */

	inline_env->cummaxstack = m->maxstack; 
	inline_env->cumextablelength = 0;
	inline_env->cumlocals = m->maxlocals;
	inline_env->cummethods = 0; /* co not global or static-used only here? */
	inline_env->inlining_stack = NULL;
	inline_env->inlining_rootinfo = NULL;
}


/*-----------------------------------------------------------*/

void inlining_setup(methodinfo *m, t_inlining_globals *inline_env)
{
/*    	t_inlining_globals *inline_env = DNEW(t_inlining_globals); */
        inlining_init0(m,inline_env);

/* define in options.h; Used in main.c, jit.c & inline.c */
#ifdef INAFTERMAIN
if ((utf_new_char("main") == m->name) && (useinliningm)) {
  	useinlining = true;
	}
#endif

if (useinlining)
        {
		#ifdef DEBUGi
 		printf("\n-------- Inlining init for: "); fflush(stdout);
  		IMETHINFO(m)
		#endif
		
        inline_env->cumjcodelength = 0;
        inline_env->inlining_stack = NEW(list);
        list_init(inline_env->inlining_stack, 
		  OFFSET(t_inlining_stacknode, linkage));
        /*------ analyze ------*/
        inline_env->inlining_rootinfo 
		= inlining_analyse_method(m, 0, 0, 0, 0, inline_env);
		#ifdef DEBUGi
	  		printf ("\n------------------------------ ");fflush(stdout);
	  		printf ("\nComplete Result of inlining analysis of:");
			fflush(stdout);
			METHINFOj(m)
			print_t_inlining_globals(inline_env); /* init ok */
		#endif
        /*---------------------*/
/*
 if (inline_env->cummethods == 0) {
	 inline_env = DNEW(t_inlining_globals);
	 inlining_init0(m,inline_env);
	 return inline_env;
 }
*/
#if 0
		#ifdef DEBUGi
  printf("(l,s) (%i,%i) was (%i,%i)\n",
    m->maxlocals, inline_env->cumlocals,
    m->maxstack,  inline_env->cummaxstack); fflush(stdout);
		#endif
/*This looks wrong*/
/* OK since other changes were also made, but in parse same stmt still */
        m->maxlocals = inline_env->cumlocals;   orig not used
        m->maxstack = inline_env->cummaxstack;  orig global maxstack var!!
#endif
        }
}


void inlining_cleanup(t_inlining_globals *inline_env)
{
	FREE(inline_env->inlining_stack, t_inlining_stacknode);
}


/*--2 push the compile variables to save the method's environment --*/

void inlining_push_compiler_variablesT(int opcode, inlining_methodinfo *inlinfo, t_inlining_globals *inline_env)
{
	t_inlining_stacknode *new = NEW(t_inlining_stacknode);

	/**new->opcode = opcode; **/
	new->method = inline_env->method;
	new->inlinfo = inlinfo;
	list_addfirst(inline_env->inlining_stack, new);
	inline_env->isinlinedmethod++;
}
/*-- push the compile variables to save the method's environment --*/

void inlining_push_compiler_variables(int i, int p, int nextp, int opcode,  u2 lineindex,u2 currentline,u2 linepcchange,inlining_methodinfo *inlinfo, t_inlining_globals *inline_env)
{
	t_inlining_stacknode *new = NEW(t_inlining_stacknode);

	new->i = i;
	new->p = p;
	new->nextp = nextp;
	new->opcode = opcode;
	new->method = inline_env->method;
	new->lineindex=lineindex;
	new->currentline=currentline;
	new->linepcchange=linepcchange;
	new->inlinfo = inlinfo;
	list_addfirst(inline_env->inlining_stack, new);
	inline_env->isinlinedmethod++;
}


void inlining_pop_compiler_variables(
                                    int *i, int *p, int *nextp,
				    int *opcode, u2 *lineindex,
				    u2 *currentline,u2 *linepcchange,
                                    inlining_methodinfo **inlinfo,
                                    t_inlining_globals *inline_env)
{
	t_inlining_stacknode *tmp 
	  = (t_inlining_stacknode *) list_first(inline_env->inlining_stack);

	if (!inline_env->isinlinedmethod) panic("Attempting to pop from inlining stack in toplevel method!\n");

	*i = tmp->i;
	*p = tmp->p;
	*nextp = tmp->nextp;
	*opcode = tmp->opcode;

	*lineindex=tmp->lineindex;
	*currentline=tmp->currentline;
	*currentline=tmp->linepcchange;

	*inlinfo = tmp->inlinfo;

        inline_env->method = tmp->method; /*co*/
        inline_env->class = inline_env->method->class; /*co*/
        inline_env->jcodelength = inline_env->method->jcodelength; /*co*/
        inline_env->jcode = inline_env->method->jcode; /*co*/

        list_remove(inline_env->inlining_stack, tmp);
        FREE(tmp, t_inlining_stacknode);
        inline_env->isinlinedmethod--;
}


void inlining_set_compiler_variables_fun(methodinfo *m,
					 t_inlining_globals *inline_env)
{
        inline_env->method = m; 
        inline_env->class  = m->class; 
        inline_env->jcode  = m->jcode; 
        inline_env->jcodelength = m->jcodelength; 
}

/* is_unique_method2 - determines if m is a unique method by looking
	in subclasses of class that define method m 
	It counts as it goes. It also saves the method name if found.

 returns count of # methods used up to 2
                        	(since if 2 used then not unique.)
		       sets mout to method found 
				(unique in class' heirarchy)
                       It looks for subclasses with method def'd.
 Input:
 * class - where looking for method
 * m     - original method ptr
 * mout  - unique method (output) 
 Output: "cnt" of methods found up to max of 2 (then not unique)
*/

int is_unique_method2(classinfo *class, methodinfo *m, methodinfo **mout)
{
utf* name = m->name;
utf* desc = m->descriptor;

int cnt = 0;  /* number of times method found in USED classes in hierarchy*/
classinfo *subs1; 	   

if ((m->class == class) && (class->classUsed == USED)) {
	/* found method in current class, which is used */
	if (*mout != m) {
        	cnt++;
  	 	*mout = m;
		}
	}

if ( ((m->flags & ACC_FINAL)  
||    (class->sub == NULL))
&& (class->classUsed == USED)) {
  	/* if final search no further */
	if (*mout != m) {
        	cnt++;
  	 	*mout = m;
		}
 	 return cnt;
	}

/* search for the method in its subclasses */
for (subs1 = class->sub;subs1 != NULL;subs1 = subs1->nextsub) {
        methodinfo * sm;
	classinfo *subs = subs1; 	   
        sm = class_resolveclassmethod(subs,name,desc,class,false);
        if (sm != NULL) {
		if ((subs->classUsed == USED) && 
		    (*mout != sm)) {
  	 		*mout = sm;
			cnt++;
			}
                cnt = cnt + is_unique_method2(subs, sm, mout);
		/* Not unique if more than 1 def of method in class heir */
                if (cnt > 1)
                        {return cnt;}
                }

        }
return cnt;
}

/*-----------------------------------------------------------*/

bool is_unique_interface_method (methodinfo *mi, methodinfo **mout) {

utf* name = mi->name;
utf* desc = mi->descriptor;
	
	classSetNode *classImplNode;
	int icnt = 0;

	for (classImplNode  = mi->class->impldBy;
	     classImplNode != NULL; 			
	     classImplNode  = classImplNode->nextClass) {

		classinfo * classImplements = classImplNode->classType;
		methodinfo *submeth;

		submeth = class_findmethod(classImplements,name, desc); 
		if (submeth != NULL) {
			icnt =+ is_unique_method2(
				    classImplements,
				    submeth,
			  	    mout);
			}	
		if (icnt > 1) return false;
		} /* end for*/
if (icnt == 1) return true;
else return false;
} 

/*-----------------------------------------------------------*/

inlining_methodinfo *inlining_analyse_method(methodinfo *m, 
					  int level, int gp, 
					  int firstlocal, int maxstackdepth,
					  t_inlining_globals *inline_env)
{
	inlining_methodinfo *newnode = DNEW(inlining_methodinfo);
	/*u1 *jcode = m->jcode;*/
	int jcodelength = m->jcodelength;
	int p;
	int nextp;
	int opcode;
	int i=0;
	bool iswide = false, oldiswide;
	bool *readonly = NULL;
	int  *label_index = NULL;
	bool isnotrootlevel = (level > 0);
	bool isnotleaflevel = (level < INLINING_MAXDEPTH);
	#ifdef DEBUGi
	  	printf ("\n------------------------------ ");fflush(stdout);
	  	printf ("\nStart of inlining analysis of: ");fflush(stdout);
		METHINFOj(m)
		if (isnotrootlevel) printf(" isnotrootlevel=T ");
		else printf(" isnotrootlevel=F ");
		print_t_inlining_globals(inline_env); /* init ok */
	#endif
	#undef DEBUGi

	/* if (level == 0) gp = 0; */

	if (isnotrootlevel) {
		newnode->readonly = readonly = DMNEW(bool, m->maxlocals); /* FIXME only paramcount entrys necessary - ok FIXED also turned on*/

		/** for (i = 0; i < m->maxlocals; readonly[i++] = true); **/
		for (i = 0; i < m->paramcount; readonly[i++] = true);
		/***isnotrootlevel = true; This had turned -inp off **/

	} else {
		readonly = NULL;
	}
	
	label_index = DMNEW(int, jcodelength+200);

	newnode->inlinedmethods = DNEW(list);
	list_init(newnode->inlinedmethods, OFFSET(inlining_methodinfo, linkage));

	newnode->method = m;
	newnode->level = level;
	newnode->startgp = gp;
	newnode->readonly = readonly;
	newnode->label_index = label_index;
	newnode->firstlocal = firstlocal;
	inline_env->cumjcodelength += jcodelength + m->paramcount + 1 + 5;

	if ((firstlocal + m->maxlocals) > inline_env->cumlocals) {
		inline_env->cumlocals = firstlocal + m->maxlocals;
	}

	if ((maxstackdepth + m->maxstack) > inline_env->cummaxstack) {
		inline_env->cummaxstack = maxstackdepth + m->maxstack;
	}

	inline_env->cumextablelength += m->exceptiontablelength;
   

	for (p = 0; p < jcodelength; gp += (nextp - p), p = nextp) {
		opcode = code_get_u1 (p,m);
		nextp = p + jcommandsize[opcode];
		oldiswide = iswide;

		/* figure out nextp */

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

		case JAVA_RET:
			if (iswide) {
				nextp = p + 3;
				iswide = false;
			}
			break;

		case JAVA_IINC:
			if (iswide) {
				nextp = p + 5;
				iswide = false;
			}
			break;

		case JAVA_WIDE:
			iswide = true;
			nextp = p + 1;
			break;

		case JAVA_LOOKUPSWITCH:
			nextp = ALIGN((p + 1), 4) + 4;
			nextp += code_get_u4(nextp,m) * 8 + 4;
			break;

		case JAVA_TABLESWITCH:
			nextp = ALIGN((p + 1), 4) + 4;
			nextp += (code_get_u4(nextp+4,m) - code_get_u4(nextp,m) + 1) * 4 + 4 +4;
			break;
		}

		/* detect readonly variables in inlined methods */
		
		if (isnotrootlevel) { 
			bool iswide = oldiswide;
			
			switch (opcode) {
			case JAVA_ISTORE:
			case JAVA_LSTORE:
			case JAVA_FSTORE:
			case JAVA_DSTORE:
			case JAVA_ASTORE: 
				if (!iswide) {
					i = code_get_u1(p + 1,m);

				} else {
					i = code_get_u2(p + 1,m);
				}
				readonly[i] = false;
				break;

			case JAVA_ISTORE_0:
			case JAVA_LSTORE_0:
			case JAVA_FSTORE_0:
			case JAVA_ASTORE_0:
				readonly[0] = false;
				break;

			case JAVA_ISTORE_1:
			case JAVA_LSTORE_1:
			case JAVA_FSTORE_1:
			case JAVA_ASTORE_1:
				readonly[1] = false;
				break;

			case JAVA_ISTORE_2:
			case JAVA_LSTORE_2:
			case JAVA_FSTORE_2:
			case JAVA_ASTORE_2:
				readonly[2] = false;
				break;

			case JAVA_ISTORE_3:
			case JAVA_LSTORE_3:
			case JAVA_FSTORE_3:
			case JAVA_ASTORE_3:
				readonly[3] = false;
				break;

			case JAVA_IINC:
				if (!iswide) {
					i = code_get_u1(p + 1,m);

				} else {
					i = code_get_u2(p + 1,m);
				}
				readonly[i] = false;
				break;
			}
		}

		/*		for (i=lastlabel; i<=p; i++) label_index[i] = gp; 
				printf("lastlabel=%d p=%d gp=%d\n",lastlabel, p, gp);
		lastlabel = p+1; */
		for (i = p; i < nextp; i++) label_index[i] = gp;

		if (isnotleaflevel) { 

			switch (opcode) {

			case JAVA_INVOKEINTERFACE:
			case JAVA_INVOKEVIRTUAL:
				if (!inlinevirtuals) 
					break;
			
			case JAVA_INVOKESPECIAL:
			case JAVA_INVOKESTATIC:
				i = code_get_u2(p + 1,m);
				{
					constant_FMIref *imr;
					methodinfo *imi;

                                        methodinfo *mout;
                                        bool uniqueVirt= false;


					if (opcode ==JAVA_INVOKEINTERFACE) {
					    imr = class_getconstant(m->class, i, CONSTANT_InterfaceMethodref);
					    LAZYLOADING(imr->class)
                                            imi = class_resolveinterfacemethod(
						imr->class,
                                                imr->name,
                                                imr->descriptor,
                                                m->class,
                       				true);
					    if (!imi)  /* extra for debug */
						panic("ExceptionI thrown while parsing bytecode"); /* XXX should be passed on */
					   }
					else {
					   imr = class_getconstant(m->class, i, CONSTANT_Methodref);
					   LAZYLOADING(imr->class)
					   imi = class_resolveclassmethod(
						imr->class,
						imr->name,
						imr->descriptor,
						m->class,
						true);
					    if (!imi) /* extra for debug */
						panic("Exception0 thrown while parsing bytecode"); /* XXX should be passed on */
					    }

					if (!imi) /* normal-but never get here now */ 
						panic("Exception thrown while parsing bytecode"); /* XXX should be passed on */

/** Inlining has problem currently with typecheck & inlining **/
/** Due problem with typecheck & inlining, class checks fail for <init>s **/
					if (utf_new_char("<init>") == imi->name) break; 
/****/					
					if (opcode == JAVA_INVOKEVIRTUAL) {
						mout = NULL;
						/* unique virt meth? then */
						/*  allow it to be inlined*/
						if (is_unique_method2(
							 imi->class,
							 imi, 
							 &mout) == 1) {
                                                  if (mout != NULL) {
                                                        imi = mout;
                                                        uniqueVirt=true; 
#ifdef INVIRTDEBUG
							METHINFOx(imi);
                                                        printf("WAS unique virtual(-iv)\n");fflush(stdout);
#endif
                                                        }
						  } /* end is unique */
						} /* end INVOKEVIRTUAL */	
					if (opcode == JAVA_INVOKEINTERFACE){
						mout = NULL; 
#ifdef INVIRTDEBUG
						METHINFOx(imi);
#endif
						if (is_unique_interface_method (
							 imi, 
							 &mout)) {
                                                     if (mout != NULL) {
                                                        imi = mout;
                                                        uniqueVirt=true;
#ifdef INVIRTDEBUG
							  METHINFOx(imi);
                                                          printf("WAS unique interface(-iv)\n");fflush(stdout);
#endif
                                                        }
							
						     } 
						} /* end INVOKEINTERFACE */	

					if ((inline_env->cummethods < INLINING_MAXMETHODS) &&
						(!(imi->flags & ACC_NATIVE)) &&  
						(inlineoutsiders || (m->class == imr->class)) && 
						(imi->jcodelength < INLINING_MAXCODESIZE) && 
						(imi->jcodelength > 0) && 
					       (((!inlinevirtuals)  || 
						 (uniqueVirt     ))   || 
						((opcode != JAVA_INVOKEVIRTUAL) || 
					 	 (opcode != JAVA_INVOKEINTERFACE)) ) &&
						(inlineexceptions || (imi->exceptiontablelength == 0))) { /* FIXME: eliminate empty methods? */
						inlining_methodinfo *tmp;
						descriptor2types(imi);

						inline_env->cummethods++;

						if (opt_verbose) {
							char logtext[MAXLOGTEXT];
							sprintf(logtext, "Going to inline: ");
							utf_sprint(logtext  +strlen(logtext), imi->class->name);
							strcpy(logtext + strlen(logtext), ".");
							utf_sprint(logtext + strlen(logtext), imi->name);
							utf_sprint(logtext + strlen(logtext), imi->descriptor);
							log_text(logtext);
							if ( (!(opcode == JAVA_INVOKEVIRTUAL)) &&
							     (! ( (imi->flags & ACC_STATIC )
                                     			     ||   ((imi->flags & ACC_PRIVATE) && (imi->class == inline_env->class))
                                     			     ||   (imi->flags & ACC_FINAL  ))) )
							   {
							   printf("DEBUG WARNING:PROBABLE INLINE PROBLEM flags not static, private or final for non-virtual inlined method\n"); fflush(stdout);
							   METHINFOx(imi);
							   log_text("PROBABLE INLINE PROBLEM flags not static, private or final for non-virtual inlined method\n See method info after DEBUG WARNING\n");
							   }

						}
						
						tmp =inlining_analyse_method(imi, level + 1, gp, firstlocal + m->maxlocals, maxstackdepth + m->maxstack, inline_env);
						list_addlast(newnode->inlinedmethods, tmp);
		#ifdef DEBUGi
printf("New node Right after an inline by:");fflush(stdout);
METHINFOj(m)
print_inlining_methodinfo(newnode);
printf("end new node\n"); fflush(stdout);
		#endif
						gp = tmp->stopgp;
						p = nextp;
					}
				}
				break;
			}
		}  
	} /* for */
	
	newnode->stopgp = gp;
        label_index[jcodelength]=gp;
    return newnode;
}

/* --------------------------------------------------------------------*/
/*  print_ functions: check inline structures contain what is expected */
/* --------------------------------------------------------------------*/
void print_t_inlining_globals (t_inlining_globals *g) 
{
printf("\n------------\nt_inlining_globals struct for: \n\t");fflush(stdout); 
METHINFOj(g->method);
printf("\tclass=");fflush(stdout);
  utf_display(g->class->name);printf("\n");fflush(stdout);

printf("\tjcodelength=%i; jcode=%p;\n",g->jcodelength, g->jcode);

if (g->isinlinedmethod==true) {
  printf("\tisinlinedmethod=true ");fflush(stdout);  
  }
else {
  printf("\tisinlinedmethod=false");fflush(stdout);  
  }

printf("\tcumjcodelength=%i ,cummaxstack=%i ,cumextablelength=%i ",
 g->cumjcodelength,    g->cummaxstack,  g->cumextablelength);fflush(stdout);
printf("\tcumlocals=%i ,cummethods=%i \n",
 g->cumlocals,    g->cummethods);fflush(stdout);  

printf("s>s>s> ");fflush(stdout);
	print_inlining_stack     (g->inlining_stack);
printf("i>i>i> "); fflush(stdout);
	print_inlining_methodinfo(g->inlining_rootinfo);
printf("-------------------\n");fflush(stdout);
}

/* --------------------------------------------------------------------*/
void print_inlining_stack     ( list                *s)
{
  t_inlining_stacknode *is;

if (s==NULL) { 
  printf("\n\tinlining_stack: NULL\n");
  return;
  }

/* print first  element to see if get into stack */
printf("\n\tinlining_stack: NOT NULL\n");

is=list_first(s); 
if (is==NULL) { 
    printf("\n\tinlining_stack = init'd but EMPTY\n");
    fflush(stdout);
    return;
    }

printf("\n\tinlining_stack: NOT NULL\n");

for (is=list_first(s); 
       is!=NULL;
       is=list_next(s,is)) {
 	 printf("\n\ti>--->inlining_stack entry: \n"); fflush(stdout);
	 METHINFOx(is->method);
	 printf("i=%i, p=%i, nextp=%i, opcode=%i;\n",
		is->i,is->p,is->nextp,is->opcode);fflush(stdout);
	 print_inlining_methodinfo(is->inlinfo);
    } /*end for */
}

/* --------------------------------------------------------------------*/
void print_inlining_methodinfo( inlining_methodinfo *r) {
  int i=0;
  int cnt,cnt2;
  inlining_methodinfo *im;
  inlining_methodinfo *im2;
  bool labellong = false;

if (r==NULL) { 
  printf("\n\tinlining_methodinfo: NULL\n");
  return;
  }
printf("\n\tinlining_methodinfo for:"); fflush(stdout);

if (r->method != NULL) {
  utf_display(r->method->class->name); printf("."); fflush(stdout); \
  method_display(r->method); fflush(stdout); \
  }
else {
  printf(" NULL!!!!!\n");fflush(stdout);
  }

if (r->readonly==NULL) {
  printf("\treadonly==NULL ");fflush(stdout);  
  }
else {
  printf("\treadonly=");fflush(stdout);  
  for (i = 0; i < r->method->maxlocals; i++)  {
    if (r->readonly[i] == true)
      printf("[i]=T;");
    else
      printf("[i]=F;");
    fflush(stdout);
    } 
  }

/**printf("\tstartgp=%i; stopgp=%i; firstlocal=%i; label_index=%p;\n", **/
printf("\tstartgp=%i; stopgp=%i; firstlocal=%i; label_index=%p;\n",
          r->startgp, r->stopgp, r->firstlocal, (void *)r->label_index);
printf ("label_index[0..%d]->", r->method->jcodelength);
if (labellong) {
for (i=0; i<r->method->jcodelength; i++) printf ("%d:%d ", i, r->label_index[i]); }
else {
printf ("%d:%d ", 0, r->label_index[i]);
printf ("%d:%d ", 
  (r->method->jcodelength-1), r->label_index[r->method->jcodelength-1]);
}

printf("\n:::::inlines::::::::::::::::::::\n");
if (list_first(r->inlinedmethods) == NULL) {
	printf("Nothing\n");fflush(stdout);
	}
else {
	for (im=list_first(r->inlinedmethods),cnt=0; 
     	     im!=NULL;
     	     im=list_next(r->inlinedmethods,im),cnt++) {
		printf("*"); fflush(stdout);
		printf("%i:",cnt);
		printf("[1L%i] ",im->firstlocal); fflush(stdout);
		METHINFOj(im->method)
		printf("::::: which inlines::"); fflush(stdout);	
		if (list_first(im->inlinedmethods) == NULL) {
			printf("Nothing\n");fflush(stdout);
			}
		else   {
			printf("##"); fflush(stdout);
			for (im2=list_first(im->inlinedmethods),cnt2=0; 
     		     	     im2!=NULL;
 		     	     im2=list_next(im2->inlinedmethods,im2),cnt2++) 
				{
				printf("\t%i::",cnt2); fflush(stdout);
				printf("[1L%i] ",im2->firstlocal); 
					fflush(stdout);
				METHINFOj(im2->method)
				}
			printf("\n"); fflush(stdout);
			}
  		} 
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
