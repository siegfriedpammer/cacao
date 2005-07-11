/* src/vm/builtintable.inc - tables of builtin functions

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

   Authors: Christian Thalinger

   Changes:

   $Id: builtintable.inc 2982 2005-07-11 11:14:17Z twisti $

*/


#include "config.h"
#include "arch.h"
#include "vm/builtin.h"
#include "vm/jit/jit.h"


/* internal and not automatically replaced functions **************************/

static builtintable_entry builtintable_internal[] = {

#if defined(__ALPHA__)
	{
		ICMD_F2L,
		BUILTIN_f2l,
		"(F)J",
		"f2l",
		NULL
	},
	{
		ICMD_D2L,
		BUILTIN_d2l,
		"(D)J",
		"d2l",
		NULL
	},
	{
		ICMD_F2I,
		BUILTIN_f2i,
		"(F)I",
		"f2i",
		NULL
	},
	{
		ICMD_D2I,
		BUILTIN_d2i,
		"(D)I",
		"d2i",
		NULL
	},
#endif

#if !SUPPORT_DIVISION
	{
		ICMD_IDIV,
		BUILTIN_idiv,
		"(II)I",
		"idiv",
		NULL
	},
	{
		ICMD_IREM,
		BUILTIN_irem,
		"(II)I",
		"irem",
		NULL
	},
#endif

#if !(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_DIV)
	{
		ICMD_LDIV,
		BUILTIN_ldiv,
		"(JJ)J",
		"ldiv",
		NULL
	},
	{
		ICMD_LREM,
		BUILTIN_lrem,
		"(JJ)J",
		"lrem",
		NULL
	},
#endif

	{
		ICMD_FREM,
		BUILTIN_frem,
		"(FF)F",
		"frem",
		NULL
	},
	{
		ICMD_DREM,
		BUILTIN_drem,
		"(DD)D",
		"drem",
		NULL
	},


	/* internal functions *****************************************************/

	{
		ICMD_NEW,
		BUILTIN_new,
		"(Ljava/lang/Class;)Ljava/lang/Object;",
		"new",
		NULL
	},
	{
		ICMD_ANEWARRAY,
		BUILTIN_newarray,
        "(ILjava/lang/Class;)[Ljava/lang/Object;",
		"newarray",
		NULL
	},
	{
		ICMD_NEWARRAY,
		BUILTIN_newarray_boolean,
		"(I)[Z",
		"newarray_boolean",
		NULL
	},
	{
		ICMD_NEWARRAY,
		BUILTIN_newarray_byte,
		"(I)[B",
		"newarray_byte",
		NULL
	},
	{
		ICMD_NEWARRAY,
		BUILTIN_newarray_char,
		"(I)[C",
		"newarray_char",
		NULL
	},
	{
		ICMD_NEWARRAY,
		BUILTIN_newarray_short,
		"(I)[S",
		"newarray_short",
		NULL
	},
	{
		ICMD_NEWARRAY,
		BUILTIN_newarray_int,
		"(I)[I",
		"newarray_int",
		NULL
	},
	{
		ICMD_NEWARRAY,
		BUILTIN_newarray_long,
		"(I)[J",
		"newarray_long",
		NULL
	},
	{
		ICMD_NEWARRAY,
		BUILTIN_newarray_float,
		"(I)[F",
		"newarray_float",
		NULL
	},
	{
		ICMD_NEWARRAY,
		BUILTIN_newarray_double,
		"(I)[D",
		"newarray_double",
		NULL
	},
	{
		ICMD_CHECKCAST,
		BUILTIN_arraycheckcast,
		"([Ljava/lang/Object;[Ljava/lang/Object;)[Ljava/lang/Object;",
		"arraycheckcast",
		NULL
	},
	{
		ICMD_INSTANCEOF,
		BUILTIN_arrayinstanceof,
		"([Ljava/lang/Object;[Ljava/lang/Object;)I",
		"arrayinstanceof",
		NULL
	},
#if !defined(__POWERPC__) && !defined(__X86_64__) && !defined(__I386__) && !defined(__ALPHA__)
	{
		ICMD_AASTORE,
		BUILTIN_aastore,
		"([Ljava/lang/Object;ILjava/lang/Object;)V",
		"aastore",
		NULL
	},
#endif
	{
		ICMD_AASTORE,
		BUILTIN_canstore,
		"([Ljava/lang/Object;Ljava/lang/Object;)I",
		"canstore",
		NULL
	},

#if defined(USE_THREADS)
	{
		ICMD_MONITORENTER,
		BUILTIN_monitorenter,
		"(Ljava/lang/Object;)V",
		"monitorenter",
		NULL
	},
	{
		ICMD_MONITOREXIT,
		BUILTIN_monitorexit,
		"(Ljava/lang/Object;)V",
		"monitorexit",
		NULL
	},
#endif


	/* assembler code patching functions **************************************/

	{
		ICMD_NEW,
		PATCHER_builtin_new,
		"(Ljava/lang/Class;)Ljava/lang/Object;",
		"new (NOT RESOLVED)",
		NULL
	},
	{
		ICMD_ANEWARRAY,
		PATCHER_builtin_newarray,
		"(ILjava/lang/Class;)[Ljava/lang/Object;",
		"newarray (NOT RESOLVED)",
		NULL
	},
	{
		ICMD_CHECKCAST,
		PATCHER_builtin_arraycheckcast,
		"([Ljava/lang/Class;[Ljava/lang/Class;)[Ljava/lang/Object;",
		"arraycheckcast (NOT RESOLVED)",
		NULL
	},
	{
		ICMD_INSTANCEOF,
		PATCHER_builtin_arrayinstanceof,
		"([Ljava/lang/Class;[Ljava/lang/Class;)I",
		"arrayinstanceof (NOT RESOLVED)",
		NULL
	},
};


/* automatically replaced functions *******************************************/

static builtintable_entry builtintable_automatic[] = {

#if !(SUPPORT_LONG && SUPPORT_LONG_CMP)
	{
		ICMD_LCMP,
		BUILTIN_lcmp,
		"(JJ)I",
		"lcmp",
		NULL
	},
#endif

#if !(SUPPORT_LONG && SUPPORT_LONG_LOGICAL)
	{
		ICMD_LAND,
		BUILTIN_land,
		"(JJ)J",
		"land",
		NULL
	},
	{
		ICMD_LOR,
		BUILTIN_lor,
		"(JJ)J",
		"lor",
		NULL
	},
	{
		ICMD_LXOR,
		BUILTIN_lxor,
		"(JJ)J",
		"lxor",
		NULL
	},
#endif /* !(SUPPORT_LONG && SUPPORT_LONG_LOGICAL) */

#if !(SUPPORT_LONG && SUPPORT_LONG_SHIFT)
	{
		ICMD_LSHL,
		BUILTIN_lshl,
		"(JI)J",
		"lshl",
		NULL
	},
	{
		ICMD_LSHR,
		BUILTIN_lshr,
		"(JI)J",
		"lshr",
		NULL
	},
	{
		ICMD_LUSHR,
		BUILTIN_lushr,
		"(JI)J",
		"lushr",
		NULL
	},
#endif /* !(SUPPORT_LONG && SUPPORT_LONG_SHIFT) */

#if !(SUPPORT_LONG && SUPPORT_LONG_ADD)
	{
		ICMD_LADD,
		BUILTIN_ladd,
		"(JJ)J",
		"ladd",
		NULL
	},
	{
		ICMD_LSUB,
		BUILTIN_lsub,
		"(JJ)J",
		"lsub",
		NULL
	},
	{
		ICMD_LNEG,
		BUILTIN_lneg,
		"(J)J",
		"lneg",
		NULL
	},
#endif /* !(SUPPORT_LONG && SUPPORT_LONG_ADD) */

#if !(SUPPORT_LONG && SUPPORT_LONG_MUL)
	{
		ICMD_LMUL,
		BUILTIN_lmul,
		"(JJ)J",
		"lmul",
		NULL
	},
#endif

#if !(SUPPORT_FLOAT && SUPPORT_IFCVT)
	{
		ICMD_I2F,
		BUILTIN_i2f,
		"(I)F",
		"i2f",
		NULL
	},
#endif

#if !(SUPPORT_DOUBLE && SUPPORT_IFCVT)
	{
		ICMD_I2D,
		BUILTIN_i2d,
		"(I)D",
		"i2d",
		NULL
	},
#endif

#if !(SUPPORT_FLOAT && SUPPORT_FICVT)
	{
		ICMD_F2I,
		BUILTIN_f2i,
		"(F)I",
		"f2i",
		NULL
	},
#endif

#if !(SUPPORT_DOUBLE && SUPPORT_FICVT)
	{
		ICMD_D2I,
		BUILTIN_d2i,
		"(D)I",
		"d2i",
		NULL
	},
#endif

#if !(SUPPORT_LONG && SUPPORT_FLOAT && SUPPORT_LONG_FCVT)
	{
		ICMD_L2F,
		BUILTIN_l2f,
		"(J)F",
		"l2f",
		NULL
	},
#endif

#if !(SUPPORT_LONG && SUPPORT_DOUBLE && SUPPORT_LONG_FCVT)
	{
		ICMD_L2D,
		BUILTIN_l2d,
		"(J)D",
		"l2d",
		NULL
	},
#endif

#if !(SUPPORT_FLOAT && SUPPORT_LONG && SUPPORT_LONG_ICVT)
	{
		ICMD_F2L,
		BUILTIN_f2l,
		"(F)J",
		"f2l",
		NULL
	},
#endif

#if !(SUPPORT_DOUBLE && SUPPORT_LONG && SUPPORT_LONG_ICVT)
	{
		ICMD_D2L,
		BUILTIN_d2l,
		"(D)J",
		"d2l",
		NULL
	},
#endif


	/* float functions ********************************************************/

#if !SUPPORT_FLOAT
	{
		ICMD_FADD,
		BUILTIN_fadd,
		"(FF)F",
		"fadd",
		NULL
	},
	{
		ICMD_FSUB,
		BUILTIN_fsub,
		"(FF)F",
		"fsub",
		NULL
	},
	{
		ICMD_FMUL,
		BUILTIN_fmul,
		"(FF)F",
		"fmul",
		NULL
	},
	{
		ICMD_FDIV,
		BUILTIN_fdiv,
		"(FF)F",
		"fdiv",
		NULL
	},
	{
		ICMD_FNEG,
		BUILTIN_fneg,
		"(F)F",
		"fneg",
		NULL
	},
	{
		ICMD_FCMPL,
		BUILTIN_fcmpl,
		"(FF)I",
		"fcmpl",
		NULL
	},
	{
		ICMD_FCMPG,
		BUILTIN_fcmpg,
		"(FF)I",
		"fcmpg",
		NULL
	},
#endif /* !SUPPORT_FLOAT */


	/* double functions *******************************************************/

#if !SUPPORT_DOUBLE
	{
		ICMD_DADD,
		BUILTIN_dadd,
		"(DD)D",
		"dadd",
		NULL
	},
	{
		ICMD_DSUB,
		BUILTIN_dsub,
		"(DD)D",
		"dsub",
		NULL
	},
	{
		ICMD_DMUL,
		BUILTIN_dmul,
		"(DD)D",
		"dmul",
		NULL
	},
	{
		ICMD_DDIV,
		BUILTIN_ddiv,
		"(DD)D",
		"ddiv",
		NULL
	},
	{
		ICMD_DNEG,
		BUILTIN_dneg,
		"(D)D",
		"dneg",
		NULL
	},
	{
		ICMD_DCMPL,
		BUILTIN_dcmpl,
		"(DD)I",
		"dcmpl",
		NULL
	},
	{
		ICMD_DCMPG,
		BUILTIN_dcmpg,
		"(DD)I",
		"dcmpg",
		NULL
	},
#endif /* !SUPPORT_DOUBLE */


	/* float/double functions *************************************************/

#if !(SUPPORT_FLOAT && SUPPORT_DOUBLE)
	{
		ICMD_F2D,
		BUILTIN_f2d,
		"(F)D",
		"f2d",
		NULL
	},
	{
		ICMD_D2F,
		BUILTIN_d2f,
		"(D)F",
		"d2f",
		NULL
	},
#endif /* !(SUPPORT_FLOAT && SUPPORT_DOUBLE) */


	/* prevent a compiler warning if everything is supported (e.g. i386) */

	{
		255,
		NULL,
		NULL,
		NULL,
		NULL
	},
};


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
