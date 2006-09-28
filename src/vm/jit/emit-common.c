/* src/vm/jit/emit-common.c - common code emitter functions

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

   Changes: Edwin Steiner

   $Id: emitfuncs.c 4398 2006-01-31 23:43:08Z twisti $

*/


#include "config.h"

#include <assert.h>

#include "vm/types.h"

#include "vm/jit/emit-common.h"
#include "vm/jit/jit.h"


/* emit_load_s1 ****************************************************************

   Emits a possible load of the first source operand.

*******************************************************************************/

s4 emit_load_s1(jitdata *jd, instruction *iptr, s4 tempreg)
{
	varinfo *src;
	s4       reg;

	src = VAROP(iptr->s1);

	reg = emit_load(jd, iptr, src, tempreg);

	return reg;
}


/* emit_load_s2 ****************************************************************

   Emits a possible load of the second source operand.

*******************************************************************************/

s4 emit_load_s2(jitdata *jd, instruction *iptr, s4 tempreg)
{
	varinfo *src;
	s4       reg;

	src = VAROP(iptr->sx.s23.s2);

	reg = emit_load(jd, iptr, src, tempreg);

	return reg;
}


/* emit_load_s3 ****************************************************************

   Emits a possible load of the third source operand.

*******************************************************************************/

s4 emit_load_s3(jitdata *jd, instruction *iptr, s4 tempreg)
{
	varinfo *src;
	s4       reg;

	src = VAROP(iptr->sx.s23.s3);

	reg = emit_load(jd, iptr, src, tempreg);

	return reg;
}


/* emit_load_s1_low ************************************************************

   Emits a possible load of the low 32-bits of the first long source
   operand.

*******************************************************************************/

#if SIZEOF_VOID_P == 4
s4 emit_load_s1_low(jitdata *jd, instruction *iptr, s4 tempreg)
{
	varinfo *src;
	s4       reg;

	src = VAROP(iptr->s1);

	reg = emit_load_low(jd, iptr, src, tempreg);

	return reg;
}
#endif


/* emit_load_s2_low ************************************************************

   Emits a possible load of the low 32-bits of the second long source
   operand.

*******************************************************************************/

#if SIZEOF_VOID_P == 4
s4 emit_load_s2_low(jitdata *jd, instruction *iptr, s4 tempreg)
{
	varinfo *src;
	s4       reg;

	src = VAROP(iptr->sx.s23.s2);

	reg = emit_load_low(jd, iptr, src, tempreg);

	return reg;
}
#endif


/* emit_load_s3_low ************************************************************

   Emits a possible load of the low 32-bits of the third long source
   operand.

*******************************************************************************/

#if SIZEOF_VOID_P == 4
s4 emit_load_s3_low(jitdata *jd, instruction *iptr, s4 tempreg)
{
	varinfo *src;
	s4       reg;

	src = VAROP(iptr->sx.s23.s3);

	reg = emit_load_low(jd, iptr, src, tempreg);

	return reg;
}
#endif


/* emit_load_s1_high ***********************************************************

   Emits a possible load of the high 32-bits of the first long source
   operand.

*******************************************************************************/

#if SIZEOF_VOID_P == 4
s4 emit_load_s1_high(jitdata *jd, instruction *iptr, s4 tempreg)
{
	varinfo *src;
	s4       reg;

	src = VAROP(iptr->s1);

	reg = emit_load_high(jd, iptr, src, tempreg);

	return reg;
}
#endif


/* emit_load_s2_high ***********************************************************

   Emits a possible load of the high 32-bits of the second long source
   operand.

*******************************************************************************/

#if SIZEOF_VOID_P == 4
s4 emit_load_s2_high(jitdata *jd, instruction *iptr, s4 tempreg)
{
	varinfo *src;
	s4       reg;

	src = VAROP(iptr->sx.s23.s2);

	reg = emit_load_high(jd, iptr, src, tempreg);

	return reg;
}
#endif


/* emit_load_s3_high ***********************************************************

   Emits a possible load of the high 32-bits of the third long source
   operand.

*******************************************************************************/

#if SIZEOF_VOID_P == 4
s4 emit_load_s3_high(jitdata *jd, instruction *iptr, s4 tempreg)
{
	varinfo *src;
	s4       reg;

	src = VAROP(iptr->sx.s23.s3);

	reg = emit_load_high(jd, iptr, src, tempreg);

	return reg;
}
#endif


/* emit_store_dst **************************************************************

   This function generates the code to store the result of an
   operation back into a spilled pseudo-variable.  If the
   pseudo-variable has not been spilled in the first place, this
   function will generate nothing.
    
*******************************************************************************/

void emit_store_dst(jitdata *jd, instruction *iptr, s4 d)
{
	emit_store(jd, iptr, VAROP(iptr->dst), d);
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
 * vim:noexpandtab:sw=4:ts=4:
 */
