/* jit/inline.c - code inliner

globals moved to structure and passed as parameter

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

   Authors: Dieter Thuernbeck

   $Id: inline.c 1432 2004-11-03 12:14:50Z jowenn $

*/


#include <stdio.h>
#include <string.h>
#include "global.h"
#include "loader.h"
#include "tables.h"
#include "options.h"
#include "jit/inline.h"
#include "jit/jit.h"
#include "jit/parse.h"
#include "toolbox/logging.h"
#include "toolbox/memory.h"

#define METHINFO(m) \
  utf_display(m->class->name); printf("."); fflush(stdout); \
  method_display(m); fflush(stdout); \

#define IMETHINFO(m) \
  utf_display(m->class->name); printf("."); fflush(stdout); \
  method_display(m); fflush(stdout); \
  printf("\tm->jcodelength=%i; ",m->jcodelength); fflush(stdout); \
  printf("m->jcode=%p;\n",m->jcode); fflush(stdout); \
  printf("\tm->maxlocals=%i; ",m->maxlocals); fflush(stdout); \
  printf("m->maxstack=%i;\n",m->maxstack); fflush(stdout);

bool DEBUGi = false;
// checked functions and macros: LOADCONST code_get OP1 BUILTIN block_insert bound_check ALIGN

// replace jcodelength loops with correct number after main for loop in parse()!

/*-----------------------------------------------------------*/
/* just initialize global structure for non-inlining         */
/*-----------------------------------------------------------*/

t_inlining_globals *inlining_init0(methodinfo *m, 
				   t_inlining_globals *inline_env)
{
 /* initialization for normal use in parse */
        inlining_set_compiler_variables_fun(m, inline_env);
        inline_env->isinlinedmethod = 0;
	inline_env->cumjcodelength = m->jcodelength; /* for not inlining */
        inline_env->cummaxstack = m->maxstack; /*why has here been 0 ? */
        inline_env->cumextablelength = 0;
        inline_env->cumlocals = m->maxlocals;
        inline_env->cummethods = 0;//co not global or static-used only here?
        inline_env->inlining_stack = NULL;
        inline_env->inlining_rootinfo = NULL;
return inline_env;
}
/*-----------------------------------------------------------*/

t_inlining_globals *inlining_init(methodinfo *m)
{
  	t_inlining_globals *inline_env = DNEW(t_inlining_globals);
        inlining_init0(m,inline_env);
if (useinlining)
        {
if (DEBUGi==true) {
 		printf("\n-------- Inlining init for: "); fflush(stdout);
  		IMETHINFO(m)
                }
        inline_env->cumjcodelength = 0;
        inline_env->inlining_stack = NEW(list);
        list_init(inline_env->inlining_stack, 
		  OFFSET(t_inlining_stacknode, linkage));
        /*------ analyze ------*/
if (DEBUGi==true) {print_t_inlining_globals(inline_env);}
        inline_env->inlining_rootinfo 
		= inlining_analyse_method(m, 0, 0, 0, 0, inline_env);
if (DEBUGi==true) {print_t_inlining_globals(inline_env);}
        /*---------------------*/
	//if (inline_env->cummethods == 0) {
  	//  inline_env = DNEW(t_inlining_globals);
	//  inlining_init0(m,inline_env);
	//  return inline_env;
        //  }
if (DEBUGi==true) {
  printf("(l,s) (%i,%i) was (%i,%i)\n",
    m->maxlocals, inline_env->cumlocals,
    m->maxstack,  inline_env->cummaxstack); fflush(stdout);
  }
#if 0
/*This looks wrong*/
        m->maxlocals = inline_env->cumlocals;   //orig not used
        m->maxstack = inline_env->cummaxstack;  //orig global maxstack var!!
#endif
//panic("TEMP so can test just inline init\n");
        }
return inline_env;
}


void inlining_cleanup(t_inlining_globals *inline_env)
{
	FREE(inline_env->inlining_stack, t_inlining_stacknode);
}


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
	/* XXX TWISTI */
        inline_env->method = m; /*co*/
        inline_env->class  = m->class; /*co*/
        inline_env->jcode  = m->jcode; /*co*/
        inline_env->jcodelength = m->jcodelength; /*co*/
}


classinfo *first_occurence(classinfo* class, utf* name, utf* desc)
{
	classinfo *first = class;
	
	for (; class->super != NULL ; class = class->super) {
		if (class_findmethod(class->super, name, desc) != NULL) {
			first = class->super;
		}			
	}

	return first;
}


bool is_unique_rec(classinfo *class, methodinfo *m, utf* name, utf* desc)
{
	methodinfo *tmp = class_findmethod(class, name, desc);
	if ((tmp != NULL) && (tmp != m))
		return false;

	for (; class != NULL; class = class->nextsub) {
		if ((class->sub != NULL) && !is_unique_rec(class->sub, m, name, desc)) {
			return false; 
		}
	}
	return true;
}


bool is_unique_method(classinfo *class, methodinfo *m, utf* name, utf* desc)
{
	classinfo *firstclass;
	
	/*	sprintf (logtext, "First occurence of: ");
	utf_sprint (logtext+strlen(logtext), m->class->name);
	strcpy (logtext+strlen(logtext), ".");
	utf_sprint (logtext+strlen(logtext), m->name);
	utf_sprint (logtext+strlen(logtext), m->descriptor);
	dolog (); */
	
	firstclass = first_occurence(class, name, desc);
	
	/*	sprintf (logtext, "\nis in class:");
	utf_sprint (logtext+strlen(logtext), firstclass->name);
	dolog (); */

	if (firstclass != class) return false;

	return is_unique_rec(class, m, name, desc);
}

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
	int i;
	bool iswide = false, oldiswide;
	bool *readonly = NULL;
	int  *label_index = NULL;
	bool isnotrootlevel = (level > 0);
	bool isnotleaflevel = (level < INLINING_MAXDEPTH);

	//	if (level == 0) gp = 0;
	/*
	sprintf (logtext, "Performing inlining analysis of: ");
	utf_sprint (logtext+strlen(logtext), m->class->name);
	strcpy (logtext+strlen(logtext), ".");
	utf_sprint (logtext+strlen(logtext), m->name);
	utf_sprint (logtext+strlen(logtext), m->descriptor);
	dolog (); */

	if (isnotrootlevel) {
		newnode->readonly = readonly = DMNEW(bool, m->maxlocals); //FIXME only paramcount entrys necessary
		for (i = 0; i < m->maxlocals; readonly[i++] = true);
		isnotrootlevel = true;

	} else {
		readonly = NULL;
	}
	
	label_index = DMNEW(int, jcodelength+200);

	newnode->inlinedmethods = DNEW(list);
	list_init(newnode->inlinedmethods, OFFSET(inlining_methodinfo, linkage));

	newnode->method = m;
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
		//		printf("lastlabel=%d p=%d gp=%d\n",lastlabel, p, gp);
		lastlabel = p+1; */
		for (i = p; i < nextp; i++) label_index[i] = gp;

		if (isnotleaflevel) { 

			switch (opcode) {
			case JAVA_INVOKEVIRTUAL:
				if (!inlinevirtuals)
					break;
			case JAVA_INVOKESTATIC:
				i = code_get_u2(p + 1,m);
				{
					constant_FMIref *imr;
					methodinfo *imi;

					imr = class_getconstant(m->class, i, CONSTANT_Methodref);

					if (!class_load(imr->class))
						return NULL;

					if (!class_link(imr->class))
						return NULL;

					imi = class_resolveclassmethod(imr->class,
												   imr->name,
												   imr->descriptor,
												   m->class,
												   true);

					if (!imi)
						panic("Exception thrown while parsing bytecode"); /* XXX should be passed on */

					if (opcode == JAVA_INVOKEVIRTUAL) {
						if (!is_unique_method(imi->class, imi, imr->name, imr->descriptor))
							break;
					}

					/*if (imi->flags & ACC_NATIVE) log_text("Native method,no inlining");*/
					if ((inline_env->cummethods < INLINING_MAXMETHODS) &&
						(!(imi->flags & ACC_NATIVE)) &&  
						(!inlineoutsiders || (m->class == imr->class)) && 
						(imi->jcodelength < INLINING_MAXCODESIZE) && 
						(imi->jcodelength > 0) && 
						(inlineexceptions || (imi->exceptiontablelength == 0))) { //FIXME: eliminate empty methods?
						inlining_methodinfo *tmp;
						descriptor2types(imi);

						inline_env->cummethods++;

						if (verbose) {
							char logtext[MAXLOGTEXT];
							sprintf(logtext, "Going to inline: ");
							utf_sprint(logtext  +strlen(logtext), imi->class->name);
							strcpy(logtext + strlen(logtext), ".");
							utf_sprint(logtext + strlen(logtext), imi->name);
							utf_sprint(logtext + strlen(logtext), imi->descriptor);
							log_text(logtext);
						}
						
						tmp =inlining_analyse_method(imi, level + 1, gp, firstlocal + m->maxlocals, maxstackdepth + m->maxstack, inline_env);
						list_addlast(newnode->inlinedmethods, tmp);
						gp = tmp->stopgp;
						p = nextp;
					}
				}
				break;
			}
		}  
	} /* for */
	
	newnode->stopgp = gp;

        if (DEBUGi==true) {
	  printf ("\nResult of inlining analysis of: ");
	  IMETHINFO(m);
	  printf("was called by:\n"); fflush(stdout);
	  IMETHINFO(inline_env->method);
	  printf ("label_index[0..%d]->", jcodelength);
	  for (i=0;i<jcodelength; i++) printf ("%d:%d ", i, label_index[i]);
	  printf("stopgp : %d\n",newnode->stopgp); 
          }

    return newnode;
}

/* --------------------------------------------------------------------*/
/*  print_ functions: check inline structures contain what is expected */
/* --------------------------------------------------------------------*/
void print_t_inlining_globals (t_inlining_globals *g) 
{
printf("\n------------\nt_inlining_globals struct for: \n\t");fflush(stdout); 
METHINFO(g->method);
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
if (s==NULL) { 
  printf("\n\tinlining_stack: NULL\n");
  return;
  }
  
  {/* print first  element to see if get into stack */
  t_inlining_stacknode *is;
  printf("\n\tinlining_stack: NOT NULL\n");

  is=list_first(s); 
  if (is==NULL) { 
    printf("\n\tinlining_stack = init'd but EMPTY\n");
    fflush(stdout);
    return;
    }
  }

  {
  t_inlining_stacknode *is;
  printf("\n\tinlining_stack: NOT NULL\n");


  for (is=list_first(s); 
       is!=NULL;
       is=list_next(s,is)) {
 	 printf("\n\ti>--->inlining_stack entry: \n"); fflush(stdout);
	 METHINFO(is->method);
	 printf("i=%i, p=%i, nextp=%i, opcode=%i;\n",
		is->i,is->p,is->nextp,is->opcode);fflush(stdout);
	 print_inlining_methodinfo(is->inlinfo);
    } /*end for */
  } 

}

/* --------------------------------------------------------------------*/
void print_inlining_methodinfo( inlining_methodinfo *r) {
if (r==NULL) { 
  printf("\n\tinlining_methodinfo: NULL\n");
  return;
  }

if (r->method != NULL) {
  utf_display(r->method->class->name); printf("."); fflush(stdout); \
  method_display(r->method); fflush(stdout); \
  }
else {
  printf("method is NULL!!!!!\n");fflush(stdout);
  }

printf("\n\tinlining_methodinfo for:"); fflush(stdout);
if (r->readonly==NULL) {
  printf("\treadonly==NULL ");fflush(stdout);  
  }
else {
  int i;
  printf("\treadonly=");fflush(stdout);  
  for (i = 0; i < r->method->maxlocals; i++)  {
    if (r->readonly[i] == true)
      printf("[i]=T;");
    else
      printf("[i]=F;");
    fflush(stdout);
    } 
  }


printf("\tstartgp=%i; stopgp=%i; firstlocal=%i; label_index=%p;\n",
          r->startgp, r->stopgp, r->firstlocal, r->label_index);
{int i;
printf ("label_index[0..%d]->", r->method->jcodelength);
for (i=0; i<r->method->jcodelength; i++) printf ("%d:%d ", i, r->label_index[i]);
}

{
inlining_methodinfo *im;
for (im=list_first(r->inlinedmethods); 
     im!=NULL;
     im=list_next(r->inlinedmethods,im)) {
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
