/* src/vm/builtin.c - functions for unsupported operations

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

   Authors: Reinhard Grafl
            Andreas Krall
            Mark Probst

   Changes: Christian Thalinger

   Contains C functions for JavaVM Instructions that cannot be
   translated to machine language directly. Consequently, the
   generated machine code for these instructions contains function
   calls instead of machine instructions, using the C calling
   convention.

   $Id: builtin.c 2269 2005-04-11 10:50:12Z twisti $

*/


#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "arch.h"
#include "types.h"
#include "mm/boehm.h"
#include "mm/memory.h"
#include "native/native.h"
#include "native/include/java_lang_Cloneable.h"
#include "native/include/java_lang_VMObject.h"

#if defined(USE_THREADS)
# if defined(NATIVE_THREADS)
#  include "threads/native/threads.h"
# else
#  include "threads/green/threads.h"
#  include "threads/green/locks.h"
# endif
#endif

#include "toolbox/logging.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/initialize.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/tables.h"
#include "vm/jit/asmpart.h"


#undef DEBUG /*define DEBUG 1*/

THREADSPECIFIC methodinfo* _threadrootmethod = NULL;
THREADSPECIFIC void *_thread_nativestackframeinfo = NULL;


#if defined(USEBUILTINTABLE)

#if 0
stdopdescriptor builtintable[] = {
	{ ICMD_LCMP,   TYPE_LONG, TYPE_LONG, TYPE_INT, ICMD_BUILTIN2,
	  (functionptr) builtin_lcmp , SUPPORT_LONG && SUPPORT_LONG_CMP, false },
	{ ICMD_LAND,   TYPE_LONG, TYPE_LONG, TYPE_LONG, ICMD_BUILTIN2,
	  (functionptr) builtin_land , SUPPORT_LONG && SUPPORT_LONG_LOGICAL, false },
	{ ICMD_LOR,    TYPE_LONG, TYPE_LONG, TYPE_LONG, ICMD_BUILTIN2,
	  (functionptr) builtin_lor , SUPPORT_LONG && SUPPORT_LONG_LOGICAL, false },
	{ ICMD_LXOR,   TYPE_LONG, TYPE_LONG, TYPE_LONG, ICMD_BUILTIN2,
	  (functionptr) builtin_lxor , SUPPORT_LONG && SUPPORT_LONG_LOGICAL, false },
	{ ICMD_LSHL,   TYPE_LONG, TYPE_INT,  TYPE_LONG, ICMD_BUILTIN2,
	  (functionptr) builtin_lshl , SUPPORT_LONG && SUPPORT_LONG_SHIFT, false },
	{ ICMD_LSHR,   TYPE_LONG, TYPE_INT,  TYPE_LONG, ICMD_BUILTIN2,
	  (functionptr) builtin_lshr, SUPPORT_LONG && SUPPORT_LONG_SHIFT, false },
	{ ICMD_LUSHR,  TYPE_LONG, TYPE_INT,  TYPE_LONG, ICMD_BUILTIN2,
	  (functionptr) builtin_lushr, SUPPORT_LONG && SUPPORT_LONG_SHIFT, false },
	{ ICMD_LADD,   TYPE_LONG, TYPE_LONG, TYPE_LONG, ICMD_BUILTIN2,
	  (functionptr) builtin_ladd , SUPPORT_LONG && SUPPORT_LONG_ADD, false },
	{ ICMD_LSUB,   TYPE_LONG, TYPE_LONG, TYPE_LONG, ICMD_BUILTIN2,
	  (functionptr) builtin_lsub , SUPPORT_LONG && SUPPORT_LONG_ADD, false },
	{ ICMD_LNEG,   TYPE_LONG, TYPE_VOID, TYPE_LONG, ICMD_BUILTIN1, 
	  (functionptr) builtin_lneg, SUPPORT_LONG && SUPPORT_LONG_ADD, true },
	{ ICMD_LMUL,   TYPE_LONG, TYPE_LONG, TYPE_LONG, ICMD_BUILTIN2,
	  (functionptr) builtin_lmul , SUPPORT_LONG && SUPPORT_LONG_MUL, false },
	{ ICMD_I2F,    TYPE_INT, TYPE_VOID, TYPE_FLOAT, ICMD_BUILTIN1,
	  (functionptr) builtin_i2f, SUPPORT_FLOAT && SUPPORT_IFCVT, true },
	{ ICMD_I2D,    TYPE_INT, TYPE_VOID, TYPE_DOUBLE, ICMD_BUILTIN1, 
	  (functionptr) builtin_i2d, SUPPORT_DOUBLE && SUPPORT_IFCVT, true },
	{ ICMD_L2F,    TYPE_LONG, TYPE_VOID, TYPE_FLOAT, ICMD_BUILTIN1,
	  (functionptr) builtin_l2f, SUPPORT_LONG && SUPPORT_FLOAT && SUPPORT_LONG_FCVT, true },
	{ ICMD_L2D,    TYPE_LONG, TYPE_VOID, TYPE_DOUBLE, ICMD_BUILTIN1, 
	  (functionptr) builtin_l2d, SUPPORT_LONG && SUPPORT_DOUBLE && SUPPORT_LONG_FCVT, true },
	{ ICMD_F2L,    TYPE_FLOAT, TYPE_VOID, TYPE_LONG, ICMD_BUILTIN1,
	  (functionptr) builtin_f2l, SUPPORT_FLOAT && SUPPORT_LONG && SUPPORT_LONG_ICVT, true },
	{ ICMD_D2L,    TYPE_DOUBLE, TYPE_VOID, TYPE_LONG, ICMD_BUILTIN1,
	  (functionptr) builtin_d2l, SUPPORT_DOUBLE && SUPPORT_LONG && SUPPORT_LONG_ICVT, true },
	{ ICMD_F2I,    TYPE_FLOAT, TYPE_VOID, TYPE_INT, ICMD_BUILTIN1,
	  (functionptr) builtin_f2i, SUPPORT_FLOAT && SUPPORT_FICVT, true },
	{ ICMD_D2I,    TYPE_DOUBLE, TYPE_VOID, TYPE_INT, ICMD_BUILTIN1,
	  (functionptr) builtin_d2i, SUPPORT_DOUBLE && SUPPORT_FICVT, true },
  	{ 255, 0, 0, 0, 0, NULL, true, false },
};

#endif

static int builtintablelen;

#endif /* USEBUILTINTABLE */


/*****************************************************************************
						 TABLE OF BUILTIN FUNCTIONS

    This table lists the builtin functions which are used inside
    BUILTIN* opcodes.

    The first part of the table (up to the 255-marker) lists the
    opcodes which are automatically replaced in stack.c.

    The second part lists the builtin functions which are "manually"
    used for BUILTIN* opcodes in parse.c and stack.c.

*****************************************************************************/

builtin_descriptor builtin_desc[] = {
#if defined(USEBUILTINTABLE)
	{ICMD_LCMP , BUILTIN_lcmp ,ICMD_BUILTIN2,TYPE_LONG  ,TYPE_LONG  ,TYPE_VOID ,TYPE_INT   ,
	             SUPPORT_LONG && SUPPORT_LONG_CMP,false,"lcmp"},
	
	{ICMD_LAND , BUILTIN_land ,ICMD_BUILTIN2,TYPE_LONG  ,TYPE_LONG  ,TYPE_VOID ,TYPE_LONG  ,
	             SUPPORT_LONG && SUPPORT_LONG_LOGICAL,false,"land"},
	{ICMD_LOR  , BUILTIN_lor  ,ICMD_BUILTIN2,TYPE_LONG  ,TYPE_LONG  ,TYPE_VOID ,TYPE_LONG  ,
	             SUPPORT_LONG && SUPPORT_LONG_LOGICAL,false,"lor"},
	{ICMD_LXOR , BUILTIN_lxor ,ICMD_BUILTIN2,TYPE_LONG  ,TYPE_LONG  ,TYPE_VOID ,TYPE_LONG  ,
	             SUPPORT_LONG && SUPPORT_LONG_LOGICAL,false,"lxor"},
	
	{ICMD_LSHL , BUILTIN_lshl ,ICMD_BUILTIN2,TYPE_LONG  ,TYPE_INT   ,TYPE_VOID ,TYPE_LONG  ,
	             SUPPORT_LONG && SUPPORT_LONG_SHIFT,false,"lshl"},
	{ICMD_LSHR , BUILTIN_lshr ,ICMD_BUILTIN2,TYPE_LONG  ,TYPE_INT   ,TYPE_VOID ,TYPE_LONG  ,
	             SUPPORT_LONG && SUPPORT_LONG_SHIFT,false,"lshr"},
	{ICMD_LUSHR, BUILTIN_lushr,ICMD_BUILTIN2,TYPE_LONG  ,TYPE_INT   ,TYPE_VOID ,TYPE_LONG  ,
	             SUPPORT_LONG && SUPPORT_LONG_SHIFT,false,"lushr"},
	
	{ICMD_LADD , BUILTIN_ladd ,ICMD_BUILTIN2,TYPE_LONG  ,TYPE_LONG  ,TYPE_VOID ,TYPE_LONG  ,
	             SUPPORT_LONG && SUPPORT_LONG_ADD,false,"ladd"},
	{ICMD_LSUB , BUILTIN_lsub ,ICMD_BUILTIN2,TYPE_LONG  ,TYPE_LONG  ,TYPE_VOID ,TYPE_LONG  ,
	             SUPPORT_LONG && SUPPORT_LONG_ADD,false,"lsub"},
	{ICMD_LNEG , BUILTIN_lneg ,ICMD_BUILTIN1,TYPE_LONG  ,TYPE_VOID  ,TYPE_VOID ,TYPE_LONG  ,
	             SUPPORT_LONG && SUPPORT_LONG_ADD,false,"lneg"},
	{ICMD_LMUL , BUILTIN_lmul ,ICMD_BUILTIN2,TYPE_LONG  ,TYPE_LONG  ,TYPE_VOID ,TYPE_LONG  ,
	             SUPPORT_LONG && SUPPORT_LONG_MUL,false,"lmul"},
	
	{ICMD_I2F  , BUILTIN_i2f  ,ICMD_BUILTIN1,TYPE_INT   ,TYPE_VOID  ,TYPE_VOID ,TYPE_FLOAT ,
	             SUPPORT_FLOAT && SUPPORT_IFCVT,true ,"i2f"},
	{ICMD_I2D  , BUILTIN_i2d  ,ICMD_BUILTIN1,TYPE_INT   ,TYPE_VOID  ,TYPE_VOID ,TYPE_DOUBLE,
	             SUPPORT_DOUBLE && SUPPORT_IFCVT,true ,"i2d"},
	{ICMD_L2F  , BUILTIN_l2f  ,ICMD_BUILTIN1,TYPE_LONG  ,TYPE_VOID  ,TYPE_VOID ,TYPE_FLOAT ,
	             SUPPORT_LONG && SUPPORT_FLOAT && SUPPORT_LONG_FCVT,true ,"l2f"},
	{ICMD_L2D  , BUILTIN_l2d  ,ICMD_BUILTIN1,TYPE_LONG  ,TYPE_VOID  ,TYPE_VOID ,TYPE_DOUBLE,
	             SUPPORT_LONG && SUPPORT_DOUBLE && SUPPORT_LONG_FCVT,true ,"l2d"},
	{ICMD_F2L  , BUILTIN_f2l  ,ICMD_BUILTIN1,TYPE_FLOAT ,TYPE_VOID  ,TYPE_VOID ,TYPE_LONG  ,
	             SUPPORT_FLOAT && SUPPORT_LONG && SUPPORT_LONG_ICVT,true ,"f2l"},
	{ICMD_D2L  , BUILTIN_d2l  ,ICMD_BUILTIN1,TYPE_DOUBLE,TYPE_VOID  ,TYPE_VOID ,TYPE_LONG  ,
	             SUPPORT_DOUBLE && SUPPORT_LONG && SUPPORT_LONG_ICVT,true ,"d2l"},
	{ICMD_F2I  , BUILTIN_f2i  ,ICMD_BUILTIN1,TYPE_FLOAT ,TYPE_VOID  ,TYPE_VOID ,TYPE_INT   ,
	             SUPPORT_FLOAT && SUPPORT_FICVT,true ,"f2i"},
	{ICMD_D2I  , BUILTIN_d2i  ,ICMD_BUILTIN1,TYPE_DOUBLE,TYPE_VOID  ,TYPE_VOID ,TYPE_INT   ,
	             SUPPORT_DOUBLE && SUPPORT_FICVT,true ,"d2i"},

	{ ICMD_FADD , BUILTIN_fadd  , ICMD_BUILTIN2, TYPE_FLT, TYPE_FLT  , TYPE_VOID , TYPE_FLT, SUPPORT_FLOAT, true, "fadd"  },
	{ ICMD_FSUB , BUILTIN_fsub  , ICMD_BUILTIN2, TYPE_FLT, TYPE_FLT  , TYPE_VOID , TYPE_FLT, SUPPORT_FLOAT, true, "fsub"  },
	{ ICMD_FMUL , BUILTIN_fmul  , ICMD_BUILTIN2, TYPE_FLT, TYPE_FLT  , TYPE_VOID , TYPE_FLT, SUPPORT_FLOAT, true, "fmul"  },
	{ ICMD_FDIV , BUILTIN_fdiv  , ICMD_BUILTIN2, TYPE_FLT, TYPE_FLT  , TYPE_VOID , TYPE_FLT, SUPPORT_FLOAT, true, "fdiv"  },
	{ ICMD_FNEG , BUILTIN_fneg  , ICMD_BUILTIN1, TYPE_FLT, TYPE_VOID , TYPE_VOID , TYPE_FLT, SUPPORT_FLOAT, true, "fneg"  },
	{ ICMD_FCMPL, BUILTIN_fcmpl , ICMD_BUILTIN2, TYPE_FLT, TYPE_FLT  , TYPE_VOID , TYPE_INT, SUPPORT_FLOAT, true, "fcmpl" },
	{ ICMD_FCMPG, BUILTIN_fcmpg , ICMD_BUILTIN2, TYPE_FLT, TYPE_FLT  , TYPE_VOID , TYPE_INT, SUPPORT_FLOAT, true, "fcmpg" },

	{ ICMD_DADD , BUILTIN_dadd  , ICMD_BUILTIN2, TYPE_DBL, TYPE_DBL  , TYPE_VOID , TYPE_DBL, SUPPORT_DOUBLE, true, "dadd"  },
	{ ICMD_DSUB , BUILTIN_dsub  , ICMD_BUILTIN2, TYPE_DBL, TYPE_DBL  , TYPE_VOID , TYPE_DBL, SUPPORT_DOUBLE, true, "dsub"  },
	{ ICMD_DMUL , BUILTIN_dmul  , ICMD_BUILTIN2, TYPE_DBL, TYPE_DBL  , TYPE_VOID , TYPE_DBL, SUPPORT_DOUBLE, true, "dmul"  },
	{ ICMD_DDIV , BUILTIN_ddiv  , ICMD_BUILTIN2, TYPE_DBL, TYPE_DBL  , TYPE_VOID , TYPE_DBL, SUPPORT_DOUBLE, true, "ddiv"  },
	{ ICMD_DNEG , BUILTIN_dneg  , ICMD_BUILTIN1, TYPE_DBL, TYPE_VOID , TYPE_VOID , TYPE_DBL, SUPPORT_DOUBLE, true, "dneg"  },
	{ ICMD_DCMPL, BUILTIN_dcmpl , ICMD_BUILTIN2, TYPE_DBL, TYPE_DBL  , TYPE_VOID , TYPE_INT, SUPPORT_DOUBLE, true, "dcmpl" },
	{ ICMD_DCMPG, BUILTIN_dcmpg , ICMD_BUILTIN2, TYPE_DBL, TYPE_DBL  , TYPE_VOID , TYPE_INT, SUPPORT_DOUBLE, true, "dcmpg" },

	{ ICMD_F2D,  BUILTIN_f2d  , ICMD_BUILTIN1, TYPE_FLT, TYPE_VOID , TYPE_VOID , TYPE_DBL, SUPPORT_FLOAT && SUPPORT_DOUBLE, true, "f2d" },
	{ ICMD_D2F,  BUILTIN_d2f  , ICMD_BUILTIN1, TYPE_DBL, TYPE_VOID , TYPE_VOID , TYPE_FLT, SUPPORT_FLOAT && SUPPORT_DOUBLE, true, "d2f" },
#endif

	/* this record marks the end of the automatically replaced opcodes */
	{255,NULL,0,0,0,0,0,0,0,"<INVALID>"},

	/* the following functions are not replaced automatically */
	
#if defined(__ALPHA__)
	{255, BUILTIN_f2l  ,ICMD_BUILTIN1,TYPE_FLOAT ,TYPE_VOID  ,TYPE_VOID ,TYPE_LONG  ,0,0,"f2l"},
	{255, BUILTIN_d2l  ,ICMD_BUILTIN1,TYPE_DOUBLE,TYPE_VOID  ,TYPE_VOID ,TYPE_LONG  ,0,0,"d2l"},
	{255, BUILTIN_f2i  ,ICMD_BUILTIN1,TYPE_FLOAT ,TYPE_VOID  ,TYPE_VOID ,TYPE_INT   ,0,0,"f2i"},
	{255, BUILTIN_d2i  ,ICMD_BUILTIN1,TYPE_DOUBLE,TYPE_VOID  ,TYPE_VOID ,TYPE_INT   ,0,0,"d2i"},
#endif

	{255,BUILTIN_instanceof      ,ICMD_BUILTIN2,TYPE_ADR   ,TYPE_ADR   ,TYPE_VOID  ,TYPE_INT   ,0,0,"instanceof"},
	{255,BUILTIN_arrayinstanceof ,ICMD_BUILTIN2,TYPE_ADR   ,TYPE_ADR   ,TYPE_VOID  ,TYPE_INT   ,0,0,"arrayinstanceof"},
	{255,BUILTIN_checkarraycast  ,ICMD_BUILTIN2,TYPE_ADR   ,TYPE_ADR   ,TYPE_VOID  ,TYPE_VOID  ,0,0,"checkarraycast"},
	{255,BUILTIN_aastore         ,ICMD_BUILTIN3,TYPE_ADR   ,TYPE_INT   ,TYPE_ADR   ,TYPE_VOID  ,0,0,"aastore"},
	{255,BUILTIN_new             ,ICMD_BUILTIN1,TYPE_ADR   ,TYPE_VOID  ,TYPE_VOID  ,TYPE_ADR   ,0,0,"new"},
	{255,BUILTIN_newarray        ,ICMD_BUILTIN2,TYPE_INT   ,TYPE_ADR   ,TYPE_VOID  ,TYPE_ADR   ,0,0,"newarray"},
	{255,BUILTIN_newarray_boolean,ICMD_BUILTIN1,TYPE_INT   ,TYPE_VOID  ,TYPE_VOID  ,TYPE_ADR   ,0,0,"newarray_boolean"},
	{255,BUILTIN_newarray_char   ,ICMD_BUILTIN1,TYPE_INT   ,TYPE_VOID  ,TYPE_VOID  ,TYPE_ADR   ,0,0,"newarray_char"},
	{255,BUILTIN_newarray_float  ,ICMD_BUILTIN1,TYPE_INT   ,TYPE_VOID  ,TYPE_VOID  ,TYPE_ADR   ,0,0,"newarray_float"},
	{255,BUILTIN_newarray_double ,ICMD_BUILTIN1,TYPE_INT   ,TYPE_VOID  ,TYPE_VOID  ,TYPE_ADR   ,0,0,"newarray_double"},
	{255,BUILTIN_newarray_byte   ,ICMD_BUILTIN1,TYPE_INT   ,TYPE_VOID  ,TYPE_VOID  ,TYPE_ADR   ,0,0,"newarray_byte"},
	{255,BUILTIN_newarray_short  ,ICMD_BUILTIN1,TYPE_INT   ,TYPE_VOID  ,TYPE_VOID  ,TYPE_ADR   ,0,0,"newarray_short"},
	{255,BUILTIN_newarray_int    ,ICMD_BUILTIN1,TYPE_INT   ,TYPE_VOID  ,TYPE_VOID  ,TYPE_ADR   ,0,0,"newarray_int"},
	{255,BUILTIN_newarray_long   ,ICMD_BUILTIN1,TYPE_INT   ,TYPE_VOID  ,TYPE_VOID  ,TYPE_ADR   ,0,0,"newarray_long"},
	{255,BUILTIN_anewarray       ,ICMD_BUILTIN2,TYPE_INT   ,TYPE_ADR   ,TYPE_VOID  ,TYPE_ADR   ,0,0,"anewarray"},
#if defined(USE_THREADS)
	{255,BUILTIN_monitorenter    ,ICMD_BUILTIN1,TYPE_ADR   ,TYPE_VOID  ,TYPE_VOID  ,TYPE_VOID  ,0,0,"monitorenter"},
	{255,BUILTIN_monitorexit     ,ICMD_BUILTIN1,TYPE_ADR   ,TYPE_VOID  ,TYPE_VOID  ,TYPE_VOID  ,0,0,"monitorexit"},
#endif
#if !SUPPORT_DIVISION
	{255,BUILTIN_idiv            ,ICMD_BUILTIN2,TYPE_INT   ,TYPE_INT   ,TYPE_VOID  ,TYPE_INT   ,0,0,"idiv"},
	{255,BUILTIN_irem            ,ICMD_BUILTIN2,TYPE_INT   ,TYPE_INT   ,TYPE_VOID  ,TYPE_INT   ,0,0,"irem"},
#endif
#if !(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_DIV)
	{255,BUILTIN_ldiv            ,ICMD_BUILTIN2,TYPE_LONG  ,TYPE_LONG  ,TYPE_VOID  ,TYPE_LONG  ,0,0,"ldiv"},
	{255,BUILTIN_lrem            ,ICMD_BUILTIN2,TYPE_LONG  ,TYPE_LONG  ,TYPE_VOID  ,TYPE_LONG  ,0,0,"lrem"},
#endif
	{255,BUILTIN_frem            ,ICMD_BUILTIN2,TYPE_FLOAT ,TYPE_FLOAT ,TYPE_VOID  ,TYPE_FLOAT ,0,0,"frem"},
	{255,BUILTIN_drem            ,ICMD_BUILTIN2,TYPE_DOUBLE,TYPE_DOUBLE,TYPE_VOID  ,TYPE_DOUBLE,0,0,"drem"},


#if defined(__X86_64__) || defined(__I386__)
	/* assembler code patching functions */

	{ 255, asm_builtin_new       , ICMD_BUILTIN1, TYPE_ADR   , TYPE_VOID  , TYPE_VOID  , TYPE_ADR   , 0, 0, "new (calling asm_builtin_new)" },
	{ 255, asm_builtin_newarray  , ICMD_BUILTIN1, TYPE_ADR   , TYPE_VOID  , TYPE_VOID  , TYPE_ADR   , 0, 0, "newarray (calling asm_builtin_newarray)" },
#endif


	/* this record marks the end of the list */

	{   0, NULL, 0, 0, 0, 0, 0, 0, 0, "<END>" }
};


#if defined(USEBUILTINTABLE)

static int stdopcompare(const void *a, const void *b)
{
	builtin_descriptor *o1 = (builtin_descriptor *) a;
	builtin_descriptor *o2 = (builtin_descriptor *) b;
	if (!o1->supported && o2->supported)
		return -1;
	if (o1->supported && !o2->supported)
		return 1;
	return (o1->opcode < o2->opcode) ? -1 : (o1->opcode > o2->opcode);
}


void sort_builtintable(void)
{
	int len;

	len = 0;
	while (builtin_desc[len].opcode != 255) len++;
	qsort(builtin_desc, len, sizeof(builtin_descriptor), stdopcompare);

	for (--len; len>=0 && builtin_desc[len].supported; len--);
	builtintablelen = ++len;
}


builtin_descriptor *find_builtin(int icmd)
{
	builtin_descriptor *first = builtin_desc;
	builtin_descriptor *last = builtin_desc + builtintablelen;
	int len = last - first;
	int half;
	builtin_descriptor *middle;

	while (len > 0) {
		half = len / 2;
		middle = first + half;
		if (middle->opcode < icmd) {
			first = middle + 1;
			len -= half + 1;
		} else
			len = half;
	}
	return first != last ? first : NULL;
}

#endif /* defined(USEBUILTINTABLE) */


/*****************************************************************************
								TYPE CHECKS
*****************************************************************************/



/*************** internal function: builtin_isanysubclass *********************

	Checks a subclass relation between two classes. Implemented interfaces
	are interpreted as super classes.
	Return value:  1 ... sub is subclass of super
				   0 ... otherwise
					
*****************************************************************************/					
s4 builtin_isanysubclass(classinfo *sub, classinfo *super)
{
	s4 res;
	castinfo classvalues;

	if (sub == super)
		return 1;

	if (super->flags & ACC_INTERFACE)
		return (sub->vftbl->interfacetablelength > super->index) &&
			(sub->vftbl->interfacetable[-super->index] != NULL);

	asm_getclassvalues_atomic(super->vftbl, sub->vftbl, &classvalues);

	res = (u4) (classvalues.sub_baseval - classvalues.super_baseval) <=
		(u4) classvalues.super_diffval;

	return res;
}


s4 builtin_isanysubclass_vftbl(vftbl_t *sub, vftbl_t *super)
{
	s4 res;
	s4 base;
	castinfo classvalues;

	if (sub == super)
		return 1;

	asm_getclassvalues_atomic(super, sub, &classvalues);

	if ((base = classvalues.super_baseval) <= 0)
		/* super is an interface */
		res = (sub->interfacetablelength > -base) &&
			(sub->interfacetable[base] != NULL);
	else
	    res = (u4) (classvalues.sub_baseval - classvalues.super_baseval)
			<= (u4) classvalues.super_diffval;

	return res;
}


/****************** function: builtin_instanceof *****************************

	Checks if an object is an instance of some given class (or subclass of
	that class). If class is an interface, checks if the interface is
	implemented.
	Return value:  1 ... obj is an instance of class or implements the interface
				   0 ... otherwise or if obj == NULL
			 
*****************************************************************************/

/* XXX should use vftbl */
s4 builtin_instanceof(java_objectheader *obj, classinfo *class)
{
#ifdef DEBUG
	log_text ("builtin_instanceof called");
#endif	
	if (!obj)
		return 0;

	return builtin_isanysubclass(obj->vftbl->class, class);
}



/**************** function: builtin_checkcast *******************************

	The same as builtin_instanceof except that 1 is returned when
	obj == NULL
			  
****************************************************************************/

/* XXX should use vftbl */
s4 builtin_checkcast(java_objectheader *obj, classinfo *class)
{
#ifdef DEBUG
	log_text("builtin_checkcast called");
#endif

	if (obj == NULL)
		return 1;
	if (builtin_isanysubclass(obj->vftbl->class, class))
		return 1;

#if DEBUG
	printf("#### checkcast failed ");
	utf_display(obj->vftbl->class->name);
	printf(" -> ");
	utf_display(class->name);
	printf("\n");
#endif

	return 0;
}


/*********** internal function: builtin_descriptorscompatible ******************

	Checks if two array type descriptors are assignment compatible
	Return value:  1 ... target = desc is possible
				   0 ... otherwise
			
******************************************************************************/

static s4 builtin_descriptorscompatible(arraydescriptor *desc,arraydescriptor *target)
{
	if (desc==target) return 1;
	if (desc->arraytype != target->arraytype) return 0;
	if (desc->arraytype != ARRAYTYPE_OBJECT) return 1;
	
	/* {both arrays are arrays of references} */
	if (desc->dimension == target->dimension) {
		/* an array which contains elements of interface types is allowed to be casted to Object (JOWENN)*/
		if ( (desc->elementvftbl->baseval<0) && (target->elementvftbl->baseval==1) ) return 1;
		return builtin_isanysubclass_vftbl(desc->elementvftbl,target->elementvftbl);
	}
	if (desc->dimension < target->dimension) return 0;

	/* {desc has higher dimension than target} */
	return builtin_isanysubclass_vftbl(pseudo_class_Arraystub->vftbl, target->elementvftbl);
}


/******************** function: builtin_checkarraycast ***********************

	Checks if an object is really a subtype of the requested array type.
	The object has to be an array to begin with. For simple arrays (int, short,
	double, etc.) the types have to match exactly.
	For arrays of objects, the type of elements in the array has to be a
	subtype (or the same type) of the requested element type. For arrays of
	arrays (which in turn can again be arrays of arrays), the types at the
	lowest level have to satisfy the corresponding sub class relation.
	
	Return value:  1 ... cast is possible
				   0 ... otherwise
	
	ATTENTION: a cast with a NULL pointer is always possible.
			
*****************************************************************************/

s4 builtin_checkarraycast(java_objectheader *o, vftbl_t *target)
{
	arraydescriptor *desc;
	
	if (!o) return 1;
	if ((desc = o->vftbl->arraydesc) == NULL) return 0;

	return builtin_descriptorscompatible(desc, target->arraydesc);
}


s4 builtin_arrayinstanceof(java_objectheader *obj, vftbl_t *target)
{
	if (!obj) return 1;
	return builtin_checkarraycast(obj, target);
}


/************************** exception functions *******************************

******************************************************************************/

java_objectheader *builtin_throw_exception(java_objectheader *xptr)
{
	if (opt_verbose) {
		char logtext[MAXLOGTEXT];
		sprintf(logtext, "Builtin exception thrown: ");

		if (xptr) {
			java_lang_Throwable *t = (java_lang_Throwable *) xptr;

			utf_sprint_classname(logtext + strlen(logtext),
								 xptr->vftbl->class->name);

			if (t->detailMessage) {
				char *buf;

				buf = javastring_tochar((java_objectheader *) t->detailMessage);
				sprintf(logtext + strlen(logtext), ": %s", buf);
				MFREE(buf, char, strlen(buf));
			}

		} else {
			sprintf(logtext + strlen(logtext), "Error: <Nullpointer instead of exception>");
		}

		log_text(logtext);
	}

	*exceptionptr = xptr;

	return xptr;
}



/******************* function: builtin_canstore *******************************

	Checks, if an object can be stored in an array.
	Return value:  1 ... possible
				   0 ... otherwise

******************************************************************************/

s4 builtin_canstore (java_objectarray *a, java_objectheader *o)
{
	arraydescriptor *desc;
	arraydescriptor *valuedesc;
	vftbl_t *componentvftbl;
	vftbl_t *valuevftbl;
    int dim_m1;
	int base;
	castinfo classvalues;
	
	if (!o) return 1;

	/* The following is guaranteed (by verifier checks):
	 *
	 *     *) a->...vftbl->arraydesc != NULL
	 *     *) a->...vftbl->arraydesc->componentvftbl != NULL
	 *     *) o->vftbl is not an interface vftbl
	 */
	
	desc = a->header.objheader.vftbl->arraydesc;
    componentvftbl = desc->componentvftbl;
	valuevftbl = o->vftbl;

    if ((dim_m1 = desc->dimension - 1) == 0) {
		s4 res;

		/* {a is a one-dimensional array} */
		/* {a is an array of references} */
		
		if (valuevftbl == componentvftbl)
			return 1;

		asm_getclassvalues_atomic(componentvftbl, valuevftbl, &classvalues);

		if ((base = classvalues.super_baseval) <= 0)
			/* an array of interface references */
			return (valuevftbl->interfacetablelength > -base &&
					valuevftbl->interfacetable[base] != NULL);
		
		res = (unsigned) (classvalues.sub_baseval - classvalues.super_baseval)
			<= (unsigned) classvalues.super_diffval;

		return res;
    }
    /* {a has dimension > 1} */
	/* {componentvftbl->arraydesc != NULL} */

	/* check if o is an array */
	if ((valuedesc = valuevftbl->arraydesc) == NULL)
		return 0;
	/* {o is an array} */

	return builtin_descriptorscompatible(valuedesc,componentvftbl->arraydesc);
}


/* This is an optimized version where a is guaranteed to be one-dimensional */
s4 builtin_canstore_onedim (java_objectarray *a, java_objectheader *o)
{
	arraydescriptor *desc;
	vftbl_t *elementvftbl;
	vftbl_t *valuevftbl;
	s4 res;
	int base;
	castinfo classvalues;
	
	if (!o) return 1;

	/* The following is guaranteed (by verifier checks):
	 *
	 *     *) a->...vftbl->arraydesc != NULL
	 *     *) a->...vftbl->arraydesc->elementvftbl != NULL
	 *     *) a->...vftbl->arraydesc->dimension == 1
	 *     *) o->vftbl is not an interface vftbl
	 */

	desc = a->header.objheader.vftbl->arraydesc;
    elementvftbl = desc->elementvftbl;
	valuevftbl = o->vftbl;

	/* {a is a one-dimensional array} */
	
	if (valuevftbl == elementvftbl)
		return 1;

	asm_getclassvalues_atomic(elementvftbl, valuevftbl, &classvalues);

	if ((base = classvalues.super_baseval) <= 0)
		/* an array of interface references */
		return (valuevftbl->interfacetablelength > -base &&
				valuevftbl->interfacetable[base] != NULL);

	res = (unsigned) (classvalues.sub_baseval - classvalues.super_baseval)
		<= (unsigned) classvalues.super_diffval;

	return res;
}


/* This is an optimized version where a is guaranteed to be a
 * one-dimensional array of a class type */
s4 builtin_canstore_onedim_class(java_objectarray *a, java_objectheader *o)
{
	vftbl_t *elementvftbl;
	vftbl_t *valuevftbl;
	s4 res;
	castinfo classvalues;
	
	if (!o) return 1;

	/* The following is guaranteed (by verifier checks):
	 *
	 *     *) a->...vftbl->arraydesc != NULL
	 *     *) a->...vftbl->arraydesc->elementvftbl != NULL
	 *     *) a->...vftbl->arraydesc->elementvftbl is not an interface vftbl
	 *     *) a->...vftbl->arraydesc->dimension == 1
	 *     *) o->vftbl is not an interface vftbl
	 */

    elementvftbl = a->header.objheader.vftbl->arraydesc->elementvftbl;
	valuevftbl = o->vftbl;

	/* {a is a one-dimensional array} */
	
	if (valuevftbl == elementvftbl)
		return 1;

	asm_getclassvalues_atomic(elementvftbl, valuevftbl, &classvalues);

	res = (unsigned) (classvalues.sub_baseval - classvalues.super_baseval)
		<= (unsigned) classvalues.super_diffval;

	return res;
}


/* builtin_new *****************************************************************

   Creates a new instance of class c on the heap.

   Return value: pointer to the object or NULL if no memory is
   available
			
*******************************************************************************/

java_objectheader *builtin_new(classinfo *c)
{
	java_objectheader *o;

	/* is the class loaded */
	/*utf_fprint(stderr,c->name);fprintf(stderr,"\n");*/
	assert(c->loaded);

	/* is the class linked */
	if (!c->linked)
		if (!link_class(c))
			return NULL;

	if (!c->initialized) {
		if (initverbose)
			log_message_class("Initialize class (from builtin_new): ", c);

		if (!initialize_class(c))
			return NULL;
	}

	o = heap_allocate(c->instancesize, true, c->finalizer);

	if (!o)
		return NULL;

	MSET(o, 0, u1, c->instancesize);

	o->vftbl = c->vftbl;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	initObjectLock(o);
#endif

	return o;
}


/* builtin_newarray ************************************************************

   Creates an array with the given vftbl on the heap.

   Return value:  pointer to the array or NULL if no memory is available

   CAUTION: The given vftbl must be the vftbl of the *array* class,
   not of the element class.

*******************************************************************************/

java_arrayheader *builtin_newarray(s4 size, vftbl_t *arrayvftbl)
{
	java_arrayheader *a;
	arraydescriptor *desc;
	s4 dataoffset;
	s4 componentsize;
	s4 actualsize;

	desc = arrayvftbl->arraydesc;
	dataoffset = desc->dataoffset;
	componentsize = desc->componentsize;

	if (size < 0) {
		*exceptionptr = new_negativearraysizeexception();
		return NULL;
	}

	actualsize = dataoffset + size * componentsize;

	if (((u4) actualsize) < ((u4) size)) { /* overflow */
		*exceptionptr = new_exception(string_java_lang_OutOfMemoryError);
		return NULL;
	}

	a = heap_allocate(actualsize, (desc->arraytype == ARRAYTYPE_OBJECT), NULL);

	if (!a)
		return NULL;

	MSET(a, 0, u1, actualsize);

	a->objheader.vftbl = arrayvftbl;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	initObjectLock(&a->objheader);
#endif

	a->size = size;

	return a;
}


/********************** Function: builtin_anewarray *************************

	Creates an array of references to the given class type on the heap.

	Return value: pointer to the array or NULL if no memory is available

    XXX This function does not do The Right Thing, because it uses a
    classinfo pointer at runtime. builtin_newarray should be used
    instead.

*****************************************************************************/

java_objectarray *builtin_anewarray(s4 size, classinfo *component)
{
	classinfo *c;
	
	/* is class loaded */
	assert(component->loaded);

	/* is class linked */
	if (!component->linked)
		if (!link_class(component))
			return NULL;

	c = class_array_of(component,true);
	if (!c)
		return NULL;
	return (java_objectarray *) builtin_newarray(size, c->vftbl);
}


/******************** Function: builtin_newarray_int ***********************

	Creates an array of 32 bit Integers on the heap.

	Return value:  pointer to the array or NULL if no memory is available

*****************************************************************************/

java_intarray *builtin_newarray_int(s4 size)
{
	return (java_intarray*) builtin_newarray(size, primitivetype_table[ARRAYTYPE_INT].arrayvftbl);
}


/******************** Function: builtin_newarray_long ***********************

	Creates an array of 64 bit Integers on the heap.

	Return value:  pointer to the array or NULL if no memory is available

*****************************************************************************/

java_longarray *builtin_newarray_long(s4 size)
{
	return (java_longarray*) builtin_newarray(size, primitivetype_table[ARRAYTYPE_LONG].arrayvftbl);
}


/******************** function: builtin_newarray_float ***********************

	Creates an array of 32 bit IEEE floats on the heap.

	Return value:  pointer to the array or NULL if no memory is available

*****************************************************************************/

java_floatarray *builtin_newarray_float(s4 size)
{
	return (java_floatarray*) builtin_newarray(size, primitivetype_table[ARRAYTYPE_FLOAT].arrayvftbl);
}


/******************** function: builtin_newarray_double ***********************

	Creates an array of 64 bit IEEE floats on the heap.

	Return value:  pointer to the array or NULL if no memory is available

*****************************************************************************/

java_doublearray *builtin_newarray_double(s4 size)
{
	return (java_doublearray*) builtin_newarray(size, primitivetype_table[ARRAYTYPE_DOUBLE].arrayvftbl);
}


/******************** function: builtin_newarray_byte ***********************

	Creates an array of 8 bit Integers on the heap.

	Return value:  pointer to the array or NULL if no memory is available

*****************************************************************************/

java_bytearray *builtin_newarray_byte(s4 size)
{
	return (java_bytearray*) builtin_newarray(size, primitivetype_table[ARRAYTYPE_BYTE].arrayvftbl);
}


/******************** function: builtin_newarray_char ************************

	Creates an array of characters on the heap.

	Return value:  pointer to the array or NULL if no memory is available

*****************************************************************************/

java_chararray *builtin_newarray_char(s4 size)
{
	return (java_chararray*) builtin_newarray(size, primitivetype_table[ARRAYTYPE_CHAR].arrayvftbl);
}


/******************** function: builtin_newarray_short ***********************

	Creates an array of 16 bit Integers on the heap.

	Return value:  pointer to the array or NULL if no memory is available

*****************************************************************************/

java_shortarray *builtin_newarray_short(s4 size)
{
	return (java_shortarray*) builtin_newarray(size, primitivetype_table[ARRAYTYPE_SHORT].arrayvftbl);
}


/******************** function: builtin_newarray_boolean ************************

	Creates an array of bytes on the heap. The array is designated as an array
	of booleans (important for casts)
	
	Return value:  pointer to the array or NULL if no memory is available

*****************************************************************************/

java_booleanarray *builtin_newarray_boolean(s4 size)
{
	return (java_booleanarray*) builtin_newarray(size, primitivetype_table[ARRAYTYPE_BOOLEAN].arrayvftbl);
}


/**************** function: builtin_nmultianewarray ***************************

	Creates a multi-dimensional array on the heap. The dimensions are passed in
	an array of longs.

    Arguments:
        n............number of dimensions to create
        arrayvftbl...vftbl of the array class
        dims.........array containing the size of each dimension to create

	Return value:  pointer to the array or NULL if no memory is available

******************************************************************************/

java_arrayheader *builtin_nmultianewarray(int n, vftbl_t *arrayvftbl, long *dims)
/*  java_arrayheader *builtin_nmultianewarray(int n, classinfo *arrayclass, long *dims) */
{
	s4 size, i;
	java_arrayheader *a;
	vftbl_t *componentvftbl;

/*  	utf_display(arrayclass->name); */

/*  	class_load(arrayclass); */
/*  	class_link(arrayclass); */
	
	/* create this dimension */
	size = (s4) dims[0];
  	a = builtin_newarray(size, arrayvftbl);
/*  	a = builtin_newarray(size, arrayclass->vftbl); */

	if (!a)
		return NULL;

	/* if this is the last dimension return */
	if (!--n)
		return a;

	/* get the vftbl of the components to create */
	componentvftbl = arrayvftbl->arraydesc->componentvftbl;
/*  	component = arrayclass->vftbl->arraydesc; */

	/* The verifier guarantees this. */
	/* if (!componentvftbl) */
	/*	panic ("multianewarray with too many dimensions"); */

	/* create the component arrays */
	for (i = 0; i < size; i++) {
		java_arrayheader *ea = 
			builtin_nmultianewarray(n, componentvftbl, dims + 1);

		if (!ea)
			return NULL;
		
		((java_objectarray *) a)->data[i] = (java_objectheader *) ea;
	}

	return a;
}


/*****************************************************************************
					  METHOD LOGGING

	Various functions for printing a message at method entry or exit (for
	debugging)
	
*****************************************************************************/

u4 methodindent = 0;

java_objectheader *builtin_trace_exception(java_objectheader *xptr,
										   methodinfo *m,
										   void *pos,
										   s4 line,
										   s4 noindent)
{
	if (!noindent) {
		if (methodindent)
			methodindent--;
		else
			log_text("WARNING: unmatched methodindent--");
	}
	if (opt_verbose || runverbose || verboseexception) {
		if (xptr) {
			printf("Exception ");
			utf_display_classname(xptr->vftbl->class->name);

		} else {
			printf("Some Throwable");
		}
		printf(" thrown in ");

		if (m) {
			utf_display_classname(m->class->name);
			printf(".");
			utf_display(m->name);
			if (m->flags & ACC_SYNCHRONIZED) {
				printf("(SYNC");

			} else{
				printf("(NOSYNC");
			}

			if (m->flags & ACC_NATIVE) {
				printf(",NATIVE");
#if POINTERSIZE == 8
				printf(")(0x%016lx) at position 0x%016lx\n", (ptrint) m->entrypoint, (ptrint) pos);
#else
				printf(")(0x%08x) at position 0x%08x\n", (ptrint) m->entrypoint, (ptrint) pos);
#endif

			} else {
#if POINTERSIZE == 8
				printf(")(0x%016lx) at position 0x%016lx (", (ptrint) m->entrypoint, (ptrint) pos);
#else
				printf(")(0x%08x) at position 0x%08x (", (ptrint) m->entrypoint, (ptrint) pos);
#endif
				if (m->class->sourcefile == NULL) {
					printf("<NO CLASSFILE INFORMATION>");

				} else {
					utf_display(m->class->sourcefile);
				}
				printf(":%d)\n", line);
			}

		} else
			printf("call_java_method\n");
		fflush(stdout);
	}

	return xptr;
}


#ifdef TRACE_ARGS_NUM
void builtin_trace_args(s8 a0, s8 a1, s8 a2, s8 a3,
#if TRACE_ARGS_NUM >= 6
						s8 a4, s8 a5,
#endif
#if TRACE_ARGS_NUM == 8
						s8 a6, s8 a7,
#endif
						methodinfo *m)
{
	s4 i;
	char logtext[MAXLOGTEXT];
	for (i = 0; i < methodindent; i++)
		logtext[i] = '\t';
	if (methodindent == 0) 
			sprintf(logtext + methodindent, "1st_call: ");
	else
		sprintf(logtext + methodindent, "called: ");

	utf_sprint_classname(logtext + strlen(logtext), m->class->name);
	sprintf(logtext + strlen(logtext), ".");
	utf_sprint(logtext + strlen(logtext), m->name);
	utf_sprint(logtext + strlen(logtext), m->descriptor);

	if (m->flags & ACC_PUBLIC)       sprintf(logtext + strlen(logtext), " PUBLIC");
	if (m->flags & ACC_PRIVATE)      sprintf(logtext + strlen(logtext), " PRIVATE");
	if (m->flags & ACC_PROTECTED)    sprintf(logtext + strlen(logtext), " PROTECTED");
   	if (m->flags & ACC_STATIC)       sprintf(logtext + strlen(logtext), " STATIC");
   	if (m->flags & ACC_FINAL)        sprintf(logtext + strlen(logtext), " FINAL");
   	if (m->flags & ACC_SYNCHRONIZED) sprintf(logtext + strlen(logtext), " SYNCHRONIZED");
   	if (m->flags & ACC_VOLATILE)     sprintf(logtext + strlen(logtext), " VOLATILE");
   	if (m->flags & ACC_TRANSIENT)    sprintf(logtext + strlen(logtext), " TRANSIENT");
   	if (m->flags & ACC_NATIVE)       sprintf(logtext + strlen(logtext), " NATIVE");
   	if (m->flags & ACC_INTERFACE)    sprintf(logtext + strlen(logtext), " INTERFACE");
   	if (m->flags & ACC_ABSTRACT)     sprintf(logtext + strlen(logtext), " ABSTRACT");
	

	sprintf(logtext + strlen(logtext), "(");

	switch (m->paramcount) {
	case 0:
		break;

#if defined(__I386__) || defined(__POWERPC__)
	case 1:
		sprintf(logtext+strlen(logtext), "%llx", a0);
		break;

	case 2:
		sprintf(logtext+strlen(logtext), "%llx, %llx", a0, a1);
		break;

	case 3:
		sprintf(logtext+strlen(logtext), "%llx, %llx, %llx", a0, a1, a2);
		break;

	case 4:
		sprintf(logtext+strlen(logtext), "%llx, %llx, %llx, %llx",
				a0,   a1,   a2,   a3);
		break;

#if TRACE_ARGS_NUM >= 6
	case 5:
		sprintf(logtext+strlen(logtext), "%llx, %llx, %llx, %llx, %llx",
				a0,   a1,   a2,   a3,   a4);
		break;

	case 6:
		sprintf(logtext+strlen(logtext), "%llx, %llx, %llx, %llx, %llx, %llx",
				a0,   a1,   a2,   a3,   a4,   a5);
		break;
#endif /* TRACE_ARGS_NUM >= 6 */

#if TRACE_ARGS_NUM == 8
	case 7:
		sprintf(logtext+strlen(logtext), "%llx, %llx, %llx, %llx, %llx, %llx, %llx",
				a0,   a1,   a2,   a3,   a4,   a5,   a6);
		break;

	case 8:
		sprintf(logtext+strlen(logtext), "%llx, %llx, %llx, %llx, %llx, %llx, %llx, %llx",
				a0,   a1,   a2,   a3,   a4,   a5,   a6,   a7);
		break;

	default:
		sprintf(logtext+strlen(logtext), "%llx, %llx, %llx, %llx, %llx, %llx, %llx, %llx, ...(%d)",
				a0,   a1,   a2,   a3,   a4,   a5,   a6,   a7,   m->paramcount - 8);
		break;
#else /* TRACE_ARGS_NUM == 8 */
	default:
		sprintf(logtext+strlen(logtext), "%llx, %llx, %llx, %llx, %llx, %llx, ...(%d)",
				a0,   a1,   a2,   a3,   a4,   a5,   m->paramcount - 6);
		break;
#endif /* TRACE_ARGS_NUM == 8 */
#else /* defined(__I386__) || defined(__POWERPC__) */
	case 1:
		sprintf(logtext+strlen(logtext), "%lx", a0);
		break;

	case 2:
		sprintf(logtext+strlen(logtext), "%lx, %lx", a0, a1);
		break;

	case 3:
		sprintf(logtext+strlen(logtext), "%lx, %lx, %lx", a0, a1, a2);
		break;

	case 4:
		sprintf(logtext+strlen(logtext), "%lx, %lx, %lx, %lx",
				a0,  a1,  a2,  a3);
		break;

#if TRACE_ARGS_NUM >= 6
	case 5:
		sprintf(logtext+strlen(logtext), "%lx, %lx, %lx, %lx, %lx",
				a0,  a1,  a2,  a3,  a4);
		break;

	case 6:
		sprintf(logtext+strlen(logtext), "%lx, %lx, %lx, %lx, %lx, %lx",
				a0,  a1,  a2,  a3,  a4,  a5);
		break;
#endif /* TRACE_ARGS_NUM >= 6 */

#if TRACE_ARGS_NUM == 8
	case 7:
		sprintf(logtext+strlen(logtext), "%lx, %lx, %lx, %lx, %lx, %lx, %lx",
				a0,  a1,  a2,  a3,  a4,  a5,  a6);
		break;

	case 8:
		sprintf(logtext+strlen(logtext), "%lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx",
				a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7);
		break;
#endif /* TRACE_ARGS_NUM == 8 */

	default:
#if TRACE_ARGS_NUM == 4
		sprintf(logtext+strlen(logtext), "%lx, %lx, %lx, %lx, ...(%d)",
				a0,  a1,  a2,  a3,   m->paramcount - 4);

#elif TRACE_ARGS_NUM == 6
		sprintf(logtext+strlen(logtext), "%lx, %lx, %lx, %lx, %lx, %lx, ...(%d)",
				a0,  a1,  a2,  a3,  a4,  a5,  m->paramcount - 6);

#elif TRACE_ARGS_NUM == 8
		sprintf(logtext+strlen(logtext), "%lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, ...(%d)",
				a0,  a1,  a2,  a3,  a4,  a5,  a6,  a7,  m->paramcount - 8);
#endif
		break;
#endif /* defined(__I386__) || defined(__POWERPC__) */
	}

	sprintf(logtext + strlen(logtext), ")");
	log_text(logtext);

	methodindent++;
}
#endif


void builtin_displaymethodstart(methodinfo *m)
{
	char logtext[MAXLOGTEXT];
	sprintf(logtext, "												");
	sprintf(logtext + methodindent, "called: ");
	utf_sprint(logtext + strlen(logtext), m->class->name);
	sprintf(logtext + strlen(logtext), ".");
	utf_sprint(logtext + strlen(logtext), m->name);
	utf_sprint(logtext + strlen(logtext), m->descriptor);

	if (m->flags & ACC_PUBLIC)       sprintf(logtext + strlen(logtext), " PUBLIC");
	if (m->flags & ACC_PRIVATE)      sprintf(logtext + strlen(logtext), " PRIVATE");
	if (m->flags & ACC_PROTECTED)    sprintf(logtext + strlen(logtext), " PROTECTED");
   	if (m->flags & ACC_STATIC)       sprintf(logtext + strlen(logtext), " STATIC");
   	if (m->flags & ACC_FINAL)        sprintf(logtext + strlen(logtext), " FINAL");
   	if (m->flags & ACC_SYNCHRONIZED) sprintf(logtext + strlen(logtext), " SYNCHRONIZED");
   	if (m->flags & ACC_VOLATILE)     sprintf(logtext + strlen(logtext), " VOLATILE");
   	if (m->flags & ACC_TRANSIENT)    sprintf(logtext + strlen(logtext), " TRANSIENT");
   	if (m->flags & ACC_NATIVE)       sprintf(logtext + strlen(logtext), " NATIVE");
   	if (m->flags & ACC_INTERFACE)    sprintf(logtext + strlen(logtext), " INTERFACE");
   	if (m->flags & ACC_ABSTRACT)     sprintf(logtext + strlen(logtext), " ABSTRACT");

	log_text(logtext);
	methodindent++;
}


void builtin_displaymethodstop(methodinfo *m, s8 l, double d, float f)
{
	int i;
	char logtext[MAXLOGTEXT];
	for (i = 0; i < methodindent; i++)
		logtext[i] = '\t';
	if (methodindent)
		methodindent--;
	else
		log_text("WARNING: unmatched methodindent--");

	sprintf(logtext + methodindent, "finished: ");
	utf_sprint_classname(logtext + strlen(logtext), m->class->name);
	sprintf(logtext + strlen(logtext), ".");
	utf_sprint(logtext + strlen(logtext), m->name);
	utf_sprint(logtext + strlen(logtext), m->descriptor);

	switch (m->returntype) {
	case TYPE_INT:
		sprintf(logtext + strlen(logtext), "->%d", (s4) l);
		break;

	case TYPE_LONG:
#if defined(__I386__) || defined(__POWERPC__)
		sprintf(logtext + strlen(logtext), "->%lld", (s8) l);
#else
		sprintf(logtext + strlen(logtext), "->%ld", (s8) l);
#endif
		break;

	case TYPE_ADDRESS:
#if defined(__I386__) || defined(__POWERPC__)
		sprintf(logtext + strlen(logtext), "->%p", (u1*) ((s4) l));
#else
		sprintf(logtext + strlen(logtext), "->%p", (u1*) l);
#endif
		break;

	case TYPE_FLOAT:
		sprintf(logtext + strlen(logtext), "->%g", f);
		break;

	case TYPE_DOUBLE:
		sprintf(logtext + strlen(logtext), "->%g", d);
		break;
	}
	log_text(logtext);
}


/****************************************************************************
			 SYNCHRONIZATION FUNCTIONS
*****************************************************************************/

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
/*
 * Lock the mutex of an object.
 */
void internal_lock_mutex_for_object(java_objectheader *object)
{
	mutexHashEntry *entry;
	int hashValue;

	assert(object != 0);

	hashValue = MUTEX_HASH_VALUE(object);
	entry = &mutexHashTable[hashValue];

	if (entry->object != 0) {
		if (entry->mutex.count == 0 && entry->conditionCount == 0) {
			entry->object = 0;
			entry->mutex.holder = 0;
			entry->mutex.count = 0;
			entry->mutex.muxWaiters = 0;

		} else {
			while (entry->next != 0 && entry->object != object)
				entry = entry->next;

			if (entry->object != object) {
				entry->next = firstFreeOverflowEntry;
				firstFreeOverflowEntry = firstFreeOverflowEntry->next;

				entry = entry->next;
				entry->object = 0;
				entry->next = 0;
				assert(entry->conditionCount == 0);
			}
		}

	} else {
		entry->mutex.holder = 0;
		entry->mutex.count = 0;
		entry->mutex.muxWaiters = 0;
	}

	if (entry->object == 0)
		entry->object = object;
	
	internal_lock_mutex(&entry->mutex);
}
#endif


#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
/*
 * Unlocks the mutex of an object.
 */
void internal_unlock_mutex_for_object (java_objectheader *object)
{
	int hashValue;
	mutexHashEntry *entry;

	hashValue = MUTEX_HASH_VALUE(object);
	entry = &mutexHashTable[hashValue];

	if (entry->object == object) {
		internal_unlock_mutex(&entry->mutex);

	} else {
		while (entry->next != 0 && entry->next->object != object)
			entry = entry->next;

		assert(entry->next != 0);

		internal_unlock_mutex(&entry->next->mutex);

		if (entry->next->mutex.count == 0 && entry->conditionCount == 0) {
			mutexHashEntry *unlinked = entry->next;

			entry->next = unlinked->next;
			unlinked->next = firstFreeOverflowEntry;
			firstFreeOverflowEntry = unlinked;
		}
	}
}
#endif


#if defined(USE_THREADS)
void builtin_monitorenter(java_objectheader *o)
{
#if !defined(NATIVE_THREADS)
	int hashValue;

	++blockInts;

	hashValue = MUTEX_HASH_VALUE(o);
	if (mutexHashTable[hashValue].object == o 
		&& mutexHashTable[hashValue].mutex.holder == currentThread)
		++mutexHashTable[hashValue].mutex.count;
	else
		internal_lock_mutex_for_object(o);

	--blockInts;
#else
	monitorEnter((threadobject *) THREADOBJECT, o);
#endif
}
#endif


#if defined(USE_THREADS)
/*
 * Locks the class object - needed for static synchronized methods.
 * The use_class_as_object call is needed in order to circumvent a
 * possible deadlock with builtin_monitorenter called by another
 * thread calling use_class_as_object.
 */
void builtin_staticmonitorenter(classinfo *c)
{
	use_class_as_object(c);
	builtin_monitorenter(&c->header);
}
#endif


#if defined(USE_THREADS)
void *builtin_monitorexit(java_objectheader *o)
{
#if !defined(NATIVE_THREADS)
	int hashValue;

	++blockInts;

	hashValue = MUTEX_HASH_VALUE(o);
	if (mutexHashTable[hashValue].object == o) {
		if (mutexHashTable[hashValue].mutex.count == 1
			&& mutexHashTable[hashValue].mutex.muxWaiters != 0)
			internal_unlock_mutex_for_object(o);
		else
			--mutexHashTable[hashValue].mutex.count;

	} else
		internal_unlock_mutex_for_object(o);

	--blockInts;
	return o;
#else
	monitorExit((threadobject *) THREADOBJECT, o);
	return o;
#endif
}
#endif


/*****************************************************************************
			  MISCELLANEOUS HELPER FUNCTIONS
*****************************************************************************/



/*********** Functions for integer divisions *****************************
 
	On some systems (eg. DEC ALPHA), integer division is not supported by the
	CPU. These helper functions implement the missing functionality.

******************************************************************************/

s4 builtin_idiv(s4 a, s4 b) { return a / b; }
s4 builtin_irem(s4 a, s4 b) { return a % b; }


/************** Functions for long arithmetics *******************************

	On systems where 64 bit Integers are not supported by the CPU, these
	functions are needed.

******************************************************************************/


s8 builtin_ladd(s8 a, s8 b)
{ 
#if U8_AVAILABLE
	return a + b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lsub(s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a - b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lmul(s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a * b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_ldiv(s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a / b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lrem(s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a % b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lshl(s8 a, s4 b) 
{ 
#if U8_AVAILABLE
	return a << (b & 63);
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lshr(s8 a, s4 b) 
{ 
#if U8_AVAILABLE
	return a >> (b & 63);
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lushr(s8 a, s4 b) 
{ 
#if U8_AVAILABLE
	return ((u8) a) >> (b & 63);
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_land(s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a & b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lor(s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a | b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lxor(s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a ^ b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lneg(s8 a) 
{ 
#if U8_AVAILABLE
	return -a;
#else
	return builtin_i2l(0);
#endif
}

s4 builtin_lcmp(s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	if (a < b) return -1;
	if (a > b) return 1;
	return 0;
#else
	return 0;
#endif
}





/*********** Functions for floating point operations *************************/

float builtin_fadd(float a, float b)
{
	if (isnanf(a)) return intBitsToFloat(FLT_NAN);
	if (isnanf(b)) return intBitsToFloat(FLT_NAN);
	if (finitef(a)) {
		if (finitef(b))
			return a + b;
		else
			return b;
	}
	else {
		if (finitef(b))
			return a;
		else {
			if (copysignf(1.0, a) == copysignf(1.0, b))
				return a;
			else
				return intBitsToFloat(FLT_NAN);
		}
	}
}


float builtin_fsub(float a, float b)
{
	return builtin_fadd(a, builtin_fneg(b));
}


float builtin_fmul(float a, float b)
{
	if (isnanf(a)) return intBitsToFloat(FLT_NAN);
	if (isnanf(b)) return intBitsToFloat(FLT_NAN);
	if (finitef(a)) {
		if (finitef(b)) return a * b;
		else {
			if (a == 0) return intBitsToFloat(FLT_NAN);
			else return copysignf(b, copysignf(1.0, b)*a);
		}
	}
	else {
		if (finitef(b)) {
			if (b == 0) return intBitsToFloat(FLT_NAN);
			else return copysignf(a, copysignf(1.0, a)*b);
		}
		else {
			return copysignf(a, copysignf(1.0, a)*copysignf(1.0, b));
		}
	}
}


float builtin_fdiv(float a, float b)
{
	if (finitef(a) && finitef(b)) {
		if (b != 0)
			return a / b;
		else {
			if (a > 0)
				return intBitsToFloat(FLT_POSINF);
			else if (a < 0)
				return intBitsToFloat(FLT_NEGINF);
		}
	}
	return intBitsToFloat(FLT_NAN);
}


float builtin_frem(float a, float b)
{
	return fmodf(a, b);
}


float builtin_fneg(float a)
{
	if (isnanf(a)) return a;
	else {
		if (finitef(a)) return -a;
		else return copysignf(a, -copysignf(1.0, a));
	}
}


s4 builtin_fcmpl(float a, float b)
{
	if (isnanf(a)) return -1;
	if (isnanf(b)) return -1;
	if (!finitef(a) || !finitef(b)) {
		a = finitef(a) ? 0 : copysignf(1.0,	a);
		b = finitef(b) ? 0 : copysignf(1.0, b);
	}
	if (a > b) return 1;
	if (a == b) return 0;
	return -1;
}


s4 builtin_fcmpg(float a, float b)
{
	if (isnanf(a)) return 1;
	if (isnanf(b)) return 1;
	if (!finitef(a) || !finitef(b)) {
		a = finitef(a) ? 0 : copysignf(1.0, a);
		b = finitef(b) ? 0 : copysignf(1.0, b);
	}
	if (a > b) return 1;
	if (a == b) return 0;
	return -1;
}



/************************* Functions for doubles ****************************/

double builtin_dadd(double a, double b)
{
	if (isnan(a)) return longBitsToDouble(DBL_NAN);
	if (isnan(b)) return longBitsToDouble(DBL_NAN);
	if (finite(a)) {
		if (finite(b)) return a + b;
		else return b;
	}
	else {
		if (finite(b)) return a;
		else {
			if (copysign(1.0, a)==copysign(1.0, b)) return a;
			else return longBitsToDouble(DBL_NAN);
		}
	}
}


double builtin_dsub(double a, double b)
{
	return builtin_dadd(a, builtin_dneg(b));
}


double builtin_dmul(double a, double b)
{
	if (isnan(a)) return longBitsToDouble(DBL_NAN);
	if (isnan(b)) return longBitsToDouble(DBL_NAN);
	if (finite(a)) {
		if (finite(b)) return a * b;
		else {
			if (a == 0) return longBitsToDouble(DBL_NAN);
			else return copysign(b, copysign(1.0, b) * a);
		}
	}
	else {
		if (finite(b)) {
			if (b == 0) return longBitsToDouble(DBL_NAN);
			else return copysign(a, copysign(1.0, a) * b);
		}
		else {
			return copysign(a, copysign(1.0, a) * copysign(1.0, b));
		}
	}
}


double builtin_ddiv(double a, double b)
{
	if (finite(a)) {
		if (finite(b)) {
			return a / b;

		} else {
			if (isnan(b))
				return longBitsToDouble(DBL_NAN);
			else
				return copysign(0.0, b);
		}

	} else {
		if (finite(b)) {
			if (a > 0)
				return longBitsToDouble(DBL_POSINF);
			else if (a < 0)
				return longBitsToDouble(DBL_NEGINF);

		} else
			return longBitsToDouble(DBL_NAN);
	}

/*  	if (finite(a) && finite(b)) { */
/*  		if (b != 0) */
/*  			return a / b; */
/*  		else { */
/*  			if (a > 0) */
/*  				return longBitsToDouble(DBL_POSINF); */
/*  			else if (a < 0) */
/*  				return longBitsToDouble(DBL_NEGINF); */
/*  		} */
/*  	} */

	/* keep compiler happy */
	return 0;
}


double builtin_drem(double a, double b)
{
	return fmod(a, b);
}


double builtin_dneg(double a)
{
	if (isnan(a)) return a;
	else {
		if (finite(a)) return -a;
		else return copysign(a, -copysign(1.0, a));
	}
}


s4 builtin_dcmpl(double a, double b)
{
	if (isnan(a)) return -1;
	if (isnan(b)) return -1;
	if (!finite(a) || !finite(b)) {
		a = finite(a) ? 0 : copysign(1.0, a);
		b = finite(b) ? 0 : copysign(1.0, b);
	}
	if (a > b) return 1;
	if (a == b) return 0;
	return -1;
}


s4 builtin_dcmpg(double a, double b)
{
	if (isnan(a)) return 1;
	if (isnan(b)) return 1;
	if (!finite(a) || !finite(b)) {
		a = finite(a) ? 0 : copysign(1.0, a);
		b = finite(b) ? 0 : copysign(1.0, b);
	}
	if (a > b) return 1;
	if (a == b) return 0;
	return -1;
}


/*********************** Conversion operations ****************************/

s8 builtin_i2l(s4 i)
{
#if U8_AVAILABLE
	return i;
#else
	s8 v;
	v.high = 0;
	v.low = i;
	return v;
#endif
}


float builtin_i2f(s4 a)
{
	float f = (float) a;
	return f;
}


double builtin_i2d(s4 a)
{
	double d = (double) a;
	return d;
}


s4 builtin_l2i(s8 l)
{
#if U8_AVAILABLE
	return (s4) l;
#else
	return l.low;
#endif
}


float builtin_l2f(s8 a)
{
#if U8_AVAILABLE
	float f = (float) a;
	return f;
#else
	return 0.0;
#endif
}


double builtin_l2d(s8 a)
{
#if U8_AVAILABLE
	double d = (double) a;
	return d;
#else
	return 0.0;
#endif
}


s4 builtin_f2i(float a) 
{

	return builtin_d2i((double) a);

	/*	float f;
	
		if (isnanf(a))
		return 0;
		if (finitef(a)) {
		if (a > 2147483647)
		return 2147483647;
		if (a < (-2147483648))
		return (-2147483648);
		return (s4) a;
		}
		f = copysignf((float) 1.0, a);
		if (f > 0)
		return 2147483647;
		return (-2147483648); */
}


s8 builtin_f2l(float a)
{

	return builtin_d2l((double) a);

	/*	float f;
	
		if (finitef(a)) {
		if (a > 9223372036854775807L)
		return 9223372036854775807L;
		if (a < (-9223372036854775808L))
		return (-9223372036854775808L);
		return (s8) a;
		}
		if (isnanf(a))
		return 0;
		f = copysignf((float) 1.0, a);
		if (f > 0)
		return 9223372036854775807L;
		return (-9223372036854775808L); */
}


double builtin_f2d(float a)
{
	if (finitef(a)) return (double) a;
	else {
		if (isnanf(a))
			return longBitsToDouble(DBL_NAN);
		else
			return copysign(longBitsToDouble(DBL_POSINF), (double) copysignf(1.0, a) );
	}
}


s4 builtin_d2i(double a) 
{ 
	double d;
	
	if (finite(a)) {
		if (a >= 2147483647)
			return 2147483647;
		if (a <= (-2147483647-1))
			return (-2147483647-1);
		return (s4) a;
	}
	if (isnan(a))
		return 0;
	d = copysign(1.0, a);
	if (d > 0)
		return 2147483647;
	return (-2147483647-1);
}


s8 builtin_d2l(double a)
{
	double d;
	
	if (finite(a)) {
		if (a >= 9223372036854775807LL)
			return 9223372036854775807LL;
		if (a <= (-9223372036854775807LL-1))
			return (-9223372036854775807LL-1);
		return (s8) a;
	}
	if (isnan(a))
		return 0;
	d = copysign(1.0, a);
	if (d > 0)
		return 9223372036854775807LL;
	return (-9223372036854775807LL-1);
}


float builtin_d2f(double a)
{
	if (finite(a))
		return (float) a;
	else {
		if (isnan(a))
			return intBitsToFloat(FLT_NAN);
		else
			return copysignf(intBitsToFloat(FLT_POSINF), (float) copysign(1.0, a));
	}
}


/* used to convert FLT_xxx defines into float values */

inline float intBitsToFloat(s4 i)
{
	imm_union imb;

	imb.i = i;
	return imb.f;
}


/* used to convert DBL_xxx defines into double values */

inline float longBitsToDouble(s8 l)
{
	imm_union imb;

	imb.l = l;
	return imb.d;
}


java_arrayheader *builtin_clone_array(void *env, java_arrayheader *o)
{
	return (java_arrayheader *)
		Java_java_lang_VMObject_clone(0, 0, (java_lang_Cloneable *) o);
}


s4 builtin_dummy()
{
	panic("Internal error: builtin_dummy called (native function is missing)");
	return 0; /* for the compiler */
}


/* builtin_asm_get_exceptionptrptr *********************************************

   this is a wrapper for calls from asmpart

*******************************************************************************/

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
java_objectheader **builtin_asm_get_exceptionptrptr()
{
	return builtin_get_exceptionptrptr();
}
#endif


methodinfo *builtin_asm_get_threadrootmethod()
{
	return *threadrootmethod;
}


inline void* builtin_asm_get_stackframeinfo()
{
/*log_text("builtin_asm_get_stackframeinfo()");*/
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	return &THREADINFO->_stackframeinfo;
#else
#if defined(__GNUC__)
#warning FIXME FOR OLD THREAD IMPL (jowenn)
#endif
		return &_thread_nativestackframeinfo; /* no threading, at least no native*/
#endif
}

stacktraceelement *builtin_stacktrace_copy(stacktraceelement **el,stacktraceelement *begin, stacktraceelement *end) {
/*	stacktraceelement *el;*/
	size_t s;
	s=(end-begin);
	/*printf ("begin: %p, end: %p, diff: %ld, size :%ld\n",begin,end,s,s*sizeof(stacktraceelement));*/
	*el=heap_allocate(sizeof(stacktraceelement)*(s+1), true, 0);
#if 0
	*el=MNEW(stacktraceelement,s+1); /*GC*/
#endif
	memcpy(*el,begin,(end-begin)*sizeof(stacktraceelement));
	(*el)[s].method=0;
#if defined(__GNUC__)
#warning change this if line numbers bigger than u2 are allowed, the currently supported class file format does no allow that
#endif
	(*el)[s].linenumber=-1; /* -1 can never be reched otherwise, since line numbers are only u2, so it is save to use that as flag */
	return *el;
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
