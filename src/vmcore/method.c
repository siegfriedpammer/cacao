/* src/vmcore/method.c - method functions

   Copyright (C) 1996-2005, 2006, 2007, 2008
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


#include "config.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "vm/types.h"

#include "mm/memory.h"

#include "native/llni.h"

#include "threads/lock-common.h"

#include "vm/array.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/resolve.h"
#include "vm/vm.h"

#include "vm/jit/code.h"
#include "vm/jit/methodheader.h"

#include "vm/jit_interface.h"

#include "vmcore/class.h"
#include "vmcore/linker.h"
#include "vmcore/loader.h"
#include "vmcore/method.h"
#include "vmcore/options.h"
#include "vmcore/suck.h"
#include "vmcore/utf8.h"


#if !defined(NDEBUG) && defined(ENABLE_INLINING)
#define INLINELOG(code)  do { if (opt_TraceInlining) { code } } while (0)
#else
#define INLINELOG(code)
#endif


/* global variables ***********************************************************/

methodinfo *method_java_lang_reflect_Method_invoke;


/* method_init *****************************************************************

   Initialize method subsystem.

*******************************************************************************/

void method_init(void)
{
#if defined(ENABLE_JAVASE)
	/* Sanity check. */

	if (class_java_lang_reflect_Method == NULL)
		vm_abort("method_init: class_java_lang_reflect_Method is NULL");

	/* Cache java.lang.reflect.Method.invoke() */

	method_java_lang_reflect_Method_invoke =
		class_findmethod(class_java_lang_reflect_Method, utf_invoke, NULL);

	if (method_java_lang_reflect_Method_invoke == NULL)
		vm_abort("method_init: Could not resolve method java.lang.reflect.Method.invoke().");
#endif
}


/* method_load *****************************************************************

   Loads a method from the class file and fills an existing methodinfo
   structure.

   method_info {
       u2 access_flags;
	   u2 name_index;
	   u2 descriptor_index;
	   u2 attributes_count;
	   attribute_info attributes[attribute_count];
   }

   attribute_info {
       u2 attribute_name_index;
	   u4 attribute_length;
	   u1 info[attribute_length];
   }

   LineNumberTable_attribute {
       u2 attribute_name_index;
	   u4 attribute_length;
	   u2 line_number_table_length;
	   {
	       u2 start_pc;
		   u2 line_number;
	   } line_number_table[line_number_table_length];
   }

*******************************************************************************/

bool method_load(classbuffer *cb, methodinfo *m, descriptor_pool *descpool)
{
	classinfo *c;
	int argcount;
	s4         i, j, k, l;
	utf       *u;
	u2         name_index;
	u2         descriptor_index;
	u2         attributes_count;
	u2         attribute_name_index;
	utf       *attribute_name;
	u2         code_attributes_count;
	u2         code_attribute_name_index;
	utf       *code_attribute_name;

	/* get classinfo */

	c = cb->clazz;

	LOCK_INIT_OBJECT_LOCK(&(m->header));

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		count_all_methods++;
#endif

	/* all fields of m have been zeroed in load_class_from_classbuffer */

	m->clazz = c;
	
	if (!suck_check_classbuffer_size(cb, 2 + 2 + 2))
		return false;

	/* access flags */

	m->flags = suck_u2(cb);

	/* name */

	name_index = suck_u2(cb);

	if (!(u = class_getconstant(c, name_index, CONSTANT_Utf8)))
		return false;

	m->name = u;

	/* descriptor */

	descriptor_index = suck_u2(cb);

	if (!(u = class_getconstant(c, descriptor_index, CONSTANT_Utf8)))
		return false;

	m->descriptor = u;

	if (!descriptor_pool_add(descpool, u, &argcount))
		return false;

#ifdef ENABLE_VERIFIER
	if (opt_verify) {
		if (!is_valid_name_utf(m->name)) {
			exceptions_throw_classformaterror(c, "Method with invalid name");
			return false;
		}

		if (m->name->text[0] == '<' &&
			m->name != utf_init && m->name != utf_clinit) {
			exceptions_throw_classformaterror(c, "Method with invalid special name");
			return false;
		}
	}
#endif /* ENABLE_VERIFIER */
	
	if (!(m->flags & ACC_STATIC))
		argcount++; /* count the 'this' argument */

#ifdef ENABLE_VERIFIER
	if (opt_verify) {
		if (argcount > 255) {
			exceptions_throw_classformaterror(c, "Too many arguments in signature");
			return false;
		}

		/* check flag consistency */
		if (m->name != utf_clinit) {
			i = (m->flags & (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED));

			if (i != 0 && i != ACC_PUBLIC && i != ACC_PRIVATE && i != ACC_PROTECTED) {
				exceptions_throw_classformaterror(c,
												  "Illegal method modifiers: 0x%X",
												  m->flags);
				return false;
			}

			if (m->flags & ACC_ABSTRACT) {
				if ((m->flags & (ACC_FINAL | ACC_NATIVE | ACC_PRIVATE |
								 ACC_STATIC | ACC_STRICT | ACC_SYNCHRONIZED))) {
					exceptions_throw_classformaterror(c,
													  "Illegal method modifiers: 0x%X",
													  m->flags);
					return false;
				}
			}

			if (c->flags & ACC_INTERFACE) {
				if ((m->flags & (ACC_ABSTRACT | ACC_PUBLIC)) != (ACC_ABSTRACT | ACC_PUBLIC)) {
					exceptions_throw_classformaterror(c,
													  "Illegal method modifiers: 0x%X",
													  m->flags);
					return false;
				}
			}

			if (m->name == utf_init) {
				if (m->flags & (ACC_STATIC | ACC_FINAL | ACC_SYNCHRONIZED |
								ACC_NATIVE | ACC_ABSTRACT)) {
					exceptions_throw_classformaterror(c, "Instance initialization method has invalid flags set");
					return false;
				}
			}
		}
	}
#endif /* ENABLE_VERIFIER */

	/* mark the method as monomorphic until further notice */

	m->flags |= ACC_METHOD_MONOMORPHIC;

	/* non-abstract methods have an implementation in this class */

	if (!(m->flags & ACC_ABSTRACT))
		m->flags |= ACC_METHOD_IMPLEMENTED;
		
	if (!suck_check_classbuffer_size(cb, 2))
		return false;

	/* attributes count */

	attributes_count = suck_u2(cb);

	for (i = 0; i < attributes_count; i++) {
		if (!suck_check_classbuffer_size(cb, 2))
			return false;

		/* attribute name index */

		attribute_name_index = suck_u2(cb);

		attribute_name =
			class_getconstant(c, attribute_name_index, CONSTANT_Utf8);

		if (attribute_name == NULL)
			return false;

		if (attribute_name == utf_Code) {
			/* Code */

			if (m->flags & (ACC_ABSTRACT | ACC_NATIVE)) {
				exceptions_throw_classformaterror(c, "Code attribute in native or abstract methods");
				return false;
			}
			
			if (m->jcode) {
				exceptions_throw_classformaterror(c, "Multiple Code attributes");
				return false;
			}

			if (!suck_check_classbuffer_size(cb, 4 + 2 + 2))
				return false;

			suck_u4(cb);
			m->maxstack = suck_u2(cb);
			m->maxlocals = suck_u2(cb);

			if (m->maxlocals < argcount) {
				exceptions_throw_classformaterror(c, "Arguments can't fit into locals");
				return false;
			}
			
			if (!suck_check_classbuffer_size(cb, 4))
				return false;

			m->jcodelength = suck_u4(cb);

			if (m->jcodelength == 0) {
				exceptions_throw_classformaterror(c, "Code of a method has length 0");
				return false;
			}
			
			if (m->jcodelength > 65535) {
				exceptions_throw_classformaterror(c, "Code of a method longer than 65535 bytes");
				return false;
			}

			if (!suck_check_classbuffer_size(cb, m->jcodelength))
				return false;

			m->jcode = MNEW(u1, m->jcodelength);
			suck_nbytes(m->jcode, cb, m->jcodelength);

			if (!suck_check_classbuffer_size(cb, 2))
				return false;

			m->rawexceptiontablelength = suck_u2(cb);
			if (!suck_check_classbuffer_size(cb, (2 + 2 + 2 + 2) * m->rawexceptiontablelength))
				return false;

			m->rawexceptiontable = MNEW(raw_exception_entry, m->rawexceptiontablelength);

#if defined(ENABLE_STATISTICS)
			if (opt_stat) {
				count_vmcode_len += m->jcodelength + 18;
				count_extable_len +=
					m->rawexceptiontablelength * sizeof(raw_exception_entry);
			}
#endif

			for (j = 0; j < m->rawexceptiontablelength; j++) {
				u4 idx;
				m->rawexceptiontable[j].startpc   = suck_u2(cb);
				m->rawexceptiontable[j].endpc     = suck_u2(cb);
				m->rawexceptiontable[j].handlerpc = suck_u2(cb);

				idx = suck_u2(cb);

				if (!idx) {
					m->rawexceptiontable[j].catchtype.any = NULL;
				}
				else {
					/* the classref is created later */
					if (!(m->rawexceptiontable[j].catchtype.any =
						  (utf *) class_getconstant(c, idx, CONSTANT_Class)))
						return false;
				}
			}

			if (!suck_check_classbuffer_size(cb, 2))
				return false;

			/* code attributes count */

			code_attributes_count = suck_u2(cb);

			for (k = 0; k < code_attributes_count; k++) {
				if (!suck_check_classbuffer_size(cb, 2))
					return false;

				/* code attribute name index */

				code_attribute_name_index = suck_u2(cb);

				code_attribute_name =
					class_getconstant(c, code_attribute_name_index, CONSTANT_Utf8);

				if (code_attribute_name == NULL)
					return false;

				/* check which code attribute */

				if (code_attribute_name == utf_LineNumberTable) {
					/* LineNumberTable */

					if (!suck_check_classbuffer_size(cb, 4 + 2))
						return false;

					/* attribute length */

					(void) suck_u4(cb);

					/* line number table length */

					m->linenumbercount = suck_u2(cb);

					if (!suck_check_classbuffer_size(cb,
												(2 + 2) * m->linenumbercount))
						return false;

					m->linenumbers = MNEW(lineinfo, m->linenumbercount);

#if defined(ENABLE_STATISTICS)
					if (opt_stat)
						size_lineinfo += sizeof(lineinfo) * m->linenumbercount;
#endif
					
					for (l = 0; l < m->linenumbercount; l++) {
						m->linenumbers[l].start_pc    = suck_u2(cb);
						m->linenumbers[l].line_number = suck_u2(cb);
					}
				}
#if defined(ENABLE_JAVASE)
				else if (code_attribute_name == utf_StackMapTable) {
					/* StackTableMap */

					if (!stackmap_load_attribute_stackmaptable(cb, m))
						return false;
				}
#endif
				else {
					/* unknown code attribute */

					if (!loader_skip_attribute_body(cb))
						return false;
				}
			}
		}
		else if (attribute_name == utf_Exceptions) {
			/* Exceptions */

			if (m->thrownexceptions != NULL) {
				exceptions_throw_classformaterror(c, "Multiple Exceptions attributes");
				return false;
			}

			if (!suck_check_classbuffer_size(cb, 4 + 2))
				return false;

			/* attribute length */

			(void) suck_u4(cb);

			m->thrownexceptionscount = suck_u2(cb);

			if (!suck_check_classbuffer_size(cb, 2 * m->thrownexceptionscount))
				return false;

			m->thrownexceptions = MNEW(classref_or_classinfo, m->thrownexceptionscount);

			for (j = 0; j < m->thrownexceptionscount; j++) {
				/* the classref is created later */
				if (!((m->thrownexceptions)[j].any =
					  (utf*) class_getconstant(c, suck_u2(cb), CONSTANT_Class)))
					return false;
			}
		}
#if defined(ENABLE_JAVASE)
		else if (attribute_name == utf_Signature) {
			/* Signature */

			if (!loader_load_attribute_signature(cb, &(m->signature)))
				return false;
		}

#if defined(ENABLE_ANNOTATIONS)
		else if (attribute_name == utf_RuntimeVisibleAnnotations) {
			/* RuntimeVisibleAnnotations */
			if (!annotation_load_method_attribute_runtimevisibleannotations(cb, m))
				return false;
		}
		else if (attribute_name == utf_RuntimeInvisibleAnnotations) {
			/* RuntimeInvisibleAnnotations */
			if (!annotation_load_method_attribute_runtimeinvisibleannotations(cb, m))
				return false;
		}
		else if (attribute_name == utf_RuntimeVisibleParameterAnnotations) {
			/* RuntimeVisibleParameterAnnotations */
			if (!annotation_load_method_attribute_runtimevisibleparameterannotations(cb, m))
				return false;
		}
		else if (attribute_name == utf_RuntimeInvisibleParameterAnnotations) {
			/* RuntimeInvisibleParameterAnnotations */
			if (!annotation_load_method_attribute_runtimeinvisibleparameterannotations(cb, m))
				return false;
		}
		else if (attribute_name == utf_AnnotationDefault) {
			/* AnnotationDefault */
			if (!annotation_load_method_attribute_annotationdefault(cb, m))
				return false;
		}
#endif
#endif
		else {
			/* unknown attribute */

			if (!loader_skip_attribute_body(cb))
				return false;
		}
	}

	if ((m->jcode == NULL) && !(m->flags & (ACC_ABSTRACT | ACC_NATIVE))) {
		exceptions_throw_classformaterror(c, "Missing Code attribute");
		return false;
	}

#if defined(ENABLE_REPLACEMENT)
	/* initialize the hit countdown field */

	m->hitcountdown = METHOD_INITIAL_HIT_COUNTDOWN;
#endif

	/* everything was ok */

	return true;
}


/* method_free *****************************************************************

   Frees all memory that was allocated for this method.

*******************************************************************************/

void method_free(methodinfo *m)
{
	if (m->jcode)
		MFREE(m->jcode, u1, m->jcodelength);

	if (m->rawexceptiontable)
		MFREE(m->rawexceptiontable, raw_exception_entry, m->rawexceptiontablelength);

	code_free_code_of_method(m);

	if (m->stubroutine) {
		if (m->flags & ACC_NATIVE) {
			removenativestub(m->stubroutine);

		} else {
			removecompilerstub(m->stubroutine);
		}
	}
}


/* method_canoverwrite *********************************************************

   Check if m and old are identical with respect to type and
   name. This means that old can be overwritten with m.
	
*******************************************************************************/

bool method_canoverwrite(methodinfo *m, methodinfo *old)
{
	if (m->name != old->name)
		return false;

	if (m->descriptor != old->descriptor)
		return false;

	if (m->flags & ACC_STATIC)
		return false;

	return true;
}


/* method_new_builtin **********************************************************

   Creates a minimal methodinfo structure for builtins. This comes handy
   when dealing with builtin stubs or stacktraces.

*******************************************************************************/

methodinfo *method_new_builtin(builtintable_entry *bte)
{
	methodinfo *m;

	/* allocate the methodinfo structure */

	m = NEW(methodinfo);

	/* initialize methodinfo structure */

	MZERO(m, methodinfo, 1);
	LOCK_INIT_OBJECT_LOCK(&(m->header));

	m->flags      = ACC_METHOD_BUILTIN;
	m->parseddesc = bte->md;
	m->name       = bte->name;
	m->descriptor = bte->descriptor;

	/* return the newly created methodinfo */

	return m;
}


/* method_vftbl_lookup *********************************************************

   Does a method lookup in the passed virtual function table.  This
   function does exactly the same thing as JIT, but additionally
   relies on the fact, that the methodinfo pointer is at the first
   data segment slot (even for compiler stubs).

*******************************************************************************/

methodinfo *method_vftbl_lookup(vftbl_t *vftbl, methodinfo* m)
{
	methodptr   mptr;
	methodptr  *pmptr;
	methodinfo *resm;                   /* pointer to new resolved method     */

	/* If the method is not an instance method, just return it. */

	if (m->flags & ACC_STATIC)
		return m;

	assert(vftbl);

	/* Get the method from the virtual function table.  Is this an
	   interface method? */

	if (m->clazz->flags & ACC_INTERFACE) {
		pmptr = vftbl->interfacetable[-(m->clazz->index)];
		mptr  = pmptr[(m - m->clazz->methods)];
	}
	else {
		mptr = vftbl->table[m->vftblindex];
	}

	/* and now get the codeinfo pointer from the first data segment slot */

	resm = code_get_methodinfo_for_pv(mptr);

	return resm;
}


/* method_get_parametercount **************************************************

   Use the descriptor of a method to determine the number of parameters
   of the method. The this pointer of non-static methods is not counted.

   IN:
       m........the method of which the parameters should be counted

   RETURN VALUE:
       The parameter count or -1 on error.

*******************************************************************************/

int32_t method_get_parametercount(methodinfo *m)
{
	methoddesc *md;             /* method descriptor of m   */
	int32_t     paramcount = 0; /* the parameter count of m */

	md = m->parseddesc;
	
	/* is the descriptor fully parsed? */

	if (md->params == NULL) {
		if (!descriptor_params_from_paramtypes(md, m->flags)) {
			return -1;
		}
	}

	paramcount = md->paramcount;

	/* skip `this' pointer */

	if (!(m->flags & ACC_STATIC)) {
		--paramcount;
	}

	return paramcount;
}


/* method_get_parametertypearray ***********************************************

   Use the descriptor of a method to generate a java.lang.Class array
   which contains the classes of the parametertypes of the method.

   This function is called by java.lang.reflect.{Constructor,Method}.

*******************************************************************************/

java_handle_objectarray_t *method_get_parametertypearray(methodinfo *m)
{
	methoddesc                *md;
	typedesc                  *paramtypes;
	int32_t                    paramcount;
	java_handle_objectarray_t *oa;
	int32_t                    i;
	classinfo                 *c;

	md = m->parseddesc;

	/* is the descriptor fully parsed? */

	if (m->parseddesc->params == NULL)
		if (!descriptor_params_from_paramtypes(md, m->flags))
			return NULL;

	paramtypes = md->paramtypes;
	paramcount = md->paramcount;

	/* skip `this' pointer */

	if (!(m->flags & ACC_STATIC)) {
		paramtypes++;
		paramcount--;
	}

	/* create class-array */

	oa = builtin_anewarray(paramcount, class_java_lang_Class);

	if (oa == NULL)
		return NULL;

    /* get classes */

	for (i = 0; i < paramcount; i++) {
		if (!resolve_class_from_typedesc(&paramtypes[i], true, false, &c))
			return NULL;

		LLNI_array_direct(oa, i) = (java_object_t *) c;
	}

	return oa;
}


/* method_get_exceptionarray ***************************************************

   Get the exceptions which can be thrown by a method.

*******************************************************************************/

java_handle_objectarray_t *method_get_exceptionarray(methodinfo *m)
{
	java_handle_objectarray_t *oa;
	classinfo                 *c;
	s4                         i;

	/* create class-array */

	oa = builtin_anewarray(m->thrownexceptionscount, class_java_lang_Class);

	if (oa == NULL)
		return NULL;

	/* iterate over all exceptions and store the class in the array */

	for (i = 0; i < m->thrownexceptionscount; i++) {
		c = resolve_classref_or_classinfo_eager(m->thrownexceptions[i], true);

		if (c == NULL)
			return NULL;

		LLNI_array_direct(oa, i) = (java_object_t *) c;
	}

	return oa;
}


/* method_returntype_get *******************************************************

   Get the return type of the method.

*******************************************************************************/

classinfo *method_returntype_get(methodinfo *m)
{
	typedesc  *td;
	classinfo *c;

	td = &(m->parseddesc->returntype);

	if (!resolve_class_from_typedesc(td, true, false, &c))
		return NULL;

	return c;
}


/* method_count_implementations ************************************************

   Count the implementations of a method in a class cone (a class and all its
   subclasses.)

   IN:
       m................the method to count
	   c................class at which to start the counting (this class and
	                    all its subclasses will be searched)

   OUT:
       *found...........if found != NULL, *found receives the method
	                    implementation that was found. This value is only
						meaningful if the return value is 1.

   RETURN VALUE:
       the number of implementations found

*******************************************************************************/

s4 method_count_implementations(methodinfo *m, classinfo *c, methodinfo **found)
{
	s4          count;
	methodinfo *mp;
	methodinfo *mend;
	classinfo  *child;

	count = 0;

	mp = c->methods;
	mend = mp + c->methodscount;

	for (; mp < mend; ++mp) {
		if (method_canoverwrite(mp, m)) {
			if (found)
				*found = mp;
			count++;
			break;
		}
	}

	for (child = c->sub; child != NULL; child = child->nextsub) {
		count += method_count_implementations(m, child, found);
	}

	return count;
}


/* method_get_annotations ******************************************************

   Get a methods' unparsed annotations in a byte array.

   IN:
       m........the method of which the annotations should be returned

   RETURN VALUE:
       The unparsed annotations in a byte array (or NULL if there aren't any).

*******************************************************************************/

java_handle_bytearray_t *method_get_annotations(methodinfo *m)
{
#if defined(ENABLE_ANNOTATIONS)
	classinfo     *c;                  /* methods' declaring class          */
	int            slot;               /* methods' slot                     */
	java_handle_t *annotations;        /* methods' unparsed annotations     */
	java_handle_t *method_annotations; /* all methods' unparsed annotations */
	                                   /* of the declaring class            */

	c           = m->clazz;
	slot        = m - c->methods;
	annotations = NULL;

	LLNI_classinfo_field_get(c, method_annotations, method_annotations);

	/* the method_annotations array might be shorter then the method
	 * count if the methods above a certain index have no annotations.
	 */	
	if (method_annotations != NULL &&
		array_length_get(method_annotations) > slot) {
		annotations = array_objectarray_element_get(
			(java_handle_objectarray_t*)method_annotations, slot);
	}
	
	return (java_handle_bytearray_t*)annotations;
#else
	return NULL;
#endif
}


/* method_get_parameterannotations ********************************************

   Get a methods' unparsed parameter annotations in an array of byte
   arrays.

   IN:
       m........the method of which the parameter annotations should be
	            returned

   RETURN VALUE:
       The unparsed parameter annotations in a byte array (or NULL if
	   there aren't any).

*******************************************************************************/

java_handle_bytearray_t *method_get_parameterannotations(methodinfo *m)
{
#if defined(ENABLE_ANNOTATIONS)
	classinfo     *c;                           /* methods' declaring class */
	int            slot;                        /* methods' slot            */
	java_handle_t *parameterAnnotations;        /* methods' unparsed        */
	                                            /* parameter annotations    */
	java_handle_t *method_parameterannotations; /* all methods' unparsed    */
	                                            /* parameter annotations of */
	                                            /* the declaring class      */

	c                    = m->clazz;
	slot                 = m - c->methods;
	parameterAnnotations = NULL;

	LLNI_classinfo_field_get(
		c, method_parameterannotations, method_parameterannotations);

	/* the method_annotations array might be shorter then the method
	 * count if the methods above a certain index have no annotations.
	 */	
	if (method_parameterannotations != NULL &&
		array_length_get(method_parameterannotations) > slot) {
		parameterAnnotations = array_objectarray_element_get(
				(java_handle_objectarray_t*)method_parameterannotations,
				slot);
	}
	
	return (java_handle_bytearray_t*)parameterAnnotations;
#else
	return NULL;
#endif
}


/* method_get_annotationdefault ***********************************************

   Get a methods' unparsed annotation default value in a byte array.
   
   IN:
       m........the method of which the annotation default value should be
	            returned

   RETURN VALUE:
       The unparsed annotation default value in a byte array (or NULL if
	   there isn't one).

*******************************************************************************/

java_handle_bytearray_t *method_get_annotationdefault(methodinfo *m)
{
#if defined(ENABLE_ANNOTATIONS)
	classinfo     *c;                         /* methods' declaring class     */
	int            slot;                      /* methods' slot                */
	java_handle_t *annotationDefault;         /* methods' unparsed            */
	                                          /* annotation default value     */
	java_handle_t *method_annotationdefaults; /* all methods' unparsed        */
	                                          /* annotation default values of */
	                                          /* the declaring class          */

	c                 = m->clazz;
	slot              = m - c->methods;
	annotationDefault = NULL;

	LLNI_classinfo_field_get(
		c, method_annotationdefaults, method_annotationdefaults);

	/* the method_annotations array might be shorter then the method
	 * count if the methods above a certain index have no annotations.
	 */	
	if (method_annotationdefaults != NULL &&
		array_length_get(method_annotationdefaults) > slot) {
		annotationDefault = array_objectarray_element_get(
				(java_handle_objectarray_t*)method_annotationdefaults, slot);
	}
	
	return (java_handle_bytearray_t*)annotationDefault;
#else
	return NULL;
#endif
}


/* method_add_to_worklist ******************************************************

   Add the method to the given worklist. If the method already occurs in
   the worklist, the worklist remains unchanged.

*******************************************************************************/

static void method_add_to_worklist(methodinfo *m, method_worklist **wl)
{
	method_worklist *wi;

	for (wi = *wl; wi != NULL; wi = wi->next)
		if (wi->m == m)
			return;

	wi = NEW(method_worklist);
	wi->next = *wl;
	wi->m = m;

	*wl = wi;
}


/* method_add_assumption_monomorphic *******************************************

   Record the assumption that the method is monomorphic.

   IN:
      m.................the method
	  caller............the caller making the assumption

*******************************************************************************/

void method_add_assumption_monomorphic(methodinfo *m, methodinfo *caller)
{
	method_assumption *as;

	/* XXX LOCKING FOR THIS FUNCTION? */

	/* check if we already have registered this assumption */

	for (as = m->assumptions; as != NULL; as = as->next) {
		if (as->context == caller)
			return;
	}

	/* register the assumption */

	as = NEW(method_assumption);
	as->next = m->assumptions;
	as->context = caller;

	m->assumptions = as;
}

/* method_break_assumption_monomorphic *****************************************

   Break the assumption that this method is monomorphic. All callers that
   have registered this assumption are added to the worklist.

   IN:
      m.................the method
	  wl................worklist where to add invalidated callers

*******************************************************************************/

void method_break_assumption_monomorphic(methodinfo *m, method_worklist **wl)
{
	method_assumption *as;

	/* XXX LOCKING FOR THIS FUNCTION? */

	for (as = m->assumptions; as != NULL; as = as->next) {
		INLINELOG(
			printf("ASSUMPTION BROKEN (monomorphism): ");
			method_print(m);
			printf(" in ");
			method_println(as->context);
		);

		method_add_to_worklist(as->context, wl);

#if defined(ENABLE_TLH) && 0
		/* XXX hack */
		method_assumption *as2;
		as2 = m->assumptions;
		m->assumptions = NULL;
		method_break_assumption_monomorphic(as->context, wl);
		/*
		assert(m->assumptions == NULL);
		m->assumptions = as2;*/
#endif

	}
}

/* method_printflags ***********************************************************

   Prints the flags of a method to stdout like.

*******************************************************************************/

#if !defined(NDEBUG)
void method_printflags(methodinfo *m)
{
	if (m == NULL) {
		printf("NULL");
		return;
	}

	if (m->flags & ACC_PUBLIC)             printf(" PUBLIC");
	if (m->flags & ACC_PRIVATE)            printf(" PRIVATE");
	if (m->flags & ACC_PROTECTED)          printf(" PROTECTED");
	if (m->flags & ACC_STATIC)             printf(" STATIC");
	if (m->flags & ACC_FINAL)              printf(" FINAL");
	if (m->flags & ACC_SYNCHRONIZED)       printf(" SYNCHRONIZED");
	if (m->flags & ACC_VOLATILE)           printf(" VOLATILE");
	if (m->flags & ACC_TRANSIENT)          printf(" TRANSIENT");
	if (m->flags & ACC_NATIVE)             printf(" NATIVE");
	if (m->flags & ACC_INTERFACE)          printf(" INTERFACE");
	if (m->flags & ACC_ABSTRACT)           printf(" ABSTRACT");
	if (m->flags & ACC_METHOD_BUILTIN)     printf(" (builtin)");
	if (m->flags & ACC_METHOD_MONOMORPHIC) printf(" (mono)");
	if (m->flags & ACC_METHOD_IMPLEMENTED) printf(" (impl)");
}
#endif /* !defined(NDEBUG) */


/* method_print ****************************************************************

   Prints a method to stdout like:

   java.lang.Object.<init>()V

*******************************************************************************/

#if !defined(NDEBUG)
void method_print(methodinfo *m)
{
	if (m == NULL) {
		printf("NULL");
		return;
	}

	if (m->clazz != NULL)
		utf_display_printable_ascii_classname(m->clazz->name);
	else
		printf("NULL");
	printf(".");
	utf_display_printable_ascii(m->name);
	utf_display_printable_ascii(m->descriptor);

	method_printflags(m);
}
#endif /* !defined(NDEBUG) */


/* method_println **************************************************************

   Prints a method plus new line to stdout like:

   java.lang.Object.<init>()V

*******************************************************************************/

#if !defined(NDEBUG)
void method_println(methodinfo *m)
{
	if (opt_debugcolor) printf("\033[31m");	/* red */
	method_print(m);
	if (opt_debugcolor) printf("\033[m");	
	printf("\n");
}
#endif /* !defined(NDEBUG) */


/* method_methodref_print ******************************************************

   Prints a method reference to stdout.

*******************************************************************************/

#if !defined(NDEBUG)
void method_methodref_print(constant_FMIref *mr)
{
	if (!mr) {
		printf("(constant_FMIref *)NULL");
		return;
	}

	if (IS_FMIREF_RESOLVED(mr)) {
		printf("<method> ");
		method_print(mr->p.method);
	}
	else {
		printf("<methodref> ");
		utf_display_printable_ascii_classname(mr->p.classref->name);
		printf(".");
		utf_display_printable_ascii(mr->name);
		utf_display_printable_ascii(mr->descriptor);
	}
}
#endif /* !defined(NDEBUG) */


/* method_methodref_println ****************************************************

   Prints a method reference to stdout, followed by a newline.

*******************************************************************************/

#if !defined(NDEBUG)
void method_methodref_println(constant_FMIref *mr)
{
	method_methodref_print(mr);
	printf("\n");
}
#endif /* !defined(NDEBUG) */


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
