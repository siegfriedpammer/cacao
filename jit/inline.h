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

   $Id: inline.h 572 2003-11-06 16:06:11Z twisti $

*/


#ifndef _INLINE_H
#define _INLINE_H

#include "global.h"
#include "toolbox/list.h"

#define INLINING_MAXDEPTH       1
#define INLINING_MAXCODESIZE    32
#define INLINING_MAXMETHODS     8


/*typedef struct {
        listnode linkage;
        instruction *iptr;
        } t_patchlistnode;*/


typedef struct {
    listnode linkage;

    methodinfo *method;
    int startgp;
    int stopgp;
    int firstlocal;

    bool *readonly;
    int  *label_index;
        
    list *inlinedmethods;
} inlining_methodinfo;


typedef struct {
    listnode linkage;
        
    // saved static compiler variables
        
    methodinfo *method;
        
    // restored through method

    // int jcodelength;
    // u1 *jcode;
    // classinfo *class;

    // descriptor never used
    // maxstack used outside of main for loop
    // maxlocals never used
	
    // exceptiontablelength
    // raw_extable used outside of main for loop
    // mreturntype used outside of main for loop
    // mparamcount used outside of main for loop
    // mparamtypes used outside of main for loop

    //local variables used in parse()  

    int  i;                     /* temporary for different uses (counters)    */
    int  p;                     /* java instruction counter                   */
    int  nextp;                 /* start of next java instruction             */
    int  opcode;                /* java opcode                                */

    inlining_methodinfo *inlinfo;

    /*	list *patchlist; */
} t_inlining_stacknode;


/* extern variables */
extern bool isinlinedmethod;
extern int cumjcodelength;
extern int cummaxstack;
extern int cumextablelength;
extern inlining_methodinfo *inlining_rootinfo;


/* function prototypes*/
void inlining_init(methodinfo *m);
void inlining_push_compiler_variables(int i, int p, int nextp, int opcode, 
                                      inlining_methodinfo* inlinfo);
void inlining_pop_compiler_variables(int *i, int *p, int *nextp, int *opcode, 
                                     inlining_methodinfo** inlinfo); 
inlining_methodinfo *inlining_analyse_method(methodinfo *m, int level, int gp,
                                             int firstlocal, int maxstackdepth);


#define inlining_save_compiler_variables() \
    inlining_push_compiler_variables(i, p, nextp, opcode, inlinfo)

#define inlining_restore_compiler_variables() \
    inlining_pop_compiler_variables(&i, &p, &nextp, &opcode, &inlinfo)

#define inlining_set_compiler_variables(i) \
    do { \
        p = nextp = 0; \
        inlining_set_compiler_variables_fun(i->method); \
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
