/* jit/inline.c - code inliner

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

   $Id: inline.c 1205 2004-06-25 06:18:44Z carolyn $

*/


#include <stdio.h>
#include <string.h>
#include "main.h"
#include "jit/inline.h"
#include "jit/jit.h"
#include "jit/parse.h"
#include "loader.h"
#include "tables.h"
#include "toolbox/logging.h"
#include "toolbox/memory.h"


// checked functions and macros: LOADCONST code_get OP1 BUILTIN block_insert bound_check ALIGN

// replace jcodelength loops with correct number after main for loop in parse()!

static list *inlining_stack;
//static list *inlining_patchlist;
bool isinlinedmethod;
int cumjcodelength;         /* cumulative immediate intruction length      */
static int cumlocals;
int cummaxstack;
int cumextablelength;
static int cummethods;
inlining_methodinfo *inlining_rootinfo;


void inlining_init(methodinfo *m)
{
	inlining_stack = NULL;
	//	inlining_patchlist = NULL;
	isinlinedmethod = 0;
	cumjcodelength = 0;
	cumlocals = 0;
	cumextablelength = 0;
	cummaxstack = 0;
	cummethods = 0;

	inlining_stack = NEW(list);
	list_init(inlining_stack, OFFSET(t_inlining_stacknode, linkage));
	
	inlining_rootinfo = inlining_analyse_method(m, 0, 0, 0, 0);
	m->maxlocals = cumlocals;
	m->maxstack = cummaxstack;
}


void inlining_cleanup()
{
	FREE(inlining_stack, t_inlining_stacknode);
}


void inlining_push_compiler_variables(methodinfo *m, int i, int p, int nextp, int opcode, inlining_methodinfo *inlinfo) 
{
	t_inlining_stacknode *new = NEW(t_inlining_stacknode);

	new->i = i;
	new->p = p;
	new->nextp = nextp;
	new->opcode = opcode;
	new->method = m;
	//	new->patchlist = inlining_patchlist;
	new->inlinfo = inlinfo;
	
	list_addfirst(inlining_stack, new);
	isinlinedmethod++;
}


void inlining_pop_compiler_variables(methodinfo *m, int *i, int *p, int *nextp, int *opcode, inlining_methodinfo **inlinfo) 
{
	t_inlining_stacknode *tmp = (t_inlining_stacknode *) list_first(inlining_stack);

	if (!isinlinedmethod) panic("Attempting to pop from inlining stack in toplevel method!\n");

	*i = tmp->i;
	*p = tmp->p;
	*nextp = tmp->nextp;
	*opcode = tmp->opcode;
	*inlinfo = tmp->inlinfo;

	/* XXX TWISTI */
/*  	method = tmp->method; */
/*  	class = method->class; */
/*  	jcodelength = method->jcodelength; */
/*  	jcode = method->jcode; */
	//	inlining_patchlist = tmp->patchlist;

	list_remove(inlining_stack, tmp);
	FREE(tmp, t_inlining_stacknode);
	isinlinedmethod--;
}


void inlining_set_compiler_variables_fun(methodinfo *m)
{
	/* XXX TWISTI */
/*  	method = m; */
/*  	class = m->class; */
/*  	jcodelength = m->jcodelength; */
/*  	jcode = m->jcode; */
	
	//	inlining_patchlist = DNEW(list);
	//	list_init(inlining_patchlist, OFFSET(t_patchlistnode, linkage));
}


/*void inlining_addpatch(instruction *iptr)  
  {
  t_patchlistnode *patch = DNEW(t_patchlistnode);
  patch->iptr = iptr;
  list_addlast(inlining_patchlist, patch);
  }*/


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


inlining_methodinfo *inlining_analyse_method(methodinfo *m, int level, int gp, int firstlocal, int maxstackdepth)
{
	inlining_methodinfo *newnode = DNEW(inlining_methodinfo);
	u1 *jcode = m->jcode;
	int jcodelength = m->jcodelength;
	int p;
	int nextp;
	int opcode;
	int i;
/*  	int lastlabel = 0; */
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
	
	label_index = DMNEW(int, jcodelength);

	newnode->inlinedmethods = DNEW(list);
	list_init(newnode->inlinedmethods, OFFSET(inlining_methodinfo, linkage));

	newnode->method = m;
	newnode->startgp = gp;
	newnode->readonly = readonly;
	newnode->label_index = label_index;
	newnode->firstlocal = firstlocal;
	cumjcodelength += jcodelength + m->paramcount + 1 + 5;

	if ((firstlocal + m->maxlocals) > cumlocals) {
		cumlocals = firstlocal + m->maxlocals;
	}

	if ((maxstackdepth + m->maxstack) > cummaxstack) {
		cummaxstack = maxstackdepth + m->maxstack;
	}

	cumextablelength += m->exceptiontablelength;
   

	for (p = 0; p < jcodelength; gp += (nextp - p), p = nextp) {
		opcode = code_get_u1 (p);
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
			nextp += code_get_u4(nextp) * 8 + 4;
			break;

		case JAVA_TABLESWITCH:
			nextp = ALIGN((p + 1), 4) + 4;
			nextp += (code_get_u4(nextp+4) - code_get_u4(nextp) + 1) * 4 + 4;
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
					i = code_get_u1(p + 1);

				} else {
					i = code_get_u2(p + 1);
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
					i = code_get_u1(p + 1);

				} else {
					i = code_get_u2(p + 1);
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
				i = code_get_u2(p + 1);
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

					if ((cummethods < INLINING_MAXMETHODS) &&
						(!(imi->flags & ACC_NATIVE)) &&  
						(!inlineoutsiders || (m->class == imr->class)) && 
						(imi->jcodelength < INLINING_MAXCODESIZE) && 
						(imi->jcodelength > 0) && 
						(!inlineexceptions || (imi->exceptiontablelength == 0))) { //FIXME: eliminate empty methods?
						inlining_methodinfo *tmp;
						descriptor2types(imi);

						cummethods++;

						if (verbose) {
							char logtext[MAXLOGTEXT];
							sprintf(logtext, "Going to inline: ");
							utf_sprint(logtext  +strlen(logtext), imi->class->name);
							strcpy(logtext + strlen(logtext), ".");
							utf_sprint(logtext + strlen(logtext), imi->name);
							utf_sprint(logtext + strlen(logtext), imi->descriptor);
							log_text(logtext);
						}
						
						tmp = inlining_analyse_method(imi, level + 1, gp, firstlocal + m->maxlocals, maxstackdepth + m->maxstack);
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

	/*
	sprintf (logtext, "Result of inlining analysis of: ");
	utf_sprint (logtext+strlen(logtext), m->class->name);
	strcpy (logtext+strlen(logtext), ".");
	utf_sprint (logtext+strlen(logtext), m->name);
	utf_sprint (logtext+strlen(logtext), m->descriptor);
	dolog ();
	sprintf (logtext, "label_index[0..%d]->", jcodelength);
	for (i=0; i<jcodelength; i++) sprintf (logtext, "%d:%d ", i, label_index[i]);
	sprintf(logtext,"stopgp : %d\n",newnode->stopgp); */

    return newnode;
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
