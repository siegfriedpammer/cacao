/* src/vm/jit/emit-common.h - common code emitter functions

   Copyright (C) 2006 R. Grafl, A. Krall, C. Kruegel, C. Oates,
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Contact: cacao@cacaojvm.org

   Authors: Christian Thalinger

   $Id: emitfuncs.c 4398 2006-01-31 23:43:08Z twisti $

*/


#ifndef _EMIT_H
#define _EMIT_H

#include "vm/types.h"

#include "vm/jit/jit.h"


/* constant range macros ******************************************************/

#if SIZEOF_VOID_P == 8

# define IS_IMM8(c) \
    (((s8) (c) >= -128) && ((s8) (c) <= 127))

# define IS_IMM32(c) \
    (((s8) (c) >= (-2147483647-1)) && ((s8) (c) <= 2147483647))

#else

# define IS_IMM8(c) \
    (((s4) (c) >= -128) && ((s4) (c) <= 127))

#endif


/* code generation functions **************************************************/

s4 emit_load(jitdata *jd, instruction *iptr, varinfo *src, s4 tempreg);
s4 emit_load_s1(jitdata *jd, instruction *iptr, s4 tempreg);
s4 emit_load_s2(jitdata *jd, instruction *iptr, s4 tempreg);
s4 emit_load_s3(jitdata *jd, instruction *iptr, s4 tempreg);

#if SIZEOF_VOID_P == 4
s4 emit_load_low(jitdata *jd, instruction *iptr, varinfo *src, s4 tempreg);
s4 emit_load_s1_low(jitdata *jd, instruction *iptr, s4 tempreg);
s4 emit_load_s2_low(jitdata *jd, instruction *iptr, s4 tempreg);
s4 emit_load_s3_low(jitdata *jd, instruction *iptr, s4 tempreg);

s4 emit_load_high(jitdata *jd, instruction *iptr, varinfo *src, s4 tempreg);
s4 emit_load_s1_high(jitdata *jd, instruction *iptr, s4 tempreg);
s4 emit_load_s2_high(jitdata *jd, instruction *iptr, s4 tempreg);
s4 emit_load_s3_high(jitdata *jd, instruction *iptr, s4 tempreg);
#endif

void emit_store(jitdata *jd, instruction *iptr, varinfo *dst, s4 d);
void emit_store_dst(jitdata *jd, instruction *iptr, s4 d);

#if SIZEOF_VOID_P == 4
void emit_store_low(jitdata *jd, instruction *iptr, varinfo *dst, s4 d);
void emit_store_high(jitdata *jd, instruction *iptr, varinfo *dst, s4 d);
#endif

void emit_copy(jitdata *jd, instruction *iptr, varinfo *src, varinfo *dst);

void emit_iconst(codegendata *cd, s4 d, s4 value);
void emit_lconst(codegendata *cd, s4 d, s8 value);

void emit_br(codegendata *cd, basicblock *target);
void emit_bc(codegendata *cd, basicblock *target, s4 condition);

void emit_beq(codegendata *cd, basicblock *target);
void emit_bne(codegendata *cd, basicblock *target);
void emit_blt(codegendata *cd, basicblock *target);
void emit_bge(codegendata *cd, basicblock *target);
void emit_bgt(codegendata *cd, basicblock *target);
void emit_ble(codegendata *cd, basicblock *target);

void emit_bnan(codegendata *cd, basicblock *target);

void emit_branch(codegendata *cd, s4 disp, s4 condition);

void emit_arithmetic_check(codegendata *cd, instruction *iptr, s4 reg);
void emit_arrayindexoutofbounds_check(codegendata *cd, instruction *iptr, s4 s1, s4 s2);
void emit_arraystore_check(codegendata *cd, instruction *iptr, s4 reg);
void emit_classcast_check(codegendata *cd, instruction *iptr, s4 condition, s4 reg, s4 s1);
void emit_nullpointer_check(codegendata *cd, instruction *iptr, s4 reg);
void emit_exception_check(codegendata *cd, instruction *iptr);

void emit_array_checks(codegendata *cd, instruction *iptr, s4 s1, s4 s2);

void emit_exception_stubs(jitdata *jd);
void emit_patcher_stubs(jitdata *jd);
#if defined(ENABLE_REPLACEMENT)
void emit_replacement_stubs(jitdata *jd);
#endif

void emit_verbosecall_enter(jitdata *jd);
void emit_verbosecall_exit(jitdata *jd);

#endif /* _EMIT_H */


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
