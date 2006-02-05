/* src/vm/jit/parse.h - parser header

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

   Author: Christian Thalinger

   Changes:

   $Id: parse.h 4449 2006-02-05 23:02:05Z edwin $

*/


#ifndef _PARSE_H
#define _PARSE_H

#include "config.h"
#include "vm/types.h"

#include "vm/global.h"
#include "vm/jit/codegen-common.h"


/* intermediate code generating macros ****************************************/

#define PINC           iptr++;ipc++

#define LOADCONST_I(v) \
    iptr->opc    = ICMD_ICONST; \
    iptr->val.i  = (v); \
    iptr->line   = currentline; \
    iptr->method = m; \
    PINC

#define LOADCONST_L(v) \
    iptr->opc    = ICMD_LCONST; \
    iptr->val.l  = (v); \
    iptr->line   = currentline; \
    iptr->method = m; \
    PINC

#define LOADCONST_F(v) \
    iptr->opc    = ICMD_FCONST; \
    iptr->val.f  = (v); \
    iptr->line   = currentline; \
    iptr->method = m; \
    PINC

#define LOADCONST_D(v) \
    iptr->opc    = ICMD_DCONST; \
    iptr->val.d  = (v); \
    iptr->line   = currentline; \
    iptr->method = m; \
    PINC

#define LOADCONST_A(v) \
    iptr->opc    = ICMD_ACONST; \
    iptr->val.a  = (v); \
    iptr->line   = currentline; \
    iptr->method = m; \
    PINC

#define LOADCONST_A_CLASS(v,t) \
    iptr->opc    = ICMD_ACONST; \
    iptr->val.a  = (v); \
    iptr->target = (t); \
    iptr->line   = currentline; \
    iptr->method = m; \
    PINC

/* ACONST instructions generated as arguments for builtin functions
 * have op1 set to non-zero. This is used for stack overflow checking
 * in stack.c. */
#define LOADCONST_A_BUILTIN(v,t) \
    iptr->opc    = ICMD_ACONST; \
    iptr->op1    = 1; \
    iptr->val.a  = (v); \
    iptr->target = (t); \
    iptr->line   = currentline; \
    iptr->method = m; \
    PINC

#define OP(o) \
    iptr->opc    = (o); \
    iptr->line   = currentline; \
    iptr->method = m; \
    PINC

#define OP1(o,o1) \
    iptr->opc    = (o); \
    iptr->op1    = (o1); \
    iptr->line   = currentline; \
    iptr->method = m; \
    PINC

#define OP2I(o,o1,v) \
    iptr->opc    = (o); \
    iptr->op1    = (o1); \
    iptr->val.i  = (v); \
    iptr->line   = currentline; \
    iptr->method = m; \
    PINC

#define OP2A_NOINC(o,o1,v,l) \
    iptr->opc    = (o); \
    iptr->op1    = (o1); \
    iptr->val.a  = (v); \
    iptr->line   = (l); \
    iptr->method = m

#define OP2A(o,o1,v,l) \
    OP2A_NOINC(o,o1,v,l); \
    PINC

#define OP2AT(o,o1,v,t,l) \
    OP2A_NOINC(o,o1,v,l); \
    iptr->target = (t); \
    PINC

#define BUILTIN(v,o1,t,l) \
    m->isleafmethod = false; \
    iptr->opc    = ICMD_BUILTIN; \
    iptr->op1    = (o1); \
    iptr->val.a  = (v); \
    iptr->target = (t); \
    iptr->line   = (l); \
    iptr->method = m; \
    PINC


/* We have to check local variables indices here because they are
 * used in stack.c to index the locals array. */

#define INDEX_ONEWORD(num) \
    do { \
        if ((num) < 0 || (num) >= m->maxlocals) { \
            *exceptionptr = \
                new_verifyerror(m, "Illegal local variable number"); \
            return NULL; \
        } \
    } while (0)

#define INDEX_TWOWORD(num) \
    do { \
        if ((num) < 0 || ((num) + 1) >= m->maxlocals) { \
            *exceptionptr = \
                new_verifyerror(m, "Illegal local variable number"); \
            return NULL; \
        } \
    } while (0)

#define OP1LOAD(o,o1)							\
	do {if (o == ICMD_LLOAD || o == ICMD_DLOAD)	\
			INDEX_TWOWORD(o1);					\
		else									\
			INDEX_ONEWORD(o1);					\
		OP1(o,o1);} while(0)

#define OP1STORE(o,o1)								\
	do {if (o == ICMD_LSTORE || o == ICMD_DSTORE)	\
			INDEX_TWOWORD(o1);						\
		else										\
			INDEX_ONEWORD(o1);						\
		OP1(o,o1);} while(0)

/* block generating and checking macros */

#define block_insert(i) \
    do { \
        if (!(m->basicblockindex[(i)] & 1)) { \
            b_count++; \
            m->basicblockindex[(i)] |= 1; \
        } \
    } while (0)


#define bound_check(i) \
    do { \
        if (i < 0 || i >= m->jcodelength) { \
            *exceptionptr = \
                new_verifyerror(m, "Illegal target of jump or branch"); \
            return NULL; \
        } \
    } while (0)

/* bound_check_exclusive is used for the exclusive ends of exception handler ranges */
#define bound_check_exclusive(i) \
    do { \
        if (i < 0 || i > m->jcodelength) { \
            *exceptionptr = \
                new_verifyerror(m, "Illegal target of jump or branch"); \
            return NULL; \
        } \
    } while (0)


/* macros for byte code fetching ***********************************************

	fetch a byte code of given size from position p in code array jcode

*******************************************************************************/

#define code_get_u1(p,m)  m->jcode[p]
#define code_get_s1(p,m)  ((s1)m->jcode[p])
#define code_get_u2(p,m)  ((((u2)m->jcode[p]) << 8) + m->jcode[p + 1])
#define code_get_s2(p,m)  ((s2)((((u2)m->jcode[p]) << 8) + m->jcode[p + 1]))
#define code_get_u4(p,m)  ((((u4)m->jcode[p]) << 24) + (((u4)m->jcode[p + 1]) << 16) \
                        +(((u4)m->jcode[p + 2]) << 8) + m->jcode[p + 3])
#define code_get_s4(p,m)  ((s4)((((u4)m->jcode[p]) << 24) + (((u4)m->jcode[p + 1]) << 16) \
                             +(((u4)m->jcode[p + 2]) << 8) + m->jcode[p + 3]))


/* function prototypes ********************************************************/

void compiler_addinitclass(classinfo *c);
methodinfo *parse(methodinfo *m, codegendata *cd);

#endif /* _PARSE_H */


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


