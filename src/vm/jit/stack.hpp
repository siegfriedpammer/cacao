/* src/vm/jit/stack.h - stack analysis header

   Copyright (C) 1996-2005, 2006, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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

*/


#ifndef _STACK_H
#define _STACK_H

/* forward typedefs ***********************************************************/

typedef struct stackelement_t stackelement_t;


#include "config.h"

#include <stdint.h>

#include "vm/types.h"

#include "vm/global.h"

#include "vm/jit/jit.hpp"
#include "vm/jit/reg.hpp"


/* stack element structure ****************************************************/

/* flags */

#define SAVEDVAR      1         /* variable has to survive method invocations */
#define INMEMORY      2         /* variable stored in memory                  */
#define SAVREG        4         /* allocated to a saved register              */
#define ARGREG        8         /* allocated to an arg register               */
#define PASSTHROUGH  32         /* stackslot was passed-through by an ICMD    */
#define PREALLOC     64         /* preallocated var like for ARGVARS. Used    */
                                /* with the new var system */
#define INOUT    128            /* variable is an invar or/and an outvar      */

#define IS_SAVEDVAR(x)    ((x) & SAVEDVAR)
#define IS_INMEMORY(x)    ((x) & INMEMORY)


/* variable kinds */

#define UNDEFVAR   0            /* stack slot will become temp during regalloc*/
#define TEMPVAR    1            /* stack slot is temp register                */
#define STACKVAR   2            /* stack slot is numbered stack slot          */
#define LOCALVAR   3            /* stack slot is local variable               */
#define ARGVAR     4            /* stack slot is argument variable            */


struct stackelement_t {
	stackelement_t *prev;       /* pointer to next element towards bottom     */
	instruction    *creator;    /* instruction that created this element      */
	s4              type;       /* slot type of stack element                 */
	s4              flags;      /* flags (SAVED, INMEMORY)                    */
	s4              varkind;    /* kind of variable or register               */
	s4              varnum;     /* number of variable                         */
};


/* macros used internally by analyse_stack ************************************/

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

/* function prototypes ********************************************************/

#ifdef __cplusplus
extern "C" {
#endif

bool stack_init(void);

bool stack_analyse(jitdata *jd);

void stack_javalocals_store(instruction *iptr, s4 *javalocals);

#ifdef __cplusplus
}
#endif

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
