/* jit/inline.h - code inliner

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Contact: cacao@cacaojvm.org

   Authors: Dieter Thuernbeck

   $Id: inline.h 4357 2006-01-22 23:33:38Z twisti $

*/


#ifndef _INLINE_H
#define _INLINE_H

/* resolve typedef cycles *****************************************************/

typedef struct t_inlining_globals t_inlining_globals;


#include "toolbox/list.h"
#include "vm/global.h"


#define INLINING_MAXDEPTH       3  /*1*/ 
#define INLINING_MAXCODESIZE    128 /*32*/
#define INLINING_MAXMETHODS     32 /*8*/

typedef struct {
    listnode linkage;

    methodinfo *method;
    int level;
    int startgp;
    int stopgp;
    int firstlocal;

    bool *readonly;
    int  *label_index;
        
    list *inlinedmethods;
} inlining_methodinfo;


typedef struct {
    listnode linkage;
        
    /* saved static compiler variables */
        
    methodinfo *method;
        
    /* restored through method */

    /* int jcodelength; */
    /* u1 *jcode; */
	/* classinfo *class; */

    /* descriptor never used */
    /* maxstack used outside of main for loop */
    /* maxlocals never used */
	
    /* exceptiontablelength */
    /* raw_extable used outside of main for loop */
    /* mreturntype used outside of main for loop */
    /* mparamcount used outside of main for loop */
    /* mparamtypes used outside of main for loop */

    /* local variables used in parse() */

    int  i;                     /* temporary for different uses (counters)*/
    int  p;                     /* java instruction counter               */
    int  nextp;                 /* start of next java instruction         */
    int  opcode;                /* java opcode                            */
    u2 lineindex;
    u2 currentline;
    u2 linepcchange;
    inlining_methodinfo *inlinfo;

} t_inlining_stacknode;


struct t_inlining_globals {  /* try in parse.h with struct not include */
	bool isinlinedmethod;
	int cumjcodelength;   /* cumulative immediate intruction length */
	int cummaxstack;
	int cumextablelength;
	int cumlocals;        /* was static */
	int cummethods;       /* was static */
	list *inlining_stack; /* was static */
	inlining_methodinfo *inlining_rootinfo;
	methodinfo *method;
	classinfo *class;
	int jcodelength;
	u1 *jcode;
	bool isleafmethod;
};


/* function prototypes*/

void inlining_setup(methodinfo *m, t_inlining_globals *inline_env);
void inlining_cleanup(t_inlining_globals *inline_env);
void inlining_push_compiler_variables(
				      int i, int p, int nextp, int opcode, 
				      u2 lineindex,u2 currentline,u2 linepcchange,
                                      inlining_methodinfo* inlinfo,
				      t_inlining_globals *inline_env);
void inlining_pop_compiler_variables(
 				    int *i, int *p, int *nextp, int *opcode,
				    u2 *lineindex,u2 *currentline,u2 *linepcchange,
                                    inlining_methodinfo **inlinfo,
				    t_inlining_globals *inline_env);
void inlining_set_compiler_variables_fun(methodinfo *m, 
					 t_inlining_globals *inline_env);
classinfo *first_occurence(classinfo* class, utf* name, utf* desc);
bool is_unique_recOLD(classinfo *class, methodinfo *m, utf* name, utf* desc);
bool is_unique_methodOLD(classinfo *class, methodinfo *m, utf* name, utf* desc);
inlining_methodinfo *inlining_analyse_method(methodinfo *m, 
					  int level, int gp,
					  int firstlocal, int maxstackdepth,					      t_inlining_globals *inline_env);

void print_t_inlining_globals (t_inlining_globals *g);
void print_inlining_stack     ( list                *s);
void print_inlining_methodinfo( inlining_methodinfo *r);

#define inlining_save_compiler_variables() \
    inlining_push_compiler_variables(i,p,nextp,opcode, lineindex,currentline, \
	linepcchange,inlinfo,inline_env)

#define inlining_restore_compiler_variables() \
    inlining_pop_compiler_variables(&i, &p, &nextp, &opcode, \
	&lineindex,&currentline,&linepcchange,&inlinfo, \
	inline_env)

#define inlining_set_compiler_variables(i) \
    do { \
        p = nextp = 0; \
        inlining_set_compiler_variables_fun(i->method, inline_env); \
        inlinfo = i; \
    } while (0)

#endif /* _INLINE_H */

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
