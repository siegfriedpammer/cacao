/* src/vm/jit/intrp/asmpart.c - Java-C interface functions for Interpreter

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
            Anton Ertl

   Changes:

   $Id: asmpart.c 3979 2005-12-21 16:39:52Z anton $

*/


#include <assert.h>

#include "config.h"
#include "vm/types.h"

#include "arch.h"

#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/exceptions.h"
#include "vm/options.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/methodheader.h"
#include "vm/jit/intrp/intrp.h"


/* this is required to link cacao with intrp */
threadcritnode asm_criticalsections = { NULL, NULL, NULL };


/* true on success, false on exception */
static bool asm_calljavafunction_intern(methodinfo *m, void *arg1, void *arg2,
										void *arg3, void *arg4)
{
	java_objectheader *retval;
	Cell *sp = global_sp;
	methoddesc *md;
	functionptr entrypoint;

	md = m->parseddesc;

	CLEAR_global_sp;
	assert(sp != NULL);

	/* XXX ugly hack: thread's run() needs 5 arguments */
	assert(md->paramcount < 6);

	if (md->paramcount > 0)
		*--sp=(Cell)arg1;
	if (md->paramcount > 1)
		*--sp=(Cell)arg2;
	if (md->paramcount > 2)
		*--sp=(Cell)arg3;
	if (md->paramcount > 3)
		*--sp=(Cell)arg4;
	if (md->paramcount > 4)
		*--sp=(Cell) 0;

	entrypoint = createcalljavafunction(m);

	retval = engine((Inst *) entrypoint, sp, NULL);

	/* XXX remove the method from the method table */

	if (retval != NULL) {
		(void) builtin_throw_exception(retval);
		return false;
	}
	else 
		return true;
}


s4 asm_calljavafunction_int(methodinfo *m, void *arg1, void *arg2,
							void *arg3, void *arg4)
{
	assert(m->parseddesc->returntype.type == TYPE_INT);

	if (asm_calljavafunction_intern(m, arg1, arg2, arg3, arg4))
		return (s4)(*global_sp++);
	else
		return 0;
}


java_objectheader *asm_calljavafunction(methodinfo *m, void *arg1, void *arg2,
										void *arg3, void *arg4)
{
	if (asm_calljavafunction_intern(m, arg1, arg2, arg3, arg4)) {
		if (m->parseddesc->returntype.type == TYPE_ADR)
			return (java_objectheader *)(*global_sp++);
		else {
			assert(m->parseddesc->returntype.type == TYPE_VOID);
			return NULL;
		}
	} else
		return NULL;
}


/* true on success, false on exception */
static bool jni_invoke_java_intern(methodinfo *m, u4 count, u4 size,
								   jni_callblock *callblock)
{
	java_objectheader *retval;
	Cell *sp = global_sp;
	s4 i;
	functionptr entrypoint;

	CLEAR_global_sp;
	assert(sp != NULL);

	for (i = 0; i < count; i++) {
		switch (callblock[i].itemtype) {
		case TYPE_INT:
		case TYPE_FLT:
		case TYPE_ADR:
			*(--sp) = callblock[i].item;
			break;
		case TYPE_LNG:
		case TYPE_DBL:
			sp -= 2;
			*((u8 *) sp) = callblock[i].item;
			break;
		}
	}

	entrypoint = createcalljavafunction(m);

	retval = engine((Inst *) entrypoint, sp, NULL);

	/* XXX remove the method from the method table */

	if (retval != NULL) {
		(void)builtin_throw_exception(retval);
		return false;
	}
	else
		return true;
}


java_objectheader *asm_calljavafunction2(methodinfo *m, u4 count, u4 size,
                                         jni_callblock *callblock)
{
	java_objectheader *retval = NULL;
	if (jni_invoke_java_intern(m, count, size, callblock)) {
		if (m->parseddesc->returntype.type == TYPE_ADR)
			retval = (java_objectheader *)*global_sp++;
		else
			assert(m->parseddesc->returntype.type == TYPE_VOID);
		return retval;
	} else
		return NULL;
}


s4 asm_calljavafunction2int(methodinfo *m, u4 count, u4 size,
                            jni_callblock *callblock)
{
	s4 retval=0;

	if (jni_invoke_java_intern(m, count, size, callblock)) {
		if (m->parseddesc->returntype.type == TYPE_INT)
			retval = *global_sp++;
		else
			assert(m->parseddesc->returntype.type == TYPE_VOID);
		return retval;
	} else
		return 0;
}


s8 asm_calljavafunction2long(methodinfo *m, u4 count, u4 size,
                             jni_callblock *callblock)
{
	s8 retval;

	assert(m->parseddesc->returntype.type == TYPE_LNG);

	if (jni_invoke_java_intern(m, count, size, callblock)) {
		retval = *(s8 *)global_sp;
		global_sp += 2;
		return retval;
	} else
		return 0;
}


float asm_calljavafunction2float(methodinfo *m, u4 count, u4 size,
								 jni_callblock *callblock)
{
	float retval;

	assert(m->parseddesc->returntype.type == TYPE_FLT);

	if (jni_invoke_java_intern(m, count, size, callblock)) {
		retval = *(float *)global_sp;
		global_sp += 1;
		return retval;
	} else
		return 0.0;
}


double asm_calljavafunction2double(methodinfo *m, u4 count, u4 size,
                                   jni_callblock *callblock)
{
	double retval;

	assert(m->parseddesc->returntype.type == TYPE_DBL);

	if (jni_invoke_java_intern(m, count, size, callblock)) {
		retval = *(double *)global_sp;
		global_sp += 2;
		return retval;
	} else
		return 0.0;
}


Inst *intrp_asm_handle_exception(Inst *ip, java_objectheader *o, Cell *fp, Cell **new_spp, Cell **new_fpp)
{
	classinfo      *c;
	s4              framesize;
	exceptionentry *ex;
	s4              exceptiontablelength;
	s4              i;

  /* for a description of the stack see IRETURN in java.vmg */

  for (; fp != NULL; ) {
	  u1 *f = codegen_findmethod((u1 *) (ip - 1));

	  /* get methodinfo pointer from method header */

	  methodinfo *m = *(methodinfo **) (((u1 *) f) + MethodPointer);

	  framesize = (*((s4 *) (((u1 *) f) + FrameSize)));
	  ex = (exceptionentry *) (((u1 *) f) + ExTableStart);
	  exceptiontablelength = *((s4 *) (((u1 *) f) + ExTableSize));

	  if (opt_verbose || runverbose || opt_verboseexception)
		  builtin_trace_exception(o, m, ip, 1);

	  for (i = 0; i < exceptiontablelength; i++) {
		  ex--;
		  c = ex->catchtype.cls;

		  if (c != NULL) {
			  if (!(c->state & CLASS_LOADED))
				  /* XXX fix me! */
				  if (!load_class_bootstrap(c->name))
					  assert(0);

			  if (!(c->state & CLASS_LINKED))
				  if (!link_class(c))
					  assert(0);
		  }

		  if (ip-1 >= (Inst *) ex->startpc && ip-1 < (Inst *) ex->endpc &&
			  (c == NULL || builtin_instanceof(o, c))) {
			  *new_spp = (Cell *)(((u1 *)fp) - framesize - SIZEOF_VOID_P);
			  *new_fpp = fp;
			  return (Inst *) (ex->handlerpc);
		  }
	  }

	  ip = (Inst *)access_local_cell(-framesize - SIZEOF_VOID_P);
	  fp = (Cell *)access_local_cell(-framesize);
  }

  return NULL; 
}


void asm_getclassvalues_atomic(vftbl_t *super, vftbl_t *sub, castinfo *out)
{
	s4 sbv, sdv, sv;

#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
	compiler_lock();
#else
	intsDisable();
#endif
#endif

	sbv = super->baseval;
	sdv = super->diffval;
	sv  = sub->baseval;

#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
	compiler_unlock();
#else
	intsRestore();
#endif
#endif

	out->super_baseval = sbv;
	out->super_diffval = sdv;
	out->sub_baseval   = sv;
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
