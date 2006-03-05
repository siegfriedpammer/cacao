/* src/vm/jit/intrp/asmpart.c - Java-C interface functions for Interpreter

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
            Anton Ertl

   Changes:

   $Id: asmpart.c 4559 2006-03-05 23:24:50Z twisti $

*/


#include "config.h"

#include <assert.h>

#include "vm/types.h"

#include "arch.h"

#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/exceptions.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/methodheader.h"
#include "vm/jit/intrp/intrp.h"


static bool intrp_asm_vm_call_method_intern(methodinfo *m, s4 vmargscount,
											vm_arg *vmargs)
{
	java_objectheader *retval;
	Cell              *sp;
	s4                 i;
	u1                *entrypoint;

	sp = global_sp;
	CLEAR_global_sp;

	assert(sp != NULL);

	for (i = 0; i < vmargscount; i++) {
		switch (vmargs[i].type) {
		case TYPE_INT:
		case TYPE_FLT:
		case TYPE_ADR:
			*(--sp) = vmargs[i].data;
			break;
		case TYPE_LNG:
		case TYPE_DBL:
			sp -= 2;
			*((u8 *) sp) = vmargs[i].data;
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


java_objectheader *intrp_asm_vm_call_method(methodinfo *m, s4 vmargscount,
											vm_arg *vmargs)
{
	java_objectheader *retval = NULL;

	if (intrp_asm_vm_call_method_intern(m, vmargscount, vmargs)) {
		if (m->parseddesc->returntype.type == TYPE_ADR)
			retval = (java_objectheader *)*global_sp++;
		else
			assert(m->parseddesc->returntype.type == TYPE_VOID);
		return retval;
	} else
		return NULL;
}


s4 intrp_asm_vm_call_method_int(methodinfo *m, s4 vmargscount, vm_arg *vmargs)
{
	s4 retval = 0;

	if (intrp_asm_vm_call_method_intern(m, vmargscount, vmargs)) {
		if (m->parseddesc->returntype.type == TYPE_INT)
			retval = *global_sp++;
		else
			assert(m->parseddesc->returntype.type == TYPE_VOID);
		return retval;
	} else
		return 0;
}


s8 intrp_asm_vm_call_method_long(methodinfo *m, s4 vmargscount, vm_arg *vmargs)
{
	s8 retval;

	assert(m->parseddesc->returntype.type == TYPE_LNG);

	if (intrp_asm_vm_call_method_intern(m, vmargscount, vmargs)) {
		retval = *(s8 *)global_sp;
		global_sp += 2;
		return retval;
	} else
		return 0;
}


float intrp_asm_vm_call_method_float(methodinfo *m, s4 vmargscount,
									 vm_arg *vmargs)
{
	float retval;

	assert(m->parseddesc->returntype.type == TYPE_FLT);

	if (intrp_asm_vm_call_method_intern(m, vmargscount, vmargs)) {
		retval = *(float *)global_sp;
		global_sp += 1;
		return retval;
	} else
		return 0.0;
}


double intrp_asm_vm_call_method_double(methodinfo *m, s4 vmargscount,
									   vm_arg *vmargs)
{
	double retval;

	assert(m->parseddesc->returntype.type == TYPE_DBL);

	if (intrp_asm_vm_call_method_intern(m, vmargscount, vmargs)) {
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

	  if (opt_verbose || opt_verbosecall || opt_verboseexception)
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


void intrp_asm_getclassvalues_atomic(vftbl_t *super, vftbl_t *sub, castinfo *out)
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
