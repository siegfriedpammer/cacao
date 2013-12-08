/* src/vm/references.hpp - references to classes/fields/methods

   Copyright (C) 1996-2013
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

#ifndef REFERENCES_HPP_
#define REFERENCES_HPP_ 1

#include "config.h"

#include "vm/global.hpp"
#include "vm/types.hpp"
#include "vm/utf8.hpp"

struct classinfo;
struct constant_classref;
struct constant_FMIref;
struct typedesc;
struct methoddesc;
struct methodinfo;
struct fieldinfo;

/* constant_classref **********************************************************/

/// a value that never occurrs in classinfo.header.vftbl
#define CLASSREF_PSEUDO_VFTBL (reinterpret_cast<void *>(1))

struct constant_classref {
	void       *pseudo_vftbl;     /* for distinguishing it from classinfo     */
	classinfo  *referer;          /* class containing the reference           */
	Utf8String  name;             /* name of the class refered to             */

	void init(classinfo *referer, Utf8String name) {
		this->pseudo_vftbl = CLASSREF_PSEUDO_VFTBL;
		this->referer      = referer;
		this->name         = name;
	}
};


/* classref_or_classinfo ******************************************************/

/**
 * @Cpp11 Replace static constructors with regular constructors.
 *        With C++98 this breaks other unions that use classref_or_classinfo.
 */
union classref_or_classinfo {
	constant_classref *ref;       /* a symbolic class reference               */
	classinfo         *cls;       /* an already loaded class                  */
	void              *any;       /* used for general access (x != NULL,...)  */

	bool is_classref()  const { return ref->pseudo_vftbl == CLASSREF_PSEUDO_VFTBL; }
	bool is_classinfo() const { return !is_classref(); }
};


/* parseddesc_t ***************************************************************/

union parseddesc_t {
	typedesc   *fd;               /* parsed field descriptor                  */
	methoddesc *md;               /* parsed method descriptor                 */
	void       *any;              /* used for simple test against NULL        */
};


/*----------------------------------------------------------------------------*/
/* References                                                                 */
/*                                                                            */
/* This header files defines the following types used for references to       */
/* classes/methods/fields and descriptors:                                    */
/*                                                                            */
/*     classinfo *                a loaded class                              */
/*     constant_classref          a symbolic reference                        */
/*     classref_or_classinfo      a loaded class or a symbolic reference      */
/*                                                                            */
/*     constant_FMIref            a symb. ref. to a field/method/intf.method  */
/*                                                                            */
/*     typedesc *                 describes a field type                      */
/*     methoddesc *               descrives a method type                     */
/*     parseddesc                 describes a field type or a method type     */
/*----------------------------------------------------------------------------*/

/* structs ********************************************************************/

/* constant_FMIref ************************************************************/

struct constant_FMIref{      /* Fieldref, Methodref and InterfaceMethodref    */
	union {
		s4                 index;     /* used only within the loader          */
		constant_classref *classref;  /* class having this field/meth./intfm. */
		fieldinfo         *field;     /* resolved field                       */
		methodinfo        *method;    /* resolved method                      */
	} p;
	Utf8String   name;       /* field/method/interfacemethod name             */
	Utf8String   descriptor; /* field/method/intfmeth. type descriptor string */
	parseddesc_t parseddesc; /* parsed descriptor                             */

	bool is_resolved() const { return p.classref->pseudo_vftbl != CLASSREF_PSEUDO_VFTBL; }
};


/* inline functions ***********************************************************/

/**
 * Functions for casting a classref/classinfo to a classref_or_classinfo
 * @Cpp11 Replace with constructors in classref_or_classinfo.
 *        With C++98 this breaks other unions that use classref_or_classinfo.
 * @{
 */ 
static inline classref_or_classinfo to_classref_or_classinfo(classinfo *c) {
	classref_or_classinfo out;
	out.cls = c;
	return out;
}
static inline classref_or_classinfo to_classref_or_classinfo(constant_classref *c) {
	classref_or_classinfo out;
	out.ref = c;
	return out;
}
/**
 * @}
 */


/* macros *********************************************************************/

/* macro for accessing the name of a classref/classinfo                       */
#define CLASSREF_OR_CLASSINFO_NAME(value) \
	((value).is_classref() ? (value).ref->name : (value).cls->name)

/* macro for accessing the class name of a method reference                   */
#define METHODREF_CLASSNAME(fmiref) \
	((fmiref)->is_resolved() ? (fmiref)->p.method->clazz->name \
								: (fmiref)->p.classref->name)

/* macro for accessing the class name of a method reference                   */
#define FIELDREF_CLASSNAME(fmiref) \
	((fmiref)->is_resolved() ? (fmiref)->p.field->clazz->name \
								: (fmiref)->p.classref->name)

#endif // REFERENCES_HPP_

/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */

