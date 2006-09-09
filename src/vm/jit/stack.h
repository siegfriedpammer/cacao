/* vm/jit/stack.h - stack analysis header

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

   Authors: Christian Thalinger

   Changes: Christian Ullrich
   			Edwin Steiner

   $Id: stack.h 5446 2006-09-09 20:05:35Z edwin $

*/


#ifndef _STACK_H
#define _STACK_H

#include "config.h"

#include "vm/types.h"

#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/jit/jit.h"
#include "vm/jit/reg.h"


/* macros used internally by analyse_stack ************************************/

/*--------------------------------------------------*/
/* STACK DEPTH CHECKING                             */
/*--------------------------------------------------*/

#if defined(ENABLE_VERIFIER)
#define CHECK_STACK_DEPTH(depthA,depthB)                             \
    do {                                                             \
        if ((depthA) != (depthB))                                    \
            goto throw_stack_depth_error;                            \
    } while (0)
#else /* !ENABLE_VERIFIER */
#define CHECK_STACK_DEPTH(depthA,depthB)
#endif /* ENABLE_VERIFIER */


/*--------------------------------------------------*/
/* BASIC TYPE CHECKING                              */
/*--------------------------------------------------*/

/* XXX would be nice if we did not have to pass the expected type */

#if defined(ENABLE_VERIFIER)
#define CHECK_BASIC_TYPE(expected,actual)                            \
    do {                                                             \
        if ((actual) != (expected)) {                                \
            expectedtype = (expected);                               \
            goto throw_stack_type_error;                             \
        }                                                            \
    } while (0)
#else /* !ENABLE_VERIFIER */
#define CHECK_BASIC_TYPE(expected,actual)
#endif /* ENABLE_VERIFIER */

/*--------------------------------------------------*/
/* STACK UNDERFLOW/OVERFLOW CHECKS                  */
/*--------------------------------------------------*/

/* underflow checks */

#if defined(ENABLE_VERIFIER)
#define REQUIRE(num)                                                 \
    do {                                                             \
        if (stackdepth < (num))                                      \
            goto throw_stack_underflow;                              \
    } while (0)
#else /* !ENABLE_VERIFIER */
#define REQUIRE(num)
#endif /* ENABLE_VERIFIER */


/* overflow check */
/* We allow ACONST instructions inserted as arguments to builtin
 * functions to exceed the maximum stack depth.  Maybe we should check
 * against maximum stack depth only at block boundaries?
 */

/* XXX we should find a way to remove the opc/op1 check */
#if defined(ENABLE_VERIFIER)
#define CHECKOVERFLOW                                                \
    do {                                                             \
        if (stackdepth > m->maxstack)                                \
            if ((iptr->opc != ICMD_ACONST) || INSTRUCTION_MUST_CHECK(iptr))\
                goto throw_stack_overflow;                           \
    } while(0)
#else /* !ENABLE_VERIFIER */
#define CHECKOVERFLOW
#endif /* ENABLE_VERIFIER */

/*--------------------------------------------------*/
/* ALLOCATING STACK SLOTS                           */
/*--------------------------------------------------*/

#define NEWSTACK(s,v,n)                                              \
    do {                                                             \
        sd.new->prev = curstack;                                     \
        sd.new->type = (s);                                          \
        sd.new->flags = 0;                                           \
        sd.new->varkind = (v);                                       \
        sd.new->varnum = (n);                                        \
        curstack = sd.new;                                           \
        sd.var[(n)].type = (s);                                      \
        sd.var[(n)].flags = 0;                                       \
        sd.new++;                                                    \
    } while (0)

/* Initialize regoff, so -sia can show regnames even before reg.inc */
/* regs[rd->intregargnum] has to be set for this                    */
/* new->regoff = (IS_FLT_DBL_TYPE(s))?-1:rd->intreg_argnum; }       */

#define NEWSTACKn(s,n)  NEWSTACK(s,UNDEFVAR,n)
#define NEWSTACK0(s)    NEWSTACK(s,UNDEFVAR,0)


/*--------------------------------------------------*/
/* MACROS FOR HANDLING BASIC BLOCKS                 */
/*--------------------------------------------------*/

/* COPYCURSTACK makes a copy of the current operand stack (curstack)
 * and returns it in the variable copy.
 *
 * This macro is used to propagate the operand stack from one basic
 * block to another. The destination block receives the copy as its
 * input stack.
 */
#define COPYCURSTACK(sd, copy) {                                     \
    stackptr s;                                                      \
    if (curstack) {                                                  \
        s = curstack;                                                \
        (sd).new += stackdepth;                                      \
        copy = (sd).new;                                             \
        while (s) {                                                  \
            copy--;                                                  \
            copy->prev = copy-1;                                     \
            copy->creator = NULL;                                    \
            copy->type = s->type;                                    \
            copy->flags = 0;                                         \
            copy->varkind = STACKVAR;                                \
            s = s->prev;                                             \
        }                                                            \
        copy->prev = NULL;                                           \
        copy = (sd).new-1;                                           \
    }                                                                \
    else                                                             \
        copy = NULL;                                                 \
}


/* external macros ************************************************************/

#define BLOCK_OF(index)                                              \
    (jd->new_basicblocks + jd->new_basicblockindex[index])


/* function prototypes ********************************************************/

bool stack_init(void);

bool new_stack_analyse(jitdata *jd);

#endif /* _STACK_H */


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
 * vim:noexpandtab:sw=4:ts=4:
 */
